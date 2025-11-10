/*
 * DFR0198.c
 *
 *  Created on: Nov 10, 2025
 *      Author: pzaragoza
 */

#include "DS18B20.h"

static inline void DWT_Delay_Init(void) {
    /* Enable DWT CYCCNT (not available on Cortex-M0/M0+) */
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
    /* For open-drain: write '1' to release; external pull-up takes line high */
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
}

static inline void pullLowPin(GPIO_TypeDef *port, uint16_t pin) {
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
}

static inline uint8_t readPin(GPIO_TypeDef *port, uint16_t pin) {
    return (uint8_t)HAL_GPIO_ReadPin(port, pin);
}

static bool resetPresence(GPIO_TypeDef *port, uint16_t pin) {
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
    uint8_t b;
    pullLowPin(port, pin);
    delay_us(3);
    releasePin(port, pin);
    delay_us(10);
    b = readPin(port, pin);
    delay_us(53);
    return b;
}

static void writeByte(GPIO_TypeDef *port, uint16_t pin, uint8_t val) {
    for (int i=0;i<8;i++){
        writeBit(port, pin, val & 0x01u);
        val >>= 1;
    }
}

static uint8_t readByte(GPIO_TypeDef *port, uint16_t pin) {
    uint8_t v=0;
    for (int i=0;i<8;i++){
        uint8_t b = readBit(port, pin);
        v |= (uint8_t)(b << i);
    }
    return v;
}

/* Dallas/Maxim CRC8 (reflected), polynomial 0x31 (0x8C reversed) */
static uint8_t ds_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    while (len--){
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

/* Resolution config register values (bits R1:R0 at 6:5) */
static uint8_t res_to_cfg(DS18B20_Resolution_t r) {
    switch (r){
        case DS18B20_RES_9_BIT:  return 0x1F; // R1=0,R0=0
        case DS18B20_RES_10_BIT: return 0x3F; // R1=0,R0=1
        case DS18B20_RES_11_BIT: return 0x5F; // R1=1,R0=0
        default:                 return 0x7F; // 12-bit: R1=1,R0=1
    }
}

HAL_StatusTypeDef DS18B20_Init(DS18B20_t *dev) {
	HAL_StatusTypeDef status;

    /* The GPIO pin must be configured as Output Open-Drain, No-Pull in CubeMX.
       Enable DWT for microsecond timing. */
    DWT_Delay_Init();
    releasePin(dev->port, dev->pin);

    if (!resetPresence(dev->port, dev->pin)){
        return HAL_ERROR; // no sensor detected
    }

    return DS18B20_SetResolution(dev, dev->resolution);
}

// TODO: integrar en la funcion Init
HAL_StatusTypeDef DS18B20_SetResolution(DS18B20_t *dev, DS18B20_Resolution_t res) {
    if (!resetPresence(dev->port, dev->pin)) return HAL_ERROR;

    uint8_t cfg = res_to_cfg(res);
    writeByte(dev->port, dev->pin, CMD_SKIP_ROM);
    writeByte(dev->port, dev->pin, CMD_WRITE_SCRATCH);
    writeByte(dev->port, dev->pin, 0x00); // TH
    writeByte(dev->port, dev->pin, 0x00); // TL
    writeByte(dev->port, dev->pin, cfg);  // CONFIG
    dev->resolution = res;
    /* COPY_SCRATCH is optional (persists to EEPROM, takes up to 10ms).
       Not strictly required for runtime since config is in SRAM until power-cycle. */
    return HAL_OK;
}

HAL_StatusTypeDef DS18B20_StartConversion(DS18B20_t *dev){
    if (!resetPresence(dev->port, dev->pin)) return HAL_ERROR;

	writeByte(dev->port, dev->pin, CMD_SKIP_ROM);
	writeByte(dev->port, dev->pin, CMD_CONVERT_T);

    /* If parasite_power==true, you'd need a strong pull-up here during conversion.
       For normal 3-wire, we just poll until conversion completes. */
    return HAL_OK;
}

bool DS18B20_WaitForConversion(DS18B20_t *dev, uint32_t timeout_ms){
    /* Sensor returns 0 while converting, 1 when ready. */
    while (timeout_ms--){
        if (readBit(dev->port, dev->pin)) return true;
        HAL_Delay(1);
    }
    return false;
}

HAL_StatusTypeDef DS18B20_ReadScratchpad(DS18B20_t *dev, uint8_t scratch[9]){
    if (!resetPresence(dev->port, dev->pin)) return HAL_ERROR;

    writeByte(dev->port, dev->pin, CMD_SKIP_ROM);
    writeByte(dev->port, dev->pin, CMD_READ_SCRATCH);
    for (int i=0;i<9;i++){
        scratch[i] = readByte(dev->port, dev->pin);
    }

    /* Verify CRC over first 8 bytes */
    uint8_t crc = ds_crc8(scratch, 8);
    if (crc != scratch[8]) return HAL_ERROR;

    return HAL_OK;
}

HAL_StatusTypeDef DS18B20_ReadTemperatureC(DS18B20_t *dev, float *tC){
    /* Start conversion */
    if (DS18B20_StartConversion(dev) != HAL_OK) return HAL_ERROR;

    /* Max conversion time depends on resolution */
    uint32_t max_ms = 750; // worst case 12-bit
    switch (dev->resolution){
        case DS18B20_RES_9_BIT:  max_ms = 94;  break;
        case DS18B20_RES_10_BIT: max_ms = 188; break;
        case DS18B20_RES_11_BIT: max_ms = 375; break;
        default:                 max_ms = 750; break;
    }

    if (!DS18B20_WaitForConversion(dev, max_ms + 50)) return HAL_ERROR;

    /* Read scratchpad and convert to Celsius */
    uint8_t s[9];
    if (DS18B20_ReadScratchpad(dev, s) != HAL_OK) return HAL_ERROR;

    int16_t raw = (int16_t)((s[1] << 8) | s[0]);
    *tC = (float)raw / 16.0f;
    return HAL_OK;
}

HAL_StatusTypeDef DS18B20_ReadROM(DS18B20_t *dev, uint8_t rom[8]){
    if (!resetPresence(dev->port, dev->pin)) return HAL_ERROR;

    writeByte(dev->port, dev->pin, CMD_READ_ROM);
    for (int i=0;i<8;i++){
        rom[i] = readByte(dev->port, dev->pin);
    }
    /* Optional: verify ROM CRC (byte 7) */
    uint8_t crc = ds_crc8(rom, 7);
    return (crc == rom[7]) ? HAL_OK : HAL_ERROR;
}
