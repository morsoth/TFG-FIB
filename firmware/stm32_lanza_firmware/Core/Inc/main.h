/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32wbxx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PV_INA_Pin GPIO_PIN_0
#define PV_INA_GPIO_Port GPIOA
#define CRI_INA_Pin GPIO_PIN_1
#define CRI_INA_GPIO_Port GPIOA
#define WAR_INA_Pin GPIO_PIN_2
#define WAR_INA_GPIO_Port GPIOA
#define S0_AEM_Pin GPIO_PIN_3
#define S0_AEM_GPIO_Port GPIOA
#define S0_AEM_EXTI_IRQn EXTI3_IRQn
#define S1_AEM_Pin GPIO_PIN_4
#define S1_AEM_GPIO_Port GPIOA
#define S1_AEM_EXTI_IRQn EXTI4_IRQn
#define S2_AEM_Pin GPIO_PIN_5
#define S2_AEM_GPIO_Port GPIOA
#define GATE_SENS_Pin GPIO_PIN_6
#define GATE_SENS_GPIO_Port GPIOA
#define DFR0198_Pin GPIO_PIN_7
#define DFR0198_GPIO_Port GPIOA
#define SEN0308_Pin GPIO_PIN_8
#define SEN0308_GPIO_Port GPIOA
#define USER_LED_Pin GPIO_PIN_10
#define USER_LED_GPIO_Port GPIOA
#define CS_FRAM_Pin GPIO_PIN_11
#define CS_FRAM_GPIO_Port GPIOA
#define WP_FRAM_Pin GPIO_PIN_12
#define WP_FRAM_GPIO_Port GPIOC
#define USART_RX_Pin GPIO_PIN_6
#define USART_RX_GPIO_Port GPIOB
#define USART_TX_Pin GPIO_PIN_7
#define USART_TX_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
