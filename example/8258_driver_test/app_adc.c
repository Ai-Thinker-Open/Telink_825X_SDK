/********************************************************************************************************
 * @file     app_adc.c 
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


#if (DRIVER_TEST_MODE == TEST_ADC)


#define  TEST_ADC_GPIO						1   //test voltage come from adc gpio
#define  TEST_ADC_VBAT						2   //test power supply


#define  TEST_ADC_SELECT					TEST_ADC_GPIO







void adc_gpio_ain_init(void)
{
	//set misc channel en,  and adc state machine state cnt 2( "set" stage and "capture" state for misc channel)
	adc_set_chn_enable_and_max_state_cnt(ADC_MISC_CHN, 2);  	//set total length for sampling state machine and channel

	//set "capture state" length for misc channel: 240
	//set "set state" length for misc channel: 10
	//adc state machine  period  = 24M/250 = 96K, T = 10.4 uS
	adc_set_state_length(240, 0, 10);  	//set R_max_mc,R_max_c,R_max_s


	//set misc channel use differential_mode (telink advice: only differential mode is available)
	//single mode adc source, PB4 for example: PB4 positive channel, GND negative channel
	gpio_set_func(GPIO_PB4, AS_GPIO);
	gpio_set_input_en(GPIO_PB4, 0);
	gpio_set_output_en(GPIO_PB4, 0);
	gpio_write(GPIO_PB4, 0);
	adc_set_ain_channel_differential_mode(ADC_MISC_CHN, B4P, GND);


	//set misc channel resolution 14 bit
	//notice that: in differential_mode MSB is sign bit, rest are data,  here BIT(13) is sign bit
	adc_set_resolution(ADC_MISC_CHN, RES14);  //set resolution


	//set misc channel vref 1.2V
	adc_set_ref_voltage(ADC_MISC_CHN, ADC_VREF_1P2V);  					//set channel Vref



	//set misc t_sample 6 cycle of adc clock:  6 * 1/4M
	adc_set_tsample_cycle(ADC_MISC_CHN, SAMPLING_CYCLES_6);  	//Number of ADC clock cycles in sampling phase

	//set Analog input pre-scaling 1/8
	adc_set_ain_pre_scaler(ADC_PRESCALER_1F8);
}




void adc_vbat_detect_init(void)
{


	//set misc channel en,  and adc state machine state cnt 2( "set" stage and "capture" state for misc channel)
	adc_set_chn_enable_and_max_state_cnt(ADC_MISC_CHN, 2);  	//set total length for sampling state machine and channel


	//set "capture state" length for misc channel: 240
	//set "set state" length for misc channel: 10
	//adc state machine  period  = 24M/250 = 96K, T = 10.4 uS
	adc_set_state_length(240, 0, 10);  	//set R_max_mc,R_max_c,R_max_s


	//telink advice: you must choose one gpio with adc function to output high level(voltage will equal to vbat), then use adc to measure high level voltage
	gpio_set_func(GPIO_PB0, AS_GPIO);
	gpio_set_input_en(GPIO_PB0, 0);
	gpio_set_output_en(GPIO_PB0, 1);
	gpio_write(GPIO_PB0, 1);

	//set misc channel use differential_mode(telink advice: only differential mode is available)
	adc_set_ain_channel_differential_mode(ADC_MISC_CHN, B0P, GND);

	//set misc channel resolution 14 bit
	//notice that: in differential_mode MSB is sign bit, rest are data,  here BIT(13) is sign bit
	adc_set_resolution(ADC_MISC_CHN, RES14);   //set resolution



	//set misc channel vref 1.2V
	adc_set_ref_voltage(ADC_MISC_CHN, ADC_VREF_1P2V);  					//set channel Vref



	//set misc t_sample 6 cycle of adc clock:  6 * 1/4M
	adc_set_tsample_cycle(ADC_MISC_CHN, SAMPLING_CYCLES_6);  	//Number of ADC clock cycles in sampling phase

	//set Analog input pre-scaling 1/8
	adc_set_ain_pre_scaler(ADC_PRESCALER_1F8);


}




void app_adc_test_init(void)
{
////Step 1: power off sar adc/////////////////////////////////////////////////////////
	/******power off sar adc********/
	adc_power_on_sar_adc(0);
//////////////////////////////////////////////////////////////////////////////////////





////Step 2: Config some common adc settings(user can not change these)/////////////////
	/******enable signal of 24M clock to sar adc********/
	adc_enable_clk_24m_to_sar_adc(1);

	/******set adc sample clk as 4MHz******/
	adc_set_sample_clk(5); //adc sample clk= 24M/(1+5)=4M

	/******set adc L R channel Gain Stage bias current trimming******/
	adc_set_left_gain_bias(GAIN_STAGE_BIAS_PER100);
	adc_set_right_gain_bias(GAIN_STAGE_BIAS_PER100);
////////////////////////////////////////////////////////////////////////////////////////





////Step 3: Config adc settings  as needed /////////////////////////////////////////////
#if (TEST_ADC_SELECT == TEST_ADC_GPIO)
	adc_gpio_ain_init();

#elif (TEST_ADC_SELECT == TEST_ADC_VBAT)

	adc_vbat_detect_init();

#endif
////////////////////////////////////////////////////////////////////////////////////////



////Step 4: power on sar adc/////////////////////////////////////////////////////////
	/******power on sar adc********/
	adc_power_on_sar_adc(1);
////////////////////////////////////////////////////////////////////////////////////////
}





u16 Adc_cur_rawData;   //unit: m V
u16 Adc_raw_data[256];
u8  Adc_raw_datIndex = 0;



//just for display, fake data
u32 Adc_cur_vol_oct; //debug
u16 Adc_cal_vol_oct[256];
u8  Adc_cal_vol_octIndex= 0;



u32 tick_adc_sample = 0;
void app_adc_test_start(void)
{



	if(clock_time_exceed(tick_adc_sample, 200000)){
		tick_adc_sample = clock_time();


		Adc_cur_rawData =  adc_sample_and_get_result();

		Adc_raw_data[Adc_raw_datIndex ++]  = Adc_cur_rawData;


#if 1 //debug
		Adc_cur_vol_oct =   (Adc_cur_rawData/1000)<<12 | ((Adc_cur_rawData/100)%10)<<8 \
				                      | ((Adc_cur_rawData%100)/10)<<4  | (Adc_cur_rawData%10);
		Adc_cal_vol_oct[Adc_cal_vol_octIndex ++] = Adc_cur_vol_oct;
#endif

	}
}




#endif   //end of DRIVER_TEST_MODE == TEST_ADC
