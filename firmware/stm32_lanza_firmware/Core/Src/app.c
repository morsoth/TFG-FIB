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
#include "SEN0308.h"

#include "../../DS18B20/DS18B20.h"

void printData();
void printHeaderCSV();
void printDataCSV();
void I2C_bus_scan();

extern I2C_HandleTypeDef hi2c1;
extern RTC_HandleTypeDef hrtc;
extern ADC_HandleTypeDef hadc1;

RTC_TimeTypeDef time;
RTC_DateTypeDef date;

uint8_t status[3];

TSL2591_t tsl;
uint16_t full, ir;
float lux;
float irradiance_Wm2;

INA3221_t ina;
float busVoltage_V[3];
float shuntVoltage_V[3];
float current_mA[3];
float power_mW[3];

SHT3X_t sht;
float airTemp_C;
float airHummidity_perc;
float dewPoint_C[3]; // TODO: seleccionar mejor manera de calcularlo

DS18B20_t dfr;
float soilTemp_C;

SEN0308_t sen;
uint16_t rawMoisture;
uint8_t soilMoisture_perc;

void setup() {
	I2C_bus_scan();

	// TSL2591
	tsl.hi2c = &hi2c1;
	tsl.gain = TSL2591_GAIN_LOW;
	tsl.integrationTime = TSL2591_INTEGRATION_200MS;
	if (TSL2591_Init(&tsl) == HAL_OK) printf("TSL2591 inicializado correctamente\r\n");
	else printf("TSL2591 no inicializado\r\n");

	// INA3221
	ina.hi2c = &hi2c1;
	ina.shuntResistance[0] = 0.1f;
	ina.shuntResistance[1] = 0.1f;
	ina.shuntResistance[2] = 0.1f;
	ina.averagingMode = INA3221_AVG_64;
	ina.convTimeBus = INA3221_CT_1100us;
	ina.convTimeShunt = INA3221_CT_1100us;
	ina.operatingMode = INA3221_MODE_SHUNT_BUS_CONTINUOUS;
	if (INA3221_Init(&ina) == HAL_OK) printf("INA3221 inicializado correctamente\r\n");
	else printf("INA3221 no inicializado\r\n");

	// SHT3x
	sht.hi2c = &hi2c1;
	sht.clockStretch = SHT3X_NOSTRETCH;
	sht.repeatability = SHT3X_REPEAT_MED;
	if (SHT3X_Init(&sht) == HAL_OK) printf("SHT3x inicializado correctamente\r\n");
	else printf("SHT3x no inicializado\r\n");
	SHT3X_Heater(&sht, SHT3X_HEATER_OFF);

	// DFR0198 (DS18B20)
	sen.port = GPIOA;
	sen.pin = GPIO_PIN_1;
	dfr.resolution = DS18B20_RES_12_BIT;
	if (DS18B20_Init(&dfr) == HAL_OK) printf("DFR0198 inicializado correctamente\r\n");
	else printf("DFR0198 no inicializado\r\n");

	// SEN0308
	sen.hadc = &hadc1;
	sen.port = GPIOA;
	sen.pin = GPIO_PIN_0;
	sen.airRaw = 3620;
	sen.waterRaw = 520;
	if (SEN0308_Init(&sen) == HAL_OK) printf("SEN0308 inicializado correctamente\r\n");
	else printf("SEN0308 no inicializado\r\n");

	//printHeaderCSV();
}

void loop() {
	if (HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) printf("Error al leer la hora\r\n");
	if (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK) printf("Error al leer la fecha\r\n");

	if (TSL2591_ReadChannels(&tsl, &full, &ir) == HAL_OK) {
		lux = TSL2591_CalculateLux(&tsl, full, ir);
		irradiance_Wm2 = TSL2591_CalculateIrradiance(lux);
	}
	else printf("Error al leer el TSL2591\r\n\r\n");

	for (int ch = 0; ch < 3; ++ch) {
		if (INA3221_ReadVoltage(&ina, ch+1, &busVoltage_V[ch], &shuntVoltage_V[ch]) == HAL_OK) {
			current_mA[ch] = INA3221_CalculateCurrent_mA(&ina, ch+1, shuntVoltage_V[ch]);
			power_mW[ch] = INA3221_CalculatePower_mW(busVoltage_V[ch], current_mA[ch]);
		}
		else printf("Error al leer el INA3221\r\n\r\n");
	}

	for (int s = 0; s < 3; ++s) {
		status[s] = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3 << s);
	}

	if (SHT3X_ReadSingleShot(&sht, &airTemp_C, &airHummidity_perc) == HAL_OK) {
		dewPoint_C[0] = airTemp_C - (100 - airHummidity_perc)/5;
		dewPoint_C[1] = SHT3X_CalculateDewpoint_Simple(airTemp_C, airHummidity_perc);
		dewPoint_C[2] = SHT3X_CalculateDewpoint(airTemp_C, airHummidity_perc);
	}
	else printf("Error al leer el SHT3x\r\n\r\n");

	if (DS18B20_ReadTemperature(&dfr, &soilTemp_C) == HAL_OK) {

	}
	else printf("Error al leer el SEN0308\r\n\r\n");

	if (SEN0308_ReadRawAvg(&sen, &rawMoisture, 10) == HAL_OK) {
		soilMoisture_perc = SEN0308_CalculateRelative(&sen, rawMoisture);
	}
	else printf("Error al leer el SEN0308\r\n\r\n");

	printData();
	//printDataCSV();

	HAL_GPIO_TogglePin(GPIOB, LD1_Pin);
	HAL_Delay(1000);
}

void printData() {
	printf("%02d:%02d:%02d - %02d/%02d/20%02d\r\n", time.Hours, time.Minutes, time.Seconds, date.Date, date.Month, date.Year);

	printf("\r\n");

	printf("Status[0]: %u, Status[1]: %u, Status[2]: %u\r\n", status[0], status[1], status[2]);

	printf("\r\n");

	printf("Total: %u, Visible: %u, Infrared: %u, Lux: %.2f, Irradiance: %.2f\r\n", full, full-ir, ir, lux, irradiance_Wm2);

	printf("\r\n");

	for (int ch = 0; ch < 3; ++ch) {
		printf("CH%d: Bus Voltage: %.3f V, Shunt Voltage: %.2f mV, Current: %.2f mA, Power: %.2f mW\r\n",
				ch+1, busVoltage_V[ch], shuntVoltage_V[ch]*1000, current_mA[ch], power_mW[ch]);
	}

	printf("\r\n");

	printf("Air Temperature: %.2fºC, Air Hummidity: %.2f\%%, Dew point: %.2fºC | %.2fºC | %.2fºC\r\n", airTemp_C, airHummidity_perc, dewPoint_C[0],dewPoint_C[1],dewPoint_C[2]);

	printf("\r\n");

	printf("Soil temperature: %.2fºC\r\n", soilTemp_C);

	printf("\r\n");

	printf("Raw Moisture: %u, Soil Moisture: %u\%%\r\n", rawMoisture, soilMoisture_perc);

	printf("\r\n");
}

void printHeaderCSV() {
	printf("timestamp,status_0,status_1,status_2,voltage_PV,current_PV,power_PV,voltage_BAT,current_BAT,power_BAT,voltage_LOAD,current_LOAD,power_LOAD,total,ir,lux,irradiance");

	printf("\r\n");
}

void printDataCSV() {
	printf("%02d:%02d:%02d", time.Hours, time.Minutes, time.Seconds);

	printf(",%d,%d,%d", status[0], status[1], status[2]);

	for (int ch = 0; ch < 3; ++ch) {
		printf(",%.3f,%.2f,%.2f", busVoltage_V[ch], current_mA[ch], power_mW[ch]);
	}

	printf(",%u,%u,%.2f,%.2f", full, ir, lux, irradiance_Wm2);

	printf("\r\n");
}

void I2C_bus_scan() {
	for (uint8_t i = 0; i < 128; i++) {
		if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i<<1), 3, 5) == HAL_OK) printf("%2x ", i);
		else printf("-- ");

		if (i > 0 && (i + 1) % 16 == 0) printf("\r\n");
	}

	printf("\r\n");
}
