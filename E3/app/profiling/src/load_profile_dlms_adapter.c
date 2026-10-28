/**
 * @file load_profile_dlms_adapter.c
 * @brief Implementasi Adapter Pemformatan A-XDR DLMS COSEM Class ID 7
 *        Sesuai Spesifikasi DLMS Blue/Green Book & SPLN D3.006: 2021
 */

#include "load_profile_dlms_adapter.h"
#include <string.h>

void dlms_encode_datetime(uint32_t epoch_sec, uint8_t *out_buf) {
    if (out_buf == NULL) return;

    /* Konversi UNIX Epoch (uint32_t, bebas Y2K38 hingga tahun 2106) ke 12-byte DLMS COSEM date-time */
    uint32_t rem_sec = epoch_sec % 86400UL;
    uint32_t days    = epoch_sec / 86400UL;
    uint32_t z       = days + 719468UL;
    uint32_t era     = z / 146097UL;
    uint32_t doe     = z - era * 146097UL;
    uint32_t yoe     = (doe - doe / 1460UL + doe / 36524UL - doe / 146096UL) / 365UL;
    uint32_t y       = yoe + era * 400UL;
    uint32_t doy     = doe - (365UL * yoe + yoe / 4UL - yoe / 100UL);
    uint32_t mp      = (5UL * doy + 2UL) / 153UL;
    uint32_t d       = doy - (153UL * mp + 2UL) / 5UL + 1UL;
    uint32_t m       = (mp < 10UL) ? (mp + 3UL) : (mp - 9UL);
    if (m <= 2UL) {
        y += 1UL;
    }
    uint8_t dow = (uint8_t)(((days + 3UL) % 7UL) + 1UL); /* 1 = Senin .. 7 = Minggu (IEC 62056-6-2) */

    out_buf[0]  = (uint8_t)((y >> 8) & 0xFFU); /* Year High Byte */
    out_buf[1]  = (uint8_t)(y & 0xFFU);        /* Year Low Byte */
    out_buf[2]  = (uint8_t)m;                  /* Month (1..12) */
    out_buf[3]  = (uint8_t)d;                  /* Day of month (1..31) */
    out_buf[4]  = dow;                         /* Day of week (1..7) */
    out_buf[5]  = (uint8_t)(rem_sec / 3600UL); /* Hour (0..23) */
    out_buf[6]  = (uint8_t)((rem_sec / 60UL) % 60UL); /* Minute (0..59) */
    out_buf[7]  = (uint8_t)(rem_sec % 60UL);   /* Second (0..59) */
    out_buf[8]  = 0x00U;                       /* Hundredths of second */
    out_buf[9]  = 0x01U; out_buf[10] = 0xA4U;  /* Deviation: +420 menit (UTC+7 WIB) */
    out_buf[11] = 0x00U;                       /* Clock status (OK) */
}

size_t load_profile_encode_dlms_entry(const load_profile_entry_t *entry, uint8_t *out_buf, size_t max_len) {
    if (entry == NULL || out_buf == NULL || max_len < 72) return 0;

    size_t idx = 0;

    /* Structure of 11 elements */
    out_buf[idx++] = DLMS_AXDR_TAG_STRUCTURE;
    out_buf[idx++] = 11;

    /* 1. Timestamp (Octet-String 12 byte) */
    out_buf[idx++] = DLMS_AXDR_TAG_OCTET_STRING;
    out_buf[idx++] = 12;
    dlms_encode_datetime(entry->timestamp, &out_buf[idx]);
    idx += 12;

    /* 2. Active Energy Import (+A) - Long64-Unsigned */
    out_buf[idx++] = DLMS_AXDR_TAG_LONG64_U;
    for (int i = 7; i >= 0; i--) {
        out_buf[idx++] = (uint8_t)(entry->active_energy_import_wh >> (i * 8));
    }

    /* 3. Active Energy Export (-A) - Long64-Unsigned (Dukungan Aliran PLTS) */
    out_buf[idx++] = DLMS_AXDR_TAG_LONG64_U;
    for (int i = 7; i >= 0; i--) {
        out_buf[idx++] = (uint8_t)(entry->active_energy_export_wh >> (i * 8));
    }

    /* 4. Reactive Energy Import (+R) - Long64-Unsigned */
    out_buf[idx++] = DLMS_AXDR_TAG_LONG64_U;
    for (int i = 7; i >= 0; i--) {
        out_buf[idx++] = (uint8_t)(entry->reactive_energy_import_varh >> (i * 8));
    }

    /* 5. Voltage R (Long-Unsigned x0.1V) */
    out_buf[idx++] = DLMS_AXDR_TAG_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->voltage_r_dvolts >> 8);
    out_buf[idx++] = (uint8_t)(entry->voltage_r_dvolts & 0xFF);

    /* 6. Voltage S (Long-Unsigned x0.1V) */
    out_buf[idx++] = DLMS_AXDR_TAG_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->voltage_s_dvolts >> 8);
    out_buf[idx++] = (uint8_t)(entry->voltage_s_dvolts & 0xFF);

    /* 7. Voltage T (Long-Unsigned x0.1V) */
    out_buf[idx++] = DLMS_AXDR_TAG_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->voltage_t_dvolts >> 8);
    out_buf[idx++] = (uint8_t)(entry->voltage_t_dvolts & 0xFF);

    /* 8. Current R (Double-Long-Unsigned x0.001A) */
    out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->current_r_mamps >> 24);
    out_buf[idx++] = (uint8_t)(entry->current_r_mamps >> 16);
    out_buf[idx++] = (uint8_t)(entry->current_r_mamps >> 8);
    out_buf[idx++] = (uint8_t)(entry->current_r_mamps & 0xFF);

    /* 9. Current S (Double-Long-Unsigned x0.001A) */
    out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->current_s_mamps >> 24);
    out_buf[idx++] = (uint8_t)(entry->current_s_mamps >> 16);
    out_buf[idx++] = (uint8_t)(entry->current_s_mamps >> 8);
    out_buf[idx++] = (uint8_t)(entry->current_s_mamps & 0xFF);

    /* 10. Current T (Double-Long-Unsigned x0.001A) */
    out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->current_t_mamps >> 24);
    out_buf[idx++] = (uint8_t)(entry->current_t_mamps >> 16);
    out_buf[idx++] = (uint8_t)(entry->current_t_mamps >> 8);
    out_buf[idx++] = (uint8_t)(entry->current_t_mamps & 0xFF);

    /* 11. Status Mask (Double-Long-Unsigned) */
    out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->status_mask >> 24);
    out_buf[idx++] = (uint8_t)(entry->status_mask >> 16);
    out_buf[idx++] = (uint8_t)(entry->status_mask >> 8);
    out_buf[idx++] = (uint8_t)(entry->status_mask & 0xFF);

    return idx;
}

size_t tamper_log_encode_dlms_entry(const tamper_event_entry_t *entry, uint8_t *out_buf, size_t max_len) {
    if (entry == NULL || out_buf == NULL || max_len < 24) return 0;

    size_t idx = 0;

    /* Structure of 3 elements: [Timestamp, Event Code, Active Mask] */
    out_buf[idx++] = DLMS_AXDR_TAG_STRUCTURE;
    out_buf[idx++] = 3;

    /* 1. Timestamp (Octet-String 12-byte) */
    out_buf[idx++] = DLMS_AXDR_TAG_OCTET_STRING;
    out_buf[idx++] = 12;
    dlms_encode_datetime(entry->timestamp, &out_buf[idx]);
    idx += 12;

    /* 2. Event Vector Code (Long-Unsigned) */
    out_buf[idx++] = DLMS_AXDR_TAG_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->vector >> 8);
    out_buf[idx++] = (uint8_t)(entry->vector & 0xFF);

    /* 3. Active Tamper Mask (Double-Long-Unsigned) */
    out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->active_mask >> 24);
    out_buf[idx++] = (uint8_t)(entry->active_mask >> 16);
    out_buf[idx++] = (uint8_t)(entry->active_mask >> 8);
    out_buf[idx++] = (uint8_t)(entry->active_mask & 0xFF);

    return idx;
}

size_t spln_log_encode_dlms_entry(const spln_standard_log_entry_t *entry, uint8_t *out_buf, size_t max_len) {
    if (entry == NULL || out_buf == NULL || max_len < 27) return 0;

    size_t idx = 0;

    /* Structure of 4 elements: [Timestamp, Event Code, Status Flags, Parameter] */
    out_buf[idx++] = DLMS_AXDR_TAG_STRUCTURE;
    out_buf[idx++] = 4;

    /* 1. Timestamp (Octet-String 12-byte) */
    out_buf[idx++] = DLMS_AXDR_TAG_OCTET_STRING;
    out_buf[idx++] = 12;
    dlms_encode_datetime(entry->timestamp_epoch, &out_buf[idx]);
    idx += 12;

    /* 2. Event Code (Long-Unsigned) */
    out_buf[idx++] = DLMS_AXDR_TAG_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->event_code >> 8);
    out_buf[idx++] = (uint8_t)(entry->event_code & 0xFF);

    /* 3. Status Flags (Long-Unsigned) */
    out_buf[idx++] = DLMS_AXDR_TAG_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->status_flags >> 8);
    out_buf[idx++] = (uint8_t)(entry->status_flags & 0xFF);

    /* 4. Parameter Value (Double-Long-Unsigned) */
    out_buf[idx++] = DLMS_AXDR_TAG_DOUBLE_LONG_U;
    out_buf[idx++] = (uint8_t)(entry->parameter_val >> 24);
    out_buf[idx++] = (uint8_t)(entry->parameter_val >> 16);
    out_buf[idx++] = (uint8_t)(entry->parameter_val >> 8);
    out_buf[idx++] = (uint8_t)(entry->parameter_val & 0xFF);

    return idx;
}

bool dlms_decode_verify_structure(const uint8_t *buf, size_t len, uint8_t expected_elements) {
    if (buf == NULL || len < 2) return false;
    if (buf[0] != DLMS_AXDR_TAG_STRUCTURE) return false;
    if (buf[1] != expected_elements) return false;

    size_t idx = 2;
    for (uint8_t elem = 0; elem < expected_elements; elem++) {
        if (idx >= len) return false; /* Underflow */

        uint8_t tag = buf[idx++];
        switch (tag) {
            case DLMS_AXDR_TAG_BOOLEAN:
            case DLMS_AXDR_TAG_UNSIGNED:
                if (idx + 1 > len) return false;
                idx += 1;
                break;

            case DLMS_AXDR_TAG_LONG_U:
            case DLMS_AXDR_TAG_LONG:
                if (idx + 2 > len) return false;
                idx += 2;
                break;

            case DLMS_AXDR_TAG_DOUBLE_LONG_U:
                if (idx + 4 > len) return false;
                idx += 4;
                break;

            case DLMS_AXDR_TAG_LONG64_U:
                if (idx + 8 > len) return false;
                idx += 8;
                break;

            case DLMS_AXDR_TAG_OCTET_STRING: {
                if (idx >= len) return false;
                uint8_t str_len = buf[idx++];
                if (idx + str_len > len) return false;
                idx += str_len;
                break;
            }

            default:
                return false; /* Unknown / unsupported tag */
        }
    }

    return (idx == len);
}
