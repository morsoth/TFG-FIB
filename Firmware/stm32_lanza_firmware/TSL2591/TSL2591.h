/*
 * TSL2591.h
 *
 *  Created on: Oct 8, 2025
 *      Author: pzaragoza
 */

#ifndef TSL2591_H_
#define TSL2591_H_

#include "stm32wbxx_hal.h"

#define TSL2591_ADDR          (0x29 << 1) // I2C address

// Commands
#define TSL2591_CMD_BIT       0xA0 // Needed to access registers

// Registers
#define TSL2591_ENABLE        0x00 // Enable register
#define TSL2591_CONTROL       0x01 // Control register
#define TSL2591_CHAN0_LOW     0x14 // Channel 0 data (full) register (LSB)
#define TSL2591_CHAN1_LOW     0x16 // Channel 1 data (ir) register (LSB)

// Control bits
#define TSL2591_ENABLE_PON    0x01 // Flag to enable sensor
#define TSL2591_ENABLE_AEN    0x02 // Flag to enable ALS

// Calibration
#define TSL2591_LUX_DF        408.0f // Lux coefficient
#define TSL2591_CALIB_A       1.0f   // Linear regression slope
#define TSL2591_CALIB_B       0.0f   // Linear regression intercept

#define TSL2591_LUM_EFF       93.0f  // Luminous efficacy

// Gain
typedef enum {
    TSL2591_GAIN_LOW  = 0x00, // 1x
    TSL2591_GAIN_MED  = 0x10, // 25x
    TSL2591_GAIN_HIGH = 0x20, // 428x
    TSL2591_GAIN_MAX  = 0x30  // 9876x
} TSL2591_Gain_t;

// Integration time
typedef enum {
    TSL2591_INTEGRATION_100MS = 0x00, // 100 ms
    TSL2591_INTEGRATION_200MS = 0x01, // 200 ms
    TSL2591_INTEGRATION_300MS = 0x02, // 300 ms
    TSL2591_INTEGRATION_400MS = 0x03, // 400 ms
    TSL2591_INTEGRATION_500MS = 0x04, // 500 ms
    TSL2591_INTEGRATION_600MS = 0x05  // 600 ms
} TSL2591_IntegrationTime_t;

// Struct
typedef struct {
    I2C_HandleTypeDef *hi2c;
    TSL2591_Gain_t gain;
    TSL2591_IntegrationTime_t integrationTime;
} TSL2591_t;

// Functions
HAL_StatusTypeDef TSL2591_Init(TSL2591_t *dev);

void TSL2591_Enable(TSL2591_t *dev);
void TSL2591_Disable(TSL2591_t *dev);

HAL_StatusTypeDef TSL2591_ReadChannels(TSL2591_t *dev, uint16_t *ch0, uint16_t *ch1);
float TSL2591_CalculateLux(TSL2591_t *dev, uint16_t full, uint16_t ir);
float TSL2591_CalculateIrradiance(float lux);


#endif /* TSL2591_H_ */
