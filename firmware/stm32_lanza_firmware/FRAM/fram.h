/*
 * fram.h
 *
 *  Created on: Dec 17, 2025
 *      Author: pzaragoza
 */

#ifndef FRAM_H_
#define FRAM_H_

#include "stm32wbxx_hal.h"

#include "MB85RS256B.h"

#define FRAM_SLOT_SIZE			32
#define FRAM_TOTAL_SLOTS		(MB85RS256B_SIZE / FRAM_SLOT_SIZE)
#define FRAM_DATA_SLOTS			(FRAM_TOTAL_SLOTS - 1)
#define FRAM_DATA_CAPACITY		(FRAM_DATA_SLOTS)

#define FRAM_SLOT_BASE			0x0000
#define FRAM_DEVICE_BASE		0x0000
#define FRAM_META_A_BASE		0x0010
#define FRAM_META_B_BASE		0x0018

enum validDataBit { IRRADIANCE_BIT, AIR_TEMP_BIT, SOIL_TEMP_BIT, AIR_HUM_BIT, SOIL_MOIST_BIT, BATT_VOLT_BIT, HOURS_BIT, MINUTES_BIT, SECONDS_BIT, DAY_BIT, MONTH_BIT, YEAR_BIT  };

#define VALID_BIT_SET(v, bit)		((v) |= ((uint16_t)1 << (bit)))
#define VALID_BIT_CLEAR(v, bit)		((v) &= ~((uint16_t)1 << (bit)))
#define VALID_BIT_IS_SET(v, bit)	((((v)) >> (bit)) & 1)

typedef struct {
	float irradiance_Wm2;				// 4 byte

	float airTemp_C;					// 4 bytes
	float soilTemp_C;					// 4 bytes

	uint8_t airHummidity_perc;			// 1 bytes
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

static_assert(sizeof(DataFrame_t) == 32, "FrameData_t must be 32 bytes");
static_assert(sizeof(MetaFrame_t) == 8,  "FrameMeta_t must be 8 bytes");

typedef struct {
	MB85RS256B_t *fram;
    uint16_t write_idx;
    uint16_t count;
    uint8_t  seq;
} FramRing_t;



#endif /* FRAM_H_ */
