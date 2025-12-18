/*
 * app.c
 *
 *  Created on: Oct 8, 2025
 *      Author: pzaragoza
 */

#include <stdio.h>

#include "app.h"
#include "main.h"

#include "TSL2591.h"
#include "INA3221.h"
#include "SHT3x.h"
#include "DS18B20.h"
#include "SEN0308.h"
#include "fram.h"

//#define DDEBUG
//#define PRINT_CSV

extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c3;
extern RTC_HandleTypeDef hrtc;
extern SPI_HandleTypeDef hspi2;

void initFRAM();
void initINA3221();
void initTSL2591();
void initSHT3X();
void initDFR0198();
void initSEN0308();

void readRTC();
void readINA3221();
void readTSL2591();
void readSHT3X();
void readDFR0198();
void readSEN0308();

void I2C_bus_scan();

MB85RS256B_t fram;
FramRing_t mem;

INA3221_t ina;
TSL2591_t tsl;
SHT3X_t sht;
DS18B20_t dfr;
SEN0308_t sen;

RTC_TimeTypeDef time;
RTC_DateTypeDef date;

float irradiance_Wm2;

float airTemp_C;
float soilTemp_C;

uint8_t airHummidity_perc;
uint8_t soilMoisture_perc;

uint16_t batteryVoltage_mV;

void setup() {
	I2C_bus_scan();

	initFRAM();

	initINA3221();
	initTSL2591();
	initSHT3X();
	initDFR0198();
	initSEN0308();
}

void loop() {
	readRTC();
	readINA3221();
	readTSL2591();
	readSHT3X();
	readDFR0198();
	readSEN0308();

	// write fram & commit

	HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
	HAL_Delay(1000);
}

// RTC
void readRTC() {
	// Reads RTC time
	if (HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) == HAL_OK) {
		VALID_BIT_SET(sampleData, HOURS_BIT);
		VALID_BIT_SET(sampleData, MINUTES_BIT);
	}
	// Error while reading RTC time
	else {
		printf("Error while reading RTC time\r\n");

		VALID_BIT_CLEAR(sampleData, HOURS_BIT);
		VALID_BIT_CLEAR(sampleData, MINUTES_BIT);
	}

	// Reads RTC date
	if (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) == HAL_OK) {
		VALID_BIT_SET(sampleData, DAY_BIT);
		VALID_BIT_SET(sampleData, MONTH_BIT);
		VALID_BIT_SET(sampleData, YEAR_BIT);
	}
	// Error while reading RTC date
	else {
		printf("Error while reading RTC date\r\n");

		VALID_BIT_CLEAR(sampleData, DAY_BIT);
		VALID_BIT_CLEAR(sampleData, MONTH_BIT);
		VALID_BIT_CLEAR(sampleData, YEAR_BIT);
	}
}

// INIT ---------------------------------------------------------------------

void initFRAM() {
	fram.hspi = &hspi2;
	fram.cs_port = CS_FRAM_GPIO_Port;
	fram.cs_pin = CS_FRAM_Pin;

	if (FRAM_Init(&fram) == HAL_OK) printf("FRAM inicializada correctamente\r\n");
	else printf("FRAM no inicializada\r\n");

	mem.fram = &fram;
	// ...
}

void initINA3221() {
	ina.hi2c = &hi2c3;
	ina.shuntResistance[0] = 0.330f;
	ina.shuntResistance[1] = 0.330f;
	ina.shuntResistance[2] = 0.330f;
	ina.averagingMode = INA3221_AVG_64;
	ina.convTimeBus = INA3221_CT_1100us;
	ina.convTimeShunt = INA3221_CT_1100us;
	ina.operatingMode = INA3221_MODE_SHUNT_BUS_CONTINUOUS;

	if (INA3221_Init(&ina) == HAL_OK) printf("INA3221 inicializado correctamente\r\n");
	else printf("INA3221 no inicializado\r\n");
}

void initTSL2591() {
	tsl.hi2c = &hi2c3;
	tsl.gain = TSL2591_GAIN_LOW;
	tsl.integrationTime = TSL2591_INTEGRATION_200MS;

	if (TSL2591_Init(&tsl) == HAL_OK) printf("TSL2591 inicializado correctamente\r\n");
	else printf("TSL2591 no inicializado\r\n");
}

void initSHT3X() {
	sht.hi2c = &hi2c3;
	sht.clockStretch = SHT3X_NOSTRETCH;
	sht.repeatability = SHT3X_REPEAT_MED;

	if (SHT3X_Init(&sht) == HAL_OK) printf("SHT3x inicializado correctamente\r\n");
	else printf("SHT3x no inicializado\r\n");

	SHT3X_Heater(&sht, SHT3X_HEATER_OFF);
}

void initDFR0198()  {
	dfr.port = DFR0198_GPIO_Port;
	dfr.pin = DFR0198_Pin;
	dfr.resolution = DS18B20_RES_12_BIT;

	if (DS18B20_Init(&dfr) == HAL_OK) printf("DFR0198 inicializado correctamente\r\n");
	else printf("DFR0198 no inicializado\r\n");
}

void initSEN0308() {
	sen.hadc = &hadc1;
	sen.port = SEN0308_GPIO_Port;
	sen.pin = SEN0308_Pin;
	sen.airRaw = 3620;
	sen.waterRaw = 520;
	if (SEN0308_Init(&sen) == HAL_OK) printf("SEN0308 inicializado correctamente\r\n");
	else printf("SEN0308 no inicializado\r\n");
}

// READ ---------------------------------------------------------------------

void readINA3221() {
	float busVoltage_V[3];
	float shuntVoltage_V[3];
	float current_mA[3];

	// Iterates the 3 channels
	for (int ch = 0; ch < 3; ++ch) {
		// Reads bus & shunt voltages
		if (INA3221_ReadVoltage(&ina, ch+1, &busVoltage_V[ch], &shuntVoltage_V[ch]) == HAL_OK) {
			// Calculates current
			current_mA[ch] = INA3221_CalculateCurrent_mA(&ina, ch+1, shuntVoltage_V[ch]);
		}
		// Error while reading the sensor
		else {
			printf("Error while reading INA3221\r\n\r\n");

			return;
		}
	}
}

void readTSL2591() {
	uint16_t full, ir;
	float lux;

	// Reads total & ir
	if (TSL2591_ReadChannels(&tsl, &full, &ir) == HAL_OK) {
		// Calculates lux
		lux = TSL2591_CalculateLux(&tsl, full, ir);

		// Calculates irradiance
		irradiance_Wm2 = TSL2591_CalculateIrradiance(lux);

		VALID_BIT_SET(sampleData, IRRADIANCE_BIT);
	}
	// Error while reading the sensor
	else {
		printf("Error while reading TSL2591\r\n\r\n");

		VALID_BIT_CLEAR(sampleData, IRRADIANCE_BIT);
	}
}

void readSHT3X() {
	float dewPoint_C;

	for (int8_t tries = 3; tries >= 0; --tries) {
		// Reads air temperature & hummidity
		if (SHT3X_ReadSingleShot(&sht, &airTemp_C, &airHummidity_perc) == HAL_OK) {
			// Calculates dew point
			dewPoint_C = SHT3X_CalculateDewpoint(airTemp_C, airHummidity_perc);
		}
		// Error while reading the sensor
		else {
			printf("Error while reading SHT3x\r\n\r\n");

			VALID_BIT_CLEAR(sampleData, AIR_TEMP_BIT);
			VALID_BIT_CLEAR(sampleData, AIR_HUM_BIT);

			return;
		}

		// RH > 95% or T-Td < 1ºC - Activate heater
		if (airHummidity_perc > 95.0f || airTemp_C - dewPoint_C < 1.0f) {
			SHT3X_Heater(&sht, SHT3X_HEATER_ON);
			HAL_Delay(15000);
			SHT3X_Heater(&sht, SHT3X_HEATER_OFF);
			HAL_Delay(60000);
		}
		// Valid reading
		else {
			VALID_BIT_SET(sampleData, AIR_TEMP_BIT);
			VALID_BIT_SET(sampleData, AIR_HUM_BIT);

			return;
		}
	}

	// Limit tries
	VALID_BIT_CLEAR(sampleData, AIR_TEMP_BIT);
	VALID_BIT_CLEAR(sampleData, AIR_HUM_BIT);
}

void readDFR0198() {
	// Reads soil temperature
	if (DS18B20_ReadTemperature(&dfr, &soilTemp_C) == HAL_OK) {
		VALID_BIT_SET(sampleData, SOIL_TEMP_BIT);
	}
	// Error while reading the sensor
	else {
		printf("Error while reading DFR0198\r\n");

		VALID_BIT_CLEAR(sampleData, SOIL_TEMP_BIT);
	}
}

void readSEN0308() {
	uint16_t rawMoisture;

	// Reads soil moisture
	if (SEN0308_ReadRawAvg(&sen, &rawMoisture, 10) == HAL_OK) {
		soilMoisture_perc = SEN0308_CalculateRelative(&sen, rawMoisture);

		VALID_BIT_SET(sampleData, SOIL_MOIST_BIT);
	}
	// Error while reading the sensor
	else {
		printf("Error while reading SEN0308\r\n\r\n");

		VALID_BIT_CLEAR(sampleData, SOIL_MOIST_BIT);
	}
}

// I2C bus scan
void I2C_bus_scan() {
	for (uint8_t i = 0; i < 128; i++) {
		if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i<<1), 3, 5) == HAL_OK) printf("%2x ", i);
		else printf("-- ");

		if (i > 0 && (i + 1) % 16 == 0) printf("\r\n");
	}

	printf("\r\n");
}
