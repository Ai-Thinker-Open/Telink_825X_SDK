/********************************************************************************************************
 * @file     blm_host.c
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


#include "blm_att.h"
#include "blm_pair.h"
#include "blm_host.h"
#include "blm_ota.h"



main_service_t		main_service = 0;

#define SMP_PENDING					1   //security management
#define SDP_PENDING					2   //service discovery

int	app_host_smp_sdp_pending = 0; 		//security & service discovery


extern u8 read_by_type_req_uuidLen;
extern u8 read_by_type_req_uuid[16];



dev_char_info_t cur_conn_device;

////////////////////////////////////////////////////////////////////
u32 host_update_conn_param_req = 0;
u16 host_update_conn_min;
u16 host_update_conn_latency;
u16 host_update_conn_timeout;

u16 final_MTU_size = 23;

int master_connected_led_on = 0;




int master_auto_connect = 0;
int user_manual_paring;



const u8 	telink_adv_trigger_paring[] = {5, 0xFF, 0x11, 0x02, 0x01, 0x00};
const u8 	telink_adv_trigger_unpair[] = {5, 0xFF, 0x11, 0x02, 0x01, 0x01};

const u8 	telink_adv_trigger_paring_8258[] = {7, 0xFF, 0x11, 0x02, 0x01, 0x00, 0x58, 0x82};
const u8 	telink_adv_trigger_unpair_8258[] = {7, 0xFF, 0x11, 0x02, 0x01, 0x01, 0x58, 0x82};


#if (BLE_HOST_SIMPLE_SDP_ENABLE)
	extern void host_att_service_disccovery_clear(void);
	int host_att_client_handler (u16 connHandle, u8 *p);
	ble_sts_t  host_att_discoveryService (u16 handle, att_db_uuid16_t *p16, int n16, att_db_uuid128_t *p128, int n128);


	#define				ATT_DB_UUID16_NUM		20
	#define				ATT_DB_UUID128_NUM		8

	u8 	conn_char_handler[8] = {0};


	u8	serviceDiscovery_adr_type;
	u8	serviceDiscovery_address[6];


	extern const u8 my_MicUUID[16];
	extern const u8 my_SpeakerUUID[16];
	extern const u8 my_OtaUUID[16];


	void app_service_discovery ()
	{

		att_db_uuid16_t 	db16[ATT_DB_UUID16_NUM];
		att_db_uuid128_t 	db128[ATT_DB_UUID128_NUM];
		memset (db16, 0, ATT_DB_UUID16_NUM * sizeof (att_db_uuid16_t));
		memset (db128, 0, ATT_DB_UUID128_NUM * sizeof (att_db_uuid128_t));


		if ( IS_CONNECTION_HANDLE_VALID(cur_conn_device.conn_handle) && \
			 host_att_discoveryService (cur_conn_device.conn_handle, db16, ATT_DB_UUID16_NUM, db128, ATT_DB_UUID128_NUM) == BLE_SUCCESS)	// service discovery OK
		{
			//int h = current_connHandle & 7;
			conn_char_handler[0] = blm_att_findHandleOfUuid128 (db128, my_MicUUID);			//MIC
			conn_char_handler[1] = blm_att_findHandleOfUuid128 (db128, my_SpeakerUUID);		//Speaker
			conn_char_handler[2] = blm_att_findHandleOfUuid128 (db128, my_OtaUUID);			//OTA


			conn_char_handler[3] = blm_att_findHandleOfUuid16 (db16, CHARACTERISTIC_UUID_HID_REPORT,
						HID_REPORT_ID_CONSUME_CONTROL_INPUT | (HID_REPORT_TYPE_INPUT<<8));		//consume report

			conn_char_handler[4] = blm_att_findHandleOfUuid16 (db16, CHARACTERISTIC_UUID_HID_REPORT,
						HID_REPORT_ID_KEYBOARD_INPUT | (HID_REPORT_TYPE_INPUT<<8));				//normal key report

			conn_char_handler[5] = blm_att_findHandleOfUuid16 (db16, CHARACTERISTIC_UUID_HID_REPORT,
						HID_REPORT_ID_MOUSE_INPUT | (HID_REPORT_TYPE_INPUT<<8));				//mouse report

			//module
			//conn_char_handler[6] = blm_att_findHandleOfUuid128 (db128, my_SppS2CUUID);			//notify
			//conn_char_handler[7] = blm_att_findHandleOfUuid128 (db128, my_SppC2SUUID);			//write_cmd



			//save current service discovery conn address
			serviceDiscovery_adr_type = cur_conn_device.mac_addr;
			memcpy(serviceDiscovery_address, cur_conn_device.mac_addr, 6);

		}

		app_host_smp_sdp_pending = 0;  //service discovery finish

	}

	void app_register_service (void *p)
	{
		main_service = p;
	}




	#define			HID_HANDLE_CONSUME_REPORT			conn_char_handler[3]
	#define			HID_HANDLE_KEYBOARD_REPORT			conn_char_handler[4]
	#define			HID_HANDLE_MOUSE_REPORT				conn_char_handler[5]
	#define			AUDIO_HANDLE_MIC					conn_char_handler[0]
#else  //no service discovery

	//need define att handle same with slave
	#define 		HID_HANDLE_MOUSE_REPORT
	#define			HID_HANDLE_CONSUME_REPORT			25
	#define			HID_HANDLE_KEYBOARD_REPORT			29
	#define			AUDIO_HANDLE_MIC					52


#endif







#if (BLE_HOST_SMP_ENABLE)
int app_host_smp_finish (void)  //smp finish callback
{
	#if (BLE_HOST_SIMPLE_SDP_ENABLE)  //smp finish, start sdp
		if(app_host_smp_sdp_pending == SMP_PENDING)
		{
			//new slave device, should do service discovery again
			if (cur_conn_device.mac_adrType != serviceDiscovery_adr_type || \
				memcmp(cur_conn_device.mac_addr, serviceDiscovery_address, 6))
			{
				app_register_service(&app_service_discovery);
				app_host_smp_sdp_pending = SDP_PENDING; //service discovery busy
			}
			else
			{
				app_host_smp_sdp_pending = 0;  //no need sdp
			}
		}
	#else
		app_host_smp_sdp_pending = 0;  //no sdp
	#endif

	return 0;
}
#endif







int blm_le_adv_report_event_handle(u8 *p)
{
	event_adv_report_t *pa = (event_adv_report_t *)p;
	s8 rssi = pa->data[pa->len];

	 //if previous connection smp&sdp not finish, can not create a new connection
	if(app_host_smp_sdp_pending){
		return 1;
	}



	/****************** Button press or Adv pair packet triggers pair ***********************/
	int master_auto_connect = 0;
	int user_manual_paring = 0;

	//manual paring methods 1: button triggers
	user_manual_paring = dongle_pairing_enable && (rssi > -56);  //button trigger pairing(rssi threshold, short distance)

	//manual paring methods 2: special paring adv data
	if(!user_manual_paring){  //special adv pair data can also trigger pairing
		user_manual_paring = (memcmp(pa->data, telink_adv_trigger_paring_8258, sizeof(telink_adv_trigger_paring_8258)) == 0) && (rssi > -56);
	}


	#if (BLE_HOST_SMP_ENABLE)
		master_auto_connect = tbl_bond_slave_search(pa->adr_type, pa->mac);
	#else
		//search in slave mac table to find whether this device is an old device which has already paired with master
		master_auto_connect = user_tbl_slave_mac_search(pa->adr_type, pa->mac);
	#endif

	if(master_auto_connect || user_manual_paring)
	{


		//send create connection cmd to controller, trigger it switch to initiating state, after this cmd,
		//controller will scan all the adv packets it received but not report to host, to find the specified
		//device(adr_type & mac), then send a connection request packet after 150us, enter to connection state
		// and send a connection complete event(HCI_SUB_EVT_LE_CONNECTION_COMPLETE)
		u8 status = blc_ll_createConnection( SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS, INITIATE_FP_ADV_SPECIFY,  \
								 pa->adr_type, pa->mac, BLE_ADDR_PUBLIC, \
								 CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 0, CONN_TIMEOUT_4S, \
								 0, 0xFFFF);

		if(status == BLE_SUCCESS)   //create connection success
		{
			#if (!BLE_HOST_SMP_ENABLE)
				if(user_manual_paring && !master_auto_connect){  //manual pair
					blm_manPair.manual_pair = 1;
					blm_manPair.mac_type = pa->adr_type;
					memcpy(blm_manPair.mac, pa->mac, 6);
					blm_manPair.pair_tick = clock_time();
				}
			#endif
		}

	}


	/****************** Adv unpair packet triggers unpair ***********************/
	int adv_unpair_en = !memcmp(pa->data, telink_adv_trigger_unpair_8258, sizeof(telink_adv_trigger_unpair_8258));
	if(adv_unpair_en)
	{
		int device_is_bond;

		#if (BLE_HOST_SMP_ENABLE)
			device_is_bond = tbl_bond_slave_search(pa->adr_type, pa->mac);
			if(device_is_bond){ //this adv mac is bonded in master
				tbl_bond_slave_delete_by_adr(pa->adr_type, pa->mac);  //by telink stack host smp
			}
		#else
			device_is_bond = user_tbl_slave_mac_search(pa->adr_type, pa->mac);
			if(device_is_bond){ //this adv mac is bonded in master
				user_tbl_slave_mac_delete_by_adr(pa->adr_type, pa->mac);  //by user application code
			}
		#endif
	}


	return 0;
}





int blm_le_connection_establish_event_handle(u8 *p)
{

	event_connection_complete_t *pCon = (event_connection_complete_t *)p;
	if (pCon->status == BLE_SUCCESS)	// status OK
	{
		#if (UI_LED_ENABLE)
			//led show connection state
			master_connected_led_on = 1;
			gpio_write(GPIO_LED_RED, LED_ON_LEVAL);     //red on
			gpio_write(GPIO_LED_WHITE, !LED_ON_LEVAL);  //white off
		#endif


		cur_conn_device.conn_handle = pCon->handle;   //mark conn handle, in fact this equals to BLM_CONN_HANDLE

		//save current connect address type and address
		cur_conn_device.mac_adrType = pCon->peer_adr_type;
		memcpy(cur_conn_device.mac_addr, pCon->mac, 6);


		#if (BLE_HOST_SMP_ENABLE)
			app_host_smp_sdp_pending = SMP_PENDING; //pair & security first
		#else


			//manual paring, device match, add this device to slave mac table
			if(blm_manPair.manual_pair && blm_manPair.mac_type == pCon->peer_adr_type && !memcmp(blm_manPair.mac, pCon->mac, 6)){
				blm_manPair.manual_pair = 0;

				user_tbl_slave_mac_add(pCon->peer_adr_type, pCon->mac);
			}


				#if (BLE_HOST_SIMPLE_SDP_ENABLE)
						//new slave device, should do service discovery again
						if (pCon->peer_adr_type != serviceDiscovery_adr_type || memcmp(pCon->mac, serviceDiscovery_address, 6)){
							app_register_service(&app_service_discovery);
							app_host_smp_sdp_pending = SDP_PENDING;  //service discovery busy
						}
						else{
							app_host_smp_sdp_pending = 0;  //no need sdp
						}
				#endif
		#endif
	}



	return 0;
}





int 	blm_disconnect_event_handle(u8 *p)
{
	event_disconnection_t	*pd = (event_disconnection_t *)p;

	//terminate reason
	//connection timeout
	if(pd->reason == HCI_ERR_CONN_TIMEOUT){

	}
	//peer device(slave) send terminate cmd on link layer
	else if(pd->reason == HCI_ERR_REMOTE_USER_TERM_CONN){

	}
	//master host disconnect( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) )
	else if(pd->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST){

	}
	 //master create connection, send conn_req, but did not received acked packet in 6 connection event
	else if(pd->reason == HCI_ERR_CONN_FAILED_TO_ESTABLISH){
		//when controller is in initiating state, find the specified device, send connection request to slave,
		//but slave lost this rf packet, there will no ack packet from slave, after 6 connection events, master
		//controller send a disconnect event with reason HCI_ERR_CONN_FAILED_TO_ESTABLISH
		//if slave got the connection request packet and send ack within 6 connection events, controller
		//send connection establish event to host(telink defined event)


	}
	else{

	}

	#if (UI_LED_ENABLE)
		//led show none connection state
		if(master_connected_led_on){
			master_connected_led_on = 0;
			gpio_write(GPIO_LED_WHITE, LED_ON_LEVAL);   //white on
			gpio_write(GPIO_LED_RED, !LED_ON_LEVAL);    //red off
		}
	#endif


	cur_conn_device.conn_handle = 0;  //when disconnect, clear conn handle


	//if previous connection smp&sdp not finished, clear this flag
	if(app_host_smp_sdp_pending){
		app_host_smp_sdp_pending = 0;
	}

	host_update_conn_param_req = 0; //when disconnect, clear update conn flag

	host_att_data_clear();


	//MTU size reset to default 23 bytes when connection terminated
	blt_att_resetEffectiveMtuSize(BLM_CONN_HANDLE);  //stack API, user can not change

	final_MTU_size = 23;

	//should set scan mode again to scan slave adv packet
	blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,
							OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);


	return 0;
}



int blm_le_conn_update_event_proc(u8 *p)
{
//	event_connection_update_t *pCon = (event_connection_update_t *)p;


	#if (BLE_MASTER_OTA_ENABLE)
		event_connection_update_t *pCon = (event_connection_update_t *)p;

		extern void host_ota_update_conn_complete(u16, u16, u16);
		host_ota_update_conn_complete( pCon->interval, pCon->latency, pCon->timeout );
	#endif


	return 0;
}

int blm_le_phy_update_complete_event_proc(u8 *p)
{
	hci_le_phyUpdateCompleteEvt_t *pPhyUpt = (hci_le_phyUpdateCompleteEvt_t *)p;



	return 0;
}



//////////////////////////////////////////////////////////
// event call back
//////////////////////////////////////////////////////////
int controller_event_callback (u32 h, u8 *p, int n)
{


	static u32 event_cb_num;
	event_cb_num++;

	if (h &HCI_FLAG_EVENT_BT_STD)		//ble controller hci event
	{
		u8 evtCode = h & 0xff;

		//------------ disconnect -------------------------------------
		if(evtCode == HCI_EVT_DISCONNECTION_COMPLETE)  //connection terminate
		{
			blm_disconnect_event_handle(p);
		}
#if (BLE_HOST_SMP_ENABLE)
		else if(evtCode == HCI_EVT_ENCRYPTION_CHANGE)
		{
			event_enc_change_t *pe = (event_enc_change_t *)p;
			blm_smp_encChangeEvt(pe->status, pe->handle, pe->enc_enable);
		}
		else if(evtCode == BLM_CONN_ENC_REFRESH)
		{
			event_enc_refresh_t *pe = (event_enc_refresh_t *)p;
			blm_smp_encChangeEvt(pe->status, pe->handle, 1);
		}
#endif
		else if(evtCode == HCI_EVT_LE_META)
		{
			u8 subEvt_code = p[0];


			//------hci le event: le connection complete event---------------------------------
			if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE)	// connection complete
			{
				//after controller is set to initiating state by host (blc_ll_createConnection(...) )
				//it will scan the specified device(adr_type & mac), when find this adv packet, send a connection request packet to slave
				//and enter to connection state, send connection complete event. but notice that connection complete not
				//equals to connection establish. connection complete means that master controller set all the ble timing
				//get ready, but has not received any slave packet, if slave rf lost the connection request packet, it will
				//not send any packet to master controller
			}
			//------hci le event: le connection establish event---------------------------------
			else if(subEvt_code == HCI_SUB_EVT_LE_CONNECTION_ESTABLISH)  //connection establish(telink private event)
			{
				//notice that: this connection event is defined by telink, not a standard ble controller event
				//after master controller send connection request packet to slave, when slave received this packet
				//and enter to connection state, send a ack packet within 6 connection event, master will send
				//connection establish event to host(HCI_SUB_EVT_LE_CONNECTION_ESTABLISH)

				blm_le_connection_establish_event_handle(p);
			}
			//--------hci le event: le adv report event ----------------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)	// ADV packet
			{
				//after controller is set to scan state, it will report all the adv packet it received by this event

				blm_le_adv_report_event_handle(p);
			}
			//------hci le event: le connection update complete event-------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE)	// connection update
			{
				//after master host send update conn param req cmd to controller( blm_ll_updateConnection(...) ),
				//when update take effect, controller send update complete event to host
				blm_le_conn_update_event_proc(p);
			}
			//------hci le event: le phy update complete event-------------------------------
			else if (subEvt_code == HCI_SUB_EVT_LE_PHY_UPDATE_COMPLETE)	// connection update
			{
				//2 situation can trigger this event:
				//   1) master host trigger: by calling API  blc_ll_setPhy(...)
				//   2) peer slave device trigger: send "LL_PHY_REQ" on linklayer
				//when update take effect, controller send update complete event to host
				blm_le_phy_update_complete_event_proc(p);
			}

		}
	}


	return 0;

}




_attribute_ram_code_
void host_update_conn_proc(void)
{
	//at least 50ms later and make sure smp/sdp is finished
	if( host_update_conn_param_req && clock_time_exceed(host_update_conn_param_req, 50000) && !app_host_smp_sdp_pending)
	{
		host_update_conn_param_req = 0;

		if(blc_ll_getCurrentState() == BLS_LINK_STATE_CONN){  //still in connection state
			blm_ll_updateConnection (cur_conn_device.conn_handle,
					host_update_conn_min, host_update_conn_min, host_update_conn_latency,  host_update_conn_timeout,
											  0, 0 );
		}
	}
}







volatile int app_l2cap_handle_cnt = 0;
int app_l2cap_handler (u16 conn_handle, u8 *raw_pkt)
{
	app_l2cap_handle_cnt ++;  //debug




	//l2cap data packeted, make sure that user see complete l2cap data
	rf_packet_l2cap_t *ptrL2cap = blm_l2cap_packet_pack (conn_handle, raw_pkt);
	if (!ptrL2cap)
	{
		return 0;
	}



	//l2cap data channel id, 4 for ATT, 5 for Signal, 6 for SMP
	if(ptrL2cap->chanId == L2CAP_CID_ATTR_PROTOCOL)  //att data
	{

		#if (BLE_HOST_SIMPLE_SDP_ENABLE)
			if(app_host_smp_sdp_pending == SDP_PENDING)  //ATT service discovery is ongoing
			{
				//when service discovery function is running, all the ATT data from slave
				//will be processed by it,  user can only send your own att cmd after  service discovery is over
				host_att_client_handler (conn_handle, (u8 *)ptrL2cap); //handle this ATT data by service discovery process
			}
		#endif


		rf_packet_att_t *pAtt = (rf_packet_att_t*)ptrL2cap;
		u16 attHandle = pAtt->handle0 | pAtt->handle1<<8;


		if(pAtt->opcode == ATT_OP_EXCHANGE_MTU_REQ || pAtt->opcode == ATT_OP_EXCHANGE_MTU_RSP)
		{
			rf_packet_att_mtu_exchange_t *pMtu = (rf_packet_att_mtu_exchange_t*)ptrL2cap;

			if(pAtt->opcode ==  ATT_OP_EXCHANGE_MTU_REQ){
				blc_att_responseMtuSizeExchange(conn_handle, ATT_RX_MTU_SIZE_MAX);
			}

			u16 peer_mtu_size = (pMtu->mtu[0] | pMtu->mtu[1]<<8);
			final_MTU_size = min(ATT_RX_MTU_SIZE_MAX, peer_mtu_size);

			blt_att_setEffectiveMtuSize(conn_handle , final_MTU_size); //stack API, user can not change
		}
		else if(pAtt->opcode == ATT_OP_READ_BY_TYPE_RSP)  //slave ack ATT_OP_READ_BY_TYPE_REQ data
		{
			#if (BLE_MASTER_OTA_ENABLE)
				//when use ATT_OP_READ_BY_TYPE_REQ to find ota atthandle, should get the result
				extern void host_find_slave_ota_attHandle(u8 *p);
				host_find_slave_ota_attHandle( (u8 *)pAtt );
			#endif
			//u16 slave_ota_handle;
		}
		else if(pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI)  //slave handle notify
		{

			if(attHandle == HID_HANDLE_CONSUME_REPORT)
			{
				static u32 app_key;
				app_key++;
				att_keyboard_media (conn_handle, pAtt->dat);
			}
			else if(attHandle == HID_HANDLE_KEYBOARD_REPORT)
			{
				static u32 app_key;
				app_key++;
				att_keyboard (conn_handle, pAtt->dat);

			}
//			else if(HID_HANDLE_MOUSE_REPORT){
//				static u32 app_mouse_dat;
//				att_mouse(conn_handle,pAtt->dat);
//			}
			else if(attHandle == AUDIO_HANDLE_MIC)
			{
				static u32 app_mic;
				app_mic	++;
				att_mic (conn_handle, pAtt->dat);
			}
			else
			{

			}
		}
		else if (pAtt->opcode == ATT_OP_HANDLE_VALUE_IND)
		{

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

			//interval < 200ms
			//long suspend < 11S
			// interval * (latency +1)*2 <= timeout
			if( interval_us < 200000 && long_suspend_us < 20000000 && (long_suspend_us*2<=timeout_us) )
			{
				//when master host accept slave's conn param update req, should send a conn param update response on l2cap
				//with CONN_PARAM_UPDATE_ACCEPT; if not accpet,should send  CONN_PARAM_UPDATE_REJECT
				blc_l2cap_SendConnParamUpdateResponse(conn_handle, req->id, CONN_PARAM_UPDATE_ACCEPT);  //send SIG Connection Param Update Response


				//if accept, master host should mark this, add will send  update conn param req on link layer later
				//set a flag here, then send update conn param req in mainloop
				host_update_conn_param_req = clock_time() | 1 ; //in case zero value
				host_update_conn_min = req->min_interval;  //backup update param
				host_update_conn_latency = req->latency;
				host_update_conn_timeout = req->timeout;
			}
			else
			{
				blc_l2cap_SendConnParamUpdateResponse(conn_handle, req->id, CONN_PARAM_UPDATE_REJECT);  //send SIG Connection Param Update Response
			}
		}


	}
	else if(ptrL2cap->chanId == L2CAP_CID_SMP) //smp
	{
		#if (BLE_HOST_SMP_ENABLE)
			if(app_host_smp_sdp_pending == SMP_PENDING)
			{
				blm_host_smp_handler(conn_handle, (u8 *)ptrL2cap);
			}

		#endif
	}
	else
	{

	}


	return 0;
}
