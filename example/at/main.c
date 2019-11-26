/********************************************************************************************************
 * @file     main.c
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
#include "vendor/common/user_config.h"
#include "tinyFlash/tinyFlash.h"

extern void user_init_normal();
extern void user_init_deepRetn();

extern void main_loop (void);
extern u8 baud_buf[];
extern u8 ATE;

_attribute_my_ram_code_ void irq_handler(void)
{
	app_uart_irq_proc();

	irq_blt_sdk_handler();
}


_attribute_ram_code_ int main (void)    //must run in ramcode
{
	blc_pm_select_internal_32k_crystal();

	cpu_wakeup_init();

	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

	rf_drv_init(RF_MODE_BLE_1M);

	gpio_init( !deepRetWakeUp );  //analog resistance will keep available in deepSleep mode, so no need initialize again

#if (CLOCK_SYS_CLOCK_HZ == 16000000)
	clock_init(SYS_CLK_16M_Crystal);
#elif (CLOCK_SYS_CLOCK_HZ == 24000000)
	clock_init(SYS_CLK_24M_Crystal);
#endif

	blc_app_loadCustomizedParameters();  //load customized freq_offset cap value

	tinyFlash_Init(0x70000,0x4000); //初始化KV存储系统

	u8 len =1;

	tinyFlash_Read(2, baud_buf, &len); //读取波特率

	app_uart_init(baud_buf[0]);  //初始化串口

	my_gpio_init(); //初始化GPIO

	tinyFlash_Read(3, &ATE, &len); //读取ATE

	at_print("    \r\nAi-Thinker Ble AT V 0.1\r\n+READY\r\n");
	
	if( deepRetWakeUp ){
		user_init_deepRetn ();
	}
	else{
		user_init_normal ();
	}

	while (1) {
#if (MODULE_WATCHDOG_ENABLE)
		wd_clear(); //clear watch dog
#endif
		main_loop ();
	}
}

 