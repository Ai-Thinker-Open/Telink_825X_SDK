/********************************************************************************************************
 * @file	 app_ui.c
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     Sep. 18, 2018
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
#include "stack/ble/ble.h"
#include "app_config.h"

#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"
#include "../common/tl_audio.h"
#include "../common/blt_led.h"
#include "../common/blt_soft_timer.h"

#include "rc_ir.h"
#include "battery_check.h"



//////////////////// key type ///////////////////////
#define IDLE_KEY	   			0
#define CONSUMER_KEY   	   		1
#define KEYBOARD_KEY   	   		2
#define IR_KEY   	   			3

_attribute_data_retention_	u8 		key_type;
_attribute_data_retention_	int 	key_not_released;


_attribute_data_retention_	u8 		ota_is_working = 0;


_attribute_data_retention_	int     ui_mtu_size_exchange_req = 0;

_attribute_data_retention_	int 	ir_not_released;
_attribute_data_retention_	u8 		user_key_mode;
u8      ir_hw_initialed = 0;   //note: can not be retention variable



_attribute_data_retention_	u8		ui_mic_enable = 0;
_attribute_data_retention_	u8 		key_voice_press = 0;



extern	u32 	latest_user_event_tick;


static const u16 vk_consumer_map[16] = {
		MKEY_VOL_UP,
		MKEY_VOL_DN,
		MKEY_MUTE,
		MKEY_CHN_UP,

		MKEY_CHN_DN,
		MKEY_POWER,
		MKEY_AC_SEARCH,
		MKEY_RECORD,

		MKEY_PLAY,
		MKEY_PAUSE,
		MKEY_STOP,
		MKEY_FAST_FORWARD,  //can not find fast_backword in <<HID Usage Tables>>

		MKEY_FAST_FORWARD,
		MKEY_AC_HOME,
		MKEY_AC_BACK,
		MKEY_MENU,
};





/////////////////////////// led management /////////////////////
#if (BLT_APP_LED_ENABLE)

	enum{
		LED_POWER_ON = 0,
		LED_AUDIO_ON,	//1
		LED_AUDIO_OFF,	//2
		LED_SHINE_SLOW, //3
		LED_SHINE_FAST, //4
		LED_SHINE_OTA, //5
	};

	const led_cfg_t led_cfg[] = {
			{1000,    0,      1,      0x00,	 },    //power-on, 1s on
			{100,	  0 ,	  0xff,	  0x02,  },    //audio on, long on
			{0,	      100 ,   0xff,	  0x02,  },    //audio off, long off
			{500,	  500 ,   2,	  0x04,	 },    //1Hz for 3 seconds
			{250,	  250 ,   4,	  0x04,  },    //2Hz for 3 seconds
			{250,	  250 ,   200,	  0x08,  },    //2Hz for 50 seconds
	};

#endif





#if (BLE_AUDIO_ENABLE)
	u32 	key_voice_pressTick = 0;

	void ui_enable_mic (int en)
	{
		ui_mic_enable = en;

		//AMIC Bias output
		gpio_set_output_en (GPIO_AMIC_BIAS, en);
		gpio_write (GPIO_AMIC_BIAS, en);

		#if (BLT_APP_LED_ENABLE)
			device_led_setup(led_cfg[en ? LED_AUDIO_ON : LED_AUDIO_OFF]);
		#endif

		if(en){  //audio on

			///////////////////// AUDIO initialization///////////////////
			//buffer_mic set must before audio_init !!!
			audio_config_mic_buf ( buffer_mic, TL_MIC_BUFFER_SIZE);

			#if (BLE_DMIC_ENABLE)  //Dmic config

			#else  //Amic config
				audio_amic_init(AUDIO_16K);
			#endif

		}
		else{  //audio off
			adc_power_on_sar_adc(0);   //power off sar adc
		}

		#if (BATT_CHECK_ENABLE)
			battery_set_detect_enable(!en);
		#endif
	}


	void voice_press_proc(void)
	{
		key_voice_press = 0;
		ui_enable_mic (1);
		if(ui_mtu_size_exchange_req && blc_ll_getCurrentState() == BLS_LINK_STATE_CONN){
			ui_mtu_size_exchange_req = 0;
			blc_att_requestMtuSizeExchange(BLS_CONN_HANDLE, 0x009e);
		}
	}


	void task_audio (void)
	{
		static u32 audioProcTick = 0;
		if(clock_time_exceed(audioProcTick, 500)){
			audioProcTick = clock_time();
		}
		else{
			return;
		}

		///////////////////////////////////////////////////////////////
		log_event(TR_T_audioTask);


		proc_mic_encoder ();

		//////////////////////////////////////////////////////////////////
		if (blc_ll_getTxFifoNumber() < 9)
		{
			int *p = mic_encoder_data_buffer ();
			if (p)					//around 3.2 ms @16MHz clock
			{
				log_event (TR_T_audioData);
				if( BLE_SUCCESS == bls_att_pushNotifyData (AUDIO_MIC_INPUT_DP_H, (u8*)p, ADPCM_PACKET_LEN) ){
					mic_encoder_data_read_ok();
				}
			}
		}
	}



	void blc_checkConnParamUpdate(void)
	{
		extern u32 interval_update_tick;

		if(	 interval_update_tick && clock_time_exceed(interval_update_tick,5*1000*1000) && \
			 blc_ll_getCurrentState() == BLS_LINK_STATE_CONN &&  bls_ll_getConnectionInterval()!= 8 )
		{
			interval_update_tick = clock_time() | 1;
			bls_l2cap_requestConnParamUpdate (8, 8, 99, 400);
		}
	}
#endif





#if (REMOTE_IR_ENABLE)
	//ir key
	#define TYPE_IR_SEND			1
	#define TYPE_IR_RELEASE			2

	///////////////////// key mode //////////////////////
	#define KEY_MODE_BLE	   		0    //ble key
	#define KEY_MODE_IR        		1    //ir  key


	static const u8 kb_map_ble[30] = 	KB_MAP_BLE;
	static const u8 kb_map_ir[30] = 	KB_MAP_IR;


	void ir_dispatch(u8 type, u8 syscode ,u8 ircode){

		if(!ir_hw_initialed){
			ir_hw_initialed = 1;
			rc_ir_init();
		}

		if(type == TYPE_IR_SEND){
			ir_nec_send(syscode,~(syscode),ircode);

		}
		else if(type == TYPE_IR_RELEASE){
			ir_send_release();
		}
	}


#endif



#if (BLE_REMOTE_OTA_ENABLE)
	void app_enter_ota_mode(void)
	{
		ota_is_working = 1;
		#if (BLT_APP_LED_ENABLE)
			device_led_setup(led_cfg[LED_SHINE_OTA]);
		#endif
		bls_ota_setTimeout(15 * 1000 * 1000); //set OTA timeout  15 seconds
	}



	void app_debug_ota_result(int result)
	{

		#if(0 && BLT_APP_LED_ENABLE)  //this is only for debug

			gpio_set_output_en(GPIO_LED, 1);

			if(result == OTA_SUCCESS){  //led for debug: OTA success
				gpio_write(GPIO_LED, 1);
				sleep_us(500000);
				gpio_write(GPIO_LED, 0);
				sleep_us(500000);
				gpio_write(GPIO_LED, 1);
				sleep_us(500000);
				gpio_write(GPIO_LED, 0);
				sleep_us(500000);
			}
			else{  //OTA fail

				#if 0 //this is only for debug,  can not use this in application code
					irq_disable();
					WATCHDOG_DISABLE;

					write_reg8(0x40001, result);  //OTA fail reason
					write_reg8(0x40000, 0x33);
					while(1){
						gpio_write(GPIO_LED, 1);
						sleep_us(200000);
						gpio_write(GPIO_LED, 0);
						sleep_us(200000);
					}
					write_reg8(0x40000, 0x44);
				#endif

			}

			gpio_set_output_en(GPIO_LED, 0);
		#endif
	}
#endif






/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////


//This function process ...
void deep_wakeup_proc(void)
{

#if(DEEPBACK_FAST_KEYSCAN_ENABLE)
	//if deepsleep wakeup is wakeup by GPIO(key press), we must quickly scan this
	//press, hold this data to the cache, when connection established OK, send to master
	//deepsleep_wakeup_fast_keyscan
	if(analog_read(USED_DEEP_ANA_REG) & CONN_DEEP_FLG){
		if(kb_scan_key (KB_NUMLOCK_STATUS_POWERON, 1) && kb_event.cnt){
			deepback_key_state = DEEPBACK_KEY_CACHE;
			key_not_released = 1;
			memcpy(&kb_event_cache,&kb_event,sizeof(kb_event));
		}

		analog_write(USED_DEEP_ANA_REG, analog_read(USED_DEEP_ANA_REG) & (~CONN_DEEP_FLG));
	}
#endif
}





void deepback_pre_proc(int *det_key)
{
#if (DEEPBACK_FAST_KEYSCAN_ENABLE)
	// to handle deepback key cache
	if(!(*det_key) && deepback_key_state == DEEPBACK_KEY_CACHE && blc_ll_getCurrentState() == BLS_LINK_STATE_CONN \
			&& clock_time_exceed(bls_ll_getConnectionCreateTime(), 25000)){

		memcpy(&kb_event,&kb_event_cache,sizeof(kb_event));
		*det_key = 1;

		if(key_not_released || kb_event_cache.keycode[0] == VOICE){  //no need manual release
			deepback_key_state = DEEPBACK_KEY_IDLE;
		}
		else{  //need manual release
			deepback_key_tick = clock_time();
			deepback_key_state = DEEPBACK_KEY_WAIT_RELEASE;
		}
	}
#endif
}

void deepback_post_proc(void)
{
#if (DEEPBACK_FAST_KEYSCAN_ENABLE)
	//manual key release
	if(deepback_key_state == DEEPBACK_KEY_WAIT_RELEASE && clock_time_exceed(deepback_key_tick,150000)){
		key_not_released = 0;
		u8 		key_buf[8] = {0};
		key_buf[2] = 0;
		bls_att_pushNotifyData (HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8); //release
		deepback_key_state = DEEPBACK_KEY_IDLE;
	}
#endif
}


void key_change_proc(void)
{

	latest_user_event_tick = clock_time();  //record latest key change time

	if(key_voice_press){  //clear voice key press flg
		key_voice_press = 0;
	}



	u8 key0 = kb_event.keycode[0];
	u8 key1 = kb_event.keycode[1];
	u8 key_value;
	u8 key_buf[8] = {0,0,0,0,0,0,0,0};

	key_not_released = 1;
	if (kb_event.cnt == 2)   //two key press, do  not process
	{
#if (BLE_PHYTEST_MODE != PHYTEST_MODE_DISABLE)  //"enter + back" trigger PhyTest
		//notice that if IR enable, trigger keys must be defined in key map
		if ( (key0 == VK_ENTER && key1 == VK_0) || (key0 == VK_0 && key1 == VK_ENTER))
		{
			extern void app_phytest_init(void);
			extern void app_trigger_phytest_mode(void);
			app_phytest_init();
			app_trigger_phytest_mode();

			device_led_setup(led_cfg[LED_SHINE_FAST]);
		}
#endif
	}
	else if(kb_event.cnt == 1)
	{

		if(key0 == KEY_MODE_SWITCH)
		{
			user_key_mode = !user_key_mode;

			#if (REMOTE_IR_ENABLE)
				if(user_key_mode == KEY_MODE_BLE){
					analog_write(USED_DEEP_ANA_REG, analog_read(USED_DEEP_ANA_REG)&(~IR_MODE_DEEP_FLG));
				}
				else{
					analog_write(USED_DEEP_ANA_REG, analog_read(USED_DEEP_ANA_REG)|IR_MODE_DEEP_FLG);
				}
			#endif

			#if (BLT_APP_LED_ENABLE)
				device_led_setup(led_cfg[LED_SHINE_SLOW + user_key_mode]);
			#endif
		}
#if (BLE_AUDIO_ENABLE)
		else if (key0 == VOICE)
		{
			if(ui_mic_enable){  //if voice on, voice off
				ui_enable_mic (0);
			}
			else{ //if voice not on, mark voice key press tick
				key_voice_press = 1;
				key_voice_pressTick = clock_time();
			}
		}
#endif

#if (REMOTE_IR_ENABLE)
		else if(user_key_mode == KEY_MODE_BLE)
		{

			key_value = kb_map_ble[key0];
			if(key_value >= 0xf0 ){
				key_type = CONSUMER_KEY;
				u16 consumer_key = vk_consumer_map[key_value & 0x0f];
				bls_att_pushNotifyData (HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
			}
			else
			{
				key_type = KEYBOARD_KEY;
				key_buf[2] = key_value;
				bls_att_pushNotifyData (HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8);
			}

		}
		else if(user_key_mode == KEY_MODE_IR)
		{  //IR mode
			key_value = kb_map_ir[key0];
			key_type = IR_KEY;
			if(!ir_not_released){
				ir_dispatch(TYPE_IR_SEND, 0x88, key_value);
				ir_not_released = 1;
			}
		}
		else
		{
			key_type = IDLE_KEY;
		}
#else
		else
		{
			key_value = key0;
			if(key_value >= 0xf0 ){
				key_type = CONSUMER_KEY;
				u16 consumer_key = vk_consumer_map[key_value & 0x0f];
				bls_att_pushNotifyData (HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
			}
			else
			{
				key_type = KEYBOARD_KEY;
				key_buf[2] = key_value;
				bls_att_pushNotifyData (HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8);
			}
		}

#endif

	}
	else   //kb_event.cnt == 0,  key release
	{
		key_not_released = 0;
		if(key_type == CONSUMER_KEY)
		{
			u16 consumer_key = 0;
			bls_att_pushNotifyData (HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
		}
		else if(key_type == KEYBOARD_KEY)
		{
			key_buf[2] = 0;
			bls_att_pushNotifyData (HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8); //release
		}
#if (REMOTE_IR_ENABLE)
		else if(key_type == IR_KEY)
		{
			if(ir_not_released){
				ir_not_released = 0;
				ir_dispatch(TYPE_IR_RELEASE, 0, 0);  //release
			}
		}
#endif
	}


}



#define GPIO_WAKEUP_KEYPROC_CNT				3


_attribute_data_retention_	static int gpioWakeup_keyProc_cnt = 0;
_attribute_data_retention_	static u32 keyScanTick = 0;
void proc_keyboard (u8 e, u8 *p, int n)
{


	//when key press gpio wakeup suspend, proc keyscan at least GPIO_WAKEUP_KEYPROC_CNT times
	//regardless of 8000 us interval
	if(e == BLT_EV_FLAG_GPIO_EARLY_WAKEUP){
		gpioWakeup_keyProc_cnt = GPIO_WAKEUP_KEYPROC_CNT;
	}
	else if(gpioWakeup_keyProc_cnt){
		gpioWakeup_keyProc_cnt --;
	}


	if(gpioWakeup_keyProc_cnt || clock_time_exceed(keyScanTick, 8000)){
		keyScanTick = clock_time();
	}
	else{
		return;
	}




	kb_event.keycode[0] = 0;
	int det_key = kb_scan_key (0, 1);


#if(DEEPBACK_FAST_KEYSCAN_ENABLE)
	if(deepback_key_state != DEEPBACK_KEY_IDLE){
		deepback_pre_proc(&det_key);
	}
#endif


	if (det_key){
		key_change_proc();
	}


#if (BLE_AUDIO_ENABLE)
	 //long press voice 1 second
	if(key_voice_press && !ui_mic_enable && blc_ll_getCurrentState() == BLS_LINK_STATE_CONN && \
		clock_time_exceed(key_voice_pressTick, 1000000)){

		voice_press_proc();
	}
#endif


#if(DEEPBACK_FAST_KEYSCAN_ENABLE)
	if(deepback_key_state != DEEPBACK_KEY_IDLE){
		deepback_post_proc();
	}
#endif
}


extern u32	scan_pin_need;




int gpio_test0(void)
{
	//gpio 0 toggle to see the effect
	DBG_CHN4_TOGGLE;

	return 0;
}


int gpio_test1(void)
{
	//gpio 1 toggle to see the effect
	DBG_CHN5_TOGGLE;


	return 0;

}

int gpio_test2(void)
{
	DBG_CHN6_TOGGLE;

	return 0;
}

int gpio_test3(void)
{
	DBG_CHN7_TOGGLE;

	return 0;
}






void app_ui_init_normal(void)
{



	/////////// keyboard gpio wakeup init ////////
	u32 pin[] = KB_DRIVE_PINS;
	for (int i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
	{
		cpu_set_gpio_wakeup (pin[i], Level_High,1);  //drive pin pad high wakeup deepsleep
	}

	bls_app_registerEventCallback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, &proc_keyboard);




#if (BLT_APP_LED_ENABLE)
	device_led_init(GPIO_LED, LED_ON_LEVAL);  //LED initialization
	device_led_setup(led_cfg[LED_POWER_ON]);
#endif


#if (REMOTE_IR_ENABLE)
	user_key_mode = analog_read(USED_DEEP_ANA_REG) & IR_MODE_DEEP_FLG ? KEY_MODE_IR : KEY_MODE_BLE;
#endif



#if (BLT_TEST_SOFT_TIMER_ENABLE)
	blt_soft_timer_init();
	blt_soft_timer_add(&gpio_test0, 35000);
	blt_soft_timer_add(&gpio_test1, 23000);
	blt_soft_timer_add(&gpio_test2, 27000);
	blt_soft_timer_add(&gpio_test3, 33000);
#endif

}



void app_ui_init_deepRetn(void)
{
	/////////// keyboard gpio wakeup init ////////
	u32 pin[] = KB_DRIVE_PINS;
	for (int i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
	{
		cpu_set_gpio_wakeup (pin[i], Level_High, 1);  //drive pin pad high wakeup deepsleep
	}


#if (BLT_APP_LED_ENABLE)
	device_led_init(GPIO_LED, 1);  //LED initialization
#endif

#if (REMOTE_IR_ENABLE)
	user_key_mode = analog_read(USED_DEEP_ANA_REG) & IR_MODE_DEEP_FLG ? KEY_MODE_IR : KEY_MODE_BLE;
#endif
}


