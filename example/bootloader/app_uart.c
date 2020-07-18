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

#define BOOT_VERSION "V0.5"

volatile unsigned char uart_rx_flag=0;
volatile unsigned char uart_dmairq_tx_cnt=0;
volatile unsigned char uart_dmairq_rx_cnt=0;
volatile unsigned int  uart_ndmairq_cnt=0;
volatile unsigned char uart_ndmairq_index=0;

#define UART_DATA_LEN    12+256    //data max ?    (UART_DATA_LEN+4) must 16 byte aligned 256 + 16 
typedef struct{
	unsigned int dma_len;        // dma len must be 4 byte
	unsigned char data[UART_DATA_LEN];
}uart_data_t;

uart_data_t rec_buff = {0,  {0, } };
uart_data_t trans_buff = {0, {0,} };

void app_uart_init(int baud)
{
	//note: dma addr must be set first before any other uart initialization! (confirmed by sihui)
	uart_recbuff_init( (unsigned char *)&rec_buff, sizeof(rec_buff));

	GPIO_PinTypeDef UART_RX_PIN = 0;

	gpio_set_func(UART_RX_PA0, AS_GPIO);//TB-02/TB-03F/TB-04
	gpio_set_func(UART_RX_PB0, AS_GPIO);//TB-01
	gpio_set_func(UART_RX_PB7, AS_GPIO);//TB-02 Kit

	gpio_set_output_en(UART_RX_PA0, 0);
	gpio_set_output_en(UART_RX_PB0, 0);
	gpio_set_output_en(UART_RX_PB7, 0);

	gpio_set_input_en(UART_RX_PA0, 1); 
	gpio_set_input_en(UART_RX_PB0, 1); 
	gpio_set_input_en(UART_RX_PB7, 1); 

	gpio_setup_up_down_resistor(UART_RX_PA0, PM_PIN_PULLDOWN_100K);
	gpio_setup_up_down_resistor(UART_RX_PB0, PM_PIN_PULLDOWN_100K);
	gpio_setup_up_down_resistor(UART_RX_PB7, PM_PIN_PULLDOWN_100K);

	while(1)//检测UART Rx 引脚
	{
		if(gpio_read(UART_RX_PA0) != 0)
		{	
			UART_RX_PIN = UART_RX_PA0;
			break;
		}
		else if (gpio_read(UART_RX_PB0) != 0)
		{
			UART_RX_PIN = UART_RX_PB0;
			break;
		}
		else if (gpio_read(UART_RX_PB7) != 0)
		{
			UART_RX_PIN = UART_RX_PB7;
			break;
		}
	}

	gpio_setup_up_down_resistor(UART_RX_PA0, PM_PIN_UP_DOWN_FLOAT);
	gpio_setup_up_down_resistor(UART_RX_PB0, PM_PIN_UP_DOWN_FLOAT);
	gpio_setup_up_down_resistor(UART_RX_PB7, PM_PIN_UP_DOWN_FLOAT);

	uart_gpio_set(UART_TX_PB1, UART_RX_PIN);

	uart_reset();  //will reset uart digital registers from 0x90 ~ 0x9f, so uart setting must set after this reset

	//baud rate: 115200
	#if (CLOCK_SYS_CLOCK_HZ == 16000000)
//		uart_init(118, 13, PARITY_NONE, STOP_BIT_ONE);
		uart_init(9, 13, PARITY_NONE, STOP_BIT_ONE);
	#elif (CLOCK_SYS_CLOCK_HZ == 24000000)
		if(baud == 115200)
		{
			uart_init(12, 15, PARITY_NONE, STOP_BIT_ONE); //baudrate = 115200
		}
		else
		{
			uart_init(1, 12, PARITY_NONE, STOP_BIT_ONE); //baudrate = 921600
		}
		
	#endif

	uart_dma_enable(1, 1); 	//uart data in hardware buffer moved by dma, so we need enable them first

	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);   	//uart Rx/Tx dma irq enable

	uart_irq_enable(0, 0);  	//uart Rx/Tx irq no need, disable them

	//irq_enable();
}

void uart_print(char * str)
{
	while(uart_tx_is_busy());
	trans_buff.dma_len = 0;

	while(*str)
	{
		trans_buff.data[trans_buff.dma_len] = *str++;
		trans_buff.dma_len += 1;
	}

	uart_dma_send((unsigned char*)&trans_buff);
}

void uart_send(char * data, u32 len)
{
	while(len > UART_DATA_LEN)
	{
		memcpy(trans_buff.data, data,  UART_DATA_LEN);
		data += UART_DATA_LEN;
		len -= UART_DATA_LEN;

		trans_buff.dma_len = UART_DATA_LEN;

		uart_dma_send((unsigned char*)&trans_buff);
		trans_buff.dma_len = 0;
		WaitMs(2);
		
	}

	if(len > 0)
	{
		memcpy(trans_buff.data, data,  len);
		trans_buff.dma_len = len;
		uart_dma_send((unsigned char*)&trans_buff);
		trans_buff.dma_len = 0;
		WaitMs(2);
	}
}

enum{
	CMD_VRSN = 0x00,	//读取boot版本
	CMD_WRTE ,			//写Flash
	CMD_READ ,			//读Flash
	CMD_ERAS ,			//擦除Flash
	CMD_MUID ,			//读取MUID
	CMD_BAUD ,			//更改波特率
};


int flash_write(unsigned long addr, unsigned char *buf)
{
	flash_write_page(addr, 256, buf);
	return 0;
}
char buff[128] = { 0 };
unsigned long addr;
unsigned int flash_mid;
unsigned char flash_uid[16];
extern int flash_read_mid_uid_with_check( unsigned int *flash_mid ,unsigned char *flash_uid);
void app_uart_loop(void)
{

	if(rec_buff.dma_len > 0)
	{
		if(rec_buff.data[1] * 256 + rec_buff.data[2] == rec_buff.dma_len - 3)
		{
			switch (rec_buff.data[0])
			{
				case CMD_VRSN: //读取版本号
					sprintf(buff, BOOT_VERSION "\r\n");
					break;

				case CMD_WRTE: //写Flash
					addr = rec_buff.data[3];
					addr <<= 8;  addr += rec_buff.data[4];
					addr <<= 8;  addr += rec_buff.data[5];
					addr <<= 8;  addr += rec_buff.data[6];

					if((addr >= 0x4000) && (addr < 0x80000))
					{
						sprintf(buff, "OK_01\r\n");
						flash_write(addr, rec_buff.data + 8);
					}
					else
					{
						sprintf(buff, "FAIL\r\n");
					}
					break;

				case CMD_READ: //读Flash
					addr = rec_buff.data[3];
					addr <<= 8;  addr += rec_buff.data[4];
					addr <<= 8;  addr += rec_buff.data[5];
					addr <<= 8;  addr += rec_buff.data[6];

					rec_buff.dma_len = rec_buff.data[7];

					flash_read_page(addr, rec_buff.dma_len, rec_buff.data);

					uart_dma_send((unsigned char*)&rec_buff);
					WaitMs(10);
					rec_buff.dma_len = 0;

					sprintf(buff, "OK_02\r\n");
					break;

				case CMD_ERAS: //擦除Flash

					addr = rec_buff.data[3];
					addr <<= 8;  addr += rec_buff.data[4];
					addr <<= 8;  addr += rec_buff.data[5];
					addr <<= 8;  addr += rec_buff.data[6];
					
					if(((addr&0xFFF) == 0) && addr >= 0x4000)//擦除Flash
					{
						for(int a = 0; a < rec_buff.data[7]; a++)
						{
							flash_erase_sector(addr);
							addr += 0x1000;
						}
						sprintf(buff, "OK_03\r\n");
					}
					else
					{
						sprintf(buff, "Fail\r\n");
					}
					break;

				case CMD_MUID: //读取Flash ID

					flash_mid = 0;
					if(flash_read_mid_uid_with_check(&flash_mid ,flash_uid) == 1) //读取成功
					{
						sprintf(buff, "OK_04:%04x,%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\r\n", flash_mid ,
						flash_uid[0],flash_uid[1],flash_uid[2],flash_uid[3],flash_uid[4],flash_uid[5],flash_uid[6],flash_uid[7],
						flash_uid[8],flash_uid[9],flash_uid[10],flash_uid[11],flash_uid[12],flash_uid[13],flash_uid[14],flash_uid[15]);
					}else
					{
						sprintf(buff, "Fail\r\n", rec_buff.dma_len );
					}
					break;

				case CMD_BAUD:
				{
					app_uart_init(921600);//将波特率改成921600
					WaitMs(20);

					sprintf(buff, "OK_05\r\n");
					
					break;
				}
				default:
					break;
			}
			uart_print(buff);
		}
		else
		{
			sprintf(buff, "cmd error : %d\r\n", rec_buff.dma_len );
			uart_print(buff);
			WaitMs(30);
		}
		
		rec_buff.dma_len = 0;
	}
}
