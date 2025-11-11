################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../SHT3x/SHT3x.c 

OBJS += \
./SHT3x/SHT3x.o 

C_DEPS += \
./SHT3x/SHT3x.d 


# Each subdirectory must supply rules for building sources it contributes
SHT3x/%.o SHT3x/%.su SHT3x/%.cyclo: ../SHT3x/%.c SHT3x/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32WB55xx -c -I../Core/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../Drivers/CMSIS/Include -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/TSL2591" -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/INA3221" -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/SEN0308" -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/SHT3x" -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/DS18B20" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-SHT3x

clean-SHT3x:
	-$(RM) ./SHT3x/SHT3x.cyclo ./SHT3x/SHT3x.d ./SHT3x/SHT3x.o ./SHT3x/SHT3x.su

.PHONY: clean-SHT3x

