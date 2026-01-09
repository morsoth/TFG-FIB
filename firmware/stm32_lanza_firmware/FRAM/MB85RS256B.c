/*
 * MB85RS256B.c
 *
 *  Created on: Dec 17, 2025
 *      Author: pzaragoza
 */

#include "MB85RS256B.h"

static inline void csLow(MB85RS256B_t *dev) {
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
}

static inline void csHigh(MB85RS256B_t *dev) {
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

static HAL_StatusTypeDef spiTransmit(MB85RS256B_t *dev, const uint8_t *buf, uint16_t len) {
    return HAL_SPI_Transmit(dev->hspi, (uint8_t*)buf, len, MB85RS256B_TIMEOUT);
}

static HAL_StatusTypeDef spiReceive(MB85RS256B_t *dev, uint8_t *buf, uint16_t len) {
    return HAL_SPI_Receive(dev->hspi, buf, len, MB85RS256B_TIMEOUT);
}

static HAL_StatusTypeDef writeEnable(MB85RS256B_t *dev) {
    uint8_t cmd = MB85RS256B_CMD_WREN;
    csLow(dev);
    HAL_StatusTypeDef status = spiTransmit(dev, &cmd, 1);
    csHigh(dev);
    return status;
}

static HAL_StatusTypeDef writeDisable(MB85RS256B_t *dev) {
    uint8_t cmd = MB85RS256B_CMD_WRDI;
    csLow(dev);
    HAL_StatusTypeDef status = spiTransmit(dev, &cmd, 1);
    csHigh(dev);
    return status;
}

static HAL_StatusTypeDef readId(MB85RS256B_t *dev, uint8_t id[3]) {
    if (!id) return HAL_ERROR;

    uint8_t cmd = MB85RS256B_CMD_RDID;
    csLow(dev);
    HAL_StatusTypeDef status = spiTransmit(dev, &cmd, 1);
    if (status == HAL_OK) status = spiReceive(dev, id, 3);
    csHigh(dev);
    return status;
}

HAL_StatusTypeDef MB85RS256B_Init(MB85RS256B_t *dev) {
	if (!dev || !dev->hspi || !dev->cs_port) return HAL_ERROR;

	csHigh(dev);
	HAL_Delay(1);

	writeDisable(dev);

	return HAL_OK;
}

HAL_StatusTypeDef MB85RS256B_Read(MB85RS256B_t *dev, uint16_t addr, uint8_t *data, size_t len) {
	if (!dev || !data) return HAL_ERROR;
	if (len == 0) return HAL_OK;

	// Rango dentro de la FRAM
	if ((uint32_t)addr + (uint32_t)len > MB85RS256B_SIZE) return HAL_ERROR;

	uint8_t hdr[3];
	hdr[0] = MB85RS256B_CMD_READ;
	hdr[1] = (uint8_t)(addr >> 8);
	hdr[2] = (uint8_t)(addr & 0xFF);

	csLow(dev);

	HAL_StatusTypeDef status = spiTransmit(dev, hdr, sizeof(hdr));
	if (status == HAL_OK) {
		status = spiReceive(dev, data, (uint16_t)len);
	}

	csHigh(dev);
	return status;
}

HAL_StatusTypeDef MB85RS256B_Write(MB85RS256B_t *dev, uint16_t addr, const uint8_t *data, size_t len) {
	if (!dev || !data) return HAL_ERROR;
	if (len == 0) return HAL_OK;

	// Rango dentro de la FRAM
	if ((uint32_t)addr + (uint32_t)len > MB85RS256B_SIZE) return HAL_ERROR;

	HAL_StatusTypeDef status = writeEnable(dev);
	if (status != HAL_OK) return status;

	uint8_t hdr[3];
	hdr[0] = MB85RS256B_CMD_WRITE;
	hdr[1] = (uint8_t)(addr >> 8);
	hdr[2] = (uint8_t)(addr & 0xFF);

	csLow(dev);

	status = spiTransmit(dev, hdr, sizeof(hdr));
	if (status == HAL_OK) {
		status = spiTransmit(dev, data, (uint16_t)len);
	}

	csHigh(dev);

	writeDisable(dev);

	return status;
}
