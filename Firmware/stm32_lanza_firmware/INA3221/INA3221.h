/*
 * INA3221.h
 *
 *  Created on: Oct 8, 2025
 *      Author: pzaragoza
 */

#include "stm32wbxx_hal.h"

#ifndef INA3221_H_
#define INA3221_H_

#define INA3221_ADDRESS             (0x40 << 1) // I2C address

// Registers
#define INA3221_REG_CONFIG          0x00 // Configuration register
#define INA3221_REG_SHUNT_VOLTAGE_1 0x01 // Channel 1 shunt voltage register
#define INA3221_REG_BUS_VOLTAGE_1   0x02 // Channel 1 bus voltage register
#define INA3221_REG_SHUNT_VOLTAGE_2 0x03 // Channel 2 shunt voltage register
#define INA3221_REG_BUS_VOLTAGE_2   0x04 // Channel 2 bus voltage register
#define INA3221_REG_SHUNT_VOLTAGE_3 0x05 // Channel 3 shunt voltage register
#define INA3221_REG_BUS_VOLTAGE_3   0x06 // Channel 3 bus voltage register

// Constants
#define INA3221_SHUNT_VOLTAGE_LSB   40e-6f // Shunt voltage LSB (40 µV/bit)
#define INA3221_BUS_VOLTAGE_LSB     0.008f // Bus voltage LSB (8 mV/bit)

// Averaging mode
typedef enum {
    INA3221_AVG_1    = 0b000, // 1
    INA3221_AVG_4    = 0b001, // 4
    INA3221_AVG_16   = 0b010, // 16
    INA3221_AVG_64   = 0b011, // 64
    INA3221_AVG_128  = 0b100, // 128
    INA3221_AVG_256  = 0b101, // 256
    INA3221_AVG_512  = 0b110, // 512
    INA3221_AVG_1024 = 0b111  // 1024
} INA3221_AveragingMode_t;

// Conversion time
typedef enum {
    INA3221_CT_140us   = 0b000, // 140 us
    INA3221_CT_204us   = 0b001, // 204 us
    INA3221_CT_332us   = 0b010, // 332 us
    INA3221_CT_588us   = 0b011, // 588 us
    INA3221_CT_1100us  = 0b100, // 1100 us
    INA3221_CT_2116us  = 0b101, // 2116 us
    INA3221_CT_4156us  = 0b110, // 4156 us
    INA3221_CT_8244us  = 0b111  // 8244 us
} INA3221_ConversionTime_t;

// Operating mode
typedef enum {
    INA3221_MODE_POWER_DOWN           = 0b000, // Power-down
    INA3221_MODE_SHUNT_TRIGGERED      = 0b001, // Shunt voltage, single shot (triggered)
    INA3221_MODE_BUS_TRIGGERED        = 0b010, // Bus voltage, single shot (triggered)
    INA3221_MODE_SHUNT_BUS_TRIGGERED  = 0b011, // Shunt and bus voltage, single shot (triggered)
    INA3221_MODE_SHUNT_CONTINUOUS     = 0b101, // Shunt voltage, continuous
    INA3221_MODE_BUS_CONTINUOUS       = 0b110, // Bus voltage, continuous
    INA3221_MODE_SHUNT_BUS_CONTINUOUS = 0b111  // Shunt and voltage, continuous
} INA3221_OperatingMode_t;

// Struct
typedef struct {
    I2C_HandleTypeDef *hi2c;
    float shuntResistance[3]; // Shunt resistance values
    INA3221_AveragingMode_t averagingMode; // Averaging mode
    INA3221_ConversionTime_t convTimeShunt; // Shunt voltage conversion time
    INA3221_ConversionTime_t convTimeBus; // Bus voltage conversion time
    INA3221_OperatingMode_t operatingMode; // Operating mode
} INA3221_t;

// Functions
HAL_StatusTypeDef INA3221_Init(INA3221_t *dev);

HAL_StatusTypeDef INA3221_ReadVoltage(INA3221_t *dev, uint8_t channel, float *busVoltage, float *shuntVoltage);
float INA3221_CalculateCurrent_mA(INA3221_t *dev, uint8_t channel, float shuntVoltage);
float INA3221_CalculatePower_mW(float busVoltage, float shuntCurrent_mA);


#endif /* INA3221_H_ */
