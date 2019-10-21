################################################################################
# Automatically-generated file. Do not edit!
################################################################################

OBJS += \
$(OUT_PATH)/app.o \
$(OUT_PATH)/app_att.o \
$(OUT_PATH)/main.o


# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/%.o: ./%.c
	@echo 'Building file: $<'
	@tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"