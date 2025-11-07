/*
 * SEN0308.h
 *
 *  Created on: Nov 7, 2025
 *      Author: pzaragoza
 */

#ifndef SEN0308_H_
#define SEN0308_H_

#include "stm32wbxx_hal.h"

#define SEN0308_ADC_MAX			4095

#define SEN0308_POLL_TIMEOUT_MS	10

typedef struct {
    ADC_HandleTypeDef *hadc;
    uint16_t airRaw, waterRaw;
} SEN0308_t;

HAL_StatusTypeDef SEN0308_Init(SEN0308_t *dev);

HAL_StatusTypeDef SEN0308_ReadRaw(SEN0308_t *dev, uint16_t *rawMoisture);
HAL_StatusTypeDef SEN0308_ReadRawAvg(SEN0308_t *dev, uint16_t *rawMoisture, uint8_t numSamples);

uint8_t SEN0308_CalculateRelative(SEN0308_t *dev, uint16_t rawMoisture);

#endif /* SEN0308_H_ */
