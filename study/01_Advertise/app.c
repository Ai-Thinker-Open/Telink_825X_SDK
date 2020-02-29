/********************************************************************************************************
 * @file     feature_adv_power.c 
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     May. 10, 2018
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
#include <stack/ble/ble.h>
#include "tl_common.h"
#include "drivers.h"
#include "app_config.h"
#include "vendor/common/blt_led.h"
#include "application/keyboard/keyboard.h"
#include "vendor/common/tl_audio.h"
#include "vendor/common/blt_soft_timer.h"
#include "vendor/common/blt_common.h"

//#define		MY_RF_POWER_INDEX	RF_POWER_P10p29dBm //  10.29 dbm 
#define	MY_RF_POWER_INDEX	RF_POWER_P0p04dBm
											
void user_init_normal(void)
{
	random_generator_init();  //初始化随机数生成器

	u8  mac_public[6];
	u8  mac_random_static[6];
	blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static); //初始化MAC地址

	////// Controller Initialization  //////////
	blc_ll_initBasicMCU();   //初始化MCU

	blc_ll_initStandby_module(mac_public);		//初始化蓝牙待机功能模块

	blc_ll_initAdvertising_module(mac_public);  //初始化蓝牙广播功能模块

	u8 tbl_advData[] = { 0x05, 0x09, 'A', 'B', 'C', 'D'}; //要广播的数据

	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) ); //设置广播数据

	//bls_ll_setScanRspData( (u8 *)tbl_advData, sizeof(tbl_advData)); //设置扫描响应数据(这里设置的与广播数据相同，实施上可以不同)

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_500MS , //广播时间间隔最小值
									ADV_INTERVAL_500MS , //广播时间间隔最大值
									ADV_TYPE_NONCONNECTABLE_UNDIRECTED, //广播类型，不可连接非定向
									OWN_ADDRESS_PUBLIC, //自身地址类型
									0,  //定向地址类型
									NULL, //定向地址
									BLT_ENABLE_ADV_ALL, //在全部广播信道(37,38,39)都广播数据
									ADV_FP_NONE); //过滤策略

	if(status != BLE_SUCCESS)//如果设置广播参数失败
	{
		write_reg8(0x40000, 0x11);  //debug
		while(1);
	}

	rf_set_power_level_index (MY_RF_POWER_INDEX); //设置发射功率

	bls_ll_setAdvEnable(1);  //开启广播

	app_uart_init(); //初始化串口，用于调试打印输出
	
	irq_enable();
}


_attribute_ram_code_ void main_loop (void)
{
	blt_sdk_main_loop();
}