/*
 * SEN0308.c
 *
 *  Created on: Nov 7, 2025
 *      Author: pzaragoza
 */

#include "SEN0308.h"

HAL_StatusTypeDef SEN0308_Init(SEN0308_t *dev) {
	// check adc works

	return HAL_OK;
}

HAL_StatusTypeDef SEN0308_ReadRaw(SEN0308_t *dev, uint16_t *rawMoisture) {
	HAL_StatusTypeDef status;

	status = HAL_ADC_Start(dev->hadc);
	if (status != HAL_OK) return status;

	status = HAL_ADC_PollForConversion(dev->hadc, SEN0308_POLL_TIMEOUT_MS);
	if (status != HAL_OK) return status;

	*rawMoisture = (uint32_t)HAL_ADC_GetValue(dev->hadc);

	status = HAL_ADC_Stop(dev->hadc);
	if (status != HAL_OK) return status;

	return HAL_OK;
}

HAL_StatusTypeDef SEN0308_ReadRawAvg(SEN0308_t *dev, uint16_t *rawMoisture, uint8_t numSamples) {
	HAL_StatusTypeDef status;

	if (numSamples == 0) return HAL_ERROR;

	uint32_t moistureAcc = 0;

	for (uint8_t s = 0; s < numSamples; ++s) {
		uint16_t rawRead = 0;

		status = SEN0308_ReadRaw(dev, &rawRead);
		if (status != HAL_OK) return status;

		moistureAcc += (uint32_t)rawRead;

		HAL_Delay(10);
	}

	*rawMoisture = (uint16_t)(moistureAcc/numSamples);

	return HAL_OK;
}

uint8_t SEN0308_CalculateRelative(SEN0308_t *dev, uint16_t rawMoisture) {
	if (rawMoisture > dev->airRaw) rawMoisture = dev->airRaw;
	if (rawMoisture < dev->waterRaw) rawMoisture = dev->waterRaw;

	uint32_t num = (uint32_t)(dev->airRaw - rawMoisture) * 100;
	uint32_t den = (uint32_t)(dev->airRaw - dev->waterRaw);
	return (uint8_t)((num + den / 2) / den);
}
