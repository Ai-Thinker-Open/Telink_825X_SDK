/********************************************************************************************************
 * @file     blm_host.h
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

#ifndef APP_HOST_H_
#define APP_HOST_H_



#define CHAR_HANDLE_MAX					10

// connection character device information
typedef struct
{
	u16 conn_handle;
	u8 conn_state;
	u8 mac_adrType;
	u8 mac_addr[6];
	u8 char_handle[CHAR_HANDLE_MAX];
}dev_char_info_t;

extern dev_char_info_t cur_conn_device;


typedef void (*main_service_t) (void);

extern main_service_t		main_service;


int controller_event_callback (u32 h, u8 *p, int n);
int app_l2cap_handler (u16 conn_handle, u8 *raw_pkt);
int app_host_smp_finish (void);

void host_update_conn_proc(void);

extern u32 host_update_conn_param_req;
extern int	app_host_smp_sdp_pending;


#endif /* APP_HOST_H_ */
