/**
 * @file e3_meter_app.h
 * @brief Layanan Terpadu Pilar E3: HMI Carousel, Tamper Sabotase, Pembatas Beban Relai, ToU 8-Zona & Telemetri
 */

#ifndef E3_METER_APP_H
#define E3_METER_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "display.h"
#include "tamper_manager.h"
#include "power_relay_manager.h"
#include "load_profile.h"
#include "board_profile_config.h"
#include "main.h"

#define E3_TOU_MAX_ZONES 8U

/**
 * @brief Inisialisasi seluruh subsistem E3 (HMI, Tamper, Relay Limiter, Load Profile, ToU & Power Rails)
 * @param customer_id Nomor ID Pelanggan PLN (IDPEL)
 * @param hadc_telemetry Pointer ke ADC handle (ADC4) untuk pengukuran VBAT & VCAP
 */
void e3_meter_app_init(const char *customer_id, ADC_HandleTypeDef *hadc_telemetry);

/**
 * @brief Eksekusi siklis task E3 (Watchdog, Tamper, Relai, Telemetri ADC4, Carousel UI, ToU & Profiling)
 * @param meas Pointer ke data pengukuran listrik terbaru
 * @param delta_ms Waktu elapsed sejak siklus terakhir dalam milidetik
 */
void e3_meter_app_step(const meter_measurements_t *meas, uint32_t delta_ms);

/**
 * @brief Handler penekanan tombol navigasi (▲ / ▼) & Wake-Up Layar 10 Detik saat PLN Padam
 * @param dir Arah navigasi (BUTTON_DIR_UP / BUTTON_DIR_DOWN)
 */
void e3_meter_app_on_button(button_dir_t dir);

/**
 * @brief Handler kejadian pemadaman listrik PLN (Last Gasp Push, Snapshot NVM & Power Gating)
 * @param is_power_cut true jika PLN padam, false jika PLN pulih normal
 */
void e3_meter_app_on_ac_fail(bool is_power_cut);

/**
 * @brief Handler kejadian interupsi tamper eksternal (misal dari AFE ADE9000)
 * @param vector Jenis vektor tamper yang terdeteksi
 */
void e3_meter_app_on_tamper(tamper_vector_t vector);

/**
 * @brief Handler interupsi Zero-Crossing (ADE_CF_ZX / PE8)
 */
void e3_meter_app_on_zero_crossing(void);

/**
 * @brief Handler interupsi Kontrol Bantu Digital Input (AUX_CTRL_IN1 PD10 / AUX_CTRL_IN2 PD14)
 * @param channel Nomor kanal (1 atau 2)
 * @param is_active Status logika pin input
 */
void e3_meter_app_on_aux_input(uint8_t channel, bool is_active);

/**
 * @brief Memperoleh timestamp UNIX Epoch saat ini
 */
uint32_t e3_meter_app_get_epoch(void);

/**
 * @brief Memperoleh akumulasi energi Time-of-Use (ToU) per zona tarif (0..7) dalam Wh
 */
uint64_t e3_meter_app_get_tou_energy_wh(uint8_t zone_idx);

/**
 * @brief Memperoleh zona tarif aktif saat ini (0 = WBP, 1 = LWBP1, 2 = LWBP2, dst.)
 */
uint8_t e3_meter_app_get_active_tariff_zone(void);

/**
 * @brief Memaksa perekaman snapshot billing bulanan dan mereset Maximum Demand
 */
void e3_meter_app_trigger_billing_snapshot(void);

/**
 * @brief Mendapatkan pointer konteks snapshot billing bulanan
 */
const billing_snapshot_ctx_t* e3_meter_app_get_billing_snapshots(void);

/**
 * @brief Mendapatkan status Maximum Demand tracker saat ini
 */
const max_demand_tracker_t* e3_meter_app_get_max_demand(void);

/**
 * @brief Mengatur flag alarm internal hardware (SPLN D3.006: 2021 Tabel 4)
 * @param alarm_flags Mask spln_internal_alarm_t (ERR00 s.d ERR07)
 */
void e3_meter_app_set_internal_alarm(uint32_t alarm_flags);

/**
 * @brief Memperoleh mask alarm internal hardware yang sedang aktif
 */
uint32_t e3_meter_app_get_internal_alarm(void);

/**
 * @brief Memperoleh pointer ke konteks tamper & 3 log register FIFO (Alarm, Event, Tamper)
 */
const tamper_context_t* e3_meter_app_get_tamper_context(void);

#ifdef __cplusplus
}
#endif

#endif /* E3_METER_APP_H */
