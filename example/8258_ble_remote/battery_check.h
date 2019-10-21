/********************************************************************************************************
 * @file     battery_check.h
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

#ifndef BATTERY_CHECK_H_
#define BATTERY_CHECK_H_


#define VBAT_ALRAM_THRES_MV				2000   // 2000 mV low battery alarm



void battery_set_detect_enable (int en);
int  battery_get_detect_enable (void);

int app_battery_power_check(u16 alram_vol_mv);


#endif /* APP_BATTDET_H_ */
