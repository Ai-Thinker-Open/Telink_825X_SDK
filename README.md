# Telink TLSR825X Software Development Kit
--------------------------------------------------
Telink 泰凌 TLSR825X 蓝牙芯片软件开发套件

# 使用方法
---------------------------------------------------

### 获取TC32编译工具链(文档仅适用于Linux 64Bit系统)
>目前仅测试了linux系统，Mac OS及Windows系统仅提供了编译工具变，需自行设置环境变量

>Mac OS版本工具链下载地址 ```http://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tc32-mac.zip```

>Windows 版本工具链下载地址 ```http://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tc32_win.rar```

linux版本获取编译工具链

    wget http://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tc32_gcc_v2.0.tar.bz2

解压到opt文件夹 *(也可解压到其他文件夹)*

    sudo tar -xvjf　tc32_gcc_v2.0.tar.bz2　-C /opt/

添加工具链到环境变量(以解压到/opt为例)

    export PATH=$PATH:/opt/tc32/bin

测试是否搭建成功

    tc32-elf-gcc -v

如果搭建成功将打印如下信息:

    Using built-in specs.
    COLLECT_GCC=tc32-elf-gcc
    COLLECT_LTO_WRAPPER=/opt/tc32/lib/gcc/tc32-elf/4.5.1.tc32-elf-1.5/lto-wrapper
    Target: tc32-elf
    Configured with: ../../gcc-4.5.1/configure --program-prefix=tc32-elf- --target=tc32-elf --prefix=/opt/tc32 --enable-languages=c --libexecdir=/opt/tc32/lib --with-gnu-as --with-gnu-ld --without-headers --disable-decimal-float --disable-nls --disable-mathvec --with-pkgversion='Telink TC32 version 2.0 build' --without-docdir --without-fp --without-tls --disable-shared --disable-threads --disable-libffi --disable-libquadmath --disable-libstdcxx-pch --disable-libmudflap --disable-libgomp --disable-libssp -v --without-docdir --enable-soft-float --with-newlib --with-gcc --with-gnu- --with-gmp=/opt/tc32/addontools --with-mpc=/opt/tc32/addontools --with-mpfr=/opt/tc32/addontools
    Thread model: single
    gcc version 4.5.1.tc32-elf-1.5 (Telink TC32 version 2.0 build) 

### 安装Python3及其依赖
烧录工具采用python语言编写，所以须确保你的电脑安装了python3运行环境及所需依赖包pyserial

python3请自行安装，一下指令用于安装prserial

    pip install pyserial

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

安信可自主开发了串口烧录工具，无需官方烧录器即可使用，前提是要先将安信可bootloader烧录到模块中。

串口烧录接线方式如下：

|串口|模块|
|----|---|
|VCC|3V3|
|GND|GND|
|TX|RX|
RX|TX|
|RTS|RST|
|DTR|SWS|

注意：SWS为boot选择引脚，为低电平进入下载模式，为高电平进入运行模式

烧录指令：

    make flash
其他指令：

    make erase_fw//擦除固件
    make monitor //打开串口监控