/********************************************************************************************************
 * @file     feature_phytest.c 
 *
 * @brief    for TLSR chips
 *
 * @author	 public@telink-semi.com;
 * @date     May. 10, 2018
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
#include "app.h"
#include <stack/ble/ble.h>
#include "tl_common.h"
#include "drivers.h"
#include "app_config.h"
#include "vendor/common/blt_led.h"
#include "application/keyboard/keyboard.h"
#include "vendor/common/tl_audio.h"
#include "vendor/common/blt_soft_timer.h"
#include "vendor/common/blt_common.h"



#if (FEATURE_TEST_MODE == TEST_BLE_PHY)



#define FEATURE_PM_ENABLE								0
#define FEATURE_DEEPSLEEP_RETENTION_ENABLE				0



#if(FEATURE_TEST_MODE == TEST_BLE_PHY)
	MYFIFO_INIT(hci_rx_fifo, 72, 2);
	MYFIFO_INIT(hci_tx_fifo, 72, 8);
#endif




#define RX_FIFO_SIZE	64
#define RX_FIFO_NUM		8

#define TX_FIFO_SIZE	40
#define TX_FIFO_NUM		16




_attribute_data_retention_  u8 		 	blt_rxfifo_b[RX_FIFO_SIZE * RX_FIFO_NUM] = {0};
_attribute_data_retention_	my_fifo_t	blt_rxfifo = {
												RX_FIFO_SIZE,
												RX_FIFO_NUM,
												0,
												0,
												blt_rxfifo_b,};


_attribute_data_retention_  u8 		 	blt_txfifo_b[TX_FIFO_SIZE * TX_FIFO_NUM] = {0};
_attribute_data_retention_	my_fifo_t	blt_txfifo = {
												TX_FIFO_SIZE,
												TX_FIFO_NUM,
												0,
												0,
												blt_txfifo_b,};







int AA_dbg_suspend;
void  func_suspend_enter (u8 e, u8 *p, int n)
{
	AA_dbg_suspend ++;
}

#define		MY_RF_POWER_INDEX					RF_POWER_P3p01dBm

_attribute_ram_code_ void  func_suspend_exit (u8 e, u8 *p, int n)
{
	rf_set_power_level_index (MY_RF_POWER_INDEX);
}





#if (BLE_PHYTEST_MODE == PHYTEST_MODE_OVER_HCI_WITH_UART)
	int rx_from_uart_cb (void)
	{
		if(my_fifo_get(&hci_rx_fifo) == 0)
		{
			return 0;
		}

		u8* p = my_fifo_get(&hci_rx_fifo);
		u32 rx_len = p[0]; //usually <= 255 so 1 byte should be sufficient

		if (rx_len)
		{
			blc_hci_handler(&p[4], rx_len - 4);
			my_fifo_pop(&hci_rx_fifo);
		}

		return 0;
	}

	_attribute_data_retention_ static u32 uart_tx_tick = 0;
	int tx_to_uart_cb (void)
	{
		uart_data_t T_txdata_buf;

		u8 *p = my_fifo_get (&hci_tx_fifo);


	#if (ADD_DELAY_FOR_UART_DATA)
		if (p && !uart_tx_is_busy () && clock_time_exceed(uart_tx_tick, 30000))
	#else
		if (p && !uart_tx_is_busy ())
	#endif
		{
			memcpy(&T_txdata_buf.data, p + 2, p[0]+p[1]*256);
			T_txdata_buf.len = p[0]+p[1]*256 ;
			uart_dma_send((unsigned char*)&T_txdata_buf);

			my_fifo_pop (&hci_tx_fifo);
			uart_tx_tick = clock_time();
		}
		return 0;
	}
#endif









void feature_phytest_init_normal(void)
{

	//random number generator must be initiated here( in the beginning of user_init_nromal)
	//when deepSleep retention wakeUp, no need initialize again
	random_generator_init();  //this is must


	u8  mac_public[6];
	u8  mac_random_static[6];
	blc_initMacAddress(CFG_ADR_MAC, mac_public, mac_random_static);

	rf_set_power_level_index (MY_RF_POWER_INDEX);

	////// Controller Initialization  //////////
	blc_ll_initBasicMCU();   //mandatory
	blc_ll_initStandby_module(mac_public);				//mandatory







	write_reg8(0x402, 0x2b);   //set rf packet preamble for BQB
	blc_phy_initPhyTest_module();
	blc_phy_setPhyTestEnable( BLC_PHYTEST_ENABLE );
	blc_phy_preamble_length_set(11);


	#if(BLE_PHYTEST_MODE == PHYTEST_MODE_THROUGH_2_WIRE_UART || BLE_PHYTEST_MODE == PHYTEST_MODE_OVER_HCI_WITH_UART)  //uart
		uart_gpio_set(UART_TX_PB1, UART_RX_PB0);
		uart_reset();
	#endif


	uart_recbuff_init((unsigned short*)hci_rx_fifo_b, hci_rx_fifo.size);

	#if (CLOCK_SYS_CLOCK_HZ == 16000000)
		uart_init(9,13,PARITY_NONE, STOP_BIT_ONE); //baud rate: 115200
	#elif (CLOCK_SYS_CLOCK_HZ == 24000000)
		uart_init(12,15,PARITY_NONE, STOP_BIT_ONE); //baud rate: 115200
	#endif

	uart_dma_enable(1,1);

	irq_set_mask(FLD_IRQ_DMA_EN);
	dma_chn_irq_enable(FLD_DMA_CHN_UART_RX | FLD_DMA_CHN_UART_TX, 1);   	//uart Rx/Tx dma irq enable
	uart_irq_enable(1,0);

	#if	(BLE_PHYTEST_MODE == PHYTEST_MODE_THROUGH_2_WIRE_UART)
		blc_register_hci_handler (phy_test_2_wire_rx_from_uart, phy_test_2_wire_tx_to_uart);
	#elif(BLE_PHYTEST_MODE == PHYTEST_MODE_OVER_HCI_WITH_UART)
		blc_register_hci_handler (rx_from_uart_cb, tx_to_uart_cb);		//default handler
	#endif




	//phy test can not enter suspend/deep
	bls_pm_setSuspendMask (SUSPEND_DISABLE);




}



#if (BLE_PHYTEST_MODE == PHYTEST_MODE_THROUGH_2_WIRE_UART )
	unsigned char uart_no_dma_rec_data[6] = {0x02,0, 0,0,0,0};
#elif(BLE_PHYTEST_MODE == PHYTEST_MODE_OVER_HCI_WITH_UART)
	unsigned char uart_no_dma_rec_data[72] = {0};
#endif


void app_phytest_irq_proc(void)
{
	unsigned char uart_dma_irqsrc;
	//1. UART irq
	uart_dma_irqsrc = dma_chn_irq_status_get();///in function,interrupt flag have already been cleared,so need not to clear DMA interrupt flag here
	if(uart_dma_irqsrc & FLD_DMA_CHN_UART_RX)
	{
		dma_chn_irq_status_clr(FLD_DMA_CHN_UART_RX);
		u8* w = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
		if(w[0]!=0)
		{
			my_fifo_next(&hci_rx_fifo);
			u8* p = hci_rx_fifo.p + (hci_rx_fifo.wptr & (hci_rx_fifo.num-1)) * hci_rx_fifo.size;
			reg_dma0_addr = (u16)((u32)p);
		}
	}
	if(uart_dma_irqsrc & FLD_DMA_CHN_UART_TX){
		dma_chn_irq_status_clr(FLD_DMA_CHN_UART_TX);
	}

}











#endif // end of  (FEATURE_TEST_MODE == TEST_BLE_PHY)

