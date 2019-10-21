/********************************************************************************************************
 * @file     app_pwm.c 
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



//normal mode
#define  TEST_PWM_NORMAL_MODE_1				1
#define  TEST_PWM_NORMAL_MODE_2				2
#define  TEST_PWM_NORMAL_MODE_3				3
#define  TEST_PWM_NORMAL_MODE_4				4

//fifo mode
#define  TEST_PWM0_FIFO_MODE				10
#define  TEST_PWM0_DMA_FIFO_MODE			11





#define  TEST_PWM_SELECT					TEST_PWM_NORMAL_MODE_1







#define IR_CARRIER_FREQ				38000  	// 1 frame -> 1/38k -> 1000/38 = 26 us
#define PWM_CARRIER_CYCLE_TICK		( CLOCK_SYS_CLOCK_HZ/IR_CARRIER_FREQ )  //16M: 421 tick, f = 16000000/421 = 38004,T = 421/16=26.3125 us
#define PWM_CARRIER_HIGH_TICK		( PWM_CARRIER_CYCLE_TICK/3 )   // 1/3 duty



#define PWM_IR_MAX_NUM    64     //user can define this max number
typedef struct{
    unsigned int dma_len;        // dma len
    unsigned short data[PWM_IR_MAX_NUM];
    unsigned int   data_num;
}pwm_dma_data_t;


pwm_dma_data_t T_dmaData_buf;

/*********************************************************************************
    PWM0   :  PA2.  PC1. PD5
    PWM1   :  PA3.  PC3.
    PWM2   :  PA4.  PC4.
    PWM3   :  PB0.  PD2.
    PWM4   :  PB1.  PB4.
    PWM5   :  PB2.  PB5.
    PWM0_N :  PA0.  PB3.  PC4	PD5
    PWM1_N :  PC1.  PD3.
    PWM2_N :  PD4.
    PWM3_N :  PC5.
    PWM4_N :  PC0.  PC6.
    PWM5_N :  PC7.
 *********************************************************************************/

void app_pwm_test(void)
{
	pwm_set_clk(CLOCK_SYS_CLOCK_HZ, CLOCK_SYS_CLOCK_HZ);


#if (TEST_PWM_SELECT == TEST_PWM_NORMAL_MODE_1)  //test PWMx (0~5)   normal mode
	//PA2 PWM0  1ms cycle  1/2 duty
	gpio_set_func(GPIO_PA2, AS_PWM0);
	pwm_set_mode(PWM0_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM0_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM0_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (500 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM0_ID);


	//PA3 PWM1  1ms cycle  1/3 duty
	gpio_set_func(GPIO_PA3, AS_PWM1);
	pwm_set_mode(PWM1_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM1_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM1_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (333 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM1_ID);


	//PA4 PWM2   1ms cycle  1/4 duty
	gpio_set_func(GPIO_PA4, AS_PWM2);
	pwm_set_mode(PWM2_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM2_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM2_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (250 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM2_ID);


	//PB0 PWM3  1ms cycle  1/5 duty
	gpio_set_func(GPIO_PB0, AS_PWM3);
	pwm_set_mode(PWM3_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM3_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM3_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (200 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM3_ID);

	//PB1 PWM4  1ms cycle  2/3 duty
	gpio_set_func(GPIO_PB1, AS_PWM4);
	pwm_set_mode(PWM4_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM4_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM4_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (667 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM4_ID);


	//PB2 PWM5  1ms cycle  3/4 duty
	gpio_set_func(GPIO_PB2, AS_PWM5);
	pwm_set_mode(PWM5_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM5_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM5_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (750 * CLOCK_SYS_CLOCK_1US) );
	pwm_start(PWM5_ID);

#elif (TEST_PWM_SELECT == TEST_PWM_NORMAL_MODE_2)  //test PWMx and PWMx_N(0~2)   normal mode

	//PC1 PWM0     1ms cycle  1/3 duty
	//PA0 PWM0_N   1ms cycle  2/3 duty
	gpio_set_func(GPIO_PC1, AS_PWM0);
	gpio_set_func(GPIO_PA0, AS_PWM0_N);
	pwm_set_mode(PWM0_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM0_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM0_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (333 * CLOCK_SYS_CLOCK_1US) );


	//PC3 PWM1     1ms cycle  1/4 duty
	//PD3 PWM1_N   1ms cycle  3/4 duty
	gpio_set_func(GPIO_PC3, AS_PWM1);
	gpio_set_func(GPIO_PD3, AS_PWM1_N);
	pwm_set_mode(PWM1_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM1_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM1_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (250 * CLOCK_SYS_CLOCK_1US) );


	//PC4 PWM2     1ms cycle  1/5 duty
	//PD4 PWM2_N   1ms cycle  4/5 duty
	gpio_set_func(GPIO_PC4, AS_PWM2);
	gpio_set_func(GPIO_PD4, AS_PWM2_N);
	pwm_set_mode(PWM2_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM2_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM2_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (200 * CLOCK_SYS_CLOCK_1US) );


	pwm_start(PWM0_ID);
	pwm_start(PWM1_ID);
	pwm_start(PWM2_ID);


#elif (TEST_PWM_SELECT == TEST_PWM_NORMAL_MODE_3)  //test PWMx and PWMx_N(3~5)   normal mode

	//PD2 PWM3     1ms cycle  1/3 duty
	//PC5 PWM3_N   1ms cycle  2/3 duty
	gpio_set_func(GPIO_PD2, AS_PWM3);
	gpio_set_func(GPIO_PC5, AS_PWM3_N);
	pwm_set_mode(PWM3_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM3_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM3_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (333 * CLOCK_SYS_CLOCK_1US) );


	//PB4 PWM4     1ms cycle  1/4 duty
	//PC0 PWM4_N   1ms cycle  3/4 duty
	gpio_set_func(GPIO_PB4, AS_PWM4);
	gpio_set_func(GPIO_PC0, AS_PWM4_N);
	pwm_set_mode(PWM4_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM4_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM4_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (250 * CLOCK_SYS_CLOCK_1US) );


	//PB5 PWM5     1ms cycle  1/5 duty
	//PC7 PWM5_N   1ms cycle  4/5 duty
	gpio_set_func(GPIO_PB5, AS_PWM5);
	gpio_set_func(GPIO_PC7, AS_PWM5_N);
	pwm_set_mode(PWM5_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM5_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM5_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (200 * CLOCK_SYS_CLOCK_1US) );


	pwm_start(PWM3_ID);
	pwm_start(PWM4_ID);
	pwm_start(PWM5_ID);

#elif (TEST_PWM_SELECT == TEST_PWM_NORMAL_MODE_4)  //test rest PWM gpio

	#if 1
		//PB3 PWM0_N   1ms cycle  2/3 duty
		gpio_set_func(GPIO_PB3, AS_PWM0_N);
	#elif 0
		//PD5 PWM0     1ms cycle  1/3 duty
		//PC4 PWM0_N   1ms cycle  2/3 duty
		gpio_set_func(GPIO_PD5, AS_PWM0);
		gpio_set_func(GPIO_PC4, AS_PWM0_N);
	#else
		//PD5 PWM0_N   1ms cycle  2/3 duty
		gpio_set_func(GPIO_PD5, AS_PWM0_N);
	#endif

	pwm_set_mode(PWM0_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM0_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM0_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (333 * CLOCK_SYS_CLOCK_1US) );



	//PC1 PWM1_N   1ms cycle  3/4 duty
	gpio_set_func(GPIO_PC1, AS_PWM1_N);
	pwm_set_mode(PWM1_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM1_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM1_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (250 * CLOCK_SYS_CLOCK_1US) );


	//PC6 PWM4_N   1ms cycle  4/5 duty
	gpio_set_func(GPIO_PC6, AS_PWM4_N);
	pwm_set_mode(PWM4_ID, PWM_NORMAL_MODE);
	pwm_set_phase(PWM4_ID, 0);   //no phase at pwm beginning
	pwm_set_cycle_and_duty(PWM4_ID, (u16) (1000 * CLOCK_SYS_CLOCK_1US),  (u16) (200 * CLOCK_SYS_CLOCK_1US) );



	pwm_start(PWM0_ID);
	pwm_start(PWM1_ID);
	pwm_start(PWM4_ID);


#elif (TEST_PWM_SELECT == TEST_PWM0_DMA_FIFO_MODE)
	sleep_us(1000000);



	//only pwm0 support fifo mode
	gpio_set_func(GPIO_PA2, AS_PWM0);
	pwm_set_mode(PWM0_ID, PWM_IR_DMA_FIFO_MODE);
	pwm_set_phase(PWM0_ID, 0);   //no phase at pwm beginning

	//config TMAX0  & TCMP0: 38k, 1/3 duty
	pwm_set_cycle_and_duty(PWM0_ID, PWM_CARRIER_CYCLE_TICK,  PWM_CARRIER_HIGH_TICK );



	//config waveforms
	T_dmaData_buf.data_num = 0;

//preamble:  9 ms carrier,  4.5 ms low
	T_dmaData_buf.data[T_dmaData_buf.data_num ++] = pwm_config_dma_fifo_waveform(1, PWM0_PULSE_NORMAL, 9000 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);
	T_dmaData_buf.data[T_dmaData_buf.data_num ++] = pwm_config_dma_fifo_waveform(0, PWM0_PULSE_NORMAL, 4500 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);

//data 1 :  560 us carrier,  560 us low
	T_dmaData_buf.data[T_dmaData_buf.data_num ++] = pwm_config_dma_fifo_waveform(1, PWM0_PULSE_NORMAL, 560 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);
	T_dmaData_buf.data[T_dmaData_buf.data_num ++] = pwm_config_dma_fifo_waveform(0, PWM0_PULSE_NORMAL, 560 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);


//data  0 :  560 us carrier,  1690 us low
	T_dmaData_buf.data[T_dmaData_buf.data_num ++] = pwm_config_dma_fifo_waveform(1, PWM0_PULSE_NORMAL, 560 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);
	T_dmaData_buf.data[T_dmaData_buf.data_num ++]= pwm_config_dma_fifo_waveform(0, PWM0_PULSE_NORMAL, 1690 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);


//end:  560 us carrier
	T_dmaData_buf.data[T_dmaData_buf.data_num ++] = pwm_config_dma_fifo_waveform(1, PWM0_PULSE_NORMAL, 560 * CLOCK_SYS_CLOCK_1US/PWM_CARRIER_CYCLE_TICK);

	//calculate  dma len
	T_dmaData_buf.dma_len = T_dmaData_buf.data_num * 2;




	pwm_set_dma_address(&T_dmaData_buf);



//add pwm0 dma fifo done irq, when all waveform send over, this irq will triggers
	//enable mcu global irq
	 irq_enable();

	//enable system irq PWM
	reg_irq_mask |= FLD_IRQ_SW_PWM_EN;

	//enable pwm0 ir dma fifo done irq
	reg_pwm_irq_sta = FLD_IRQ_PWM0_IR_DMA_FIFO_DONE; //clear irq status
	reg_pwm_irq_mask |= FLD_IRQ_PWM0_IR_DMA_FIFO_DONE;


//PWM0 ir dma fifo mode begin
	pwm_start_dma_ir_sending();

	DBG_CHN0_HIGH;  //debug


#else


#endif

}




_attribute_ram_code_ void app_pwm_irq_test_proc(void)
{

	if(reg_pwm_irq_sta & FLD_IRQ_PWM0_IR_DMA_FIFO_DONE){
		reg_pwm_irq_sta = FLD_IRQ_PWM0_IR_DMA_FIFO_DONE;
		DBG_CHN0_LOW;  //finish
	}
}






