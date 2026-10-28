/**
 * @file dlms_tamper_log.h
 * @brief DLMS Tamper Log Definition according to SPLN D3.006:2021 Table 6
 */

#ifndef DLMS_TAMPER_LOG_H
#define DLMS_TAMPER_LOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DLMS_OK = 0,
    DLMS_ERR_NULL_PTR,
    DLMS_ERR_INVALID_PARAM,
    DLMS_ERR_INVALID_STATE,
    DLMS_ERR_UNAUTHORIZED,
    DLMS_ERR_INVALID_APDU,
    DLMS_ERR_BUFFER_OVERFLOW,
    DLMS_ERR_BUFFER_EMPTY,
    DLMS_ERR_TIMEOUT
} dlms_result_t;

/* Kapasitas Minimal Log Sesuai SPLN D3.006:2021 (30 Rekaman FIFO) */
#define DLMS_TAMPER_LOG_MAX_ENTRIES 30U

/* Kode Kejadian Tamper Utama Sesuai SPLN D3.006:2021 Tabel 6 */
typedef enum {
    DLMS_TAMPER_NONE                = 0x00,
    DLMS_TAMPER_TERMINAL_COVER_OPEN = 0x01,  /* OBIS 0.0.96.20.5.255  - SPLN ERR20 */
    DLMS_TAMPER_METER_COVER_OPEN    = 0x02,  /* OBIS 0.0.96.20.1.255  - SPLN RUSAK */
    DLMS_TAMPER_MAGNETIC_INDUCTION  = 0x03,  /* OBIS 0.0.96.20.26.255 - SPLN ERR25 */
    DLMS_TAMPER_REVERSE_CURRENT     = 0x04,  /* OBIS 0.0.96.20.27.255 - SPLN ERR26 */
    DLMS_TAMPER_MISSING_NEUTRAL     = 0x05,  /* OBIS 0.0.96.20.24.255 - SPLN ERR23 */
    DLMS_TAMPER_WRONG_PHASE_SEQ     = 0x06,  /* SPLN ERR21 */
    DLMS_TAMPER_CROSS_PHASE         = 0x07,  /* SPLN ERR22 */
    DLMS_TAMPER_MISSING_VOLTAGE     = 0x08,  /* SPLN ERR24 */
    DLMS_TAMPER_LOW_PF_QUAD4        = 0x09   /* SPLN ERR27 */
} dlms_tamper_code_t;

/* Struktur Entri Tunggal Rekaman Tamper Log */
typedef struct {
    uint32_t timestamp_epoch;  /* Waktu kejadian (UNIX Timestamp RTC) */
    uint8_t  event_code;       /* Kode jenis tamper (dlms_tamper_code_t) */
    uint8_t  status;           /* 1 = Event Occurred (Mulai), 0 = Event Restored (Selesai) */
} dlms_tamper_record_t;

/* Struktur Buffer Sirkular Log Kejadian (Class 7 Profile Generic) */
typedef struct {
    dlms_tamper_record_t records[DLMS_TAMPER_LOG_MAX_ENTRIES];
    size_t head;            /* Indeks penulisan entri baru */
    size_t entries_in_use;  /* Jumlah rekaman aktif saat ini (0 .. 30) */
    uint32_t total_events;  /* Akumulator total seluruh kejadian tamper */
} dlms_tamper_log_t;

dlms_result_t dlms_tamper_log_init(dlms_tamper_log_t *log);
dlms_result_t dlms_tamper_log_add_event(dlms_tamper_log_t *log, 
                                        uint32_t timestamp_epoch, 
                                        dlms_tamper_code_t code, 
                                        uint8_t status);
dlms_result_t dlms_tamper_log_get_entry(const dlms_tamper_log_t *log, 
                                        size_t index, 
                                        dlms_tamper_record_t *out_record);
size_t dlms_tamper_log_encode_axdr(const dlms_tamper_log_t *log, 
                                   uint8_t *out_buffer, 
                                   size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_TAMPER_LOG_H */

