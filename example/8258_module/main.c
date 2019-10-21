/********************************************************************************************************
 * @file     main.c 
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "vendor/common/blt_fw_sign.h"


extern my_fifo_t spp_rx_fifo;
extern void user_init_normal();
extern void user_init_deepRetn();
extern void main_loop (void);

_attribute_ram_code_ void irq_handler(void)
{
	irq_blt_sdk_handler ();

#if (HCI_ACCESS==HCI_USE_UART)
	unsigned char irqS = dma_chn_irq_status_get();
    if(irqS & FLD_DMA_CHN_UART_RX)	//rx
    {
    	dma_chn_irq_status_clr(FLD_DMA_CHN_UART_RX);

    	u8* w = spp_rx_fifo.p + (spp_rx_fifo.wptr & (spp_rx_fifo.num-1)) * spp_rx_fifo.size;
    	if(w[0]!=0)
    	{
    		my_fifo_next(&spp_rx_fifo);
    		u8* p = spp_rx_fifo.p + (spp_rx_fifo.wptr & (spp_rx_fifo.num-1)) * spp_rx_fifo.size;
    		reg_dma_uart_rx_addr = (u16)((u32)p);  //switch uart RX dma address
    	}
    }

    if(irqS & FLD_DMA_CHN_UART_TX)	//tx
    {
    	dma_chn_irq_status_clr(FLD_DMA_CHN_UART_TX);
    }
#endif
}

_attribute_ram_code_ int main (void)    //must run in ramcode
{
	DBG_CHN0_LOW;   //debug

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


	if( deepRetWakeUp ){
		user_init_deepRetn ();
	}
	else{
		#if FIRMWARES_SIGNATURE_ENABLE
			blt_firmware_signature_check();
		#endif
		user_init_normal ();
	}


    irq_enable();




	while (1) {
#if (MODULE_WATCHDOG_ENABLE)
		wd_clear(); //clear watch dog
#endif
		main_loop ();
	}
}
