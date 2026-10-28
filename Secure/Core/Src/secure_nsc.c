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
static volatile uint16_t s_sec_century_base   = 2000U;

#define SECURE_RTC_BKP_CENTURY_REG   RTC_BKP_DR2
#define SECURE_RTC_CENTURY_MAGIC     0xCE000000U /* "CE" = Century Signature */
#define SECURE_RTC_CENTURY_MASK      0xFFFF0000U

/**
  * @brief  Membaca dan memvalidasi basis abad (2000 / 2100) dari Backup Register RTC_BKP_DR2 (Domain VBAT)
  */
static uint16_t secure_rtc_get_century_base(void)
{
  uint32_t val = HAL_RTCEx_BKUPRead(&hrtc, SECURE_RTC_BKP_CENTURY_REG);
  if ((val & SECURE_RTC_CENTURY_MASK) == SECURE_RTC_CENTURY_MAGIC)
  {
    uint16_t century = (uint16_t)(val & 0xFFFFU);
    if (century == 2000U || century == 2100U)
    {
      s_sec_century_base = century;
      return s_sec_century_base;
    }
  }
  else if (val == 2000U || val == 2100U)
  {
    s_sec_century_base = (uint16_t)val;
    return s_sec_century_base;
  }

  /* Default jika domain VBAT belum pernah diinisialisasi */
  s_sec_century_base = 2000U;
  HAL_RTCEx_BKUPWrite(&hrtc, SECURE_RTC_BKP_CENTURY_REG, SECURE_RTC_CENTURY_MAGIC | (uint32_t)s_sec_century_base);
  return s_sec_century_base;
}

/**
  * @brief  Menyimpan basis abad ke Backup Register RTC_BKP_DR2 (Domain VBAT) dengan stempel magic
  */
static void secure_rtc_save_century_base(uint16_t century)
{
  s_sec_century_base = century;
  HAL_RTCEx_BKUPWrite(&hrtc, SECURE_RTC_BKP_CENTURY_REG, SECURE_RTC_CENTURY_MAGIC | (uint32_t)s_sec_century_base);
}

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

#ifndef BOARD_CFG_TAMPER_CASE_OPEN_LVL
#define BOARD_CFG_TAMPER_CASE_OPEN_LVL  1
#endif
#ifndef BOARD_CFG_TAMPER_TERM_OPEN_LVL
#define BOARD_CFG_TAMPER_TERM_OPEN_LVL  1
#endif
#ifndef BOARD_CFG_TAMPER_MAGNET_LVL
#define BOARD_CFG_TAMPER_MAGNET_LVL     1
#endif

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

  /* Periksa latch hardware TAMP->SR maupun status pin fisik Secure GPIO (PC13, PE6, PE4) */
  if (((sr & TAMP_SR_TAMP1F) != 0U) ||
      (HAL_GPIO_ReadPin(TAMP_CASE_OPEN_GPIO_Port, TAMP_CASE_OPEN_Pin) == (BOARD_CFG_TAMPER_CASE_OPEN_LVL ? GPIO_PIN_SET : GPIO_PIN_RESET)))
  {
    mask |= (1U << 0); /* TAMPER_VECTOR_CASE_OPEN */
  }
  if (((sr & TAMP_SR_TAMP3F) != 0U) ||
      (HAL_GPIO_ReadPin(TAMP_TERM_COVER_GPIO_Port, TAMP_TERM_COVER_Pin) == (BOARD_CFG_TAMPER_TERM_OPEN_LVL ? GPIO_PIN_SET : GPIO_PIN_RESET)))
  {
    mask |= (1U << 1); /* TAMPER_VECTOR_TERMINAL_OPEN */
  }
  if (HAL_GPIO_ReadPin(SENSOR_MAGNET_GPIO_Port, SENSOR_MAGNET_Pin) == (BOARD_CFG_TAMPER_MAGNET_LVL ? GPIO_PIN_SET : GPIO_PIN_RESET))
  {
    mask |= (1U << 2); /* TAMPER_VECTOR_MAGNETIC_FIELD */
  }
  else if ((sr & TAMP_SR_TAMP7F) != 0U)
  {
    /* Bersihkan flag latch TAMP7F saat medan magnet telah dijauhkan agar pemulihan 10s ERR25 bekerja */
    TAMP->SCR = TAMP_SCR_CTAMP7F;
  }
  return mask;
}

static uint32_t rtc_days_from_civil(int y, unsigned m, unsigned d)
{
  if (m <= 2U) y -= 1;
  int era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153U * (m > 2U ? m - 3U : m + 9U) + 2U) / 5U + d - 1U;
  unsigned doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
  return (uint32_t)(era * 146097 + (int)doe - 719468);
}

static void rtc_civil_from_days(uint32_t z, uint16_t *out_y, uint8_t *out_m, uint8_t *out_d)
{
  z += 719468U;
  uint32_t era = z / 146097U;
  uint32_t doe = z - era * 146097U;
  uint32_t yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U) / 365U;
  uint32_t y = yoe + era * 400U;
  uint32_t doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
  uint32_t mp = (5U * doy + 2U) / 153U;
  uint32_t d = doy - (153U * mp + 2U) / 5U + 1U;
  uint32_t m = (mp < 10U) ? (mp + 3U) : (mp - 9U);
  if (m <= 2U)
  {
    y += 1U;
  }
  if (out_y) *out_y = (uint16_t)y;
  if (out_m) *out_m = (uint8_t)m;
  if (out_d) *out_d = (uint8_t)d;
}

/**
  * @brief  Membaca waktu kalender dari Secure RTC dalam detik UNIX Epoch
  */
CMSE_NS_ENTRY uint32_t SECURE_GetRTCDateTime(void)
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* Pastikan basis abad selalu ter-sinkronisasi dengan Backup Register RTC_BKP_DR2 (Domain VBAT) */
  uint16_t century_base = secure_rtc_get_century_base();

  /* Catatan: Pada STM32 HAL RTC, GetTime harus selalu dipanggil sebelum GetDate
   * agar shadow registers RTC ter-unlock dengan benar */
  if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK &&
      HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) == HAL_OK)
  {
    if (sDate.Month >= 1U && sDate.Month <= 12U && sDate.Date >= 1U && sDate.Date <= 31U)
    {
      /* Deteksi transisi abad otonom: Jika meter beroperasi melewati 2099 menuju 2100 (Year = 00..19) */
      if (century_base == 2000U && sDate.Year < 20U)
      {
        century_base = 2100U;
        secure_rtc_save_century_base(century_base);
      }

      uint32_t days = rtc_days_from_civil((int)century_base + (int)sDate.Year, (unsigned)sDate.Month, (unsigned)sDate.Date);
      uint32_t day_sec = ((uint32_t)sTime.Hours * 3600UL) +
                         ((uint32_t)sTime.Minutes * 60UL) +
                         (uint32_t)sTime.Seconds;
      uint32_t epoch = (days * 86400UL) + day_sec;
      s_sec_epoch_override = epoch;
      return epoch;
    }
  }
  return s_sec_epoch_override;
}

/**
  * @brief  Menyinkronkan waktu kalender Secure RTC dari perintah DLMS/COSEM Clock atau Konsol CLI
  */
CMSE_NS_ENTRY void SECURE_SetRTCDateTime(uint32_t epoch_sec)
{
  s_sec_epoch_override = epoch_sec;

  uint16_t y = 2026;
  uint8_t m = 10, d = 28;
  rtc_civil_from_days(epoch_sec / 86400UL, &y, &m, &d);

  uint16_t new_century = (y >= 2100U) ? 2100U : 2000U;
  secure_rtc_save_century_base(new_century);

  RTC_DateTypeDef sDate = {0};
  sDate.Year = (uint8_t)(y >= s_sec_century_base ? (y - s_sec_century_base) : 0U);
  sDate.Month = m;
  sDate.Date = d;
  sDate.WeekDay = (uint8_t)(((epoch_sec / 86400UL) + 3UL) % 7UL + 1UL); // 1 = Monday .. 7 = Sunday

  RTC_TimeTypeDef sTime = {0};
  uint32_t rem = epoch_sec % 86400UL;
  sTime.Hours   = (uint8_t)(rem / 3600UL);
  sTime.Minutes = (uint8_t)((rem % 3600UL) / 60UL);
  sTime.Seconds = (uint8_t)(rem % 60UL);

  (void)HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
  (void)HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
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
