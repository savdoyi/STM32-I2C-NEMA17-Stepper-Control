/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  * This file contains the common defines of the application.
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
#include "stm32f1xx_hal.h"

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
#define Enable_1_Pin GPIO_PIN_14
#define Enable_1_GPIO_Port GPIOC
#define Direction_1_Pin GPIO_PIN_15
#define Direction_1_GPIO_Port GPIOC
#define Pulse_1_Pin GPIO_PIN_0
#define Pulse_1_GPIO_Port GPIOA
#define Enable_2_Pin GPIO_PIN_12
#define Enable_2_GPIO_Port GPIOB
#define Direction_2_Pin GPIO_PIN_13
#define Direction_2_GPIO_Port GPIOB
#define Pulse_2_Pin GPIO_PIN_14
#define Pulse_2_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define MAX_DATA_SIZE 16 // ESP32 dan keladigan ma'lumotlar massivining maksimal hajmi
#define MS_32 6400      // Drayverga bog'liq: 1 aylanishdagi microstepslar soni (masalan, 32 microstep uchun 200 * 32 = 6400)
#define MS_16 3200
#define MS_8 1600
#define MS_4 800
#define MS_2 400

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
