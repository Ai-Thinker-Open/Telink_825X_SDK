#ifndef __AT_CMD_H__
#define __AT_CMD_H__

#define AT_MAX_CMD_ARGS            10

//void _at_cmd_func_init(void);
void at_data_process(char *pbuf, int len);

void at_response_result(unsigned char result_code);

typedef enum {
    AT_RESULT_CODE_OK = 0,
    AT_RESULT_CODE_ERROR,
    AT_RESULT_CODE_INVALID,
    AT_RESULT_CODE_MAX,
} at_result_code_string_index_t;

typedef unsigned char (*_at_cmd_handler_fp_t)(char * buf,  int mode, int len);

extern _at_cmd_handler_fp_t _at_cmd_handler;


/**
 * @brief AT Command Structure
 *
 */
typedef struct _at_command {
	const char *cmd; /**< Command String. */
	_at_cmd_handler_fp_t cmd_handle; /**< Command Handler. */
	const char *help;
}_at_command_t;

extern _at_command_t gAtCmdTb_writeRead[];
extern _at_command_t gAtCmdTb_exe[];
extern	_at_command_t gAtCmdTb_test[];

#define PREFIX_AT_CMD_EXE ""
#define PREFIX_ATCMD_SET "="
#define PREFIX_AT_CMD_READ "?"
#define PREFIX_AT_CMD_HELP "=?"

typedef enum {
    AT_CMD_MODE_READ = 0,       /**< Read mode command, such as "AT+CMD?". */
    AT_CMD_MODE_SET,        /**< Execute mode command, such as "AT+CMD=<op>". */
	AT_CMD_MODE_EXECUTION,	/**< Active mode command, such as "AT+CMD". */
    AT_CMD_MODE_HELP,    /**< Test mode command, such as "AT+CMD=?". */
    AT_CMD_MODE_INVALID     /**< The input command doesn't belong to any of the four types. */
} at_cmd_mode_t;

void at_print(char * str);
void at_send(char * data, u32 len);

#endif //__AT_CMD_H__
