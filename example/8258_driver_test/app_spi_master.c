/********************************************************************************************************
 * @file     app_spi_master.c 
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

#if (DRIVER_TEST_MODE == TEST_SPI)




/**************************************************************************************
   8258 Demo Code Config
   1. select spi demo:  master or slave( config it in app_config.h )
   2. if spi slave demo,  select slave working mode: dma mode
   3. if spi master demo, select the peer device spi slave address mode:
 *************************************************************************************/
    #define     SLAVE_TELINK_825x_DMA_MODE      1 //slave  825x dma mode, master send 3 byte sram address(0x40000~0x4ffff)
	#define     SLAVE_TELINK_826x_DMA_MODE      2 //slave  826x dma mode, master send 2 byte sram address(0x8000~0xffff)
	#define		PEER_SLAVE_DEVICE				SLAVE_TELINK_825x_DMA_MODE
    #define     SPI_CS_PIN				        GPIO_PD6//SPI CS pin

#if (PEER_SLAVE_DEVICE == SLAVE_TELINK_825x_DMA_MODE)
    unsigned char slaveRegAddr_WriteCMD[] = {SLAVE_REGADDR1,SLAVE_REGADDR2,SLAVE_REGADDR3,SPI_WRITE_CMD};
	unsigned char slaveRegAddr_ReadCMD[]  = {SLAVE_REGADDR1,SLAVE_REGADDR2,SLAVE_REGADDR3,SPI_READ_CMD};
#elif(PEER_SLAVE_DEVICE == SLAVE_TELINK_826x_DMA_MODE)
    unsigned char slaveRegAddr_WriteCMD[] = {SLAVE_REGADDR2,SLAVE_REGADDR3,SPI_WRITE_CMD};
    unsigned char slaveRegAddr_ReadCMD[]  = {SLAVE_REGADDR2,SLAVE_REGADDR3,SPI_READ_CMD};
#endif
//write buff
volatile unsigned char spi_master_tx_buff[DBG_DATA_LEN]={0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff};
//read buff
volatile unsigned char spi_master_rx__buff[DBG_DATA_LEN]={0x00};

void spi_master_test_init(void)
{
	//spi clock 500K, only master need set i2c clock
	spi_master_init((unsigned char)(CLOCK_SYS_CLOCK_HZ/(2*500000)-1),SPI_MODE0);          //div_clock. spi_clk = sys_clk/((div_clk+1)*2),mode select
    spi_master_gpio_set(SPI_GPIO_GROUP_A2A3A4D6);    //master mode £ºspi pin set
}

void spi_master_mainloop(void)
{
	WaitMs(1000);   //1S
	spi_master_tx_buff[0] += 1;
	spi_master_tx_buff[0] &= 0xff;
#if (PEER_SLAVE_DEVICE == SLAVE_TELINK_825x_DMA_MODE)
    spi_write(slaveRegAddr_WriteCMD, 4,(unsigned char*)spi_master_tx_buff, DBG_DATA_LEN,SPI_CS_PIN);// pls refer to the datasheet for the write and read format of spi.
	spi_read( slaveRegAddr_ReadCMD , 4,(unsigned char*)spi_master_rx__buff,DBG_DATA_LEN,SPI_CS_PIN);
#elif (PEER_SLAVE_DEVICE == SLAVE_TELINK_826x_DMA_MODE)
	spi_write(slaveRegAddr_WriteCMD, 3,(unsigned char*)spi_master_tx_buff, DBG_DATA_LEN,SPI_CS_PIN);// pls refer to the datasheet for the write and read format of spi.
    spi_read( slaveRegAddr_ReadCMD , 3,(unsigned char*)spi_master_rx__buff,DBG_DATA_LEN,SPI_CS_PIN);
#endif
}




#endif //end of DRIVER_TEST_MODE == TEST_SPI
