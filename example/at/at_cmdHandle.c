#include "tl_common.h"
#include "at_cmd.h"
#include "drivers.h"

#include "stack/ble/ble.h"

#include "tinyFlash/tinyFlash.h"

#define STORAGE_NAME 1
#define STORAGE_BAUD 2
#define STORAGE_ATE  3

static unsigned char buf[64] = { 0 };

extern void at_print(char * str);
extern u8 baud_buf[];
extern  const u8 tbl_scanRsp[];
extern u8 my_scanRsp[32];
extern u8 ATE;
extern u8  mac_public[6];

static unsigned char atCmd_ATE0(char *pbuf,  int mode, int lenth)
{
	ATE = 0;
	tinyFlash_Write(STORAGE_ATE, &ATE, 1);
	return 0;
}

static unsigned char atCmd_ATE1(char *pbuf,  int mode, int lenth)
{
	ATE = 1;
	tinyFlash_Write(STORAGE_ATE, &ATE, 1);
	return 0;
}

static unsigned char atCmd_GMR(char *pbuf,  int mode, int lenth)
{
	at_print("\r\n+VER:0.1");
	return 0;
}

static unsigned char atCmd_Reset(char *pbuf,  int mode, int lenth)
{
	at_print("\r\nOK\r\n");
	start_reboot();
	return 0;
}

static unsigned char atCmd_Sleep(char *pbuf,  int mode, int lenth)
{
	at_print("\r\nOK\r\n");
	gpio_setup_up_down_resistor(GPIO_PB0, PM_PIN_PULLDOWN_100K);
	cpu_set_gpio_wakeup (GPIO_PB0, Level_Low, 1); 
	cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  //deepsleep
	return 0;
}

static unsigned char atCmd_Restore(char *pbuf,  int mode, int lenth)
{
	tinyFlash_Format();
	at_print("\r\nOK\r\n");
	start_reboot();
	return 0;
}

static unsigned char atCmd_Baud(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		at_print("\r\n+BAUD:");
		buf[0] = baud_buf[0] + '0';
		at_send(buf, 1);

		return 0;
	}

	if(mode == AT_CMD_MODE_SET)
	{
		if((pbuf[0] >= '0') && (pbuf[0] <= '9'))
		{
			pbuf[0] -= '0';
			tinyFlash_Write(STORAGE_BAUD, pbuf, 1);
			return 0;
		}
		else
		{
			return 2;
		}
	}
}

static unsigned char atCmd_Name(char *pbuf,  int mode, int lenth)
{
	unsigned char len = 64;

	if(mode == AT_CMD_MODE_READ)
	{
		memset(buf, 0, 64);

		at_print("\r\n+NAME:");

		if(my_scanRsp[1] == 0x09) //客户自定义的蓝牙设备名称
		{
			memcpy(buf, my_scanRsp+2, my_scanRsp[0] -1);
		}
		else
		{
			memcpy(buf, tbl_scanRsp+2, 10);
		}
		
		at_print(buf);

		return 0;
	}

	if(mode == AT_CMD_MODE_SET)
	{
		tinyFlash_Write(STORAGE_NAME, pbuf, lenth);
		return 0;
	}
}

static unsigned char atCmd_Mac(char *pbuf,  int mode, int lenth)
{
	if(mode == AT_CMD_MODE_READ)
	{
		at_print("\r\n+MAC:");
		u_sprintf(buf, "%X%X%X%X%X%X", mac_public[5], mac_public[4], mac_public[3], mac_public[2], mac_public[1], mac_public[0] );
		at_print(buf);
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
		flash_write_page (CFG_ADR_MAC, 8, pbuf);
		
		return 0;
	}
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



_at_command_t gAtCmdTb_writeRead[] =
{ 
	{ "BAUD", 	atCmd_Baud,	"Set/Read BT Baud\r\n"},
	{ "NAME", 	atCmd_Name,	"Set/Read BT Name\r\n"},
	{ "MAC", 	atCmd_Mac,	"Set/Read BT MAC\r\n"},
	{ "READ", 	atCmd_Read,	"Read Flash Data\r\n"},
	{ "STATE", 	atCmd_State,"Read BT Connect State\r\n"},
	{0, 	0,	0}
};

_at_command_t gAtCmdTb_exe[] =
{
	{ "1", 		atCmd_ATE1, "ATE1\r\n"},  //ATE1
	{ "0", 		atCmd_ATE0, "ATE0\r\n"},  //ATE0
	{ "GMR", 	atCmd_GMR,  "GMR\r\n"}, 
	{ "RST", 	atCmd_Reset, "RESET\r\n"}, 
	{ "SLEEP", 	atCmd_Sleep, "Sleep\r\n"}, 	
	{ "RESTORE",atCmd_Restore,"RESTORE\r\n"},
	{ "STATE",  atCmd_State,  "State\r\n"},
	{0, 	0,	0}
};
