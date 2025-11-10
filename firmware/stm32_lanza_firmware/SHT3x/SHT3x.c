/*
 * SHT3x.c
 *
 *  Created on: Nov 10, 2025
 *      Author: pzaragoza
 */

#include "SHT3x.h"

static uint8_t calculateCRC(const uint8_t *data, int len) {
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static uint16_t singleShotCMD(SHT3X_t *dev) {
    if (dev->clockStretch == SHT3X_STRETCH) {
        switch (dev->repeatability) {
            case SHT3X_REPEAT_HIGH: return SHT3X_CMD_SS_CS_HIGH;
            case SHT3X_REPEAT_MED:  return SHT3X_CMD_SS_CS_MED;
            default:                return SHT3X_CMD_SS_CS_LOW;
        }
    } else {
        switch (dev->repeatability) {
            case SHT3X_REPEAT_HIGH: return SHT3X_CMD_SS_NCS_HIGH;
            case SHT3X_REPEAT_MED:  return SHT3X_CMD_SS_NCS_MED;
            default:                return SHT3X_CMD_SS_NCS_LOW;
        }
    }
}

static HAL_StatusTypeDef sendCommand(SHT3X_t *dev, uint16_t cmd) {
    uint8_t tx[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };

    return HAL_I2C_Master_Transmit(dev->hi2c, SHT3X_I2C_ADDR, tx, 2, HAL_MAX_DELAY);
}

static HAL_StatusTypeDef readRegister(SHT3X_t *dev, uint8_t *data, uint16_t size) {
    return HAL_I2C_Master_Receive(dev->hi2c, SHT3X_I2C_ADDR, data, size, HAL_MAX_DELAY);
}

static uint32_t convTime_ms(SHT3X_t *dev) {
    switch (dev->repeatability) {
        case SHT3X_REPEAT_HIGH: return 20; // ~15ms
        case SHT3X_REPEAT_MED:  return 10; // ~6ms
        default:                return 6;  // ~4ms
    }
}

HAL_StatusTypeDef SHT3X_Init(SHT3X_t *dev) {
    HAL_StatusTypeDef ret;

    // Soft reset
	ret = SHT3X_SoftReset(dev);
    HAL_Delay(5);

    return ret;
}

HAL_StatusTypeDef SHT3X_SoftReset(SHT3X_t *dev) {
	return sendCommand(dev, SHT3X_CMD_SOFT_RESET);
}

HAL_StatusTypeDef SHT3X_ReadRaw(SHT3X_t *dev, uint16_t *rawT, uint16_t *rawRH) {
    HAL_StatusTypeDef ret;
    uint16_t cmd = singleShotCMD(dev);

    // Lanzar medición
    ret = sendCommand(dev, cmd);
    if (ret != HAL_OK) return ret;

    // Si no usamos clock stretching, esperamos conversión
    if (dev->clockStretch == SHT3X_NOSTRETCH) {
        HAL_Delay(convTime_ms(dev));
    }

    // Leer 6 bytes: T[2]+CRC, RH[2]+CRC
    uint8_t buf[6];
    ret = readRegister(dev, buf, sizeof(buf));
    if (ret != HAL_OK) return ret;

    // Verificar CRC
    if (calculateCRC(buf, 2) != buf[2]) return HAL_ERROR;
    if (calculateCRC(buf + 3, 2) != buf[5]) return HAL_ERROR;

    *rawT  = (uint16_t)((buf[0] << 8) | buf[1]);
    *rawRH = (uint16_t)((buf[3] << 8) | buf[4]);

    return HAL_OK;
}

HAL_StatusTypeDef SHT3X_ReadSingleShot(SHT3X_t *dev, float *temp_c, float *rh_perc) {
    HAL_StatusTypeDef ret;
    uint16_t rawT, rawRH;

    ret = SHT3X_ReadRaw(dev, &rawT, &rawRH);
    if (ret != HAL_OK) return ret;

    // Conversión
    *temp_c  = -45.0f + 175.0f * ((float)rawT  / 65535.0f);
    *rh_perc = 100.0f * ((float)rawRH / 65535.0f);

    // Limitar RH a [0,100]
    if (*rh_perc < 0.0f)   *rh_perc = 0.0f;
    if (*rh_perc > 100.0f) *rh_perc = 100.0f;
    return HAL_OK;
}

float SHT3X_CalculateDewpoint_Simple(float temp_c, float rh_perc) {
	if (rh_perc < 1.0f) rh_perc = 1.0f; // Evita log(0)
	if (rh_perc > 100.0f) rh_perc = 100.0f;

	float a = 17.625f, b = 243.04f;
	float gamma = logf(rh_perc / 100.0f) + (a * temp_c) / (b + temp_c);

	return (b * gamma) / (a - gamma);
}

float SHT3X_CalculateDewpoint(float temp_c, float rh_perc) {
	if (rh_perc < 1.0f) rh_perc = 1.0f; // Evita log(0)
	if (rh_perc > 100.0f) rh_perc = 100.0f;

	float es = (temp_c >= 0.0f) ?
				6.1121f * expf((18.678f - temp_c/234.5f) * (temp_c / (257.14f + temp_c))) :
				6.1115f * expf((23.036f - temp_c/333.7f) * (temp_c / (279.82f + temp_c)));
	float e  = (rh_perc / 100.0f) * es;

	if (temp_c >= 0.0f) {
		float x = logf(e / 6.1121f);
		return (257.14f * x) / (18.678f - x);
	}
	else {
		float x = logf(e / 6.1115f);
		return (279.82f * x) / (23.036f - x);
	}
}

HAL_StatusTypeDef SHT3X_ReadStatus(SHT3X_t *dev, uint16_t *status) {
    if (!status) return HAL_ERROR;

    uint8_t buf[3] = {0};
    HAL_StatusTypeDef ret = sendCommand(dev, SHT3X_CMD_STATUS_READ);
    if (ret != HAL_OK) return ret;

    ret = readRegister(dev, buf, sizeof(buf));
    if (ret != HAL_OK) return ret;

    if (calculateCRC(buf, 2) != buf[2]) return HAL_ERROR;
    *status = (uint16_t)((buf[0] << 8) | buf[1]);

    return HAL_OK;
}

HAL_StatusTypeDef SHT3X_ClearStatus(SHT3X_t *dev) {
    return sendCommand(dev, SHT3X_CMD_STATUS_CLEAR);
}

HAL_StatusTypeDef SHT3X_Heater(SHT3X_t *dev, SHT3X_Heater_t state) {
    return sendCommand(dev, (state == SHT3X_HEATER_ON) ? SHT3X_CMD_HEATER_ENABLE : SHT3X_CMD_HEATER_DISABLE);
}
