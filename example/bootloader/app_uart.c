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

#define BOOT_VERSION "V0.2"

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


void app_uart_init(void)
{
	//note: dma addr must be set first before any other uart initialization! (confirmed by sihui)
	uart_recbuff_init( (unsigned char *)&rec_buff, sizeof(rec_buff));

#if (_MODULE_TYPE_ == TB_01) //TB01模块
	uart_gpio_set(UART_TX_PB1, UART_RX_PB0);// uart tx/rx pin set
	#pragma message("_MODULE_TYPE_ == TB_01")
#elif (_MODULE_TYPE_ == TB_02) //TB02模块
	uart_gpio_set(UART_TX_PB1, UART_RX_PA0);// uart tx/rx pin set
	#pragma message("_MODULE_TYPE_ == TB_02")
#else
	#error "please set module type"
#endif
	uart_reset();  //will reset uart digital registers from 0x90 ~ 0x9f, so uart setting must set after this reset

	//baud rate: 115200
	#if (CLOCK_SYS_CLOCK_HZ == 16000000)
//		uart_init(118, 13, PARITY_NONE, STOP_BIT_ONE);
		uart_init(9, 13, PARITY_NONE, STOP_BIT_ONE);
	#elif (CLOCK_SYS_CLOCK_HZ == 24000000)
		uart_init(1, 12, PARITY_NONE, STOP_BIT_ONE); //baudrate = 500000
	#endif

	uart_dma_enable(1, 1); 	//uart data in hardware buffer moved by dma, so we need enable them first

	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);   	//uart Rx/Tx dma irq enable

	uart_irq_enable(0, 0);  	//uart Rx/Tx irq no need, disable them

	//irq_enable();
}

void uart_print(char * str)
{
	while(*str)
	{
		trans_buff.data[trans_buff.dma_len] = *str++;
		trans_buff.dma_len += 1;
		if(trans_buff.dma_len == 12)
		{
			uart_dma_send((unsigned char*)&trans_buff);
			trans_buff.dma_len = 0;
			WaitMs(2);
		}
	}

	if(trans_buff.dma_len)
	{
		uart_dma_send((unsigned char*)&trans_buff);
		trans_buff.dma_len = 0;
		WaitMs(2);
	}
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
    CMD_VRSN = 0x00, //??Boot???
	CMD_WRTE , 		 //?Flash
	CMD_READ ,       //?Flash
	CMD_ERAS ,       //??Flash
};

// /**
//  * @brief This function serves to erase a sector.
//  * @param[in]   addr the start address of the sector needs to erase.
//  * @return none
//  */
// _attribute_ram_code_ void flash_erase_sector(unsigned long addr);

// /**
//  * @brief This function writes the buffer's content to a page.
//  * @param[in]   addr the start address of the page
//  * @param[in]   len the length(in byte) of content needs to write into the page
//  * @param[in]   buf the start address of the content needs to write into
//  * @return none
//  */
// _attribute_ram_code_ void flash_write_page(unsigned long addr, unsigned long len, unsigned char *buf);

// /**
//  * @brief This function reads the content from a page to the buf.
//  * @param[in]   addr the start address of the page
//  * @param[in]   len the length(in byte) of content needs to read out from the page
//  * @param[out]  buf the start address of the buffer
//  * @return none
//  */
// _attribute_ram_code_ void flash_read_page(unsigned long addr, unsigned long len, unsigned char *buf);

int flash_write(unsigned long addr, unsigned char *buf)
{
	flash_write_page(addr, 256, buf);
	return 0;
}
char buff[128] = { 0 };
unsigned long addr;
void app_uart_loop(void)
{

	if(rec_buff.dma_len > 0)
	{
		if(rec_buff.data[1] * 256 + rec_buff.data[2] == rec_buff.dma_len - 3)
		{
			switch (rec_buff.data[0])
			{
				case CMD_VRSN: //读取版本号
					sprintf(buff, BOOT_VERSION"\r\n");
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

					sprintf(buff, "OK_02\r\n", rec_buff.dma_len );
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
						sprintf(buff, "OK_03\r\n", rec_buff.dma_len );
					}
					else
					{
						sprintf(buff, "Fail\r\n", rec_buff.dma_len );
					}
					break;

				default:
					break;
			}
			uart_print(buff);
			//WaitMs(0);
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
