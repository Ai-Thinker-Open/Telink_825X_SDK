################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
OBJS += \
$(OUT_PATH)/app_uart.o \
$(OUT_PATH)/main.o

# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/%.o: ./%.c
	@echo 'Building file: $<'
	@tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"



OBJS += $(OUT_PATH)/bootloader.o  $(OUT_PATH)/div_mod.o  

# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/%.o: ./%.S
	@echo 'Building file: $<'
	@tc32-elf-gcc -DMCU_BOOT_8258_RET_16K -c -o"$@" "$<"
