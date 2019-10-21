/********************************************************************************************************
 * @file     app_config.h 
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     Sep. 18, 2015
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
#pragma once

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif




/////////////////// MODULE /////////////////////////////////
#define BLE_MODULE_PM_ENABLE				1
#define PM_DEEPSLEEP_RETENTION_ENABLE		1

#define BLE_OTA_ENABLE						1
#define TELIK_SPP_SERVICE_ENABLE			1
#define BLE_MODULE_INDICATE_DATA_TO_MCU		1
#define BATT_CHECK_ENABLE       			1   //enable or disable battery voltage detection
#define BLT_APP_LED_ENABLE					1

//firmware check
#define FIRMWARES_SIGNATURE_ENABLE          0


//////////////// SMP SETTING  //////////////////////////////
#define BLE_SECURITY_ENABLE 			   	1



/////////////////// DEEP SAVE FLG //////////////////////////////////
#define USED_DEEP_ANA_REG                   DEEP_ANA_REG0 //u8,can save 8 bit info when deep
#define	LOW_BATT_FLG					    BIT(0)



#if (BATT_CHECK_ENABLE)
//telink device: you must choose one gpio with adc function to output high level(voltage will equal to vbat), then use adc to measure high level voltage
	//use PB7 output high level, then adc measure this high level voltage
	#define GPIO_VBAT_DETECT				GPIO_PB7
	#define PB7_FUNC						AS_GPIO
	#define PB7_INPUT_ENABLE				0
	#define ADC_INPUT_PCHN					B7P    //corresponding  ADC_InputPchTypeDef in adc.h
#endif

//////////////////// LED CONFIG (EVK board) ///////////////////////////
#if (BLT_APP_LED_ENABLE)
	#define LED_ON_LEVAL 					1 			//gpio output high voltage to turn on led
	#define	GPIO_LED						GPIO_PD5    //red
	#define PD5_FUNC						AS_GPIO
#endif





//////////////////////////// MODULE PM GPIO	(EVK board) /////////////////////////////////
#define GPIO_WAKEUP_MODULE					GPIO_PA2   //mcu wakeup module
#define	PA2_FUNC							AS_GPIO
#define PA2_INPUT_ENABLE					1
#define	PA2_OUTPUT_ENABLE					0
#define	PA2_DATA_OUT						0
#define GPIO_WAKEUP_MODULE_HIGH				gpio_setup_up_down_resistor(GPIO_WAKEUP_MODULE, PM_PIN_PULLUP_10K);
#define GPIO_WAKEUP_MODULE_LOW				gpio_setup_up_down_resistor(GPIO_WAKEUP_MODULE, PM_PIN_PULLDOWN_100K);

#define GPIO_WAKEUP_MCU						GPIO_PD0   //module wakeup mcu
#define	PD0_FUNC							AS_GPIO
#define PD0_INPUT_ENABLE					1
#define	PD0_OUTPUT_ENABLE					1
#define	PD0_DATA_OUT						0
#define GPIO_WAKEUP_MCU_HIGH				do{gpio_set_output_en(GPIO_WAKEUP_MCU, 1); gpio_write(GPIO_WAKEUP_MCU, 1);}while(0)
#define GPIO_WAKEUP_MCU_LOW					do{gpio_set_output_en(GPIO_WAKEUP_MCU, 1); gpio_write(GPIO_WAKEUP_MCU, 0);}while(0)
#define GPIO_WAKEUP_MCU_FLOAT				do{gpio_set_output_en(GPIO_WAKEUP_MCU, 0); gpio_write(GPIO_WAKEUP_MCU, 0);}while(0)






/////////////////// Clock  /////////////////////////////////
#define CLOCK_SYS_CLOCK_HZ  				16000000

enum{
	CLOCK_SYS_CLOCK_1S = CLOCK_SYS_CLOCK_HZ,
	CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
	CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};



/////////////////// watchdog  //////////////////////////////
#define MODULE_WATCHDOG_ENABLE				0
#define WATCHDOG_INIT_TIMEOUT				500  //ms





/////////////////////HCI UART variables///////////////////////////////////////
#define UART_DATA_LEN    					68   // data max 252
typedef struct{
    unsigned int len; // data max 252
    unsigned char data[UART_DATA_LEN];
}uart_data_t;







///////////////////////////////////// ATT  HANDLER define ///////////////////////////////////////
typedef enum
{
	ATT_H_START = 0,


	//// Gap ////
	/**********************************************************************************************/
	GenericAccess_PS_H, 					//UUID: 2800, 	VALUE: uuid 1800
	GenericAccess_DeviceName_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read | Notify
	GenericAccess_DeviceName_DP_H,			//UUID: 2A00,   VALUE: device name
	GenericAccess_Appearance_CD_H,			//UUID: 2803, 	VALUE:  			Prop: Read
	GenericAccess_Appearance_DP_H,			//UUID: 2A01,	VALUE: appearance
	CONN_PARAM_CD_H,						//UUID: 2803, 	VALUE:  			Prop: Read
	CONN_PARAM_DP_H,						//UUID: 2A04,   VALUE: connParameter


	//// gatt ////
	/**********************************************************************************************/
	GenericAttribute_PS_H,					//UUID: 2800, 	VALUE: uuid 1801
	GenericAttribute_ServiceChanged_CD_H,	//UUID: 2803, 	VALUE:  			Prop: Indicate
	GenericAttribute_ServiceChanged_DP_H,   //UUID:	2A05,	VALUE: service change
	GenericAttribute_ServiceChanged_CCB_H,	//UUID: 2902,	VALUE: serviceChangeCCC


	//// device information ////
	/**********************************************************************************************/
	DeviceInformation_PS_H,					 //UUID: 2800, 	VALUE: uuid 180A
	DeviceInformation_pnpID_CD_H,			 //UUID: 2803, 	VALUE:  			Prop: Read
	DeviceInformation_pnpID_DP_H,			 //UUID: 2A50,	VALUE: PnPtrs

	//// SPP ////
	/**********************************************************************************************/
	SPP_PS_H, 							 //UUID: 2800, 	VALUE: telink spp service uuid

	//server to client
	SPP_SERVER_TO_CLIENT_CD_H,		     //UUID: 2803, 	VALUE:  			Prop: read | Notify
	SPP_SERVER_TO_CLIENT_DP_H,			 //UUID: telink spp s2c uuid,  VALUE: SppDataServer2ClientData
	SPP_SERVER_TO_CLIENT_CCB_H,			 //UUID: 2902, 	VALUE: SppDataServer2ClientDataCCC
	SPP_SERVER_TO_CLIENT_DESC_H,		 //UUID: 2901, 	VALUE: TelinkSPPS2CDescriptor

	//client to server
	SPP_CLIENT_TO_SERVER_CD_H,		     //UUID: 2803, 	VALUE:  			Prop: read | write_without_rsp
	SPP_CLIENT_TO_SERVER_DP_H,			 //UUID: telink spp c2s uuid,  VALUE: SppDataClient2ServerData
	SPP_CLIENT_TO_SERVER_DESC_H,		 //UUID: 2901, 	VALUE: TelinkSPPC2SDescriptor

	//// Ota ////
	/**********************************************************************************************/
	OTA_PS_H, 							 //UUID: 2800, 	VALUE: telink ota service uuid
	OTA_CMD_OUT_CD_H,					 //UUID: 2803, 	VALUE:  			Prop: read | write_without_rsp
	OTA_CMD_OUT_DP_H,					 //UUID: telink ota uuid,  VALUE: otaData
	OTA_CMD_OUT_DESC_H,					 //UUID: 2901, 	VALUE: otaName

	ATT_END_H,

}ATT_HANDLE;




#define DEBUG_GPIO_ENABLE						0

#if(DEBUG_GPIO_ENABLE)
	//define debug GPIO here according to your hardware

	#define GPIO_CHN0							GPIO_PB4
	#define GPIO_CHN1							GPIO_PB5
	#define GPIO_CHN2							GPIO_PB6


	#define PB4_OUTPUT_ENABLE					1
	#define PB5_OUTPUT_ENABLE					1
	#define PB6_OUTPUT_ENABLE					1




	#define DBG_CHN0_LOW		gpio_write(GPIO_CHN0, 0)
	#define DBG_CHN0_HIGH		gpio_write(GPIO_CHN0, 1)
	#define DBG_CHN0_TOGGLE		gpio_toggle(GPIO_CHN0)
	#define DBG_CHN1_LOW		gpio_write(GPIO_CHN1, 0)
	#define DBG_CHN1_HIGH		gpio_write(GPIO_CHN1, 1)
	#define DBG_CHN1_TOGGLE		gpio_toggle(GPIO_CHN1)
	#define DBG_CHN2_LOW		gpio_write(GPIO_CHN2, 0)
	#define DBG_CHN2_HIGH		gpio_write(GPIO_CHN2, 1)
	#define DBG_CHN2_TOGGLE		gpio_toggle(GPIO_CHN2)
#else
	#define DBG_CHN0_LOW
	#define DBG_CHN0_HIGH
	#define DBG_CHN0_TOGGLE
	#define DBG_CHN1_LOW
	#define DBG_CHN1_HIGH
	#define DBG_CHN1_TOGGLE
	#define DBG_CHN2_LOW
	#define DBG_CHN2_HIGH
	#define DBG_CHN2_TOGGLE
	#define DBG_CHN3_LOW
	#define DBG_CHN3_HIGH
	#define DBG_CHN3_TOGGLE
	#define DBG_CHN4_LOW
	#define DBG_CHN4_HIGH
	#define DBG_CHN4_TOGGLE
	#define DBG_CHN5_LOW
	#define DBG_CHN5_HIGH
	#define DBG_CHN5_TOGGLE
	#define DBG_CHN6_LOW
	#define DBG_CHN6_HIGH
	#define DBG_CHN6_TOGGLE
	#define DBG_CHN7_LOW
	#define DBG_CHN7_HIGH
	#define DBG_CHN7_TOGGLE
#endif  //end of DEBUG_GPIO_ENABLE


/////////////////// set default   ////////////////

#include "vendor/common/default_config.h"

/* Disable C linkage for C++ Compilers: */
#if defined(__cplusplus)
}
#endif

