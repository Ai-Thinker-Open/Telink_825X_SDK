/********************************************************************************************************
 * @file     feature_data_len_extension.c
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
#include "vendor/common/blt_common.h"




#if (FEATURE_TEST_MODE == TEST_SDATA_LENGTH_EXTENSION)



#define FEATURE_PM_ENABLE								0
#define FEATURE_DEEPSLEEP_RETENTION_ENABLE				0



#if (1) // support RF RX/TX MAX data Length: 251byte
	#define RX_FIFO_SIZE	288  //rx-24   max:251+24 = 275  16 align-> 288
	#define RX_FIFO_NUM		8

	#define TX_FIFO_SIZE	264  //tx-12   max:251+12 = 263  4 align-> 264
	#define TX_FIFO_NUM		8

	#define MTU_SIZE_SETTING   			 247
	#define DLE_TX_SUPPORTED_DATA_LEN    MAX_OCTETS_DATA_LEN_EXTENSION //264-12 = 252 > Tx max:251
#else
	#define RX_FIFO_SIZE	224 //rx-24   max:200+24 = 224  16 align-> 224
	#define RX_FIFO_NUM		8

	#define TX_FIFO_SIZE	212 //tx-12   max:200+12 = 212  4 align-> 212
	#define TX_FIFO_NUM		8

	#define MTU_SIZE_SETTING   			 196
	#define DLE_TX_SUPPORTED_DATA_LEN    (TX_FIFO_SIZE-12)
#endif






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



_attribute_data_retention_	u32 connect_event_occurTick = 0;
_attribute_data_retention_  u32 mtuExchange_check_tick = 0;

_attribute_data_retention_ 	int  dle_started_flg = 0;

_attribute_data_retention_ 	int  mtuExchange_started_flg = 0;


_attribute_data_retention_	u16  final_MTU_size = 23;


int module_onReceiveData(rf_packet_att_write_t *p)
{
	u8 len = p->l2capLen - 3;
	if(len > 0)
	{
		printf("RF_RX len: %d\nc2s:write data: %d\n", p->rf_len, len);
		array_printf(&p->value, len);

		printf("s2c:notify data: %d\n", len);
		array_printf(&p->value, len);
#if 1
		blc_gatt_pushHandleValueNotify(BLS_CONN_HANDLE, 0x11, &p->value, len);  //this API can auto handle MTU size
#else
		if( len + 3 <= final_MTU_size){   //opcode: 1 byte; attHandle: 2 bytes
			bls_att_pushNotifyData(0x11, &p->value, len);
		}
		else{
			//can not send this packet, cause MTU size exceed
		}
#endif
	}

	return 0;
}

void	task_connect (u8 e, u8 *p, int n)
{
	printf("----- connected -----\n");
	connect_event_occurTick = clock_time()|1;

	bls_l2cap_requestConnParamUpdate (8, 8, 19, 200);  //interval=10ms latency=19 timeout=2s
	bls_l2cap_setMinimalUpdateReqSendingTime_after_connCreate(1000);

	//MTU size reset to default 23 bytes every new connection, it can be only updated by MTU size exchange procedure
	final_MTU_size = 23;
}


void	task_terminate (u8 e, u8 *p, int n)
{
	printf("----- terminate rsn: 0x%x -----\n", *p);
	connect_event_occurTick = 0;
	mtuExchange_check_tick = 0;

	//MTU size exchange and data length exchange procedure must be executed on every new connection,
	//so when connection terminate, relative flags must be cleared
	dle_started_flg = 0;
	mtuExchange_started_flg = 0;

	//MTU size reset to default 23 bytes when connection terminated
	final_MTU_size = 23;
}

void	task_dle_exchange (u8 e, u8 *p, int n)
{
	ll_data_extension_t* dle_param = (ll_data_extension_t*)p;
	printf("----- DLE exchange: -----\n");
	printf("connEffectiveMaxRxOctets: %d\n", dle_param->connEffectiveMaxRxOctets);
	printf("connEffectiveMaxTxOctets: %d\n", dle_param->connEffectiveMaxTxOctets);
	printf("connMaxRxOctets: %d\n", dle_param->connMaxRxOctets);
	printf("connMaxTxOctets: %d\n", dle_param->connMaxTxOctets);
	printf("connRemoteMaxRxOctets: %d\n", dle_param->connRemoteMaxRxOctets);
	printf("connRemoteMaxTxOctets: %d\n", dle_param->connRemoteMaxTxOctets);

	dle_started_flg = 1;
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
			printf("----- Pairing begin -----\n");
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
			printf("----- Pairing failed:rsn:0x%x -----\n", p->reason);
		}
		break;

		case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
		{
			gap_smp_connEncDoneEvt_t* p = (gap_smp_connEncDoneEvt_t*)para;
			printf("----- Connection encryption done -----\n");

			if(p->re_connect == SMP_STANDARD_PAIR){  //first paring

			}
			else if(p->re_connect == SMP_FAST_CONNECT){  //auto connect

			}
		}
		break;

		case GAP_EVT_ATT_EXCHANGE_MTU:
		{
			gap_gatt_mtuSizeExchangeEvt_t *pEvt = (gap_gatt_mtuSizeExchangeEvt_t *)para;
			printf("MTU Peer MTU(%d)/Effect ATT MTU(%d).\n", pEvt->peer_MTU, pEvt->effective_MTU);
			final_MTU_size = pEvt->effective_MTU;
			mtuExchange_started_flg = 1;   //set MTU size exchange flag here
		}
		break;


		default:
		break;
	}

	return 0;
}


void feature_sdle_test_init_normal(void)
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
	my_att_init(); 		  //GATT initialization

	//ATT initialization
	//If not set RX MTU size, default is: 23 bytes.  In this situation, if master send MtuSize Request before slave send MTU size request,
	//slave will response default RX MTU size 23 bytes, then master will not send long packet on host l2cap layer, link layer data length
	//extension feature can not be used.  So in data length extension application, RX MTU size must be enlarged when initialization.
	blc_att_setRxMtuSize(MTU_SIZE_SETTING);

	blc_l2cap_register_handler (blc_l2cap_packet_receive);  	//l2cap initialization


	blc_smp_param_setBondingDeviceMaxNumber(4);    //if not set, default is : SMP_BONDING_DEVICE_MAX_NUM

	//Smp Initialization may involve flash write/erase(when one sector stores too much information,
	//   is about to exceed the sector threshold, this sector must be erased, and all useful information
	//   should re_stored) , so it must be done after battery check
	//Notice:if user set smp parameters: it should be called after usr smp settings
	blc_smp_peripheral_init();

	blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_PEND_SEND, 1000);  //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection )

	//host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
	blc_gap_registerHostEventHandler( app_host_event_callback );
	blc_gap_setEventMask( GAP_EVT_MASK_SMP_PARING_BEAGIN 			|  \
						  GAP_EVT_MASK_SMP_PARING_SUCCESS   		|  \
						  GAP_EVT_MASK_SMP_PARING_FAIL				|  \
						  GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE 	|  \
						  GAP_EVT_MASK_ATT_EXCHANGE_MTU);


///////////////////// USER application initialization ///////////////////
	u8 tbl_advData[] = {
		 0x08, 0x09, 't', 'e', 's', 't', 'D', 'L', 'E',
		};
	u8	tbl_scanRsp [] = {
			0x08, 0x09, 't', 'e', 's', 't', 'D', 'L', 'E',
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
	bls_app_registerEventCallback (BLT_EV_FLAG_DATA_LENGTH_EXCHANGE, &task_dle_exchange);


#if(FEATURE_PM_ENABLE)
	blc_ll_initPowerManagement_module();

	#if (FEATURE_DEEPSLEEP_RETENTION_ENABLE)
		bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV | SUSPEND_CONN | DEEPSLEEP_RETENTION_CONN);
		blc_pm_setDeepsleepRetentionThreshold(50, 50);
		blc_pm_setDeepsleepRetentionEarlyWakeupTiming(200);
	#else
		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
	#endif

	bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &func_suspend_exit);
#else
	bls_pm_setSuspendMask (SUSPEND_DISABLE);
#endif


}



_attribute_ram_code_ void feature_sdle_test_init_deepRetn(void)
{
#if (FEATURE_DEEPSLEEP_RETENTION_ENABLE)
	blc_ll_initBasicMCU();   //mandatory
	rf_set_power_level_index (MY_RF_POWER_INDEX);

	blc_ll_recoverDeepRetention();

	irq_enable();

	DBG_CHN0_HIGH;    //debug
#endif
}



void feature_sdle_test_mainloop(void)
{
	if(connect_event_occurTick && clock_time_exceed(connect_event_occurTick, 1500000)){  //1.5 S after connection established
		connect_event_occurTick = 0;

		mtuExchange_check_tick = clock_time() | 1;
		if(!mtuExchange_started_flg){  //master do not send MTU exchange request in time
			blc_att_requestMtuSizeExchange(BLS_CONN_HANDLE, MTU_SIZE_SETTING);
			printf("After conn 1.5s, S send  MTU size req to the Master.\n");
		}


	}



	if(mtuExchange_check_tick && clock_time_exceed(mtuExchange_check_tick, 500000 )){  //2 S after connection established
		mtuExchange_check_tick = 0;

		if(!dle_started_flg){ //master do not send data length request in time
			printf("Master hasn't initiated the DLE yet, S send DLE req to the Master.\n");
			blc_ll_exchangeDataLength(LL_LENGTH_REQ , DLE_TX_SUPPORTED_DATA_LEN);
		}
	}
}


#endif
