/**
 * @file tamper_manager.c
 * @brief Implementasi Modul Pengelola Sabotase, Alarm Hardware, dan 3 Register Log FIFO Non-Volatile
 *        Sesuai SPLN D3.006: 2021 Butir 12.3 Tabel 18 (Hal. 33-34) & Tabel 6
 */

#include "tamper_manager.h"
#include <string.h>

static spln_tamper_log_code_t tamper_vector_to_log_code(tamper_vector_t vector) {
    switch (vector) {
        case TAMPER_VECTOR_CASE_OPEN:      return SPLN_LOG_TAMPER_CASE_OPEN;
        case TAMPER_VECTOR_TERMINAL_OPEN:  return SPLN_LOG_TAMPER_TERMINAL_OPEN;
        case TAMPER_VECTOR_WRONG_SEQUENCE: return SPLN_LOG_TAMPER_WRONG_SEQUENCE;
        case TAMPER_VECTOR_CROSS_PHASE:    return SPLN_LOG_TAMPER_CROSS_PHASE;
        case TAMPER_VECTOR_NEUTRAL_BYPASS: return SPLN_LOG_TAMPER_MISSING_NEUTRAL;
        case TAMPER_VECTOR_PHASE_LOSS:     return SPLN_LOG_TAMPER_PHASE_LOSS;
        case TAMPER_VECTOR_MAGNETIC_FIELD: return SPLN_LOG_TAMPER_MAGNETIC_FIELD;
        case TAMPER_VECTOR_REVERSE_POWER:  return SPLN_LOG_TAMPER_REVERSE_POWER;
        case TAMPER_VECTOR_LOW_PF_QUAD4:   return SPLN_LOG_TAMPER_LOW_PF_QUAD4;
        default:                           return SPLN_LOG_TAMPER_NONE;
    }
}

static void fifo_push_entry(spln_log_fifo_t *fifo, const spln_standard_log_entry_t *entry) {
    if (fifo == NULL || entry == NULL) return;

    fifo->records[fifo->head] = *entry;
    fifo->head = (fifo->head + 1) % SPLN_LOG_FIFO_MAX_ENTRIES;
    if (fifo->count < SPLN_LOG_FIFO_MAX_ENTRIES) {
        fifo->count++;
    }
    fifo->total_events++;
}

void tamper_init(tamper_context_t *ctx) {
    if (ctx == NULL) return;
    memset(ctx, 0, sizeof(tamper_context_t));
}

bool tamper_process_signal(tamper_context_t *ctx, tamper_vector_t vector, bool is_asserted, uint32_t timestamp) {
    if (ctx == NULL) return false;

    uint32_t prev_mask = ctx->active_tamper_mask;

    if (is_asserted) {
        ctx->active_tamper_mask |= (uint32_t)vector;
    } else {
        ctx->active_tamper_mask &= ~((uint32_t)vector);
    }

    /* Jika terjadi perubahan status kecurangan */
    if (prev_mask != ctx->active_tamper_mask) {
        /* 1. Catat ke legacy log buffer (kompatibilitas penuh) */
        tamper_event_entry_t *entry = &ctx->log_fifo[ctx->fifo_head];
        entry->timestamp = timestamp;
        entry->vector = vector;
        entry->is_asserted = is_asserted;
        entry->active_mask = ctx->active_tamper_mask;

        ctx->fifo_head = (ctx->fifo_head + 1) % TAMPER_LOG_MAX_ENTRIES;
        if (ctx->fifo_count < TAMPER_LOG_MAX_ENTRIES) ctx->fifo_count++;
        ctx->total_event_count++;

        /* 2. Catat ke Register Log 3: Tamper Log FIFO (SPLN Butir 12.3 Tabel 18 No. 20-28) */
        spln_tamper_log_code_t lcode = tamper_vector_to_log_code(vector);
        (void)spln_log_record_tamper(ctx, lcode, is_asserted, timestamp, ctx->active_tamper_mask);

        /* 3. Sesuai SPLN D3.006: 2021 Tabel 6:
         * LED Kuning aktif HANYA untuk vektor pada TAMPER_MASK_LED_YELLOW.
         * Untuk Tutup Meter (1) dan Medan Magnet (7), kolom LED kuning adalah '-' (mati). */
        ctx->alarm_led_status = ((ctx->active_tamper_mask & TAMPER_MASK_LED_YELLOW) != 0);

        /* 4. Sesuai SPLN D3.006: 2021 Tabel 6:
         * Relai membuka pada: Pembukaan Tutup Meter (1), Tutup Terminal (2),
         * Urutan Fase Terbalik (3), Arus/Tegangan Tidak Sefase (4), Hilang Tegangan 1/2 Fase (6).
         * Catatan 4: Operasi relai TIDAK terpengaruh pada induksi medan magnet sampai 500 mT. */
        ctx->alarm_relay_trigger = ((ctx->active_tamper_mask & TAMPER_MASK_RELAY_TRIP) != 0);

        return true;
    }
    return false;
}

uint32_t tamper_get_active_mask(const tamper_context_t *ctx) {
    return (ctx != NULL) ? ctx->active_tamper_mask : 0;
}

void tamper_get_log_entries(const tamper_context_t *ctx, tamper_event_entry_t *out_buffer, size_t max_records, size_t *out_count) {
    if (ctx == NULL || out_buffer == NULL || out_count == NULL || max_records == 0) {
        if (out_count) *out_count = 0;
        return;
    }

    size_t count_to_copy = (ctx->fifo_count < max_records) ? ctx->fifo_count : max_records;
    size_t start_index = (ctx->fifo_count < TAMPER_LOG_MAX_ENTRIES) ? 0 : ctx->fifo_head;

    for (size_t i = 0; i < count_to_copy; i++) {
        size_t idx = (start_index + i) % TAMPER_LOG_MAX_ENTRIES;
        out_buffer[i] = ctx->log_fifo[idx];
    }
    *out_count = count_to_copy;
}

bool tamper_is_alarm_pending(const tamper_context_t *ctx) {
    return (ctx != NULL) && (ctx->active_tamper_mask != 0);
}

bool tamper_should_trip_relay(const tamper_context_t *ctx) {
    if (ctx == NULL) return false;
    return ((ctx->active_tamper_mask & TAMPER_MASK_RELAY_TRIP) != 0);
}

bool tamper_should_turn_on_led(const tamper_context_t *ctx) {
    if (ctx == NULL) return false;
    return ((ctx->active_tamper_mask & TAMPER_MASK_LED_YELLOW) != 0);
}

/* ========================================================================= */
/*  Operasi 3 Register Log FIFO Non-Volatile Sesuai SPLN Butir 12.3          */
/* ========================================================================= */

bool spln_log_record_alarm(tamper_context_t *ctx, spln_alarm_log_code_t code, bool is_asserted, uint32_t timestamp, uint32_t param) {
    if (ctx == NULL || code == SPLN_LOG_ALARM_NONE) return false;

    spln_standard_log_entry_t entry;
    entry.timestamp_epoch = timestamp;
    entry.event_code = (uint16_t)code;
    entry.status_flags = is_asserted ? 1U : 0U;
    entry.parameter_val = param;

    fifo_push_entry(&ctx->alarm_log_fifo, &entry);

    /* Update bitstring di Error Register (bit 0..7 untuk Alarm) */
    uint32_t bit_pos = ((uint32_t)code - 1U);
    if (is_asserted) {
        ctx->error_register_bitstring |= (1U << bit_pos);
    } else {
        ctx->error_register_bitstring &= ~(1U << bit_pos);
    }

    return true;
}

bool spln_log_record_event(tamper_context_t *ctx, spln_event_log_code_t code, bool is_asserted, uint32_t timestamp, uint32_t param) {
    if (ctx == NULL || code == SPLN_LOG_EVENT_NONE) return false;

    spln_standard_log_entry_t entry;
    entry.timestamp_epoch = timestamp;
    entry.event_code = (uint16_t)code;
    entry.status_flags = is_asserted ? 1U : 0U;
    entry.parameter_val = param;

    fifo_push_entry(&ctx->event_log_fifo, &entry);

    /* Update bitstring di Error Register (bit 8..19 untuk Event) */
    uint32_t bit_pos = 8U + ((uint32_t)code - 1U);
    if (is_asserted) {
        ctx->error_register_bitstring |= (1U << bit_pos);
    } else {
        ctx->error_register_bitstring &= ~(1U << bit_pos);
    }

    return true;
}

bool spln_log_record_tamper(tamper_context_t *ctx, spln_tamper_log_code_t code, bool is_asserted, uint32_t timestamp, uint32_t param) {
    if (ctx == NULL || code == SPLN_LOG_TAMPER_NONE) return false;

    spln_standard_log_entry_t entry;
    entry.timestamp_epoch = timestamp;
    entry.event_code = (uint16_t)code;
    entry.status_flags = is_asserted ? 1U : 0U;
    entry.parameter_val = param;

    fifo_push_entry(&ctx->tamper_log_fifo, &entry);

    /* Update bitstring di Error Register (bit 20..28 untuk Tamper) */
    uint32_t bit_pos = 20U + ((uint32_t)code - 1U);
    if (is_asserted) {
        ctx->error_register_bitstring |= (1U << bit_pos);
    } else {
        ctx->error_register_bitstring &= ~(1U << bit_pos);
    }

    return true;
}

void spln_log_get_entries(const spln_log_fifo_t *fifo, spln_standard_log_entry_t *out_buffer, size_t max_records, size_t *out_count) {
    if (fifo == NULL || out_buffer == NULL || out_count == NULL || max_records == 0) {
        if (out_count) *out_count = 0;
        return;
    }

    size_t count_to_copy = (fifo->count < max_records) ? fifo->count : max_records;
    size_t start_index = (fifo->count < SPLN_LOG_FIFO_MAX_ENTRIES) ? 0 : fifo->head;

    for (size_t i = 0; i < count_to_copy; i++) {
        size_t idx = (start_index + i) % SPLN_LOG_FIFO_MAX_ENTRIES;
        out_buffer[i] = fifo->records[idx];
    }
    *out_count = count_to_copy;
}

bool spln_log_get_entry_by_index(const spln_log_fifo_t *fifo, size_t index, spln_standard_log_entry_t *out_entry) {
    if (fifo == NULL || out_entry == NULL || index >= fifo->count) return false;

    size_t start_index = (fifo->count < SPLN_LOG_FIFO_MAX_ENTRIES) ? 0 : fifo->head;
    size_t actual_idx = (start_index + index) % SPLN_LOG_FIFO_MAX_ENTRIES;

    *out_entry = fifo->records[actual_idx];
    return true;
}

uint32_t spln_log_get_error_register(const tamper_context_t *ctx) {
    return (ctx != NULL) ? ctx->error_register_bitstring : 0U;
}

/* ========================================================================= */
/*  Konversi DLMS                                                            */
/* ========================================================================= */

dlms_tamper_code_t tamper_vector_to_dlms_code(tamper_vector_t vector) {
    if (vector & TAMPER_VECTOR_CASE_OPEN) return DLMS_TAMPER_METER_COVER_OPEN;
    if (vector & TAMPER_VECTOR_TERMINAL_OPEN) return DLMS_TAMPER_TERMINAL_COVER_OPEN;
    if (vector & TAMPER_VECTOR_MAGNETIC_FIELD) return DLMS_TAMPER_MAGNETIC_INDUCTION;
    if (vector & TAMPER_VECTOR_REVERSE_POWER) return DLMS_TAMPER_REVERSE_CURRENT;
    if (vector & TAMPER_VECTOR_NEUTRAL_BYPASS) return DLMS_TAMPER_MISSING_NEUTRAL;
    if (vector & TAMPER_VECTOR_WRONG_SEQUENCE) return DLMS_TAMPER_WRONG_PHASE_SEQ;
    if (vector & TAMPER_VECTOR_CROSS_PHASE) return DLMS_TAMPER_CROSS_PHASE;
    if (vector & TAMPER_VECTOR_PHASE_LOSS) return DLMS_TAMPER_MISSING_VOLTAGE;
    if (vector & TAMPER_VECTOR_LOW_PF_QUAD4) return DLMS_TAMPER_LOW_PF_QUAD4;
    return DLMS_TAMPER_NONE;
}

tamper_vector_t dlms_code_to_tamper_vector(dlms_tamper_code_t code) {
    switch (code) {
        case DLMS_TAMPER_METER_COVER_OPEN: return TAMPER_VECTOR_CASE_OPEN;
        case DLMS_TAMPER_TERMINAL_COVER_OPEN: return TAMPER_VECTOR_TERMINAL_OPEN;
        case DLMS_TAMPER_MAGNETIC_INDUCTION: return TAMPER_VECTOR_MAGNETIC_FIELD;
        case DLMS_TAMPER_REVERSE_CURRENT: return TAMPER_VECTOR_REVERSE_POWER;
        case DLMS_TAMPER_MISSING_NEUTRAL: return TAMPER_VECTOR_NEUTRAL_BYPASS;
        case DLMS_TAMPER_WRONG_PHASE_SEQ: return TAMPER_VECTOR_WRONG_SEQUENCE;
        case DLMS_TAMPER_CROSS_PHASE: return TAMPER_VECTOR_CROSS_PHASE;
        case DLMS_TAMPER_MISSING_VOLTAGE: return TAMPER_VECTOR_PHASE_LOSS;
        case DLMS_TAMPER_LOW_PF_QUAD4: return TAMPER_VECTOR_LOW_PF_QUAD4;
        default: return (tamper_vector_t)0;
    }
}

#if defined(USE_FREERTOS)
#include "rtos_tasks.h"
#include "display_task.h"

void tamper_handle_button_press_isr(uint32_t timestamp_ms) {
    static uint32_t s_last_btn_tick = 0;
    if (timestamp_ms - s_last_btn_tick > 250) {
        s_last_btn_tick = timestamp_ms;
        bool current = display_task_is_alarm_active();
        bool new_state = !current;
        display_task_trigger_instant_refresh(new_state);
    }
}
#else
void tamper_handle_button_press_isr(uint32_t timestamp_ms) {
    (void)timestamp_ms;
}
#endif
