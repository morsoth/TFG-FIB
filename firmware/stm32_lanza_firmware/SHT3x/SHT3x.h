/*
 * SHT3x.h
 *
 *  Created on: Nov 10, 2025
 *      Author: pzaragoza
 */

#ifndef SHT3X_H_
#define SHT3X_H_

#include <math.h>

#include "stm32wbxx_hal.h"

#define SHT3X_I2C_ADDR				(0x44 << 1) // I2C address

// Commands
#define SHT3X_CMD_SOFT_RESET		0x30A2 // Soft reset

#define SHT3X_CMD_STATUS_READ		0xF32D // Read status register
#define SHT3X_CMD_STATUS_CLEAR		0x3041 // Clear status register

#define SHT3X_CMD_HEATER_ENABLE		0x306D // Internal heater on
#define SHT3X_CMD_HEATER_DISABLE	0x3066 // Internal heater off

#define SHT3X_CMD_SS_NCS_HIGH       0x2400 // Single-shot without clock stretching, high repeatability
#define SHT3X_CMD_SS_NCS_MED        0x240B // Single-shot without clock stretching, medium repeatability
#define SHT3X_CMD_SS_NCS_LOW        0x2416 // Single-shot without clock stretching, low repeatability

#define SHT3X_CMD_SS_CS_HIGH        0x2C06 // Single-shot with clock stretching, high repeatability
#define SHT3X_CMD_SS_CS_MED         0x2C0D // Single-shot with clock stretching, medium repeatability
#define SHT3X_CMD_SS_CS_LOW         0x2C10 // Single-shot with clock stretching, low repeatability

// Repeatability
typedef enum {
    SHT3X_REPEAT_LOW, // Low
    SHT3X_REPEAT_MED, // Medium
    SHT3X_REPEAT_HIGH // High
} SHT3X_Repeatability_t;

// Clock Stretch
typedef enum {
    SHT3X_NOSTRETCH, // No stretch
    SHT3X_STRETCH	 // Stretch
} SHT3X_ClockStretch_t;

// Heater
typedef enum {
    SHT3X_HEATER_OFF = 0, // Heater on
    SHT3X_HEATER_ON  = 1  // Heater off
} SHT3X_Heater_t;

// Struct
typedef struct {
    I2C_HandleTypeDef *hi2c;
    SHT3X_Repeatability_t repeatability;
	SHT3X_ClockStretch_t clockStretch;
} SHT3X_t;

// Functions
HAL_StatusTypeDef SHT3X_Init(SHT3X_t *dev);

HAL_StatusTypeDef SHT3X_SoftReset(SHT3X_t *dev);

HAL_StatusTypeDef SHT3X_ReadRaw(SHT3X_t *dev, uint16_t *rawT, uint16_t *rawRH);
HAL_StatusTypeDef SHT3X_ReadSingleShot(SHT3X_t *dev, float *temp_c, float *rh_perc);
float SHT3X_CalculateDewpoint(float temp_c, float rh_perc);

HAL_StatusTypeDef SHT3X_ReadStatus(SHT3X_t *dev, uint16_t *status);
HAL_StatusTypeDef SHT3X_ClearStatus(SHT3X_t *dev);
HAL_StatusTypeDef SHT3X_Heater(SHT3X_t *dev, SHT3X_Heater_t state);

#endif /* SHT3X_H_ */
