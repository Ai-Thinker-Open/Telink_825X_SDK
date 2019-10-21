/********************************************************************************************************
 * @file     audio_buffer.c
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
#include "vendor/common/tl_audio.h"
#define			USB_ISO_IN_SIZE			(MIC_SAMPLE_RATE / 1000)

#define			PACK_POINTER				(abuf_dec_rptr/248) | (abuf_dec_wptr<<8) | (abuf_mic_wptr<<16)
/////////////////////////////////////////////////////////
//	abuf_mic:
//		abuf_mic_wptr: step by 2
//		abuf_dec_wptr: step by 8	(abuf_mic_rptr)
/////////////////////////////////////////////////////////
u8		abuf_mic_wptr, abuf_dec_wptr;
u16		abuf_dec_rptr;

#define DEC_BUFFER_SIZE		(MIC_SHORT_DEC_SIZE<<2)

u8		abuf_mic[MIC_ADPCM_FRAME_SIZE * 4];
s16		abuf_dec[DEC_BUFFER_SIZE];

int		abuf_reset = 0;


void abuf_init ()
{
	abuf_mic_wptr = abuf_dec_wptr = 0;
	abuf_reset = 16;
	log_event (TR_T_abuf_overflow);
}

void abuf_mic_add (u32 *p)
{
	log_event (TR_T_abuf_add);
	u32 *pd = (u32 *) (abuf_mic + (abuf_mic_wptr & 3) * MIC_ADPCM_FRAME_SIZE);
	for (int i=0; i<(MIC_ADPCM_FRAME_SIZE>>2); i++)
	{
		*pd ++ = *p++;
	}
	abuf_mic_wptr ++;
}


#if (CLOCK_SYS_CLOCK_HZ == 24000000)
_attribute_ram_code_
#endif
void abuf_mic_dec ()
{
	static int start = 1;
	static int abuf_reset_no;
	if (abuf_reset)
	{
		abuf_dec_wptr = abuf_mic_wptr;
	}
	else
	{
		u8 num_mic = abuf_mic_wptr - abuf_dec_wptr;
		u8 num_dec = abuf_dec_wptr - (abuf_dec_rptr/MIC_SHORT_DEC_SIZE);

		if (num_mic > 4) 			// in case of overflow
		{
			log_event (TR_T_abuf_overflow_mic);
			log_data (TR_24_abuf_dec_rptr, PACK_POINTER);
			abuf_dec_wptr ++;
		}

		if (num_dec > 4)
		{
			//log_event (TR_T_abuf_overflow_dec);
			//log_data (TR_24_abuf_dec_wptr, (abuf_dec_rptr>>8) | (abuf_dec_wptr<<8) | (abuf_mic_wptr<<16));

			log_event (TR_T_abuf_overflow_dec);
			abuf_reset = 16;
			start = 1;
			abuf_reset_no++;
		}
		else if ( ((!start && num_mic>=1) || (start && num_mic>=2)) && (num_dec <= 3) )
		{
			log_event (TR_T_abuf_dec);
			log_data (TR_24_abuf_dec_wptr, PACK_POINTER);
			adpcm_to_pcm (
					(s16 *) (abuf_mic + (abuf_dec_wptr & 3) * MIC_ADPCM_FRAME_SIZE),
					abuf_dec + (abuf_dec_wptr & 3) * MIC_SHORT_DEC_SIZE,
					MIC_SHORT_DEC_SIZE );

			abuf_dec_wptr ++;			// 256-byte = 128-s16
			start = 0;
		}
	}
}

_attribute_ram_code_ void abuf_dec_usb ()
{
	log_event (TR_T_abuf_usb);
	log_data (TR_24_abuf_dec_rptr, PACK_POINTER);
	log_data (TR_24_abuf_reset, abuf_reset);
	static u32 tick_usb_iso_in;
	static u8  buffer_empty = 1;
	static u8  n_usb_iso = 0;

	n_usb_iso++;

	if (clock_time_exceed (tick_usb_iso_in, 4000))
	{
		abuf_reset = 16;
	}

	tick_usb_iso_in = clock_time ();
	if (abuf_reset)
	{
		abuf_dec_rptr = abuf_dec_wptr*MIC_SHORT_DEC_SIZE;
		abuf_reset--;
	}
	/////////////////// copy data to usb iso in buffer ///////////////
	reg_usb_ep7_ptr = 0;
	u8 num = abuf_dec_wptr - (abuf_dec_rptr/MIC_SHORT_DEC_SIZE);
	if (num)
	{
		if ( (buffer_empty && num >= 3) || (!buffer_empty && (num >= 1 || (n_usb_iso & 3))) )
		{
			buffer_empty = 0;

			u16 offset = abuf_dec_rptr%DEC_BUFFER_SIZE;
			s16 *ps = abuf_dec + offset;


			if(offset == DEC_BUFFER_SIZE - (USB_ISO_IN_SIZE/2)){
				for (int i=0; i<(USB_ISO_IN_SIZE/2); i++)
				{
					reg_usb_ep7_dat = *ps;
					reg_usb_ep7_dat = *ps++ >> 8;
				}
				ps = abuf_dec;
				for (int i=0; i<(USB_ISO_IN_SIZE/2); i++)
				{
					reg_usb_ep7_dat = *ps;
					reg_usb_ep7_dat = *ps++ >> 8;
				}
			}
			else{
				for (int i=0; i<USB_ISO_IN_SIZE; i++)
				{
					reg_usb_ep7_dat = *ps;
					reg_usb_ep7_dat = *ps++ >> 8;
				}
			}


			abuf_dec_rptr += USB_ISO_IN_SIZE;
			if(abuf_dec_rptr >= (MIC_SHORT_DEC_SIZE<<8) ){
				abuf_dec_rptr = 0;
			}
		}
		else
		{
			for (int i=0; i<USB_ISO_IN_SIZE * 2; i++)
			{
				reg_usb_ep7_dat = 0;
			}
		}
	}
	else
	{
		for (int i=0; i<USB_ISO_IN_SIZE * 2; i++)
		{
			reg_usb_ep7_dat = 0;
		}
		buffer_empty = 1;
		log_event (TR_T_abuf_zero);
	}
	reg_usb_ep7_ctrl = BIT(0);			//ACK iso in
}
