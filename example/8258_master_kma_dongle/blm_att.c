/********************************************************************************************************
 * @file  	 blm_att.c
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

#include "application/keyboard/keyboard.h"
#include "application/rf_frame.h"

#define     TELINK_UNPAIR_KEYVALUE		0xFF  //conn state, unpair


const u8 my_MicUUID[16]		= TELINK_MIC_DATA;
const u8 my_SpeakerUUID[16]	= TELINK_SPEAKER_DATA;
const u8 my_OtaUUID[16]		= TELINK_SPP_DATA_OTA;
const u8 my_SppS2CUUID[16]		= TELINK_SPP_DATA_SERVER2CLIENT;
const u8 my_SppC2SUUID[16]		= TELINK_SPP_DATA_CLIENT2SERVER;



u8 read_by_type_req_uuid[16] = {};
u8 read_by_type_req_uuidLen;

u16 	current_read_req_handle;

void host_att_set_current_readByTypeReq_uuid(u8 *uuid, u8 uuid_len)
{
	read_by_type_req_uuidLen = uuid_len;
	memcpy(read_by_type_req_uuid, uuid, uuid_len);
}







u8	*p_att_response = 0;

volatile u32	host_att_req_busy = 0;

int host_att_client_handler (u16 connHandle, u8 *p)
{
	att_readByTypeRsp_t *p_rsp = (att_readByTypeRsp_t *) p;
	if (p_att_response)
	{
		if ((connHandle & 7) == (host_att_req_busy & 7) && p_rsp->chanId == 0x04 &&
				(p_rsp->opcode == 0x01 || p_rsp->opcode == ((host_att_req_busy >> 16) | 1)))
		{
			memcpy (p_att_response, p, 32);
			host_att_req_busy = 0;
		}
	}
	return 0;
}

void host_att_service_disccovery_clear(void)
{
	p_att_response = 0;
}

typedef int (*host_att_idle_func_t) (void);
host_att_idle_func_t host_att_idle_func = 0;

int host_att_register_idle_func (void *p)
{
	if (host_att_idle_func)
		return 1;

	host_att_idle_func = p;
	return 0;
}

int host_att_response ()
{
	return host_att_req_busy == 0;
}


int host_att_service_wait_event (u16 handle, u8 *p, u32 timeout)
{
	host_att_req_busy = handle | (p[6] << 16);
	p_att_response = p;
	blm_push_fifo (handle, p);

	u32 t = clock_time ();
	while (!clock_time_exceed (t, timeout))
	{
		if (host_att_response ())
		{
			return 0;
		}
		if (host_att_idle_func)
		{
			if (host_att_idle_func ())
			{
				break;
			}
		}
	}
	return 1;
}


ble_sts_t  host_att_discoveryService (u16 handle, att_db_uuid16_t *p16, int n16, att_db_uuid128_t *p128, int n128)
{
	att_db_uuid16_t *ps16 = p16;
	att_db_uuid128_t *ps128 = p128;
	int i16 = 0;
	int i128 = 0;

	ps16->num = 0;
	ps128->num = 0;

	// char discovery: att_read_by_type
		// hid discovery: att_find_info
	u8  dat[32];
	u16 s = 1;
	u16 uuid = GATT_UUID_CHARACTER;
	do {

		att_req_read_by_type (dat, s, 0xffff, (u8 *)&uuid, 2);
		if (host_att_service_wait_event(handle, dat, 1000000))
		{
			return  GATT_ERR_SERVICE_DISCOVERY_TIEMOUT;			//timeout
		}

		// process response data
		att_readByTypeRsp_t *p_rsp = (att_readByTypeRsp_t *) dat;
		if (p_rsp->opcode != ATT_OP_READ_BY_TYPE_RSP)
		{
			break;
		}

		if (p_rsp->datalen == 21)		//uuid128
		{
			s = p_rsp->data[3] + p_rsp->data[4] * 256;
			if (i128 < n128)
			{
				p128->property = p_rsp->data[2];
				p128->handle = s;
				memcpy (p128->uuid, p_rsp->data + 5, 16);
				i128++;
				p128++;
			}
		}
		else if (p_rsp->datalen == 7) //uuid16
		{
			u8 *pd = p_rsp->data;
			while (p_rsp->l2capLen > 7)
			{
				s = pd[3] + pd[4] * 256;
				if (i16 < n16)
				{
					p16->property = pd[2];
					p16->handle = s;
					p16->uuid = pd[5] | (pd[6] << 8);
					p16->ref = 0;
					i16 ++;
					p16++;
				}
				p_rsp->l2capLen -= 7;
				pd += 7;
			}
		}
	} while (1);

	ps16->num = i16;
	ps128->num = i128;

	//--------- use att_find_info to find the reference property for hid ----------
	p16 = ps16;
	for (int i=0; i<i16; i++)
	{
		if (p16->uuid == CHARACTERISTIC_UUID_HID_REPORT)		//find reference
		{

			att_req_find_info (dat, p16->handle, 0xffff);
			if (host_att_service_wait_event(handle, dat, 1000000))
			{
				return  GATT_ERR_SERVICE_DISCOVERY_TIEMOUT;			//timeout
			}

			att_findInfoRsp_t *p_rsp = (att_findInfoRsp_t *) dat;
			if (p_rsp->opcode == ATT_OP_FIND_INFO_RSP && p_rsp->format == 1)
			{
				int n = p_rsp->l2capLen - 2;
				u8 *pd = p_rsp->data;
				while (n > 0)
				{
					if ((pd[2]==U16_LO(GATT_UUID_CHARACTER) && pd[3]==U16_HI(GATT_UUID_CHARACTER)) ||
						(pd[2]==U16_LO(GATT_UUID_PRIMARY_SERVICE) && pd[3]==U16_HI(GATT_UUID_PRIMARY_SERVICE))	)
					{
						break;
					}

					if (pd[2]==U16_LO(GATT_UUID_REPORT_REF) && pd[3]==U16_HI(GATT_UUID_REPORT_REF))
					{
					//-----------		read attribute ----------------

						att_req_read (dat, pd[0]);
						if (host_att_service_wait_event(handle, dat, 1000000))
						{
								return  GATT_ERR_SERVICE_DISCOVERY_TIEMOUT;			//timeout
						}

						att_readRsp_t *pr = (att_readRsp_t *) dat;
						if (pr->opcode == ATT_OP_READ_RSP)
						{
							p16->ref = pr->value[0] | (pr->value[1] << 8);
						}

						break;
					}
					n -= 4;
					pd += 4;
				}
			}
		} //----- end for if CHARACTERISTIC_UUID_HID_REPORT

		p16++;
	}

	return  BLE_SUCCESS;
}







rf_packet_mouse_t	pkt_mouse = {
		sizeof (rf_packet_mouse_t) - 4,	// dma_len

		sizeof (rf_packet_mouse_t) - 5,	// rf_len
		RF_PROTO_BYTE,		// proto
		PKT_FLOW_DIR,		// flow
		FRAME_TYPE_MOUSE,					// type

//		U32_MAX,			// gid0

		0,					// rssi
		0,					// per
		0,					// seq_no
		1,					// number of frame
};


extern void usbmouse_add_frame (rf_packet_mouse_t *packet_mouse);

void	att_mouse (u16 conn, u8 *p)
{
	memcpy (pkt_mouse.data, p, 4);
	pkt_mouse.seq_no++;
    usbmouse_add_frame(&pkt_mouse);
}



extern void usbkb_hid_report(kb_data_t *data);
extern void report_to_KeySimTool(u8 len,u8 * keycode);
extern void usbkb_report_consumer_key(u16 consumer_key);

extern void report_media_key_to_KeySimTool(u16);

void	att_keyboard_media (u16 conn, u8 *p)
{
	u16 media_key = p[0] | p[1]<<8;


#if (UI_UPPER_COMPUTER_ENABLE)
	if(read_reg8(0) == 0x5a){ //report to KeySimTool  mode
		report_media_key_to_KeySimTool(media_key);
	}
	else{
		usbkb_report_consumer_key(media_key);
	}
#else
	usbkb_report_consumer_key(media_key);
#endif

}

//////////////// keyboard ///////////////////////////////////////////////////
int Adbg_att_kb_cnt = 0;
kb_data_t		kb_dat_report = {1, 0, {0,0,0,0,0,0} };
int keyboard_not_release = 0;
extern int 	dongle_unpair_enable;
void	att_keyboard (u16 conn, u8 *p)
{
	Adbg_att_kb_cnt ++;

	memcpy(&kb_dat_report, p, sizeof(kb_data_t));

	if(kb_dat_report.keycode[0] == TELINK_UNPAIR_KEYVALUE){ //slave special unpair cmd

		if(!dongle_unpair_enable){
			dongle_unpair_enable = 1;
		}

		return;  //TELINK_UNPAIR_KEYVALUE not report
	}


	if (kb_dat_report.keycode[0])  			//keycode[0]
	{
		kb_dat_report.cnt = 1;  //1 key value
		keyboard_not_release = 1;
	}
	else{
		kb_dat_report.cnt = 0;  //key release
		keyboard_not_release = 0;
	}


#if (UI_UPPER_COMPUTER_ENABLE)
	if(read_reg8(0) == 0x5a){ //report to KeySimTool  mode
		report_to_KeySimTool(kb_dat_report.cnt, kb_dat_report.keycode);
	}
	else{
		usbkb_hid_report((kb_data_t *) &kb_dat_report);
	}
#else
	usbkb_hid_report((kb_data_t *) &kb_dat_report);
#endif

}




void att_keyboard_release(void)
{
	kb_dat_report.cnt = 0;  //key release
//	usbkb_hid_report((kb_data_t *) &kb_dat_report);
}



////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
extern	void abuf_init ();
extern	void abuf_mic_add (u32 *p);
extern	void abuf_mic_dec (void);
extern	void abuf_dec_usb (void);

u8		att_mic_rcvd = 0;
u32		tick_adpcm;
u8		buff_mic_adpcm[MIC_ADPCM_FRAME_SIZE];

u32		tick_iso_in;
int		mode_iso_in;
_attribute_ram_code_ void  usb_endpoints_irq_handler (void)
{
	u32 t = clock_time ();
	/////////////////////////////////////
	// ISO IN
	/////////////////////////////////////
	if (reg_usb_irq & BIT(7)) {
		mode_iso_in = 1;
		tick_iso_in = t;
		reg_usb_irq = BIT(7);	//clear interrupt flag of endpoint 7

		/////// get MIC input data ///////////////////////////////
		//usb_iso_in_1k_square ();
		//usb_iso_in_from_mic ();
		abuf_dec_usb ();
	}

}

#if (!AUDIO_SDM_ENBALE)
void	att_mic (u16 conn, u8 *p)
{
	att_mic_rcvd = 1;
	memcpy (buff_mic_adpcm, p, MIC_ADPCM_FRAME_SIZE);
	abuf_mic_add ((u32 *)buff_mic_adpcm);
}
#else
s16 temp_buf[248];
void	att_mic (u16 conn, rf_packet_att_t *p)
{
	att_mic_rcvd = 1;
	memcpy (buff_mic_adpcm, p->dat, MIC_ADPCM_FRAME_SIZE);
	adpcm_to_pcm((s16 *)(buff_mic_adpcm),temp_buf,248);
	pcm_to_sdm (temp_buf, 248);
}
#endif


_attribute_ram_code_ void proc_audio (void)
{
	if (att_mic_rcvd)
	{
		tick_adpcm = clock_time ();
		att_mic_rcvd = 0;
	}
	if (clock_time_exceed (tick_adpcm, 200000))
	{
		tick_adpcm = clock_time ();
		abuf_init ();
	}
	abuf_mic_dec ();
}




void host_att_data_clear(void)
{
	if(keyboard_not_release){
		keyboard_not_release = 0;
		att_keyboard_release();
	}
}



void app_setCurrentReadReq_attHandle(u16 handle)
{
	current_read_req_handle = handle;
}

u16 app_getCurrentReadReq_attHandle(void)
{
	return current_read_req_handle;
}


