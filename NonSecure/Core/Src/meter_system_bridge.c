/**
 * @file meter_system_bridge.c
 * @brief Implementasi Jembatan Integrasi Sistem (System Bridge / Glue Code)
 *        Memusatkan seluruh callback HAL STM32, routing interrupt, inisialisasi
 *        tiga pilar (E1, E2, E3), dan loop task RTOS di luar main.c.
 * 
 * Target: STM32U585VIT6 (ARM Cortex-M33 TrustZone Non-Secure)
 */

#include "meter_system_bridge.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* Header Layanan Modular Tiga Pilar (E1, E2, E3) */
#include "e1_metrology_service.h"
#include "e2_dlms_service.h"
#include "e3_meter_app.h"
#include "console_io.h"

/* -------------------------------------------------------------------------- */
/* Variabel Statis Internal                                                  */
/* -------------------------------------------------------------------------- */

/* Handle UART untuk transmisi serial konsol / modem (USART2) */
static UART_HandleTypeDef *s_console_uart = NULL;

/* Context data pengukuran listrik yang dikoordinasikan antar pilar */
static meter_measurements_t s_meas;

/* -------------------------------------------------------------------------- */
/* Callback Console I/O Transmit Retargeting                                  */
/* -------------------------------------------------------------------------- */

static void bridge_console_uart_tx(const uint8_t *data, size_t len)
{
    if ((s_console_uart != NULL) && (data != NULL) && (len > 0U))
    {
        HAL_UART_Transmit(s_console_uart, (uint8_t *)data, (uint16_t)len, 100U);
    }
}

/* -------------------------------------------------------------------------- */
/* Implementasi API Bridge                                                    */
/* -------------------------------------------------------------------------- */

void meter_system_bringup(UART_HandleTypeDef *huart_opt, UART_HandleTypeDef *huart_modem, ADC_HandleTypeDef *hadc_telemetry)
{
    s_console_uart = huart_modem;
    memset(&s_meas, 0, sizeof(s_meas));

    /* 1. Inisialisasi Console I/O & printf retargeting ke USART2 */
    console_io_init(bridge_console_uart_tx);
    console_io_print_boot_banner("SMART METER 3-PHASE STM32U585", "USART2 @ 19200 bps", 160U);

    /* 2. Inisialisasi Layanan Terpadu Pilar E1 (Metrologi), E2 (DLMS/COSEM), dan E3 (Aplikasi) */
    e1_metrology_service_init();
    e2_dlms_service_init(huart_opt);
    e2_dlms_service_set_modem_uart(huart_modem);
    e3_meter_app_init("54321678901", hadc_telemetry);

    printf("[BRIDGE] All Pillars (E1 Metrology, E2 DLMS, E3 App, FreeRTOS LowPower) initialized.\n");
}

const meter_measurements_t *meter_system_get_measurements(void)
{
    return &s_meas;
}

void app_metering_step(void)
{
    static uint32_t last_step_tick_ms = 0;
    uint32_t current_tick_ms = HAL_GetTick();
    uint32_t delta_ms = (last_step_tick_ms == 0) ? 100U : (current_tick_ms - last_step_tick_ms);
    last_step_tick_ms = current_tick_ms;
    if (delta_ms == 0U)
    {
        delta_ms = 1U;
    }

    /* 1. Pilar E1: Akuisisi Data ADC/SPI ADE9000, Konversi Fisik & Akumulasi Energi Wh */
    e1_metrology_service_step(&s_meas, delta_ms);

    /* 2. Pilar E2: Sinkronisasi Register Data Model OBIS DLMS/COSEM */
    e2_dlms_service_sync_snapshot(e2_dlms_service_get_snapshot(), &s_meas);

    /* 3. Pilar E3: Evaluasi Tamper, Relai Shunt Trip, Telemetri Baterai, Carousel Display & Profiling */
    e3_meter_app_step(&s_meas, delta_ms);
}

void app_metering_task_loop(void)
{
    for (;;)
    {
        app_metering_step();
        osDelay(100U);
    }
}

/* -------------------------------------------------------------------------- */
/* Callback Interrupt Hardware Abstraction Layer (STM32 HAL)                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Callback interrupt penerimaan karakter UART (Optik & Modem).
 *        Meng-override deklarasi __weak pada stm32u5xx_hal_uart.c.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* Serahkan pemrosesan byte optik (USART3) maupun modem (USART2) ke Pilar E2 */
    e2_dlms_service_uart_rx_callback(huart);
}

/**
 * @brief Callback interrupt falling-edge EXTI GPIO.
 *        Meng-override deklarasi __weak pada stm32u5xx_hal_gpio.c.
 */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN_SCROLL_UP_Pin)
    {
        /* Sesuai SPLN Tabel 5 No 12: Deteksi tombol simultan (▲ + ▼) */
        if (HAL_GPIO_ReadPin(BTN_SCROLL_DN_GPIO_Port, BTN_SCROLL_DN_Pin) == GPIO_PIN_RESET)
        {
            e3_meter_app_on_button(BUTTON_DIR_BOTH);
        }
        else
        {
            e3_meter_app_on_button(BUTTON_DIR_UP);
        }
    }
    else if (GPIO_Pin == BTN_SCROLL_DN_Pin)
    {
        /* Sesuai SPLN Tabel 5 No 12: Deteksi tombol simultan (▲ + ▼) */
        if (HAL_GPIO_ReadPin(BTN_SCROLL_UP_GPIO_Port, BTN_SCROLL_UP_Pin) == GPIO_PIN_RESET)
        {
            e3_meter_app_on_button(BUTTON_DIR_BOTH);
        }
        else
        {
            e3_meter_app_on_button(BUTTON_DIR_DOWN);
        }
    }
    else if (GPIO_Pin == AC_FAIL_DET_Pin)
    {
        /* Deteksi Listrik Padam (Last Gasp): Matikan beban tinggi (E3) & ADE9000 ke Sleep (E1) */
        e3_meter_app_on_ac_fail(true);
        e1_metrology_set_power_mode(true);
    }
    else if (GPIO_Pin == ADE_CF1_Pin)
    {
        e1_metrology_on_active_pulse();
    }
    else if (GPIO_Pin == ADE_CF2_Pin)
    {
        e1_metrology_on_reactive_pulse();
    }
    else if (GPIO_Pin == ADE_IRQ0_Pin)
    {
        e3_meter_app_on_tamper(TAMPER_VECTOR_REVERSE_POWER);
    }
    else if (GPIO_Pin == ADE_IRQ1_Pin)
    {
        e3_meter_app_on_tamper(TAMPER_VECTOR_NEUTRAL_BYPASS);
    }
    else if (GPIO_Pin == ADE_EVENT_Pin)
    {
        /* Mutu Daya: Dip/Sag, Swell, Overcurrent (SPLN D3.006) */
        e3_meter_app_on_tamper(TAMPER_VECTOR_PHASE_LOSS);
    }
    else if (GPIO_Pin == ADE_CF_ZX_Pin)
    {
        /* Zero-Crossing pulse */
        e3_meter_app_on_zero_crossing();
    }
    else if (GPIO_Pin == AUX_CTRL_IN1_Pin)
    {
        e3_meter_app_on_aux_input(1U, true);
    }
    else if (GPIO_Pin == AUX_CTRL_IN2_Pin)
    {
        e3_meter_app_on_aux_input(2U, true);
    }
}

/**
 * @brief Callback interrupt rising-edge EXTI GPIO.
 *        Meng-override deklarasi __weak pada stm32u5xx_hal_gpio.c.
 */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == AC_FAIL_DET_Pin)
    {
        /* Deteksi Listrik PLN Normal Kembali: Hidupkan periferal (E3) & ADE9000 ke Run (E1) */
        e3_meter_app_on_ac_fail(false);
        e1_metrology_set_power_mode(false);
    }
    else if (GPIO_Pin == ADE_CF_ZX_Pin)
    {
        e3_meter_app_on_zero_crossing();
    }
    else if (GPIO_Pin == AUX_CTRL_IN1_Pin)
    {
        e3_meter_app_on_aux_input(1U, false);
    }
    else if (GPIO_Pin == AUX_CTRL_IN2_Pin)
    {
        e3_meter_app_on_aux_input(2U, false);
    }
}

