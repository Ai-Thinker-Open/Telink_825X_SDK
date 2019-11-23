#include "tl_common.h"
#include "at_cmd.h"
#include "drivers.h"

#include "tinyFlash/tinyFlash.h"

#define STORAGE_NAME 1
#define STORAGE_BAUD 2

static unsigned char buf[64] = { 0 };

extern void at_print(char * str);

static unsigned char atCmd_ATE0(char *pbuf,  int mode, int lenth)
{
	at_print("OK\r\n");
	return 0;
}

static unsigned char atCmd_ATE1(char *pbuf,  int mode, int lenth)
{
	at_print("OK\r\n");
	return 0;
}

static unsigned char atCmd_GMR(char *pbuf,  int mode, int lenth)
{
	at_print("+VER:0.1\r\n");
	return 0;
}

static unsigned char atCmd_Reset(char *pbuf,  int mode, int lenth)
{
	at_print("OK\r\n");
	start_reboot();
	return 0;
}

static unsigned char atCmd_Sleep(char *pbuf,  int mode, int lenth)
{
	at_print("OK\r\n");
	gpio_setup_up_down_resistor(GPIO_PB0, PM_PIN_PULLDOWN_100K);
	cpu_set_gpio_wakeup (GPIO_PB0, Level_Low, 1); 
	cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  //deepsleep
	return 0;
}

static unsigned char atCmd_Restore(char *pbuf,  int mode, int lenth)
{
	at_print("OK\r\n");
	return 0;
}

static unsigned char atCmd_Baud(char *pbuf,  int mode, int lenth)
{
	unsigned char len = 64;

	if(mode == AT_CMD_MODE_READ)
	{
		memset(buf, 0, 64);

		tinyFlash_Read(STORAGE_BAUD, buf, &len);

		at_print("+BAUD:");
		at_print(buf);
		at_print("\r\n");

		return 0;
	}

	if(mode == AT_CMD_MODE_SET)
	{
		tinyFlash_Write(STORAGE_BAUD, pbuf, lenth);
		return 0;
	}
}

static unsigned char atCmd_Name(char *pbuf,  int mode, int lenth)
{
	unsigned char len = 64;

	if(mode == AT_CMD_MODE_READ)
	{
		memset(buf, 0, 64);
		tinyFlash_Read(STORAGE_NAME, buf, &len);

		at_print("+NAME:");
		at_print(buf);
		at_print("\r\n");

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
	at_print("+MAC:\r\n");
	return 0;
}

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

static unsigned char atCmd_State(char *pbuf,  int mode, int lenth)
{
	at_print("+STATE:1\r\n");
	return 0;
}



_at_command_t gAtCmdTb_writeRead[] =
{ 
	{ "BAUD", 	atCmd_Baud,	"Set/Read BT MAC\r\n"},
	{ "NAME", 	atCmd_Name,	"Set/Read BT Name\r\n"},
	{ "MAC", 	atCmd_Mac,	"Set/Read BT MAC\r\n"},
	{ "READ", 	atCmd_Read,	"Set/Read BT MAC\r\n"},
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
