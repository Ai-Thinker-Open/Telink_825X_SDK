/********************************************************************************************************
 * @file     app_ipperCom.c
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

#include "application/usbstd/usbkeycode.h"

#include "blm_att.h"
#include "blm_pair.h"
#include "blm_host.h"
#include "blm_ota.h"


#if (UI_UPPER_COMPUTER_ENABLE)

#define CONTROL_START               0xF0
#define CONTROL_PAIR                0xF0
#define CONTROL_UNPAIR              0xF1
#define CONTROL_OTA		            0xF8
#define CONTROL_REBOOT		        0xF9

#define reg_cmd 					0x40000
#define reg_datalen 				0x40002
#define reg_att_read 				0x40008     // ~ 0x4000f 8 bytes
#define reg_cmd_result 				0x40008     // cmd exe result



int cmd_no_accept = 0;


#define SET_CMD_DONE   write_reg8(0x40000, 0);   \
					   cmd_no_accept = 0



#define KEY_MASK_PRESS		0x10
#define KEY_MASK_REPEAT		0x20
#define KEY_MASK_RELEASE	0x30


u8 release_key_pending;
u32 release_key_tick;

void    report_to_KeySimTool(u8 len,u8 * keycode)
{

#if 1  //pc tool verison_1.9 or later
		static u8 last_len = 0;
		static u8 last_key = 0;
//		static u32 last_key_tick = 0;

		u8 mask = 0;

		if(!(read_reg8(0x40004)&0xf0)){ //pc tool cleared 0x40004
			if(!len){  //release
				write_reg8(0x40004,KEY_MASK_RELEASE);
				write_reg8(0x40005,0);
			}
			else{//press or repeat
				if(last_len==len && last_key==keycode[0]){//repeat
					mask = KEY_MASK_REPEAT;
				}
				else{ //press
					mask = KEY_MASK_PRESS;
				}
				write_reg8(0x40004,mask | len);
				write_reg8(0x40005,keycode[0]);
			}
		}
		else{  //pc tool not clear t0x40004, drop the key
			if(!len){  //release can not drop
				release_key_pending = 1;
				release_key_tick = clock_time();
			}
		}

		last_len = len;
		last_key = keycode[0];

#else //old pc tool
		write_reg8(0x40004,len);
		write_reg8(0x40005,keycode[0]);
#endif
}




#define			CR_VOL_UP				0xf0  ////
#define			CR_VOL_DN				0xf1
#define			CR_VOL_MUTE				0xf2
#define			CR_CHN_UP				0xf3
#define			CR_CHN_DN				0xf4  ////
#define			CR_POWER				0xf5
#define			CR_SEARCH				0xf6
#define			CR_RECORD				0xf7
#define			CR_PLAY					0xf8  ////
#define			CR_PAUSE				0xf9
#define			CR_STOP					0xfa
#define			CR_FAST_BACKWARD		0xfb
#define			CR_FAST_FORWARD			0xfc  ////
#define			CR_HOME					0xfd
#define			CR_BACK					0xfe
#define			CR_MENU					0xff


static u16 vk_consumer_map[16] = {
		MKEY_VOL_UP,
		MKEY_VOL_DN,
		MKEY_MUTE,
		MKEY_CHN_UP,

		MKEY_CHN_DN,
		MKEY_POWER,
		MKEY_AC_SEARCH,
		MKEY_RECORD,

		MKEY_PLAY,
		MKEY_PAUSE,
		MKEY_STOP,
		MKEY_FAST_FORWARD,  //can not find fast_backword in <<HID Usage Tables>>

		MKEY_FAST_FORWARD,
		MKEY_AC_HOME,
		MKEY_AC_BACK,
		MKEY_MENU,
};




void report_media_key_to_KeySimTool(u16 media_key)
{
	u8 key_buffer[1] = {0};
	for(int i=0; i<16; i++){
		if(media_key == vk_consumer_map[i]){
			key_buffer[0] = 0xf0 +	i;
		}
	}


	report_to_KeySimTool(key_buffer[0] ? 1 : 0, key_buffer);
}





void app_upper_com_init(void)
{
    //Sram 0x808000 ~ 0x80800f  16byte can be used for communication with upper computer
    for(int i=0; i<6 ;i++)  write_reg8(0x40000+i, 0);
}



extern void host_button_trigger_ota_start(int button0_press_flag, int button1_press_flag);


void app_upper_com_proc(void)
{

	//proc KeySimTool key release
	if(release_key_pending){
		if(!(read_reg8(0x40004)&0xf0)){ //KeySimTool cleared 0x40004
			write_reg8(0x40004,KEY_MASK_RELEASE);
			write_reg8(0x40005,0);
			release_key_pending = 0;
		}

		if(clock_time_exceed(release_key_tick,100000)){
			release_key_pending = 0;
		}
	}



	static u16 cmd_time_out_counter=0;

	u8 cmd = read_reg8(reg_cmd);
	// if there is command remained , need set up a timeout, or may has a stuck
	if (cmd_no_accept) {
		if(cmd_time_out_counter++ > 0x8ff0){
			cmd_no_accept = 0;
			cmd_time_out_counter = 0;
		}
	}

	if (cmd !=0 && !cmd_no_accept) {
		cmd_no_accept = 1;  //not accept other cmd before the old cmd is done

		if (cmd < 200)   //upper computer trigger ble master send read_req
		{
			u16 att_handle = cmd;
			u8 dat[12];
			att_req_read (dat, att_handle);
			if( blm_push_fifo (BLM_CONN_HANDLE, dat) ){ //push to TX fifo OK
				app_setCurrentReadReq_attHandle(att_handle);  //mark read_req att handle
			}
		}
		else if(cmd >= CONTROL_START) //control cmd
		{
			switch(cmd)
			{
				case CONTROL_PAIR:
					//pair

					SET_CMD_DONE;
					break;

				case CONTROL_UNPAIR:
					//unpair

					SET_CMD_DONE;
					break;

				case CONTROL_OTA:

					host_button_trigger_ota_start(1, 0);  //start OTA
					break;

				case CONTROL_REBOOT:  //reboot dongle
					usb_dp_pullup_en (0);
					start_reboot();
					while(1);
					break;

				default:
					write_reg8(reg_cmd_result, 0);// control NG/ unknow cmd
					break;

			}

		}
	}

}

#endif

