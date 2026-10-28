/**
 * @file load_profile.c
 * @brief Implementasi Modul Basis Data Profil Beban 15-Menitan (E3/ENG-3)
 *        Termasuk Snapshot Billing Bulanan (12 Bulan) dan Maximum Demand (MD 15-Menit)
 */

#include "load_profile.h"
#include <string.h>

/**
 * @brief Menghitung CRC-16 (CCITT-FALSE) untuk integritas data entri load profile
 */
uint16_t load_profile_calc_crc16(const load_profile_entry_t *entry) {
    if (entry == NULL) return 0;

    const uint8_t *data = (const uint8_t *)entry;
    size_t length = offsetof(load_profile_entry_t, crc16); // Byte sebelum crc16
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= ((uint16_t)data[i]) << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void load_profile_init(load_profile_ctx_t *ctx) {
    if (ctx == NULL) return;
    memset(ctx, 0, sizeof(load_profile_ctx_t));
}

bool load_profile_add_entry(load_profile_ctx_t *ctx, load_profile_entry_t *entry) {
    if (ctx == NULL || entry == NULL) return false;

    /* Hitung dan sematkan CRC-16 sebelum disimpan */
    entry->crc16 = load_profile_calc_crc16(entry);

    /* Simpan ke ring buffer */
    ctx->entries[ctx->head] = *entry;
    ctx->head = (ctx->head + 1) % LOAD_PROFILE_MAX_ENTRIES;

    if (ctx->count < LOAD_PROFILE_MAX_ENTRIES) {
        ctx->count++;
    }
    ctx->total_written++;

    return true;
}

bool load_profile_get_entry(const load_profile_ctx_t *ctx, size_t index, load_profile_entry_t *out_entry) {
    if (ctx == NULL || out_entry == NULL || index >= ctx->count) {
        return false;
    }

    /* Hitung indeks fisik dari paling lama (oldest) ke paling baru (newest) */
    size_t start_idx;
    if (ctx->count < LOAD_PROFILE_MAX_ENTRIES) {
        start_idx = 0;
    } else {
        start_idx = ctx->head;
    }

    size_t actual_idx = (start_idx + index) % LOAD_PROFILE_MAX_ENTRIES;
    *out_entry = ctx->entries[actual_idx];

    return true;
}

size_t load_profile_get_count(const load_profile_ctx_t *ctx) {
    return (ctx != NULL) ? ctx->count : 0;
}

bool load_profile_verify_integrity(const load_profile_entry_t *entry) {
    if (entry == NULL) return false;
    uint16_t expected_crc = load_profile_calc_crc16(entry);
    return (entry->crc16 == expected_crc);
}

/* ========================================================================= */
/* IMPLEMENTASI SNAPSHOT BILLING BULANAN (12 BULAN) SPLN D3.006 BUTIR 6.13    */
/* ========================================================================= */

uint16_t billing_snapshot_calc_crc16(const billing_snapshot_entry_t *entry) {
    if (entry == NULL) return 0;

    const uint8_t *data = (const uint8_t *)entry;
    size_t length = offsetof(billing_snapshot_entry_t, crc16);
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < length; i++) {
        crc ^= ((uint16_t)data[i]) << 8;
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void billing_snapshot_init(billing_snapshot_ctx_t *ctx) {
    if (ctx == NULL) return;
    memset(ctx, 0, sizeof(billing_snapshot_ctx_t));
}

bool billing_snapshot_record(billing_snapshot_ctx_t *ctx, billing_snapshot_entry_t *entry) {
    if (ctx == NULL || entry == NULL) return false;

    entry->crc16 = billing_snapshot_calc_crc16(entry);

    ctx->entries[ctx->head] = *entry;
    ctx->head = (ctx->head + 1) % BILLING_MAX_SNAPSHOTS;

    if (ctx->count < BILLING_MAX_SNAPSHOTS) {
        ctx->count++;
    }
    ctx->total_snapshots++;

    return true;
}

bool billing_snapshot_get_entry(const billing_snapshot_ctx_t *ctx, size_t index, billing_snapshot_entry_t *out_entry) {
    if (ctx == NULL || out_entry == NULL || index >= ctx->count) {
        return false;
    }

    size_t start_idx;
    if (ctx->count < BILLING_MAX_SNAPSHOTS) {
        start_idx = 0;
    } else {
        start_idx = ctx->head;
    }

    size_t actual_idx = (start_idx + index) % BILLING_MAX_SNAPSHOTS;
    *out_entry = ctx->entries[actual_idx];

    return true;
}

size_t billing_snapshot_get_count(const billing_snapshot_ctx_t *ctx) {
    return (ctx != NULL) ? ctx->count : 0;
}

bool billing_snapshot_verify_integrity(const billing_snapshot_entry_t *entry) {
    if (entry == NULL) return false;
    uint16_t expected_crc = billing_snapshot_calc_crc16(entry);
    return (entry->crc16 == expected_crc);
}

/* ========================================================================= */
/* IMPLEMENTASI MAXIMUM DEMAND (MD 15-MENIT) SPLN D3.006 TABEL E.1 NO 4-7     */
/* ========================================================================= */

void max_demand_init(max_demand_tracker_t *tracker, uint32_t interval_sec) {
    if (tracker == NULL) return;
    memset(tracker, 0, sizeof(max_demand_tracker_t));
    tracker->interval_sec = (interval_sec > 0) ? interval_sec : LOAD_PROFILE_INTERVAL_SEC;
}

void max_demand_process_second(max_demand_tracker_t *tracker, int32_t active_power_w, uint32_t epoch_sec) {
    if (tracker == NULL) return;

    /* Akumulasi daya aktif positif (impor) setiap detik */
    if (active_power_w > 0) {
        tracker->power_accum_w_sec += (int64_t)active_power_w;
    }

    tracker->timer_sec++;
    if (tracker->timer_sec >= tracker->interval_sec) {
        /* Hitung daya rata-rata interval 15 menit (Watt) */
        tracker->current_demand_w = (uint32_t)(tracker->power_accum_w_sec / tracker->interval_sec);

        /* Jika demand interval saat ini melampaui rekor MD periode berjalan, catat sebagai MD baru */
        if (tracker->current_demand_w > tracker->max_demand_w) {
            tracker->max_demand_w = tracker->current_demand_w;
            tracker->max_demand_timestamp = epoch_sec;
        }

        /* Reset akumulator untuk interval 15-menit berikutnya */
        tracker->power_accum_w_sec = 0;
        tracker->timer_sec = 0;
    }
}

void max_demand_reset(max_demand_tracker_t *tracker) {
    if (tracker == NULL) return;

    /* Sesuai SPLN D3.006: Saat reset billing, MD ditambahkan ke Kumulatif MD (CMD) */
    tracker->cumulative_max_demand_w += tracker->max_demand_w;
    tracker->reset_count++;

    tracker->max_demand_w = 0;
    tracker->max_demand_timestamp = 0;
    tracker->current_demand_w = 0;
    tracker->power_accum_w_sec = 0;
    tracker->timer_sec = 0;
}
