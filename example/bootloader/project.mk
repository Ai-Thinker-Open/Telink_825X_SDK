################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
OBJS += \
$(OUT_PATH)/app_uart.o \
$(OUT_PATH)/app_flash.o \
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


################################################################################
# Automatically-generated file. Do not edit!
################################################################################
OUT_DIR += /application/print

OBJS += \
$(OUT_PATH)/application/print/putchar.o \
$(OUT_PATH)/application/print/u_printf.o

# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/application/%.o: $(TEL_PATH)/components/application/%.c 
	@echo 'Building file: $<'
	@tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"

################################################################################
# Automatically-generated file. Do not edit!
################################################################################
OUT_DIR += /drivers/8258

OBJS += \
$(OUT_PATH)/drivers/8258/adc.o \
$(OUT_PATH)/drivers/8258/analog.o \
$(OUT_PATH)/drivers/8258/clock.o \
$(OUT_PATH)/drivers/8258/flash.o \
$(OUT_PATH)/drivers/8258/gpio_8258.o \
$(OUT_PATH)/drivers/8258/timer.o \
$(OUT_PATH)/drivers/8258/uart.o 

# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/drivers/8258/%.o: $(TEL_PATH)/components/drivers/8258/%.c
	@echo 'Building file: $<'
	@tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"