/**
 * @file bsp_power_mode.c
 * @brief Implementasi Driver Manajemen Hemat Daya Ekstrem STM32U5 (Stop 2 Mode saat Listrik Padam)
 * @details Mengimplementasikan transisi daya hemat baterai ekstrem untuk meter 3-fasa.
 *          Saat listrik PLN padam (AC Fail) dan timer layar 10 detik habis, CPU STM32U5
 *          dimasukkan ke Stop 2 Mode via HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI).
 *          Konsumsi arus turun drastis dari ~19 mA (Run 160MHz) ke ~4.5 uA (Stop 2).
 *
 * Target: STM32U585VIT6 (ARM Cortex-M33 TrustZone) & Host Test Native
 */

#include "bsp_power_mode.h"
#include "board_profile_config.h"
#include <stdio.h>
#include <string.h>

#if defined(EMBEDDED_HARDWARE_TARGET)
#include "stm32u5xx_hal.h"
#include "main.h"
#include "cmsis_os2.h"
#include "secure_nsc.h"
#include "e3_meter_app.h"

extern ADC_HandleTypeDef hadc4;
#endif

/* -------------------------------------------------------------------------- */
/* Variabel Internal State Machine Manajemen Daya                              */
/* -------------------------------------------------------------------------- */
static volatile meter_power_mode_t   s_current_mode       = METER_POWER_MODE_RUN;
static volatile meter_wakeup_reason_t s_last_wakeup_reason = METER_WAKEUP_REASON_NONE;
static volatile uint32_t             s_wakeup_counter     = 0U;
static volatile bool                 s_is_ac_failed       = false;

#if !defined(EMBEDDED_HARDWARE_TARGET)
static meter_wakeup_reason_t         s_mock_next_wakeup   = METER_WAKEUP_REASON_BUTTON_PRESS;
#endif

/* -------------------------------------------------------------------------- */
/* Implementasi Fungsi API Publik                                             */
/* -------------------------------------------------------------------------- */

void bsp_power_mode_init(void)
{
    s_current_mode       = METER_POWER_MODE_RUN;
    s_last_wakeup_reason = METER_WAKEUP_REASON_NONE;
    s_wakeup_counter     = 0U;
    s_is_ac_failed       = false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    /* 1. Konfigurasi Wake-up Clock dari Stop Mode ke MSI (default 4 MHz) */
    __HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_MSI);

    /* 2. Pastikan retensi penuh SRAM1, SRAM2, SRAM3, SRAM4 di Stop 2 aktif (PDS bits = 0) */
    CLEAR_BIT(PWR->CR2, PWR_CR2_SRAM1PDS1 | PWR_CR2_SRAM1PDS2 | PWR_CR2_SRAM1PDS3 |
                        PWR_CR2_SRAM2PDS1 | PWR_CR2_SRAM2PDS2 | PWR_CR2_SRAM4PDS);
    SET_BIT(PWR->CR2, PWR_CR2_SRAM4FWU);

    /* 3. Konfigurasi Ultra-Low-Power (ULP) mode */
    HAL_PWREx_EnableUltraLowPowerMode();
#endif
}

meter_power_mode_t bsp_power_mode_get_current(void)
{
    return s_current_mode;
}

const char *bsp_power_mode_get_name(meter_power_mode_t mode)
{
    switch (mode)
    {
        case METER_POWER_MODE_RUN:
            return "RUN_160MHZ_AC_MAINS";
        case METER_POWER_MODE_BATTERY_ACTIVE:
            return "BATTERY_ACTIVE_MSIS_4MHZ";
        case METER_POWER_MODE_STOP2:
            return "STOP2_STANDBY_ULTRA_LOW_POWER";
        default:
            return "UNKNOWN_POWER_MODE";
    }
}

meter_wakeup_reason_t bsp_power_mode_get_last_wakeup_reason(void)
{
    return s_last_wakeup_reason;
}

uint32_t bsp_power_mode_get_wakeup_count(void)
{
    return s_wakeup_counter;
}

void bsp_power_mode_set_ac_failed(bool failed)
{
    s_is_ac_failed = failed;
    if (!failed)
    {
        s_current_mode = METER_POWER_MODE_RUN;
    }
}

bool bsp_power_mode_is_ac_failed(void)
{
    return s_is_ac_failed;
}

void bsp_power_restore_run_clock(void)
{
#if defined(EMBEDDED_HARDWARE_TARGET)
    /* 1. Atur Voltage Scaling ke Scale 1 untuk performa frekuensi tinggi 160 MHz */
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        /* Fallback bila gagal */
    }

    /* 2. Aktifkan kembali PLL1 dengan sumber MSI (4 MHz input) -> 160 MHz output */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4; /* 4 MHz */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 80; /* 4 MHz * 80 = 320 MHz VCO */
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;  /* 320 MHz / 2 = 160 MHz PLLCLK */
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) == HAL_OK)
    {
        RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
        RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                    | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                    | RCC_CLOCKTYPE_PCLK3;
        RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
        RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
        RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
        RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
        RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

        (void)HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
        SystemCoreClockUpdate();
    }
#endif
    s_current_mode = METER_POWER_MODE_RUN;
}

void bsp_power_set_battery_clock_msis(void)
{
#if defined(EMBEDDED_HARDWARE_TARGET)
    /* Pastikan clock sistem menggunakan MSIS 4 MHz dan Voltage Scale 4 untuk konsumsi arus ultra-rendah */
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                | RCC_CLOCKTYPE_PCLK3;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
    (void)HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);

    /* Matikan PLL jika sebelumnya aktif untuk menghemat daya baterai */
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_NONE;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_OFF;
    (void)HAL_RCC_OscConfig(&RCC_OscInitStruct);

    (void)HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE4);
    SystemCoreClockUpdate();
#endif
    s_current_mode = METER_POWER_MODE_BATTERY_ACTIVE;
}

void bsp_power_configure_rtc_wakeup_timer(uint32_t period_sec)
{
    (void)period_sec;
#if defined(EMBEDDED_HARDWARE_TARGET)
    /* RTC Wakeup timer dikonfigurasi melalui antarmuka Secure RTC */
#endif
}

meter_wakeup_reason_t bsp_power_enter_stop2(void)
{
#if (BOARD_CFG_ENABLE_STOP2_MODE == 0)
    /* Jika mode Stop 2 dinonaktifkan di konfigurasi papan, abaikan */
    return METER_WAKEUP_REASON_NONE;
#endif

#if defined(EMBEDDED_HARDWARE_TARGET)
    /* 1. Heartbeat Denyut Terakhir External Watchdog (PD12) sebelum tidur */
    HAL_GPIO_TogglePin(EXT_WDI_PULSE_GPIO_Port, EXT_WDI_PULSE_Pin);

    /* 2. Matikan konversi ADC4 analog agar tidak terjadi arus bocor input */
    if (hadc4.Instance != NULL)
    {
        HAL_ADC_Stop(&hadc4);
    }

    /* 3. Pastikan seluruh rail periferal berdaya tinggi dalam kondisi mati */
    HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LCD_PWR_EN_GPIO_Port, LCD_PWR_EN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RS232_PWR_EN_GPIO_Port, RS232_PWR_EN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MODEM_12V_EN_GPIO_Port, MODEM_12V_EN_Pin, GPIO_PIN_RESET);

    /* 4. Nonaktifkan SysTick interrupt agar tidak membangunkan CPU tiap 1ms */
    SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk);
    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;

    s_current_mode = METER_POWER_MODE_STOP2;

    /* 5. MASUK KE MODE STOP 2 EKSTREM (~4.5 uA) - CPU BERHENTI DI SINI */
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

    /* ---------------------------------------------------------------------- */
    /* 6. CPU BANGUN DARI STOP 2 OLEH INTERRUPT EXTI ATAU RTC                */
    /* ---------------------------------------------------------------------- */

    /* Pulihkan SysTick interrupt */
    SysTick->CTRL |= (SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk);

    /* Denyut External Watchdog segera setelah bangun */
    HAL_GPIO_TogglePin(EXT_WDI_PULSE_GPIO_Port, EXT_WDI_PULSE_Pin);

    /* Evaluasi Sumber Pemicu Bangun (Wakeup Source Identification) */
    meter_wakeup_reason_t reason = METER_WAKEUP_REASON_NONE;

    /* Cek A: Apakah listrik PLN sudah pulih? (PC3 AC_FAIL_DET bernilai HIGH) */
    if (HAL_GPIO_ReadPin(AC_FAIL_DET_GPIO_Port, AC_FAIL_DET_Pin) == GPIO_PIN_SET)
    {
        reason = METER_WAKEUP_REASON_AC_RESTORED;
        s_is_ac_failed = false;
        bsp_power_restore_run_clock();
    }
    /* Cek B: Apakah tombol navigasi display ditekan? (PA0 UP atau PB6 DOWN bernilai LOW/Active-low) */
    else if ((HAL_GPIO_ReadPin(BTN_SCROLL_UP_GPIO_Port, BTN_SCROLL_UP_Pin) == GPIO_PIN_RESET) ||
             (HAL_GPIO_ReadPin(BTN_SCROLL_DN_GPIO_Port, BTN_SCROLL_DN_Pin) == GPIO_PIN_RESET))
    {
        reason = METER_WAKEUP_REASON_BUTTON_PRESS;
        s_current_mode = METER_POWER_MODE_BATTERY_ACTIVE;
        bsp_power_set_battery_clock_msis();
    }
    /* Cek C: Apakah sakelar proteksi tamper terpicu? (PC13, PE6, PE4 via Secure NSC) */
    else if (SECURE_GetTamperStatusMask() != 0U)
    {
        reason = METER_WAKEUP_REASON_TAMPER;
        s_current_mode = METER_POWER_MODE_STOP2;
    }
    /* Cek D: Timer RTC Periodik */
    else
    {
        reason = METER_WAKEUP_REASON_RTC_PERIODIC;
        s_current_mode = METER_POWER_MODE_STOP2;
    }

    s_wakeup_counter++;
    s_last_wakeup_reason = reason;
    return reason;

#else
    /* Simulasi Host Test Native (x86_64 / CTest) */
    s_current_mode = METER_POWER_MODE_STOP2;

    meter_wakeup_reason_t reason = s_mock_next_wakeup;
    if (reason == METER_WAKEUP_REASON_AC_RESTORED)
    {
        s_is_ac_failed = false;
        s_current_mode = METER_POWER_MODE_RUN;
    }
    else if (reason == METER_WAKEUP_REASON_BUTTON_PRESS)
    {
        s_current_mode = METER_POWER_MODE_BATTERY_ACTIVE;
    }
    else
    {
        s_current_mode = METER_POWER_MODE_STOP2;
    }

    s_wakeup_counter++;
    s_last_wakeup_reason = reason;
    return reason;
#endif
}

#if !defined(EMBEDDED_HARDWARE_TARGET)
void bsp_power_mock_simulate_wakeup(meter_wakeup_reason_t reason)
{
    s_mock_next_wakeup = reason;
}
#endif
