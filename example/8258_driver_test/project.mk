################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add outputs from these tool invocations to the build variables 

OBJS += \
$(OUT_PATH)/app_adc.o \
$(OUT_PATH)/app.o \
$(OUT_PATH)/app_emi.o \
$(OUT_PATH)/app_gpio_irq.o \
$(OUT_PATH)/app_i2c.o \
$(OUT_PATH)/app_i2c_master.o \
$(OUT_PATH)/app_i2c_slave.o \
$(OUT_PATH)/app_pwm.o \
$(OUT_PATH)/app_spi.o \
$(OUT_PATH)/app_spi_master.o \
$(OUT_PATH)/app_spi_slave.o \
$(OUT_PATH)/app_timer.o \
$(OUT_PATH)/app_uart.o \
$(OUT_PATH)/main.o \
$(OUT_PATH)/test_low_power.o



# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/%.o: ./%.c
	@echo 'Building file: $<'
	@tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"