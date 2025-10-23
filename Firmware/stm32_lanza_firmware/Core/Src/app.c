/*
 * app.c
 *
 *  Created on: Oct 8, 2025
 *      Author: pzaragoza
 */

#include "stdio.h"

#include "app.h"
#include "main.h"

#include "TSL2591.h"
#include "INA3221.h"

void I2C_bus_scan();

extern I2C_HandleTypeDef hi2c1;
extern RTC_HandleTypeDef hrtc;

RTC_TimeTypeDef time;
RTC_DateTypeDef date;

TSL2591_t tsl;
uint16_t full, ir;
float lux;
float irradiance_Wm2;

INA3221_t ina;
float busVoltage_V[3];
float shuntVoltage_V[3];
float current_mA[3];
float power_mW[3];

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

}

void loop() {
	if (HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK) printf("Error al leer la hora\r\n");
	if (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK) printf("Error al leer la fecha\r\n");

	printf("%02d:%02d:%02d - %02d/%02d/20%02d\r\n", time.Hours, time.Minutes, time.Seconds, date.Date, date.Month, date.Year);

	printf("\r\n");

	if (TSL2591_ReadChannels(&tsl, &full, &ir) == HAL_OK) {
		lux = TSL2591_CalculateLux(&tsl, full, ir);
		irradiance_Wm2 = TSL2591_CalculateIrradiance(lux);

		printf("Total: %u, Visible: %u, Infrared: %u, Lux: %.2f, Irradiance: %.2f\r\n", full, full-ir, ir, lux, irradiance_Wm2);
	}
	else printf("Error al leer el TSL2591\r\n");

	printf("\r\n");

	for (int ch = 1; ch <= 3; ++ch) {
		if (INA3221_ReadVoltage(&ina, ch, &busVoltage_V[ch - 1], &shuntVoltage_V[ch - 1]) == HAL_OK) {
			current_mA[ch - 1] = INA3221_CalculateCurrent_mA(&ina, ch, shuntVoltage_V[ch - 1]);
			power_mW[ch - 1] = INA3221_CalculatePower_mW(busVoltage_V[ch - 1], current_mA[ch - 1]);

			printf("CH%d: Bus Voltage: %.3f V, Shunt Voltage: %.2f mV, Current: %.2f mA, Power: %.2f mW\r\n",
					ch, busVoltage_V[ch - 1], shuntVoltage_V[ch - 1]*1000, current_mA[ch - 1], power_mW[ch - 1]);
		}
		else printf("Error al leer el INA3221\r\n");
	}

	printf("\r\n");

	HAL_GPIO_TogglePin(GPIOB, LD1_Pin);
	HAL_Delay(500);
}

void I2C_bus_scan() {
	for (uint8_t i = 0; i < 128; i++) {
		if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(i<<1), 3, 5) == HAL_OK) printf("%2x ", i);
		else printf("-- ");

		if (i > 0 && (i + 1) % 16 == 0) printf("\r\n");
	}

	printf("\r\n");
}
