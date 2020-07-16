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

#define 	ADV_IDLE_ENTER_DEEP_TIME			60  //60 s
#define 	CONN_IDLE_ENTER_DEEP_TIME			60  //60 s

#define 	MY_DIRECT_ADV_TMIE					2000000


#define     MY_APP_ADV_CHANNEL					BLT_ENABLE_ADV_ALL
#define 	MY_ADV_INTERVAL_MIN					ADV_INTERVAL_30MS
#define 	MY_ADV_INTERVAL_MAX					ADV_INTERVAL_35MS


#define		MY_RF_POWER_INDEX					4 //默认发射功率

const u8 my_rf_power_array[10] = //发射功率梯度列表
{
	RF_POWER_P0p04dBm,//0.04dBm
	RF_POWER_P1p73dBm,//1.93dBm
	RF_POWER_P3p01dBm,//3.01dBm
	RF_POWER_P3p94dBm,//3.94dBm
	RF_POWER_P5p13dBm,//5.13dBm
	RF_POWER_P6p14dBm,//6.14dBm
	RF_POWER_P7p02dBm,//7.02dBm
	RF_POWER_P8p13dBm,//8.13dBm
	RF_POWER_P9p24dBm,//9.24dBm
	RF_POWER_P10p46dBm,//10.46dBm
};

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
u8 tbl_advData[32] = {
	 0x02, 0x01, 0x05, 							// BLE limited discoverable mode and BR/EDR not supported
	 0x03, 0x19, 0x80, 0x01, 					// 384, Generic Remote Control, Generic category
	 0x05, 0x02, 0x12, 0x18, 0x0F, 0x18,		// incomplete list of service class UUIDs (0x1812, 0x180F)
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

const u8 tbl_scanRsp [] = {
		 0x0B, 0x09, 'A', 'i', '-', 'T', 'h', 'i', 'n', 'k', 'e', 'r',
	};//此项设定蓝牙被扫描回应的名称，在微信小程序体现为localName，更多请看SKD103页

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

void app_switch_to_indirect_adv(u8 e, u8 *p, int n)
{

	bls_ll_setAdvParam( MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
						ADV_TYPE_CONNECTABLE_UNDIRECTED, app_own_address_type,
						0,  NULL,
						MY_APP_ADV_CHANNEL,
						ADV_FP_NONE);

	bls_ll_setAdvEnable(1);  //must: set adv enable
}

extern void at_print(unsigned char * str);

void ble_remote_terminate(u8 e,u8 *p, int n) //*p is terminate reason
{
	device_in_connection_state = 0;


	if(*p == HCI_ERR_CONN_TIMEOUT){

	}
	else if(*p == HCI_ERR_REMOTE_USER_TERM_CONN){  //0x13

	}
	else if(*p == HCI_ERR_CONN_TERM_MIC_FAILURE){

	}
	else{

	}

#if (BLE_APP_PM_ENABLE)
	 //user has push terminate pkt to ble TX buffer before deepsleep
	if(sendTerminate_before_enterDeep == 1){
		sendTerminate_before_enterDeep = 2;
	}
#endif

	advertise_begin_tick = clock_time();

	at_print((unsigned char *)"\r\n+BLE_DISCONNECTED\r\n");

	gpio_write(CONN_STATE_GPIO, 0);
}

_attribute_ram_code_ void user_set_rf_power (u8 e, u8 *p, int n)
{
	rf_set_power_level_index (my_rf_power_array[user_rf_power_index]);
}

static unsigned char print_connect_state()
{
	blt_soft_timer_delete(print_connect_state);
	at_print((unsigned char *)"\r\n+BLE_CONNECTED\r\n");
}

void task_connect (u8 e, u8 *p, int n)
{
//  bls_l2cap_requestConnParamUpdate (8, 8, 19, 200);   // 200mS
	bls_l2cap_requestConnParamUpdate (8, 8, 99, 400);   // 1 S
//	bls_l2cap_requestConnParamUpdate (8, 8, 149, 600);  // 1.5 S
//	bls_l2cap_requestConnParamUpdate (8, 8, 199, 800);  // 2 S
//	bls_l2cap_requestConnParamUpdate (8, 8, 249, 800);  // 2.5 S
//	bls_l2cap_requestConnParamUpdate (8, 8, 299, 800);  // 3 S

	latest_user_event_tick = clock_time();

	device_in_connection_state = 1;//

	gpio_write(CONN_STATE_GPIO, 1);
	gpio_write(LOWPWR_STATE_GPIO, 1);//将低功耗状态指示置1

	//at_print((unsigned char *)"\r\n+BLE_CONNECTED\r\n");
	blt_soft_timer_add(&print_connect_state, 100000);

	interval_update_tick = clock_time() | 1; //none zero
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
	u8 data_len = 16;
	tinyFlash_Read(STORAGE_IUUID, ibeacon_data + 9, &data_len);

	data_len = 2;
	tinyFlash_Read(STORAGE_IMAJOR, ibeacon_data + 25, &data_len);

	data_len = 2;
	tinyFlash_Read(STORAGE_IMONOR, ibeacon_data + 27, &data_len);

	bls_ll_setAdvData((u8 *)ibeacon_data, 30);
}


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
	bls_pm_setWakeupSource(PM_WAKEUP_PAD); 
}

_attribute_data_retention_ u8  mac_public[6];
_attribute_data_retention_ u8  mac_random_static[6];
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
	blc_ll_initAdvertising_module(mac_public); 	//adv module: 		 mandatory for BLE slave,
	blc_ll_initConnection_module();				//connection module  mandatory for BLE slave/master
	blc_ll_initSlaveRole_module();				//slave module: 	 mandatory for BLE slave,


	////// Host Initialization  //////////
	blc_gap_peripheral_init();    //gap initialization
	extern void my_att_init ();
	my_att_init (); //gatt initialization
	blc_att_setRxMtuSize(250);
	blc_l2cap_register_handler(blc_l2cap_packet_receive);  	//l2cap initialization

	//Smp Initialization may involve flash write/erase(when one sector stores too much information,
	//   is about to exceed the sector threshold, this sector must be erased, and all useful information
	//   should re_stored) , so it must be done after battery check
#if (BLE_REMOTE_SECURITY_ENABLE)
	blc_smp_peripheral_init();
#else
	blc_smp_setSecurityLevel(No_Security);
#endif

///////////////////// USER application initialization ///////////////////
	my_scanRsp_len = 16;
	if( tinyFlash_Read(STORAGE_ADVDATA, tbl_advData + 15, &my_scanRsp_len) == 0) //用户自定义厂商数据
	{
	}
	else //默认厂商数据为MAC地址，解决iOS设备无法获取MAC地址的问题
	{
		my_scanRsp_len = 6;
		memcpy(tbl_advData + 15, mac_public, 6);
	}
	tbl_advData[13] = my_scanRsp_len + 1;
	tbl_advData[14] = 0xff;
	bls_ll_setAdvData((u8 *)tbl_advData, 15 + my_scanRsp_len);
	
	my_scanRsp_len = 30;
	if( tinyFlash_Read(STORAGE_NAME, my_scanRsp + 2, &my_scanRsp_len) == 0) //用户自定义蓝牙名称
	{
		my_scanRsp_len += 2;
		my_scanRsp[0] = my_scanRsp_len - 1;
		my_scanRsp[1] = 0x09;
		bls_ll_setScanRspData( (u8 *)my_scanRsp, my_scanRsp_len);
		//at_print(my_scanRsp + 2);
	}
	else //默认蓝牙名称
	{
		bls_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));
	}

	extern u8 device_mode;
	if(device_mode == 2) //iBeacon 模式
	{
		iBeacon_init();
	}

	////////////////// config adv packet /////////////////////
#if (BLE_REMOTE_SECURITY_ENABLE)
	u8 bond_number = blc_smp_param_getCurrentBondingDeviceNumber();  //get bonded device number
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
		if(status != BLE_SUCCESS) { write_reg8(0x40002, 0x11); 	while(1); }  //debug: adv setting err

		//it is recommended that direct adv only last for several seconds, then switch to indirect adv
		bls_ll_setAdvDuration(MY_DIRECT_ADV_TMIE, 1);
		bls_app_registerEventCallback (BLT_EV_FLAG_ADV_DURATION_TIMEOUT, &app_switch_to_indirect_adv);

	}
	else   //set indirect adv
#endif
	{
		my_scanRsp_len = 2;
		if(tinyFlash_Read(STORAGE_ADVINTV, &user_adv_interval_ms, &my_scanRsp_len) == 0) //读取用户是否设置广播间隙
		{
			u16  interval = user_adv_interval_ms * 16; //广播间隙的值等于 mS数 * 1.6
			interval = (u16)(interval / 10);
			u8 status = bls_ll_setAdvParam( interval, interval,
											(device_mode == 2)?ADV_TYPE_NONCONNECTABLE_UNDIRECTED:ADV_TYPE_CONNECTABLE_UNDIRECTED, 
											app_own_address_type,
											0,  NULL,
											MY_APP_ADV_CHANNEL,
											ADV_FP_NONE);
			if(status != BLE_SUCCESS) { write_reg8(0x40002, 0x11); 	while(1); }  //debug: adv setting err
		}
		else //使用默认广播间隙
		{
			u8 status = bls_ll_setAdvParam(  MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX,
											(device_mode == 2)?ADV_TYPE_NONCONNECTABLE_UNDIRECTED:ADV_TYPE_CONNECTABLE_UNDIRECTED,
											app_own_address_type,
											0,  NULL,
											MY_APP_ADV_CHANNEL,
											ADV_FP_NONE);
			if(status != BLE_SUCCESS) { write_reg8(0x40002, 0x11); 	while(1); }  //debug: adv setting err
		}
	}

	bls_ll_setAdvEnable(1);  //adv enable

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