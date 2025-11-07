################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../INA3221/INA3221.c 

OBJS += \
./INA3221/INA3221.o 

C_DEPS += \
./INA3221/INA3221.d 


# Each subdirectory must supply rules for building sources it contributes
INA3221/%.o INA3221/%.su INA3221/%.cyclo: ../INA3221/%.c INA3221/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32WB55xx -c -I../Core/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../Drivers/CMSIS/Include -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/TSL2591" -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/INA3221" -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/SEN0308" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-INA3221

clean-INA3221:
	-$(RM) ./INA3221/INA3221.cyclo ./INA3221/INA3221.d ./INA3221/INA3221.o ./INA3221/INA3221.su

.PHONY: clean-INA3221

