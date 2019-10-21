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




#if (FEATURE_TEST_MODE == TEST_MDATA_LENGTH_EXTENSION)

	#define FEATURE_PM_ENABLE								0
	#define FEATURE_DEEPSLEEP_RETENTION_ENABLE				0

	//need define att handle same with slave(Here: we use 8258 feature_test/slave_dle demo as slave device)
	#define			SPP_HANDLE_DATA_S2C			0x11
	#define			SPP_HANDLE_DATA_C2S			0x15


#if (1) // support RF RX/TX MAX data Length: 251byte
	#define RX_FIFO_SIZE						288  //rx-24   max:251+24 = 275  16 align-> 288
	#define RX_FIFO_NUM							8

	#define TX_FIFO_SIZE						264  //tx-12   max:251+12 = 263  4 align-> 264
	#define TX_FIFO_NUM							8

	#define MTU_SIZE_SETTING   			 		247
	#define DLE_TX_SUPPORTED_DATA_LEN    		MAX_OCTETS_DATA_LEN_EXTENSION //264-12 = 252 > Tx max:251
#else
	#define RX_FIFO_SIZE						224 //rx-24   max:200+24 = 224  16 align-> 224
	#define RX_FIFO_NUM							8

	#define TX_FIFO_SIZE						212 //tx-12   max:200+12 = 212  4 align-> 212
	#define TX_FIFO_NUM							16

	#define MTU_SIZE_SETTING   			 		196
	#define DLE_TX_SUPPORTED_DATA_LEN    		(BLM_TX_FIFO_SIZE-12)
#endif


MYFIFO_INIT(blt_rxfifo, RX_FIFO_SIZE, RX_FIFO_NUM);
MYFIFO_INIT(blt_txfifo, TX_FIFO_SIZE, TX_FIFO_NUM);



static u32 master_connected_led_on;
static u32 host_update_conn_param_req;
static u16 host_update_conn_min;
static u16 host_update_conn_latency;
static u16 host_update_conn_timeout;
static u32 connect_event_occurTick;
static u32 mtuExchange_check_tick;
static u32 dle_started_flg;
static u32 mtuExchange_started_flg;
static u32 dongle_pairing_enable;
static u32 dongle_unpair_enable;
static u32 final_MTU_size = 23;
static u32 cur_conn_device_hdl; //conn_handle




#if (1) //button
	#define MAX_BTN_SIZE			2
	#define BTN_VALID_LEVEL			0
	#define BTN_PAIR				0x01
	#define BTN_UNPAIR				0x02

	u32 ctrl_btn[] = {SW1_GPIO, SW2_GPIO};
	u8 btn_map[MAX_BTN_SIZE] = {BTN_PAIR, BTN_UNPAIR};

	typedef	struct{
		u8 	cnt;				//count button num
		u8 	btn_press;
		u8 	keycode[MAX_BTN_SIZE];			//6 btn
	}vc_data_t;
	vc_data_t vc_event;

	typedef struct{
		u8  btn_history[4];		//vc history btn save
		u8  btn_filter_last;
		u8	btn_not_release;
		u8 	btn_new;					//new btn  flag
	}btn_status_t;
	btn_status_t 	btn_status;

	u8 btn_debounce_filter(u8 *btn_v)
	{
		u8 change = 0;

		for(int i=3; i>0; i--){
			btn_status.btn_history[i] = btn_status.btn_history[i-1];
		}
		btn_status.btn_history[0] = *btn_v;

		if(  btn_status.btn_history[0] == btn_status.btn_history[1] && btn_status.btn_history[1] == btn_status.btn_history[2] && \
			btn_status.btn_history[0] != btn_status.btn_filter_last ){
			change = 1;

			btn_status.btn_filter_last = btn_status.btn_history[0];
		}
		return change;
	}

	u8 vc_detect_button(int read_key)
	{
		u8 btn_changed, i;
		memset(&vc_event,0,sizeof(vc_data_t));			//clear vc_event
		//vc_event.btn_press = 0;

		for(i=0; i<MAX_BTN_SIZE; i++){
			if(BTN_VALID_LEVEL != !gpio_read(ctrl_btn[i])){
				vc_event.btn_press |= BIT(i);
			}
		}

		btn_changed = btn_debounce_filter(&vc_event.btn_press);

		if(btn_changed && read_key){
			for(i=0; i<MAX_BTN_SIZE; i++){
				if(vc_event.btn_press & BIT(i)){
					vc_event.keycode[vc_event.cnt++] = btn_map[i];
				}
			}
			return 1;
		}
		return 0;
	}

	void proc_button (void)
	{
		int det_key = vc_detect_button (1);

		if (det_key)  //key change: press or release
		{
			u8 key0 = vc_event.keycode[0];

			if(vc_event.cnt == 1) //one key press
			{
				if(key0 == BTN_PAIR)
				{
					dongle_pairing_enable = 1;

					if(master_connected_led_on) //test DLE, write cmd data
					{
						u8 write_pkt_buf[L2CAP_RX_BUFF_LEN_MAX];//buffer
						generateRandomNum(sizeof(write_pkt_buf), write_pkt_buf);
						u8 len = final_MTU_size-3;

						blc_gatt_pushWriteComand(cur_conn_device_hdl, SPP_HANDLE_DATA_C2S, write_pkt_buf, len);

						printf("c2s:write data: %d\n", len);
						array_printf(write_pkt_buf, len);
					}
				}
				else if(key0 == BTN_UNPAIR)
				{
					dongle_unpair_enable = 1;
				}
			}
			else
			{  //release
				if(dongle_pairing_enable)
				{
					dongle_pairing_enable = 0;
				}

				if(dongle_unpair_enable)
				{
					dongle_unpair_enable = 0;
				}
			}
		}
	}
#endif

int app_l2cap_handler (u16 conn_handle, u8 *raw_pkt)
{
	//l2cap data packeted, make sure that user see complete l2cap data
	rf_packet_l2cap_t *ptrL2cap = blm_l2cap_packet_pack (conn_handle, raw_pkt);
	if (!ptrL2cap)
		return 0;

	//l2cap data channel id, 4 for att, 5 for signal, 6 for smp
	if(ptrL2cap->chanId == L2CAP_CID_ATTR_PROTOCOL)  //att data
	{
		rf_packet_att_t *pAtt = (rf_packet_att_t*)ptrL2cap;
		u16 attHandle = pAtt->handle0 | pAtt->handle1<<8;

		if(pAtt->opcode == ATT_OP_EXCHANGE_MTU_REQ || pAtt->opcode == ATT_OP_EXCHANGE_MTU_RSP)
		{
			rf_packet_att_mtu_exchange_t *pMtu = (rf_packet_att_mtu_exchange_t*)ptrL2cap;

			if(pAtt->opcode ==  ATT_OP_EXCHANGE_MTU_REQ){
				blc_att_responseMtuSizeExchange(conn_handle, MTU_SIZE_SETTING);
			}

			u16 peer_mtu_size = (pMtu->mtu[0] | pMtu->mtu[1]<<8);
			final_MTU_size = min(MTU_SIZE_SETTING, peer_mtu_size);

			blt_att_setEffectiveMtuSize(cur_conn_device_hdl , final_MTU_size); //stack API, user can not change


			mtuExchange_started_flg = 1;   //set MTU size exchange flag here

			printf("Final MTU size:%d\n", final_MTU_size);
		}
		else if(pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI)  //slave handle notify
		{
			if(attHandle == SPP_HANDLE_DATA_S2C)
			{
				u8 len = pAtt->l2capLen - 3;
				if(len > 0)
				{
					printf("RF_RX len: %d\ns2c:notify data: %d\n", pAtt->rf_len, len);
					array_printf(pAtt->dat, len);
				}
			}
		}
	}
	else if(ptrL2cap->chanId == L2CAP_CID_SIG_CHANNEL)  //signal
	{
		if(ptrL2cap->opcode == L2CAP_CMD_CONN_UPD_PARA_REQ)  //slave send conn param update req on l2cap
		{
			rf_packet_l2cap_connParaUpReq_t  * req = (rf_packet_l2cap_connParaUpReq_t *)ptrL2cap;

			u32 interval_us = req->min_interval*1250;  //1.25ms unit
			u32 timeout_us = req->timeout*10000; //10ms unit
			u32 long_suspend_us = interval_us * (req->latency+1);

			//interval < 200ms, long suspend < 11S, interval * (latency +1)*2 <= timeout
			if( interval_us < 200000 && long_suspend_us < 20000000 && (long_suspend_us*2<=timeout_us) )
			{
				//when master host accept slave's conn param update req, should send a conn param update response on l2cap
				//with CONN_PARAM_UPDATE_ACCEPT; if not accpet,should send  CONN_PARAM_UPDATE_REJECT
				blc_l2cap_SendConnParamUpdateResponse(conn_handle, req->id, CONN_PARAM_UPDATE_ACCEPT);  //send SIG Connection Param Update Response

				printf("send SIG Connection Param Update accept\n");

				//if accept, master host should mark this, add will send  update conn param req on link layer later set a flag here, then send update conn param req in mainloop
				host_update_conn_param_req = clock_time() | 1 ; //in case zero value
				host_update_conn_min = req->min_interval;  //backup update param
				host_update_conn_latency = req->latency;
				host_update_conn_timeout = req->timeout;
			}
			else
			{
				blc_l2cap_SendConnParamUpdateResponse(conn_handle, req->id, CONN_PARAM_UPDATE_REJECT);  //send SIG Connection Param Update Response
				printf("send SIG Connection Param Update reject\n");
			}
		}
	}
	else if(ptrL2cap->chanId == L2CAP_CID_SMP) //smp
	{

	}
	return 0;
}

//////////////////////////////////////////////////////////
// event call back
//////////////////////////////////////////////////////////
int controller_event_callback (u32 h, u8 *p, int n)
{
	if (h &HCI_FLAG_EVENT_BT_STD)		//ble controller hci event
	{
		u8 evtCode = h & 0xff;

		//------------ disconnect -------------------------------------
		if(evtCode == HCI_EVT_DISCONNECTION_COMPLETE)  //connection terminate
		{
			event_disconnection_t	*pd = (event_disconnection_t *)p;

			//terminate reason//connection timeout
			if(pd->reason == HCI_ERR_CONN_TIMEOUT){
			}
			//peer device(slave) send terminate cmd on link layer
			else if(pd->reason == HCI_ERR_REMOTE_USER_TERM_CONN){
			}
			//master host disconnect( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) )
			else if(pd->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST){
			}
			 //master create connection, send conn_req, but did not received acked packet in 6 connection event
			else if(pd->reason == HCI_ERR_CONN_FAILED_TO_ESTABLISH){ //send connection establish event to host(telink defined event)
			}
			else{
			}

			printf("----- terminate rsn: 0x%x -----\n", pd->reason);

			//led show none connection state
			if(master_connected_led_on){
				master_connected_led_on = 0;
				gpio_write(GPIO_LED_WHITE, LED_ON_LEVAL);   //white on
				gpio_write(GPIO_LED_RED, !LED_ON_LEVAL);    //red off
			}

			connect_event_occurTick = 0;
			host_update_conn_param_req = 0; //when disconnect, clear update conn flag
			cur_conn_device_hdl = 0;  //when disconnect, clear conn handle

			//MTU size exchange and data length exchange procedure must be executed on every new connection,
			//so when connection terminate, relative flags must be cleared
			dle_started_flg = 0;
			mtuExchange_started_flg = 0;

			//MTU size reset to default 23 bytes when connection terminated
			blt_att_resetEffectiveMtuSize(pd->handle | (pd->hh<<8));  //stack API, user can not change

			//should set scan mode again to scan slave adv packet
			blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS, OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
			blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);

		}
		else if(evtCode == HCI_EVT_LE_META)
		{
			u8 subEvt_code = p[0];

			//------hci le event: le connection establish event---------------------------------
			if(subEvt_code == HCI_SUB_EVT_LE_CONNECTION_ESTABLISH)  //connection establish(telink private event)
			{
				event_connection_complete_t *pCon = (event_connection_complete_t *)p;

				if (pCon->status == BLE_SUCCESS)	// status OK
				{
					printf("----- connected -----\n");

					//led show connection state
					master_connected_led_on = 1;
					gpio_write(GPIO_LED_RED, LED_ON_LEVAL);     //red on
					gpio_write(GPIO_LED_WHITE, !LED_ON_LEVAL);  //white off

					cur_conn_device_hdl = pCon->handle;   //mark conn handle, in fact this equals to BLM_CONN_HANDLE
					connect_event_occurTick = clock_time()|1;
				}
			}
			//--------hci le event: le adv report event ----------------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)	// ADV packet
			{
				//after controller is set to scan state, it will report all the adv packet it received by this event
				event_adv_report_t *pa = (event_adv_report_t *)p;
				s8 rssi = pa->data[pa->len];

				int user_manual_paring = dongle_pairing_enable && (rssi > -56);  //button trigger pairing(rssi threshold, short distance)
				if(user_manual_paring)
				{
					//send create connection cmd to controller, trigger it switch to initiating state, after this cmd,
					//controller will scan all the adv packets it received but not report to host, to find the specified
					//device(adr_type & mac), then send a connection request packet after 150us, enter to connection state
					// and send a connection complete event(HCI_SUB_EVT_LE_CONNECTION_COMPLETE)
					blc_ll_createConnection( SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS, INITIATE_FP_ADV_SPECIFY,  \
											 pa->adr_type, pa->mac, BLE_ADDR_PUBLIC, \
											 CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 0, CONN_TIMEOUT_4S, \
											 0, 0xFFFF);
				}
			}
			//--------hci le event: le data length change event ----------------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_DATA_LENGTH_CHANGE)
			{
				hci_le_dataLengthChangeEvt_t* dle_param = (hci_le_dataLengthChangeEvt_t*)p;
				printf("----- DLE exchange: -----\n");
				printf("Effective Max Rx Octets: %d\n", dle_param->maxRxOct);
				printf("Effective Max Tx Octets: %d\n", dle_param->maxTxOct);

				dle_started_flg = 1;
			}
		}
	}
	return 0;
}

void feature_mdle_test_init_normal(void)
{
	//random number generator must be initiated here( in the beginning of user_init_nromal)
	//when deepSleep retention wakeUp, no need initialize again
	random_generator_init();  //this is must


	u8  mac_public[6];
	u8  mac_random_static[6];
	blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);

	////// Controller Initialization  //////////
	blc_ll_initBasicMCU();
	blc_ll_initStandby_module(mac_public);				//mandatory
	blc_ll_initScanning_module(mac_public); 	//scan module: 		 mandatory for BLE master,
	blc_ll_initInitiating_module();			//initiate module: 	 mandatory for BLE master,
	blc_ll_initConnection_module();						//connection module  mandatory for BLE slave/master
	blc_ll_initMasterRoleSingleConn_module();			//master module: 	 mandatory for BLE master,

	rf_set_power_level_index (RF_POWER_P3p01dBm);

	////// Host Initialization  //////////
	blc_gap_central_init();										//gap initialization
	blc_l2cap_register_handler (app_l2cap_handler);    			//l2cap initialization
	blc_hci_registerControllerEventHandler(controller_event_callback); //controller hci event to host all processed in this func

	//bluetooth event
	blc_hci_setEventMask_cmd (HCI_EVT_MASK_DISCONNECTION_COMPLETE | HCI_EVT_MASK_ENCRYPTION_CHANGE);

	//bluetooth low energy(LE) event
	blc_hci_le_setEventMask_cmd(  HCI_LE_EVT_MASK_CONNECTION_COMPLETE  \
							    | HCI_LE_EVT_MASK_ADVERTISING_REPORT \
							    | HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE \
							    | HCI_LE_EVT_MASK_DATA_LENGTH_CHANGE \
							    | HCI_LE_EVT_MASK_CONNECTION_ESTABLISH ); //connection establish: telink private event

	//ATT initialization
	blc_att_setRxMtuSize(MTU_SIZE_SETTING); //If not set RX MTU size, default is: 23 bytes.

	//NO SMP process
	blc_smp_setSecurityLevel(No_Security);

	//set scan parameter and scan enable
	blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS, OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);

	#if(FEATURE_PM_ENABLE)
		//
	#else
		bls_pm_setSuspendMask (SUSPEND_DISABLE);
	#endif
}



_attribute_ram_code_ void feature_mdle_test_init_deepRetn(void)
{
#if (FEATURE_DEEPSLEEP_RETENTION_ENABLE)
	//
#endif
}



void feature_mdle_test_mainloop(void)
{
	/////////////////////////////////////// HCI ///////////////////////////////////////
	blc_hci_proc ();

	////////////////////////////////////// UI entry /////////////////////////////////
	static u8 button_detect_en = 0;
	if(!button_detect_en && clock_time_exceed(0, 1000000)){// proc button 1 second later after power on
		button_detect_en = 1;
	}
	static u32 button_detect_tick = 0;
	if(button_detect_en && clock_time_exceed(button_detect_tick, 5000))
	{
		button_detect_tick = clock_time();
		proc_button();
	}

	if( host_update_conn_param_req && clock_time_exceed(host_update_conn_param_req, 50000))
	{
		host_update_conn_param_req = 0;
		if(blc_ll_getCurrentState() == BLS_LINK_STATE_CONN){  //still in connection state
			blm_ll_updateConnection (cur_conn_device_hdl, host_update_conn_min, host_update_conn_min, host_update_conn_latency,  host_update_conn_timeout, 0, 0 );
		}
	}

	if(connect_event_occurTick && clock_time_exceed(connect_event_occurTick, 500000)){  //500ms after connection established
		connect_event_occurTick = 0;
		mtuExchange_check_tick = clock_time() | 1;

		if(!mtuExchange_started_flg){  //master do not send MTU exchange request in time
			printf("After conn 200ms, if not receive S's MTU exchange pkt, M send MTU exchange req to S.\n");
			blc_att_requestMtuSizeExchange(cur_conn_device_hdl, MTU_SIZE_SETTING);
		}
	}

	if(mtuExchange_check_tick && clock_time_exceed(mtuExchange_check_tick, 500000 )){  //1S after connection established
		mtuExchange_check_tick = 0;

		if(!dle_started_flg){ //master do not send data length request in time
			printf("Master initiated the DLE.\n");
			blc_ll_exchangeDataLength(LL_LENGTH_REQ , DLE_TX_SUPPORTED_DATA_LEN);
		}
	}


	//terminate and unpair proc
	static int master_disconnect_flag;
	if(dongle_unpair_enable){
		if(!master_disconnect_flag && blc_ll_getCurrentState() == BLS_LINK_STATE_CONN){
			if( blm_ll_disconnect(cur_conn_device_hdl, HCI_ERR_REMOTE_USER_TERM_CONN) == BLE_SUCCESS){
				master_disconnect_flag = 1;
				dongle_unpair_enable = 0;
			}
		}
	}
	if(master_disconnect_flag && blc_ll_getCurrentState() != BLS_LINK_STATE_CONN){
		master_disconnect_flag = 0;
	}
}


#endif
