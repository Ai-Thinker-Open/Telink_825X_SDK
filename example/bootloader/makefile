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
TEL_CHIP := -DCHIP_TYPE=CHIP_TYPE_8258  -D_MODULE_TB_02_

# Select link lib
LIBS :=  -llt_8258

##########################################################################################
#Set SDK Path , if you copy this project other dir, please set TEL_PATH absolute path
#设置 SDK Path，目前使用的是相对路径，如果你将此工程复制到别的文件夹，请将TEL_PATH设为绝对路径
# 比如： TEL_PATH := /home/ospanic/Telink_825X_SDK
#########################################################################################
TEL_PATH := ../..

# Set Project Name
PROJECT_NAME := bootloader


#########################################################################################
# The following content need not be modified by the user!!!!!							#
#########################################################################################
PROJECT_PATH := .
OUT_PATH :=$(PROJECT_PATH)/out


GCC_FLAGS := \
-ffunction-sections \
-fdata-sections \
-Wall \
-O2 \
-fpack-struct \
-fshort-enums \
-finline-small-functions \
-std=gnu99 \
-fshort-wchar \
-fms-extensions 

INCLUDE_PATHS := -I$(TEL_PATH)/components -I$(PROJECT_PATH)

GCC_FLAGS += $(TEL_CHIP)

#include Project makefile
-include $(PROJECT_PATH)/project.mk

# Add inputs and outputs from these tool invocations to the build variables 
LST_FILE := $(OUT_PATH)/$(PROJECT_NAME).lst
BIN_FILE := $(OUT_PATH)/$(PROJECT_NAME).bin
ELF_FILE := $(OUT_PATH)/$(PROJECT_NAME).elf

SIZEDUMMY := sizedummy

# All Target
all: pre-build main-build

# Main-build Target
main-build: $(ELF_FILE) secondary-outputs

# Tool invocations
$(ELF_FILE): $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@tc32-elf-ld --gc-sections -L $(TEL_PATH)/components/proj_lib -T ./bootloader.link -o $(ELF_FILE) $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

$(LST_FILE): $(ELF_FILE)
	@echo 'Invoking: TC32 Create Extended Listing'
	tc32-elf-objdump -x -D -l -S  $(ELF_FILE)  > $(LST_FILE)
	@echo 'Finished building: $@'
	@echo ' '

$(BIN_FILE): $(ELF_FILE)
	@echo 'Create Flash image (binary format)'
	tc32-elf-objcopy -v -O binary $(ELF_FILE)  $(BIN_FILE)
	@echo 'Finished building: $@'
	@echo ' '

sizedummy: $(ELF_FILE)
	@echo 'Invoking: Print Size'
	tc32-elf-size -t $(ELF_FILE)
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(FLASH_IMAGE)$(ELFS)$(OBJS)$(LST)$(SIZEDUMMY) $(ELF_FILE) $(BIN_FILE) $(LST_FILE)
	-@echo ' '

pre-build:
	mkdir -p $(foreach s,$(OUT_DIR),$(OUT_PATH)$(s))
#-"D:\tel\ble_sdk_release/getver.sh"
	-@echo ' '


secondary-outputs: $(BIN_FILE) $(LST_FILE) $(SIZEDUMMY)

.PHONY: all clean dependents pre-build 
.SECONDARY: main-build 