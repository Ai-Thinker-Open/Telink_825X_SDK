/********************************************************************************************************
 * @file     app_i2c.c 
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


extern void i2c_master_test_init(void);
extern void i2c_slave_test_init(void);
extern void	i2c_master_mainloop(void);
extern void	i2c_slave_mainloop(void);


void app_i2c_test_init(void)
{
	WaitMs(2000);  //leave enough time for SWS_reset when power on


#if(I2C_DEMO_SELECT == I2C_DEMO_MASTER)

	i2c_master_test_init();

#elif(I2C_DEMO_SELECT == I2C_DEMO_SLAVE)

	i2c_slave_test_init();

#endif
}



void app_i2c_test_start(void)
{
#if (I2C_DEMO_SELECT == I2C_DEMO_MASTER)   //master demo mainloop


	i2c_master_mainloop();


#elif(I2C_DEMO_SELECT == I2C_DEMO_SLAVE)  //slave demo mainloop

	i2c_slave_mainloop();

#endif
}
