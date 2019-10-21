/********************************************************************************************************
 * @file     feature_security.c 
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
#include "app.h"
#include <stack/ble/ble.h>
#include "tl_common.h"
#include "drivers.h"
#include "app_config.h"
#include "vendor/common/blt_led.h"
#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"
#include "vendor/common/blt_soft_timer.h"
#include "vendor/common/blt_common.h"



#if (FEATURE_TEST_MODE == TEST_SMP_SECURITY)



#define FEATURE_PM_ENABLE								0
#define FEATURE_DEEPSLEEP_RETENTION_ENABLE				0



// LE_Security_Mode_1_Level_1, no authentication and no encryption
#define 	SMP_TEST_NO_SECURITY				1


// LE_Security_Mode_1_Level_2, unauthenticated paring with encryption
#define 	SMP_TEST_LEGACY_PARING_JUST_WORKS	2 //JustWorks
#define 	SMP_TEST_SC_PARING_JUST_WORKS		3 //JustWorks

// LE_Security_Mode_1_Level_3, authenticated paring with encryption
#define 	SMP_TEST_LEGACY_PASSKEY_ENTRY_SDMI	4 //PK_Resp_Dsply_Init_Input
#define 	SMP_TEST_LEGACY_PASSKEY_ENTRY_MDSI	5 //PK_Init_Dsply_Resp_Input
#define 	SMP_TEST_LEGACY_PASSKEY_ENTRY_MISI	6 //PK_BOTH_INPUT, not test
#define 	SMP_TEST_LEGACY_PASSKEY_ENTRY_OOB	7 //OOB_Authentication, not test

// LE_Security_Mode_1_Level_4, authenticated paring with encryption
#define 	SMP_TEST_SC_NUMERIC_COMPARISON		8 //Numric_Comparison
#define 	SMP_TEST_SC_PASSKEY_ENTRY_SDMI		9 //PK_Resp_Dsply_Init_Input
#define 	SMP_TEST_SC_PASSKEY_ENTRY_MDSI		10//PK_Init_Dsply_Resp_Input
#define 	SMP_TEST_SC_PASSKEY_ENTRY_MISI		11//PK_BOTH_INPUT, not test
#define 	SMP_TEST_SC_PASSKEY_ENTRY_OOB		12//OOB_Authentication, not test




#define     SMP_TEST_MODE						SMP_TEST_LEGACY_PARING_JUST_WORKS


#if (SMP_TEST_MODE == SMP_TEST_SC_NUMERIC_COMPARISON || \
	 SMP_TEST_MODE == SMP_TEST_SC_PASSKEY_ENTRY_MDSI || SMP_TEST_MODE == SMP_TEST_SC_PASSKEY_ENTRY_MISI || \
	 SMP_TEST_MODE == SMP_TEST_LEGACY_PASSKEY_ENTRY_MISI || SMP_TEST_MODE == SMP_TEST_LEGACY_PASSKEY_ENTRY_MDSI)

_attribute_data_retention_	u8 		key_type;
_attribute_data_retention_	int 	key_not_released;
_attribute_data_retention_  u8      digital_key_cnt;
_attribute_data_retention_  u8      tk_input[6];

static u16 vk_consumer_map[16] = {
	MKEY_VOL_UP, MKEY_VOL_DN, MKEY_MUTE, MKEY_CHN_UP, MKEY_CHN_DN, MKEY_POWER,
	MKEY_AC_SEARCH, MKEY_RECORD, MKEY_PLAY, MKEY_PAUSE, MKEY_STOP, MKEY_FAST_FORWARD,
	MKEY_FAST_FORWARD, MKEY_AC_HOME, MKEY_AC_BACK, MKEY_MENU,
};

static u8 vk_dig_map[10] = {
	VK_0, VK_1, VK_2, VK_3, VK_4, VK_5, VK_6, VK_7, VK_8, VK_9,
};

static void led_onoff(bool on_or_off)
{
	u8 onoff = on_or_off ? 1:0;
	gpio_set_output_en(GPIO_LED, onoff);
	gpio_write(GPIO_LED, onoff);
}

void  ble_remote_set_sleep_wakeup (u8 e, u8 *p, int n)
{
	if( blc_ll_getCurrentState() == BLS_LINK_STATE_CONN && ((u32)(bls_pm_getSystemWakeupTick() - clock_time())) > 80 * CLOCK_16M_SYS_TIMER_CLK_1MS){  //suspend time > 30ms.add gpio wakeup
		bls_pm_setWakeupSource(PM_WAKEUP_PAD);  //gpio pad wakeup suspend/deepsleep
	}
}

void key_change_proc(void)
{
	u8 key0 = kb_event.keycode[0];
	u8 key_value;
	u8 key_buf[8] = {0,0,0,0,0,0,0,0};

	key_not_released = 1;
	if (kb_event.cnt >= 2){   //two or more key press, do  not process

	}
	else if(kb_event.cnt == 1){
		key_value = key0;
		if(key_value >= 0xf0 ){
			key_type = CONSUMER_KEY;
			u16 consumer_key = vk_consumer_map[key_value & 0x0f];
			bls_att_pushNotifyData (HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
			printf("CK:0x%x pressed\n", consumer_key);

			#if (SMP_TEST_MODE == SMP_TEST_SC_NUMERIC_COMPARISON)
				//NOTICE:smp NC confirm (vol-)"YES" or (vol+)"NO"
				if(!(blc_smpMng.tk_status & TK_ST_NUMERIC_CHECK_YES) && !(blc_smpMng.tk_status & TK_ST_NUMERIC_CHECK_NO)){
					if(blc_smpMng.tk_status & TK_ST_NUMERIC_COMPARE){
						if(consumer_key == MKEY_VOL_DN){
							blc_smp_setNumericComparisonResult(1);// YES
							printf("confirmed YES\n");

							led_onoff(LED_ON_LEVAL);
						}
						else if(consumer_key == MKEY_VOL_UP){
							blc_smp_setNumericComparisonResult(0);// NO
							printf("confirmed NO\n");

							led_onoff(LED_ON_LEVAL);
						}
					}
				}
			#endif
		}
		else
		{
			key_type = KEYBOARD_KEY;
			key_buf[2] = key_value;
			bls_att_pushNotifyData (HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8);
			printf("KK:0x%x pressed\n", key_value);

			#if (SMP_TEST_MODE == SMP_TEST_SC_PASSKEY_ENTRY_MDSI || SMP_TEST_MODE == SMP_TEST_SC_PASSKEY_ENTRY_MISI || \
				 SMP_TEST_MODE == SMP_TEST_LEGACY_PASSKEY_ENTRY_MISI || SMP_TEST_MODE == SMP_TEST_LEGACY_PASSKEY_ENTRY_MDSI)

				if(!(blc_smpMng.tk_status & TK_ST_UPDATE)){
					if (blc_smpMng.tk_status & TK_ST_REQUEST){
						if( key_value <= VK_0 && key_value >= VK_1 && digital_key_cnt < 6){//key: 0~9
							int i;
							for(i = 0; i<10; i++){
								if(vk_dig_map[i] == key_value){
									printf("%d\n", i);
									tk_input[digital_key_cnt++] = i;
									break;
								}
							}

							led_onoff(LED_ON_LEVAL);
						}
						else if( key_value == VK_ENTER){// key: Enter/OK

							led_onoff(LED_ON_LEVAL);

							if(digital_key_cnt >= 6){
								digital_key_cnt = 0;
								u32 pincode = tk_input[0]*100000 + tk_input[1]*10000 + \
											  tk_input[2]*1000 + tk_input[3]*100 + tk_input[4]*10 + tk_input[5];
								printf("TK 6-dig value:%d\n", pincode);
								blc_smp_setTK_by_PasskeyEntry(pincode);
							}
						}
					}
				}
			#endif
		}
	}
	else   //kb_event.cnt == 0,  key release
	{
		key_not_released = 0;
		if(key_type == CONSUMER_KEY)
		{
			u16 consumer_key = 0;
			bls_att_pushNotifyData (HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
			printf("CK:released\n");
		}
		else if(key_type == KEYBOARD_KEY)
		{
			key_buf[2] = 0;
			bls_att_pushNotifyData (HID_NORMAL_KB_REPORT_INPUT_DP_H, key_buf, 8); //release
			printf("KK:released\n");
		}

		led_onoff(!LED_ON_LEVAL);
	}
}

_attribute_data_retention_ static int gpioWakeup_keyProc_cnt = 0;
_attribute_data_retention_ static u32 keyScanTick = 0;
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

	if (det_key){
		key_change_proc();
	}
}

#endif

void feature_security_test_mainloop(void)
{
#if (SMP_TEST_MODE == SMP_TEST_SC_NUMERIC_COMPARISON || \
	 SMP_TEST_MODE == SMP_TEST_SC_PASSKEY_ENTRY_MDSI || SMP_TEST_MODE == SMP_TEST_SC_PASSKEY_ENTRY_MISI || \
	 SMP_TEST_MODE == SMP_TEST_LEGACY_PASSKEY_ENTRY_MISI || SMP_TEST_MODE == SMP_TEST_LEGACY_PASSKEY_ENTRY_MDSI)

	proc_keyboard (0,0, 0);

	#if (FEATURE_DEEPSLEEP_RETENTION_ENABLE)
		bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
	#else
		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
	#endif

	int user_task_flg = scan_pin_need || key_not_released;

	if(user_task_flg){
		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
		extern int  key_matrix_same_as_last_cnt;
		if(key_matrix_same_as_last_cnt > 5){  //key matrix stable can optize
			bls_pm_setManualLatency(3);
		}
		else{
			bls_pm_setManualLatency(0);  //latency off: 0
		}
	}
#endif
}


#define RX_FIFO_SIZE	64
#define RX_FIFO_NUM		8

#define TX_FIFO_SIZE	40
#define TX_FIFO_NUM		16


_attribute_data_retention_  u8 		 	blt_rxfifo_b[RX_FIFO_SIZE * RX_FIFO_NUM] = {0};
_attribute_data_retention_	my_fifo_t	blt_rxfifo = {
												RX_FIFO_SIZE,
												RX_FIFO_NUM,
												0,
												0,
												blt_rxfifo_b,};


_attribute_data_retention_  u8 		 	blt_txfifo_b[TX_FIFO_SIZE * TX_FIFO_NUM] = {0};
_attribute_data_retention_	my_fifo_t	blt_txfifo = {
												TX_FIFO_SIZE,
												TX_FIFO_NUM,
												0,
												0,
												blt_txfifo_b,};




















void	task_connect (u8 e, u8 *p, int n)
{
	printf("connected\n");
}

volatile u8 A_dis_conn_rsn;
void	task_terminate (u8 e, u8 *p, int n)
{
	printf("terminate rsn: 0x%x\n", *p);
}


int AA_dbg_suspend;
void  func_suspend_enter (u8 e, u8 *p, int n)
{
	AA_dbg_suspend ++;
}

#define		MY_RF_POWER_INDEX					RF_POWER_P3p01dBm

_attribute_ram_code_ void  func_suspend_exit (u8 e, u8 *p, int n)
{
	rf_set_power_level_index (MY_RF_POWER_INDEX);
}









int app_host_event_callback (u32 h, u8 *para, int n)
{
	u8 event = h & 0xFF;

	switch(event)
	{
		case GAP_EVT_SMP_PARING_BEAGIN:
		{
			printf("Pairing begin\n");

			#if (SMP_TEST_MODE == SMP_TEST_SC_PASSKEY_ENTRY_MDSI || SMP_TEST_MODE == SMP_TEST_SC_PASSKEY_ENTRY_MISI || \
				 SMP_TEST_MODE == SMP_TEST_LEGACY_PASSKEY_ENTRY_MISI || SMP_TEST_MODE == SMP_TEST_LEGACY_PASSKEY_ENTRY_MDSI)
				digital_key_cnt = 0;//clr
			#endif
		}
		break;

		case GAP_EVT_SMP_PARING_SUCCESS:
		{
			gap_smp_paringSuccessEvt_t* p = (gap_smp_paringSuccessEvt_t*)para;
			printf("Pairing success:bond flg %s\n", p->bonding ?"true":"false");

			if(p->bonding_result){
				printf("save smp key succ\n");
			}
			else{
				printf("save smp key failed\n");
			}
		}
		break;

		case GAP_EVT_SMP_PARING_FAIL:
		{
			gap_smp_paringFailEvt_t* p = (gap_smp_paringFailEvt_t*)para;
			printf("Pairing failed:rsn:0x%x\n", p->reason);
		}
		break;

		case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
		{
			gap_smp_connEncDoneEvt_t* p = (gap_smp_connEncDoneEvt_t*)para;
			printf("Connection encryption done\n");

			if(p->re_connect == SMP_STANDARD_PAIR){  //first paring

			}
			else if(p->re_connect == SMP_FAST_CONNECT){  //auto connect

			}
		}
		break;

		case GAP_EVT_SMP_TK_DISPALY:
		{
			char pc[7];
			u32 pinCode = *(u32*)para;
			sprintf(pc, "%d", pinCode);
			printf("TK display:%s\n", pc);
		}
		break;

		case GAP_EVT_SMP_TK_REQUEST_PASSKEY:
		{
			printf("TK Request passkey\n");
		}
		break;

		case GAP_EVT_SMP_TK_REQUEST_OOB:
		{
			printf("TK Request OOB\n");
		}
		break;

		case GAP_EVT_SMP_TK_NUMERIC_COMPARE:
		{
			char pc[7];
			u32 pinCode = *(u32*)para;
			sprintf(pc, "%d", pinCode);
			printf("TK numeric comparison:%s\n", pc);
		}
		break;

		default:
		break;
	}

	return 0;
}

















extern void	my_att_init(void);


void feature_security_test_init_normal(void)
{

	//random number generator must be initiated here( in the beginning of user_init_nromal)
	//when deepSleep retention wakeUp, no need initialize again
	random_generator_init();  //this is must



	u8  mac_public[6];
	u8  mac_random_static[6];
	blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);

	rf_set_power_level_index (MY_RF_POWER_INDEX);

	////// Controller Initialization  //////////
	blc_ll_initBasicMCU();   //mandatory
	blc_ll_initStandby_module(mac_public);				//mandatory


	blc_ll_initAdvertising_module(mac_public); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initConnection_module();				//connection module  mandatory for BLE slave/master
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,

	////// Host Initialization  //////////
	blc_gap_peripheral_init();    //gap initialization
	my_att_init(); 		//gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization



#if ( SMP_TEST_MODE	== SMP_TEST_NO_SECURITY)

	blc_smp_setSecurityLevel(No_Authentication_No_Encryption);  // LE_Security_Mode_1_Level_1


#elif ( SMP_TEST_MODE == SMP_TEST_LEGACY_PARING_JUST_WORKS  )

	blc_smp_param_setBondingDeviceMaxNumber(4);    //if not set, default is : SMP_BONDING_DEVICE_MAX_NUM

	//set security level: "LE_Security_Mode_1_Level_2"
	blc_smp_setSecurityLevel(Unauthenticated_Paring_with_Encryption);  //if not set, default is : LE_Security_Mode_1_Level_2(Unauthenticated_Paring_with_Encryption)
	blc_smp_setBondingMode(Bondable_Mode);	// if not set, default is : Bondable_Mode
	blc_smp_setIoCapability(IO_CAPABLITY_NO_IN_NO_OUT);	// if not set, default is : IO_CAPABILITY_NO_INPUT_NO_OUTPUT

	//Smp Initialization may involve flash write/erase(when one sector stores too much information,
	//   is about to exceed the sector threshold, this sector must be erased, and all useful information
	//   should re_stored) , so it must be done after battery check
	//Notice:if user set smp parameters: it should be called after usr smp settings
	blc_smp_peripheral_init();

	blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_PEND_SEND, 1000); //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection )

	//host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
	blc_gap_registerHostEventHandler( app_host_event_callback );
	blc_gap_setEventMask( GAP_EVT_MASK_SMP_PARING_BEAGIN 			|  \
						  GAP_EVT_MASK_SMP_PARING_SUCCESS   		|  \
						  GAP_EVT_MASK_SMP_PARING_FAIL				|  \
						  GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE );

#elif ( SMP_TEST_MODE == SMP_TEST_SC_PARING_JUST_WORKS )

	blc_smp_param_setBondingDeviceMaxNumber(4);    //if not set, default is : SMP_BONDING_DEVICE_MAX_NUM

	//set security level: "LE_Security_Mode_1_Level_2"
	blc_smp_setSecurityLevel(Unauthenticated_Paring_with_Encryption);  //if not set, default is : LE_Security_Mode_1_Level_2(Unauthenticated_Paring_with_Encryption)
	blc_smp_setParingMethods(LE_Secure_Connection);
	blc_smp_setSecurityParamters(Bondable_Mode, 1, 0, 0, IO_CAPABLITY_NO_IN_NO_OUT);
	blc_smp_setEcdhDebugMode(debug_mode); //use debug mode for sniffer decryption

	//Smp Initialization may involve flash write/erase(when one sector stores too much information,
	//   is about to exceed the sector threshold, this sector must be erased, and all useful information
	//   should re_stored) , so it must be done after battery check
	//Notice:if user set smp parameters: it should be called after usr smp settings
	blc_smp_peripheral_init();

	blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_PEND_SEND, 1000); //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection )

	//host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
	blc_gap_registerHostEventHandler( app_host_event_callback );
	blc_gap_setEventMask( GAP_EVT_MASK_SMP_PARING_BEAGIN 			|  \
						  GAP_EVT_MASK_SMP_PARING_SUCCESS   		|  \
						  GAP_EVT_MASK_SMP_PARING_FAIL				|  \
						  GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE );

#elif ( SMP_TEST_MODE == SMP_TEST_LEGACY_PASSKEY_ENTRY_SDMI  )

	blc_smp_param_setBondingDeviceMaxNumber(4);    //if not set, default is : SMP_BONDING_DEVICE_MAX_NUM

	//set security level: "LE_Security_Mode_1_Level_3"
	blc_smp_setSecurityLevel(Authenticated_Paring_with_Encryption);  //if not set, default is : LE_Security_Mode_1_Level_2(Unauthenticated_Paring_with_Encryption)
	blc_smp_enableAuthMITM(1);
	blc_smp_setBondingMode(Bondable_Mode);	// if not set, default is : Bondable_Mode
	blc_smp_setIoCapability(IO_CAPABILITY_DISPLAY_ONLY);	// if not set, default is : IO_CAPABILITY_NO_INPUT_NO_OUTPUT

	//Smp Initialization may involve flash write/erase(when one sector stores too much information,
	//   is about to exceed the sector threshold, this sector must be erased, and all useful information
	//   should re_stored) , so it must be done after battery check
	//Notice:if user set smp parameters: it should be called after usr smp settings
	blc_smp_peripheral_init();

	blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_PEND_SEND, 1000); //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection )

	//host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
	blc_gap_registerHostEventHandler( app_host_event_callback );
	blc_gap_setEventMask( GAP_EVT_MASK_SMP_PARING_BEAGIN 			|  \
						  GAP_EVT_MASK_SMP_PARING_SUCCESS   		|  \
						  GAP_EVT_MASK_SMP_PARING_FAIL				|  \
						  GAP_EVT_MASK_SMP_TK_DISPALY				|  \
						  GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE );

#elif ( SMP_TEST_MODE == SMP_TEST_LEGACY_PASSKEY_ENTRY_MDSI )

	blc_smp_param_setBondingDeviceMaxNumber(4);    //if not set, default is : SMP_BONDING_DEVICE_MAX_NUM

	//set security level: "LE_Security_Mode_1_Level_3"
	blc_smp_setSecurityLevel(Authenticated_Paring_with_Encryption);  //if not set, default is : LE_Security_Mode_1_Level_2(Unauthenticated_Paring_with_Encryption)
	blc_smp_enableAuthMITM(1);
	blc_smp_setBondingMode(Bondable_Mode);	// if not set, default is : Bondable_Mode
	blc_smp_setIoCapability(IO_CAPABLITY_KEYBOARD_ONLY);	// if not set, default is : IO_CAPABILITY_NO_INPUT_NO_OUTPUT

	//Smp Initialization may involve flash write/erase(when one sector stores too much information,
	//   is about to exceed the sector threshold, this sector must be erased, and all useful information
	//   should re_stored) , so it must be done after battery check
	//Notice:if user set smp parameters: it should be called after usr smp settings
	blc_smp_peripheral_init();

	blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_PEND_SEND, 1000); //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection )

	//host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
	blc_gap_registerHostEventHandler( app_host_event_callback );
	blc_gap_setEventMask( GAP_EVT_MASK_SMP_PARING_BEAGIN 			|  \
						  GAP_EVT_MASK_SMP_PARING_SUCCESS   		|  \
						  GAP_EVT_MASK_SMP_PARING_FAIL				|  \
						  GAP_EVT_MASK_SMP_TK_DISPALY				|  \
						  GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE );

#elif ( SMP_TEST_MODE == SMP_TEST_SC_NUMERIC_COMPARISON  )

	blc_smp_param_setBondingDeviceMaxNumber(4);    //if not set, default is : SMP_BONDING_DEVICE_MAX_NUM

	//set security level: "LE_Security_Mode_1_Level_4"
	blc_smp_setSecurityLevel(Authenticated_LE_Secure_Connection_Paring_with_Encryption);  //if not set, default is : LE_Security_Mode_1_Level_2(Unauthenticated_Paring_with_Encryption)
	blc_smp_setParingMethods(LE_Secure_Connection);
	blc_smp_setSecurityParamters(Bondable_Mode, 1, 0, 0, IO_CAPABLITY_DISPLAY_YESNO);
	blc_smp_setEcdhDebugMode(debug_mode); //use debug mode for sniffer decryption

	//Smp Initialization may involve flash write/erase(when one sector stores too much information,
	//   is about to exceed the sector threshold, this sector must be erased, and all useful information
	//   should re_stored) , so it must be done after battery check
	//Notice:if user set smp parameters: it should be called after usr smp settings
	blc_smp_peripheral_init();

	blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_PEND_SEND, 1000); //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection )

	//host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
	blc_gap_registerHostEventHandler( app_host_event_callback );
	blc_gap_setEventMask( GAP_EVT_MASK_SMP_PARING_BEAGIN 			|  \
						  GAP_EVT_MASK_SMP_PARING_SUCCESS   		|  \
						  GAP_EVT_MASK_SMP_PARING_FAIL				|  \
						  GAP_EVT_MASK_SMP_TK_NUMERIC_COMPARE		|  \
						  GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE );

	/////////// keyboard gpio wakeup init ////////
	u32 pin[] = KB_DRIVE_PINS;
	for (int i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
	{
		cpu_set_gpio_wakeup (pin[i], Level_High, 1);  //drive pin pad high wakeup deepsleep
	}

	bls_app_registerEventCallback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, &proc_keyboard);
	bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_ENTER, &ble_remote_set_sleep_wakeup);

#elif ( SMP_TEST_MODE == SMP_TEST_SC_PASSKEY_ENTRY_SDMI  )

	blc_smp_param_setBondingDeviceMaxNumber(4);    //if not set, default is : SMP_BONDING_DEVICE_MAX_NUM

	//set security level: "LE_Security_Mode_1_Level_4"
	blc_smp_setSecurityLevel(Authenticated_LE_Secure_Connection_Paring_with_Encryption);  //if not set, default is : LE_Security_Mode_1_Level_2(Unauthenticated_Paring_with_Encryption)
	blc_smp_setParingMethods(LE_Secure_Connection);
	blc_smp_setSecurityParamters(Bondable_Mode, 1, 0, 0, IO_CAPABILITY_DISPLAY_ONLY);
	blc_smp_setEcdhDebugMode(debug_mode); //use debug mode for sniffer decryption

	//Smp Initialization may involve flash write/erase(when one sector stores too much information,
	//   is about to exceed the sector threshold, this sector must be erased, and all useful information
	//   should re_stored) , so it must be done after battery check
	//Notice:if user set smp parameters: it should be called after usr smp settings
	blc_smp_peripheral_init();

	blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_PEND_SEND, 1000); //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection )

	//host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
	blc_gap_registerHostEventHandler( app_host_event_callback );
	blc_gap_setEventMask( GAP_EVT_MASK_SMP_PARING_BEAGIN 			|  \
						  GAP_EVT_MASK_SMP_PARING_SUCCESS   		|  \
						  GAP_EVT_MASK_SMP_PARING_FAIL				|  \
						  GAP_EVT_MASK_SMP_TK_DISPALY				|  \
						  GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE );

#elif ( SMP_TEST_MODE == SMP_TEST_SC_PASSKEY_ENTRY_MDSI  )

	blc_smp_param_setBondingDeviceMaxNumber(4);    //if not set, default is : SMP_BONDING_DEVICE_MAX_NUM

	//set security level: "LE_Security_Mode_1_Level_4"
	blc_smp_setSecurityLevel(Authenticated_LE_Secure_Connection_Paring_with_Encryption);  //if not set, default is : LE_Security_Mode_1_Level_2(Unauthenticated_Paring_with_Encryption)
	blc_smp_setParingMethods(LE_Secure_Connection);
	blc_smp_setSecurityParamters(Bondable_Mode, 1, 0, 0, IO_CAPABLITY_KEYBOARD_ONLY);
	blc_smp_setEcdhDebugMode(debug_mode); //use debug mode for sniffer decryption

	//Smp Initialization may involve flash write/erase(when one sector stores too much information,
	//   is about to exceed the sector threshold, this sector must be erased, and all useful information
	//   should re_stored) , so it must be done after battery check
	//Notice:if user set smp parameters: it should be called after usr smp settings
	blc_smp_peripheral_init();

	blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_PEND_SEND, 1000); //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection )

	//host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
	blc_gap_registerHostEventHandler( app_host_event_callback );
	blc_gap_setEventMask( GAP_EVT_MASK_SMP_PARING_BEAGIN 			|  \
						  GAP_EVT_MASK_SMP_PARING_SUCCESS   		|  \
						  GAP_EVT_MASK_SMP_PARING_FAIL				|  \
						  GAP_EVT_MASK_SMP_TK_REQUEST_PASSKEY		|  \
						  GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE );

	/////////// keyboard gpio wakeup init ////////
	u32 pin[] = KB_DRIVE_PINS;
	for (int i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
	{
		cpu_set_gpio_wakeup (pin[i], Level_High, 1);  //drive pin pad high wakeup deepsleep
	}

	bls_app_registerEventCallback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, &proc_keyboard);
	bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_ENTER, &ble_remote_set_sleep_wakeup);


#endif


///////////////////// USER application initialization ///////////////////
	u8 tbl_advData[] = {
		 0x08, 0x09, 't', 'e', 's', 't', 'S', 'M', 'P',
		#if (1) //We'd better add this, because for some smartphones, the system may be forbidden to connect
			 0x02, 0x01, 0x05, 							// BLE limited discoverable mode and BR/EDR not supported
			 0x03, 0x19, 0x80, 0x01, 					// 384, Generic Remote Control, Generic category
			 0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,		// incomplete list of service class UUIDs (0x1812, 0x180F)
		#endif
		};
	u8	tbl_scanRsp [] = {
			 0x08, 0x09, 't', 'e', 's', 't', 'S', 'M', 'P',
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS,
						ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC,
						0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);

	bls_ll_setAdvEnable(1);  //adv enable

	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);




#if(FEATURE_PM_ENABLE)
	blc_ll_initPowerManagement_module();

	#if (FEATURE_DEEPSLEEP_RETENTION_ENABLE)
		bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
		blc_pm_setDeepsleepRetentionThreshold(50, 50);
		blc_pm_setDeepsleepRetentionEarlyWakeupTiming(200);
	#else
		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
	#endif

	//bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_ENTER, &func_suspend_enter);
	bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &func_suspend_exit);
#else
	bls_pm_setSuspendMask (SUSPEND_DISABLE);
#endif



}



_attribute_ram_code_ void feature_security_test_init_deepRetn(void)
{
#if (FEATURE_DEEPSLEEP_RETENTION_ENABLE)
	blc_ll_initBasicMCU();   //mandatory
	rf_set_power_level_index (MY_RF_POWER_INDEX);

	blc_ll_recoverDeepRetention();

	irq_enable();

	DBG_CHN0_HIGH;    //debug

#if (SMP_TEST_MODE == SMP_TEST_SC_NUMERIC_COMPARISON || \
	 SMP_TEST_MODE == SMP_TEST_SC_PASSKEY_ENTRY_MDSI || SMP_TEST_MODE == SMP_TEST_SC_PASSKEY_ENTRY_MISI || \
	 SMP_TEST_MODE == SMP_TEST_LEGACY_PASSKEY_ENTRY_MISI || SMP_TEST_MODE == SMP_TEST_LEGACY_PASSKEY_ENTRY_MDSI)

		/////////// keyboard gpio wakeup init ////////
		u32 pin[] = KB_DRIVE_PINS;
		for (int i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
		{
			cpu_set_gpio_wakeup (pin[i], Level_High, 1);  //drive pin pad high wakeup deepsleep
		}
	#endif
#endif
}




#endif
