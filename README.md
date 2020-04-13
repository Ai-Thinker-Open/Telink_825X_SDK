# Telink TLSR825X Software Development Kit
--------------------------------------------------
Telink 泰凌 TLSR825X 蓝牙芯片软件开发套件,推荐配合安信可开发板一起使用。

# 使用方法

### 搭建开发环境

>Windows系统开发环境搭建： [Window开发环境搭建](https://github.com/Ai-Thinker-Open/Telink_825X_SDK/blob/master/start_windows.md)

> Linux 64Bit 系统开发环境搭建：[Linux开发环境搭建](https://github.com/Ai-Thinker-Open/Telink_825X_SDK/blob/master/start_linux.md)

> mac OS 开发环境搭建：[mac OS 开发环境搭建](https://github.com/Ai-Thinker-Open/Telink_825X_SDK/blob/master/start_macos.md)


### 获取SDK

    git clone https://github.com/Ai-Thinker-Open/Telink_825X_SDK.git

### 编译demo 程序
进入blink示例工程目录

    cd Telink_825X_SDK/example/blink 

执行下列编译指令：

    make

输出类似如下信息说明编译成功：

    Invoking: Print Size
    tc32-elf-size -t /home/aithinker/ESP/Telink_SDK/example/blink/out/blink.elf
    text	   data	    bss	    dec	    hex	filename
    3712	      8	    593	   4313	   10d9	/home/aithinker/ESP/Telink_SDK/example/blink/out/blink.elf
    3712	      8	    593	   4313	   10d9	(TOTALS)
    Finished building: sizedummy

### 烧录程序到芯片

> 芯片本身并不支持串口烧录，只能使用芯片原厂提供的烧录器烧录。安信可自主开发了串口烧录工具，无需官方烧录器即可使用，前提是要先将安信可bootloader烧录到模块中，一般安信可出厂的模块及开发板都烧录了支持串口烧录的bootloader。

- 如果你使用安信可TB系列开发板进行开发，直接将开发板通过USB连接到计算机即可。
- 如果你使用安信可TB系列模块进行开发，需要准备一个支持硬件流控的USB转串口模块，并按照下表将蓝牙模块与USB转串口连接起来，然后将USB转串口插入计算机。

#### 串口烧录接线方式如下：

|串口|模块|
|----|---|
|VCC|3V3|
|GND|GND|
|TX|RX|
|RX|TX|
|RTS|RST|
|DTR|SWS|

备注：SWS为boot选择引脚，为低电平进入下载模式，为高电平进入运行模式

#### 设置串口号

将开发板或模块通过USB连接计算机后，查看下其对应的串口号：

- Windows系统在设备管理器中可以查看串口，windows串口号以```com```开头
- Linux系统通过```ls /dev/ttyUSB*```指令查看产口号，linux系统串口号以```/dev/ttyUSB```开头
- mac OS 系统通过```ls /dev/cu*```指令查看产口号，linux系统串口号以```/dev/cu.```开头


查看到串口号，修改blink目录下的makefile文件，将 ```DOWNLOAD_PORT``` 的值修改成开发板的串口号，比如在windows系统下，查看到开发板对应的串口号是```com3```，则修改后的 ```DOWNLOAD_PORT := com3```

#### 烧录固件
成功设置串口号后，可使用如下指令烧录固件到芯片中：
烧录指令：

    make flash

#### 烧录常见错误
----------------------------------------------
    Telink_Tools.py v0.3 dev 
    Open /dev/ttyUSB0 ... ... Fail!
若出现以上错误，请检查串口号是否设置正确，串口是否被占用。

----------------------------------------------

### 运行固件

按下开发板上的RST键可复位开发板，开始运行刚烧录的固件。

如果你使用单模块进行开发或者想要打开串口，可使用```make monitor```指令。

### 其他指令：

    make erase_fw//擦除固件
    make erase_all//擦除整片Flash(除boot外)

## 其他资料

[API 参考手册](https://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tb/Telink%20Kite%20BLE%20SDK%20Developer%20Handbook.pdf)
