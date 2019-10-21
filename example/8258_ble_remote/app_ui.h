/********************************************************************************************************
 * @file	 app_ui.h
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

#ifndef APP_UI_H_
#define APP_UI_H_


void app_ui_init_normal(void);
void app_ui_init_deepRetn(void);

void proc_keyboard (u8 e, u8 *p, int n);

void task_audio (void);

void ui_enable_mic (int en);

#if (BLE_REMOTE_OTA_ENABLE)
	void app_enter_ota_mode(void);
	void app_debug_ota_result(int result);
#endif



extern 	u8 		key_type;
extern	int 	key_not_released;
extern	int     ui_mtu_size_exchange_req;
extern	int 	ir_not_released;
extern	u8 		user_key_mode;
extern	u8      ir_hw_initialed;
extern	u8 		ota_is_working;
extern	u8		ui_mic_enable;
extern	u8 		key_voice_press;
extern	int		lowBatt_alarmFlag;











#endif /* APP_UI_H_ */
