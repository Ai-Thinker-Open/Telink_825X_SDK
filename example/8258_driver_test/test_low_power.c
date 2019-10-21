/********************************************************************************************************
 * @file     test_low_power.c 
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
 * test_low_power.c
 *
 *  Created on: 2018-4-17
 *      Author: Administrator
 */

#include "tl_common.h"
#include "drivers.h"



#define TEST_LP_SUSPEND_NO_WAKEUP						1 //ext32k+suspend+none: 45.09uA
#define TEST_LP_SUSPEND_GPIO_PAD_WAKEUP					2
#define TEST_LP_SUSPEND_TIMER_WAKEUP					3

#define TEST_LP_SUSPEND_TIMER_GPIO_PAD_WAKEUP			4

#define TEST_LP_DEEPSLEEP_NO_WAKEUP						5 //ext32k+deep+none: 0.43uA    //int32k+deep+none: 0.41uA
#define TEST_LP_DEEPSLEEP_GPIO_PAD_WAKEUP				6 //ext32k+deep+pad:  0.43uA    //int32k+deep+pad:  0.41uA
#define TEST_LP_DEEPSLEEP_TIMER_WAKEUP					7 //ext32k+deep+tmr:  2.03uA    //int32k+deep+tmr:  0.92uA

#define TEST_LP_DEEPSLEEP_RET16K_NO_WAKEUP				8 //ext32k+deepret16k+none(tmr should open: 3.00uA    //int32k+deepret16k+none: 1.41uA
#define TEST_LP_DEEPSLEEP_RET16K_GPIO_PAD_WAKEUP		9 //ext32k+deepret16k+pad(tmr should open:  3.00uA    //int32k+deepret16k+pad:  1.41uA
#define TEST_LP_DEEPSLEEP_RET16K_TIMER_WAKEUP			10//ext32k+deepret16k+tmr:  3.00uA                    //int32k+deepret16k+tmr:  1.87uA


#define TEST_LP_MODE									5



int first_power_on = 0;

u8 AA_wakeupSrc[256];
u8 AA_wkp_index;

void test_low_power(void)
{
	if(analog_read(0x3a) == 0x5a){  //read flag
		first_power_on = 0;
	}
	else{
		first_power_on = 1;
	}


	if(first_power_on){
		DBG_CHN0_TOGGLE;
		sleep_us(500000);
		DBG_CHN0_TOGGLE;
		sleep_us(500000);
		DBG_CHN0_TOGGLE;
		sleep_us(500000);
		DBG_CHN0_TOGGLE;
		sleep_us(500000);
	}
	else{
		DBG_CHN0_TOGGLE;
		sleep_us(500000);
		DBG_CHN0_TOGGLE;
		sleep_us(500000);
		DBG_CHN0_TOGGLE;
		sleep_us(500000);
		DBG_CHN0_TOGGLE;
		sleep_us(500000);
	}

	analog_write(0x3a, 0x5a); //set flag



#if(TEST_LP_MODE == TEST_LP_SUSPEND_NO_WAKEUP)

	shutdown_gpio();  //all GPIO high_Z state, no current leakage

	cpu_sleep_wakeup(SUSPEND_MODE , 0, 0);

#elif(TEST_LP_MODE == TEST_LP_SUSPEND_GPIO_PAD_WAKEUP)

	gpio_setup_up_down_resistor(GPIO_PB6, PM_PIN_PULLDOWN_100K);
	cpu_set_gpio_wakeup(GPIO_PB6, Level_High, 1);


	while(1){

		u32 wakeup_src = cpu_sleep_wakeup(SUSPEND_MODE, PM_WAKEUP_PAD, 0);  //PM_WAKEUP_PAD

		AA_wakeupSrc[AA_wkp_index++] = wakeup_src;

		DBG_CHN1_TOGGLE;
		sleep_us(300000);
		DBG_CHN1_TOGGLE;
		sleep_us(100000);
		DBG_CHN1_TOGGLE;
		sleep_us(600000);
		DBG_CHN1_TOGGLE;
		sleep_us(100000);
	}

#elif(TEST_LP_MODE == TEST_LP_SUSPEND_TIMER_WAKEUP)


	u32 now_tick;
	while(1){

		now_tick = clock_time();

		u32 wakeup_src = cpu_sleep_wakeup(SUSPEND_MODE, PM_WAKEUP_TIMER,  now_tick + 20 * CLOCK_16M_SYS_TIMER_CLK_1S);

		AA_wakeupSrc[AA_wkp_index++] = wakeup_src;

		DBG_CHN1_TOGGLE;
		sleep_us(20000);
		DBG_CHN1_TOGGLE;
		sleep_us(10000);
		DBG_CHN1_TOGGLE;
		sleep_us(60000);
		DBG_CHN1_TOGGLE;
		sleep_us(10000);
	}

#elif(TEST_LP_MODE == TEST_LP_SUSPEND_TIMER_GPIO_PAD_WAKEUP)

	gpio_setup_up_down_resistor(GPIO_PB6, PM_PIN_PULLDOWN_100K);
	cpu_set_gpio_wakeup(GPIO_PB6, Level_High, 1);


	u32 now_tick;
	while(1){

		now_tick = clock_time();

		u32 wakeup_src = cpu_sleep_wakeup(SUSPEND_MODE, PM_WAKEUP_TIMER | PM_WAKEUP_PAD,  now_tick + 20 * CLOCK_16M_SYS_TIMER_CLK_1S);

		AA_wakeupSrc[AA_wkp_index++] = wakeup_src;

		DBG_CHN1_TOGGLE;
		sleep_us(300000);
		DBG_CHN1_TOGGLE;
		sleep_us(100000);
		DBG_CHN1_TOGGLE;
		sleep_us(600000);
		DBG_CHN1_TOGGLE;
		sleep_us(100000);
	}

#elif(TEST_LP_MODE == TEST_LP_DEEPSLEEP_NO_WAKEUP)

	shutdown_gpio();  //all GPIO high_Z state, no current leakage

	cpu_sleep_wakeup(DEEPSLEEP_MODE , 0, 0);

#elif(TEST_LP_MODE == TEST_LP_DEEPSLEEP_GPIO_PAD_WAKEUP)


	gpio_setup_up_down_resistor(GPIO_PB6, PM_PIN_PULLDOWN_100K);
	cpu_set_gpio_wakeup(GPIO_PB6, Level_High, 1);

	while(1){
		cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  //PM_WAKEUP_PAD
	}


#elif(TEST_LP_MODE == TEST_LP_DEEPSLEEP_TIMER_WAKEUP)

	shutdown_gpio();  //all GPIO high_Z state, no current leakage

	while(1){

		cpu_sleep_wakeup(DEEPSLEEP_MODE , PM_WAKEUP_TIMER, clock_time() + 20*CLOCK_16M_SYS_TIMER_CLK_1S);

		DBG_CHN1_TOGGLE;
		sleep_us(700000);
		DBG_CHN1_TOGGLE;
		sleep_us(300000);
		DBG_CHN1_TOGGLE;
		sleep_us(700000);
		DBG_CHN1_TOGGLE;
		sleep_us(300000);
	}

#elif(TEST_LP_MODE == TEST_LP_DEEPSLEEP_RET16K_NO_WAKEUP)

	shutdown_gpio();  //all GPIO high_Z state, no current leakage
	cpu_sleep_wakeup(DEEPSLEEP_MODE_RET_SRAM_LOW16K , 0, 0);

#elif(TEST_LP_MODE == TEST_LP_DEEPSLEEP_RET16K_GPIO_PAD_WAKEUP)

	gpio_setup_up_down_resistor(GPIO_PB6, PM_PIN_PULLDOWN_100K);
	cpu_set_gpio_wakeup(GPIO_PB6, Level_High, 1);

	while(1){
		cpu_sleep_wakeup(DEEPSLEEP_MODE_RET_SRAM_LOW16K, PM_WAKEUP_PAD, 0);  //PM_WAKEUP_PAD
	}

#elif(TEST_LP_MODE == TEST_LP_DEEPSLEEP_RET16K_TIMER_WAKEUP)

	shutdown_gpio();  //all GPIO high_Z state, no current leakage

	while(1){

		cpu_sleep_wakeup(DEEPSLEEP_MODE_RET_SRAM_LOW16K , PM_WAKEUP_TIMER, clock_time() + 20*CLOCK_16M_SYS_TIMER_CLK_1S);

		DBG_CHN1_TOGGLE;
		sleep_us(700000);
		DBG_CHN1_TOGGLE;
		sleep_us(300000);
		DBG_CHN1_TOGGLE;
		sleep_us(700000);
		DBG_CHN1_TOGGLE;
		sleep_us(300000);
	}
#else


#endif


}
