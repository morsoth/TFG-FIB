/*
 * DFR0198.c
 *
 *  Created on: Nov 10, 2025
 *      Author: pzaragoza
 */

#include "DS18B20.h"

static inline void DWT_DelayInit(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static inline void delay_us(uint32_t us) {
    uint32_t cycles = (SystemCoreClock/1000000U) * us;
    uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles) { __NOP(); }
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
    /* Master reset: pull low >=480us, then release; sample presence ~70us */
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
        delay_us(6);

        releasePin(port, pin);
        delay_us(64);
    }
    else {
    	pullLowPin(port, pin);
        delay_us(60);

        releasePin(port, pin);
        delay_us(10);
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
    for (int i = 0; i < 8; ++i) {
        writeBit(port, pin, val & 0x01u);
        val >>= 1;
    }
}

static uint8_t readByte(GPIO_TypeDef *port, uint16_t pin) {
    uint8_t v=0;

    for (int i = 0; i < 8; ++i) {
        uint8_t b = readBit(port, pin);
        v |= (uint8_t)(b << i);
    }

    return v;
}

static uint8_t crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;

    while (len--) {
        uint8_t inbyte = *data++;

        for (uint8_t i=0;i<8;i++){
            uint8_t mix = (crc ^ inbyte) & 0x01u;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

static uint8_t resolutionToConfig(DS18B20_Resolution_t r) {
    switch (r) {
        case DS18B20_RES_9_BIT:  return 0x1F;
        case DS18B20_RES_10_BIT: return 0x3F;
        case DS18B20_RES_11_BIT: return 0x5F;
        default:                 return 0x7F;
    }
}

HAL_StatusTypeDef DS18B20_Init(DS18B20_t *dev) {
	DWT_DelayInit();
    releasePin(dev->port, dev->pin);

    if (!resetPresence(dev->port, dev->pin)) return HAL_ERROR;

    uint8_t cfg = resolutionToConfig(dev->resolution);

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
    uint32_t timeout_ms;
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
    *temp_c = (float)raw / 16.0f;

    return HAL_OK;
}

HAL_StatusTypeDef DS18B20_ReadROM(DS18B20_t *dev, uint8_t rom[8]){
    if (!resetPresence(dev->port, dev->pin)) return HAL_ERROR;

    writeByte(dev->port, dev->pin, DS18B20_CMD_READ_ROM);
    for (int i = 0; i < 8; ++i) rom[i] = readByte(dev->port, dev->pin);

    uint8_t crc = crc8(rom, 7);
    return (crc == rom[7]) ? HAL_OK : HAL_ERROR;
}
