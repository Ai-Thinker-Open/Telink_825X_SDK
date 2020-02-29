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
#include "vendor/common/user_config.h"
#include "app_config.h"

#include "drivers/8258/gpio_8258.h"

void user_init()
{
	gpio_set_func(GPIO_PC2, AS_GPIO);
	gpio_set_func(GPIO_PC3, AS_GPIO);
	gpio_set_func(GPIO_PC4, AS_GPIO);
	gpio_set_func(GPIO_PB4, AS_GPIO);
	gpio_set_func(GPIO_PB5, AS_GPIO);

	gpio_set_output_en(GPIO_PC2, 1);
	gpio_set_output_en(GPIO_PC3, 1);
	gpio_set_output_en(GPIO_PC4, 1);
	gpio_set_output_en(GPIO_PB4, 1);
	gpio_set_output_en(GPIO_PB5, 1);

	gpio_set_input_en(GPIO_PC2, 0); 
	gpio_set_input_en(GPIO_PC3, 0); 
	gpio_set_input_en(GPIO_PC4, 0); 
	gpio_set_input_en(GPIO_PB4, 0); 
	gpio_set_input_en(GPIO_PB5, 0); 

	gpio_set_func(GPIO_PD2, AS_GPIO);
	gpio_setup_up_down_resistor(GPIO_PD2, PM_PIN_PULLUP_10K);
	gpio_set_output_en(GPIO_PD2, 0);
	gpio_set_input_en(GPIO_PD2, 1); 	
}

void my_key_proocess()
{
	int c = 20;
	while (c--)
	{	
		if(gpio_read(GPIO_PD2) == 0)
		{
			while (gpio_read(GPIO_PD2) == 0){sleep_ms(1);};

			sleep_ms(20);
			while (1)
			{
				if(gpio_read(GPIO_PD2) == 0) 
				{
					while (gpio_read(GPIO_PD2) == 0){sleep_ms(1);};
					return;
				}
				sleep_ms(1);
			}
		}
		sleep_ms(10);
	}
}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
void main_loop ()
{
	gpio_write(GPIO_PC2, 1); 
	gpio_write(GPIO_PC3, 0); 
	gpio_write(GPIO_PC4, 0); 
	gpio_write(GPIO_PB4, 0); 
	gpio_write(GPIO_PB5, 0);  my_key_proocess();

	gpio_write(GPIO_PC2, 0); 
	gpio_write(GPIO_PC3, 1); 
	gpio_write(GPIO_PC4, 0); 
	gpio_write(GPIO_PB4, 0); 
	gpio_write(GPIO_PB5, 0); my_key_proocess();

	gpio_write(GPIO_PC2, 0); 
	gpio_write(GPIO_PC3, 0); 
	gpio_write(GPIO_PC4, 1); 
	gpio_write(GPIO_PB4, 0); 
	gpio_write(GPIO_PB5, 0); my_key_proocess();

	gpio_write(GPIO_PC2, 0); 
	gpio_write(GPIO_PC3, 0); 
	gpio_write(GPIO_PC4, 0); 
	gpio_write(GPIO_PB4, 1); 
	gpio_write(GPIO_PB5, 0); my_key_proocess();

	gpio_write(GPIO_PC2, 0); 
	gpio_write(GPIO_PC3, 0); 
	gpio_write(GPIO_PC4, 0); 
	gpio_write(GPIO_PB4, 0); 
	gpio_write(GPIO_PB5, 5); my_key_proocess();
}

_attribute_ram_code_ void irq_handler(void)
{
	return;
}

void system_init()
{
	blc_pm_select_internal_32k_crystal();

	cpu_wakeup_init();

	//int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

	rf_drv_init(RF_MODE_BLE_1M);

	gpio_init(1);

#if (CLOCK_SYS_CLOCK_HZ == 16000000)
	clock_init(SYS_CLK_16M_Crystal);
#elif (CLOCK_SYS_CLOCK_HZ == 24000000)
	clock_init(SYS_CLK_24M_Crystal);
#endif

}

_attribute_ram_code_ int main (void)    //must run in ramcode
{
	system_init();

	user_init();

    irq_enable();

	while (1) 
	{
#if (MODULE_WATCHDOG_ENABLE)
		wd_clear(); //clear watch dog
#endif
		main_loop ();
	}
}


