/********************************************************************************************************
 * @file     app_uart.c 
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     May. 12, 2018
 *
 * @par      Copyright (c) Telink Semiconductor (Shanghai) Co., Ltd.
 *           All rights reserved.
 *           
 *			 The information contained herein is confidential and proprietary property of Telink 
 * 		     Semiconductor (Shanghai) Co., Ltd. and is available under the terms 
 *			 of Commercial License Agreement between Telink Semiconductor (Shanghai) 
 *			 Co., Ltd. and the licensee in separate contract or the terms described here-in. 
 *           This heading MUST NOT be removed from this file.
 *
 * 			 Licensees are granted free, non-transferable use of the information in this 
 *			 file under Mutual Non-Disclosure Agreement. NO WARRENTY of ANY KIND is provided. 
 *           
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "at_cmd.h"

#define UART_DATA_LEN    12+256     //data max ?    (UART_DATA_LEN+4) must 16 byte aligned

typedef struct{
	unsigned int dma_len;        // dma len must be 4 byte
	unsigned char data[UART_DATA_LEN];
}uart_data_t;

uart_data_t trans_buff = {0, {0,} };

#define UART_RXFIFO_NUM			8

_attribute_data_retention_  u8 		 	uart_rx_fifo_b[UART_DATA_LEN * UART_RXFIFO_NUM] = {0};
_attribute_data_retention_	my_fifo_t	uart_rx_fifo = {
												UART_DATA_LEN,
												UART_RXFIFO_NUM,
												0,
												0,
												uart_rx_fifo_b,};

// _attribute_data_retention_  u8 		 	ble_rx_fifo_b[UART_RXFIFO_SIZE * UART_RXFIFO_NUM] = {0};
// _attribute_data_retention_	my_fifo_t	ble_rx_fifo = {
// 												UART_RXFIFO_SIZE,
// 												UART_RXFIFO_NUM,
// 												0,
// 												0,
// 												ble_rx_fifo_b,};

_attribute_data_retention_  u8 baud_buf[1] = { 6 };
_attribute_data_retention_  u8 ATE = 0;

_attribute_data_retention_  char at_print_buf[256] = { 0 };

typedef enum {
    AT_BAUD_2400 = 0,
	AT_BAUD_4800,
	AT_BAUD_9600,
	AT_BAUD_19200,
	AT_BAUD_38400,
	AT_BAUD_57600,
	AT_BAUD_115200,
	AT_BAUD_230400,
	AT_BAUD_460800,
	AT_BAUD_921600,
} AT_BAUD;

void app_uart_init(AT_BAUD baud)
{
	//WaitMs(100);  //leave enough time for SWS_reset when power on

	//note: dma addr must be set first before any other uart initialization! (confirmed by sihui)
	uart_recbuff_init( (unsigned short *)my_fifo_wptr(&uart_rx_fifo), UART_DATA_LEN);

	uart_gpio_set(UART_TX_PB1, UART_RX_PIN);// uart tx/rx pin set

	uart_reset();  //will reset uart digital registers from 0x90 ~ 0x9f, so uart setting must set after this reset

	switch (baud)
	{
		case AT_BAUD_2400  : uart_init(999, 9, PARITY_NONE, STOP_BIT_ONE); break;
		case AT_BAUD_4800  : uart_init(499, 9, PARITY_NONE, STOP_BIT_ONE); break;
		case AT_BAUD_9600  : uart_init(249, 9, PARITY_NONE, STOP_BIT_ONE); break;
		case AT_BAUD_19200 : uart_init(124, 9, PARITY_NONE, STOP_BIT_ONE); break;
		case AT_BAUD_38400 : uart_init(61,  9, PARITY_NONE, STOP_BIT_ONE); break;
		case AT_BAUD_57600 : uart_init(25, 15, PARITY_NONE, STOP_BIT_ONE); break;
		case AT_BAUD_115200: uart_init(12, 15, PARITY_NONE, STOP_BIT_ONE); break;
		case AT_BAUD_230400: uart_init(7,  12, PARITY_NONE, STOP_BIT_ONE); break;
		case AT_BAUD_460800: uart_init(3,  12, PARITY_NONE, STOP_BIT_ONE); break;
		case AT_BAUD_921600: uart_init(1,  12, PARITY_NONE, STOP_BIT_ONE); break;

		default : break;
	};
	 	

	uart_dma_enable(1, 1); 	//uart data in hardware buffer moved by dma, so we need enable them first

	irq_set_mask(FLD_IRQ_DMA_EN);

	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);   	//uart Rx/Tx dma irq enable

	uart_irq_enable(0, 0);  	//uart Rx/Tx irq no need, disable them

	irq_enable();
}
/* GPIO初始化
CONTROL_GPIO默认上拉，如果需要关闭透传模式，将其下拉即可。 */
void my_gpio_init(void)
{
	
	gpio_set_func(CONTROL_GPIO, AS_GPIO);

	gpio_setup_up_down_resistor(CONTROL_GPIO, PM_PIN_PULLUP_10K);
//	gpio_setup_up_down_resistor(CONTROL_GPIO, PM_PIN_PULLDOWN_100K);//关闭透传模式

	gpio_set_output_en(CONTROL_GPIO, 0);//enable output

	gpio_set_input_en(CONTROL_GPIO, 1); //disable input

	gpio_set_func(CONN_STATE_GPIO, AS_GPIO);
	gpio_set_output_en(CONN_STATE_GPIO, 1);//enable output
	gpio_set_input_en(CONN_STATE_GPIO, 0); //disable input
	gpio_setup_up_down_resistor(CONN_STATE_GPIO, PM_PIN_UP_DOWN_FLOAT); 
	gpio_write(CONN_STATE_GPIO, 0);
	
	gpio_set_func(LOWPWR_STATE_GPIO, AS_GPIO);
	gpio_set_output_en(LOWPWR_STATE_GPIO, 1);//enable output
	gpio_set_input_en(LOWPWR_STATE_GPIO, 0); //disable input
	gpio_setup_up_down_resistor(LOWPWR_STATE_GPIO, PM_PIN_UP_DOWN_FLOAT); 
	gpio_write(LOWPWR_STATE_GPIO, 0);
}

void at_print(char * str)
{
	while(*str)
	{
		trans_buff.data[trans_buff.dma_len] = *str++;
		trans_buff.dma_len += 1;
		if(trans_buff.dma_len == 12)
		{
			uart_dma_send((unsigned char*)&trans_buff);
			trans_buff.dma_len = 0;
			WaitMs(20);
		}
	}

	if(trans_buff.dma_len)
	{
		uart_dma_send((unsigned char*)&trans_buff);
		trans_buff.dma_len = 0;
		WaitMs(20);
	}
}
const unsigned char hextab[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
void at_print_hexstr(char * data, u32 len)
{
	unsigned char buf[128] = { 0 };
	for(int i =0; i < len; i ++)
	{
		buf[i*3] = hextab[(data[i] >> 4)];
		buf[i*3 +1] = hextab[(data[i]&0xf)];
		buf[i*3 +2] = ' ';
	}
	at_print((char*)buf);
}

void at_print_array(char * data, u32 len)
{
	unsigned char buf[128] = { 0 };
	for(int i =0; i < len; i ++)
	{
		buf[i*2] = hextab[(data[i] >> 4)];
		buf[i*2 +1] = hextab[(data[i]&0xf)];
	}
	at_print((char*)buf);
}

void at_send(char * data, u32 len)
{
	while(len > UART_DATA_LEN)//如果超过串口传输最大长度
	{
		memcpy(trans_buff.data, data,  UART_DATA_LEN);
		data += UART_DATA_LEN;
		len -= UART_DATA_LEN;

		trans_buff.dma_len = UART_DATA_LEN;

		while(uart_tx_is_busy());
		uart_dma_send((unsigned char*)&trans_buff);//输出到串口
		trans_buff.dma_len = 0;
		
	}

	if(len > 0)
	{
		memcpy(trans_buff.data, data,  len);
		trans_buff.dma_len = len;
		while(uart_tx_is_busy());
		uart_dma_send((unsigned char*)&trans_buff);//输出到串口
		trans_buff.dma_len = 0;
	}
}

extern u32 device_in_connection_state;

void app_uart_irq_proc(void)
{
	unsigned char uart_dma_irqsrc;
	//1. UART irq
	uart_dma_irqsrc = dma_chn_irq_status_get();///in function,interrupt flag have already been cleared,so need not to clear DMA interrupt flag here

	if(uart_dma_irqsrc & FLD_DMA_CHN_UART_RX)
	{
		//Received uart data in rec_buff, user can copy data here
		// u_sprintf(print_buff,"%d", rec_buff.dma_len);
		//at_print("uart data\r\n");

		u8* w = my_fifo_wptr(&uart_rx_fifo);
		if((w[0]!=0) || (w[1]!=0))
		{
			my_fifo_next(&uart_rx_fifo); //写指针前移
			u8* p = my_fifo_wptr(&uart_rx_fifo); //获取当前写指针
			reg_dma_uart_rx_addr = (u16)((u32)p);  //switch uart RX dma address
		}

		dma_chn_irq_status_clr(FLD_DMA_CHN_UART_RX);
	}

	if(uart_dma_irqsrc & FLD_DMA_CHN_UART_TX)
	{
		dma_chn_irq_status_clr(FLD_DMA_CHN_UART_TX);
	}
}

u8 * data = NULL;
uart_data_t * p = NULL;
//用户层UART循环收发数据
void app_uart_loop()
{
	if(data = my_fifo_get(&uart_rx_fifo)) //从fifo中获取数据
	{
		p = (uart_data_t *)data;

		if((device_in_connection_state == 0) || ((gpio_read(CONTROL_GPIO) == 0))) //蓝牙未连接，或者PC5为低电平，响应AT指令
		{
			if(ATE)
			{
				while(uart_tx_is_busy());
				uart_dma_send((unsigned char*)p);
				sleep_us(20);
				while(uart_tx_is_busy());
			}

			at_data_process((char*)(p->data), p->dma_len);
		}
		else //透传模式且蓝牙已连接，所有数据通过BLE发送出去
		{
			bls_att_pushNotifyData(SPP_SERVER_TO_CLIENT_DP_H, p->data, p->dma_len); //release
		}
		my_fifo_pop(&uart_rx_fifo);
	}
}