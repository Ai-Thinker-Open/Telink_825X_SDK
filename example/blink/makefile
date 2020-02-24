#########################################################################################
# As a new project, you should  the following variables		

# First, select the chip type, the chip type can be:					
#			TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8261
#			TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8266
#			TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8267
#			TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8269
#
# Second, select the link lib, the available link lib file in Telink SDK proj_lib folder
#
# Third, Set Telink SDK path to  variable TEL_PATH
# 
# Forth, Set Project Name
#########################################################################################

# Select Chip  
TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8258

# Select link lib
LIBS :=  -llt_8258

##########################################################################################
#Set SDK Path , if you copy this project other dir, please set TEL_PATH absolute path
#设置 SDK Path，目前使用的是相对路径，如果你将此工程复制到别的文件夹，请将TEL_PATH设为绝对路径
# 比如： TEL_PATH := /home/ospanic/Telink_825X_SDK
#########################################################################################
TEL_PATH := ../..

# Set Project Name
PROJECT_NAME := blink

# 设置下载固件的串口号
DOWNLOAD_PORT := /dev/ttyUSB0

#########################################################################################
# The following content need not be modified by the user!!!!!							#
#########################################################################################
PROJECT_PATH := .
OUT_PATH :=$(PROJECT_PATH)/out

ifneq ($(TEL_PATH)/make/makefile, $(wildcard $(TEL_PATH)/make/makefile))
$(error "Please check if the TEL_PATH is correct, 请检查 TEL_PATH 是否设置正确！！！")
endif

#include the SDK makefile
-include $(TEL_PATH)/make/makefile