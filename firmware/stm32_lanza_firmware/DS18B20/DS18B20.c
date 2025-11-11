/*
 * DFR0198.c
 *
 *  Created on: Nov 10, 2025
 *      Author: pzaragoza
 */

#include "DS18B20.h"

static inline uint8_t DWT_DelayInit(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    __NOP();
    __NOP();
    __NOP();

    if(DWT->CYCCNT) return 1;
    else return 0;
}

static inline void delay_us(uint32_t us) {
    uint32_t cycles = (SystemCoreClock/1000000) * us;
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles);
}

static inline void releasePin(GPIO_TypeDef *port, uint16_t pin) {
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
}

static inline void pullLowPin(GPIO_TypeDef *port, uint16_t pin) {
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
}

static inline uint8_t readPin(GPIO_TypeDef *port, uint16_t pin) {
    return (uint8_t)HAL_GPIO_ReadPin(port, pin);
}

static uint8_t resetPresence(GPIO_TypeDef *port, uint16_t pin) {
	releasePin(port, pin);
	delay_us(10);
	if (!readPin(port, pin)) return 0;

	pullLowPin(port, pin);
    delay_us(480);

    releasePin(port, pin);
    delay_us(70);

    uint8_t presence = (readPin(port, pin) == 0); // 0 means presence pulse
    delay_us(410);

    return presence;
}

static void writeBit(GPIO_TypeDef *port, uint16_t pin, uint8_t bit) {
    if (bit){
    	pullLowPin(port, pin);
        delay_us(10);

        releasePin(port, pin);
        delay_us(55);
    }
    else {
    	pullLowPin(port, pin);
        delay_us(65);

        releasePin(port, pin);
        delay_us(5);
    }
}

static uint8_t readBit(GPIO_TypeDef *port, uint16_t pin) {
    pullLowPin(port, pin);
    delay_us(3);

    releasePin(port, pin);
    delay_us(10);

    uint8_t b = readPin(port, pin);
    delay_us(53);

    return b;
}

static void writeByte(GPIO_TypeDef *port, uint16_t pin, uint8_t val) {
    for (uint8_t bitMask = 0x01; bitMask != 0; bitMask <<= 1) {
        writeBit(port, pin, (val & bitMask) ? 1:0);
    }
}

static uint8_t readByte(GPIO_TypeDef *port, uint16_t pin) {
    uint8_t r = 0;

    for (uint8_t bitMask = 0x01; bitMask != 0; bitMask <<= 1) {
        uint8_t b = readBit(port, pin);
        r |= bitMask * b;
    }

    return r;
}

static uint8_t crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;

    while (len--) {
        uint8_t inbyte = *data++;

        for (uint8_t i = 8; i != 0; --i){
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }

    return crc;
}

HAL_StatusTypeDef DS18B20_Init(DS18B20_t *dev) {
	if (!DWT_DelayInit()) return HAL_ERROR;
    releasePin(dev->port, dev->pin);

    if (!resetPresence(dev->port, dev->pin)) return HAL_ERROR;

    uint8_t cfg = (uint8_t)dev->resolution;

	writeByte(dev->port, dev->pin, DS18B20_CMD_SKIP_ROM);
	writeByte(dev->port, dev->pin, DS18B20_CMD_WRITE_SCRATCH);
	writeByte(dev->port, dev->pin, 0x00); // TH
	writeByte(dev->port, dev->pin, 0x00); // TL
	writeByte(dev->port, dev->pin, cfg);  // CONFIG

	return HAL_OK;
}

HAL_StatusTypeDef DS18B20_ReadTemperature(DS18B20_t *dev, float *temp_c){
	// Transacción 1: iniciar conversión
	if (!resetPresence(dev->port, dev->pin)) return HAL_ERROR;

	writeByte(dev->port, dev->pin, DS18B20_CMD_SKIP_ROM);
	writeByte(dev->port, dev->pin, DS18B20_CMD_CONVERT_T);

	// Espera por fin de conversión
    int32_t timeout_ms;
    switch (dev->resolution){
        case DS18B20_RES_9_BIT:  timeout_ms = 94;  break;
        case DS18B20_RES_10_BIT: timeout_ms = 188; break;
        case DS18B20_RES_11_BIT: timeout_ms = 375; break;
        default:                 timeout_ms = 750; break;
    }

    while (timeout_ms--) {
		if (readBit(dev->port, dev->pin)) break;
		HAL_Delay(1);
	}
    if (timeout_ms <= 0) return HAL_ERROR;

    // Transacción 2: leer scratchpad
    if (!resetPresence(dev->port, dev->pin)) return HAL_ERROR;

    writeByte(dev->port, dev->pin, DS18B20_CMD_SKIP_ROM);
	writeByte(dev->port, dev->pin, DS18B20_CMD_READ_SCRATCH);

    uint8_t scratch[9];
	for (int i = 0; i < 9; ++i) scratch[i] = readByte(dev->port, dev->pin);

	// CRC Dallas/Maxim
	uint8_t crc = crc8(scratch, 8);
	if (crc != scratch[8]) return HAL_ERROR;

	// Conversión a ºC
    int16_t raw = (int16_t)((scratch[1] << 8) | scratch[0]);
    *temp_c = (float)raw * 0.0625f;

    return HAL_OK;
}

HAL_StatusTypeDef DS18B20_ReadROM(DS18B20_t *dev, uint8_t rom[8]){
    if (!resetPresence(dev->port, dev->pin)) return HAL_ERROR;

    writeByte(dev->port, dev->pin, DS18B20_CMD_READ_ROM);
    for (int i = 0; i < 8; ++i) rom[i] = readByte(dev->port, dev->pin);

    uint8_t crc = crc8(rom, 7);
    return (crc == rom[7]) ? HAL_OK : HAL_ERROR;
}
