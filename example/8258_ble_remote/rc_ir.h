/********************************************************************************************************
 * @file	 rc_ir.h
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

#ifndef RC_IR_H_
#define RC_IR_H_




#define IR_CARRIER_FREQ					38000  	// 1 frame -> 1/38k -> 1000/38 = 26 us
#define PWM_CARRIER_CYCLE_TICK			( CLOCK_SYS_CLOCK_HZ/IR_CARRIER_FREQ )  //16M: 421 tick, f = 16000000/421 = 38004,T = 421/16=26.3125 us
#define PWM_CARRIER_HIGH_TICK			( PWM_CARRIER_CYCLE_TICK/3 )   // 1/3 duty

#define PWM_CARRIER_HALF_CYCLE_TICK		(PWM_CARRIER_CYCLE_TICK>>1)


#define IR_HIGH_CARR_TIME			565			// in us
#define IR_HIGH_NO_CARR_TIME		1685
#define IR_LOW_CARR_TIME			560
#define IR_LOW_NO_CARR_TIME			565
#define IR_INTRO_CARR_TIME			9000
#define IR_INTRO_NO_CARR_TIME		4500

#define IR_SWITCH_CODE              0x0d
#define IR_ADDR_CODE                0x00
#define IR_CMD_CODE                 0xbf

#define IR_REPEAT_INTERVAL_TIME     40500
#define IR_REPEAT_NO_CARR_TIME      2250
#define IR_END_TRANS_TIME			563

//#define IR_CARRIER_FREQ				37917//38222
#define IR_CARRIER_DUTY				3
#define IR_LEARN_SERIES_CNT     	160




enum{
	IR_SEND_TYPE_TIME_SERIES,
	IR_SEND_TYPE_BYTE,
	IR_SEND_TYPE_HALF_TIME_SERIES,
};



typedef struct{
	u32 cycle;
	u16 hich;
	u16 cnt;
}ir_ctrl_t;


typedef struct{
	ir_ctrl_t *ptr_irCtl;
	u8 type;
	u8 start_high;
	u8 ir_number;
	u8 code;
}ir_send_ctrl_data_t;


#define IR_GROUP_MAX		8

#define IR_SENDING_NONE  		0
#define IR_SENDING_DATA			1
#define IR_SENDING_REPEAT		2

typedef struct{
	u8 is_sending;
	u8 repeat_enable;
	u8 last_cmd;
	u8 rsvd;

	u32 sending_start_time;
}ir_send_ctrl_t;

ir_send_ctrl_t ir_send_ctrl;









void ir_config_carrier(u16 cycle_tick, u16 high_tick);
void ir_config_byte_timing(u32 logic_1_carr, u32 logic_1_none, u32 logic_0_carr, u32 logic_0_none);

void ir_send_add_series_item(u32 *time_series, u8 series_cnt, ir_ctrl_t *pIrCtrl, u8 start_high);
void ir_send_add_byte_item(u8 code, u8 start_high);


void rc_ir_init(void);
void ir_send_release(void);

void ir_irq_send(void);
void ir_repeat_handle(void);







void ir_nec_send(u8 addr1, u8 addr2, u8 cmd);




#endif /* RC_IR_H_ */
