/********************************************************************************************************
 * @file     app_i2c_slave.c 
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
/*
 * app_i2c_slave.c
 *
 *  Created on: 2018-6-15
 *      Author: Administrator
 */

#include "tl_common.h"
#include "drivers.h"



#if(DRIVER_TEST_MODE == TEST_IIC && I2C_DEMO_SELECT == I2C_DEMO_SLAVE)


/**************************************************************************************
   8258 Demo Code Config
   1. select i2c demo:  master or slave( config it in app_config.h )
   2. if i2c slave demo,  select slave working mode: mapping mode or dma mode
   3. if i2c master demo, select the peer device i2c slave address mode

 *************************************************************************************/
#if(I2C_DEMO_SELECT == I2C_DEMO_SLAVE)

	#define     SLAVE_DMA_MODE          1		//i2c slave use   dma mode
	#define     SLAVE_MAPPING_MODE      2       //i2c slave use   mapping mode


	//select i2c slave mode here(dma mode or mapping mode)
	#define     I2C_SLAVE_MODE			SLAVE_DMA_MODE
#endif

/**************************************************************************************
 *
 *************************************************************************************/





/*************** i2c slave data buffer ****************/
#if(I2C_SLAVE_MODE == SLAVE_DMA_MODE)

	//dma mode write & read  buff
	unsigned char * pBuf_slave_dma_for_write;
	unsigned char * pBuf_slave_dma_for_read;

#elif(I2C_SLAVE_MODE == SLAVE_MAPPING_MODE)

	//mapping mode, write buffer is the first half 64 byte, read buffer is last half 64 byte
	_attribute_aligned_(128) volatile unsigned char slave_mapping_buff[128] = {0};
#endif

#define SLAVE_DMA_READ_DATA_CHANGE_EN      0   //1: read buff data changes;  0: read buff data stay unchanged



unsigned char slave_rx_buff_debug[DBG_DATA_LEN *DBG_DATA_NUM];   //store i2c master writing data here to debug
int slave_rx_index = 0;



#define IRQ_IIC_CLEAR       0
#define IRQ_IIC_READ  		BIT(0)  //read
#define IRQ_IIC_WRITE		BIT(1)  //write


int i2c_irq_flag = IRQ_IIC_CLEAR;









void i2c_slave_test_init(void)
{

	i2c_gpio_set(I2C_GPIO_GROUP_C0C1);  	//SDA/CK : C0/C1


#if(I2C_SLAVE_MODE == SLAVE_DMA_MODE)
	//slave device id 0x5C(write) 0x5D(read)
	//i2c slave dma mode,  master need send 3 byte sram address(0x40000~0x4ffff) on i2c bus in i2c reading/writing
	i2c_slave_init(0x5C, I2C_SLAVE_DMA, NULL);

	//write&read sram address in dma mode
	//master write data to sram address SLAVE_DMA_MODE_ADDR_WRITE
	//slave put data on sram address SLAVE_DMA_MODE_ADDR_READ
	pBuf_slave_dma_for_write = (unsigned char *)(SLAVE_DMA_MODE_ADDR_WRITE | REG_BASE_ADDR);
	pBuf_slave_dma_for_read = (unsigned char *)(SLAVE_DMA_MODE_ADDR_READ | REG_BASE_ADDR);

	memset(pBuf_slave_dma_for_write, 0, DBG_DATA_LEN);  //clear write buffer data
	for(int i=0;i<DBG_DATA_LEN;i++){  //set read buffer initial data (1,2,3....16)
		pBuf_slave_dma_for_read[i] = i+1;
	}



#elif(I2C_SLAVE_MODE == SLAVE_MAPPING_MODE)
	//slave device id 0x5C(write) 0x5D(read)
	//i2c slave mapping mode, master no need send any address when reading/writing,
	//for i2c master: writing data buffer is slave_mapping_buff,
	//and reading data buffer is (slave_mapping_buff+64) (this offset 64 is managed by MCU hardware, user can not change it)
	i2c_slave_init(0x5C, I2C_SLAVE_MAP, (unsigned char *)slave_mapping_buff);

	//slave_mapping_buff + 64 is master reading data buffer in mapping mode, put some data here for master read
	//initial data (1,2,3 ... 16)
	for(int i=0;i<DBG_DATA_LEN;i++){
		slave_mapping_buff[64+i] = i+1;
	}

#endif



	reg_i2c_map_host_status = (FLD_HOST_CMD_IRQ | FLD_HOST_READ_IRQ);  //clean irq status

	reg_irq_mask |= FLD_IRQ_HOST_CMD_EN;   //enable i2c irq

	irq_enable(); //enable system irq

}


void i2c_slave_mainloop(void)
{



#if(I2C_SLAVE_MODE == SLAVE_DMA_MODE)  //8258 i2c slave dma mode
	//in dma mode, write & read data buffer could be different
	#if (SLAVE_DMA_READ_DATA_CHANGE_EN)
		   //i2c slave data changes, and i2c master can read different data
		   static u32 data_change_time = 0;
		   static u32 data_change_index = 0;
		   if(clock_time_exceed(data_change_time, 1000000)){
			   data_change_time = clock_time();
			   data_change_index ++;


			   for(int i=0;i<DBG_DATA_LEN;i++){
				   pBuf_slave_dma_for_read[i] = data_change_index;
			   }
		   }
	#endif


#elif(I2C_SLAVE_MODE == SLAVE_MAPPING_MODE)   //8258 i2c slave mapping  mode
	   //in mapping mode, i2c master reading data from (slave_mapping_buff+64)

	   #if (SLAVE_DMA_READ_DATA_CHANGE_EN)
			   //i2c slave data changes, and i2c master can read different data
			   static u32 data_change_time = 0;
			   static u32 data_change_index = 0;
			   if(clock_time_exceed(data_change_time, 1000000)){
				   data_change_time = clock_time();
				   data_change_index ++;
			   }

			   if(i2c_irq_flag){  //i2c master write or read operation happens
				   if(i2c_irq_flag & IRQ_IIC_READ){  //i2c master has just read data
					   DBG_CHN2_TOGGLE;
					   for(int i=0;i<DBG_DATA_LEN;i++){
						   slave_mapping_buff[64 + i] = data_change_index;
					   }
				   }
				   i2c_irq_flag = IRQ_IIC_CLEAR; //clear
			   }
	   #endif

#endif

}



















int irq_cnt = 0;
int i2c_read_cnt = 0;   //for debug
int i2c_write_cnt = 0;  //for debug


_attribute_ram_code_ void app_i2c_test_irq_proc(void)
{

	irq_cnt ++;       //for debug



	u8  irq_status = reg_i2c_map_host_status;//i2c slave can distinguish the operation host write or read.



	if(irq_status & FLD_HOST_CMD_IRQ){  //both host write & read trigger this status

		reg_i2c_map_host_status = irq_status; //clear all irq status

		if(irq_status & FLD_HOST_READ_IRQ){  //host read

			i2c_read_cnt ++;  //for debug
			DBG_CHN0_TOGGLE;  //for gpio debug

			i2c_irq_flag |= IRQ_IIC_READ;
		}
		else{  //host write

			i2c_write_cnt ++;  //for debug
			DBG_CHN1_TOGGLE;   //for debug

			i2c_irq_flag |= IRQ_IIC_WRITE;


			/*********** copy the data written by i2c master to slave_rx_buff_debug for debug ****************/
			#if (I2C_SLAVE_MODE == SLAVE_DMA_MODE)
				memcpy( (unsigned char*)(slave_rx_buff_debug + slave_rx_index*DBG_DATA_LEN), (unsigned char*)pBuf_slave_dma_for_write, DBG_DATA_LEN);
			#elif (I2C_SLAVE_MODE == SLAVE_MAPPING_MODE)
				memcpy( (unsigned char*)(slave_rx_buff_debug + slave_rx_index*DBG_DATA_LEN), (unsigned char*)slave_mapping_buff, DBG_DATA_LEN);
			#endif

			slave_rx_index ++;
			if(slave_rx_index>=DBG_DATA_NUM){
				slave_rx_index = 0;
			}
			/******************************** end ****************************************************/
		}

	}


}






#endif
