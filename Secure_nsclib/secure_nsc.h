/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    Secure_nsclib/secure_nsc.h
  * @author  MCD Application Team
  * @brief   Header for secure non-secure callable APIs list
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

/* USER CODE BEGIN Non_Secure_CallLib_h */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef SECURE_NSC_H
#define SECURE_NSC_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported types ------------------------------------------------------------*/
/**
  * @brief  non-secure callback ID enumeration definition
  */
typedef enum
{
  SECURE_FAULT_CB_ID     = 0x00U, /*!< System secure fault callback ID */
  GTZC_ERROR_CB_ID       = 0x01U  /*!< GTZC secure error callback ID */
} SECURE_CallbackIDTypeDef;
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void SECURE_RegisterCallback(SECURE_CallbackIDTypeDef CallbackId, void *func);

/* Smart Meter 3-Fasa Non-Secure Callable (NSC) Gateway APIs (Acuan & KONFIGURASI_MX) */
uint32_t SECURE_GetActiveEnergyWh(void);
uint32_t SECURE_GetReactiveEnergyVarh(void);
void     SECURE_RecordPulseCF1(void);
void     SECURE_RecordPulseCF2(void);
uint32_t SECURE_GetTamperStatusMask(void);
void     SECURE_ClearTamperLatch(uint32_t tamper_bitmask);
/* Format Register Persistensi Boot/Watchdog/Fault MCU (RTC_BKP_DR10 Domain VSW) */
#ifndef WDT_BOOT_REG_MAGIC
#define WDT_BOOT_REG_MAGIC          0xA5500000U
#define WDT_BOOT_REG_MAGIC_MASK     0xFFF00000U
#define WDT_BOOT_REG_SRC_SHIFT      16U
#define WDT_BOOT_REG_SRC_MASK       0x000F0000U
#define WDT_BOOT_REG_TASK_SHIFT     8U
#define WDT_BOOT_REG_TASK_MASK      0x0000FF00U
#define WDT_BOOT_REG_ERR04_BIT      (1U << 7)
#define WDT_BOOT_REG_PENDING_BIT    (1U << 6)
#define WDT_BOOT_REG_STREAK_MASK    0x0000000FU
#endif

uint32_t SECURE_GetRTCDateTime(void);
void     SECURE_SetRTCDateTime(uint32_t epoch_sec);
uint8_t  SECURE_IsLseClockHealthy(void);
uint32_t SECURE_ReadBootFaultReg(void);
void     SECURE_WriteBootFaultReg(uint32_t val);
void     SECURE_StampFatalFault(uint8_t fault_src);

#endif /* SECURE_NSC_H */
/* USER CODE END Non_Secure_CallLib_h */

