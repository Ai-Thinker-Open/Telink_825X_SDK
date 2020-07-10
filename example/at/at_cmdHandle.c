#include "tl_common.h"
#include "at_cmd.h"
#include "drivers.h"

#include "stack/ble/ble.h"

#include "tinyFlash/tinyFlash.h"
#include "tinyFlash_Index.h"

//外部变量
extern u8 baud_buf[];
extern  const u8 tbl_scanRsp[];
extern u8 my_scanRsp[32];
extern u8 ATE;
extern u8  mac_public[6];

int str2hex(char * pbuf, int len)
{
	int i = 0;
	for(i = 0; i < len; i ++)
	{
		if(((pbuf[i] >= '0') && (pbuf[i] <= '9')) || ((pbuf[i] >= 'A') && (pbuf[i] <= 'F')))
		{
			if((pbuf[i] >= '0') && (pbuf[i] <= '9'))
			{
				pbuf[i] -= '0';
			}
			else
			{
				pbuf[i] -= 'A';
				pbuf[i] += 0x0A;
			}

			if(i%2)
			{
				pbuf[i/2] = (pbuf[i-1] << 4) | pbuf[i];
			}
		}
		else
		{
			return -1;
		}
	}

	return 0;
}

/* 经过data_process_parse函数分析执行下列函数 */

//关回显 回显即 将指令重复并输出结果
static unsigned char atCmd_ATE0(char *pbuf,  int mode, int lenth)
{
	ATE = 0;
	tinyFlash_Write(STORAGE_ATE, &ATE, 1);
	return 0;
}
//开回显 回显即 将指令重复并输出结果
static unsigned char atCmd_ATE1(char *pbuf,  int mode, int lenth)
{
	ATE = 1;
	tinyFlash_Write(STORAGE_ATE, &ATE, 1);
	return 0;
}
//获取AT版本
static unsigned char atCmd_GMR(char *pbuf,  int mode, int lenth)
{
	
	at_print("\r\n+VER:"AT_VERSION);
	return 0;
}
//重启
static unsigned char atCmd_Reset(char *pbuf,  int mode, int lenth)
{
	at_print("\r\nOK\r\n");
	start_reboot();
	return 0;
}
//睡眠
static unsigned char atCmd_Sleep(char *pbuf,  int mode, int lenth)
{
	at_print("\r\nOK\r\n");

	gpio_setup_up_down_resistor(UART_RX_PIN, PM_PIN_PULLDOWN_100K);
	cpu_set_gpio_wakeup (UART_RX_PIN, Level_Low, 1); 

	cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  //deepsleep
	return 0;
}

//轻度睡眠，保持蓝牙及连接功能
extern u8 lsleep_model;
static unsigned char  atCmd_LSleep(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		if(lsleep_model == 1)
		{
			at_print("\r\n+LSLEEP:1");
		}
		else
		{
			at_print("\r\n+LSLEEP:0");
		}
		return 0;
	}
	else if(mode == AT_CMD_MODE_SET)
	{
		if(pbuf[0] == '1') lsleep_model = 1;
		else lsleep_model = 0;
		tinyFlash_Write(STORAGE_LSLEEP, &lsleep_model, 1);
		return 0;
	}
	else if(mode == AT_CMD_MODE_EXECUTION)
	{
		lsleep_enable();
		at_print("\r\nOK\r\n");
		return 0xFF;
	}
	else
	{
		return 2;
	}
}
//恢复出厂设置并重启
static unsigned char atCmd_Restore(char *pbuf,  int mode, int lenth)
{
	tinyFlash_Format();
	at_print("\r\nOK\r\n");
	start_reboot();
	return 0;
}
//波特率
static unsigned char atCmd_Baud(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		printf("\r\n+BAUD:%d",baud_buf[0]);
		return 0;
	}

	if(mode == AT_CMD_MODE_SET)
	{
		if((pbuf[0] >= '0') && (pbuf[0] <= '9'))
		{
			pbuf[0] -= '0';
			tinyFlash_Write(STORAGE_BAUD, (unsigned char*)pbuf, 1);
			return 0;
		}
		else
		{
			return 2;
		}
	}
	return 1;
}
//名字
static unsigned char atCmd_Name(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		at_print("\r\n+NAME:");

		if(my_scanRsp[1] == 0x09) //客户自定义的蓝牙设备名称
		{
			at_send(my_scanRsp+2, my_scanRsp[0] -1);
		}
		else
		{
			at_send(tbl_scanRsp+2, 10);
		}

		return 0;
	}

	if(mode == AT_CMD_MODE_SET)
	{
		tinyFlash_Write(STORAGE_NAME, (unsigned char*)pbuf, lenth);
		return 0;
	}
	return 1;
}
//MAC地址
static unsigned char atCmd_Mac(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		printf("\r\n+MAC:%X%X%X%X%X%X", mac_public[5], mac_public[4], mac_public[3], mac_public[2], mac_public[1], mac_public[0] );
		return 0;
	}

	if(mode == AT_CMD_MODE_SET)
	{
		if(lenth != 12) 
		{
			at_print("len error\r\n");
			return 2;
		}

		if(str2hex(pbuf, 12) == -1 ) return 2;

		pbuf[6] = pbuf[0];
		pbuf[0] = pbuf[5];
		pbuf[5] = pbuf[6];

		pbuf[6] = pbuf[1];
		pbuf[1] = pbuf[4];
		pbuf[4] = pbuf[6];

		pbuf[6] = pbuf[2];
		pbuf[2] = pbuf[3];
		pbuf[3] = pbuf[6];

		flash_erase_sector (CFG_ADR_MAC);
		flash_write_page (CFG_ADR_MAC, 8, (unsigned char*)pbuf);
		
		return 0;
	}

	return 1;
}

/*读取某片Flash数据到全局变量，主要用于调试tinyFlash*/
unsigned long r_addr = 0;
static unsigned char atCmd_Read(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_SET)
	{

		if((pbuf[0] >= '0') && (pbuf[0] <= '9'))
		{
			pbuf[0] -= '0';
		}
		else if((pbuf[0] >= 'A') && (pbuf[0] <= 'F'))
		{
			pbuf[0] -= ('A' -10);
		}
		

		if((pbuf[1] >= '0') && (pbuf[1] <= '9'))
		{
			pbuf[1] -= '0';
		}
		else if((pbuf[1] >= 'A') && (pbuf[1] <= 'F'))
		{
			pbuf[1] -= ('A' -10);
		}

		pbuf[0] = pbuf[0] *16 + pbuf[1];

		r_addr = pbuf[0] * 256;

		r_addr += 0x70000;

		tinyFlash_Debug(r_addr);
		return 0;
	}
	else
	{
		return 1;
	}
}

extern u32 device_in_connection_state; //从机状态下已被连接标志位
extern u32 cur_conn_device_hdl;//主机状态下已建立连接标志位
static unsigned char atCmd_State(char *pbuf,  int mode, int lenth)
{
	if((device_in_connection_state ==0) && (cur_conn_device_hdl == 0))
	{
		at_print("\r\n+STATE:0");
	}
	else
	{
		at_print("\r\n+STATE:1");
	}
	return 0;
}

//设置主机模式或者从机模式 0:从机模式，1:主机模式,重启后生效
extern u32 device_mode;
static unsigned char atCmd_Mode(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		if(device_mode == 1) 
			at_print("\r\n+MODE:1");
		else if(device_mode == 2) 
			at_print("\r\n+MODE:2");
		else
			at_print("\r\n+MODE:0");
	}
	else if(mode == AT_CMD_MODE_SET)
	{
		if((pbuf[0] >= '0') && (pbuf[0] <= '2'))
		{
			pbuf[0] -= '0';
			tinyFlash_Write(STORAGE_MODE, (unsigned char*)pbuf, 1);
			return 0;
		}
		else
		{
			return 2;
		}
	}
	else
	{
		return 2;
	}
	return 0;
}

static unsigned char Scan_Stop()
{
	at_print("OK\r\n");
	blt_soft_timer_delete(Scan_Stop);
	blc_ll_setScanEnable (BLC_SCAN_DISABLE, DUP_FILTER_DISABLE);
}
//蓝牙主机模式开始扫描
static unsigned char atCmd_Scan(char *pbuf,  int mode, int lenth)
{
	if(device_mode == 1)
	{
		//set scan parameter and scan enable
		blc_ll_setScanParameter(SCAN_TYPE_ACTIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS, OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
		blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_ENABLE);

		blt_soft_timer_add(&Scan_Stop, 3000000);//3S
		return 0xff;
	}

	return 2;
}
//主动断开连接
static unsigned char atCmd_Disconnect(char *pbuf,  int mode, int lenth)
{
	if(device_mode == 0)  //从机模式
	{
		bls_ll_terminateConnection(HCI_ERR_REMOTE_USER_TERM_CONN);
	}
	else if(device_mode == 1) //主机模式
	{
		blm_ll_disconnect(cur_conn_device_hdl, HCI_ERR_REMOTE_USER_TERM_CONN);
	}
	
	return 0;
}
//主动连接
static unsigned char atCmd_Connect(char *pbuf,  int mode, int lenth)
{
	//只有是主机模式且未建立连接才能发起连接
	if((mode == AT_CMD_MODE_SET) && (device_mode == 1) && (cur_conn_device_hdl == 0))
	{
		if(lenth != 12) 
		{
			at_print("len error\r\n");
			return 2;
		}

		if(str2hex(pbuf, 12) == -1 ) return 2;

		pbuf[6] = pbuf[0];
		pbuf[0] = pbuf[5];
		pbuf[5] = pbuf[6];

		pbuf[6] = pbuf[1];
		pbuf[1] = pbuf[4];
		pbuf[4] = pbuf[6];

		pbuf[6] = pbuf[2];
		pbuf[2] = pbuf[3];
		pbuf[3] = pbuf[6];

		blc_ll_createConnection( SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS, INITIATE_FP_ADV_SPECIFY,  \
								0, pbuf, BLE_ADDR_PUBLIC, \
								CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 0, CONN_TIMEOUT_4S, \
								0, 0xFFFF);
	}
	else
	{
		return 2;
	}

	at_print("Connecting... ...\r\n");
	return 0xff;
}


//AT+SEND=46,4646464646546\r\n
static unsigned char atCmd_Send(char *pbuf,  int mode, int lenth)
{
	if((device_in_connection_state == 0) && (cur_conn_device_hdl == 0)) //如果蓝牙未连接,或者未开启Notify
	{
		return 2;
	}

	char *tmp = strchr(pbuf,',');
	int len =0;

	if((tmp != NULL) && ((tmp - pbuf) < 4))
	{
		char *data = tmp + 1; //要发送的数据的指针
		char *len_p = pbuf;	 //数据长度指针
		//解析数据长度
		while(tmp != len_p)
		{
			len = len * 10 + (len_p[0] - '0');
			len_p++;
		}

		//检验长度是否一致
		if((len + (data - pbuf)) != lenth)
		{
			return 2;
		}

		if(device_mode == 0)//当前为从机模式，发送数据到主机
		{
			bls_att_pushNotifyData(SPP_SERVER_TO_CLIENT_DP_H, (u8*)data, len);
		}
		else //当前为主机模式，发送数据到从机
		{
			blc_gatt_pushWriteComand(cur_conn_device_hdl, SPP_SERVER_TO_CLIENT_DP_H,  (u8*)data, len);
		}
		return 0;
	}
	else
	{
		return 2;
	}
}

extern u8 tbl_advData[];
static unsigned char atCmd_Advdata(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		at_print("\r\n+ADVDATA:");
		at_send(tbl_advData+15, tbl_advData[13] -1);
		return 0;
	}
	else if(mode == AT_CMD_MODE_SET)
	{
		if(lenth > 16) return 2;
		tinyFlash_Write(STORAGE_ADVDATA, pbuf, lenth);
		return 0;
	}
}

//设置关闭间隙
extern u16 user_adv_interval_ms;
static unsigned char atCmd_Advintv(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		printf("\r\n+ADVINTV:%d", user_adv_interval_ms);
		return 0;
	}
	else if(mode == AT_CMD_MODE_SET)
	{
		u16 interval = 0;
		while(lenth--)
		{
			interval = interval * 10 + (pbuf[0] - '0');
			pbuf++;
		}
		tinyFlash_Write(STORAGE_ADVINTV, &interval, 2);
		return 0;
	}
}

//设置发射功率
extern u8 user_rf_power_index;
void user_set_rf_power (u8 e, u8 *p, int n);
static unsigned char atCmd_rf_power(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		printf("\r\n+RFPWR:%d", user_rf_power_index);
		return 0;
	}
	else if(mode == AT_CMD_MODE_SET)
	{
		u8 tmp =  (pbuf[0] - '0');

		if(tmp < 10)
		{
			user_rf_power_index = tmp;
			user_set_rf_power(0,0,0);
			tinyFlash_Write(STORAGE_RFPWR, &user_rf_power_index, 1);
			return 0;
		}
		return 2;
	}
}

 extern u8 ibeacon_data[30];
//设置或者查询iBeacon UUID
static unsigned char atCmd_Ibeacon_UUID(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		printf("\r\n+IBCNIIUD:%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", 
										ibeacon_data[ 9], ibeacon_data[10], ibeacon_data[11],ibeacon_data[12],
										ibeacon_data[13], ibeacon_data[14], ibeacon_data[15],ibeacon_data[16],
										ibeacon_data[17], ibeacon_data[18], ibeacon_data[19],ibeacon_data[20],
										ibeacon_data[21], ibeacon_data[22], ibeacon_data[23],ibeacon_data[24]
		);
		return 0;
	}
	else if(mode == AT_CMD_MODE_SET)
	{
		if(lenth != 32) return 2;

		if(str2hex(pbuf, 32) == -1 ) return 2;

		memcpy(ibeacon_data+9, pbuf, 16);

		tinyFlash_Write(STORAGE_IUUID, pbuf, 16);
		return 0;
	}
}

//设置或者查询iBeacon Major
static unsigned char atCmd_Major(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		printf("\r\n+MAJOR:%02X%02X", ibeacon_data[25], ibeacon_data[26]);
		return 0;
	}
	else if(mode == AT_CMD_MODE_SET)
	{
		if(lenth != 4) return 2;

		if(str2hex(pbuf, 4) == -1 ) return 2;

		memcpy(ibeacon_data+25, pbuf, 2);

		tinyFlash_Write(STORAGE_IMAJOR, &pbuf, 2);
		return 0;
	}
}

//设置后者查询iBeacon Minor
static unsigned char atCmd_Minor(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		printf("\r\n+MINOR:%02X%02X", ibeacon_data[27],ibeacon_data[28]);
		return 0;
	}
	else if(mode == AT_CMD_MODE_SET)
	{
		if(lenth != 4) return 2;

		if(str2hex(pbuf, 4) == -1 ) return 2;

		memcpy(ibeacon_data+27, pbuf, 2);
		tinyFlash_Write(STORAGE_IMONOR, &pbuf, 2);
		return 0;
	}
}

//用于测试开发板
static unsigned char atCmd_Board_test(char *pbuf,  int mode, int lenth)
{
	gpio_set_func(GPIO_PC2, AS_GPIO);
	gpio_set_func(GPIO_PC3, AS_GPIO);
	gpio_set_func(GPIO_PC4, AS_GPIO);
	gpio_set_func(GPIO_PB4, AS_GPIO);
	gpio_set_func(GPIO_PB5, AS_GPIO);

	gpio_set_output_en(GPIO_PC2, 1);//enable output
	gpio_set_output_en(GPIO_PC3, 1);//enable output
	gpio_set_output_en(GPIO_PC4, 1);//enable output
	gpio_set_output_en(GPIO_PB4, 1);//enable output
	gpio_set_output_en(GPIO_PB5, 1);//enable output

	gpio_set_input_en(GPIO_PC2, 0);//disenable input
	gpio_set_input_en(GPIO_PC3, 0);//disenable input
	gpio_set_input_en(GPIO_PC4, 0);//disenable input
	gpio_set_input_en(GPIO_PB4, 0);//disenable input
	gpio_set_input_en(GPIO_PB5, 0);//disenable input

	while(1)
	{
		gpio_write(GPIO_PC2,1);
		gpio_write(GPIO_PC3,0);
		gpio_write(GPIO_PC4,0);
		gpio_write(GPIO_PB4,0);
		gpio_write(GPIO_PB5,0); WaitMs(200);

		gpio_write(GPIO_PC2,0);
		gpio_write(GPIO_PC3,1);
		gpio_write(GPIO_PC4,0);
		gpio_write(GPIO_PB4,0);
		gpio_write(GPIO_PB5,0); WaitMs(200);

		gpio_write(GPIO_PC2,0);
		gpio_write(GPIO_PC3,0);
		gpio_write(GPIO_PC4,1);
		gpio_write(GPIO_PB4,0);
		gpio_write(GPIO_PB5,0); WaitMs(200);

		gpio_write(GPIO_PC2,0);
		gpio_write(GPIO_PC3,0);
		gpio_write(GPIO_PC4,0);
		gpio_write(GPIO_PB4,1);
		gpio_write(GPIO_PB5,0); WaitMs(200);

		gpio_write(GPIO_PC2,0);
		gpio_write(GPIO_PC3,0);
		gpio_write(GPIO_PC4,0);
		gpio_write(GPIO_PB4,0);
		gpio_write(GPIO_PB5,1); WaitMs(200);
	}
}
//读写命令
_at_command_t gAtCmdTb_writeRead[] =
{ 
	{ "BAUD", 	atCmd_Baud,	"Set/Read BT Baud\r\n"},
	{ "NAME", 	atCmd_Name,	"Set/Read BT Name\r\n"},
	{ "MAC", 	atCmd_Mac,	"Set/Read BT MAC\r\n"},
	{ "READ", 	atCmd_Read,	"Read Flash Data\r\n"},
	{ "MODE", 	atCmd_Mode, "Set/Read BT Mode\r\n"},
	{ "SEND", 	atCmd_Send, "Send data to phone\r\n"},
	{ "CONNECT",atCmd_Connect,"Connect other slave device\r\n"},
	{ "ADVDATA",atCmd_Advdata,"Set/Read Adv Data\r\n"},
	{ "ADVINTV",atCmd_Advintv,"Set/Read Adv interval\r\n"},
	{ "LSLEEP", atCmd_LSleep, "Sleep\r\n"},
	{ "RFPWR",  atCmd_rf_power, "RF Power\r\n"},
	{ "IBCNUUID",atCmd_Ibeacon_UUID, "iBeacon UUID\r\n"},
	{ "MAJOR",  atCmd_Major, "iBeacon Major\r\n"},
	{ "MINOR",  atCmd_Minor, "iBeacon Minor\r\n"},
	{0, 	0,	0}
};
//控制命令
_at_command_t gAtCmdTb_exe[] =
{
	{ "1", 		atCmd_ATE1, "ATE1\r\n"},  //ATE1
	{ "0", 		atCmd_ATE0, "ATE0\r\n"},  //ATE0
	{ "GMR", 	atCmd_GMR,  "GMR\r\n"}, 
	{ "RST", 	atCmd_Reset, "RESET\r\n"}, 
	{ "SLEEP", 	atCmd_Sleep, "Sleep\r\n"}, 	
	{ "LSLEEP", atCmd_LSleep, "Sleep\r\n"},
	{ "RESTORE",atCmd_Restore,"RESTORE\r\n"},
	{ "STATE",  atCmd_State,  "State\r\n"},
	{ "SCAN",   atCmd_Scan,   "Scan\r\n"},
	{ "DISCONN",atCmd_Disconnect,"disconnect\r\n"},
	{ "BTEST",  atCmd_Board_test,"Board_test\r\n"},
	{0, 	0,	0}
};
