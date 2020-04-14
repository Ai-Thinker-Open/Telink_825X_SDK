#include "tl_common.h"
#include "at_cmd.h"
#include "drivers.h"

#include "stack/ble/ble.h"

#include "tinyFlash/tinyFlash.h"

#define STORAGE_NAME 1
#define STORAGE_BAUD 2
#define STORAGE_ATE  3
#define STORAGE_MODE 4
//外部变量
extern u8 baud_buf[];
extern  const u8 tbl_scanRsp[];
extern u8 my_scanRsp[32];
extern u8 ATE;
extern u8  mac_public[6];

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
		at_print("\r\n+BAUD:");
		at_print_buf[0] = baud_buf[0] + '0';
		at_send(at_print_buf, 1);

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
		memset(at_print_buf, 0, 64);

		at_print("\r\n+NAME:");

		if(my_scanRsp[1] == 0x09) //客户自定义的蓝牙设备名称
		{
			memcpy(at_print_buf, my_scanRsp+2, my_scanRsp[0] -1);
		}
		else
		{
			memcpy(at_print_buf, tbl_scanRsp+2, 10);
		}
		
		at_print((char*)at_print_buf);

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
		at_print("\r\n+MAC:");
		u_sprintf((char*)at_print_buf, "%X%X%X%X%X%X", mac_public[5], mac_public[4], mac_public[3], mac_public[2], mac_public[1], mac_public[0] );
		at_print((char*)at_print_buf);
		return 0;
	}

	if(mode == AT_CMD_MODE_SET)
	{
		if(lenth != 12) 
		{
			at_print("len error\r\n");
			return 2;
		}

		for(lenth = 0; lenth < 12; lenth ++)
		{
			if(((pbuf[lenth] >= '0') && (pbuf[lenth] <= '9')) || ((pbuf[lenth] >= 'A') && (pbuf[lenth] <= 'F')))
			{
				if((pbuf[lenth] >= '0') && (pbuf[lenth] <= '9'))
				{
					pbuf[lenth] -= '0';
				}
				else
				{
					pbuf[lenth] -= 'A';
					pbuf[lenth] += 0x0A;
				}

				if(lenth%2)
				{
					pbuf[lenth/2] = (pbuf[lenth-1] << 4) | pbuf[lenth];
				}
			}
			else
			{
				return 2;
			}
		}

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

extern u32 device_in_connection_state;
static unsigned char atCmd_State(char *pbuf,  int mode, int lenth)
{
	if(device_in_connection_state ==0)
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
		else
			at_print("\r\n+MODE:0");
	}
	else if(mode == AT_CMD_MODE_SET)
	{
		if((pbuf[0] >= '0') && (pbuf[0] <= '1'))
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
	//set scan parameter and scan enable
	blc_ll_setScanParameter(SCAN_TYPE_ACTIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS, OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
	blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_ENABLE);

	blt_soft_timer_add(&Scan_Stop, 3000000);//3S

	return 0xff;
}
//主动断开连接
static unsigned char atCmd_Disconnect(char *pbuf,  int mode, int lenth)
{
	return 0;
}
//主动连接
static unsigned char atCmd_Connect(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_SET)
	{
		if(lenth != 12) 
		{
			at_print("len error\r\n");
			return 2;
		}

		for(lenth = 0; lenth < 12; lenth ++)
		{
			if(((pbuf[lenth] >= '0') && (pbuf[lenth] <= '9')) || ((pbuf[lenth] >= 'A') && (pbuf[lenth] <= 'F')))
			{
				if((pbuf[lenth] >= '0') && (pbuf[lenth] <= '9'))
				{
					pbuf[lenth] -= '0';
				}
				else
				{
					pbuf[lenth] -= 'A';
					pbuf[lenth] += 0x0A;
				}

				if(lenth%2)
				{
					pbuf[lenth/2] = (pbuf[lenth-1] << 4) | pbuf[lenth];
				}
			}
			else
			{
				return 2;
			}
		}

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
	if(device_in_connection_state == 0) //如果蓝牙未连接,或者未开启Notify
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

		bls_att_pushNotifyData(SPP_SERVER_TO_CLIENT_DP_H, (u8*)data, len); //release

		return 0;
	}
	else
	{
		return 2;
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
	{ "RESTORE",atCmd_Restore,"RESTORE\r\n"},
	{ "STATE",  atCmd_State,  "State\r\n"},
	{ "SCAN",   atCmd_Scan,   "Scan\r\n"},
	{ "DISCONN",atCmd_Disconnect,"disconnect\r\n"},
	{ "BTEST",  atCmd_Board_test,"Board_test\r\n"},
	{0, 	0,	0}
};
