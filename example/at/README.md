<!--
 * @Author: your name
 * @Date: 2020-03-31 19:46:54
 * @LastEditTime: 2020-04-01 16:53:37
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /Telink_825X_SDK/example/at/README.md
 -->
# AT 固件设计原理

## AT 设计原理

~~自行理解代码！！！~~

为部分文件添加了注释~

## AT 模式选择与转换
如果不对PC5引脚进行处理(悬空)，模块未与手机连接时，将处于AT模式，可响应AT指令。模块与手机连接后即进入透传模式，在透传模式下，MCU通过串口发送给模块的数据，模块会将其原封不动通过蓝牙转发到手机。同样，手机通过蓝牙发送给模块的数据，模块也会原封不动通过串口传送给MCU。

模块未与手机连接时，将处于AT模式，可响应AT指令。与手机连接后将进入透传模式，此时不再响应AT指令。如果用户需要在透传模式下发送AT指令，可将PC5引脚拉低，拉低后模块将临时进入AT模式，释放后重新回到透传模式。状态对应如下表：

||未与手机建立连接|已与手机建立连接
|---|---|---|
|<del>PC5</del>CONTROL_GPIO为高电平|AT模式|透传模式
|<del>PC5</del>CONTROL_GPIO为低电平|AT模式|AT模式

备注：如果用户不需要使用透传模式，将<del>PC5</del>CONTROL_GPIO通过电阻下拉即可。AT模式下可通过AT+SEND指令发送数据。

## 板子选择

- 默认是 TB02模块开发板，如下选择对应您的板子模块，代码在  ```app_config.h```

```
#define _MODULE_TB_02_DEV_BOARD_

#if defined _MODULE_TB_01_ //TB01模块
#define CONTROL_GPIO GPIO_PC5
#define UART_RX_PIN UART_RX_PB0
#elif defined _MODULE_TB_02_ //TB02模块
#define CONTROL_GPIO GPIO_PB7
#define UART_RX_PIN UART_RX_PA0
#elif defined _MODULE_TB_02_DEV_BOARD_ //TB02开发板
#define CONTROL_GPIO GPIO_PA0
#define UART_RX_PIN UART_RX_PB7
#else
#error "please set module type"
#endif
```

## AT 指令格式
AT 指令可以细分为四种格式类型：

|类型|指令格式|描述|备注
|---|---------|---|---|
|查询指令|AT+<x>?|查询命令中的当前值。	
|设置指令|AT+<x>=<…>|设置用户自定义的参数值。	
|执行指令|AT+<x>|执行某些参数不可变的功能。	
|测试指令|AT+<x>=?|返回指令帮助信息	


## AT 指令集

|序号|指令|功能|备注|
|----|-----|----|----|
|1|AT|测试AT|
|2|ATE|开关回显|
|3|AT+GMR|查询固件版本|
|4|AT+RST|重启模组
|5|AT+SLEEP|深度睡眠|
|6|AT+ RESTORE|恢复出厂设置|恢复后将重启|
|7|AT+BAUD|查询或设置波特率|重启后生效|
|8|AT+NAME|查询或设置蓝牙广播名称|重启后生效|
|9|AT+MAC|设置或查询模组MAC地址|重启后生效|
|10|AT+MODE|查询或者是主从模式
|11|AT+STATE|查询蓝牙连接状态
|12|AT+SCAN|主机模式下发起扫描
|13|AT+CONNECT|主机模式下发起连接
|14|AT+DISCON|断开连接
|15|AT+SEND|AT模式下发送数据|
|16|+DATA|AT模式下收到数据|
|17|AT+ADVDATA|设置广播数据中的厂商自定义字段内容|
|18|AT+LSLEEP|设置或进入轻度睡眠|

## 主机模式
在主机模式下，模块可与另一个从机模块通信，主要操作如下：

将模块配置为主机模式：

	AT+MODE=1

扫描周围的模块：

	AT+SCAN

连接指导定的模块：

	AT+CONNECT=AC04187852AD

注意将上面的MAC地址换成你的从机模块的MAC地址

返回```OK``表示连接成功，采用如下指令发数据到从机：

	AT+SEND=5,12345

备注：主机状态下只有AT指令模式，没有透传模式。

## 低功耗
该AT固件支持两种睡眠模式，即```深度睡眠```和```浅睡眠```,在深度睡眠模式下，模块除GPIO唤醒功能外，其他功能全部关闭，功耗在1uA一下。浅睡眠模式除了保留GPIO唤醒外，还保持蓝牙的功能，功耗以广播参数而定，平均约10uA以下。

进入深度睡眠模式：

	AT+SLEEP

执行上诉指令模块返回OK后将将立即进入睡眠模式，并将串口RX设为唤醒引脚，再次向模块发送任意字符即可唤醒。

浅睡眠设置：
在未连接状态下，发送如下指令，模块将进入浅睡眠模式：

	AT+LSLEEP

在浅睡眠模式下，模块依然会进行蓝牙广播。浅睡眠模式不再响应任何AT指令，可通过串口RX引脚发送任何数据唤醒模块。

当有别的蓝牙设备与该模块连接成功时，也会唤醒模块。

上电自动进入浅睡眠模式：

	AT+LSLEEP=1

上电不自动进入浅睡眠模式;

	AT+LSLEEP=0

备注：浅睡眠模式在对从机状态下起作用。
## FAQ 

- 1/ 修改蓝牙广播默认名字，默认是“Ai-Thinker”

```
//app.c
const u8 tbl_scanRsp [] = {
		 0x0B, 0x09, 'A', 'i', '-', 'T', 'h', 'i', 'n', 'k', 'e', 'r',
	};
```
