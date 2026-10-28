/**
 * @file meter_system_bridge.h
 * @brief Jembatan Integrasi Sistem (Glue Code / System Bridge)
 *        Memisahkan implementasi callback HAL STM32, penjadwalan loop metering,
 *        dan orkestrasi inisialisasi Tiga Pilar (E1, E2, E3) dari main.c.
 * 
 * Target: STM32U585VIT6 (ARM Cortex-M33 TrustZone Non-Secure)
 */

#ifndef METER_SYSTEM_BRIDGE_H
#define METER_SYSTEM_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "display.h"

/**
 * @brief Inisialisasi terpadu seluruh subsistem dan Tiga Pilar (E1, E2, E3).
 *        Menggantikan runtunan inisialisasi manual di USER CODE BEGIN 2 main.c.
 * 
 * @param huart_opt       Handle UART untuk port optik lokal (USART3)
 * @param huart_modem     Handle UART untuk modem telemetri / console printf (USART2)
 * @param hadc_telemetry  Handle ADC untuk sensor tegangan baterai & VCAP (ADC4)
 */
void meter_system_bringup(UART_HandleTypeDef *huart_opt, UART_HandleTypeDef *huart_modem, ADC_HandleTypeDef *hadc_telemetry);

/**
 * @brief Eksekusi satu langkah periodik metering sinkron (E1 -> E2 -> E3).
 *        - E1: Pengambilan sample metrologi ADE9000 & kalkulasi daya/energi
 *        - E2: Sinkronisasi register data model OBIS DLMS/COSEM
 *        - E3: Evaluasi state machine breaker/tamper, update LCD & profiling
 */
void app_metering_step(void);

/**
 * @brief Loop tugas metering FreeRTOS default task (dipanggil dari app_freertos.c).
 */
void app_metering_task_loop(void);

/**
 * @brief Mengambil pointer data pengukuran listrik terkini (read-only).
 * 
 * @return Pointer konstan ke struktur meter_measurements_t
 */
const meter_measurements_t *meter_system_get_measurements(void);

#ifdef __cplusplus
}
#endif

#endif /* METER_SYSTEM_BRIDGE_H */

