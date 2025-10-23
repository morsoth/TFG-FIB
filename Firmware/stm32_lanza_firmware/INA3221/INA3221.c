/*
 * INA3221.c
 *
 *  Created on: Oct 8, 2025
 *      Author: pzaragoza
 */

#include "INA3221.h"

static HAL_StatusTypeDef writeRegister(INA3221_t *dev, uint8_t reg, uint16_t value) {
    uint8_t data[2];
    data[0] = (value >> 8) & 0xFF;
    data[1] = value & 0xFF;

    return HAL_I2C_Mem_Write(dev->hi2c,
                             INA3221_ADDRESS,
                             reg,
                             I2C_MEMADD_SIZE_8BIT,
                             data,
                             2,
                             HAL_MAX_DELAY);
}

static HAL_StatusTypeDef readRegister(INA3221_t *dev, uint8_t reg, uint8_t *value) {
    return HAL_I2C_Mem_Read(dev->hi2c,
                            INA3221_ADDRESS,
                            reg,
                            I2C_MEMADD_SIZE_8BIT,
                            value,
                            2,
                            HAL_MAX_DELAY);
}

HAL_StatusTypeDef INA3221_Init(INA3221_t *dev) {
	uint16_t config = (1<<14) | (1<<13) | (1<<12) | (dev->averagingMode << 9) | (dev->convTimeBus << 6) | (dev->convTimeShunt << 3) | (dev->operatingMode);

    return writeRegister(dev, INA3221_REG_CONFIG, config);
}

HAL_StatusTypeDef INA3221_ReadVoltage(INA3221_t *dev, uint8_t channel, float *busVoltage, float *shuntVoltage) {
    if (channel < 1 || channel > 3) return HAL_ERROR;

    uint8_t buf1[2], buf2[2];
    HAL_StatusTypeDef status;

    uint16_t rawShunt, rawBus;

    status = readRegister(dev, INA3221_REG_SHUNT_VOLTAGE_1 + (channel - 1) * 2, buf1);
	if (status != HAL_OK) return status;

	status = readRegister(dev, INA3221_REG_BUS_VOLTAGE_1 + (channel - 1) * 2, buf2);
	if (status != HAL_OK) return status;

    rawShunt = ((uint16_t)buf1[0] << 8) | buf1[1];
    rawBus = ((uint16_t)buf2[0] << 8) | buf2[1];

    *shuntVoltage = ((float)((int16_t)rawShunt >> 3)) * INA3221_SHUNT_VOLTAGE_LSB;
    *busVoltage   = ((float)(rawBus   >> 3)) * INA3221_BUS_VOLTAGE_LSB;

    return HAL_OK;
}

float INA3221_CalculateCurrent_mA(INA3221_t *dev, uint8_t channel, float shuntVoltage) {
    return (shuntVoltage / dev->shuntResistance[channel - 1]) * 1000.0f;
}


float INA3221_CalculatePower_mW(float busVoltage, float shuntCurrent_mA) {
	return busVoltage * shuntCurrent_mA;
}
