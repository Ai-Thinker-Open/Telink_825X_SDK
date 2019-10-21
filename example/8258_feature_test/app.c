/********************************************************************************************************
 * @file     app.c 
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
#include "app.h"
#include <stack/ble/ble.h>
#include "tl_common.h"
#include "drivers.h"
#include "app_config.h"
#include "vendor/common/blt_led.h"
#include "application/keyboard/keyboard.h"
#include "vendor/common/tl_audio.h"
#include "vendor/common/blt_soft_timer.h"



void user_init_normal(void)
{
#if (   FEATURE_TEST_MODE == TEST_ADVERTISING_ONLY || FEATURE_TEST_MODE == TEST_SCANNING_ONLY \
	 || FEATURE_TEST_MODE == TEST_ADVERTISING_IN_CONN_SLAVE_ROLE \
	 || FEATURE_TEST_MODE==TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE \
	 || FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)

	feature_linklayer_state_test_init_normal();

#elif (FEATURE_TEST_MODE == TEST_POWER_ADV)

	feature_adv_power_test_init_normal();

#elif (FEATURE_TEST_MODE == TEST_SMP_SECURITY)

	feature_security_test_init_normal();

#elif (FEATURE_TEST_MODE == TEST_GATT_SECURITY)

	feature_gatt_security_test_init_normal();

#elif (FEATURE_TEST_MODE == TEST_SDATA_LENGTH_EXTENSION)
	feature_sdle_test_init_normal();

#elif (FEATURE_TEST_MODE == TEST_MDATA_LENGTH_EXTENSION)
	feature_mdle_test_init_normal();

#elif (FEATURE_TEST_MODE == TEST_USER_BLT_SOFT_TIMER)

	feature_soft_timer_test_init_normal();

#elif (FEATURE_TEST_MODE == TEST_WHITELIST)

	feature_whitelist_test_init_normal();

#elif (FEATURE_TEST_MODE == TEST_BLE_PHY)

	feature_phytest_init_normal();

#elif (FEATURE_TEST_MODE == TEST_EXTENDED_ADVERTISING)

	feature_ext_adv_init_normal();

#elif (FEATURE_TEST_MODE == TEST_2M_CODED_PHY_EXT_ADV)

	feature_2m_coded_phy_adv_init_normal();

#elif (FEATURE_TEST_MODE == TEST_2M_CODED_PHY_CONNECTION)

	feature_2m_coded_phy_conn_init_normal();

#elif (FEATURE_TEST_MODE == TEST_CSA2)

	feature_csa2_init_normal();

#else

#endif
}




_attribute_ram_code_ void user_init_deepRetn(void)
{

#if (   FEATURE_TEST_MODE == TEST_ADVERTISING_ONLY || FEATURE_TEST_MODE == TEST_SCANNING_ONLY \
	 || FEATURE_TEST_MODE == TEST_ADVERTISING_IN_CONN_SLAVE_ROLE \
	 || FEATURE_TEST_MODE==TEST_SCANNING_IN_ADV_AND_CONN_SLAVE_ROLE \
	 || FEATURE_TEST_MODE == TEST_ADVERTISING_SCANNING_IN_CONN_SLAVE_ROLE)

	feature_linklayer_state_test_init_deepRetn();

#elif (FEATURE_TEST_MODE == TEST_POWER_ADV)

	feature_adv_power_test_init_deepRetn();

#elif (FEATURE_TEST_MODE == TEST_SMP_SECURITY)

	feature_security_test_init_deepRetn();

#elif (FEATURE_TEST_MODE == TEST_GATT_SECURITY)

	feature_gatt_security_test_init_deepRetn();

#elif (FEATURE_TEST_MODE == TEST_SDATA_LENGTH_EXTENSION)
	feature_sdle_test_init_deepRetn();

#elif (FEATURE_TEST_MODE == TEST_MDATA_LENGTH_EXTENSION)
	feature_mdle_test_init_deepRetn();

#elif (FEATURE_TEST_MODE == TEST_USER_BLT_SOFT_TIMER)

	feature_soft_timer_test_init_deepRetn();

#elif (FEATURE_TEST_MODE == TEST_WHITELIST)

	feature_whitelist_test_init_deepRetn();

#elif (FEATURE_TEST_MODE == TEST_BLE_PHY)


#elif (FEATURE_TEST_MODE == TEST_EXTENDED_ADVERTISING)

	feature_ext_adv_init_deepRetn();

#elif (FEATURE_TEST_MODE == TEST_2M_CODED_PHY_EXT_ADV)

	feature_2m_coded_phy_adv_init_deepRetn();

#elif (FEATURE_TEST_MODE == TEST_2M_CODED_PHY_CONNECTION)

	feature_2m_coded_phy_conn_init_deepRetn();

#elif (FEATURE_TEST_MODE == TEST_CSA2)

	feature_csa2_init_deepRetn();

#else

#endif
}





u32 tick_loop=0;
/*----------------------------------------------------------------------------*/
/*-------- Main Loop                                                ----------*/
/*----------------------------------------------------------------------------*/
_attribute_ram_code_ void main_loop (void)
{
	tick_loop++;

#if (FEATURE_TEST_MODE == TEST_USER_BLT_SOFT_TIMER)
	blt_soft_timer_process(MAINLOOP_ENTRY);
#endif

	blt_sdk_main_loop();

#if (FEATURE_TEST_MODE == TEST_SMP_SECURITY)
	feature_security_test_mainloop();
#elif (FEATURE_TEST_MODE == TEST_SDATA_LENGTH_EXTENSION)
	feature_sdle_test_mainloop();
#elif (FEATURE_TEST_MODE == TEST_MDATA_LENGTH_EXTENSION)
	feature_mdle_test_mainloop();
#elif (FEATURE_TEST_MODE == TEST_2M_CODED_PHY_CONNECTION)
	feature_2m_coded_phy_conn_mainloop();
#endif
}

/*----------------------------- End of File ----------------------------------*/


