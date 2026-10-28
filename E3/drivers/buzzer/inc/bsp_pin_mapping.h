/**
 * @file bsp_pin_mapping.h
 * @brief Pin mapping definition for BSP Alarm Buzzer / LED (PB7)
 */

#ifndef BSP_PIN_MAPPING_H
#define BSP_PIN_MAPPING_H

#include "main.h"

#define BSP_BUZZER_CLK_ENABLE() __HAL_RCC_GPIOB_CLK_ENABLE()
#define BSP_BUZZER_GPIO_PORT    LED_ALM_GPIO_Port
#define BSP_BUZZER_PIN          LED_ALM_Pin

#endif /* BSP_PIN_MAPPING_H */

