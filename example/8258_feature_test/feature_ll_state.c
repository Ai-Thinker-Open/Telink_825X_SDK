/********************************************************************************************************
 * @file     feature_ll_state.c 
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
#include "vendor/common/tl_audio.h"
#include "vendor/common/blt_soft_timer.h"
#include "vendor/common/blt_common.h"

#if (   FEATURE_TEST_MODE == TEST_ADVERTISING_ONLY || FEATURE_TEST_MODE == TEST_SCANNING_ONLY \
	 || FEATURE_TEST_MODE == TEST_ADVERTISING_IN_CONN_SLAVE_ROLE \
	 || FEATURE_TEST_MODE==TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE \
	 || FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)


#define FEATURE_PM_ENABLE								0
#define FEATURE_DEEPSLEEP_RETENTION_ENABLE				0


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

}


void	task_terminate (u8 e, u8 *p, int n)
{

}







void  func_suspend_enter (u8 e, u8 *p, int n)
{

}

#define		MY_RF_POWER_INDEX					RF_POWER_P3p01dBm

_attribute_ram_code_ void  func_suspend_exit (u8 e, u8 *p, int n)
{
	rf_set_power_level_index (MY_RF_POWER_INDEX);
}













#if (	 FEATURE_TEST_MODE == TEST_SCANNING_ONLY || FEATURE_TEST_MODE == TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE \
	  || FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)
//////////////////////////////////////////////////////////
// event call back
//////////////////////////////////////////////////////////
#define DBG_ADV_REPORT_ON_RAM 				1
#if (DBG_ADV_REPORT_ON_RAM)  //debug adv report on ram
	#define  RAM_ADV_MAX		64
	u8 AA_advRpt[RAM_ADV_MAX][48];
	u8 AA_advRpt_index = 0;
#endif

int controller_event_callback (u32 h, u8 *p, int n)
{
	if (h &HCI_FLAG_EVENT_BT_STD)		//ble controller hci event
	{
		u8 evtCode = h & 0xff;

		if(evtCode == HCI_EVT_LE_META)
		{
			u8 subEvt_code = p[0];
			if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)	// ADV packet
			{
				//after controller is set to scan state, it will report all the adv packet it received by this event

				event_adv_report_t *pa = (event_adv_report_t *)p;
				s8 rssi = (s8)pa->data[pa->len];//rssi has already plus 110.
				printf("LE advertising report (rssi:%ddb, len:%d):\n", rssi, pa->len+11);
				array_printf(p, (pa->len + 11));

				#if (DBG_ADV_REPORT_ON_RAM)
					if(pa->len > 31){
						pa->len = 31;
					}
					memcpy( (u8 *)AA_advRpt[AA_advRpt_index++],  p, pa->len + 11);
					if(AA_advRpt_index >= RAM_ADV_MAX){
						AA_advRpt_index = 0;
					}
				#endif
			}
		}
	}

}
#endif







void feature_linklayer_state_test_init_normal(void)
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
	blc_ll_initConnection_module();						//connection module  mandatory for BLE slave/master


#if (FEATURE_TEST_MODE == TEST_ADVERTISING_ONLY)

	printf("\n\rtst adv only\n");
	blc_ll_initAdvertising_module(mac_public);


	u8 tbl_advData[] = {
		 0x08, 0x09, 't', 'e', 's', 't', 'a', 'd', 'v',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp [] = {
			 0x08, 0x09, 'T', 'E', 'S', 'T', 'A', 'D', 'V',	//scan name
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_100MS, ADV_INTERVAL_100MS, \
									ADV_TYPE_NONCONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_NONE);


	if(status != BLE_SUCCESS){  //adv setting err
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}

	blc_ll_setAdvCustomedChannel(37, 38, 39);
	bls_ll_setAdvEnable(1);  //adv enable

#elif (FEATURE_TEST_MODE == TEST_SCANNING_ONLY)
	printf("\n\rtst scan only\n");
	blc_ll_initScanning_module(mac_public);
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(controller_event_callback);


	////// set scan parameter and scan enable /////
	#if 1  //report all adv
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_90MS, SCAN_INTERVAL_90MS,
							  	  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	#else //report adv only in whitelist
		ll_whiteList_reset();
		u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
		ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_90MS, SCAN_INTERVAL_90MS,
							  	  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL);
	#endif

	blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);

#elif (FEATURE_TEST_MODE == TEST_ADVERTISING_IN_CONN_SLAVE_ROLE)
	printf("\n\rtst adv in conn slave role\n");
	blc_ll_initAdvertising_module(mac_public); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,


	////// Host Initialization  //////////
	blc_gap_peripheral_init();    //gap initialization
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	blc_smp_peripheral_init();									//smp initialization

///////////////////// USER application initialization ///////////////////
	u8 tbl_advData[] = {
		 0x09, 0x09, 's', 'l', 'a', 'v', 'e', 'a', 'd', 'v',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp [] = {
			 0x09, 0x09, 'S', 'L', 'A', 'V', 'E', 'A', 'D', 'V',
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, \
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);
	if(status != BLE_SUCCESS){  //adv setting err
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}


	bls_ll_setAdvEnable(1);  //adv enable


	//add advertising in connection slave role
	u8 tbl_advData_test[] = {
		 0x09, 0x09, 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp_test [] = {
			 0x09, 0x09, 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B',
		};
	blc_ll_addAdvertisingInConnSlaveRole();  //adv in conn slave role
	blc_ll_setAdvParamInConnSlaveRole(  (u8 *)tbl_advData_test, sizeof(tbl_advData_test), \
										(u8 *)tbl_scanRsp_test, sizeof(tbl_scanRsp_test), \
										ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);

	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);

#elif (FEATURE_TEST_MODE == TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE)
	printf("\n\rtst scan in adv and conn slave role\n");
	blc_ll_initAdvertising_module(mac_public); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,


	////// Host Initialization  //////////
	blc_gap_peripheral_init();    //gap initialization
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	blc_smp_peripheral_init();									//smp initialization

///////////////////// USER application initialization ///////////////////
	u8 tbl_advData[] = {
		 0x0A, 0x09, 's', 'l', 'a', 'v', 'e', 's', 'c', 'a', 'n',
		 0x02, 0x01, 0x05,
		};
	u8	tbl_scanRsp [] = {
			 0x0A, 0x09, 'S', 'L', 'A', 'V', 'E', 'S', 'C', 'A','N'
		};
	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, \
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_37, ADV_FP_NONE);
	if(status != BLE_SUCCESS){  //adv setting err
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}


	bls_ll_setAdvEnable(1);  //adv enable



	//scan setting
	blc_ll_initScanning_module(mac_public);
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(controller_event_callback);

	#if 1  //report all adv
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	#else //report adv only in whitelist
		ll_whiteList_reset();
		u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
		ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL);

	#endif

	blc_ll_addScanningInAdvState();  //add scan in adv state
	blc_ll_addScanningInConnSlaveRole();  //add scan in conn slave role




	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);


#elif (FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)
	blc_ll_initAdvertising_module(mac_public); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,


	////// Host Initialization  //////////
	blc_gap_peripheral_init();    //gap initialization
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	blc_smp_peripheral_init();									//smp initialization

///////////////////// USER application initialization ///////////////////
	u8	tbl_advData[] = {
		 0x05, 0x09, 'f', 'h', 'i', 'd',
		 0x02, 0x01, 0x05,
		 0x03, 0x19, 0x80, 0x01,
		 0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,
	};

	u8	tbl_scanRsp [] = {
			 0x08, 0x09, 'f', 'e', 'a', 't', 'u', 'r', 'e',
		};

	bls_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
	bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));

	u8 status = bls_ll_setAdvParam( ADV_INTERVAL_30MS, ADV_INTERVAL_35MS, \
									 ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, \
									 0,  NULL,  BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
	if(status != BLE_SUCCESS){  //adv setting err
		write_reg8(0x8000, 0x11);  //debug
		while(1);
	}


	bls_ll_setAdvEnable(1);  //adv enable



	//add advertising in connection slave role
	u8 tbl_advData_test[] = {
			 0x09, 0x09, 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
			 0x02, 0x01, 0x05,
			};
		u8	tbl_scanRsp_test [] = {
				 0x09, 0x09, 'B', 'B', 'B', 'B', 'B', 'B', 'B', 'B',
			};
		blc_ll_addAdvertisingInConnSlaveRole();  //adv in conn slave role
		blc_ll_setAdvParamInConnSlaveRole(  (u8 *)tbl_advData_test, sizeof(tbl_advData_test), \
											(u8 *)tbl_scanRsp_test, sizeof(tbl_scanRsp_test), \
											ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);



	//scan setting
	blc_ll_initScanning_module(mac_public);
	blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
	blc_hci_registerControllerEventHandler(controller_event_callback);

	#if  1  //report all adv
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	#else //report adv only in whitelist
		ll_whiteList_reset();
		u8 test_adv[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
		ll_whiteList_add(BLE_ADDR_PUBLIC, test_adv);
		blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
								  OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_WL);
	#endif

	blc_ll_addScanningInConnSlaveRole();  //add scan in conn slave role




	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);
#endif




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



_attribute_ram_code_ void feature_linklayer_state_test_init_deepRetn(void)
{
#if (FEATURE_DEEPSLEEP_RETENTION_ENABLE)
	blc_ll_initBasicMCU();   //mandatory
	rf_set_power_level_index (MY_RF_POWER_INDEX);

	blc_ll_recoverDeepRetention();

	irq_enable();

	DBG_CHN0_HIGH;    //debug
#endif
}




#endif


