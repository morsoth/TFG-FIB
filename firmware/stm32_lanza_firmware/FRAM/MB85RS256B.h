/*
 * MB85RS256B.h
 *
 *  Created on: Dec 17, 2025
 *      Author: pzaragoza
 */

#ifndef MB85RS256B_H_
#define MB85RS256B_H_

#include "stm32wbxx_hal.h"

#define MB85RS256B_SIZE 		32768

#define MB85RS256B_CMD_WREN		0x06
#define MB85RS256B_CMD_WRDI		0x04
#define MB85RS256B_CMD_RDSR		0x05
#define MB85RS256B_CMD_WRSR		0x01
#define MB85RS256B_CMD_READ		0x03
#define MB85RS256B_CMD_WRITE	0x02
#define MB85RS256B_CMD_RDID		0x9F
#define MB85RS256B_CMD_FSTRD	0x0B

#define MB85RS256B_TIMEOUT		50

typedef struct {
    SPI_HandleTypeDef *hspi;
    GPIO_TypeDef *cs_port;
    uint16_t cs_pin;
} MB85RS256B_t;

HAL_StatusTypeDef MB85RS256B_Init(MB85RS256B_t *dev);

HAL_StatusTypeDef MB85RS256B_Read(MB85RS256B_t *dev, uint16_t addr, uint8_t *data, size_t len);
HAL_StatusTypeDef MB85RS256B_Write(MB85RS256B_t *dev, uint16_t addr, const uint8_t *data, size_t len);

#endif /* MB85RS256B_H_ */
