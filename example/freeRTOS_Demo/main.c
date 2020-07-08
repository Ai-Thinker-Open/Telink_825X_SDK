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

#include "freertos/include/freertos_api.h"

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
			while (gpio_read(GPIO_PD2) == 0){vTaskDelay(1);};

			vTaskDelay(20);
			while (1)
			{
				if(gpio_read(GPIO_PD2) == 0) 
				{
					while (gpio_read(GPIO_PD2) == 0){vTaskDelay(1);};
					return;
				}
				vTaskDelay(10);
			}
		}
		vTaskDelay(10);
	}
}

void RGB_LED_loop ()
{
	while(1)
	{
		gpio_write(GPIO_PC2, 1); 
		gpio_write(GPIO_PC3, 0); 
		gpio_write(GPIO_PC4, 0);   my_key_proocess();

		gpio_write(GPIO_PC2, 0); 
		gpio_write(GPIO_PC3, 1); 
		gpio_write(GPIO_PC4, 0);  my_key_proocess();

		gpio_write(GPIO_PC2, 0); 
		gpio_write(GPIO_PC3, 0); 
		gpio_write(GPIO_PC4, 1); my_key_proocess();
	}
}

void CW_LED_loop()
{
	while(1)
	{
		gpio_write(GPIO_PB4, 0); 
		gpio_write(GPIO_PB5, 1); 
		vTaskDelay(1000);

		gpio_write(GPIO_PB4, 1); 
		gpio_write(GPIO_PB5, 0); 
		vTaskDelay(1000);
	}
}

void Print_loop()
{
	at_print("Ai-Thinker,.LTD\r\n");

	while(1)
	{
		at_print("Hello FreeRTOS!!\r\n");
		vTaskDelay(1000);
	}
}

_attribute_ram_code_ void irq_handler(void)
{
	u32 src = reg_irq_src;
	int timerSwitching = (src & FLD_IRQ_TMR0_EN);
	int allow = irq_allow_task_switch();
	if(timerSwitching)
	{
		reg_tmr_sta = FLD_TMR_STA_TMR0; 		//clear irq status
		reg_tmr0_tick = 0;
		if(allow && xTaskIncrementTick() != pdFALSE){
			void *old = (void*)xTaskGetCurrentTaskHandle();
			vTaskSwitchContext();
			void *nw = (void*)xTaskGetCurrentTaskHandle();

			if(old != nw){
				portDISABLE_INTERRUPTS();
				vPortYieldSvc(old, nw);
			}
			return;
		}
	}
}

void system_init()
{
	blc_pm_select_internal_32k_crystal();

	cpu_wakeup_init();

	//int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

	rf_drv_init(RF_MODE_BLE_1M);

	gpio_init(1);

	app_uart_init();

#if (CLOCK_SYS_CLOCK_HZ == 16000000)
	clock_init(SYS_CLK_16M_Crystal);
#elif (CLOCK_SYS_CLOCK_HZ == 24000000)
	clock_init(SYS_CLK_24M_Crystal);
#endif

}

TaskHandle_t handle_task0;

_attribute_ram_code_ int main (void)    //must run in ramcode
{
	system_init();

	user_init();

	xTaskCreate( CW_LED_loop, "cw_task", 128, (void*)0, 6, &handle_task0 );
	xTaskCreate( RGB_LED_loop, "rgb_task", 128, (void*)0, 5, &handle_task0 );
	xTaskCreate( Print_loop, "uart_task", 1024, (void*)0, 8, &handle_task0 );

	vTaskStartScheduler();

	at_print("FreeRTOS Start Fail!\r\n");

	while (1) 
	{
		 
	};
}


