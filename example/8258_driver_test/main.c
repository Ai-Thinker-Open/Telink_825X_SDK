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



extern my_fifo_t hci_rx_fifo;
extern void user_init();
extern void main_loop (void);

extern void app_i2c_test_irq_proc(void);
extern void app_uart_test_irq_proc(void);
extern void app_spi_test_irq_proc(void);
extern void app_timer_test_irq_proc(void);
extern void app_gpio_irq_test_proc(void);
extern void app_pwm_irq_test_proc(void);


_attribute_ram_code_ void irq_handler(void)
{

/***timer demo***/
#if (DRIVER_TEST_MODE == TEST_HW_TIMER)
	app_timer_test_irq_proc();

/***uart demo***/
#elif (DRIVER_TEST_MODE == TEST_UART)
	app_uart_test_irq_proc();

/***i2c demo***/
#elif (DRIVER_TEST_MODE == TEST_IIC)

	app_i2c_test_irq_proc();

/***spi demo ***/
#elif (DRIVER_TEST_MODE == TEST_SPI)
	app_spi_test_irq_proc();

#elif (DRIVER_TEST_MODE == TEST_GPIO_IRQ)

	app_gpio_irq_test_proc();

#elif (DRIVER_TEST_MODE == TEST_PWM)

	app_pwm_irq_test_proc();

#endif

}

int main (void) {

	blc_pm_select_internal_32k_crystal();

	cpu_wakeup_init();

	int deepRetWakeUp = pm_is_MCU_deepRetentionWakeup();  //MCU deep retention wakeUp

	rf_drv_init(RF_MODE_BLE_1M);

	gpio_init(!deepRetWakeUp);

#if (CLOCK_SYS_CLOCK_HZ == 16000000)
	clock_init(SYS_CLK_16M_Crystal);
#elif (CLOCK_SYS_CLOCK_HZ == 24000000)
	clock_init(SYS_CLK_24M_Crystal);
#endif

	if(!deepRetWakeUp){
		random_generator_init();
	}

	user_init();

	while (1) {
#if (MODULE_WATCHDOG_ENABLE)
		wd_clear(); //clear watch dog
#endif
		main_loop ();
	}
}




