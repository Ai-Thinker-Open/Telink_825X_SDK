# windows平台开发环境搭建

windows平台下需要安装git，python，已安装了的无需重复安装。

## 安装git
从如下地址下载最新版git软件：https://git-scm.com/download/win
，下载完成后双击安装即可，安装过程中记得勾选 ```Git Bash Here``` 及 ```Git GUI Here``` 功能。

安装成功后，在桌面点击右键，如果出现如下图的 ```Git Bash Here``` 及 ```Git GUI Here``` 选项说明git安装成功。

![](https://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tb/git_ok.png)

接下来的很多操作都是在```Git Bash Here```中完成的，开发者需熟悉基本的命令，如：```ls```,```cd```,```mkdie```,```echo ```

另外，对于未接触过```git```的开发者，建议学习下```git```的使用。

## 安装Python3

在左面点击鼠标右键，然后点击```Git Bash Here```选项，进入命令行，输入 ```python -V``` 按回车键，如果出现类似如下现象，说明你已经安装过python，无需重复安装。如果提示```bash: python: command not found```,说明的的电脑未安装python，需按照下文的指导安装python。

![](https://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tb/python_ok.png)

如果你的电脑未安装，请从如下地址下载最新版本的python(建议下载3.x.x版本)，https://www.python.org/downloads/windows/
，下载完成后双击安装。

安装完成后按照上面的方法，在命令行输入```python -V```验证下是否安装成功，然后再在命令行输入 ```pip install pyserial```,安装pyserial 模块。

## 下载编译器

从如下地址下载泰凌微的编译器：

    http://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tc32_win.rar

下载完成后解压出来，建议解压到 ```D:\TB``` 文件夹，下面设置环境变量的操作将以此文件夹为示例。

## 设置环境变量
在```我的电脑``` 点击右键 依次进入 -> ```属性``` -> ```高级系统设置``` -> ```环境变量``` -> ```系统变量```,找到```Path```变量，点击```编辑```,将编译器的路径添加到环境变量里面。

以解压到```D:\TB```为例，win10操作页面如下图，具体操作是```新建```一个值，输入```D:\TB\tc32_win\bin```,并通过   ```上移```按钮将其移动到最上方：

![](https://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tb/path_win10.png)

win7操作页面如下,具体操作是在```变量值```输入框的最前面添加```D:\TB\tc32_win\bin;```：

![](https://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tb/path_win7.png)

注意：以上操作都是以编译器解压到```D:\TB```为示例，实际操作中可根据你要解压的路径做相应的更改。路径中不能带有中文。

## 测试编译器

设置好环境变量后，在任意位置点击鼠标左键，进入 ```Git Bash Here```，在打开的命令行中输入 ```tc32-elf-gcc -v```，如果出现如下信息，说明编译器及环境变量设置成功。

![](https://shyboy.oss-cn-shenzhen.aliyuncs.com/readonly/tb/gcc_ok.png)