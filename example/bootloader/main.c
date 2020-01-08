/********************************************************************************************************
 * @file     main.c 
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     May. 12, 2018
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

extern void uart_print(char * str);
extern void app_uart_init(void);
extern void app_uart_loop(void);

#define BOOT_PIN GPIO_PA1

void boot_jump()
{
	asm("tjl _uart_boot_start_");//跳转到汇编，去拷贝应用程序
}

void system_init()
{
 	cpu_wakeup_init();
	
	gpio_init(0);

	gpio_set_func(BOOT_PIN, AS_GPIO);

	gpio_setup_up_down_resistor(BOOT_PIN, PM_PIN_PULLUP_10K);

	gpio_set_output_en(BOOT_PIN, 0);

	gpio_set_input_en(BOOT_PIN, 1);

	if(gpio_read(BOOT_PIN) != 0)
	{
		gpio_setup_up_down_resistor(BOOT_PIN, PM_PIN_UP_DOWN_FLOAT);
		boot_jump();
	}

	blc_pm_select_internal_32k_crystal();

#if (CLOCK_SYS_CLOCK_HZ == 16000000)
	clock_init(SYS_CLK_16M_Crystal);
#elif (CLOCK_SYS_CLOCK_HZ == 24000000)
	clock_init(SYS_CLK_24M_Crystal);
#endif
}

void main_loop (void)
{
	app_uart_loop();
}

void (*jump)(void)  = 0xA0; 

int main (void) 
{
	system_init();

	app_uart_init();

	uart_print("     \r\nboot loader ready\r\n");

	while (1) 
	{
#if (MODULE_WATCHDOG_ENABLE)
		wd_clear(); //clear watch dog
#endif
		main_loop ();
	}
}




