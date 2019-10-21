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
#include "vendor/common/blt_common.h"


#include "blm_att.h"
#include "blm_pair.h"
#include "blm_host.h"
#include "blm_ota.h"



MYFIFO_INIT(blt_rxfifo, 64, 8);
MYFIFO_INIT(blt_txfifo, 40, 8);










int main_idle_loop (void);



























///////////////////////////////////////////
void user_init(void)
{
	//random number generator must be initiated here( in the beginning of user_init_nromal)
	//when deepSleep retention wakeUp, no need initialize again
	random_generator_init();  //this is must


	//set USB ID
	REG_ADDR8(0x74) = 0x62;
	REG_ADDR16(0x7e) = 0x08d0;
	REG_ADDR8(0x74) = 0x00;

	//////////////// config USB ISO IN/OUT interrupt /////////////////
	reg_usb_mask = BIT(7);			//audio in interrupt enable
	reg_irq_mask |= FLD_IRQ_IRQ4_EN;
	reg_usb_ep6_buf_addr = 0x80;
	reg_usb_ep7_buf_addr = 0x60;
	reg_usb_ep_max_size = (256 >> 3);

	usb_dp_pullup_en (1);  //open USB enum



	///////////////// SDM /////////////////////////////////
#if (AUDIO_SDM_ENBALE)
	u16 sdm_step = config_sdm  (buffer_sdm, TL_SDM_BUFFER_SIZE, 16000, 4);
#endif


///////////// BLE stack Initialization ////////////////
	u8  mac_public[6];
	u8  mac_random_static[6];
	blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);


	////// Controller Initialization  //////////
	blc_ll_initBasicMCU();
	blc_ll_initStandby_module(mac_public);				//mandatory
	blc_ll_initScanning_module(mac_public); 			//scan module: 		 mandatory for BLE master,
	blc_ll_initInitiating_module();						//initiate module: 	 mandatory for BLE master,
	blc_ll_initConnection_module();						//connection module  mandatory for BLE slave/master
	blc_ll_initMasterRoleSingleConn_module();			//master module: 	 mandatory for BLE master,


//	blc_ll_init2MPhyCodedPhy_feature();					//debug
//	blc_ll_setDefaultConnCodingIndication(CODED_PHY_PREFER_S2);

//	blc_ll_initChannelSelectionAlgorithm_2_feature();	//debug


	rf_set_power_level_index (RF_POWER_P3p01dBm);


	////// Host Initialization  //////////
	blc_gap_central_init();										//gap initialization
	blc_l2cap_register_handler (app_l2cap_handler);    			//l2cap initialization
	blc_hci_registerControllerEventHandler(controller_event_callback); //controller hci event to host all processed in this func

	//bluetooth event
	blc_hci_setEventMask_cmd (HCI_EVT_MASK_DISCONNECTION_COMPLETE | HCI_EVT_MASK_ENCRYPTION_CHANGE);

	//bluetooth low energy(LE) event
	blc_hci_le_setEventMask_cmd(		HCI_LE_EVT_MASK_CONNECTION_COMPLETE 		\
									|	HCI_LE_EVT_MASK_ADVERTISING_REPORT 			\
									|   HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE  \
									|	HCI_LE_EVT_MASK_PHY_UPDATE_COMPLETE			\
									|   HCI_LE_EVT_MASK_CONNECTION_ESTABLISH );         //connection establish: telink private event



	#if (BLE_HOST_SMP_ENABLE)
		blm_smp_configParingSecurityInfoStorageAddr(FLASH_ADR_PARING);
		blm_smp_registerSmpFinishCb(app_host_smp_finish);

		blc_smp_central_init();

		//SMP trigger by master
		blm_host_smp_setSecurityTrigger(MASTER_TRIGGER_SMP_FIRST_PAIRING | MASTER_TRIGGER_SMP_AUTO_CONNECT);
	#else  //TeLink referenced paring&bonding without standard paring in BLE spec
		blc_smp_setSecurityLevel(No_Security);

		user_master_host_pairing_management_init();
	#endif



	extern int host_att_register_idle_func (void *p);
	host_att_register_idle_func (main_idle_loop);





	//set scan parameter and scan enable

	blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS,	\
							OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);



	#if (UI_UPPER_COMPUTER_ENABLE)
		extern void app_upper_com_init(void);
		app_upper_com_init();
	#endif
}








extern void usb_handle_irq(void);
extern void proc_button (void);
extern void proc_audio (void);
/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
int main_idle_loop (void)
{


	////////////////////////////////////// BLE entry /////////////////////////////////
	blt_sdk_main_loop();


	///////////////////////////////////// proc usb cmd from host /////////////////////
	usb_handle_irq();



	/////////////////////////////////////// HCI ///////////////////////////////////////
	blc_hci_proc ();



	////////////////////////////////////// UI entry /////////////////////////////////
#if (UI_BUTTON_ENABLE)
	static u8 button_detect_en = 0;
	if(!button_detect_en && clock_time_exceed(0, 1000000)){// proc button 1 second later after power on
		button_detect_en = 1;
	}
	static u32 button_detect_tick = 0;
	if(button_detect_en && clock_time_exceed(button_detect_tick, 5000))
	{
		button_detect_tick = clock_time();
		proc_button();  //button triggers pair & unpair  and OTA
	}
#endif


#if (UI_UPPER_COMPUTER_ENABLE)
	extern void app_upper_com_proc(void);
	app_upper_com_proc();
#endif


	////////////////////////////////////// proc audio ////////////////////////////////
#if (UI_AUDIO_ENABLE)
	proc_audio();

	static u32 tick_bo;
	if (REG_ADDR8(0x125) & BIT(0))
	{
		tick_bo = clock_time ();
	}
	else if (clock_time_exceed (tick_bo, 200000))
	{
		REG_ADDR8(0x125) = BIT(0);
	}
#endif


	host_pair_unpair_proc();


#if(BLE_MASTER_OTA_ENABLE)
	proc_ota();
#endif

#if 1
	//proc master update
	if(host_update_conn_param_req){
		host_update_conn_proc();
	}
#endif




	return 0;
}




void main_loop (void)
{

	main_idle_loop ();

	if (main_service)
	{
		main_service ();
		main_service = 0;
	}
}





