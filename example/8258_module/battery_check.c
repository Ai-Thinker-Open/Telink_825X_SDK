/********************************************************************************************************
 * @file     battery_check.c 
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     Sep. 18, 2015
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
 * battery_check.c
 *
 *  Created on: 2018-8-3
 *      Author: Administrator
 */

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "battery_check.h"


#if (BATT_CHECK_ENABLE)




//_attribute_data_retention_	u8		adc_first_flg = 1;
_attribute_data_retention_	u8 		lowBattDet_enable = 1;
							u8      adc_hw_initialized = 0;   //note: can not be retention variable
_attribute_data_retention_  u16     batt_vol_mv;






#define ADC_SAMPLE_NUM		8



_attribute_data_retention_	volatile unsigned int adc_dat_buf[ADC_SAMPLE_NUM];  //size must 16 byte aligned(16/32/64...)







int app_suspend_enter_low_battery (void)
{
	if (!gpio_read(GPIO_WAKEUP_MODULE)) //gpio low level
	{
		analog_write(USED_DEEP_ANA_REG,  analog_read(USED_DEEP_ANA_REG)|LOW_BATT_FLG);  //mark
		return 1;//allow enter cpu_sleep_wakeup
	}

	analog_write(USED_DEEP_ANA_REG,  analog_read(USED_DEEP_ANA_REG)&(~LOW_BATT_FLG));  //clr
	return 0; //forbidden enter cpu_sleep_wakeup
}


void battery_set_detect_enable (int en)
{
	lowBattDet_enable = en;

	if(!en){
		adc_hw_initialized = 0;   //need initialized again
	}

}


int battery_get_detect_enable (void)
{
	return lowBattDet_enable;
}



_attribute_ram_code_ void adc_vbat_detect_init(void)
{

	//telink advice: you must choose one gpio with adc function to output high level(voltage will equal to vbat), then use adc to measure high level voltage
	gpio_set_output_en(GPIO_VBAT_DETECT, 1);
	gpio_write(GPIO_VBAT_DETECT, 1);




	/******set adc sample clk as 4MHz******/
	adc_set_sample_clk(5); //adc sample clk= 24M/(1+5)=4M

	/******set adc L R channel Gain Stage bias current trimming******/
	adc_set_left_right_gain_bias(GAIN_STAGE_BIAS_PER100, GAIN_STAGE_BIAS_PER100);




	//set misc channel en,  and adc state machine state cnt 2( "set" stage and "capture" state for misc channel)
	adc_set_chn_enable_and_max_state_cnt(ADC_MISC_CHN, 2);  	//set total length for sampling state machine and channel

	//set "capture state" length for misc channel: 240
	//set "set state" length for misc channel: 10
	//adc state machine  period  = 24M/250 = 96K, T = 10.4 uS
	adc_set_state_length(240, 0, 10);  	//set R_max_mc,R_max_c,R_max_s





#if 1  //optimize, for saving time
	//set misc channel use differential_mode,
	//set misc channel resolution 14 bit,  misc channel differential mode
	//notice that: in differential_mode MSB is sign bit, rest are data,  here BIT(13) is sign bit
	analog_write (anareg_adc_res_m, RES14 | FLD_ADC_EN_DIFF_CHN_M);
	adc_set_ain_chn_misc(ADC_INPUT_PCHN, GND);
#else
////set misc channel use differential_mode,
	adc_set_ain_channel_differential_mode(ADC_MISC_CHN, ADC_INPUT_PCHN, GND);

	//set misc channel resolution 14 bit
	//notice that: in differential_mode MSB is sign bit, rest are data,  here BIT(13) is sign bit
	adc_set_resolution(ADC_MISC_CHN, RES14);
#endif


	//set misc channel vref 1.2V
	adc_set_ref_voltage(ADC_MISC_CHN, ADC_VREF_1P2V);


	//set misc t_sample 6 cycle of adc clock:  6 * 1/4M
#if 1   //optimize, for saving time
	adc_set_tsample_cycle_chn_misc(SAMPLING_CYCLES_6);  	//Number of ADC clock cycles in sampling phase
#else
	adc_set_tsample_cycle(ADC_MISC_CHN, SAMPLING_CYCLES_6);   	//Number of ADC clock cycles in sampling phase
#endif

	//set Analog input pre-scal.ing 1/8
	adc_set_ain_pre_scaler(ADC_PRESCALER_1F8);




	/******power on sar adc********/
	//note: this setting must be set after all other settings
	adc_power_on_sar_adc(1);
}


_attribute_ram_code_ void app_battery_power_check(u16 alram_vol_mv)
{
	u16 temp;
	int i,j;

	//when MCU powered up or wakeup from deep/deep with retention, adc need be initialized
	if(!adc_hw_initialized){
		adc_hw_initialized = 1;
		adc_vbat_detect_init();
	}



	adc_reset_adc_module();
	u32 t0 = clock_time();


	u16 adc_sample[ADC_SAMPLE_NUM] = {0};
	u32 adc_result;

	for(i=0;i<ADC_SAMPLE_NUM;i++){   	//dfifo data clear
		adc_dat_buf[i] = 0;
	}
	while(!clock_time_exceed(t0, 25));  //wait at least 2 sample cycle(f = 96K, T = 10.4us)

	//dfifo setting will lose in suspend/deep, so we need config it every time
	adc_config_misc_channel_buf((u16 *)adc_dat_buf, ADC_SAMPLE_NUM<<2);  //size: ADC_SAMPLE_NUM*4
	dfifo_enable_dfifo2();





//////////////// get adc sample data and sort these data ////////////////
	for(i=0;i<ADC_SAMPLE_NUM;i++){
		while(!adc_dat_buf[i]);


		if(adc_dat_buf[i] & BIT(13)){  //14 bit resolution, BIT(13) is sign bit, 1 means negative voltage in differential_mode
			adc_sample[i] = 0;
		}
		else{
			adc_sample[i] = ((u16)adc_dat_buf[i] & 0x1FFF);  //BIT(12..0) is valid adc result
		}



		//insert sort
		if(i){
			if(adc_sample[i] < adc_sample[i-1]){
				temp = adc_sample[i];
				adc_sample[i] = adc_sample[i-1];
				for(j=i-1;j>=0 && adc_sample[j] > temp;j--){
					adc_sample[j+1] = adc_sample[j];
				}
				adc_sample[j+1] = temp;
			}
		}
	}
//////////////////////////////////////////////////////////////////////////////




	dfifo_disable_dfifo2();   //misc channel data dfifo disable





///// get average value from raw data(abandon some small and big data ), then filter with history data //////
#if (ADC_SAMPLE_NUM == 4)  	//use middle 2 data (index: 1,2)
	u32 adc_average = (adc_sample[1] + adc_sample[2])/2;
#elif(ADC_SAMPLE_NUM == 8) 	//use middle 4 data (index: 2,3,4,5)
	u32 adc_average = (adc_sample[2] + adc_sample[3] + adc_sample[4] + adc_sample[5])/4;
#endif




#if 1
	adc_result = adc_average;
#else  	//history data filter
	if(adc_first_flg){
		adc_result = adc_average;
		adc_first_flg = 0;
	}
	else{
		adc_result = ( (adc_result*3) + adc_average + 2 )>>2;  //filter
	}
#endif




//////////////// adc sample data convert to voltage(mv) ////////////////
	//                          (Vref, 1/8 scaler)   (BIT<12~0> valid data)
	//			 =  adc_result * Vref * 8 / 0x2000
	//           =  adc_result * Vref >>10
	batt_vol_mv  = (adc_result * adc_vref_cfg.adc_vref)>>10;




	if(batt_vol_mv < alram_vol_mv){

		#if (1 && BLT_APP_LED_ENABLE)  //led indicate
			gpio_set_output_en(GPIO_LED, 1);  //output enable
			for(int k=0;k<3;k++){
				gpio_write(GPIO_LED, LED_ON_LEVAL);
				sleep_us(200000);
				gpio_write(GPIO_LED, !LED_ON_LEVAL);
				sleep_us(200000);
			}
		#endif


		GPIO_WAKEUP_MODULE_LOW;
		bls_pm_registerFuncBeforeSuspend( &app_suspend_enter_low_battery );
//		bls_pm_registerFuncBeforeSuspend( NULL );

		cpu_set_gpio_wakeup (GPIO_WAKEUP_MODULE, Level_High, 1);  //drive pin pad high wakeup deepsleep

		cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  //deepsleep
//		cpu_sleep_wakeup(DEEPSLEEP_MODE, 0, 0);  //deepsleep
	}
	else{ // batt level > alarm level
		analog_write(USED_DEEP_ANA_REG,  analog_read(USED_DEEP_ANA_REG)&(~LOW_BATT_FLG));  //clr
	}
}

#endif
