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
extern UART_HandleTypeDef huart1;

void SystemClock_Config();
void ADC1_Init();

static void RTC_Wakeup_Config(uint16_t time_s);
static void EnterStop2();

void InitFRAM();
void InitINA3221();
void InitTSL2591();
void InitSHT3X();
void InitDFR0198();
void InitSEN0308();

void ReadRTC();
void ReadINA3221();
void ReadTSL2591();
void ReadSHT3X();
void ReadDFR0198();
void ReadSEN0308();

void DumpFRAM();

void I2C_bus_scan();

uint16_t cycle = 1;

volatile uint8_t g_wakeRTC = 1;
volatile uint8_t g_wakeEXTI = 0;
volatile uint16_t g_extiPin = 0;

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

uint8_t airHumidity_perc;
uint8_t soilMoisture_perc;

uint16_t batteryVoltage_mV;
float pvVoltage_V;

// INTERRUPTIONS ------------------------------------------------------------

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc) {
	g_wakeRTC = 1;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	switch(GPIO_Pin) {
		case PV_INA_Pin:
		case CRI_INA_Pin:
		case WAR_INA_Pin:
		case S0_AEM_Pin:
		case S1_AEM_Pin:
			g_wakeEXTI = 1;
			g_extiPin = GPIO_Pin;
			break;
	}
}

// MAIN FUNCTIONS -----------------------------------------------------------

void setup() {
	//I2C_bus_scan();

	time.Hours = 10;
	time.Minutes = 45;
	time.Seconds = 0;
	if (HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) Error_Handler();

	date.Date = 8;
	date.Month = RTC_MONTH_JANUARY;
	date.Year = 26;

	if (HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK) Error_Handler();

	InitFRAM();
	FRAM_Reset(&mem);
	printf("Ciclo,Hum Suelo (),Temp Suelo (C)\r\n");

	InitINA3221();
	InitTSL2591();
	InitSHT3X();
	InitDFR0198();
	InitSEN0308();

	RTC_Wakeup_Config(10);

	HAL_Delay(500);
}

void loop() {
	if (g_wakeRTC) {
		g_wakeRTC = 0;

		ReadRTC();
		ReadINA3221();
		ReadTSL2591();
		ReadSHT3X();
		ReadDFR0198();
		ReadSEN0308();

		DataSample_t data = {
			.irradiance_Wm2 = irradiance_Wm2,
			.airTemp_C = airTemp_C,
			.soilTemp_C = soilTemp_C,
			.airHumidity_perc = airHumidity_perc,
			.soilMoisture_perc = soilMoisture_perc,
			.batteryVoltage_mV = batteryVoltage_mV,
			.hours = time.Hours,
			.minutes = time.Minutes,
			.seconds = time.Seconds,
			.day = date.Date,
			.month = date.Month,
			.year = date.Year
		};

		FRAM_SaveData(&mem, &data);

		DataSample_t rx = {0};
		uint8_t valid = 0;

		FRAM_GetSlot(&mem, mem.write_idx-1, &rx, &valid);

		if (valid) {/*
			printf("FRAM Slot: %u\r\n", mem.write_idx-1);
			printf("%02d/%02d/20%02d %02d:%02d:%02d\r\n", rx.day, rx.month, rx.year, rx.hours, rx.minutes, rx.seconds);
			printf("Battery: %.3f V\r\n", rx.batteryVoltage_mV/1000.0);
			printf("Irradiance: %.3f W/m2\r\n", rx.irradiance_Wm2);
			printf("Air: %.3f ºC, %u %%\r\n", rx.airTemp_C, rx.airHumidity_perc);
			printf("Soil: %.3f ºC, %u %%\r\n", rx.soilTemp_C, rx.soilMoisture_perc);*/

			printf("%u,%u,%.3f\r\n", cycle++, rx.soilMoisture_perc, rx.soilTemp_C);
		}
		else printf("Slot not valid\r\n");
	}
	else if (g_wakeEXTI) {
		g_wakeEXTI = 0;

		switch(g_extiPin) {
			case PV_INA_Pin:
				//
				break;
			case CRI_INA_Pin:
				//
				break;
			case WAR_INA_Pin:
				//
				break;
			case S0_AEM_Pin:
				//
				break;
			case S1_AEM_Pin:
				//
				break;
		}

		ReadRTC();
		printf("%02d:%02d:%02d", time.Hours, time.Minutes, time.Seconds);
		printf(" - EXTI%u\r\n", __builtin_ctz(g_extiPin));
	}

	EnterStop2();
}

// STOP2 MODE ---------------------------------------------------------------

static void RTC_Wakeup_Config(uint16_t time_s) {
  __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

  HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, time_s - 1, RTC_WAKEUPCLOCK_CK_SPRE_16BITS) != HAL_OK) Error_Handler();
}

static void EnterStop2() {
	HAL_GPIO_WritePin(GATE_SENS_GPIO_Port, GATE_SENS_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_RESET);

	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
	HAL_SuspendTick();

	HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

	HAL_ResumeTick();
	SystemClock_Config();

	ADC1_Init();

	HAL_GPIO_WritePin(GATE_SENS_GPIO_Port, GATE_SENS_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(USER_LED_GPIO_Port, USER_LED_Pin, GPIO_PIN_SET);

	InitINA3221();
	InitTSL2591();
	InitSHT3X();
	InitDFR0198();
	InitSEN0308();

	HAL_Delay(500);
}

// INIT ---------------------------------------------------------------------

void InitFRAM() {
	fram.hspi = &hspi2;
	fram.cs_port = CS_FRAM_GPIO_Port;
	fram.cs_pin = CS_FRAM_Pin;

	mem.fram = &fram;

	if (FRAM_Init(&mem) == HAL_OK);// printf("FRAM inicializada correctamente\r\n");
	else printf("FRAM no inicializada\r\n");
}

void InitINA3221() {
	ina.hi2c = &hi2c3;
	ina.shuntResistance[0] = 0.220f;
	ina.shuntResistance[1] = 0.220f;
	ina.shuntResistance[2] = 0.220f;
	ina.averagingMode = INA3221_AVG_64;
	ina.convTimeBus = INA3221_CT_1100us;
	ina.convTimeShunt = INA3221_CT_1100us;
	ina.operatingMode = INA3221_MODE_SHUNT_BUS_CONTINUOUS;

	if (INA3221_Init(&ina) == HAL_OK);// printf("INA3221 inicializado correctamente\r\n");
	//else printf("INA3221 no inicializado\r\n");
}

void InitTSL2591() {
	tsl.hi2c = &hi2c3;
	tsl.gain = TSL2591_GAIN_LOW;
	tsl.integrationTime = TSL2591_INTEGRATION_200MS;

	if (TSL2591_Init(&tsl) == HAL_OK);// printf("TSL2591 inicializado correctamente\r\n");
	//else printf("TSL2591 no inicializado\r\n");
}

void InitSHT3X() {
	sht.hi2c = &hi2c3;
	sht.clockStretch = SHT3X_NOSTRETCH;
	sht.repeatability = SHT3X_REPEAT_MED;

	if (SHT3X_Init(&sht) == HAL_OK);// printf("SHT3x inicializado correctamente\r\n");
	//else printf("SHT3x no inicializado\r\n");

	SHT3X_Heater(&sht, SHT3X_HEATER_OFF);
}

void InitDFR0198()  {
	dfr.port = DFR0198_GPIO_Port;
	dfr.pin = DFR0198_Pin;
	dfr.resolution = DS18B20_RES_12_BIT;

	if (DS18B20_Init(&dfr) == HAL_OK);// printf("DFR0198 inicializado correctamente\r\n");
	//else printf("DFR0198 no inicializado\r\n");
}

void InitSEN0308() {
	sen.hadc = &hadc1;
	sen.port = SEN0308_GPIO_Port;
	sen.pin = SEN0308_Pin;
	sen.airRaw = 3620;
	sen.waterRaw = 520;
	if (SEN0308_Init(&sen) == HAL_OK);// printf("SEN0308 inicializado correctamente\r\n");
	//else printf("SEN0308 no inicializado\r\n");
}

// READ ---------------------------------------------------------------------

void ReadRTC() {
	// Reads RTC time
	if (HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) == HAL_OK) {

	}
	// Error while reading RTC time
	else {
		//printf("Error while reading RTC time\r\n");
	}

	// Reads RTC date
	if (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) == HAL_OK) {

	}
	// Error while reading RTC date
	else {
		//printf("Error while reading RTC date\r\n");
	}
}

void ReadINA3221() {
	float busVoltage_V[3];
	float shuntVoltage_V[3];
	float current_mA[3];

	// Iterates the 3 channels
	for (int ch = 0; ch < 3; ++ch) {
		// Reads bus & shunt voltages
		if (INA3221_ReadVoltage(&ina, ch+1, &busVoltage_V[ch], &shuntVoltage_V[ch]) == HAL_OK) {
			// Calculates current
			current_mA[ch] = INA3221_CalculateCurrent_mA(&ina, ch+1, shuntVoltage_V[ch]);

			//printf("CH%u: %f V, %f mV, %f mA\r\n", ch, busVoltage_V[ch], shuntVoltage_V[ch]*1000, current_mA[ch]);
		}
		// Error while reading the sensor
		else {
			//printf("Error while reading INA3221\r\n\r\n");

			return;
		}
	}

	batteryVoltage_mV = (uint16_t)(busVoltage_V[1] * 1000);
	pvVoltage_V = busVoltage_V[0];
}

void ReadTSL2591() {
	uint16_t full, ir;
	float lux;

	// Reads total & ir
	if (TSL2591_ReadChannels(&tsl, &full, &ir) == HAL_OK) {
		// Calculates lux
		lux = TSL2591_CalculateLux(&tsl, full, ir);

		// Calculates irradiance
		irradiance_Wm2 = TSL2591_CalculateIrradiance(lux);
	}
	// Error while reading the sensor
	else {
		//printf("Error while reading TSL2591\r\n\r\n");
	}
}

void ReadSHT3X() {
	float airHumidity_perc_f, dewPoint_C;

	for (int8_t tries = 3; tries >= 0; --tries) {
		// Reads air temperature & hummidity
		if (SHT3X_ReadSingleShot(&sht, &airTemp_C, &airHumidity_perc_f) == HAL_OK) {
			airHumidity_perc = (uint8_t)airHumidity_perc_f;
			// Calculates dew point
			dewPoint_C = SHT3X_CalculateDewpoint(airTemp_C, airHumidity_perc);
		}
		// Error while reading the sensor
		else {
			//printf("Error while reading SHT3x\r\n\r\n");

			return;
		}

		// RH > 95% or T-Td < 1ºC - Activate heater
		if (airHumidity_perc > 90.0f || airTemp_C - dewPoint_C < 1.0f) {
			printf("Heater ON");
			SHT3X_Heater(&sht, SHT3X_HEATER_ON);
			HAL_Delay(500);
			SHT3X_Heater(&sht, SHT3X_HEATER_OFF);
			printf("Heater OFF");
		}
		// Valid reading
		else {
			return;
		}
	}

	// Limit tries
}

void ReadDFR0198() {
	// Reads soil temperature
	if (DS18B20_ReadTemperature(&dfr, &soilTemp_C) == HAL_OK) {

	}
	// Error while reading the sensor
	else {
		//printf("Error while reading DFR0198\r\n\r\n");
	}
}

void ReadSEN0308() {
	uint16_t rawMoisture;

	// Reads soil moisture
	if (SEN0308_ReadRawAvg(&sen, &rawMoisture, 5) == HAL_OK) {
		soilMoisture_perc = SEN0308_CalculateRelative(&sen, rawMoisture);
	}
	// Error while reading the sensor
	else {
		//printf("Error while reading SEN0308\r\n\r\n");
	}
}

// Dump

void DumpFRAM(void) {
    printf("Ciclo,Fecha,Hora,Slot Memoria,Slot Valido,Bateria (V),Irradiancia (W/m2),Temp Aire (C),Hum Aire (%),Temp Suelo (C),Hum Suelo (%)\r\n");

    for (uint16_t i = 0; i < mem.count; i++) {
        DataSample_t rx = {0};
        uint8_t valid = 0;

        uint16_t slot = (mem.write_idx + FRAM_DATA_SLOTS - mem.count + i) % FRAM_DATA_SLOTS;

        FRAM_GetSlot(&mem, slot, &rx, &valid);

        printf("%u", i);
        if (valid) {
            printf(",%02u/%02u/20%02u", rx.day, rx.month, rx.year);
            printf(",%02u:%02u:%02u", rx.hours, rx.minutes, rx.seconds);
            printf(",%u,%u", slot, valid);
            printf(",%.3f", rx.batteryVoltage_mV / 1000.0);
            printf(",%.3f", rx.irradiance_Wm2);
            printf(",%.3f,%u", rx.airTemp_C, rx.airHumidity_perc);
            printf(",%.3f,%u", rx.soilTemp_C, rx.soilMoisture_perc);
        }
        else printf(",,,%u,%u,,,,", slot, valid);

        printf("\r\n");
    }
}


// I2C bus scan
void I2C_bus_scan() {
	for (uint8_t i = 0; i < 128; i++) {
		if (HAL_I2C_IsDeviceReady(&hi2c3, (uint16_t)(i<<1), 3, 5) == HAL_OK) printf("%2x ", i);
		else printf("-- ");

		if (i > 0 && (i + 1) % 16 == 0) printf("\r\n");
	}

	printf("\r\n");
}
