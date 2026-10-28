/**
 * @file power_relay_manager.c
 * @brief Implementasi Pengelola Asimetri Beban, Pembatas Daya, Proteksi Tegangan (Cut-Off),
 *        dan State Relai "READY ON" Sesuai Standar SPLN D3.006: 2021 Tabel 5 & Butir 10.3 Tabel 9
 */

#include "power_relay_manager.h"
#include <stdio.h>

void power_relay_init(power_relay_context_t *ctx, uint32_t power_limit_w, uint32_t trip_sec, uint32_t asym_pct) {
    if (ctx == NULL) return;

    ctx->power_limit_w = (power_limit_w > 0) ? power_limit_w : 6600;
    ctx->over_power_duration_sec = (trip_sec > 0) ? trip_sec : 5;
    ctx->asymmetry_threshold_pct = (asym_pct > 0) ? asym_pct : 30;

    ctx->relay_state = RELAY_STATE_CONNECTED;
    ctx->last_reason = RELAY_REASON_NONE;
    ctx->is_asymmetry_active = false;
    ctx->current_deviation_pct = 0;
    ctx->is_limiter_tripped = false;
    ctx->over_power_timer_sec = 0;
    ctx->last_event_timestamp = 0;

    ctx->is_under_voltage_trip = false;
    ctx->is_over_voltage_trip = false;
}

bool power_relay_check_asymmetry(power_relay_context_t *ctx, uint32_t i_r_ma, uint32_t i_s_ma, uint32_t i_t_ma) {
    if (ctx == NULL) return false;

    /* Rata-rata arus ketiga fasa */
    uint32_t i_avg = (i_r_ma + i_s_ma + i_t_ma) / 3;

    /* Hindari false alarm saat kondisi tanpa beban (arus < 500 mA) */
    if (i_avg < 500) {
        ctx->current_deviation_pct = 0;
        ctx->is_asymmetry_active = false;
        return false;
    }

    /* Hitung selisih absolut tiap fasa terhadap rata-rata */
    uint32_t diff_r = (i_r_ma >= i_avg) ? (i_r_ma - i_avg) : (i_avg - i_r_ma);
    uint32_t diff_s = (i_s_ma >= i_avg) ? (i_s_ma - i_avg) : (i_avg - i_s_ma);
    uint32_t diff_t = (i_t_ma >= i_avg) ? (i_t_ma - i_avg) : (i_avg - i_t_ma);

    /* Cari deviasi maksimum */
    uint32_t max_diff = diff_r;
    if (diff_s > max_diff) max_diff = diff_s;
    if (diff_t > max_diff) max_diff = diff_t;

    /* Deviasi = (Deviasi Maksimum / Rata-rata Arus) x 100% per SPLN Tabel 5 No 11 */
    uint32_t dev_pct = (uint32_t)(((uint64_t)max_diff * 100) / i_avg);
    ctx->current_deviation_pct = dev_pct;

    if (dev_pct > ctx->asymmetry_threshold_pct) {
        ctx->is_asymmetry_active = true;
        return true;
    } else {
        ctx->is_asymmetry_active = false;
        return false;
    }
}

bool power_relay_process_power(power_relay_context_t *ctx, int32_t active_power_w, uint32_t timestamp) {
    if (ctx == NULL) return false;

    /* Jika daya aktif melebihi ambang batas kontrak */
    if (active_power_w > (int32_t)ctx->power_limit_w) {
        ctx->over_power_timer_sec++;
        if (ctx->over_power_timer_sec >= ctx->over_power_duration_sec) {
            ctx->is_limiter_tripped = true;
            if (ctx->relay_state == RELAY_STATE_CONNECTED) {
                power_relay_disconnect(ctx, RELAY_REASON_POWER_LIMITER, timestamp);
                return true; /* Trip terjadi */
            }
        }
    } else {
        ctx->over_power_timer_sec = 0;
        ctx->is_limiter_tripped = false;
    }

    return false;
}

bool power_relay_process_voltage(power_relay_context_t *ctx, uint32_t v_r_dv, uint32_t v_s_dv, uint32_t v_t_dv, uint32_t timestamp) {
    if (ctx == NULL) return false;

    /* SPLN D3.006: 2021 Butir 10.3 Tabel 9:
     * - Ketika tegangan pasok di bawah 110 V, maka relai akan membuka.
     * - Ketika tegangan pasok di atas 115 V, maka relai akan menutup kembali.
     * - Serta ketika tegangan pasok di atas 280 V, maka relai akan membuka.
     * - Dan ketika tegangan pasok di bawah 276 V, maka relai akan menutup kembali.
     * Operasi relai memutus semua fase secara serempak.
     * (Abaikan v_dv < 100 / < 10.0V yang merupakan indikasi padam total / saluran mati).
     */
    bool has_under_voltage = ((v_r_dv > 100U && v_r_dv < SPLN_VOLT_UNDER_CUTOFF_DV) ||
                              (v_s_dv > 100U && v_s_dv < SPLN_VOLT_UNDER_CUTOFF_DV) ||
                              (v_t_dv > 100U && v_t_dv < SPLN_VOLT_UNDER_CUTOFF_DV));

    bool has_over_voltage  = (v_r_dv > SPLN_VOLT_OVER_CUTOFF_DV ||
                              v_s_dv > SPLN_VOLT_OVER_CUTOFF_DV ||
                              v_t_dv > SPLN_VOLT_OVER_CUTOFF_DV);

    /* 1. Pemutusan (Trip) akibat Under-Voltage (< 110 V) */
    if (has_under_voltage) {
        ctx->is_under_voltage_trip = true;
        if (ctx->relay_state == RELAY_STATE_CONNECTED) {
            power_relay_disconnect(ctx, RELAY_REASON_UNDER_VOLTAGE, timestamp);
            return true;
        }
        return false;
    }

    /* 2. Pemutusan (Trip) akibat Over-Voltage (> 280 V) */
    if (has_over_voltage) {
        ctx->is_over_voltage_trip = true;
        if (ctx->relay_state == RELAY_STATE_CONNECTED) {
            power_relay_disconnect(ctx, RELAY_REASON_OVER_VOLTAGE, timestamp);
            return true;
        }
        return false;
    }

    /* 3. Pemulihan Otomatis Under-Voltage: Menutup kembali saat semua fasa > 115 V */
    if (ctx->is_under_voltage_trip && ctx->last_reason == RELAY_REASON_UNDER_VOLTAGE) {
        bool all_above_115 = ((v_r_dv == 0 || v_r_dv > SPLN_VOLT_UNDER_RECLOSE_DV) &&
                              (v_s_dv == 0 || v_s_dv > SPLN_VOLT_UNDER_RECLOSE_DV) &&
                              (v_t_dv == 0 || v_t_dv > SPLN_VOLT_UNDER_RECLOSE_DV));
        if (all_above_115 && !has_over_voltage) {
            ctx->relay_state = RELAY_STATE_CONNECTED;
            ctx->last_reason = RELAY_REASON_NONE;
            ctx->is_under_voltage_trip = false;
            ctx->last_event_timestamp = timestamp;
            return false;
        }
    }

    /* 4. Pemulihan Otomatis Over-Voltage: Menutup kembali saat semua fasa < 276 V */
    if (ctx->is_over_voltage_trip && ctx->last_reason == RELAY_REASON_OVER_VOLTAGE) {
        bool all_below_276 = (v_r_dv < SPLN_VOLT_OVER_RECLOSE_DV &&
                              v_s_dv < SPLN_VOLT_OVER_RECLOSE_DV &&
                              v_t_dv < SPLN_VOLT_OVER_RECLOSE_DV);
        if (all_below_276 && !has_under_voltage) {
            ctx->relay_state = RELAY_STATE_CONNECTED;
            ctx->last_reason = RELAY_REASON_NONE;
            ctx->is_over_voltage_trip = false;
            ctx->last_event_timestamp = timestamp;
            return false;
        }
    }

    return false;
}

void power_relay_disconnect(power_relay_context_t *ctx, relay_reason_t reason, uint32_t timestamp) {
    if (ctx == NULL) return;

    ctx->relay_state = RELAY_STATE_DISCONNECTED;
    ctx->last_reason = reason;
    ctx->last_event_timestamp = timestamp;
}

void power_relay_request_reconnect(power_relay_context_t *ctx, uint32_t timestamp) {
    if (ctx == NULL) return;

    /* Sesuai SPLN Tabel 5 No 12: Remote reconnect masuk ke state READY ON */
    if (ctx->relay_state == RELAY_STATE_DISCONNECTED) {
        ctx->relay_state = RELAY_STATE_READY_ON;
        ctx->last_reason = RELAY_REASON_REMOTE_RECONNECT;
        ctx->last_event_timestamp = timestamp;
    }
}

bool power_relay_confirm_reconnect(power_relay_context_t *ctx, uint32_t timestamp) {
    if (ctx == NULL) return false;

    /* Aksi konfirmasi lokal pengguna menyambungkan relai kembali ke CONNECTED */
    if (ctx->relay_state == RELAY_STATE_READY_ON) {
        ctx->relay_state = RELAY_STATE_CONNECTED;
        ctx->last_reason = RELAY_REASON_LOCAL_CONFIRM;
        ctx->last_event_timestamp = timestamp;
        ctx->over_power_timer_sec = 0;
        ctx->is_limiter_tripped = false;
        ctx->is_under_voltage_trip = false;
        ctx->is_over_voltage_trip = false;
        return true;
    }

    return false;
}

relay_state_t power_relay_get_state(const power_relay_context_t *ctx) {
    if (ctx == NULL) return RELAY_STATE_CONNECTED;
    return ctx->relay_state;
}

bool power_relay_is_ready_on(const power_relay_context_t *ctx) {
    if (ctx == NULL) return false;
    return (ctx->relay_state == RELAY_STATE_READY_ON);
}

const char* power_relay_state_to_string(relay_state_t state) {
    switch (state) {
        case RELAY_STATE_CONNECTED:    return "CONNECTED";
        case RELAY_STATE_DISCONNECTED: return "DISCONNECTED";
        case RELAY_STATE_READY_ON:     return "READY ON";
        default:                       return "UNKNOWN";
    }
}

const char* power_relay_reason_to_string(relay_reason_t reason) {
    switch (reason) {
        case RELAY_REASON_NONE:              return "NONE";
        case RELAY_REASON_MANUAL:            return "MANUAL";
        case RELAY_REASON_POWER_LIMITER:     return "POWER_LIMITER";
        case RELAY_REASON_REMOTE_DISCONNECT: return "REMOTE_DISCONNECT";
        case RELAY_REASON_REMOTE_RECONNECT:  return "REMOTE_RECONNECT";
        case RELAY_REASON_LOCAL_CONFIRM:     return "LOCAL_CONFIRM";
        case RELAY_REASON_UNDER_VOLTAGE:     return "UNDER_VOLTAGE (<110V)";
        case RELAY_REASON_OVER_VOLTAGE:      return "OVER_VOLTAGE (>280V)";
        default:                             return "UNKNOWN";
    }
}
