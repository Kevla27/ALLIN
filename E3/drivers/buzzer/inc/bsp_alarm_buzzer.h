/**
 * @file bsp_alarm_buzzer.h
 * @brief Board Support Package untuk Aktuasi Buzzer dan Indikator Alarm SPLN (Pin PB7)
 *        Memenuhi Standar SPLN D3.006: 2021 Butir 6.14 (Indikator Alarm Audio & Visual)
 */

#ifndef BSP_ALARM_BUZZER_H
#define BSP_ALARM_BUZZER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inisialisasi pin GPIO untuk Buzzer / LED Indikator Alarm (PB7)
 */
void bsp_alarm_buzzer_init(void);

/**
 * @brief Menyalakan atau mematikan buzzer/indikator alarm
 * @param on true untuk menyalakan (HIGH), false untuk mematikan (LOW)
 */
void bsp_alarm_buzzer_set(bool on);

/**
 * @brief Membalikkan (toggle) status pin buzzer/indikator alarm (untuk pola beeping)
 */
void bsp_alarm_buzzer_toggle(void);

/**
 * @brief Membaca status keluaran buzzer/indikator saat ini
 * @return true jika aktif/HIGH, false jika non-aktif/LOW
 */
bool bsp_alarm_buzzer_get(void);

/**
 * @brief Memperbarui siklus kadensi audio buzzer sesuai SPLN D3.006: 2021 Butir 6.14
 * @param alarm_active true jika kondisi alarm sabotase atau trip sedang aktif (200 ms ON / 200 ms OFF)
 * @param beep_override true jika ada override bunyi tombol 50 ms
 * @param delta_ms Waktu langkah eksekusi dalam milidetik
 * @return Status keluaran buzzer saat ini (true = aktif/ON, false = mati/OFF)
 */
bool bsp_alarm_buzzer_update_cadence(bool alarm_active, bool beep_override, uint32_t delta_ms);

#ifdef __cplusplus
}
#endif

#endif /* BSP_ALARM_BUZZER_H */

