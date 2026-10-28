/**
 * @file tamper_manager.h
 * @brief Modul Pengelola Kejadian Sabotase, Alarm Hardware, dan Event Log (E3/ENG-3)
 *        Memenuhi Standar SPLN D3.006: 2021 Butir 12.3 Tabel 18 (Hal. 33-34)
 */

#ifndef TAMPER_MANAGER_H
#define TAMPER_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "dlms_tamper_log.h"

#define TAMPER_LOG_MAX_ENTRIES     30U
#define SPLN_LOG_FIFO_MAX_ENTRIES  30U  /**< Kapasitas minimal 30 rekaman per log FIFO (SPLN Butir 12.3) */

/* ========================================================================= */
/*  1. Vektor Sabotase & Mask Operasi Relai / LED Kuning (SPLN Tabel 6)      */
/* ========================================================================= */
typedef enum {
    TAMPER_VECTOR_CASE_OPEN       = (1 << 0), /* SPLN Tabel 6 No 1: Tutup Meter Dibuka -> Teks: "RUSAK", Kode: "-", Relai: Buka, LED: - */
    TAMPER_VECTOR_TERMINAL_OPEN   = (1 << 1), /* SPLN Tabel 6 No 2: Tutup Terminal Dibuka -> Teks: "PERIKSA", Kode: "ERR20", Relai: Buka, LED: Aktif */
    TAMPER_VECTOR_MAGNETIC_FIELD  = (1 << 2), /* SPLN Tabel 6 No 7: Induksi Medan Magnet -> Teks: "PERIKSA", Kode: "ERR25", Relai: Tidak, LED: - */
    TAMPER_VECTOR_NEUTRAL_BYPASS  = (1 << 3), /* SPLN Tabel 6 No 5: Kawat Netral Hilang/Putus -> Teks: "PERIKSA", Kode: "ERR23", Relai: Tidak, LED: Aktif */
    TAMPER_VECTOR_REVERSE_POWER   = (1 << 4), /* SPLN Tabel 6 No 8: Reverse Power -> Teks: "REVERSE", Kode: "ERR26", Relai: Tidak, LED: Aktif */
    TAMPER_VECTOR_PHASE_LOSS      = (1 << 5), /* SPLN Tabel 6 No 6: Hilang Tegangan 1/2 Fase -> Teks: "PERIKSA", Kode: "ERR24", Relai: Buka, LED: Aktif */
    TAMPER_VECTOR_WRONG_SEQUENCE  = (1 << 6), /* SPLN Tabel 6 No 3: Urutan Fase Terbalik -> Teks: "PERIKSA", Kode: "ERR21", Relai: Buka, LED: Aktif */
    TAMPER_VECTOR_CROSS_PHASE     = (1 << 7), /* SPLN Tabel 6 No 4: Pengawatan Arus/Tegangan Silang -> Teks: "PERIKSA", Kode: "ERR22", Relai: Buka, LED: Aktif */
    TAMPER_VECTOR_LOW_PF_QUAD4    = (1 << 8)  /* SPLN Tabel 6 No 9: Kuadran 4 / PF < 0.85 -> Teks: "PERIKSA", Kode: "ERR27", Relai: Tidak, LED: Aktif */
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

/* ========================================================================= */
/*  2. Alarm Internal Hardware (SPLN D3.006: 2021 Tabel 4)                   */
/* ========================================================================= */
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

static inline bool spln_alarm_should_trip_relay(uint32_t alarm_mask) {
    return ((alarm_mask & SPLN_ALARM_MASK_RELAY_TRIP) != 0U);
}

static inline bool spln_alarm_should_turn_on_led(uint32_t alarm_mask) {
    return ((alarm_mask & SPLN_ALARM_MASK_LED_YELLOW) != 0U);
}

/* ========================================================================= */
/*  3. Kode Jenis Kejadian 3 Log FIFO Sesuai SPLN Butir 12.3 Tabel 18         */
/* ========================================================================= */

/**
 * @brief Kode Kejadian Register Log 1: Alarm Log (8 Jenis Sesuai Tabel 18 No. 1 s.d 8)
 */
typedef enum {
    SPLN_LOG_ALARM_NONE          = 0x00,
    SPLN_LOG_ALARM_FLASH_ERROR   = 0x01, /* No 1: Flash memory rusak/error (ERR00) */
    SPLN_LOG_ALARM_RAM_ERROR     = 0x02, /* No 2: RAM rusak/error (ERR01) */
    SPLN_LOG_ALARM_CLOCK_LOSS    = 0x03, /* No 3: Clock loss (ERR02) */
    SPLN_LOG_ALARM_LOW_BATTERY   = 0x04, /* No 4: Low battery threshold 2.7V (ERR03) */
    SPLN_LOG_ALARM_MCU_ERROR     = 0x05, /* No 5: Mikroprosesor tidak berfungsi normal (ERR04) */
    SPLN_LOG_ALARM_ADC_ERROR     = 0x06, /* No 6: Kegagalan sampling ADC (ERR05) */
    SPLN_LOG_ALARM_SUPERCAP_FAIL = 0x07, /* No 7: Superkapasitor rusak/lepas (ERR06) */
    SPLN_LOG_ALARM_RELAY_FAIL    = 0x08  /* No 8: Relai/shunt trip gagal membuka/menutup (ERR07) */
} spln_alarm_log_code_t;

/**
 * @brief Kode Kejadian Register Log 2: Event Log (12 Jenis Sesuai Tabel 18 No. 9 s.d 19 & 29)
 */
typedef enum {
    SPLN_LOG_EVENT_NONE             = 0x00,
    SPLN_LOG_EVENT_POWER_FAILURE    = 0x01, /* No 9: Power supply failure */
    SPLN_LOG_EVENT_POWER_UP         = 0x02, /* No 10: Power supply up */
    SPLN_LOG_EVENT_TOU_CHANGE       = 0x03, /* No 11: Perubahan setting ToU */
    SPLN_LOG_EVENT_NON_TOU_CHANGE   = 0x04, /* No 12: Perubahan setting (selain ToU) */
    SPLN_LOG_EVENT_STOP_MEASUREMENT = 0x05, /* No 13: Stop-measurement */
    SPLN_LOG_EVENT_IO_CHANGE        = 0x06, /* No 14: Perubahan status I/O (termasuk trigger test) */
    SPLN_LOG_EVENT_RELAY_CHANGE     = 0x07, /* No 15: Perubahan status relai/kontaktor internal */
    SPLN_LOG_EVENT_METER_RESET      = 0x08, /* No 16: Meter reset */
    SPLN_LOG_EVENT_TIME_CHANGE      = 0x09, /* No 17: Perubahan tanggal dan waktu */
    SPLN_LOG_EVENT_ASYMMETRIC_POWER = 0x0A, /* No 18: Asymmetric power */
    SPLN_LOG_EVENT_READY_RECONNECT  = 0x0B, /* No 19: Meter ready remote shunt trip ON */
    SPLN_LOG_EVENT_POWER_LIMIT_EXC  = 0x0C  /* No 29: Power limiter threshold exceeded */
} spln_event_log_code_t;

/**
 * @brief Kode Kejadian Register Log 3: Tamper Log (9 Jenis Sesuai Tabel 18 No. 20 s.d 28)
 */
typedef enum {
    SPLN_LOG_TAMPER_NONE            = 0x00,
    SPLN_LOG_TAMPER_CASE_OPEN       = 0x01, /* No 20: Pembukaan tutup meter (RUSAK) */
    SPLN_LOG_TAMPER_TERMINAL_OPEN   = 0x02, /* No 21: Pembukaan tutup terminal (ERR20) */
    SPLN_LOG_TAMPER_WRONG_SEQUENCE  = 0x03, /* No 22: Pengawatan tidak sesuai urutan fase (ERR21) */
    SPLN_LOG_TAMPER_CROSS_PHASE     = 0x04, /* No 23: Pengawatan arus dan tegangan tidak sefase (ERR22) */
    SPLN_LOG_TAMPER_MISSING_NEUTRAL = 0x05, /* No 24: Kawat netral putus / bypass (ERR23) */
    SPLN_LOG_TAMPER_PHASE_LOSS      = 0x06, /* No 25: Hilang tegangan 1 atau 2 fase (ERR24) */
    SPLN_LOG_TAMPER_MAGNETIC_FIELD  = 0x07, /* No 26: Induksi medan magnet luar <= 500mT (ERR25) */
    SPLN_LOG_TAMPER_REVERSE_POWER   = 0x08, /* No 27: Reverse power (ERR26) */
    SPLN_LOG_TAMPER_LOW_PF_QUAD4    = 0x09  /* No 28: Pengukuran kuadran 4 PF < 0.85 (ERR27) */
} spln_tamper_log_code_t;

/**
 * @brief Entri Rekaman Log Standar SPLN D3.006: 2021 Butir 12.3
 */
typedef struct {
    uint32_t timestamp_epoch;  /**< Waktu kejadian (Epoch UNIX RTC) */
    uint16_t event_code;       /**< Kode kejadian (Alarm/Event/Tamper) */
    uint16_t status_flags;     /**< 1 = Mulai (Occurred), 0 = Selesai (Restored) */
    uint32_t parameter_val;    /**< Parameter tambahan (misal nilai deviasi asimetri, daya, dll) */
} spln_standard_log_entry_t;

/**
 * @brief Buffer Sirkular FIFO Log Minimal 30 Rekaman Non-Volatile (SPLN Butir 12.3)
 */
typedef struct {
    spln_standard_log_entry_t records[SPLN_LOG_FIFO_MAX_ENTRIES];
    size_t                    head;          /**< Indeks tulis FIFO (0..29) */
    size_t                    count;         /**< Jumlah rekaman aktif (0..30) */
    uint32_t                  total_events;  /**< Akumulator total seluruh kejadian yang pernah dicatat */
} spln_log_fifo_t;

/* Entri legacy untuk kompatibilitas */
typedef struct {
    uint32_t        timestamp;
    tamper_vector_t vector;
    bool            is_asserted;
    uint32_t        duration_sec;
    uint32_t        active_mask;
} tamper_event_entry_t;

/**
 * @brief Konteks Pengelola Sabotase, Alarm, dan 3 Register Log FIFO Non-Volatile
 */
typedef struct {
    uint32_t             active_tamper_mask;
    uint32_t             total_event_count;
    tamper_event_entry_t log_fifo[TAMPER_LOG_MAX_ENTRIES];
    size_t               fifo_head;
    size_t               fifo_count;
    bool                 alarm_relay_trigger;
    bool                 alarm_led_status;

    /* 3 Register Log FIFO Non-Volatile Terpisah Sesuai SPLN D3.006: 2021 Butir 12.3 */
    spln_log_fifo_t      alarm_log_fifo;   /**< Register 1: Alarm Log (8 jenis) - Min. 30 Entri FIFO */
    spln_log_fifo_t      event_log_fifo;   /**< Register 2: Event Log (12 jenis) - Min. 30 Entri FIFO */
    spln_log_fifo_t      tamper_log_fifo;  /**< Register 3: Tamper Log (9 jenis) - Min. 30 Entri FIFO */

    /* Error Register: Data rangkuman berupa bit-string (SPLN Butir 12.3 Hal. 34) */
    uint32_t             error_register_bitstring;
} tamper_context_t;

/* ========================================================================= */
/*  4. API Operasional Tamper & Log Manager                                  */
/* ========================================================================= */
void tamper_init(tamper_context_t *ctx);
bool tamper_process_signal(tamper_context_t *ctx, tamper_vector_t vector, bool is_asserted, uint32_t timestamp);
uint32_t tamper_get_active_mask(const tamper_context_t *ctx);
void tamper_get_log_entries(const tamper_context_t *ctx, tamper_event_entry_t *out_buffer, size_t max_records, size_t *out_count);
bool tamper_is_alarm_pending(const tamper_context_t *ctx);
bool tamper_should_trip_relay(const tamper_context_t *ctx);
bool tamper_should_turn_on_led(const tamper_context_t *ctx);

/**
 * @brief Operasi 3 Register Log FIFO Non-Volatile Sesuai SPLN Butir 12.3
 */
bool spln_log_record_alarm(tamper_context_t *ctx, spln_alarm_log_code_t code, bool is_asserted, uint32_t timestamp, uint32_t param);
bool spln_log_record_event(tamper_context_t *ctx, spln_event_log_code_t code, bool is_asserted, uint32_t timestamp, uint32_t param);
bool spln_log_record_tamper(tamper_context_t *ctx, spln_tamper_log_code_t code, bool is_asserted, uint32_t timestamp, uint32_t param);

void spln_log_get_entries(const spln_log_fifo_t *fifo, spln_standard_log_entry_t *out_buffer, size_t max_records, size_t *out_count);
bool spln_log_get_entry_by_index(const spln_log_fifo_t *fifo, size_t index, spln_standard_log_entry_t *out_entry);
uint32_t spln_log_get_error_register(const tamper_context_t *ctx);

/**
 * @brief Konversi antara Tamper Vector E3 dan DLMS Tamper Code SPLN D3.006
 */
dlms_tamper_code_t tamper_vector_to_dlms_code(tamper_vector_t vector);
tamper_vector_t dlms_code_to_tamper_vector(dlms_tamper_code_t code);

/**
 * @brief Handler interupsi pendeteksian sabotase / sakelar sensor tamper
 */
void tamper_handle_button_press_isr(uint32_t timestamp_ms);

#ifdef __cplusplus
}
#endif

#endif /* TAMPER_MANAGER_H */
