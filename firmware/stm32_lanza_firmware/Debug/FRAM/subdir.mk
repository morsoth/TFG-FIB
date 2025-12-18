################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FRAM/MB85RS256B.c \
../FRAM/fram.c 

OBJS += \
./FRAM/MB85RS256B.o \
./FRAM/fram.o 

C_DEPS += \
./FRAM/MB85RS256B.d \
./FRAM/fram.d 


# Each subdirectory must supply rules for building sources it contributes
FRAM/%.o FRAM/%.su FRAM/%.cyclo: ../FRAM/%.c FRAM/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32WB55xx -c -I../Core/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc -I../Drivers/STM32WBxx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32WBxx/Include -I../Drivers/CMSIS/Include -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/TSL2591" -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/INA3221" -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/SEN0308" -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/SHT3x" -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/DS18B20" -I"C:/Users/pzaragoza/Documents/GitHub/TFG-FIB/Firmware/stm32_lanza_firmware/FRAM" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-FRAM

clean-FRAM:
	-$(RM) ./FRAM/MB85RS256B.cyclo ./FRAM/MB85RS256B.d ./FRAM/MB85RS256B.o ./FRAM/MB85RS256B.su ./FRAM/fram.cyclo ./FRAM/fram.d ./FRAM/fram.o ./FRAM/fram.su

.PHONY: clean-FRAM

