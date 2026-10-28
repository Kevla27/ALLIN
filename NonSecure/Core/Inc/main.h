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
#include "stm32u5xx_hal.h"

#include "secure_nsc.h" /* For export Non-secure callable APIs */

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
#define SENSOR_MAGNET_Pin GPIO_PIN_4
#define SENSOR_MAGNET_GPIO_Port GPIOE
#define TAMP_TERM_COVER_Pin GPIO_PIN_6
#define TAMP_TERM_COVER_GPIO_Port GPIOE
#define TAMP_CASE_OPEN_Pin GPIO_PIN_13
#define TAMP_CASE_OPEN_GPIO_Port GPIOC
#define TCXO_32K_IN_Pin GPIO_PIN_14
#define TCXO_32K_IN_GPIO_Port GPIOC
#define OSC_IN_Pin GPIO_PIN_0
#define OSC_IN_GPIO_Port GPIOH
#define OSC_OUT_Pin GPIO_PIN_1
#define OSC_OUT_GPIO_Port GPIOH
#define LCD_I2C3_SCL_Pin GPIO_PIN_0
#define LCD_I2C3_SCL_GPIO_Port GPIOC
#define LCD_I2C3_SDA_Pin GPIO_PIN_1
#define LCD_I2C3_SDA_GPIO_Port GPIOC
#define VCAP_SENSE_Pin GPIO_PIN_2
#define VCAP_SENSE_GPIO_Port GPIOC
#define AC_FAIL_DET_Pin GPIO_PIN_3
#define AC_FAIL_DET_GPIO_Port GPIOC
#define AC_FAIL_DET_EXTI_IRQn EXTI3_IRQn
#define BTN_SCROLL_UP_Pin GPIO_PIN_0
#define BTN_SCROLL_UP_GPIO_Port GPIOA
#define BTN_SCROLL_UP_EXTI_IRQn EXTI0_IRQn
#define MODEM_TX_Pin GPIO_PIN_2
#define MODEM_TX_GPIO_Port GPIOA
#define MODEM_RX_Pin GPIO_PIN_3
#define MODEM_RX_GPIO_Port GPIOA
#define ADE_NSS_Pin GPIO_PIN_4
#define ADE_NSS_GPIO_Port GPIOA
#define ADE_SCK_Pin GPIO_PIN_5
#define ADE_SCK_GPIO_Port GPIOA
#define ADE_MISO_Pin GPIO_PIN_6
#define ADE_MISO_GPIO_Port GPIOA
#define ADE_MOSI_Pin GPIO_PIN_7
#define ADE_MOSI_GPIO_Port GPIOA
#define OPTICAL_TX_Pin GPIO_PIN_4
#define OPTICAL_TX_GPIO_Port GPIOC
#define OPTICAL_RX_Pin GPIO_PIN_5
#define OPTICAL_RX_GPIO_Port GPIOC
#define ADE_IRQ1_Pin GPIO_PIN_1
#define ADE_IRQ1_GPIO_Port GPIOB
#define ADE_IRQ1_EXTI_IRQn EXTI1_IRQn
#define RTC_CAL_OUT_Pin GPIO_PIN_2
#define RTC_CAL_OUT_GPIO_Port GPIOB
#define ADE_RESET_Pin GPIO_PIN_7
#define ADE_RESET_GPIO_Port GPIOE
#define ADE_CF_ZX_Pin GPIO_PIN_8
#define ADE_CF_ZX_GPIO_Port GPIOE
#define ADE_CF_ZX_EXTI_IRQn EXTI8_IRQn
#define ADE_PM0_Pin GPIO_PIN_9
#define ADE_PM0_GPIO_Port GPIOE
#define ADE_PM1_Pin GPIO_PIN_10
#define ADE_PM1_GPIO_Port GPIOE
#define ADE_EVENT_Pin GPIO_PIN_11
#define ADE_EVENT_GPIO_Port GPIOE
#define ADE_EVENT_EXTI_IRQn EXTI11_IRQn
#define ADE_CF1_Pin GPIO_PIN_12
#define ADE_CF1_GPIO_Port GPIOE
#define ADE_CF1_EXTI_IRQn EXTI12_IRQn
#define ADE_CF2_Pin GPIO_PIN_13
#define ADE_CF2_GPIO_Port GPIOE
#define ADE_CF2_EXTI_IRQn EXTI13_IRQn
#define FLASH_SPI2_CS_Pin GPIO_PIN_12
#define FLASH_SPI2_CS_GPIO_Port GPIOB
#define FLASH_SPI2_SCK_Pin GPIO_PIN_13
#define FLASH_SPI2_SCK_GPIO_Port GPIOB
#define FLASH_SPI2_MISO_Pin GPIO_PIN_14
#define FLASH_SPI2_MISO_GPIO_Port GPIOB
#define FLASH_SPI2_MOSI_Pin GPIO_PIN_15
#define FLASH_SPI2_MOSI_GPIO_Port GPIOB
#define AUX_CTRL_OUT1_Pin GPIO_PIN_8
#define AUX_CTRL_OUT1_GPIO_Port GPIOD
#define AUX_CTRL_IN1_Pin GPIO_PIN_10
#define AUX_CTRL_IN1_GPIO_Port GPIOD
#define AUX_CTRL_IN1_EXTI_IRQn EXTI10_IRQn
#define VBAT_SENSE_Pin GPIO_PIN_11
#define VBAT_SENSE_GPIO_Port GPIOD
#define EXT_WDI_PULSE_Pin GPIO_PIN_12
#define EXT_WDI_PULSE_GPIO_Port GPIOD
#define AUX_CTRL_IN2_Pin GPIO_PIN_14
#define AUX_CTRL_IN2_GPIO_Port GPIOD
#define AUX_CTRL_IN2_EXTI_IRQn EXTI14_IRQn
#define RELAY_STATUS_Pin GPIO_PIN_7
#define RELAY_STATUS_GPIO_Port GPIOC
#define RELAY_STATUS_EXTI_IRQn EXTI7_IRQn
#define RELAY_TRIP_Pin GPIO_PIN_8
#define RELAY_TRIP_GPIO_Port GPIOC
#define LCD_PWR_EN_Pin GPIO_PIN_2
#define LCD_PWR_EN_GPIO_Port GPIOD
#define LCD_BL_CTRL_Pin GPIO_PIN_3
#define LCD_BL_CTRL_GPIO_Port GPIOD
#define RS232_PWR_EN_Pin GPIO_PIN_4
#define RS232_PWR_EN_GPIO_Port GPIOD
#define MODEM_12V_EN_Pin GPIO_PIN_5
#define MODEM_12V_EN_GPIO_Port GPIOD
#define ADE_IRQ0_Pin GPIO_PIN_5
#define ADE_IRQ0_GPIO_Port GPIOB
#define ADE_IRQ0_EXTI_IRQn EXTI5_IRQn
#define BTN_SCROLL_DN_Pin GPIO_PIN_6
#define BTN_SCROLL_DN_GPIO_Port GPIOB
#define BTN_SCROLL_DN_EXTI_IRQn EXTI6_IRQn
#define LED_ALM_Pin GPIO_PIN_7
#define LED_ALM_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
