/********************************************************************************************************
 * @file     app.h 
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
#ifndef _APP_H
#define _APP_H

#include "tl_common.h"
#include "drivers.h"

void user_init_normal(void);
void user_init_deepRetn(void);
void main_loop (void);


void feature_linklayer_state_test_init_normal(void);
void feature_linklayer_state_test_init_deepRetn(void);


void feature_adv_power_test_init_normal(void);
void feature_adv_power_test_init_deepRetn(void);


void feature_security_test_init_normal(void);
void feature_security_test_init_deepRetn(void);
void feature_security_test_mainloop(void);


void feature_soft_timer_test_init_normal(void);
void feature_soft_timer_test_init_deepRetn(void);


void feature_whitelist_test_init_normal(void);
void feature_whitelist_test_init_deepRetn(void);


void feature_phytest_init_normal(void);
void feature_phytest_irq_proc(void);


void feature_sdle_test_init_normal(void);
void feature_sdle_test_init_deepRetn(void);
void feature_sdle_test_mainloop(void);


void feature_mdle_test_init_normal(void);
void feature_mdle_test_init_deepRetn(void);
void feature_mdle_test_mainloop(void);


void feature_ext_adv_init_normal(void);
void feature_ext_adv_init_deepRetn(void);


void feature_2m_coded_phy_adv_init_normal(void);


void feature_2m_coded_phy_conn_init_normal(void);
void feature_2m_coded_phy_conn_init_deepRetn(void);
void feature_2m_coded_phy_conn_mainloop(void);

void feature_csa2_init_normal(void);
void feature_csa2_init_deepRetn(void);

#endif /* APP_H_ */
