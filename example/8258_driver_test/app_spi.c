/********************************************************************************************************
 * @file     app_spi.c 
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




extern void spi_master_test_init(void);
extern void spi_slave_test_init(void);
extern void spi_master_mainloop(void);




void app_spi_test_init(void)
{
	WaitMs(2000);  //leave enough time for SWS_reset when power on
	//SPI:CK/CN/DO/DI   A4/D6/A2/A3, D7/D2/B7/B6
	#if (SPI_MODE==SPI_MASTER_MODE)
	 spi_master_test_init();

	#else
	 spi_slave_test_init();
	#endif
}






void app_spi_test_start(void)
{
#if (SPI_MODE==SPI_MASTER_MODE)

	spi_master_mainloop();

#else
		WaitMs(50);
#endif
}


void app_spi_test_irq_proc(void)
{

}
