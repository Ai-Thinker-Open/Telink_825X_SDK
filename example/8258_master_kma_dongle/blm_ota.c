/********************************************************************************************************
 * @file	 blm_ota.c
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

#include "blm_host.h"


extern u8 read_by_type_req_uuidLen;
extern u8 read_by_type_req_uuid[16];
extern void host_att_set_current_readByTypeReq_uuid(u8 *uuid, u8 uuid_len);

extern const u8 my_OtaUUID[16];


int master_ota_test_mode = 0;

#if(BLE_MASTER_OTA_ENABLE)



#define FW_SIZE_MAX			128  //128K
#define OTA_TIMEOUT_S		20

#define LED_INTERVAL_US  	400000
#define OTA_INTERVAL_US  	500000


#define OTA_STEP_IDLE						0
#define	OTA_STEP_UPDATE_CONN				1



#define CMD_OTA_FW_VERSION					0xff00
#define CMD_OTA_START						0xff01
#define CMD_OTA_END							0xff02
#define CMD_OTA_START_REQ                   0xff03 //Request OTA start, reserved for BLE Fullstack
#define CMD_OTA_START_RSP                   0xff04 //Slave allow OTA start, reserved for BLE Fullstack
#define CMD_OTA_TEST                        0xff05 //Master send OTA TEST to slave to check slave's state, reserved for BLE Fullstack
#define CMD_OTA_TEST_RSP                    0xff06 //Slave respond to master's CMD_OTA_TEST, reserved for BLE Fullstack
#define CMD_OTA_ERROR                       0xff07 //Slave send OTA error to master once it stop OTA process because of some error, reserved for BLE Fullstack




u16 slave_ota_handle = 0;

typedef struct{
	u16 adr_index;
	u8	data[16];
	u16 crc_16;
}rf_packet_att_ota_data_t;


u32 flash_adr_ota_master = 0x20000;
u8 *p_firmware;
u32 n_firmware = 0;
u32 ota_adr = 0;


u32 ota_mode_begin_tick;


int master_ota_cmd;
u32 led_indicate_begin_tick;
u32 led_indicate_gpio;
u32 ota_result_tick;

int host_ota_start = 0;
int host_ota_update_pending;


void ota_set_result(int success)
{
#if (UI_LED_ENABLE)
	if(master_ota_cmd == 1){
		led_indicate_gpio = GPIO_LED_BLUE;
	}
	else{
		led_indicate_gpio = GPIO_LED_GREEN;
	}
#endif

	if(success){
		led_indicate_begin_tick = clock_time() | 1;
	}
	else{
		led_indicate_begin_tick = 0;
	}

#if (UI_LED_ENABLE)
	gpio_write(led_indicate_gpio,!LED_ON_LEVAL);  //ota fail
#endif

	host_ota_update_pending = 0;

	host_ota_start = 0;
	master_ota_cmd = 0;
	ota_result_tick = clock_time() | 1;

	//write_reg8(reg_cmd_result,status);  //1 OK  0 fail
	//SET_CMD_DONE;
}

/*Set OTA process timeout*/
u8 adr_index_max_l;
u8 adr_index_max_h;


void host_button_trigger_ota_start(int button0_press_flag, int button1_press_flag)
{
	if(blc_ll_getCurrentState() == BLS_LINK_STATE_CONN){
		if(button0_press_flag){
			master_ota_cmd = 1;
		}
		else if(button1_press_flag){
			master_ota_cmd = 2;
		}
	}
}




void host_ota_update_conn_complete( u16 interval, u16 latency, u16 timeout )
{
	if(host_ota_update_pending == 2 && interval == 8 && latency == 0 && timeout == 200){
		host_ota_update_pending = 0;

		if(host_ota_start == 1){
			host_ota_start = 2;
		}
	}
}





void host_find_slave_ota_attHandle(u8 *p)
{
	if(host_ota_start == 3 && read_by_type_req_uuidLen == 16 && !memcmp(read_by_type_req_uuid, my_OtaUUID, 16) )
	{
		rf_pkt_att_readByTypeRsp_t *ptr = (rf_pkt_att_readByTypeRsp_t *)p;
		slave_ota_handle = ptr->data[0] | ptr->data[1]<<8;

		host_ota_start = 4;
	}
}



u32 otaStart_cmd_tick;
u32 ota_lastData_tick;

void proc_ota (void)
{


	if(host_ota_update_pending == 1 && !host_update_conn_param_req && !app_host_smp_sdp_pending)
	{

		if(blc_ll_getCurrentState() == BLS_LINK_STATE_CONN){  //still in connection state
			//10ms interval,  latency = 0   timeout = 2S
			blm_ll_updateConnection (cur_conn_device.conn_handle,
										8, 8, 0,  200,
											  0, 0 );

			host_ota_update_pending = 2;

		}
	}




	static u32 ota_begin_tick;
    static int led_step = 0;


	//enter OTA mode,  2 led shine for 3 times
    if(master_ota_test_mode == 1)
    {
#if (UI_LED_ENABLE)
    	if(led_step==0 && clock_time_exceed(ota_mode_begin_tick, OTA_INTERVAL_US)){
    		gpio_write(GPIO_LED_BLUE, LED_ON_LEVAL);
    		gpio_write(GPIO_LED_GREEN, LED_ON_LEVAL);
    		led_step = 1;
    	}
    	else if(led_step==1 && clock_time_exceed(ota_mode_begin_tick, OTA_INTERVAL_US*2)){
    		gpio_write(GPIO_LED_BLUE, !LED_ON_LEVAL);
    		gpio_write(GPIO_LED_GREEN, !LED_ON_LEVAL);
    		led_step = 2;
    	}
    	else if(led_step==2 && clock_time_exceed(ota_mode_begin_tick, OTA_INTERVAL_US*3)){
    		gpio_write(GPIO_LED_BLUE, LED_ON_LEVAL);
    		gpio_write(GPIO_LED_GREEN, LED_ON_LEVAL);
    		led_step = 3;
    	}
    	else if(led_step==3 && clock_time_exceed(ota_mode_begin_tick, OTA_INTERVAL_US*4)){
    		gpio_write(GPIO_LED_BLUE, !LED_ON_LEVAL);
    		gpio_write(GPIO_LED_GREEN, !LED_ON_LEVAL);
    		led_step = 4;
    	}
    	else if(led_step==4 && clock_time_exceed(ota_mode_begin_tick, OTA_INTERVAL_US*5)){
    		gpio_write(GPIO_LED_BLUE, LED_ON_LEVAL);
    		gpio_write(GPIO_LED_GREEN, LED_ON_LEVAL);
    		led_step = 5;
    	}
    	else if(led_step==5 && clock_time_exceed(ota_mode_begin_tick, OTA_INTERVAL_US*6)){
    		gpio_write(GPIO_LED_BLUE, !LED_ON_LEVAL);
    		gpio_write(GPIO_LED_GREEN, !LED_ON_LEVAL);
    		led_step = 0;


    		master_ota_test_mode = 2;
    	}
#else
    	master_ota_test_mode = 2;
#endif
    }



    //led indicate ota
    if(led_indicate_begin_tick){
#if (UI_LED_ENABLE)
    	if(led_step==0 && clock_time_exceed(led_indicate_begin_tick, LED_INTERVAL_US)){
    		gpio_write(led_indicate_gpio,LED_ON_LEVAL);
    		led_step = 1;
    	}
    	else if(led_step==1 && clock_time_exceed(led_indicate_begin_tick, LED_INTERVAL_US*2)){
    		gpio_write(led_indicate_gpio,!LED_ON_LEVAL);
    		led_step = 2;
    	}
    	else if(led_step==2 && clock_time_exceed(led_indicate_begin_tick, LED_INTERVAL_US*3)){
    		gpio_write(led_indicate_gpio,LED_ON_LEVAL);
    		led_step = 3;
    	}
    	else if(led_step==3 && clock_time_exceed(led_indicate_begin_tick, LED_INTERVAL_US*4)){
    		gpio_write(led_indicate_gpio,!LED_ON_LEVAL);
    		led_step = 4;
    	}
    	else if(led_step==4 && clock_time_exceed(led_indicate_begin_tick, LED_INTERVAL_US*5)){
    		gpio_write(led_indicate_gpio,LED_ON_LEVAL);
    		led_step = 5;
    	}
    	else if(led_step==5 && clock_time_exceed(led_indicate_begin_tick, LED_INTERVAL_US*6)){
    		gpio_write(led_indicate_gpio,!LED_ON_LEVAL);
    		led_indicate_begin_tick = 0;
    		led_step = 0;
    	}
#else
    	led_indicate_begin_tick = 0;
#endif
    }



    //process OTA timeout
    if(host_ota_start){
    	//ota timeout or disconnect. ota fail
    	if( clock_time_exceed(ota_begin_tick, OTA_TIMEOUT_S * 1000000) || blc_ll_getCurrentState() != BLS_LINK_STATE_CONN){
        	ota_set_result(0);
    		return;
    	}
    }


    u8  dat[32] = {0};

	if(host_ota_start == 0)
	{
		if(master_ota_cmd){  //UI: pc tool trig OTA mode

			if(master_ota_cmd == 1){
				#if (UI_LED_ENABLE)
					gpio_write(GPIO_LED_BLUE,LED_ON_LEVAL);  //ota begin
				#endif
			    flash_adr_ota_master = 0x20000;
			}
			else {
				#if (UI_LED_ENABLE)
					gpio_write(GPIO_LED_GREEN,LED_ON_LEVAL);  //ota begin
				#endif
			    flash_adr_ota_master = 0x40000;
			}


			n_firmware = *(u32 *)(flash_adr_ota_master + 0x18);
			if(n_firmware > (FW_SIZE_MAX<<10)){  //bigger then 128K
				ota_set_result(0);
				return;
			}


		    host_ota_start = 1;
		    ota_begin_tick = clock_time();

		    host_ota_update_pending = 1;
		}
	}
	else if(host_ota_start == 2)
	{

	    /*Send read_by_type_req to get remote's OTA handle
	     * once receive remote's response, call back function att_read_by_type_rsp_handler
	     * will be called to get the handle and set host_ota_start to 2*/
		host_att_set_current_readByTypeReq_uuid((u8 *)my_OtaUUID, 16);

	    att_req_read_by_type (dat, 0, 0xffff, read_by_type_req_uuid, 16);
	    if( blm_push_fifo (BLM_CONN_HANDLE, dat) ){
	    	host_ota_start = 3;
	    }

	}
	else if (host_ota_start == 4)
	{
		p_firmware = (u8 *)flash_adr_ota_master;
		ota_adr = 0;


		u16 ota_start = CMD_OTA_START;
		att_req_write_cmd (dat, slave_ota_handle, (u8 *)&ota_start, 2);

		if( blm_push_fifo (BLM_CONN_HANDLE, dat) ){
			host_ota_start = 5;
			otaStart_cmd_tick = clock_time();
		}

	}
	else if (host_ota_start == 5)
	{
		if( clock_time_exceed(otaStart_cmd_tick, 50000) ){
			host_ota_start = 6;
		}
	}
	else if (host_ota_start == 6)  //send ota data form addr 0 ~ n_firmware
	{
		if( !blm_ll_isRfStateMachineBusy() && blc_ll_getTxFifoNumber() < 5 )
		{
				u8 ota_buffer[20];
				rf_packet_att_ota_data_t *p = (rf_packet_att_ota_data_t *)ota_buffer;

				int nlen = ota_adr < n_firmware ? 16 : 0;

				p->adr_index = ota_adr>>4;

				if(nlen == 16){
					memcpy(p->data, p_firmware + ota_adr, 16);

					p->crc_16 = crc16((u8 *)&(p->adr_index), 18);

					att_req_write_cmd (dat, slave_ota_handle, (u8 *)p, 20);


					if( blm_push_fifo (BLM_CONN_HANDLE, dat) ){  //current data push TX fifo OK
						ota_adr += 16;  //next data index
					}

				}
				else{
					host_ota_start = 7;  //all ota data OK, to next step
				}

		}
	}
	else if(host_ota_start == 7)
	{
		if( clock_time_exceed(ota_lastData_tick, 1) ){  //50ms after last valid data, master send OTA_end cmd
			host_ota_start = 8;
		}
	}
	else if(host_ota_start == 8)   //send OTA end cmd, no need 2 byte CRC
	{
		if( !blm_ll_isRfStateMachineBusy() && blc_ll_getTxFifoNumber() < 5 )
		{
			u8 ota_buffer[20];
			rf_packet_att_ota_data_t *p = (rf_packet_att_ota_data_t *)ota_buffer;
			p->adr_index = CMD_OTA_END;

			adr_index_max_l = (ota_adr-16)>>4;
			adr_index_max_h = (ota_adr-16)>>12;
			p->data[0] = adr_index_max_l;    //adr_index_max for slave to check if any packet miss
			p->data[1] = adr_index_max_h;
			p->data[2] = ~adr_index_max_l;  //adr_index_max check
			p->data[3] = ~adr_index_max_h;

			memset(p->data + 4, 0, 12);

			att_req_write_cmd (dat, slave_ota_handle, (u8 *)p, 6);


			if( blm_push_fifo (BLM_CONN_HANDLE, dat) ){
					host_ota_start = 9;
			}
		}

	}
	else if(host_ota_start == 9)
	{
		if( blc_ll_getTxFifoNumber() == 0 ){ //all data acked,OTA OK
			ota_set_result(1);
		}
	}
}




#endif  //end of BLE_MASTER_OTA_ENABLE
