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
#include "stm32f4xx_hal.h"

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
#define IIC_SCL_Pin GPIO_PIN_4
#define IIC_SCL_GPIO_Port GPIOA
#define IIC_SDA_Pin GPIO_PIN_5
#define IIC_SDA_GPIO_Port GPIOA
#define TRIG_Pin GPIO_PIN_6
#define TRIG_GPIO_Port GPIOA
#define ECHO_Pin GPIO_PIN_7
#define ECHO_GPIO_Port GPIOA
#define OLED_SCK_Pin GPIO_PIN_9
#define OLED_SCK_GPIO_Port GPIOE
#define OLED_MOSI_Pin GPIO_PIN_10
#define OLED_MOSI_GPIO_Port GPIOE
#define OLED_RES_Pin GPIO_PIN_11
#define OLED_RES_GPIO_Port GPIOE
#define OLED_DC_Pin GPIO_PIN_12
#define OLED_DC_GPIO_Port GPIOE
#define OLED_CS_Pin GPIO_PIN_13
#define OLED_CS_GPIO_Port GPIOE
#define LED_BLUE_Pin GPIO_PIN_12
#define LED_BLUE_GPIO_Port GPIOD
#define LED_RED_Pin GPIO_PIN_13
#define LED_RED_GPIO_Port GPIOD
#define LED_GREEN_Pin GPIO_PIN_14
#define LED_GREEN_GPIO_Port GPIOD
#define Buzzer_Pin GPIO_PIN_4
#define Buzzer_GPIO_Port GPIOB
#define KEY1_Pin GPIO_PIN_5
#define KEY1_GPIO_Port GPIOB
#define KEY2_Pin GPIO_PIN_6
#define KEY2_GPIO_Port GPIOB
#define KEY3_Pin GPIO_PIN_7
#define KEY3_GPIO_Port GPIOB
#define KEY4_Pin GPIO_PIN_8
#define KEY4_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
