/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32g0xx_hal.h"

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
#define BTN_PREV_Pin GPIO_PIN_9
#define BTN_PREV_GPIO_Port GPIOB
#define BTN_PREV_EXTI_IRQn EXTI4_15_IRQn
#define T_NRST_Pin GPIO_PIN_2
#define T_NRST_GPIO_Port GPIOF
#define T_NRST_EXTI_IRQn EXTI2_3_IRQn
#define ENC_A_Pin GPIO_PIN_0
#define ENC_A_GPIO_Port GPIOA
#define ENC_B_Pin GPIO_PIN_1
#define ENC_B_GPIO_Port GPIOA
#define SD_CS_Pin GPIO_PIN_4
#define SD_CS_GPIO_Port GPIOA
#define SPI1_SCK_Pin GPIO_PIN_5
#define SPI1_SCK_GPIO_Port GPIOA
#define SPI1_MISO_Pin GPIO_PIN_6
#define SPI1_MISO_GPIO_Port GPIOA
#define SPI1_MOSI_Pin GPIO_PIN_7
#define SPI1_MOSI_GPIO_Port GPIOA
#define BTN_PLAY_Pin GPIO_PIN_0
#define BTN_PLAY_GPIO_Port GPIOB
#define BTN_PLAY_EXTI_IRQn EXTI0_1_IRQn
#define LD3_Pin GPIO_PIN_6
#define LD3_GPIO_Port GPIOC
#define MP3_CS_Pin GPIO_PIN_11
#define MP3_CS_GPIO_Port GPIOA
#define XD_CS_Pin GPIO_PIN_12
#define XD_CS_GPIO_Port GPIOA
#define T_JTMS_Pin GPIO_PIN_13
#define T_JTMS_GPIO_Port GPIOA
#define T_JTCK_Pin GPIO_PIN_14
#define T_JTCK_GPIO_Port GPIOA
#define DREQ_Pin GPIO_PIN_3
#define DREQ_GPIO_Port GPIOB
#define LED_PAUSE_Pin GPIO_PIN_4
#define LED_PAUSE_GPIO_Port GPIOB
#define LED_PLAY_Pin GPIO_PIN_5
#define LED_PLAY_GPIO_Port GPIOB
#define BTN_NEXT_Pin GPIO_PIN_8
#define BTN_NEXT_GPIO_Port GPIOB
#define BTN_NEXT_EXTI_IRQn EXTI4_15_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
