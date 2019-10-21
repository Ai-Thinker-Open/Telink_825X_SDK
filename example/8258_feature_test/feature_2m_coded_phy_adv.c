/********************************************************************************************************
 * @file     feature_2m_coded_phy_adv.c
 *
 * @brief    for TLSR chips
 *
 * @author	 BLE Group
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





#if (FEATURE_TEST_MODE == TEST_2M_CODED_PHY_EXT_ADV)


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


const u8	tbl_advData[] = {
	 0x05, 0x09, 'x', 'H', 'i', 'd',
	 0x02, 0x01, 0x05, 							// BLE limited discoverable mode and BR/EDR not supported
	 0x03, 0x19, 0x80, 0x01, 					// 384, Generic Remote Control, Generic category
	 0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,		// incomplete list of service class UUIDs (0x1812, 0x180F)
};

const u8	tbl_scanRsp [] = {
		 0x08, 0x09, 'x', 'R', 'e', 'm', 'o', 't', 'e',
	};








#define	APP_ADV_SETS_NUMBER						1			// Number of Supported Advertising Sets
#define APP_MAX_LENGTH_ADV_DATA					1024		// Maximum Advertising Data Length,   (if legacy ADV, max length 31 bytes is enough)
#define APP_MAX_LENGTH_SCAN_RESPONSE_DATA		31		// Maximum Scan Response Data Length, (if legacy ADV, max length 31 bytes is enough)



_attribute_data_retention_	u8  app_adv_set_param[ADV_SET_PARAM_LENGTH * APP_ADV_SETS_NUMBER];

_attribute_data_retention_	u8	app_primary_adv_pkt[MAX_LENGTH_PRIMARY_ADV_PKT * APP_ADV_SETS_NUMBER];

_attribute_data_retention_	u8	app_secondary_adv_pkt[MAX_LENGTH_SECOND_ADV_PKT * APP_ADV_SETS_NUMBER];

_attribute_data_retention_	u8 	app_advData[APP_MAX_LENGTH_ADV_DATA	* APP_ADV_SETS_NUMBER];
_attribute_data_retention_	u8 	app_scanRspData[APP_MAX_LENGTH_SCAN_RESPONSE_DATA * APP_ADV_SETS_NUMBER];




_attribute_ram_code_ void	user_set_rf_power (u8 e, u8 *p, int n)
{
	rf_set_power_level_index (RF_POWER_P3p01dBm);
}






void feature_2m_coded_phy_adv_init_normal(void)
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

	//Extended ADV module:
	blc_ll_initExtendedAdvertising_module(app_adv_set_param, app_primary_adv_pkt, APP_ADV_SETS_NUMBER);
	blc_ll_initExtSecondaryAdvPacketBuffer(app_secondary_adv_pkt, MAX_LENGTH_SECOND_ADV_PKT);
	blc_ll_initExtAdvDataBuffer(app_advData, APP_MAX_LENGTH_ADV_DATA);
	blc_ll_initExtScanRspDataBuffer(app_scanRspData, APP_MAX_LENGTH_SCAN_RESPONSE_DATA);

	blc_ll_init2MPhyCodedPhy_feature();



	rf_set_power_level_index (RF_POWER_P3p01dBm);

	//blc_ll_setMaxAdvDelay_for_AdvEvent(0);  //no ADV random delay, for debug

	u32 my_adv_interval_min = ADV_INTERVAL_50MS;
	u32 my_adv_interval_max = ADV_INTERVAL_50MS;

	le_phy_type_t  user_primary_adv_phy;
	le_phy_type_t  user_secondary_adv_phy;

#if 0 // Extended, None_Connectable_None_Scannable undirected, without auxiliary packet



	#if 1      // ADV_EXT_IND: 1M PHY
		user_primary_adv_phy = BLE_PHY_1M;
	#elif 1    // ADV_EXT_IND: Coded PHY(S2)
		user_primary_adv_phy = BLE_PHY_CODED;
		blc_ll_setDefaultExtAdvCodingIndication_for_codedPhy(ADV_HANDLE0, CODED_PHY_PREFER_S2);
	#elif 0	   // ADV_EXT_IND: Coded PHY(S8)
		user_primary_adv_phy = BLE_PHY_CODED;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S8);
	#else

	#endif


	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED, my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_8dBm,												   	user_primary_adv_phy, 			0,
						   BLE_PHY_1M, 	 		ADV_SID_0, 													   	0);


	blc_ll_setExtAdvEnable_1( BLC_ADV_ENABLE, 1, ADV_HANDLE0, 0 , 0);

#elif 0 // Extended, None_Connectable_None_Scannable undirected, with auxiliary packet


	#if 1      // ADV_EXT_IND: 1M PHY;  		AUX_ADV_IND/AUX_CHAIN_IND: 1M PHY
		user_primary_adv_phy  = BLE_PHY_1M;
		user_secondary_adv_phy = BLE_PHY_1M;
	#elif 0      // ADV_EXT_IND: 1M PHY;  		AUX_ADV_IND/AUX_CHAIN_IND: 2M PHY
		user_primary_adv_phy   = BLE_PHY_1M;
		user_secondary_adv_phy = BLE_PHY_2M;
	#elif 0      // ADV_EXT_IND: 1M PHY;  		AUX_ADV_IND/AUX_CHAIN_IND: Coded PHY(S2)
		user_primary_adv_phy   = BLE_PHY_1M;
		user_secondary_adv_phy = BLE_PHY_CODED;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S2);
	#elif 0      // ADV_EXT_IND: 1M PHY;  		AUX_ADV_IND/AUX_CHAIN_IND: Coded PHY(S8)
		user_primary_adv_phy   = BLE_PHY_1M;
		user_secondary_adv_phy = BLE_PHY_CODED;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S8);


	#elif 0      // ADV_EXT_IND: Coded PHY(S2) 		AUX_ADV_IND/AUX_CHAIN_IND: 1M PHY
		user_primary_adv_phy  = BLE_PHY_CODED;
		user_secondary_adv_phy = BLE_PHY_1M;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S2);
	#elif 0      // ADV_EXT_IND: Coded PHY(S8) 		AUX_ADV_IND/AUX_CHAIN_IND: 1M PHY
		user_primary_adv_phy  = BLE_PHY_CODED;
		user_secondary_adv_phy = BLE_PHY_1M;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S8);
	#elif 0      // ADV_EXT_IND: Coded PHY(S2)  	AUX_ADV_IND/AUX_CHAIN_IND: 2M PHY
		user_primary_adv_phy   = BLE_PHY_CODED;
		user_secondary_adv_phy = BLE_PHY_2M;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S2);
	#elif 0      // ADV_EXT_IND: Coded PHY(S8)  	AUX_ADV_IND/AUX_CHAIN_IND: 2M PHY
		user_primary_adv_phy   = BLE_PHY_CODED;
		user_secondary_adv_phy = BLE_PHY_2M;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S8);
	#elif 0      // ADV_EXT_IND: Coded PHY(S2);  		AUX_ADV_IND/AUX_CHAIN_IND: Coded PHY(S2)
		user_primary_adv_phy   = BLE_PHY_CODED;
		user_secondary_adv_phy = BLE_PHY_CODED;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S2);
	#elif 0      // ADV_EXT_IND: Coded PHY(S8);  		AUX_ADV_IND/AUX_CHAIN_IND: Coded PHY(S8)
		user_primary_adv_phy   = BLE_PHY_CODED;
		user_secondary_adv_phy = BLE_PHY_CODED;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S8);
	#else

	#endif

	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED, my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_8dBm,												   	user_primary_adv_phy, 			0,
						   user_secondary_adv_phy, 	ADV_SID_0, 													0);



	u8	testAdvData[1024];
	for(int i=0;i<1024;i++){
		testAdvData[i]=i;
	}

	#if 0
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, sizeof(tbl_advData),   (u8*)tbl_advData);
	#elif 0   //AdvData: 100 bytes,  check that APP_MAX_LENGTH_ADV_DATA must bigger than 100
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, 100, testAdvData);
	#elif 0 //AdvData: 251 bytes,  check that APP_MAX_LENGTH_ADV_DATA must bigger than 251
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, 251, testAdvData);
	#elif 0 //AdvData: 300 bytes,  check that APP_MAX_LENGTH_ADV_DATA must bigger than 300
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_FIRST,    DATA_FRAGM_ALLOWED, 251, testAdvData);
		blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_LAST,     DATA_FRAGM_ALLOWED, 49,  testAdvData + 251);
	#elif 0 //AdvData: 600 bytes,  check that APP_MAX_LENGTH_ADV_DATA must bigger than 600
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


#elif 1 // Extended, Scannable, Undirected

	#if 1      // ADV_EXT_IND: 1M PHY;  		AUX_ADV_IND/AUX_CHAIN_IND: 1M PHY
		user_primary_adv_phy  = BLE_PHY_1M;
		user_secondary_adv_phy = BLE_PHY_1M;
	#elif 0      // ADV_EXT_IND: 1M PHY;  		AUX_ADV_IND/AUX_CHAIN_IND: 2M PHY
		user_primary_adv_phy   = BLE_PHY_1M;
		user_secondary_adv_phy = BLE_PHY_2M;
	#elif 0      // ADV_EXT_IND: 1M PHY;  		AUX_ADV_IND/AUX_CHAIN_IND: Coded PHY(S8)
		user_primary_adv_phy   = BLE_PHY_1M;
		user_secondary_adv_phy = BLE_PHY_CODED;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S8);


	#elif 0      // ADV_EXT_IND: Coded PHY(S8) 		AUX_ADV_IND/AUX_CHAIN_IND: 1M PHY
		user_primary_adv_phy  = BLE_PHY_CODED;
		user_secondary_adv_phy = BLE_PHY_1M;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S8);
	#elif 0      // ADV_EXT_IND: Coded PHY(S8)  	AUX_ADV_IND/AUX_CHAIN_IND: 2M PHY
		user_primary_adv_phy   = BLE_PHY_CODED;
		user_secondary_adv_phy = BLE_PHY_2M;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S8);
	#elif 0      // ADV_EXT_IND: Coded PHY(S8);  		AUX_ADV_IND/AUX_CHAIN_IND: Coded PHY(S8)
		user_primary_adv_phy   = BLE_PHY_CODED;
		user_secondary_adv_phy = BLE_PHY_CODED;
		blc_ll_setDefaultExtAdvCodingIndication(ADV_HANDLE0, CODED_PHY_PREFER_S8);
	#else

	#endif


	blc_ll_setExtAdvParam( ADV_HANDLE0, 		ADV_EVT_PROP_EXTENDED_SCANNABLE_UNDIRECTED, 					my_adv_interval_min, 			my_adv_interval_max,
						   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										    BLE_ADDR_PUBLIC, 				NULL,
						   ADV_FP_NONE,  		TX_POWER_8dBm,												   	user_primary_adv_phy, 			0,
						   user_secondary_adv_phy, 	ADV_SID_0, 													0);

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



#else



#endif





	///////////////////// Power Management initialization///////////////////
#if(FEATURE_PM_ENABLE)
	blc_ll_initPowerManagement_module();

	#if 0 //(FEATURE_DEEPSLEEP_RETENTION_ENABLE)
		bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
		blc_pm_setDeepsleepRetentionThreshold(95, 95);
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




_attribute_ram_code_ void feature_2m_coded_phy_adv_init_deepRetn(void)
{
#if (FEATURE_DEEPSLEEP_RETENTION_ENABLE)

	blc_ll_initBasicMCU();   //mandatory
	rf_set_power_level_index (RF_POWER_P3p01dBm);

	blc_ll_recoverDeepRetention();

	DBG_CHN0_HIGH;    //debug
#endif
}





#endif  // end of  (FEATURE_TEST_MODE == TEST_EXTENDED_ADVERTISING)

