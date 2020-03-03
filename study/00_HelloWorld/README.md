# 你的第一个程序

本文主要通过8258芯片串口打印功能的实现，用来测试你的开发环境是否完整，同时介绍下编译及调试步骤。

## 编译
在此目录下执行```make```指令，将开始执行编译，编译成功将出现类似如下信息，如果编译失败，请检查开发环境是否搭建正确，参考搭建开发环境章节的内容分。


    Invoking: Print Size
    tc32-elf-size -t ./out/iBeacon.elf
    text    data     bss     dec     hex filename
    11000    1020     520   12540    30fc ./out/iBeacon.elf
    11000    1020     520   12540    30fc (TOTALS)
    Finished building: sizedummy
## 烧录

将开发板连接到计算机，修改此目录下的makefile文件，将```DOWNLOAD_PORT```的值修改为开发板对应的的串口号。

windows操作系统可在设备管理器中查看串口号，linux操作系统可通过```ls /dev/ttyUSB* ```查看串口号.

例如在windows下查看到开发板对应的串口号是 ```com3```，则修改后的makefile为 ```DOWNLOAD_PORT := com3```

修改好串口号后，执行```make flash ```指令, 即可将编译好的固件烧录到开发板。

    ospanic@ospanic:~/ESP/Telink_825X_SDK/study/00_HelloWorld$ make flash
    python3 ../../make/Telink_Tools.py -p /dev/ttyUSB0 burn ./out/iBeacon.bin
    Telink_Tools.py v0.3 dev
    Open /dev/ttyUSB0 ... ... Success!
    Connect Board ... ... Success!
    Try to change Baud to 921600 ... ... OK!
    Erase Flash at 0x4000 len 176 KB ... ... OK!
    Burn Firmware :./out/iBeacon.bin
    100% [>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>]

如果烧录失败，请检查串口号是否设置正确，串口是否被占用，python运行环境是否已安装。
## 运行

执行 ```make monitor```指令，将复位开发板，并打开串口显示开发板发出的数据：

    ospanic@ospanic:~/ESP/Telink_825X_SDK/study/00_HelloWorld$ make monitor
    python3 ../../make/monitor.py --port /dev/ttyUSB0 --baud 115200 ./out/iBeacon.elf
    --- idf_monitor on /dev/ttyUSB0 115200 ---
    --- Quit: Ctrl+] | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---
    ▒
    Ai-Thinker Ble Demo
    Hello World
    Hello World
    Hello World
    Hello World
    Hello World

使用组合键``` Ctrl + ]```可退出 monitor 。