/********************************************************************************************************
 * @file     app_config.h
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

#pragma once

/* Enable C linkage for C++ Compilers: */
#if defined(__cplusplus)
extern "C" {
#endif


//////////// product  Information  //////////////////////////////
#define ID_VENDOR				0x248a			// for report
#define ID_PRODUCT_BASE			0x880C
#define STRING_VENDOR			L"Telink"
#define STRING_PRODUCT			L"BLE Remote KMA Dongle"
#define STRING_SERIAL			L"TLSR8258"

#define	FLOW_NO_OS				1

#define CHIP_TYPE				CHIP_TYPE_8258


//firmware check
#define FIRMWARES_SIGNATURE_ENABLE          0


#define FLASH_ADR_PARING   							0x78000



/////////////////// MODULE /////////////////////////////////
#define BLE_HOST_SMP_ENABLE							0  //1 for standard security management,  0 for telink referenced paring&bonding(no security)
#define BLE_HOST_SIMPLE_SDP_ENABLE					1  //simple service discovery


#define BLE_MASTER_OTA_ENABLE						1  //slave ota test
#define AUDIO_SDM_ENBALE							0  //if using sdm playback, should better disable USB MIC

#define UI_AUDIO_ENABLE								1
#define UI_BUTTON_ENABLE							1
#define UI_LED_ENABLE								1
#define UI_UPPER_COMPUTER_ENABLE					0  //work with upper computer




#define APPLICATION_DONGLE							1
#if(APPLICATION_DONGLE)
	#define PA5_FUNC				AS_USB
	#define PA6_FUNC				AS_USB
	#define PA5_INPUT_ENABLE		1
	#define PA6_INPUT_ENABLE		1

	#define	USB_PRINTER_ENABLE 		1
	#define	USB_SPEAKER_ENABLE 		0
	#define	USB_MIC_ENABLE 			1
	#define	USB_MOUSE_ENABLE 		1
	#define	USB_KEYBOARD_ENABLE 	1
	#define	USB_SOMATIC_ENABLE      0   //  when USB_SOMATIC_ENABLE, USB_EDP_PRINTER_OUT disable
	#define USB_CUSTOM_HID_REPORT	1
#endif

//////////////////// Audio /////////////////////////////////////
#define MIC_RESOLUTION_BIT		16
#define MIC_SAMPLE_RATE			16000
#define MIC_CHANNLE_COUNT		1
#define	MIC_ENOCDER_ENABLE		0



////////////////////////// MIC BUFFER /////////////////////////////
#define BLE_DMIC_ENABLE					0  //0: Amic   1: Dmic

#if (UI_AUDIO_ENABLE)
	#define	MIC_ADPCM_FRAME_SIZE		128 //128
	#define	MIC_SHORT_DEC_SIZE			248 //248
#endif


#if (AUDIO_SDM_ENBALE)
	#define TL_SDM_BUFFER_SIZE			1024
#endif


//----------------------- GPIO for UI --------------------------------
#if (UI_BUTTON_ENABLE)
	//---------------  Button ----------------------------------
	#define	SW1_GPIO				GPIO_PD5
	#define	SW2_GPIO				GPIO_PD6
	#define PD5_FUNC				AS_GPIO
	#define PD6_FUNC				AS_GPIO
	#define PD5_INPUT_ENABLE		1
	#define PD6_INPUT_ENABLE		1
	#define PULL_WAKEUP_SRC_PD5     PM_PIN_PULLUP_10K
	#define PULL_WAKEUP_SRC_PD6     PM_PIN_PULLUP_10K
#endif



#if (UI_LED_ENABLE)
	#define	GPIO_LED_RED			GPIO_PA3
	#define	GPIO_LED_WHITE			GPIO_PB1
	#define	GPIO_LED_GREEN			GPIO_PA2
	#define	GPIO_LED_BLUE			GPIO_PB0
    #define	GPIO_LED_YELLOW			GPIO_PA4

	#define PA3_FUNC				AS_GPIO
	#define PB1_FUNC				AS_GPIO
	#define PA2_FUNC				AS_GPIO
	#define PB0_FUNC				AS_GPIO
	#define PA4_FUNC				AS_GPIO

	#define	PA3_OUTPUT_ENABLE		1
	#define	PB1_OUTPUT_ENABLE		1
	#define PA2_OUTPUT_ENABLE		1
	#define	PB0_OUTPUT_ENABLE		1
	#define	PA4_OUTPUT_ENABLE		1

	#define LED_ON_LEVAL 			1 		//gpio output high voltage to turn on led
#endif







/////////////////// Clock  /////////////////////////////////
#define CLOCK_SYS_CLOCK_HZ  	32000000

enum{
	CLOCK_SYS_CLOCK_1S = CLOCK_SYS_CLOCK_HZ,
	CLOCK_SYS_CLOCK_1MS = (CLOCK_SYS_CLOCK_1S / 1000),
	CLOCK_SYS_CLOCK_1US = (CLOCK_SYS_CLOCK_1S / 1000000),
};


/////////////////// watchdog  //////////////////////////////
#define MODULE_WATCHDOG_ENABLE		0
#define WATCHDOG_INIT_TIMEOUT		500  //ms









#define DEBUG_GPIO_ENABLE							0

#if(DEBUG_GPIO_ENABLE)
//define debug GPIO here according to your hardware
	#define GPIO_CHN0							GPIO_PB2
	#define GPIO_CHN1							GPIO_PB3
	#define GPIO_CHN2							GPIO_PB4
	#define GPIO_CHN3							GPIO_PB5
	#define GPIO_CHN4							GPIO_PC2
	#define GPIO_CHN5							GPIO_PC3

	#define PB2_OUTPUT_ENABLE					1
	#define PB3_OUTPUT_ENABLE					1
	#define PB4_OUTPUT_ENABLE					1
	#define PB5_OUTPUT_ENABLE					1
	#define PC2_OUTPUT_ENABLE					1
	#define PC3_OUTPUT_ENABLE					1


	#define DBG_CHN0_LOW		gpio_write(GPIO_CHN0, 0)
	#define DBG_CHN0_HIGH		gpio_write(GPIO_CHN0, 1)
	#define DBG_CHN0_TOGGLE		gpio_toggle(GPIO_CHN0)
	#define DBG_CHN1_LOW		gpio_write(GPIO_CHN1, 0)
	#define DBG_CHN1_HIGH		gpio_write(GPIO_CHN1, 1)
	#define DBG_CHN1_TOGGLE		gpio_toggle(GPIO_CHN1)
	#define DBG_CHN2_LOW		gpio_write(GPIO_CHN2, 0)
	#define DBG_CHN2_HIGH		gpio_write(GPIO_CHN2, 1)
	#define DBG_CHN2_TOGGLE		gpio_toggle(GPIO_CHN2)
	#define DBG_CHN3_LOW		gpio_write(GPIO_CHN3, 0)
	#define DBG_CHN3_HIGH		gpio_write(GPIO_CHN3, 1)
	#define DBG_CHN3_TOGGLE		gpio_toggle(GPIO_CHN3)
	#define DBG_CHN4_LOW		gpio_write(GPIO_CHN4, 0)
	#define DBG_CHN4_HIGH		gpio_write(GPIO_CHN4, 1)
	#define DBG_CHN4_TOGGLE		gpio_toggle(GPIO_CHN4)
	#define DBG_CHN5_LOW		gpio_write(GPIO_CHN5, 0)
	#define DBG_CHN5_HIGH		gpio_write(GPIO_CHN5, 1)
	#define DBG_CHN5_TOGGLE		gpio_toggle(GPIO_CHN5)

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
