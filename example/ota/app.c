/********************************************************************************************************
 * @file     app.c
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

#include "vendor/common/blt_led.h"
#include "vendor/common/blt_common.h"
#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"
#include "tinyFlash/tinyFlash.h"
#include "tinyFlash_Index.h"
#include "at_cmd.h"
#define 	ADV_IDLE_ENTER_DEEP_TIME			60  //60 s
#define 	CONN_IDLE_ENTER_DEEP_TIME			60  //60 s

#define 	MY_DIRECT_ADV_TMIE					2000000


#define     MY_APP_ADV_CHANNEL					BLT_ENABLE_ADV_ALL
#define 	MY_ADV_INTERVAL_MIN					ADV_INTERVAL_30MS
#define 	MY_ADV_INTERVAL_MAX					ADV_INTERVAL_50MS




#define		BLE_DEVICE_ADDRESS_TYPE 			BLE_DEVICE_ADDRESS_PUBLIC

_attribute_data_retention_	own_addr_type_t 	app_own_address_type = OWN_ADDRESS_PUBLIC;


#define RX_FIFO_SIZE	64
#define RX_FIFO_NUM		8

#define TX_FIFO_SIZE	40
#define TX_FIFO_NUM		16


// #if 0
// 	MYFIFO_INIT(blt_rxfifo, RX_FIFO_SIZE, RX_FIFO_NUM);
// #else
// _attribute_data_retention_  u8 		 	blt_rxfifo_b[RX_FIFO_SIZE * RX_FIFO_NUM] = {0};
// _attribute_data_retention_	my_fifo_t	blt_rxfifo = {
// 												RX_FIFO_SIZE,
// 												RX_FIFO_NUM,
// 												0,
// 												0,
// 												blt_rxfifo_b,};
// #endif


// #if 0
// 	MYFIFO_INIT(blt_txfifo, TX_FIFO_SIZE, TX_FIFO_NUM);
// #else
// 	_attribute_data_retention_  u8 		 	blt_txfifo_b[TX_FIFO_SIZE * TX_FIFO_NUM] = {0};
// 	_attribute_data_retention_	my_fifo_t	blt_txfifo = {
// 													TX_FIFO_SIZE,
// 													TX_FIFO_NUM,
// 													0,
// 													0,
// 													blt_txfifo_b,};
// #endif


//////////////////////////////////////////////////////////////////////////////
//	 Adv Packet, Response Packet
//////////////////////////////////////////////////////////////////////////////
/*u8 tbl_advData[32] = {
	 0x02, 0x01, 0x05, 							// BLE limited discoverable mode and BR/EDR not supported
	 0x03, 0x19,0x80, 0x01,  					// 384, Generic Remote Control, Generic category
	 0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,		// incomplete list of service class UUIDs (0x1812, 0x180F)
};*/
#define ADV_MAC   100
 u8 tbl_advData[ADV_MAC] = {
	  0x02, 0x01, 0x05, 						 // BLE limited discoverable mode and BR/EDR not supported
//	  0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,		 // incomplete list of service class UUIDs (0x1812, 0x180F)
};

_attribute_data_retention_  u8 ibeacon_data[30] = 
{
	0x02, 0x01, 0x05,
	0x1A, 0xFF,
	0x4C, 0x00, //公司的标志 (0x004C == Apple)
	0x02, 0x15, //iBeacon advertisement indicator
	0xB9, 0x40, 0x7F, 0x30, 0xF5, 0xF8, 0x46, 0x6E, 0xAF, 0xF9, 0x25, 0x55, 0x6B, 0x57, 0xFE, 0x6D, // iBeacon proximity uuid
	0x00, 0x01, // major 
	0x00, 0x01, // minor 
	0xc5, // calibrated Tx Power
};

u8 tbl_scanRsp [] = {
		 0x0B, 0x09, 'A', 'i', '-', 'T', 'h', 'i', 'n', 'k', 'e', 'r',
	};//此项设定蓝牙被扫描回应的名称，在微信小程序体现为localName，更多请看SKD103页


 u8 test_tbl_advData[] = {
	  0x02, 0x01, 0x0, 						 // BLE limited discoverable mode and BR/EDR not supported
	  0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,		 // incomplete list of service class UUIDs (0x1812, 0x180F)
	  0x10, 0x09, 'A', 'i', '-', 'T', 'h', 'i', 'n', 'k', 'e', 'r', 'h', 'i', 'n', 'k', 'e', 'r',
	  0x10, 0xFF, 'A', 'i', '-', 'T', 'h', 'i', 'n', 'k', 'e', 'r', 'h', 'i', 'n', 'k', 'e', 'r',
};

static u8 my_scanRsp_len = 30;
_attribute_data_retention_ u8 my_scanRsp[32] = { 0 };
_attribute_data_retention_ u8 lsleep_model = 0;

_attribute_data_retention_  u16 user_adv_interval_ms = 0;//用户自己设置的蓝牙广播间隙

_attribute_data_retention_	u32 device_in_connection_state = 0;

_attribute_data_retention_	u32 advertise_begin_tick;

_attribute_data_retention_	u32	interval_update_tick;

_attribute_data_retention_	u8	sendTerminate_before_enterDeep = 0;

_attribute_data_retention_	u32	latest_user_event_tick;

_attribute_data_retention_	u8	user_rf_power_index = 0;

_attribute_data_retention_	u32 connect_event_occurTick = 0;
_attribute_data_retention_  u32 mtuExchange_check_tick = 0;

_attribute_data_retention_ 	int  dle_started_flg = 0;

_attribute_data_retention_ 	int  mtuExchange_started_flg = 0;

#define AXK_ADV_5_0  0

#if AXK_ADV_5_0
#define	APP_ADV_SETS_NUMBER						1			// Number of Supported Advertising Sets
#define APP_MAX_LENGTH_ADV_DATA					1024		// Maximum Advertising Data Length,   (if legacy ADV, max length 31 bytes is enough)
#define APP_MAX_LENGTH_SCAN_RESPONSE_DATA		  31		// Maximum Scan Response Data Length, (if legacy ADV, max length 31 bytes is enough)



_attribute_data_retention_	u8  app_adv_set_param[ADV_SET_PARAM_LENGTH * APP_ADV_SETS_NUMBER];

_attribute_data_retention_	u8	app_primary_adv_pkt[MAX_LENGTH_PRIMARY_ADV_PKT * APP_ADV_SETS_NUMBER];

_attribute_data_retention_	u8	app_secondary_adv_pkt[MAX_LENGTH_SECOND_ADV_PKT * APP_ADV_SETS_NUMBER];

_attribute_data_retention_	u8 	app_advData[APP_MAX_LENGTH_ADV_DATA	* APP_ADV_SETS_NUMBER];

_attribute_data_retention_	u8 	app_scanRspData[APP_MAX_LENGTH_SCAN_RESPONSE_DATA * APP_ADV_SETS_NUMBER];

#endif

const u8 my_rf_power_array[10] = //发射功率梯度列表
{
	RF_POWER_N19p27dBm,//-19.27 dbm
	RF_POWER_N7p65dBm,//-7.65 dbm
	RF_POWER_N4p26dBm,//-4.26 dbm
	RF_POWER_N0p97dBm,//-0.97 dbm
	RF_POWER_P3p01dBm,// 3.01 dbm
	RF_POWER_P6p14dBm,//6.14dBm
	RF_POWER_P7p02dBm,//7.02dBm
	RF_POWER_P8p13dBm,//8.13dBm
	RF_POWER_P9p24dBm,//9.24dBm
	RF_POWER_P10p46dBm,//10.46dBm
};

u32 AUTHPWD=0xffffffff;

_attribute_data_retention_ u8  user_mtu=250;


int ble_conrang_flash_read(BLE_INTER * p)
{
	u8 len=sizeof(BLE_INTER);
	if(tinyFlash_Read(STORAGE_CONRANG, (unsigned char *)p, &len )==0)
		return 0;
	else 
		return -1;
}

void ble_conrang_flash_write(BLE_INTER * p)
{
	unsigned char len=sizeof(BLE_INTER); 
	tinyFlash_Write(STORAGE_CONRANG, (unsigned char *)p,len);
}

void app_switch_to_indirect_adv(u8 e, u8 *p, int n)
{

	bls_ll_setAdvParam( MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
						ADV_TYPE_CONNECTABLE_UNDIRECTED, app_own_address_type,
						0,  NULL,
						MY_APP_ADV_CHANNEL,
						ADV_FP_NONE);

	bls_ll_setAdvEnable(1);  //must: set adv enable
}

//extern void at_print(unsigned char * str);

void ble_remote_terminate(u8 e,u8 *p, int n) //*p is terminate reason
{
	u8 dis_log[50];
	device_in_connection_state = 0;
	set_uart_mode(1);

	if(*p == HCI_ERR_CONN_TIMEOUT){
       sprintf(dis_log,"\r\nDisconnect 0x%02x\r\n",*p);
	}
	else if(*p == HCI_ERR_REMOTE_USER_TERM_CONN){  //0x13
       sprintf(dis_log,"\r\nDisconnect 0x%02x\r\n",*p);
	}
	else if(*p == HCI_ERR_CONN_TERM_MIC_FAILURE){
       sprintf(dis_log,"\r\nDisconnect 0x%02x\r\n",*p);
	}
	else{
       sprintf(dis_log,"\r\nDisconnect 0x%02x\r\n",*p);
	}

#if (BLE_APP_PM_ENABLE)
	 //user has push terminate pkt to ble TX buffer before deepsleep
	if(sendTerminate_before_enterDeep == 1){
		sendTerminate_before_enterDeep = 2;
	}
#endif

	advertise_begin_tick = clock_time();

	//at_print((unsigned char *)"\r\n+BLE_DISCONNECTED\r\n");
    at_print(dis_log);
	gpio_write(CONN_STATE_GPIO, 0);
}

_attribute_ram_code_ void user_set_rf_power (u8 e, u8 *p, int n)
{
	rf_set_power_level_index (my_rf_power_array[user_rf_power_index]);
}

static unsigned char print_connect_state()
{
	blc_att_requestMtuSizeExchange(BLS_CONN_HANDLE, 247);
	blt_soft_timer_delete(print_connect_state);
	at_print((unsigned char *)"\r\n+BLE_CONNECTED\r\n");
}

void task_connect (u8 e, u8 *p, int n)
{

 // bls_l2cap_requestConnParamUpdate (8, 8, 19, 200);   // 200mS
//	bls_l2cap_requestConnParamUpdate (8, 8, 149, 600);  // 1.5 S
//	bls_l2cap_requestConnParamUpdate (8, 8, 199, 800);  // 2 S
//	bls_l2cap_requestConnParamUpdate (8, 8, 249, 800);  // 2.5 S
//	bls_l2cap_requestConnParamUpdate (8, 8, 299, 800);  // 3 S
if (AUTHPWD!=0xffffffff)
{
	
}
else
{
	BLE_INTER ble_conn_timeout;
	if(ble_conrang_flash_read(&ble_conn_timeout)==0)//用户自己设置的连接间隔
	{
		
		bls_l2cap_requestConnParamUpdate(ble_conn_timeout.min_interval,ble_conn_timeout.max_interval,ble_conn_timeout.latency,ble_conn_timeout.timeout);
	}else 
		bls_l2cap_requestConnParamUpdate (8, 8, 99, 400);   // 1 S

		bls_l2cap_setMinimalUpdateReqSendingTime_after_connCreate(1000*2);
}
	
	latest_user_event_tick = clock_time();

	device_in_connection_state = 1;//
	set_uart_mode(0);
	gpio_write(CONN_STATE_GPIO, 1);
	gpio_write(LOWPWR_STATE_GPIO, 1);//将低功耗状态指示置1

	//at_print((unsigned char *)"\r\n+BLE_CONNECTED\r\n");
	blt_soft_timer_add(&print_connect_state, 100000);
	connect_event_occurTick = clock_time()|1;

	interval_update_tick = clock_time() | 1; //none zero
}

void task_dle_exchange (u8 e, u8 *p, int n)
{
	ll_data_extension_t* dle_param = (ll_data_extension_t*)p;
	/*printf("----- DLE exchange: -----\n");
	printf("connEffectiveMaxRxOctets: %d\n", dle_param->connEffectiveMaxRxOctets);
	printf("connEffectiveMaxTxOctets: %d\n", dle_param->connEffectiveMaxTxOctets);
	printf("connMaxRxOctets: %d\n", dle_param->connMaxRxOctets);
	printf("connMaxTxOctets: %d\n", dle_param->connMaxTxOctets);
	printf("connRemoteMaxRxOctets: %d\n", dle_param->connRemoteMaxRxOctets);
	printf("connRemoteMaxTxOctets: %d\n", dle_param->connRemoteMaxTxOctets);
*/
	dle_started_flg = 1;
}


void task_conn_update_req (u8 e, u8 *p, int n)
{
	//at_print("+UpData\r\n");
}

void task_conn_update_done (u8 e, u8 *p, int n)
{
	//at_print("+UpData_Done\r\n");
}

_attribute_ram_code_ void  ble_sleep_enter (u8 e, u8 *p, int n)
{
	while(uart_tx_is_busy())//等待串口数据发送完成，不同波特率需要时间不同
	{
		sleep_us(10);
	};

	gpio_write(LOWPWR_STATE_GPIO, 0);//将低功耗状态指示置1
	gpio_setup_up_down_resistor(LOWPWR_STATE_GPIO, PM_PIN_PULLDOWN_100K);
	bls_pm_setWakeupSource(PM_WAKEUP_PAD);  //gpio pad wakeup suspend/deepsleep
}

_attribute_ram_code_ void  ble_suspend_wakeup (u8 e, u8 *p, int n)
{
	rf_set_power_level_index (my_rf_power_array[user_rf_power_index]);
	//at_print("ble_suspend_wakeup\r\n");
}

_attribute_ram_code_ void  ble_suspend_gpio_wakeup (u8 e, u8 *p, int n)
{
	gpio_write(LOWPWR_STATE_GPIO, 1);//将低功耗状态指示置1
	bls_pm_setSuspendMask(0);  //退出低功耗
	at_print("\r\n+WAKEUP\r\n");
}

void iBeacon_init()
{
	u8 data_len = 0;

	data_len = 2;
	tinyFlash_Read(STORAGE_IBCN_ID, ibeacon_data + 5, &data_len);

	data_len = 16;
	tinyFlash_Read(STORAGE_IUUID, ibeacon_data + 9, &data_len);

	data_len = 2;
	tinyFlash_Read(STORAGE_IMAJOR, ibeacon_data + 25, &data_len);

	data_len = 2;
	tinyFlash_Read(STORAGE_IMONOR, ibeacon_data + 27, &data_len);

	data_len = 1;
	tinyFlash_Read(STORAGE_IBCN_PWER, ibeacon_data + 29, &data_len);

	bls_ll_setAdvData((u8 *)ibeacon_data, 30);
}

extern  GPIO_PinTypeDef UART_RX_PIN;
void lsleep_enable()
{
	#if (PM_DEEPSLEEP_RETENTION_ENABLE)
		bls_pm_setSuspendMask (SUSPEND_ADV | DEEPSLEEP_RETENTION_ADV );
		blc_pm_setDeepsleepRetentionThreshold(95, 95);
		blc_pm_setDeepsleepRetentionEarlyWakeupTiming(TEST_CONN_CURRENT_ENABLE ? 220 : 240);
		blc_pm_setDeepsleepRetentionType(DEEPSLEEP_MODE_RET_SRAM_LOW32K); //default use 16k deep retention
	#else
		bls_pm_setSuspendMask (SUSPEND_ADV | SUSPEND_CONN);
	#endif

	bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_ENTER, &ble_sleep_enter);
	bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &ble_suspend_wakeup);

	bls_app_registerEventCallback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, &ble_suspend_gpio_wakeup);

	gpio_setup_up_down_resistor(UART_RX_PIN, PM_PIN_PULLUP_10K);
	cpu_set_gpio_wakeup (UART_RX_PIN, Level_Low, 1); 
	//test
//	gpio_set_func(GPIO_PB5, AS_GPIO);
//	gpio_set_output_en(GPIO_PB5, 0);		
//	gpio_set_input_en(GPIO_PB5, 1);
//	cpu_set_gpio_wakeup (GPIO_PB5, Level_High,1); 

	bls_pm_setWakeupSource(PM_WAKEUP_PAD); 
}

#if BLE_OTA_ENABLE
void app_enter_ota_mode(void)
{
	//at_print("OTA start \r\n");
	//uart_dma_enable(0, 0);
	gpio_write(GPIO_PC3, 1);
	bls_ota_setTimeout(10*60 * 1000 * 1000); //set OTA timeout  15 seconds
}




void app_debug_ota_result(int result)
{

	#if(1 && 1)  //this is only for debug

		switch(result)
		{
			case OTA_SUCCESS:
				at_print("OTA_SUCCESS\r\n");break;
			case OTA_PACKET_LOSS:
				at_print("OTA_PACKET_LOSS\r\n");break;
			case OTA_DATA_CRC_ERR:
				at_print("OTA_DATA_CRC_ERR\r\n");break;
			case OTA_WRITE_FLASH_ERR :
				at_print("OTA_WRITE_FLASH_ERR\r\n");break;
			case OTA_DATA_UNCOMPLETE:
				at_print("OTA_DATA_UNCOMPLETE\r\n");break;
			case OTA_TIMEOUT:
				at_print("OTA_TIMEOUT\r\n");break;
			case OTA_FW_CHECK_ERR:
				at_print("OTA_FW_CHECK_ERR\r\n");break;
		
				

			
		}
		
		if(result == OTA_SUCCESS){  //led for debug: OTA success
			//at_print("OTA success \r\n");
			gpio_write(GPIO_PC2, 1);
			sleep_us(500000);
			gpio_write(GPIO_PC2, 0);
			sleep_us(500000);
			gpio_write(GPIO_PC2, 1);
			sleep_us(500000);
			gpio_write(GPIO_PC2, 0);
			sleep_us(500000);
			gpio_write(GPIO_PC2, 1);
		}
		else{  //OTA fail
		//	at_print("OTA fail \r\n");
		gpio_write(GPIO_PC2, 0);

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

	
	#endif
}
#endif

int app_host_event_callback (u32 h, u8 *para, int n)
{
	u8 event = h & 0xFF;

	switch(event)
	{
		case GAP_EVT_SMP_PARING_BEAGIN:
		{
		//	printf("Pairing begin--------\r\n");
			gap_smp_paringBeginEvt_t* p =(gap_smp_paringBeginEvt_t*)para;
		//	printf("secure_conn=%d \r\n",p->secure_conn);
		//	printf("tk mode =%d--------\r\n",p->tk_method);
		}
		break;

		case GAP_EVT_SMP_PARING_SUCCESS:
		{
			gap_smp_paringSuccessEvt_t* p = (gap_smp_paringSuccessEvt_t*)para;
			//printf("Pairing success:bond flg %s\r\n", p->bonding ?"true":"false");
			printf("\r\n+BLE_PAIRING SUCCESS AND BOND  %s\r\n", p->bonding ?"TRUE":"FALSE");
			if(p->bonding_result){
		//		printf("save smp key succ\r\n");
			}
			else{
		//		printf("save smp key failed\r\n");
			}
		}
		break;

		case GAP_EVT_SMP_PARING_FAIL:
		{
			gap_smp_paringFailEvt_t* p = (gap_smp_paringFailEvt_t*)para;
		//	printf("Pairing failed:rsn:0x%x\r\n", p->reason);
			printf("\r\n+BLE_PAIRING FAIL rsn:0x%x\r\n",p->reason);
		}
		break;

		case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
		{
			gap_smp_connEncDoneEvt_t* p = (gap_smp_connEncDoneEvt_t*)para;
		//	printf("Connection encryption done\n");

			if(p->re_connect == SMP_STANDARD_PAIR){  //first paring
		//		printf(" first encryption-- \r\n");
			}
			else if(p->re_connect == SMP_FAST_CONNECT){  //auto connect
		//		printf(" auto connect-- \r\n");

			}
		}
		break;

		case GAP_EVT_SMP_TK_DISPALY:
		{
			char pc[7];
		 if(AUTHPWD!=0xffffffff)
			{
				u32 pinCode = AUTHPWD;
		
				memset(smp_param_own.paring_tk,0,16);
				memcpy(smp_param_own.paring_tk,&pinCode,4);
			
				
			}
			
		/*	
			{
				u32 pinCode = *(u32*)para;
				sprintf(pc, "%d", pinCode);
				printf("TK display:%s\n", pc);
			}
		*/	
		}
		break;

		case GAP_EVT_SMP_TK_REQUEST_PASSKEY://30s not do entry then Pairing failed
		{
		//	printf("TK Request passkey  need pk entry\r\n");
		}
		break;

		case GAP_EVT_SMP_TK_REQUEST_OOB://30s not do entry then Pairing failed
		{
		//	printf("TK Request OOB need OOB entry\r\n");
		}
		break;

		case GAP_EVT_SMP_TK_NUMERIC_COMPARE:
		{
			char pc[7];
			u32 pinCode = *(u32*)para;
		//	sprintf(pc, "%d", pinCode);
		//	printf("TK numeric comparison:%s\n", pc);
		}
		break;
		case GAP_EVT_ATT_EXCHANGE_MTU:
		{
			gap_gatt_mtuSizeExchangeEvt_t *pEvt = (gap_gatt_mtuSizeExchangeEvt_t *)para;
		//	printf("MTU Peer MTU(%d)/Effect ATT MTU(%d).\n", pEvt->peer_MTU, pEvt->effective_MTU);
	
		
			mtuExchange_started_flg = 1;   //set MTU size exchange flag here
		}
		break;

		default:
		break;
	}

	return 0;
}


_attribute_data_retention_ u8  mac_public[6];
_attribute_data_retention_ u8  mac_random_static[6];


extern u8 TelinkSppServiceUUID[16];
void ble_slave_init_normal(void)
{
	//random number generator must be initiated here( in the beginning of user_init_nromal)
	//when deepSleep retention wakeUp, no need initialize again

	random_generator_init();  //this is must

////////////////// BLE stack initialization ////////////////////////////////////
	blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);

	#if(BLE_DEVICE_ADDRESS_TYPE == BLE_DEVICE_ADDRESS_PUBLIC)
		app_own_address_type = OWN_ADDRESS_PUBLIC;
	#elif(BLE_DEVICE_ADDRESS_TYPE == BLE_DEVICE_ADDRESS_RANDOM_STATIC)
		app_own_address_type = OWN_ADDRESS_RANDOM;
		blc_ll_setRandomAddr(mac_random_static);
	#endif

	////// Controller Initialization  //////////
	blc_ll_initBasicMCU();                      //mandatory
	blc_ll_initStandby_module(mac_public);		//mandatory
//	blc_ll_initAdvertising_module(mac_public); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initConnection_module();				//connection module  mandatory for BLE slave/master
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,
	
#if AXK_ADV_5_0   	//Extended ADV module:(core 5.0)
		blc_ll_initExtendedAdvertising_module(app_adv_set_param, app_primary_adv_pkt, APP_ADV_SETS_NUMBER);
		blc_ll_initExtSecondaryAdvPacketBuffer(app_secondary_adv_pkt, MAX_LENGTH_SECOND_ADV_PKT);
		blc_ll_initExtAdvDataBuffer(app_advData, APP_MAX_LENGTH_ADV_DATA);
		blc_ll_initExtScanRspDataBuffer(app_scanRspData, APP_MAX_LENGTH_SCAN_RESPONSE_DATA);
#else
		blc_ll_initAdvertising_module(mac_public);	//adv module:		 mandatory for BLE slave,
#endif


	////// Host Initialization  //////////
	blc_gap_peripheral_init();    //gap initialization
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	u8 user_mtu_len=1;
	if(tinyFlash_Read(STORAGE_MTU,&user_mtu, &user_mtu_len) == 0)
	{
		blc_att_setRxMtuSize(user_mtu);
	}else
	{
		user_mtu=MTU_SIZE_SETTING;
		blc_att_setRxMtuSize(user_mtu);
	}
	blc_l2cap_register_handler(blc_l2cap_packet_receive);  	//l2cap initialization

	//Smp Initialization may involve flash write/erase(when one sector stores too much information,
	//   is about to exceed the sector threshold, this sector must be erased, and all useful information
	//   should re_stored) , so it must be done after battery check

	u8 authpwd_len=4;
	if(tinyFlash_Read(STORAGE_AUTHPWD,&AUTHPWD, &authpwd_len) == 0 && AUTHPWD!=0xffffffff) //设置了 或者设置后又不想用了
	 {
	 //	printf("smp init--------\r\n");
		blc_smp_param_setBondingDeviceMaxNumber(SMP_BONDING_DEVICE_MAX_NUM);    //if not set, default is : SMP_BONDING_DEVICE_MAX_NUM

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

		blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_NOT_SEND, 1000); //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection )

		//host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
		
	 }else
	 	{
			blc_smp_setSecurityLevel(No_Security);
			//printf("AUTHPWD=%04x\r\n",AUTHPWD);
	 	}

	 blc_gap_registerHostEventHandler( app_host_event_callback );
			 blc_gap_setEventMask( GAP_EVT_MASK_SMP_PARING_BEAGIN			 |	\
								   GAP_EVT_MASK_SMP_PARING_SUCCESS			 |	\
								   GAP_EVT_MASK_SMP_PARING_FAIL 			 |	\
								   GAP_EVT_MASK_SMP_TK_DISPALY			 |	\
								   GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE|GAP_EVT_MASK_ATT_EXCHANGE_MTU);

///////////////////// USER application initialization ///////////////////
	u8 my_mac_uuid[8]={0};//mac 6字节 uuid2字节
	my_mac_uuid[0]=mac_public[5];
	my_mac_uuid[1]=mac_public[4];
	my_mac_uuid[2]=mac_public[3];
	my_mac_uuid[3]=mac_public[2];
	my_mac_uuid[4]=mac_public[1];
	my_mac_uuid[5]=mac_public[0];
	#if 1
	u8 name_len=10;
	if( tinyFlash_Read(STORAGE_NAME, tbl_advData + 5, &name_len) == 0)
	{
		tbl_advData[9] = name_len + 1;
		tbl_advData[10] = 0x09;
	}else
	{
		name_len=10;
		memcpy(tbl_advData + 3,tbl_scanRsp,12);
	}

	
	my_scanRsp_len = 16;
	if( tinyFlash_Read(STORAGE_ADVDATA, tbl_advData + 4+name_len+3, &my_scanRsp_len) == 0) //用户自定义厂商数据
	{
	}
	else //默认厂商数据为MAC地址，解决iOS设备无法获取MAC地址的问题 在mac地址后面再加2位uuid
	{
		
	
		//memcpy(my_mac_uuid+6,TelinkSppServiceUUID,2);
		my_mac_uuid[6]=TelinkSppServiceUUID[1];
		my_mac_uuid[7]=TelinkSppServiceUUID[0];
		
		//at_print_array(my_mac_uuid,8);
		my_scanRsp_len = 8;
		memcpy(tbl_advData + 4+name_len+3, my_mac_uuid, 8);
	}
	tbl_advData[4+name_len+1] = my_scanRsp_len + 1;
	tbl_advData[4+name_len+2] = 0xff;
	
	bls_ll_setAdvData((u8 *)tbl_advData, 4+name_len+3+my_scanRsp_len);

	//at_print_array(tbl_advData, 10+name_len+3+my_scanRsp_len);
	#else 
	u8 name_len=10;
	if( tinyFlash_Read(STORAGE_NAME, tbl_advData + 11, &name_len) == 0)
	{
		tbl_advData[9] = name_len + 1;
		tbl_advData[10] = 0x09;
	}else
	{
		name_len=10;
		memcpy(tbl_advData + 9,tbl_scanRsp,12);
	}

	
	my_scanRsp_len = 16;
	if( tinyFlash_Read(STORAGE_ADVDATA, tbl_advData + 10+name_len+3, &my_scanRsp_len) == 0) //用户自定义厂商数据
	{
	}
	else //默认厂商数据为MAC地址，解决iOS设备无法获取MAC地址的问题 在mac地址后面再加2位uuid
	{
		
	
		//memcpy(my_mac_uuid+6,TelinkSppServiceUUID,2);
		my_mac_uuid[6]=TelinkSppServiceUUID[1];
		my_mac_uuid[7]=TelinkSppServiceUUID[0];
		
	//	at_print_array(my_mac_uuid,8);
		my_scanRsp_len = 8;
		memcpy(tbl_advData + 10+name_len+3, my_mac_uuid, 8);
	}
	tbl_advData[10+name_len+1] = my_scanRsp_len + 1;
	tbl_advData[10+name_len+2] = 0xff;
	printf("10+name_len+3+my_scanRsp_len=%d\r\n",10+name_len+3+my_scanRsp_len);
	
	#if AXK_ADV_5_0
	//blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, 10+name_len+3+my_scanRsp_len , (u8 *)tbl_advData);
	blc_ll_setExtAdvData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, sizeof(test_tbl_advData), (u8 *)test_tbl_advData);

	#else
	bls_ll_setAdvData((u8 *)tbl_advData, 10+name_len+3+my_scanRsp_len);
	#endif

	#endif

	my_scanRsp_len = 30;
	if( tinyFlash_Read(STORAGE_NAME, my_scanRsp + 2, &my_scanRsp_len) == 0) //用户自定义蓝牙名称
	{
		my_scanRsp_len += 2;
		my_scanRsp[0] = my_scanRsp_len - 1;
		my_scanRsp[1] = 0x09;
		#if AXK_ADV_5_0
		blc_ll_setExtScanRspData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, my_scanRsp_len, (u8 *)my_scanRsp);

		#else
		bls_ll_setScanRspData( (u8 *)my_scanRsp, my_scanRsp_len);
		#endif
		//at_print(my_scanRsp + 2);
	}
	else //默认蓝牙名称
	{
		
		#if AXK_ADV_5_0
		blc_ll_setExtScanRspData( ADV_HANDLE0, DATA_OPER_COMPLETE, DATA_FRAGM_ALLOWED, sizeof(tbl_scanRsp), (u8 *)tbl_scanRsp);

		#else
		bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));
		#endif
	}

	extern u8 device_mode;
	if(device_mode == 2) //iBeacon 模式
	{
		iBeacon_init();
	}

	////////////////// config adv packet /////////////////////
#if (BLE_REMOTE_SECURITY_ENABLE)
	u8 bond_number = blc_smp_param_getCurrentBondingDeviceNumber();  //get bonded device number
	//printf("get bond_number=%d\r\n",bond_number);
	smp_param_save_t  bondInfo;
	if(bond_number)   //at least 1 bonding device exist
	{
		bls_smp_param_loadByIndex( bond_number - 1, &bondInfo);  //get the latest bonding device (index: bond_number-1 )

	}

	if(bond_number)   //set direct adv
	{
		//set direct adv
		u8 status = bls_ll_setAdvParam( MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
										ADV_TYPE_CONNECTABLE_DIRECTED_LOW_DUTY, app_own_address_type,
										bondInfo.peer_addr_type,  bondInfo.peer_addr,
										MY_APP_ADV_CHANNEL,
										ADV_FP_NONE);
		if(status != BLE_SUCCESS) { write_reg8(0x40002, 0x11); 	while(1);}  //debug: adv setting err

		//it is recommended that direct adv only last for several seconds, then switch to indirect adv
		bls_ll_setAdvDuration(MY_DIRECT_ADV_TMIE, 1);
		bls_app_registerEventCallback (BLT_EV_FLAG_ADV_DURATION_TIMEOUT, &app_switch_to_indirect_adv);

	}
	else   //set indirect adv
#endif
	{
		my_scanRsp_len = 2;
			u8 status;
		if(tinyFlash_Read(STORAGE_ADVINTV, &user_adv_interval_ms, &my_scanRsp_len) == 0) //读取用户是否设置广播间隙
		{
			u16  interval = user_adv_interval_ms * 16; //广播间隙的值等于 mS数 * 1.6
			interval = (u16)(interval / 10);
			#if AXK_ADV_5_0
			
			status=blc_ll_setExtAdvParam( ADV_HANDLE0, 		(device_mode == 2)?ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED:ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED,		 
										interval, 			interval,
								   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
								   ADV_FP_NONE, 		TX_POWER_8dBm,												   BLE_PHY_1M,						0,
								   BLE_PHY_1M,			ADV_SID_0,													   0);

			#else
			status = bls_ll_setAdvParam( interval, interval,
											(device_mode == 2)?ADV_TYPE_NONCONNECTABLE_UNDIRECTED:ADV_TYPE_CONNECTABLE_UNDIRECTED, 
											app_own_address_type,
											0,  NULL,
											MY_APP_ADV_CHANNEL,
											ADV_FP_NONE);
			
			#endif
			if(status != BLE_SUCCESS) { write_reg8(0x40002, 0x11); 	while(1); }  //debug: adv setting err
			
		}
		else //使用默认广播间隙
		{
			#if AXK_ADV_5_0
			
			 status=blc_ll_setExtAdvParam( ADV_HANDLE0, 		(device_mode == 2)?ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED:ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED,		 
										MY_ADV_INTERVAL_MIN, 			MY_ADV_INTERVAL_MIN,
								   BLT_ENABLE_ADV_ALL,	OWN_ADDRESS_PUBLIC, 										   BLE_ADDR_PUBLIC, 				NULL,
								   ADV_FP_NONE, 		TX_POWER_8dBm,												   BLE_PHY_1M,						0,
								   BLE_PHY_1M,			ADV_SID_0,													   0);
			#else
			 status = bls_ll_setAdvParam(  MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
											(device_mode == 2)?ADV_TYPE_NONCONNECTABLE_UNDIRECTED:ADV_TYPE_CONNECTABLE_UNDIRECTED,
											app_own_address_type,
											0,  NULL,
											MY_APP_ADV_CHANNEL,
											ADV_FP_NONE);
				
			#endif
		if(status != BLE_SUCCESS) { write_reg8(0x40002, 0x11); 	while(1); }  //debug: adv setting err
		}
	}

#if AXK_ADV_5_0
	blc_ll_setExtAdvEnable_1( BLC_ADV_ENABLE, 1, ADV_HANDLE0, 0 , 0);			
#else
	bls_ll_setAdvEnable(1);  //adv enable
#endif

	u8 buff_len = 1;
	if(tinyFlash_Read(STORAGE_RFPWR, &user_rf_power_index, &buff_len) == 0) //读取用户是否设置发射功率
	{
		if(user_rf_power_index > 9 ) user_rf_power_index = MY_RF_POWER_INDEX; //发射功率非法
	}
	else
	{
		user_rf_power_index = MY_RF_POWER_INDEX; //设为默认发射功率
	}
	
	//set rf power index, user must set it after every suspend wakeup, cause relative setting will be reset in suspend
	user_set_rf_power(0, 0, 0);
	bls_app_registerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &user_set_rf_power);

	//ble event call back
	bls_app_registerEventCallback (BLT_EV_FLAG_CONNECT, &task_connect);
	bls_app_registerEventCallback (BLT_EV_FLAG_TERMINATE, &ble_remote_terminate);

	bls_app_registerEventCallback (BLT_EV_FLAG_CONN_PARA_REQ, &task_conn_update_req);
	bls_app_registerEventCallback (BLT_EV_FLAG_CONN_PARA_UPDATE, &task_conn_update_done);
	bls_app_registerEventCallback (BLT_EV_FLAG_DATA_LENGTH_EXCHANGE, &task_dle_exchange);

	///////////////////// Power Management initialization///////////////////
#if(BLE_APP_PM_ENABLE)
	blc_ll_initPowerManagement_module();

	my_scanRsp_len = 1;
	tinyFlash_Read(STORAGE_LSLEEP, &lsleep_model, &my_scanRsp_len); //读取用户是否设置开机即进入睡眠

	if(lsleep_model) //开机即进入睡眠模式
	{
		lsleep_enable();
		gpio_write(LOWPWR_STATE_GPIO, 0);//将低功耗状态指示置0
	}
	else
	{
		gpio_write(LOWPWR_STATE_GPIO, 1);//将低功耗状态指示置1
	}
	
#else
	bls_pm_setSuspendMask (SUSPEND_DISABLE);
#endif

	#if BLE_OTA_ENABLE
			////////////////// OTA relative ////////////////////////
			bls_ota_clearNewFwDataArea(); //must
			bls_ota_registerStartCmdCb(app_enter_ota_mode);
			bls_ota_registerResultIndicateCb(app_debug_ota_result);  //debug
	#endif
	advertise_begin_tick = clock_time();
}


_attribute_ram_code_ void ble_slave_init_deepRetn(void)
{
	blc_ll_initBasicMCU();   //mandatory
	rf_set_power_level_index (my_rf_power_array[user_rf_power_index]);

	blc_ll_recoverDeepRetention();

	if(pm_is_deepPadWakeup()) //如果是GPIO唤醒
	{
		bls_pm_setSuspendMask(0);  //退出低功耗
	}
	else
	{
		cpu_set_gpio_wakeup (UART_RX_PIN, Level_Low, 1); 
	}

	irq_enable();
}


void feature_sdle_test_mainloop(void)
{
	if(connect_event_occurTick && clock_time_exceed(connect_event_occurTick, 1500000)){  //1.5 S after connection established
		connect_event_occurTick = 0;

		mtuExchange_check_tick = clock_time() | 1;
		if(!mtuExchange_started_flg){  //master do not send MTU exchange request in time
			blc_att_requestMtuSizeExchange(BLS_CONN_HANDLE, user_mtu);
		//	printf("After conn 1.5s, S send  MTU size %d req to the Master.\n",user_mtu);
		}


	}


	if(user_mtu>128)
	{
		if(mtuExchange_check_tick && clock_time_exceed(mtuExchange_check_tick, 500000 )){  //2 S after connection established
			mtuExchange_check_tick = 0;

			if(!dle_started_flg){ //master do not send data length request in time
				//printf("Master hasn't initiated the DLE yet, S send DLE req to the Master.\n");
				blc_ll_exchangeDataLength(LL_LENGTH_REQ , DLE_TX_SUPPORTED_DATA_LEN);
			}
		}
	}
}

