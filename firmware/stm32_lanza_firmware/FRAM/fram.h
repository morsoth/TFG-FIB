/*
 * fram.h
 *
 *  Created on: Dec 17, 2025
 *      Author: pzaragoza
 */

#ifndef FRAM_H_
#define FRAM_H_

#include "stm32wbxx_hal.h"

#include <stdio.h>
#include <string.h>

#include "MB85RS256B.h"

#define FRAM_SLOT_SIZE			32
#define FRAM_TOTAL_SLOTS		(MB85RS256B_SIZE / FRAM_SLOT_SIZE)
#define FRAM_DATA_SLOTS			(FRAM_TOTAL_SLOTS - 1)

#define FRAM_START				0x0000
#define FRAM_DEVICE_START		0x0000
#define FRAM_TEST_START			0x0008
#define FRAM_META_A_START		0x0010
#define FRAM_META_B_START		0x0018
#define FRAM_DATA_START			0x0020

#define FRAM_FRAME_COMMIT_VALUE		0x3C
#define FRAM_META_COMMIT_VALUE		0xA5

enum validDataBit { IRRADIANCE_BIT, AIR_TEMP_BIT, SOIL_TEMP_BIT, AIR_HUM_BIT, SOIL_MOIST_BIT, BATT_VOLT_BIT, HOURS_BIT, MINUTES_BIT, SECONDS_BIT, DAY_BIT, MONTH_BIT, YEAR_BIT };

#define VALID_BIT_SET(v, bit)		((v) |= ((uint16_t)1 << (bit)))
#define VALID_BIT_CLEAR(v, bit)		((v) &= ~((uint16_t)1 << (bit)))
#define VALID_BIT_IS_SET(v, bit)	((((v)) >> (bit)) & 1)

typedef struct {
	float irradiance_Wm2;				// 4 byte

	float airTemp_C;					// 4 bytes
	float soilTemp_C;					// 4 bytes

	uint8_t airHumidity_perc;			// 1 bytes
	uint8_t soilMoisture_perc;			// 1 byte

	uint16_t batteryVoltage_mV;			// 2 bytes

	uint8_t hours, minutes, seconds;	// 3 bytes
	uint8_t day, month, year;			// 3 bytes

	uint16_t validDataVector;			// 2 bytes
} DataSample_t; // 24 bytes aligned

typedef struct {
	DataSample_t data;		// 24 bytes
	uint16_t crc;			// 2 bytes
	uint8_t commit;			// 1 bytes
	uint8_t  _reserved[5];	// 5 bytes
} DataFrame_t; // 32 bytes aligned

typedef struct {
    uint16_t write_idx; // 2 bytes
    uint16_t count;		// 2 bytes
    uint16_t crc;       // 2 bytes
	uint8_t seq;		// 1 bytes
	uint8_t commit;		// 1 bytes
} MetaFrame_t; // 8 bytes aligned


typedef struct {
	uint32_t system_id;
	uint32_t modified_date;
} DeviceFrame_t; // 8 bytes aligned

_Static_assert(sizeof(DataFrame_t) == 32, "DataFrame_t must be 32 bytes");
_Static_assert(sizeof(MetaFrame_t) == 8, "MetaFrame_t must be 8 bytes");

typedef struct {
	MB85RS256B_t *fram;
    uint16_t write_idx; // [0..1022]
    uint16_t count; // [0..1023]
    uint8_t  seq;
} FramRing_t;

HAL_StatusTypeDef FRAM_Init(FramRing_t *mem);

HAL_StatusTypeDef FRAM_SaveData(FramRing_t *mem, DataSample_t *data);
HAL_StatusTypeDef FRAM_GetSlot(FramRing_t *mem, uint16_t slot, DataSample_t *data, uint8_t *valid);

HAL_StatusTypeDef FRAM_WriteData(FramRing_t *mem, uint16_t addr, DataSample_t *data);
HAL_StatusTypeDef FRAM_WriteDeviceInfo(FramRing_t *mem, DeviceFrame_t *dev_info);

HAL_StatusTypeDef FRAM_Reset(FramRing_t *mem);
HAL_StatusTypeDef FRAM_EraseAll(FramRing_t *mem);


#endif /* FRAM_H_ */
