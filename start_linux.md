# Linux 平台开发环境搭建

<label style="color:red">注意：Linux平台编译工具仅支持64Bit Linux操作系统</label>

## linux版本获取编译工具链

    wget http://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tc32_gcc_v2.0.tar.bz2

解压到opt文件夹 *(也可解压到其他文件夹)*

    sudo tar -xvjf　tc32_gcc_v2.0.tar.bz2　-C /opt/

添加工具链到环境变量(以解压到/opt为例)

    export PATH=$PATH:/opt/tc32/bin

> 以上命令只在当前命令行生效，下次打开命令行再次复执行该命令，最好的办法是将以上命令添加到```~/.bashrc```文件的尾部，这样就不用每次都执行此命令了。

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

## 安装Python及其依赖
烧录工具采用python语言编写，所以须确保你的电脑安装了python运行环境及所需依赖包pyserial

查看你的电脑是否安装了Python

    python -v

如果能输出python版本号，说明你的电脑已安装python，如果未安装，请自行安装。

如果已正确安装Python，还需使用以下指令安装pyserial

    pip install pyserial