/*
 * TSL2591.c
 *
 *  Created on: Oct 8, 2025
 *      Author: pzaragoza
 */

#include "tsl2591.h"

#include <math.h>

static HAL_StatusTypeDef writeRegister(TSL2591_t *dev, uint8_t reg, uint8_t value) {
    return HAL_I2C_Mem_Write(dev->hi2c,
                             TSL2591_ADDR,
                             TSL2591_CMD_BIT | reg,
                             I2C_MEMADD_SIZE_8BIT,
                             &value,
                             1,
                             HAL_MAX_DELAY);
}

static HAL_StatusTypeDef readRegister(TSL2591_t *dev, uint8_t reg, uint8_t *data, uint8_t len) {
    return HAL_I2C_Mem_Read(dev->hi2c,
                            TSL2591_ADDR,
                            TSL2591_CMD_BIT | reg,
                            I2C_MEMADD_SIZE_8BIT,
                            data,
                            len,
                            HAL_MAX_DELAY);
}

HAL_StatusTypeDef TSL2591_Init(TSL2591_t *dev) {
    HAL_StatusTypeDef ret;

    // Encender el sensor
    TSL2591_Enable(dev);
    HAL_Delay(10);

    // Configurar ganancia e integración
    uint8_t ctrl = dev->integrationTime | dev->gain;
    ret = writeRegister(dev, TSL2591_CONTROL, ctrl);
    HAL_Delay(100);

    return ret;
}

void TSL2591_Enable(TSL2591_t *dev) {
    uint8_t val = TSL2591_ENABLE_PON | TSL2591_ENABLE_AEN;
    writeRegister(dev, TSL2591_ENABLE, val);
}

void TSL2591_Disable(TSL2591_t *dev) {
    uint8_t val = 0x00;
    writeRegister(dev, TSL2591_ENABLE, val);
}

HAL_StatusTypeDef TSL2591_ReadChannels(TSL2591_t *dev, uint16_t *ch0, uint16_t *ch1) {
    uint8_t buf[4];
    HAL_StatusTypeDef ret;

    ret = readRegister(dev, TSL2591_CHAN0_LOW, buf, 4);
    if (ret != HAL_OK) return ret;

    *ch0 = (buf[1] << 8) | buf[0]; // FULL spectrum (CH0)
    *ch1 = (buf[3] << 8) | buf[2]; // IR only (CH1)

    return HAL_OK;
}

float TSL2591_CalculateLux(TSL2591_t *dev, uint16_t full, uint16_t ir) {
	if ((full == 0xFFFF) || (ir == 0xFFFF)) return -1.0f;  // Overflow

    float atime_ms, again;
	switch (dev->integrationTime) {
		case TSL2591_INTEGRATION_100MS: atime_ms = 100.0f; break;
		case TSL2591_INTEGRATION_200MS: atime_ms = 200.0f; break;
		case TSL2591_INTEGRATION_300MS: atime_ms = 300.0f; break;
		case TSL2591_INTEGRATION_400MS: atime_ms = 400.0f; break;
		case TSL2591_INTEGRATION_500MS: atime_ms = 500.0f; break;
		case TSL2591_INTEGRATION_600MS: atime_ms = 600.0f; break;
		default: atime_ms = 100.0f; break;
	}

	switch (dev->gain) {
		case TSL2591_GAIN_LOW:  again = 1.0f; break;
		case TSL2591_GAIN_MED:  again = 25.0f; break;
		case TSL2591_GAIN_HIGH: again = 428.0f; break;
		case TSL2591_GAIN_MAX:  again = 9876.0f; break;
		default: again = 1.0f; break;
	}

    float cpl = (atime_ms * again) / TSL2591_LUX_DF;
    if (cpl <= 0.0f || full == 0) return 0.0f;

    float lux_raw = ((float)(full - ir)) * (1.0f - ((float)ir / (float)full)) / cpl;
    if (lux_raw < 0.0f) lux_raw = 0.0f;

    float lux_cal = (TSL2591_CALIB_A * lux_raw) + TSL2591_CALIB_B;
    if (lux_cal < 0.0f) lux_cal = 0.0f;

    return lux_cal;
}

float TSL2591_CalculateIrradiance(float lux) {
	return lux / TSL2591_LUM_EFF;
}

