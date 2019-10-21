/********************************************************************************************************
 * @file     blm_att.h
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

#ifndef BLM_ATT_H_
#define BLM_ATT_H_




void	att_keyboard (u16 conn, u8 *p);
void	att_keyboard_media (u16 conn, u8 *p);
void	att_mic (u16 conn, u8 *p);
void 	att_mouse(u16 conn, u8 *p);

void host_att_data_clear(void);

#endif /* BLM_ATT_H_ */
