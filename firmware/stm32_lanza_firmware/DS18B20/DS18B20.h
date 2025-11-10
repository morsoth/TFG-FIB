/*
 * DFR0198.h
 *
 *  Created on: Nov 10, 2025
 *      Author: pzaragoza
 */

#ifndef DS18B20_H_
#define DS18B20_H_

#include "stm32wbxx_hal.h"

#define CMD_SEARCH_ROM   0xF0
#define CMD_READ_ROM     0x33
#define CMD_MATCH_ROM    0x55
#define CMD_SKIP_ROM     0xCC
#define CMD_CONVERT_T    0x44
#define CMD_READ_SCRATCH 0xBE
#define CMD_WRITE_SCRATCH 0x4E
#define CMD_COPY_SCRATCH  0x48

// Resolution
typedef enum {
    DS18B20_RES_9_BIT  = 9,
    DS18B20_RES_10_BIT = 10,
    DS18B20_RES_11_BIT = 11,
    DS18B20_RES_12_BIT = 12,
} DS18B20_Resolution_t;

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    DS18B20_Resolution_t resolution;
} DS18B20_t;

HAL_StatusTypeDef DS18B20_Init(DS18B20_t *dev);

HAL_StatusTypeDef DS18B20_ReadTemperature(DS18B20_t *dev, float *tC);
HAL_StatusTypeDef DS18B20_StartConversion(DS18B20_t *dev);

#endif /* DS18B20_H_ */
