#include "at_cmd.h"

static unsigned char atCmd_Read(char *pbuf,  int mode, int lenth)
{
	at_print("+data\r\n");
	return 0;
}

static unsigned char atCmd_version(char *pbuf,  int mode, int lenth)
{
	at_print("+VER 0.1\r\n");
	return 0;
}

_at_command_t gAtCmdTb_writeRead[] =
{ 
	{ "READ", 	atCmd_Read,	"SF BW CR set, example: AT+SBC=7,8,2\r\n"},
	{0, 	0,	0}
};

_at_command_t gAtCmdTb_exe[] =
{
	{ "VER", 	atCmd_version, "Version\r\n"},
	{0, 	0,	0}
};
