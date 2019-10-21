/********************************************************************************************************
 * @file     feature_ext_adv.c
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     March. 1, 2019
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
#include "stack/ble/ble.h"
#include "tl_common.h"
#include "drivers.h"
#include "vendor/common/blt_common.h"





#if (FEATURE_TEST_MODE == TEST_EXTENDED_ADVERTISING)





#define FEATURE_PM_ENABLE								1
#define FEATURE_DEEPSLEEP_RETENTION_ENABLE				1





#define     MY_APP_ADV_CHANNEL					BLT_ENABLE_ADV_ALL




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





//////////////////////////////////////////////////////////////////////////////
//	 Adv Packet, Response Packet
//////////////////////////////////////////////////////////////////////////////
//const u8	tbl_advData[] = {
//	 0x05, 0x09, 'x', 'H', 'I', 'D',
//};


const u8	tbl_advData[] = {
	 0x05, 0x09, 'x', 'H', 'i', 'd',
	 0x02, 0x01, 0x05, 							// BLE limited discoverable mode and BR/EDR not supported
	 0x03, 0x19, 0x80, 0x01, 					// 384, Generic Remote Control, Generic category
	 0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,		// incomplete list of service class UUIDs (0x1812, 0x180F)
};

const u8	tbl_scanRsp [] = {
		 0x08, 0x09, 'x', 'R', 'e', 'm', 'o', 't', 'e',
	};


_attribute_data_retention_	int device_in_connection_state;


_attribute_data_retention_	u32	interval_update_tick;

_attribute_data_retention_	u8	sendTerminate_before_enterDeep = 0;

_attribute_data_retention_	u32	latest_user_event_tick;









#define	APP_ADV_SETS_NUMBER						1			// Number of Supported Advertising Sets
#define APP_MAX_LENGTH_ADV_DATA					1024		// Maximum Advertising Data Length,   (if legacy ADV, max length 31 bytes is enough)
#define APP_MAX_LENGTH_SCAN_RESPONSE_DATA		  31		// Maximum Scan Response Data Length, (if legacy ADV, max length 31 bytes is enough)



_attribute_data_retention_	u8  app_adv_set_param[ADV_SET_PARAM_LENGTH * APP_ADV_SETS_NUMBER];

_attribute_data_retention_	u8	app_primary_adv_pkt[MAX_LENGTH_PRIMARY_ADV_PKT * APP_ADV_SETS_NUMBER];

_attribute_data_retention_	u8	app_secondary_adv_pkt[MAX_LENGTH_SECOND_ADV_PKT * APP_ADV_SETS_NUMBER];

_attribute_data_retention_	u8 	app_advData[APP_MAX_LENGTH_ADV_DATA	* APP_ADV_SETS_NUMBER];

_attribute_data_retention_	u8 	app_scanRspData[APP_MAX_LENGTH_SCAN_RESPONSE_DATA * APP_ADV_SETS_NUMBER];





void	task_connect (u8 e, u8 *p, int n)
{
//	bls_l2cap_requestConnParamUpdate (8, 8, 99, 400);  // 1 S

}

void 	task_terminate(u8 e,u8 *p, int n) //*p is terminate reason
{


}




_attribute_ram_code_ void	user_set_rf_power (u8 e, u8 *p, int n)
{
	rf_set_power_level_index (RF_POWER_P3p01dBm);
}









void feature_ext_adv_init_normal(void)
{

	//when debugging, if long time deepSleep retention or suspend happens quickly after power on, it will make "ResetMCU" very hard, so add some time here
	sleep_us(2000000);  //only for debug


	//random number generator must be initiated here( in the beginning of user_init_nromal)
	//when deepSleep retention wakeUp, no need initialize again
	random_generator_init();  //this is must




////////////////// BLE stack initialization ////////////////////////////////////
	u8  mac_public[6];
	u8  mac_random_static[6];
	blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);



	////// Controller Initialization  //////////
	blc_ll_initBasicMCU();                      //mandatory
	blc_ll_initStandby_module(mac_public);				//mandatory
#if 1   	//Extended ADV module:
	blc_ll_initExtendedAdvertising_module(app_adv_set_param, app_primary_adv_pkt, APP_ADV_SETS_NUMBER);
	blc_ll_initExtSecondaryAdvPacketBuffer(app_secondary_adv_pkt, MAX_LENGTH_SECOND_ADV_PKT);
	blc_ll_initExtAdvDataBuffer(app_advData, APP_MAX_LENGTH_ADV_DATA);
	blc_ll_initExtScanRspDataBuffer(app_scanRspData, APP_MAX_LENGTH_SCAN_RESPONSE_DATA);
#else
	blc_ll_initAdvertising_module(mac_public); 	//adv module: 		 mandatory for BLE slave,
#endif




	rf_set_power_level_index (RF_POWER_P3p01dBm);



	blc_ll_setMaxAdvDelay_for_AdvEvent(0);  //no ADV random delay, for debug


	u32 my_adv_interval_min = ADV_INTERVAL_50MS;
	u32 my_adv_interval_max = ADV_INTERVAL_50MS;

#if 1 //Legacy, non_connectable_non_scannable

	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED,  my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_8dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   0);

	blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, sizeof(tbl_advData) , (u8 *)tbl_advData);

	blc_ll_setExtAdvEnable_1( BLC_ADV_ENABLE, 1, ADV_HANDLE0, 0 , 0);

#elif 0  //Legacy, connectable_scannable

	blc_ll_initConnection_module();				//connection module  mandatory for BLE slave/master
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,

	////// Host Initialization  //////////
	blc_gap_peripheral_init();    //gap initialization
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	blc_smp_peripheral_init(); 									//SMP initialization


	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED,  		   my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_8dBm,												   BLE_PHY_1M, 						0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   0);

	blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, sizeof(tbl_advData) , (u8 *)tbl_advData);

	blc_ll_setExtScanRspData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, sizeof(tbl_scanRsp), (u8 *)tbl_scanRsp);

	blc_ll_setExtAdvEnable_1( BLC_ADV_ENABLE, 1, ADV_HANDLE0, 0 , 0);

#elif 0 // Extended, None_Connectable_None_Scannable undirected, without auxiliary packet


	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED, my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_8dBm,												   	BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   	0);



	//blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, 0 , NULL);   //do not set ADV data, or set it with adv_dataLen "0"

	blc_ll_setExtAdvEnable_1( BLC_ADV_ENABLE, 1, ADV_HANDLE0, 0 , 0);

#elif 0 // Extended, None_Connectable_None_Scannable directed, without auxiliary packet

	u8 test_peer_type = BLE_ADDR_PUBLIC;  // BLE_ADDR_RANDOM
	u8 test_peer_mac[6] = {0x11,0x11,0x11,0x11,0x11,0x11};

	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_DIRECTED, 	my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    test_peer_type, 				test_peer_mac,
						   ADV_FP_NONE,  		TX_POWER_8dBm,												   	BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   	0);


	blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, 0 , NULL);   //do not set ADV data, or set it with adv_dataLen "0"

	blc_ll_setExtAdvEnable_1( BLC_ADV_ENABLE, 1, ADV_HANDLE0, 0 , 0);

#elif 0 // Extended, None_Connectable_None_Scannable undirected, with auxiliary packet


	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED, my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_8dBm,												   	BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   	0);


	u8	testAdvData[1024];
	for(int i=0;i<1024;i++){
		testAdvData[i]=i;
	}

	#if 1   //AdvData: 100 bytes, check that APP_MAX_LENGTH_ADV_DATA must bigger than 100
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, 100, testAdvData);
	#elif 0 //AdvData: 251 bytes, check that APP_MAX_LENGTH_ADV_DATA must bigger than 300
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, 251, testAdvData);
	#elif 0 //AdvData: 300 bytes, check that APP_MAX_LENGTH_ADV_DATA must bigger than 300
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_FIRST,    DATA_FRAGM_ALLOWED, 251, testAdvData);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_LAST,     DATA_FRAGM_ALLOWED, 49,  testAdvData + 251);
	#elif 0 //AdvData: 600 bytes, check that APP_MAX_LENGTH_ADV_DATA must bigger than 600
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_FIRST,    DATA_FRAGM_ALLOWED, 251, testAdvData);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_INTER,    DATA_FRAGM_ALLOWED, 251, testAdvData + 251);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_LAST,     DATA_FRAGM_ALLOWED, 98,  testAdvData + 502);
	#elif 1 //AdvData: 1010 bytes,  check that APP_MAX_LENGTH_ADV_DATA must bigger than 1010
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_FIRST,    DATA_FRAGM_ALLOWED, 251, testAdvData);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_INTER,    DATA_FRAGM_ALLOWED, 251, testAdvData + 251);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_INTER,    DATA_FRAGM_ALLOWED, 251, testAdvData + 502);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_INTER,    DATA_FRAGM_ALLOWED, 251, testAdvData + 753);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_LAST,     DATA_FRAGM_ALLOWED, 6,   testAdvData + 1004);
	#endif


	blc_ll_setExtAdvEnable_1( BLC_ADV_ENABLE, 1, ADV_HANDLE0, 0 , 0);

#elif 0 // Extended, None_Connectable_None_Scannable Directed, with auxiliary packet


	u8 test_peer_type = BLE_ADDR_RANDOM;  // BLE_ADDR_RANDOM
	u8 test_peer_mac[6] = {0x11,0x11,0x11,0x11,0x11,0x11};

	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_DIRECTED, 	my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    test_peer_type, 				test_peer_mac,
						   ADV_FP_NONE,  		TX_POWER_8dBm,												   	BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   	0);



	u8	testAdvData[1024];
	for(int i=0;i<1024;i++){
		testAdvData[i]=i;
	}

	#if 1 //AdvData: 600 bytes, check that APP_MAX_LENGTH_ADV_DATA must bigger than 600
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_FIRST,    DATA_FRAGM_ALLOWED, 251, testAdvData);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_INTER,    DATA_FRAGM_ALLOWED, 251, testAdvData + 251);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_LAST,     DATA_FRAGM_ALLOWED, 98,  testAdvData + 502);
	#elif 1 //AdvData: 1010 bytes, check that APP_MAX_LENGTH_ADV_DATA must bigger than 1010
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_FIRST,    DATA_FRAGM_ALLOWED, 251, testAdvData);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_INTER,    DATA_FRAGM_ALLOWED, 251, testAdvData + 251);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_INTER,    DATA_FRAGM_ALLOWED, 251, testAdvData + 502);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_INTER,    DATA_FRAGM_ALLOWED, 251, testAdvData + 753);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_LAST,     DATA_FRAGM_ALLOWED, 6,   testAdvData + 1004);
	#endif


	blc_ll_setExtAdvEnable_1( BLC_ADV_ENABLE, 1, ADV_HANDLE0, 0 , 0);


#elif 0 // Extended, Scannable, Undirected


	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_SCANNABLE_UNDIRECTED, 					my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_8dBm,												   	BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   	0);

	//Extended Scannable Event do not have ADV data

	u8	testScanRspData[1024];
	for(int i=0;i<1024;i++){
		testScanRspData[i]=i;
	}

	#if 1
		blc_ll_setExtScanRspData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, sizeof(tbl_scanRsp) , (u8 *)tbl_scanRsp);
	#else  //ExtScanRspData: 1010 bytes,   check that APP_MAX_LENGTH_SCAN_RESPONSE_DATA must bigger than 1010
		blc_ll_setExtScanRspData( ADV_HANDLE0, DATA_OPER_FIRST,    DATA_FRAGM_ALLOWED, 251, testScanRspData);
		blc_ll_setExtScanRspData( ADV_HANDLE0, DATA_OPER_INTER,    DATA_FRAGM_ALLOWED, 251, testScanRspData + 251);
		blc_ll_setExtScanRspData( ADV_HANDLE0, DATA_OPER_INTER,    DATA_FRAGM_ALLOWED, 251, testScanRspData + 502);
		blc_ll_setExtScanRspData( ADV_HANDLE0, DATA_OPER_INTER,    DATA_FRAGM_ALLOWED, 251, testScanRspData + 753);
		blc_ll_setExtScanRspData( ADV_HANDLE0, DATA_OPER_LAST,     DATA_FRAGM_ALLOWED, 6,   testScanRspData + 1004);
	#endif


	blc_ll_setExtAdvEnable_1( BLC_ADV_ENABLE, 1, ADV_HANDLE0, 0 , 0);


#elif 0 // Extended, Connectable, Undirected

	blc_ll_initConnection_module();				//connection module  mandatory for BLE slave/master
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,

	blc_ll_initChannelSelectionAlgorithm_2_feature();

	////// Host Initialization  //////////
	blc_gap_peripheral_init();    //gap initialization
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization
	blc_smp_peripheral_init(); 									//SMP initialization

	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED, 					my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_8dBm,												   	BLE_PHY_1M, 					0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   	0);

	blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, sizeof(tbl_advData) , (u8 *)tbl_advData);


	//Extended Connectable Event do not have scan_rsp data

	blc_ll_setExtAdvEnable_1( BLC_ADV_ENABLE, 1, ADV_HANDLE0, 0 , 0);

#else

#endif






	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &task_terminate);


	///////////////////// Power Management initialization///////////////////
#if (FEATURE_PM_ENABLE)
	blc_ll_initPowerManagement_module();

	#if (FEATURE_DEEPSLEEP_RETENTION_ENABLE)
		bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
		blc_pm_setDeepsleepRetentionThreshold(80, 80);
		blc_pm_setDeepsleepRetentionEarlyWakeupTiming(250);
		blc_pm_setDeepsleepRetentionType(DEEPSLEEP_MODE_RET_SRAM_LOW32K);
	#else
		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
	#endif

		//user must set rf power index after every suspend wakeUp, cause relative setting will be reset in suspend
	bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &user_set_rf_power);
#else
	bls_pm_setSuspendMask (SUSPEND_DISABLE);
#endif








}




_attribute_ram_code_ void feature_ext_adv_init_deepRetn(void)
{
#if (FEATURE_DEEPSLEEP_RETENTION_ENABLE)

	blc_ll_initBasicMCU();   //mandatory
	rf_set_power_level_index (RF_POWER_P3p01dBm);

	blc_ll_recoverDeepRetention();

	DBG_CHN0_HIGH;    //debug
#endif
}


/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
u32 tick_loop;





#endif  // end of  (FEATURE_TEST_MODE == TEST_EXTENDED_ADVERTISING)

