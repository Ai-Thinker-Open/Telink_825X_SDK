################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add outputs from these tool invocations to the build variables 

OBJS += \
$(OUT_PATH)/app_att.o \
$(OUT_PATH)/app.o \
$(OUT_PATH)/app_phytest.o \
$(OUT_PATH)/app_ui.o \
$(OUT_PATH)/battery_check.o \
$(OUT_PATH)/main.o \
$(OUT_PATH)/rc_ir.0


# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/%.o: ./%.c
	@echo 'Building file: $<'
	@tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"