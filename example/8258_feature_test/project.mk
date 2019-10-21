################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
./app.c \
./main.c 

OBJS += \
$(OUT_PATH)/app.o \
$(OUT_PATH)/app_att.o \
$(OUT_PATH)/feature_2m_coded_phy_adv.o \
$(OUT_PATH)/feature_adv_power.o \
$(OUT_PATH)/feature_ext_adv.o \
$(OUT_PATH)/feature_ll_state.o \
$(OUT_PATH)/feature_phytest.o \
$(OUT_PATH)/feature_slave_dle.o \
$(OUT_PATH)/feature_whitelist.o \
$(OUT_PATH)/feature_2m_coded_phy_conn.o \
$(OUT_PATH)/feature_csa2.o \
$(OUT_PATH)/feature_gatt_security.o \
$(OUT_PATH)/feature_master_dle.o \
$(OUT_PATH)/feature_security.o \
$(OUT_PATH)/feature_soft_timer.o \
$(OUT_PATH)/main.o


# Each subdirectory must supply rules for building sources it contributes
$(OUT_PATH)/%.o: ./%.c
	@echo 'Building file: $<'
	@tc32-elf-gcc $(GCC_FLAGS) $(INCLUDE_PATHS) -c -o"$@" "$<"