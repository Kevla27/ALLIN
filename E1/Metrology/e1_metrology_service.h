/**
 * @file e1_metrology_service.h
 * @brief Layanan Terpadu Pilar E1: Akuisisi Data Metrologi, Konversi Satuan & Akumulasi Energi
 */

#ifndef E1_METROLOGY_SERVICE_H
#define E1_METROLOGY_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "Metrology.h"
#include "display.h" /* Untuk struct meter_measurements_t */

/**
 * @brief Inisialisasi hardware metrologi ADE9000 & Mock Simulator
 */
void e1_metrology_service_init(void);

/**
 * @brief Sekuens konfigurasi dan bring-up register internal ADE9000 (Desain Sistem Hal. 67-75)
 */
void e1_metrology_bringup_init(void);

/**
 * @brief Kalibrasi gain kanal tegangan, arus, dan daya aktif per fasa (0=R, 1=S, 2=T)
 */
void e1_metrology_calibrate_channel(uint8_t phase_idx, float ref_v, float ref_i, float ref_w);

/**
 * @brief Pembacaan siklis metrologi, konversi ke satuan teknik, dan akumulasi energi
 * @param[out] meas Pointer ke struct pengukuran meter
 * @param[in] delta_ms Waktu berlalu sejak siklus terakhir dalam milidetik
 */
void e1_metrology_service_step(meter_measurements_t *meas, uint32_t delta_ms);

/**
 * @brief Kontrol mode daya AFE ADE9000 (Run PSM0 / Sleep PSM3)
 * @param sleep true untuk mode sleep saat padam, false untuk mode aktif normal
 */
void e1_metrology_set_power_mode(bool sleep);

/**
 * @brief Handler pulsa kalibrasi energi aktif (CF1 / PE12)
 */
void e1_metrology_on_active_pulse(void);

/**
 * @brief Handler pulsa kalibrasi energi reaktif (CF2 / PE13)
 */
void e1_metrology_on_reactive_pulse(void);

/**
 * @brief Memperoleh jumlah pulsa energi aktif terakumulasi
 */
uint32_t e1_metrology_get_active_pulses(void);

/**
 * @brief Memperoleh jumlah pulsa energi reaktif terakumulasi
 */
uint32_t e1_metrology_get_reactive_pulses(void);

/**
 * @brief Memperoleh akumulasi energi reaktif 4-kuadran (Q1, Q2, Q3, Q4) dalam Varh
 */
const ReactiveEnergyQuadrants_t *e1_metrology_get_reactive_quadrants(void);

#ifdef __cplusplus
}
#endif

#endif /* E1_METROLOGY_SERVICE_H */
