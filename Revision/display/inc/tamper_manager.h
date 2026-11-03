/**
 * @file tamper_manager.h
 * @brief Definisi Vektor Sabotase dan Alarm Internal untuk Modul Display SPLN
 */

#ifndef TAMPER_MANAGER_H
#define TAMPER_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ========================================================================= */
/*  1. Vektor Sabotase & Mask Operasi Relai / LED Kuning (SPLN Tabel 6)      */
/* ========================================================================= */
typedef enum {
    TAMPER_VECTOR_CASE_OPEN       = (1 << 0), /* SPLN Tabel 6 No 1: Tutup Meter Dibuka -> Teks: "RUSAK", Kode: "-" */
    TAMPER_VECTOR_TERMINAL_OPEN   = (1 << 1), /* SPLN Tabel 6 No 2: Tutup Terminal Dibuka -> Teks: "PERIKSA", Kode: "ERR20" */
    TAMPER_VECTOR_MAGNETIC_FIELD  = (1 << 2), /* SPLN Tabel 6 No 7: Induksi Medan Magnet -> Teks: "PERIKSA", Kode: "ERR25" */
    TAMPER_VECTOR_NEUTRAL_BYPASS  = (1 << 3), /* SPLN Tabel 6 No 5: Kawat Netral Hilang/Putus -> Teks: "PERIKSA", Kode: "ERR23" */
    TAMPER_VECTOR_REVERSE_POWER   = (1 << 4), /* SPLN Tabel 6 No 8: Reverse Power -> Teks: "REVERSE", Kode: "ERR26" */
    TAMPER_VECTOR_PHASE_LOSS      = (1 << 5), /* SPLN Tabel 6 No 6: Hilang Tegangan 1/2 Fase -> Teks: "PERIKSA", Kode: "ERR24" */
    TAMPER_VECTOR_WRONG_SEQUENCE  = (1 << 6), /* SPLN Tabel 6 No 3: Urutan Fase Terbalik -> Teks: "PERIKSA", Kode: "ERR21" */
    TAMPER_VECTOR_CROSS_PHASE     = (1 << 7), /* SPLN Tabel 6 No 4: Pengawatan Arus/Tegangan Silang -> Teks: "PERIKSA", Kode: "ERR22" */
    TAMPER_VECTOR_LOW_PF_QUAD4    = (1 << 8)  /* SPLN Tabel 6 No 9: Kuadran 4 / PF < 0.85 -> Teks: "PERIKSA", Kode: "ERR27" */
} tamper_vector_t;

#define TAMPER_MASK_RELAY_TRIP ( \
    TAMPER_VECTOR_CASE_OPEN      | \
    TAMPER_VECTOR_TERMINAL_OPEN  | \
    TAMPER_VECTOR_WRONG_SEQUENCE | \
    TAMPER_VECTOR_CROSS_PHASE    | \
    TAMPER_VECTOR_PHASE_LOSS       \
)

#define TAMPER_MASK_LED_YELLOW ( \
    TAMPER_VECTOR_TERMINAL_OPEN  | \
    TAMPER_VECTOR_WRONG_SEQUENCE | \
    TAMPER_VECTOR_CROSS_PHASE    | \
    TAMPER_VECTOR_NEUTRAL_BYPASS | \
    TAMPER_VECTOR_PHASE_LOSS     | \
    TAMPER_VECTOR_REVERSE_POWER  | \
    TAMPER_VECTOR_LOW_PF_QUAD4     \
)

/**
 * @brief Kode Alarm Internal Meter / Kerusakan Komponen Sesuai SPLN D3.006:2021 Tabel 4
 */
typedef enum {
    SPLN_ALARM_NONE          = 0,
    SPLN_ALARM_FLASH_ERROR   = (1 << 0), /* ERR00: Flash memory rusak/error */
    SPLN_ALARM_RAM_ERROR     = (1 << 1), /* ERR01: RAM rusak/error */
    SPLN_ALARM_RTC_ERROR     = (1 << 2), /* ERR02: Clock loss / RTC error */
    SPLN_ALARM_LOW_BATTERY   = (1 << 3), /* ERR03: Low battery (threshold 2.7 V) */
    SPLN_ALARM_MCU_ERROR     = (1 << 4), /* ERR04: Mikroprosesor tidak berfungsi normal */
    SPLN_ALARM_ADC_ERROR     = (1 << 5), /* ERR05: Kegagalan sampling data ADC */
    SPLN_ALARM_SUPERCAP_FAIL = (1 << 6), /* ERR06: Superkapasitor rusak/lepas */
    SPLN_ALARM_RELAY_FAIL    = (1 << 7)  /* ERR07: Relai/shunt trip gagal membuka/menutup */
} spln_internal_alarm_t;

#define SPLN_ALARM_MASK_RELAY_TRIP ( \
    SPLN_ALARM_FLASH_ERROR | \
    SPLN_ALARM_RAM_ERROR   | \
    SPLN_ALARM_RTC_ERROR   | \
    SPLN_ALARM_MCU_ERROR   | \
    SPLN_ALARM_ADC_ERROR     \
)

#define SPLN_ALARM_MASK_LED_YELLOW ( \
    SPLN_ALARM_FLASH_ERROR   | \
    SPLN_ALARM_RAM_ERROR     | \
    SPLN_ALARM_RTC_ERROR     | \
    SPLN_ALARM_LOW_BATTERY   | \
    SPLN_ALARM_MCU_ERROR     | \
    SPLN_ALARM_ADC_ERROR     | \
    SPLN_ALARM_SUPERCAP_FAIL | \
    SPLN_ALARM_RELAY_FAIL      \
)

#ifdef __cplusplus
}
#endif

#endif /* TAMPER_MANAGER_H */
