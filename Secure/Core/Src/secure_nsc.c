/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    Secure/Src/secure_nsc.c
  * @author  MCD Application Team
  * @brief   This file contains the non-secure callable APIs (secure world)
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

/* USER CODE BEGIN Non_Secure_CallLib */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "secure_nsc.h"

/** @addtogroup STM32U5xx_HAL_Examples
  * @{
  */

/** @addtogroup Templates
  * @{
  */

/* Global variables ----------------------------------------------------------*/
void *pSecureFaultCallback = NULL;   /* Pointer to secure fault callback in Non-secure */
void *pSecureErrorCallback = NULL;   /* Pointer to secure error callback in Non-secure */

extern RTC_HandleTypeDef hrtc;

/* Private variables (Isolated in Secure SRAM1 & VSW Backup Domain) ----------*/
static volatile uint32_t s_sec_cf1_pulses     = 0U;
static volatile uint32_t s_sec_cf2_pulses     = 0U;
static volatile uint32_t s_sec_active_wh      = 0U;
static volatile uint32_t s_sec_reactive_varh  = 0U;
static volatile uint32_t s_sec_epoch_override = 1791794500UL;

/**
  * @brief  Secure registration of non-secure callback.
  * @param  CallbackId  callback identifier
  * @param  func        pointer to non-secure function
  * @retval None
  */
CMSE_NS_ENTRY void SECURE_RegisterCallback(SECURE_CallbackIDTypeDef CallbackId, void *func)
{
  if (func != NULL)
  {
    switch (CallbackId)
    {
      case SECURE_FAULT_CB_ID:           /* SecureFault Interrupt occurred */
        pSecureFaultCallback = func;
        break;
      case GTZC_ERROR_CB_ID:             /* GTZC Interrupt occurred */
        pSecureErrorCallback = func;
        break;
      default:
        /* unknown */
        break;
    }
  }
}

/**
  * @brief  Mencatat pulsa kalibrasi energi aktif CF1 (PE12) di Secure World
  *         Konstanta kalibrasi: 1.000 imp/kWh (1 imp = 1 Wh)
  *         Menyimpan nilai ke TAMP_BKP0R (Domain Baterai VSW) sesuai Desain Sistem Hal. 98
  */
CMSE_NS_ENTRY void SECURE_RecordPulseCF1(void)
{
  s_sec_cf1_pulses++;
  s_sec_active_wh = s_sec_cf1_pulses;
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, s_sec_active_wh);
}

/**
  * @brief  Mencatat pulsa kalibrasi energi reaktif CF2 (PE13) di Secure World
  *         Konstanta kalibrasi: 1.000 imp/kVARh (1 imp = 1 VARh)
  */
CMSE_NS_ENTRY void SECURE_RecordPulseCF2(void)
{
  s_sec_cf2_pulses++;
  s_sec_reactive_varh = s_sec_cf2_pulses;
  HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, s_sec_reactive_varh);
}

/**
  * @brief  Membaca akumulasi energi aktif independen (Wh) dari Secure World
  */
CMSE_NS_ENTRY uint32_t SECURE_GetActiveEnergyWh(void)
{
  uint32_t bkp_wh = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR0);
  if (bkp_wh > s_sec_active_wh)
  {
    s_sec_active_wh = bkp_wh;
  }
  return s_sec_active_wh;
}

/**
  * @brief  Membaca akumulasi energi reaktif independen (VARh) dari Secure World
  */
CMSE_NS_ENTRY uint32_t SECURE_GetReactiveEnergyVarh(void)
{
  uint32_t bkp_varh = HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
  if (bkp_varh > s_sec_reactive_varh)
  {
    s_sec_reactive_varh = bkp_varh;
  }
  return s_sec_reactive_varh;
}

/**
  * @brief  Membaca status hardware tamper latch dari peripheral TAMP (Domain VSW)
  *         - Bit 0: PC13 (TAMP_IN1 - Case Open)
  *         - Bit 1: PE6  (TAMP_IN3 - Terminal Cover Open)
  *         - Bit 2: PE4  (TAMP_IN7 - Magnetic Field <= 500 mT)
  */
CMSE_NS_ENTRY uint32_t SECURE_GetTamperStatusMask(void)
{
  uint32_t mask = 0U;
  uint32_t sr   = TAMP->SR;

  if ((sr & TAMP_SR_TAMP1F) != 0U)
  {
    mask |= (1U << 0); /* TAMPER_VECTOR_CASE_OPEN */
  }
  if ((sr & TAMP_SR_TAMP3F) != 0U)
  {
    mask |= (1U << 1); /* TAMPER_VECTOR_TERMINAL_OPEN */
  }
  if ((sr & TAMP_SR_TAMP7F) != 0U)
  {
    mask |= (1U << 2); /* TAMPER_VECTOR_MAGNETIC_FIELD */
  }
  return mask;
}

/**
  * @brief  Membaca waktu kalender dari Secure RTC dalam detik UNIX Epoch
  */
CMSE_NS_ENTRY uint32_t SECURE_GetRTCDateTime(void)
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK &&
      HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK)
  {
    /* Jika RTC sudah berjalan (tahun >= 26), hitung offset detik harian */
    uint32_t day_sec = ((uint32_t)sTime.Hours * 3600UL) +
                       ((uint32_t)sTime.Minutes * 60UL) +
                       (uint32_t)sTime.Seconds;
    uint32_t base_day_epoch = (s_sec_epoch_override / 86400UL) * 86400UL;
    return base_day_epoch + day_sec;
  }
  return s_sec_epoch_override;
}

/**
  * @brief  Menyinkronkan waktu kalender Secure RTC dari perintah DLMS/COSEM Clock
  */
CMSE_NS_ENTRY void SECURE_SetRTCDateTime(uint32_t epoch_sec)
{
  s_sec_epoch_override = epoch_sec;

  RTC_TimeTypeDef sTime = {0};
  uint32_t rem = epoch_sec % 86400UL;
  sTime.Hours   = (uint8_t)(rem / 3600UL);
  sTime.Minutes = (uint8_t)((rem % 3600UL) / 60UL);
  sTime.Seconds = (uint8_t)(rem % 60UL);
  (void)HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
}

/**
  * @brief  Memeriksa kesehatan osilator eksternal TCXO 32.768 kHz (PC14) via LSECSS
  *         Jika gagal (LSECSSD = 1), Non-Secure mengaktifkan alarm ERR02 (Clock Loss)
  * @retval 1 jika clock sehat, 0 jika terdeteksi kegagalan clock (ERR02)
  */
CMSE_NS_ENTRY uint8_t SECURE_IsLseClockHealthy(void)
{
  if ((RCC->BDCR & RCC_BDCR_LSECSSD) != 0U)
  {
    return 0U; /* Clock loss detected -> ERR02 */
  }
  return 1U;
}

/**
  * @}
  */

/**
  * @}
  */
/* USER CODE END Non_Secure_CallLib */
