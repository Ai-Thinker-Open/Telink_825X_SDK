/********************************************************************************************************
 * @file     app_gpio_irq.c 
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
/*
 * app_gpio_irq.c
 *
 *  Created on: 2017-8-25
 *      Author: Administrator
 */
#include "tl_common.h"
#include "drivers.h"

#if (DRIVER_TEST_MODE == TEST_GPIO_IRQ)




//you can change IO here
#define  GPIO_LED1_PIN				GPIO_PA0
#define  GPIO_LED2_PIN				GPIO_PA1
#define  GPIO_LED3_PIN				GPIO_PA2


#define  LED_INIT_LEVEL				0


#define GPIO_TEST_PIN1				GPIO_PD0
#define GPIO_TEST_PIN2				GPIO_PD1
#define GPIO_TEST_PIN3				GPIO_PD2


unsigned char gpio_irq_test_mode = 0;

void app_gpio_irq_test_init(void)
{

//demo1: IRQ_GPIO,       TEST_PIN1, falling edge triggers
	/***step1. set pin as gpio and enable input********/
	gpio_set_func(GPIO_TEST_PIN1, AS_GPIO);           //enable GPIO func
	gpio_set_input_en(GPIO_TEST_PIN1, 1);             //enable input
	gpio_set_output_en(GPIO_TEST_PIN1, 0);            //disable output

	/***step2.      set the polarity and open pullup ***/
	gpio_setup_up_down_resistor(GPIO_TEST_PIN1, PM_PIN_PULLUP_10K);  //open pull up resistor
	gpio_set_interrupt_pol(GPIO_TEST_PIN1, pol_falling);    //falling edge

	/***step3.      set irq enable  ***/
	reg_irq_src = FLD_IRQ_GPIO_EN; //clean irq status
	reg_irq_mask |= FLD_IRQ_GPIO_EN;
	gpio_en_interrupt(GPIO_TEST_PIN1, 1);





//demo2: IRQ_GPIO_RISC0,  TEST_PIN2, falling edge triggers
	/***step1. set pin as gpio and enable input********/
	gpio_set_func(GPIO_TEST_PIN2, AS_GPIO);           //enable GPIO func
	gpio_set_input_en(GPIO_TEST_PIN2, 1);             //enable input
	gpio_set_output_en(GPIO_TEST_PIN2, 0);            //disable output

	/***step2.      set the polarity and open pullup ***/
	gpio_setup_up_down_resistor(GPIO_TEST_PIN2, PM_PIN_PULLUP_10K);  //open pull up resistor
	gpio_set_interrupt_pol(GPIO_TEST_PIN2, pol_falling);    //falling edge

	/***step3.      set irq enable  ***/
	reg_irq_src = FLD_IRQ_GPIO_RISC0_EN; //clean irq status
	reg_irq_mask |= FLD_IRQ_GPIO_RISC0_EN;
	gpio_en_interrupt_risc0(GPIO_TEST_PIN2, 1);




//demo3: IRQ_GPIO_RISC1,    TEST_PIN3, rising edge triggers
	/***step1. set pin as gpio and enable input********/
	gpio_set_func(GPIO_TEST_PIN3, AS_GPIO);           //enable GPIO func
	gpio_set_input_en(GPIO_TEST_PIN3, 1);             //enable input
	gpio_set_output_en(GPIO_TEST_PIN3, 0);            //disable output

	/***step2.      set the polarity and open pullup ***/
	gpio_setup_up_down_resistor(GPIO_TEST_PIN3, PM_PIN_PULLDOWN_100K);  //open pull down resistor
	gpio_set_interrupt_pol(GPIO_TEST_PIN3, pol_rising);    //rising edge

	/***step3.      set irq enable  ***/
	reg_irq_src = FLD_IRQ_GPIO_RISC1_EN; //clean irq status
	reg_irq_mask |= FLD_IRQ_GPIO_RISC1_EN;
	gpio_en_interrupt_risc1(GPIO_TEST_PIN3, 1);







	irq_enable();  //must

}

//init the LED pin,for indication
void app_led_init(void)
{

	gpio_set_func(GPIO_LED1_PIN, AS_GPIO);
	gpio_set_output_en(GPIO_LED1_PIN, 1);//enable output
	gpio_set_input_en(GPIO_LED1_PIN, 0); //disable input
	gpio_write(GPIO_LED1_PIN, LED_INIT_LEVEL);

	gpio_set_func(GPIO_LED2_PIN, AS_GPIO);
	gpio_set_output_en(GPIO_LED2_PIN, 1);//enable output
	gpio_set_input_en(GPIO_LED2_PIN, 0); //disable input
	gpio_write(GPIO_LED2_PIN, LED_INIT_LEVEL);

	gpio_set_func(GPIO_LED3_PIN, AS_GPIO);
	gpio_set_output_en(GPIO_LED3_PIN, 1);//enable output
	gpio_set_input_en(GPIO_LED3_PIN, 0); //disable input
	gpio_write(GPIO_LED3_PIN, LED_INIT_LEVEL);

}


int gpio_all_irq_cnt;
int gpio_irq_cnt;
int gpio_irq_risr0_cnt;
int gpio_irq_risr1_cnt;

_attribute_ram_code_ void app_gpio_irq_test_proc(void)
{
	gpio_all_irq_cnt ++;


	/************ gpio irq  ***************/
	if(reg_irq_src & FLD_IRQ_GPIO_EN){
		reg_irq_src = FLD_IRQ_GPIO_EN;        // clear irq_gpio irq flag

		gpio_irq_cnt ++;
		DBG_CHN0_TOGGLE;
		gpio_toggle(GPIO_LED1_PIN);
	}


	/************* gpio irq risc0 *************/
	if(reg_irq_src & FLD_IRQ_GPIO_RISC0_EN){
		reg_irq_src = FLD_IRQ_GPIO_RISC0_EN;        // clear irq_gpio irq flag


		gpio_irq_risr0_cnt ++;
		DBG_CHN1_TOGGLE;
		gpio_toggle(GPIO_LED2_PIN);
	}


	/************* gpio irq risc1 *************/
	if(reg_irq_src & FLD_IRQ_GPIO_RISC1_EN){
		reg_irq_src = FLD_IRQ_GPIO_RISC1_EN;        // clear irq_gpio irq flag

		gpio_irq_risr1_cnt ++;
		DBG_CHN2_TOGGLE;
		gpio_toggle(GPIO_LED3_PIN);
	}


}



#endif
