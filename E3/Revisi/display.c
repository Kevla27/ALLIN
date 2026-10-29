/**
 * @file display.c
 * @brief Implementasi Modul Tampilan LCD & Carousel UI (E3/ENG-3)
 */

#include "display.h"
#include <stdio.h>
#include <string.h>

static void display_epoch_to_date(uint32_t epoch_sec, uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *hour, uint8_t *min, uint8_t *sec) {
    uint32_t rem_sec = epoch_sec % 86400U;

    if (sec)  *sec  = (uint8_t)(rem_sec % 60U);
    if (min)  *min  = (uint8_t)((rem_sec / 60U) % 60U);
    if (hour) *hour = (uint8_t)(rem_sec / 3600U);

    uint32_t z = (epoch_sec / 86400U) + 719468U;
    uint32_t era = z / 146097U;
    uint32_t doe = z - era * 146097U;
    uint32_t yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U) / 365U;
    uint32_t y = yoe + era * 400U;
    uint32_t doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
    uint32_t mp = (5U * doy + 2U) / 153U;
    uint32_t d = doy - (153U * mp + 2U) / 5U + 1U;
    uint32_t m = (mp < 10U) ? (mp + 3U) : (mp - 9U);
    if (m <= 2U) {
        y += 1U;
    }

    if (day)   *day   = (uint8_t)d;
    if (month) *month = (uint8_t)m;
    if (year)  *year  = (uint16_t)y;
}

bool display_init(display_context_t *ctx, const char *customer_id, uint32_t carousel_interval_ms) {
    if (ctx == NULL) return false;
    memset(ctx, 0, sizeof(display_context_t));
    
    ctx->mode = DISPLAY_MODE_AUTO_CAROUSEL;
    ctx->current_page = DISP_PAGE_IDPEL;
    ctx->carousel_interval_ms = (carousel_interval_ms > 0) ? carousel_interval_ms : 5000;
    ctx->manual_timeout_ms = 10000; // Timeout manual scroll 10 detik
    ctx->current_time_epoch = 1791794500; // Basis waktu default (12-10-2026 08:41:40)

    if (customer_id != NULL) {
        strncpy(ctx->customer_id, customer_id, sizeof(ctx->customer_id) - 1);
    } else {
        strncpy(ctx->customer_id, "12345678901", sizeof(ctx->customer_id) - 1);
    }
    ctx->customer_id[sizeof(ctx->customer_id)-1]='\0';
    ctx->meas_buffer.power_limit_w = 11000U;
    ctx->meas_buffer.firmware_checksum = 0xA5B6U;
    ctx->meas_buffer.angle_ur_deg = 0;
    ctx->meas_buffer.angle_us_deg = 120;
    ctx->meas_buffer.angle_ut_deg = 240;
    ctx->meas_buffer.angle_ir_deg = 0;
    ctx->meas_buffer.angle_is_deg = 120;
    ctx->meas_buffer.angle_it_deg = 240;
    ctx->meas_buffer.eob_day = 1;
    ctx->meas_buffer.eob_hour = 10;
    strncpy(ctx->meas_buffer.pln_unit_code, "PLN UID JAYA", sizeof(ctx->meas_buffer.pln_unit_code) - 1);
    return true;
}

void display_set_time(display_context_t *ctx, uint32_t epoch_sec) {
    if (ctx != NULL) {
        ctx->current_time_epoch = epoch_sec;
    }
}

void display_set_relay_ready_on(display_context_t *ctx, bool ready_on) {
    if (ctx != NULL) {
        ctx->relay_ready_on = ready_on;
    }
}

void display_set_asymmetry_warning(display_context_t *ctx, bool warning) {
    if (ctx != NULL) {
        ctx->asymmetry_warning = warning;
    }
}

void display_set_modem_status(display_context_t *ctx, bool active) {
    if (ctx != NULL) {
        ctx->is_modem_active = active;
    }
}

void display_update_measurements(display_context_t *ctx, const meter_measurements_t *meas) {
    if (ctx != NULL && meas != NULL) {
        uint32_t saved_power_limit = ctx->meas_buffer.power_limit_w;
        uint16_t saved_checksum    = ctx->meas_buffer.firmware_checksum;
        uint8_t  saved_eob_day     = ctx->meas_buffer.eob_day;
        uint8_t  saved_eob_hour    = ctx->meas_buffer.eob_hour;
        char     saved_pln_unit[16];
        memcpy(saved_pln_unit, ctx->meas_buffer.pln_unit_code, sizeof(saved_pln_unit));

        memcpy(&ctx->meas_buffer, meas, sizeof(meter_measurements_t));

        if (ctx->meas_buffer.power_limit_w == 0U && saved_power_limit != 0U) {
            ctx->meas_buffer.power_limit_w = saved_power_limit;
        }
        if (ctx->meas_buffer.firmware_checksum == 0U && saved_checksum != 0U) {
            ctx->meas_buffer.firmware_checksum = saved_checksum;
        }
        if (ctx->meas_buffer.eob_day == 0U && saved_eob_day != 0U) {
            ctx->meas_buffer.eob_day = saved_eob_day;
            ctx->meas_buffer.eob_hour = saved_eob_hour;
        }
        if (ctx->meas_buffer.pln_unit_code[0] == '\0' && saved_pln_unit[0] != '\0') {
            memcpy(ctx->meas_buffer.pln_unit_code, saved_pln_unit, sizeof(ctx->meas_buffer.pln_unit_code));
        }
    }
}

void display_process_tick(display_context_t *ctx, uint32_t elapsed_ms) {
    if (ctx == NULL) return;

    ctx->timer_accumulator_ms += elapsed_ms;
    ctx->current_time_epoch += (elapsed_ms / 1000);

    if (ctx->mode == DISPLAY_MODE_AUTO_CAROUSEL) {
        if (ctx->timer_accumulator_ms >= ctx->carousel_interval_ms) {
            ctx->timer_accumulator_ms = 0;
            display_page_id_t next_page;
            if (ctx->current_page == DISP_AUTO_PAGE_LAST) {
                if (ctx->alarm_icon_active) {
                    next_page = DISP_PAGE_TAMPER_ALARM;
                } else if (ctx->relay_ready_on) {
                    next_page = DISP_PAGE_RELAY_READY_ON;
                } else {
                    next_page = DISP_AUTO_PAGE_FIRST;
                }
            } else if (ctx->current_page == DISP_PAGE_TAMPER_ALARM) {
                next_page = (ctx->relay_ready_on) ? DISP_PAGE_RELAY_READY_ON : DISP_AUTO_PAGE_FIRST;
            } else if (ctx->current_page == DISP_PAGE_RELAY_READY_ON) {
                next_page = DISP_AUTO_PAGE_FIRST;
            } else if (ctx->current_page < DISP_AUTO_PAGE_LAST) {
                next_page = (display_page_id_t)(ctx->current_page + 1);
            } else {
                next_page = (ctx->alarm_icon_active) ? DISP_PAGE_TAMPER_ALARM : ((ctx->relay_ready_on) ? DISP_PAGE_RELAY_READY_ON : DISP_AUTO_PAGE_FIRST);
            }
            ctx->current_page = next_page;
        }
    } else if (ctx->mode == DISPLAY_MODE_MANUAL_SCROLL) {
        if (ctx->timer_accumulator_ms >= ctx->manual_timeout_ms) {
            ctx->timer_accumulator_ms = 0;
            ctx->mode = DISPLAY_MODE_AUTO_CAROUSEL;
            ctx->current_page = DISP_AUTO_PAGE_FIRST;
        }
    }
}

void display_handle_button_press(display_context_t *ctx, button_dir_t dir) {
    if (ctx == NULL) return;

    ctx->mode = DISPLAY_MODE_MANUAL_SCROLL;
    ctx->timer_accumulator_ms = 0;

    if (dir == BUTTON_DIR_BOTH) {
        /* Tekan tombol simultan (▲ + ▼): Aksi konfirmasi lokal (SPLN Tabel 5 No 12) */
        if (!ctx->relay_ready_on) {
            ctx->current_page = DISP_PAGE_IDPEL;
        }
        return;
    }

    if (dir == BUTTON_DIR_DOWN) {
        display_page_id_t next_page = (display_page_id_t)((ctx->current_page + 1) % DISP_PAGE_COUNT);
        if (!ctx->alarm_icon_active && next_page == DISP_PAGE_TAMPER_ALARM) {
            next_page = (ctx->relay_ready_on) ? DISP_PAGE_RELAY_READY_ON : DISP_PAGE_IDPEL;
        } else if (!ctx->relay_ready_on && next_page == DISP_PAGE_RELAY_READY_ON) {
            next_page = DISP_PAGE_IDPEL;
        }
        ctx->current_page = next_page;
    } else if (dir == BUTTON_DIR_UP) {
        display_page_id_t prev_page = (ctx->current_page == 0) ? (display_page_id_t)(DISP_PAGE_COUNT - 1) 
                                                               : (display_page_id_t)(ctx->current_page - 1);
        display_page_id_t last_normal = (display_page_id_t)(DISP_PAGE_TAMPER_ALARM - 1);
        if (!ctx->relay_ready_on && prev_page == DISP_PAGE_RELAY_READY_ON) {
            prev_page = (ctx->alarm_icon_active) ? DISP_PAGE_TAMPER_ALARM : last_normal;
        }
        if (!ctx->alarm_icon_active && prev_page == DISP_PAGE_TAMPER_ALARM) {
            prev_page = last_normal;
        }
        ctx->current_page = prev_page;
    }
}

void display_set_tamper_status(display_context_t *ctx, uint32_t active_tamper_mask) {
    if (ctx == NULL) return;
    ctx->active_tamper_mask = active_tamper_mask;
    ctx->alarm_icon_active = (active_tamper_mask != 0 || ctx->active_alarm_mask != 0);
}

void display_set_internal_alarm(display_context_t *ctx, uint32_t active_alarm_mask) {
    if (ctx == NULL) return;
    ctx->active_alarm_mask = active_alarm_mask;
    ctx->alarm_icon_active = (ctx->active_tamper_mask != 0 || active_alarm_mask != 0);
}

uint32_t display_get_internal_alarm(const display_context_t *ctx) {
    if (ctx == NULL) return 0U;
    return ctx->active_alarm_mask;
}

void display_get_combined_error_info(uint32_t tamper_mask, uint32_t alarm_mask, uint32_t epoch_sec,
                                     char *out_text, size_t text_sz, char *out_code, size_t code_sz) {
    if (out_text == NULL || text_sz == 0 || out_code == NULL || code_sz == 0) return;

    /* Kumpulkan seluruh vektor sabotase aktif (SPLN D3.006: 2021 Tabel 6) */
    tamper_vector_t active_tampers[9];
    uint8_t n_tamper = 0;
    const tamper_vector_t all_tamp_vecs[] = {
        TAMPER_VECTOR_CASE_OPEN,
        TAMPER_VECTOR_TERMINAL_OPEN,
        TAMPER_VECTOR_WRONG_SEQUENCE,
        TAMPER_VECTOR_CROSS_PHASE,
        TAMPER_VECTOR_NEUTRAL_BYPASS,
        TAMPER_VECTOR_PHASE_LOSS,
        TAMPER_VECTOR_MAGNETIC_FIELD,
        TAMPER_VECTOR_REVERSE_POWER,
        TAMPER_VECTOR_LOW_PF_QUAD4
    };
    for (size_t i = 0; i < sizeof(all_tamp_vecs)/sizeof(all_tamp_vecs[0]); i++) {
        if (tamper_mask & all_tamp_vecs[i]) {
            active_tampers[n_tamper++] = all_tamp_vecs[i];
        }
    }

    /* Kumpulkan seluruh alarm hardware internal aktif (SPLN D3.006: 2021 Tabel 4) */
    spln_internal_alarm_t active_alarms[8];
    uint8_t n_alarm = 0;
    const spln_internal_alarm_t all_alarm_flags[] = {
        SPLN_ALARM_FLASH_ERROR,
        SPLN_ALARM_RAM_ERROR,
        SPLN_ALARM_RTC_ERROR,
        SPLN_ALARM_LOW_BATTERY,
        SPLN_ALARM_MCU_ERROR,
        SPLN_ALARM_ADC_ERROR,
        SPLN_ALARM_SUPERCAP_FAIL,
        SPLN_ALARM_RELAY_FAIL
    };
    for (size_t i = 0; i < sizeof(all_alarm_flags)/sizeof(all_alarm_flags[0]); i++) {
        if (alarm_mask & all_alarm_flags[i]) {
            active_alarms[n_alarm++] = all_alarm_flags[i];
        }
    }

    uint8_t total_errors = n_tamper + n_alarm;
    if (total_errors == 0) {
        snprintf(out_text, text_sz, "RUSAK");
        snprintf(out_code, code_sz, "-");
        return;
    }

    /* SPLN Butir 6.11: Jika terdapat beberapa kode respons aktif bersamaan,
     * ditampilkan bergantian dengan interval waktu 3 detik. */
    uint32_t sel_idx = (epoch_sec / 3U) % (uint32_t)total_errors;

    if (sel_idx < n_tamper) {
        tamper_vector_t vec = active_tampers[sel_idx];
        switch (vec) {
            case TAMPER_VECTOR_CASE_OPEN:
                snprintf(out_text, text_sz, "RUSAK");
                snprintf(out_code, code_sz, "-");
                break;
            case TAMPER_VECTOR_TERMINAL_OPEN:
                snprintf(out_text, text_sz, "PERIKSA");
                snprintf(out_code, code_sz, "ERR20");
                break;
            case TAMPER_VECTOR_WRONG_SEQUENCE:
                snprintf(out_text, text_sz, "PERIKSA");
                snprintf(out_code, code_sz, "ERR21");
                break;
            case TAMPER_VECTOR_CROSS_PHASE:
                snprintf(out_text, text_sz, "PERIKSA");
                snprintf(out_code, code_sz, "ERR22");
                break;
            case TAMPER_VECTOR_NEUTRAL_BYPASS:
                snprintf(out_text, text_sz, "PERIKSA");
                snprintf(out_code, code_sz, "ERR23");
                break;
            case TAMPER_VECTOR_PHASE_LOSS:
                snprintf(out_text, text_sz, "PERIKSA");
                snprintf(out_code, code_sz, "ERR24");
                break;
            case TAMPER_VECTOR_MAGNETIC_FIELD:
                snprintf(out_text, text_sz, "PERIKSA");
                snprintf(out_code, code_sz, "ERR25");
                break;
            case TAMPER_VECTOR_REVERSE_POWER:
                snprintf(out_text, text_sz, "REVERSE");
                snprintf(out_code, code_sz, "ERR26");
                break;
            case TAMPER_VECTOR_LOW_PF_QUAD4:
                snprintf(out_text, text_sz, "PERIKSA");
                snprintf(out_code, code_sz, "ERR27");
                break;
            default:
                snprintf(out_text, text_sz, "PERIKSA");
                snprintf(out_code, code_sz, "-");
                break;
        }
    } else {
        spln_internal_alarm_t alm = active_alarms[sel_idx - n_tamper];
        snprintf(out_text, text_sz, "-");
        switch (alm) {
            case SPLN_ALARM_FLASH_ERROR:   snprintf(out_code, code_sz, "ERR00"); break;
            case SPLN_ALARM_RAM_ERROR:     snprintf(out_code, code_sz, "ERR01"); break;
            case SPLN_ALARM_RTC_ERROR:     snprintf(out_code, code_sz, "ERR02"); break;
            case SPLN_ALARM_LOW_BATTERY:   snprintf(out_code, code_sz, "ERR03"); break;
            case SPLN_ALARM_MCU_ERROR:     snprintf(out_code, code_sz, "ERR04"); break;
            case SPLN_ALARM_ADC_ERROR:     snprintf(out_code, code_sz, "ERR05"); break;
            case SPLN_ALARM_SUPERCAP_FAIL: snprintf(out_code, code_sz, "ERR06"); break;
            case SPLN_ALARM_RELAY_FAIL:    snprintf(out_code, code_sz, "ERR07"); break;
            default:                       snprintf(out_code, code_sz, "-"); break;
        }
    }
}

void display_get_spln_tamper_info(uint32_t tamper_mask, char *out_text, size_t text_sz, char *out_code, size_t code_sz) {
    display_get_combined_error_info(tamper_mask, 0, 0, out_text, text_sz, out_code, code_sz);
}

void display_get_spln_alarm_info(uint32_t alarm_mask, char *out_text, size_t text_sz, char *out_code, size_t code_sz) {
    display_get_combined_error_info(0, alarm_mask, 0, out_text, text_sz, out_code, code_sz);
}

display_voltage_status_t display_get_voltage_status(uint32_t v_dvolts) {
    if (v_dvolts > 0 && v_dvolts < 1150U) { /* < 0.5 Un (115.0 V) - SPLN Butir 10.3 Tabel 9 */
        return DISPLAY_VOLT_LOW;
    } else if (v_dvolts > 2760U) {          /* > 1.2 Un (276.0 V) - SPLN Butir 10.3 Tabel 9 */
        return DISPLAY_VOLT_HIGH;
    }
    return DISPLAY_VOLT_NORMAL;
}

void display_render_frame(const display_context_t *ctx, char *out_line1, char *out_line2, char *out_line3, size_t max_len) {
    if (ctx == NULL || out_line1 == NULL || out_line2 == NULL || out_line3 == NULL) return;

    bool has_voltage_anomaly = (display_get_voltage_status(ctx->meas_buffer.voltage_r_dvolts) != DISPLAY_VOLT_NORMAL) ||
                               (display_get_voltage_status(ctx->meas_buffer.voltage_s_dvolts) != DISPLAY_VOLT_NORMAL) ||
                               (display_get_voltage_status(ctx->meas_buffer.voltage_t_dvolts) != DISPLAY_VOLT_NORMAL);
    bool alarm_on = (ctx->alarm_icon_active || ctx->asymmetry_warning || has_voltage_anomaly);
    snprintf(out_line1, max_len, "[%s] %s", 
             (ctx->mode == DISPLAY_MODE_AUTO_CAROUSEL) ? "AUTO" : "MANUAL",
             alarm_on ? "(!)" : " OK ");

    switch (ctx->current_page) {
        case DISP_PAGE_IDPEL:
            snprintf(out_line2, max_len, "IDPEL:");
            snprintf(out_line3, max_len, "%s", ctx->customer_id);
            break;
        case DISP_PAGE_VOLTAGE_R: {
            uint32_t v = ctx->meas_buffer.voltage_r_dvolts;
            display_voltage_status_t st = display_get_voltage_status(v);
            const char *tag = (st == DISPLAY_VOLT_LOW)  ? " [Lo Volt]" :
                              (st == DISPLAY_VOLT_HIGH) ? " [Hi Volt]" : "";
            snprintf(out_line2, max_len, "VOLTAGE PHASE R (UR)%s", tag);
            if (st == DISPLAY_VOLT_LOW) {
                snprintf(out_line3, max_len, "Lo Volt (%lu.%lu V)", (unsigned long)(v / 10), (unsigned long)(v % 10));
            } else if (st == DISPLAY_VOLT_HIGH) {
                snprintf(out_line3, max_len, "Hi Volt (%lu.%lu V)", (unsigned long)(v / 10), (unsigned long)(v % 10));
            } else {
                snprintf(out_line3, max_len, "%lu.%lu V", (unsigned long)(v / 10), (unsigned long)(v % 10));
            }
            break;
        }
        case DISP_PAGE_VOLTAGE_S: {
            uint32_t v = ctx->meas_buffer.voltage_s_dvolts;
            display_voltage_status_t st = display_get_voltage_status(v);
            const char *tag = (st == DISPLAY_VOLT_LOW)  ? " [Lo Volt]" :
                              (st == DISPLAY_VOLT_HIGH) ? " [Hi Volt]" : "";
            snprintf(out_line2, max_len, "VOLTAGE PHASE S (US)%s", tag);
            if (st == DISPLAY_VOLT_LOW) {
                snprintf(out_line3, max_len, "Lo Volt (%lu.%lu V)", (unsigned long)(v / 10), (unsigned long)(v % 10));
            } else if (st == DISPLAY_VOLT_HIGH) {
                snprintf(out_line3, max_len, "Hi Volt (%lu.%lu V)", (unsigned long)(v / 10), (unsigned long)(v % 10));
            } else {
                snprintf(out_line3, max_len, "%lu.%lu V", (unsigned long)(v / 10), (unsigned long)(v % 10));
            }
            break;
        }
        case DISP_PAGE_VOLTAGE_T: {
            uint32_t v = ctx->meas_buffer.voltage_t_dvolts;
            display_voltage_status_t st = display_get_voltage_status(v);
            const char *tag = (st == DISPLAY_VOLT_LOW)  ? " [Lo Volt]" :
                              (st == DISPLAY_VOLT_HIGH) ? " [Hi Volt]" : "";
            snprintf(out_line2, max_len, "VOLTAGE PHASE T (UT)%s", tag);
            if (st == DISPLAY_VOLT_LOW) {
                snprintf(out_line3, max_len, "Lo Volt (%lu.%lu V)", (unsigned long)(v / 10), (unsigned long)(v % 10));
            } else if (st == DISPLAY_VOLT_HIGH) {
                snprintf(out_line3, max_len, "Hi Volt (%lu.%lu V)", (unsigned long)(v / 10), (unsigned long)(v % 10));
            } else {
                snprintf(out_line3, max_len, "%lu.%lu V", (unsigned long)(v / 10), (unsigned long)(v % 10));
            }
            break;
        }
        case DISP_PAGE_CURRENT_R:
            snprintf(out_line2, max_len, "CURRENT PHASE R (IR)");
            snprintf(out_line3, max_len, "%lu.%03lu A", (unsigned long)(ctx->meas_buffer.current_r_mamps / 1000), (unsigned long)(ctx->meas_buffer.current_r_mamps % 1000));
            break;
        case DISP_PAGE_CURRENT_S:
            snprintf(out_line2, max_len, "CURRENT PHASE S (IS)");
            snprintf(out_line3, max_len, "%lu.%03lu A", (unsigned long)(ctx->meas_buffer.current_s_mamps / 1000), (unsigned long)(ctx->meas_buffer.current_s_mamps % 1000));
            break;
        case DISP_PAGE_CURRENT_T:
            snprintf(out_line2, max_len, "CURRENT PHASE T (IT)");
            snprintf(out_line3, max_len, "%lu.%03lu A", (unsigned long)(ctx->meas_buffer.current_t_mamps / 1000), (unsigned long)(ctx->meas_buffer.current_t_mamps % 1000));
            break;
        case DISP_PAGE_CURRENT_N:
            snprintf(out_line2, max_len, "CURRENT NEUTRAL (IN)");
            snprintf(out_line3, max_len, "%lu.%03lu A", (unsigned long)(ctx->meas_buffer.current_n_mamps / 1000), (unsigned long)(ctx->meas_buffer.current_n_mamps % 1000));
            break;
        case DISP_PAGE_ACTIVE_POWER: {
            int32_t pw = ctx->meas_buffer.active_power_w;
            uint32_t abs_pw = (pw < 0) ? (uint32_t)(-pw) : (uint32_t)pw;
            snprintf(out_line2, max_len, "ACTIVE POWER");
            snprintf(out_line3, max_len, "%s%lu.%03lu kW", (pw < 0) ? "-" : "", (unsigned long)(abs_pw / 1000), (unsigned long)(abs_pw % 1000));
            break;
        }
        case DISP_PAGE_REACTIVE_POWER: {
            int32_t pvar = ctx->meas_buffer.reactive_power_var;
            uint32_t abs_pvar = (pvar < 0) ? (uint32_t)(-pvar) : (uint32_t)pvar;
            snprintf(out_line2, max_len, "REACTIVE POWER");
            snprintf(out_line3, max_len, "%s%lu.%03lu kvar", (pvar < 0) ? "-" : "", (unsigned long)(abs_pvar / 1000), (unsigned long)(abs_pvar % 1000));
            break;
        }
        case DISP_PAGE_APPARENT_POWER:
            snprintf(out_line2, max_len, "APPARENT POWER");
            snprintf(out_line3, max_len, "%lu.%03lu kVA", (unsigned long)(ctx->meas_buffer.apparent_power_va / 1000), (unsigned long)(ctx->meas_buffer.apparent_power_va % 1000));
            break;
        case DISP_PAGE_POWER_FACTOR:
            snprintf(out_line2, max_len, "POWER FACTOR");
            snprintf(out_line3, max_len, "%lu.%03lu", (unsigned long)(ctx->meas_buffer.power_factor_ppm / 1000), (unsigned long)(ctx->meas_buffer.power_factor_ppm % 1000));
            break;
        case DISP_PAGE_FREQUENCY:
            snprintf(out_line2, max_len, "GRID FREQUENCY");
            snprintf(out_line3, max_len, "%lu.%02lu Hz", (unsigned long)(ctx->meas_buffer.frequency_mhz / 1000), (unsigned long)((ctx->meas_buffer.frequency_mhz % 1000) / 10));
            break;
        case DISP_PAGE_ACTIVE_ENERGY:
            snprintf(out_line2, max_len, "TOTAL ENERGY");
            snprintf(out_line3, max_len, "%lu.%02lu kWh", (unsigned long)(ctx->meas_buffer.active_energy_wh / 1000), (unsigned long)((ctx->meas_buffer.active_energy_wh % 1000) / 10));
            break;
        case DISP_PAGE_ENERGY_WBP:
            snprintf(out_line2, max_len, "ENERGY WBP (T1)");
            snprintf(out_line3, max_len, "%lu.%03lu kWh", (unsigned long)(ctx->meas_buffer.active_energy_wbp_wh / 1000), (unsigned long)(ctx->meas_buffer.active_energy_wbp_wh % 1000));
            break;
        case DISP_PAGE_ENERGY_LWBP:
            snprintf(out_line2, max_len, "ENERGY LWBP (T2)");
            snprintf(out_line3, max_len, "%lu.%03lu kWh", (unsigned long)(ctx->meas_buffer.active_energy_lwbp_wh / 1000), (unsigned long)(ctx->meas_buffer.active_energy_lwbp_wh % 1000));
            break;
        case DISP_PAGE_ENERGY_LWBP2:
            snprintf(out_line2, max_len, "ENERGY LWBP2 (T3)");
            snprintf(out_line3, max_len, "%lu.%03lu kWh", (unsigned long)(ctx->meas_buffer.active_energy_lwbp2_wh / 1000), (unsigned long)(ctx->meas_buffer.active_energy_lwbp2_wh % 1000));
            break;
        case DISP_PAGE_ENERGY_IMPORT:
            snprintf(out_line2, max_len, "ENERGY IMPORT (EA_IMP)");
            snprintf(out_line3, max_len, "%lu.%03lu kWh", (unsigned long)(ctx->meas_buffer.active_energy_import_wh / 1000), (unsigned long)(ctx->meas_buffer.active_energy_import_wh % 1000));
            break;
        case DISP_PAGE_MAX_DEMAND:
            snprintf(out_line2, max_len, "MAX DEMAND (P_MAX)");
            snprintf(out_line3, max_len, "%lu.%03lu kW",
                     (unsigned long)(ctx->meas_buffer.max_demand_w / 1000),
                     (unsigned long)(ctx->meas_buffer.max_demand_w % 1000));
            break;
        case DISP_PAGE_MD_TIME: {
            uint16_t yr; uint8_t mo, dy, hr, mn, sc;
            display_epoch_to_date(ctx->meas_buffer.max_demand_timestamp, &yr, &mo, &dy, &hr, &mn, &sc);
            snprintf(out_line2, max_len, "MD TIME & DATE (01.06.0)");
            if (ctx->meas_buffer.max_demand_timestamp > 0) {
                snprintf(out_line3, max_len, "%02d/%02d %02d:%02d WIB", dy, mo, hr, mn);
            } else {
                snprintf(out_line3, max_len, "---");
            }
            break;
        }
        case DISP_PAGE_CMD:
            snprintf(out_line2, max_len, "CUMULATIVE MD (CMD)");
            snprintf(out_line3, max_len, "%lu.%03lu kW",
                     (unsigned long)(ctx->meas_buffer.cumulative_max_demand_w / 1000),
                     (unsigned long)(ctx->meas_buffer.cumulative_max_demand_w % 1000));
            break;
        case DISP_PAGE_MD_RESET_COUNT:
            snprintf(out_line2, max_len, "MD RESET COUNT");
            snprintf(out_line3, max_len, "%04lu KALI",
                     (unsigned long)ctx->meas_buffer.max_demand_reset_count);
            break;
        case DISP_PAGE_REACTIVE_ENERGY:
            snprintf(out_line2, max_len, "REACTIVE ENERGY (ER_BIL)");
            snprintf(out_line3, max_len, "%lu.%03lu kvarh",
                     (unsigned long)(ctx->meas_buffer.reactive_energy_varh / 1000),
                     (unsigned long)(ctx->meas_buffer.reactive_energy_varh % 1000));
            break;
        case DISP_PAGE_TIME: {
            uint16_t yr; uint8_t mo, dy, hr, mn, sc;
            display_epoch_to_date(ctx->current_time_epoch, &yr, &mo, &dy, &hr, &mn, &sc);
            snprintf(out_line2, max_len, "TIME (01.00)");
            snprintf(out_line3, max_len, "%02d:%02d:%02d WIB", hr, mn, sc);
            break;
        }
        case DISP_PAGE_DATE: {
            uint16_t yr; uint8_t mo, dy, hr, mn, sc;
            display_epoch_to_date(ctx->current_time_epoch, &yr, &mo, &dy, &hr, &mn, &sc);
            snprintf(out_line2, max_len, "DATE (00.09.02)");
            snprintf(out_line3, max_len, "%02d-%02d-%04d", dy, mo, yr);
            break;
        }
        case DISP_PAGE_TEST_DISPLAY:
            snprintf(out_line2, max_len, "LCD TEST (DISPLAY)");
            snprintf(out_line3, max_len, "88888888");
            break;
        case DISP_PAGE_HISTORICAL_BILLING:
            snprintf(out_line2, max_len, "BILLING STAND M-1");
            snprintf(out_line3, max_len, "%lu.%02lu kWh",
                     (unsigned long)(ctx->meas_buffer.billing_last_month_wh / 1000),
                     (unsigned long)((ctx->meas_buffer.billing_last_month_wh % 1000) / 10));
            break;
        case DISP_PAGE_POWER_LIMITER: {
            uint32_t plim = (ctx->meas_buffer.power_limit_w > 0) ? ctx->meas_buffer.power_limit_w : 11000U;
            snprintf(out_line2, max_len, "POWER LIMITER (P_LIM)");
            snprintf(out_line3, max_len, "%lu.%03lu kW", (unsigned long)(plim / 1000), (unsigned long)(plim % 1000));
            break;
        }
        case DISP_PAGE_METER_ID:
            snprintf(out_line2, max_len, "METER ID (METERID)");
            snprintf(out_line3, max_len, "%s", ctx->customer_id);
            break;
        case DISP_PAGE_ACTIVE_POWER_IMP: {
            int32_t p = ctx->meas_buffer.active_power_imp_w;
            uint32_t abs_p = (p < 0) ? (uint32_t)(-p) : (uint32_t)p;
            snprintf(out_line2, max_len, "ACTIVE POWER IMP (P_IMP)");
            snprintf(out_line3, max_len, "%lu.%03lu kW", (unsigned long)(abs_p / 1000), (unsigned long)(abs_p % 1000));
            break;
        }
        case DISP_PAGE_REACTIVE_POWER_IMP: {
            int32_t q = ctx->meas_buffer.reactive_power_imp_var;
            uint32_t abs_q = (q < 0) ? (uint32_t)(-q) : (uint32_t)q;
            snprintf(out_line2, max_len, "REACTIVE POWER IMP (Q_IMP)");
            snprintf(out_line3, max_len, "%lu.%03lu kvar", (unsigned long)(abs_q / 1000), (unsigned long)(abs_q % 1000));
            break;
        }
        case DISP_PAGE_ENERGY_IMP_WBP:
            snprintf(out_line2, max_len, "ENERGY IMP WBP (EA_IMP1)");
            snprintf(out_line3, max_len, "%lu.%03lu kWh",
                     (unsigned long)(ctx->meas_buffer.active_energy_imp_wbp_wh / 1000),
                     (unsigned long)(ctx->meas_buffer.active_energy_imp_wbp_wh % 1000));
            break;
        case DISP_PAGE_ENERGY_IMP_LWBP1:
            snprintf(out_line2, max_len, "ENERGY IMP LWBP1 (EA_IMP2)");
            snprintf(out_line3, max_len, "%lu.%03lu kWh",
                     (unsigned long)(ctx->meas_buffer.active_energy_imp_lwbp1_wh / 1000),
                     (unsigned long)(ctx->meas_buffer.active_energy_imp_lwbp1_wh % 1000));
            break;
        case DISP_PAGE_ENERGY_IMP_LWBP2:
            snprintf(out_line2, max_len, "ENERGY IMP LWBP2 (EA_IMP3)");
            snprintf(out_line3, max_len, "%lu.%03lu kWh",
                     (unsigned long)(ctx->meas_buffer.active_energy_imp_lwbp2_wh / 1000),
                     (unsigned long)(ctx->meas_buffer.active_energy_imp_lwbp2_wh % 1000));
            break;
        case DISP_PAGE_REACTIVE_ENERGY_EXP:
            snprintf(out_line2, max_len, "REACTIVE ENERGY EXP (ER_EXP)");
            snprintf(out_line3, max_len, "%lu.%03lu kvarh",
                     (unsigned long)(ctx->meas_buffer.reactive_energy_exp_varh / 1000),
                     (unsigned long)(ctx->meas_buffer.reactive_energy_exp_varh % 1000));
            break;
        case DISP_PAGE_REACTIVE_ENERGY_IMP:
            snprintf(out_line2, max_len, "REACTIVE ENERGY IMP (ER_IMP)");
            snprintf(out_line3, max_len, "%lu.%03lu kvarh",
                     (unsigned long)(ctx->meas_buffer.reactive_energy_imp_varh / 1000),
                     (unsigned long)(ctx->meas_buffer.reactive_energy_imp_varh % 1000));
            break;
        case DISP_PAGE_ANGLE_UR:
            snprintf(out_line2, max_len, "VOLTAGE ANGLE R (Sdt_UR)");
            snprintf(out_line3, max_len, "%u DEG", (unsigned int)ctx->meas_buffer.angle_ur_deg);
            break;
        case DISP_PAGE_ANGLE_US:
            snprintf(out_line2, max_len, "VOLTAGE ANGLE S (Sdt_US)");
            snprintf(out_line3, max_len, "%u DEG", (unsigned int)ctx->meas_buffer.angle_us_deg);
            break;
        case DISP_PAGE_ANGLE_UT:
            snprintf(out_line2, max_len, "VOLTAGE ANGLE T (Sdt_UT)");
            snprintf(out_line3, max_len, "%u DEG", (unsigned int)ctx->meas_buffer.angle_ut_deg);
            break;
        case DISP_PAGE_ANGLE_IR:
            snprintf(out_line2, max_len, "CURRENT ANGLE R (Sdt_IR)");
            snprintf(out_line3, max_len, "%u DEG", (unsigned int)ctx->meas_buffer.angle_ir_deg);
            break;
        case DISP_PAGE_ANGLE_IS:
            snprintf(out_line2, max_len, "CURRENT ANGLE S (Sdt_IS)");
            snprintf(out_line3, max_len, "%u DEG", (unsigned int)ctx->meas_buffer.angle_is_deg);
            break;
        case DISP_PAGE_ANGLE_IT:
            snprintf(out_line2, max_len, "CURRENT ANGLE T (Sdt_IT)");
            snprintf(out_line3, max_len, "%u DEG", (unsigned int)ctx->meas_buffer.angle_it_deg);
            break;
        case DISP_PAGE_MAX_DEMAND_S:
            snprintf(out_line2, max_len, "MAX DEMAND SEMU (S_MAX)");
            snprintf(out_line3, max_len, "%lu.%03lu kVA",
                     (unsigned long)(ctx->meas_buffer.max_demand_s_va / 1000),
                     (unsigned long)(ctx->meas_buffer.max_demand_s_va % 1000));
            break;
        case DISP_PAGE_MD_TIME_S: {
            uint16_t yr; uint8_t mo, dy, hr, mn, sc;
            display_epoch_to_date(ctx->meas_buffer.max_demand_s_timestamp, &yr, &mo, &dy, &hr, &mn, &sc);
            snprintf(out_line2, max_len, "TIME S_MAX (TS_MAX)");
            if (ctx->meas_buffer.max_demand_s_timestamp > 0) {
                snprintf(out_line3, max_len, "%02d/%02d %02d:%02d WIB", dy, mo, hr, mn);
            } else {
                snprintf(out_line3, max_len, "---");
            }
            break;
        }
        case DISP_PAGE_EOB:
            snprintf(out_line2, max_len, "END OF BILLING (EoB)");
            snprintf(out_line3, max_len, "%02u-%02u (DD-HH)",
                     (unsigned int)ctx->meas_buffer.eob_day,
                     (unsigned int)ctx->meas_buffer.eob_hour);
            break;
        case DISP_PAGE_PLN_UNIT:
            snprintf(out_line2, max_len, "UNIT PLN (PLN)");
            snprintf(out_line3, max_len, "%s",
                     (ctx->meas_buffer.pln_unit_code[0] != '\0') ? ctx->meas_buffer.pln_unit_code : "PLN UID");
            break;
        case DISP_PAGE_CHECKSUM: {
            uint32_t csum = (ctx->meas_buffer.firmware_checksum > 0) ? ctx->meas_buffer.firmware_checksum : 0xA5B6U;
            snprintf(out_line2, max_len, "FIRMWARE CHECKSUM");
            snprintf(out_line3, max_len, "CSUM %04lX", (unsigned long)csum);
            break;
        }
        case DISP_PAGE_TAMPER_ALARM: {
            char spln_text[16] = "RUSAK";
            char spln_code[10] = "-";
            display_get_combined_error_info(ctx->active_tamper_mask, ctx->active_alarm_mask, ctx->current_time_epoch,
                                            spln_text, sizeof(spln_text), spln_code, sizeof(spln_code));
            snprintf(out_line2, max_len, "STATUS SABOTASE");
            if (spln_code[0] != '-' && spln_code[0] != '\0') {
                snprintf(out_line3, max_len, "%s %s", spln_text, spln_code);
            } else {
                snprintf(out_line3, max_len, "%s", spln_text);
            }
            break;
        }
        case DISP_PAGE_RELAY_READY_ON:
            snprintf(out_line2, max_len, "RELAY STATUS (00.00)");
            snprintf(out_line3, max_len, "READY ON");
            break;
        default:
            snprintf(out_line2, max_len, "DISPLAY PAGE %d", ctx->current_page);
            snprintf(out_line3, max_len, "---");
            break;
    }
}

void display_render_spln_frame(const display_context_t *ctx, display_spln_frame_t *frame) {
    if (ctx == NULL || frame == NULL) return;
    memset(frame, 0, sizeof(display_spln_frame_t));
    frame->page_id = ctx->current_page;

    const char *obis_code = "00.00";
    frame->is_large_font = true;

    /* Evaluasi Anomali Tegangan Pasok (Lo Volt / Hi Volt - SPLN Butir 10.3 Tabel 9) */
    bool has_voltage_anomaly = (display_get_voltage_status(ctx->meas_buffer.voltage_r_dvolts) != DISPLAY_VOLT_NORMAL) ||
                               (display_get_voltage_status(ctx->meas_buffer.voltage_s_dvolts) != DISPLAY_VOLT_NORMAL) ||
                               (display_get_voltage_status(ctx->meas_buffer.voltage_t_dvolts) != DISPLAY_VOLT_NORMAL);

    frame->is_alarm_active = (ctx->alarm_icon_active || ctx->asymmetry_warning || has_voltage_anomaly);

    if (frame->is_alarm_active) {
        snprintf(frame->alarm_code, sizeof(frame->alarm_code), "!ALM");
    } else {
        snprintf(frame->alarm_code, sizeof(frame->alarm_code), "OK");
    }

    switch (ctx->current_page) {
        case DISP_PAGE_IDPEL:
            obis_code = "96.01";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "IDPEL");
            snprintf(frame->main_value, sizeof(frame->main_value), "%s", ctx->customer_id);
            frame->unit[0] = '\0';
            frame->is_large_font = false; /* 12 digit IDPEL menggunakan Font 7x10 */
            break;
        case DISP_PAGE_VOLTAGE_R: {
            obis_code = "32.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "UR");
            uint32_t v = ctx->meas_buffer.voltage_r_dvolts;
            display_voltage_status_t st = display_get_voltage_status(v);
            if (st == DISPLAY_VOLT_LOW && ((ctx->timer_accumulator_ms / 1500U) % 2 == 0)) {
                snprintf(frame->main_value, sizeof(frame->main_value), "Lo Volt");
                frame->is_large_font = false;
            } else if (st == DISPLAY_VOLT_HIGH && ((ctx->timer_accumulator_ms / 1500U) % 2 == 0)) {
                snprintf(frame->main_value, sizeof(frame->main_value), "Hi Volt");
                frame->is_large_font = false;
            } else {
                snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%lu",
                         (unsigned long)(v / 10), (unsigned long)(v % 10));
                frame->is_large_font = true;
            }
            snprintf(frame->unit, sizeof(frame->unit), "V");
            break;
        }
        case DISP_PAGE_VOLTAGE_S: {
            obis_code = "52.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "US");
            uint32_t v = ctx->meas_buffer.voltage_s_dvolts;
            display_voltage_status_t st = display_get_voltage_status(v);
            if (st == DISPLAY_VOLT_LOW && ((ctx->timer_accumulator_ms / 1500U) % 2 == 0)) {
                snprintf(frame->main_value, sizeof(frame->main_value), "Lo Volt");
                frame->is_large_font = false;
            } else if (st == DISPLAY_VOLT_HIGH && ((ctx->timer_accumulator_ms / 1500U) % 2 == 0)) {
                snprintf(frame->main_value, sizeof(frame->main_value), "Hi Volt");
                frame->is_large_font = false;
            } else {
                snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%lu",
                         (unsigned long)(v / 10), (unsigned long)(v % 10));
                frame->is_large_font = true;
            }
            snprintf(frame->unit, sizeof(frame->unit), "V");
            break;
        }
        case DISP_PAGE_VOLTAGE_T: {
            obis_code = "72.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "UT");
            uint32_t v = ctx->meas_buffer.voltage_t_dvolts;
            display_voltage_status_t st = display_get_voltage_status(v);
            if (st == DISPLAY_VOLT_LOW && ((ctx->timer_accumulator_ms / 1500U) % 2 == 0)) {
                snprintf(frame->main_value, sizeof(frame->main_value), "Lo Volt");
                frame->is_large_font = false;
            } else if (st == DISPLAY_VOLT_HIGH && ((ctx->timer_accumulator_ms / 1500U) % 2 == 0)) {
                snprintf(frame->main_value, sizeof(frame->main_value), "Hi Volt");
                frame->is_large_font = false;
            } else {
                snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%lu",
                         (unsigned long)(v / 10), (unsigned long)(v % 10));
                frame->is_large_font = true;
            }
            snprintf(frame->unit, sizeof(frame->unit), "V");
            break;
        }
        case DISP_PAGE_CURRENT_R:
            obis_code = "31.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "IR");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.current_r_mamps / 1000),
                     (unsigned long)(ctx->meas_buffer.current_r_mamps % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "A");
            break;
        case DISP_PAGE_CURRENT_S:
            obis_code = "51.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "IS");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.current_s_mamps / 1000),
                     (unsigned long)(ctx->meas_buffer.current_s_mamps % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "A");
            break;
        case DISP_PAGE_CURRENT_T:
            obis_code = "71.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "IT");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.current_t_mamps / 1000),
                     (unsigned long)(ctx->meas_buffer.current_t_mamps % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "A");
            break;
        case DISP_PAGE_CURRENT_N:
            obis_code = "91.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "IN");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.current_n_mamps / 1000),
                     (unsigned long)(ctx->meas_buffer.current_n_mamps % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "A");
            break;
        case DISP_PAGE_ACTIVE_POWER: {
            obis_code = "01.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "09");
            int32_t pw = ctx->meas_buffer.active_power_w;
            uint32_t abs_pw = (pw < 0) ? (uint32_t)(-pw) : (uint32_t)pw;
            snprintf(frame->main_value, sizeof(frame->main_value), "%s%lu.%03lu",
                     (pw < 0) ? "-" : "",
                     (unsigned long)(abs_pw / 1000),
                     (unsigned long)(abs_pw % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kW");
            break;
        }
        case DISP_PAGE_REACTIVE_POWER: {
            obis_code = "03.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "10");
            int32_t pvar = ctx->meas_buffer.reactive_power_var;
            uint32_t abs_pvar = (pvar < 0) ? (uint32_t)(-pvar) : (uint32_t)pvar;
            snprintf(frame->main_value, sizeof(frame->main_value), "%s%lu.%03lu",
                     (pvar < 0) ? "-" : "",
                     (unsigned long)(abs_pvar / 1000),
                     (unsigned long)(abs_pvar % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kvar");
            break;
        }
        case DISP_PAGE_APPARENT_POWER:
            obis_code = "09.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "11");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.apparent_power_va / 1000),
                     (unsigned long)(ctx->meas_buffer.apparent_power_va % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kVA");
            break;
        case DISP_PAGE_POWER_FACTOR:
            obis_code = "13.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "12");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.power_factor_ppm / 1000),
                     (unsigned long)(ctx->meas_buffer.power_factor_ppm % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "PF");
            break;
        case DISP_PAGE_FREQUENCY:
            obis_code = "14.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "13");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%02lu",
                     (unsigned long)(ctx->meas_buffer.frequency_mhz / 1000),
                     (unsigned long)((ctx->meas_buffer.frequency_mhz % 1000) / 10));
            snprintf(frame->unit, sizeof(frame->unit), "Hz");
            break;
        case DISP_PAGE_ACTIVE_ENERGY:
            obis_code = "01.08";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "EA_EXP");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.active_energy_wh / 1000),
                     (unsigned long)(ctx->meas_buffer.active_energy_wh % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kWh");
            break;
        case DISP_PAGE_ENERGY_WBP:
            obis_code = "01.08.1";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "EA_EXP1");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.active_energy_wbp_wh / 1000),
                     (unsigned long)(ctx->meas_buffer.active_energy_wbp_wh % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kWh");
            break;
        case DISP_PAGE_ENERGY_LWBP:
            obis_code = "01.08.2";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "EA_EXP2");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.active_energy_lwbp_wh / 1000),
                     (unsigned long)(ctx->meas_buffer.active_energy_lwbp_wh % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kWh");
            break;
        case DISP_PAGE_ENERGY_LWBP2:
            obis_code = "01.08.3";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "EA_EXP3");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.active_energy_lwbp2_wh / 1000),
                     (unsigned long)(ctx->meas_buffer.active_energy_lwbp2_wh % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kWh");
            break;
        case DISP_PAGE_ENERGY_IMPORT:
            obis_code = "02.08";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "EA_IMP");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.active_energy_import_wh / 1000),
                     (unsigned long)(ctx->meas_buffer.active_energy_import_wh % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kWh");
            break;
        case DISP_PAGE_MAX_DEMAND:
            obis_code = "01.06";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "17");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.max_demand_w / 1000),
                     (unsigned long)(ctx->meas_buffer.max_demand_w % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kW");
            break;
        case DISP_PAGE_MD_TIME: {
            obis_code = "01.06.0";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "18");
            frame->is_large_font = false;
            if (ctx->meas_buffer.max_demand_timestamp > 0) {
                uint16_t yr; uint8_t mo, dy, hr, mn, sc;
                display_epoch_to_date(ctx->meas_buffer.max_demand_timestamp, &yr, &mo, &dy, &hr, &mn, &sc);
                snprintf(frame->main_value, sizeof(frame->main_value), "%02d/%02d %02d:%02d", dy, mo, hr, mn);
            } else {
                snprintf(frame->main_value, sizeof(frame->main_value), "-");
            }
            frame->unit[0] = '\0';
            break;
        }
        case DISP_PAGE_CMD:
            obis_code = "01.02";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "19");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.cumulative_max_demand_w / 1000),
                     (unsigned long)(ctx->meas_buffer.cumulative_max_demand_w % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kW");
            break;
        case DISP_PAGE_MD_RESET_COUNT:
            obis_code = "01.01";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "20");
            snprintf(frame->main_value, sizeof(frame->main_value), "%04lu",
                     (unsigned long)ctx->meas_buffer.max_demand_reset_count);
            snprintf(frame->unit, sizeof(frame->unit), "RST");
            frame->is_large_font = false;
            break;
        case DISP_PAGE_REACTIVE_ENERGY:
            obis_code = "03.08";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "ER_BIL");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.reactive_energy_varh / 1000),
                     (unsigned long)(ctx->meas_buffer.reactive_energy_varh % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kVARh");
            break;
        case DISP_PAGE_TIME: {
            obis_code = "01.00";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "22");
            frame->is_large_font = false;
            uint16_t yr; uint8_t mo, dy, hr, mn, sc;
            display_epoch_to_date(ctx->current_time_epoch, &yr, &mo, &dy, &hr, &mn, &sc);
            snprintf(frame->main_value, sizeof(frame->main_value), "%02d:%02d:%02d", hr, mn, sc);
            snprintf(frame->unit, sizeof(frame->unit), "WIB");
            break;
        }
        case DISP_PAGE_DATE: {
            obis_code = "00.09.02";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "23");
            frame->is_large_font = false;
            uint16_t yr; uint8_t mo, dy, hr, mn, sc;
            display_epoch_to_date(ctx->current_time_epoch, &yr, &mo, &dy, &hr, &mn, &sc);
            snprintf(frame->main_value, sizeof(frame->main_value), "%02d-%02d-%04d", dy, mo, yr);
            snprintf(frame->unit, sizeof(frame->unit), "WIB");
            break;
        }
        case DISP_PAGE_TEST_DISPLAY:
            obis_code = "88.88";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "00");
            snprintf(frame->main_value, sizeof(frame->main_value), "88888888");
            snprintf(frame->unit, sizeof(frame->unit), "8888");
            frame->is_large_font = true;
            break;
        case DISP_PAGE_HISTORICAL_BILLING:
            obis_code = "01.08*01";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "24");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%02lu",
                     (unsigned long)(ctx->meas_buffer.billing_last_month_wh / 1000),
                     (unsigned long)((ctx->meas_buffer.billing_last_month_wh % 1000) / 10));
            snprintf(frame->unit, sizeof(frame->unit), "kWh");
            break;
        case DISP_PAGE_POWER_LIMITER: {
            obis_code = "00.01";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "34");
            uint32_t plim = (ctx->meas_buffer.power_limit_w > 0) ? ctx->meas_buffer.power_limit_w : 11000U;
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(plim / 1000), (unsigned long)(plim % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kW");
            break;
        }
        case DISP_PAGE_METER_ID:
            obis_code = "96.02";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "02");
            snprintf(frame->main_value, sizeof(frame->main_value), "%s", ctx->customer_id);
            frame->unit[0] = '\0';
            frame->is_large_font = false;
            break;
        case DISP_PAGE_ACTIVE_POWER_IMP: {
            obis_code = "02.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "27");
            int32_t p = ctx->meas_buffer.active_power_imp_w;
            uint32_t abs_p = (p < 0) ? (uint32_t)(-p) : (uint32_t)p;
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(abs_p / 1000), (unsigned long)(abs_p % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kW");
            break;
        }
        case DISP_PAGE_REACTIVE_POWER_IMP: {
            obis_code = "04.07";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "28");
            int32_t q = ctx->meas_buffer.reactive_power_imp_var;
            uint32_t abs_q = (q < 0) ? (uint32_t)(-q) : (uint32_t)q;
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(abs_q / 1000), (unsigned long)(abs_q % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kvar");
            break;
        }
        case DISP_PAGE_ENERGY_IMP_WBP:
            obis_code = "02.08.1";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "29");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.active_energy_imp_wbp_wh / 1000),
                     (unsigned long)(ctx->meas_buffer.active_energy_imp_wbp_wh % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kWh");
            break;
        case DISP_PAGE_ENERGY_IMP_LWBP1:
            obis_code = "02.08.2";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "30");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.active_energy_imp_lwbp1_wh / 1000),
                     (unsigned long)(ctx->meas_buffer.active_energy_imp_lwbp1_wh % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kWh");
            break;
        case DISP_PAGE_ENERGY_IMP_LWBP2:
            obis_code = "02.08.3";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "31");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.active_energy_imp_lwbp2_wh / 1000),
                     (unsigned long)(ctx->meas_buffer.active_energy_imp_lwbp2_wh % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kWh");
            break;
        case DISP_PAGE_REACTIVE_ENERGY_EXP:
            obis_code = "03.08.0";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "32");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.reactive_energy_exp_varh / 1000),
                     (unsigned long)(ctx->meas_buffer.reactive_energy_exp_varh % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kvarh");
            break;
        case DISP_PAGE_REACTIVE_ENERGY_IMP:
            obis_code = "04.08.0";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "33");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.reactive_energy_imp_varh / 1000),
                     (unsigned long)(ctx->meas_buffer.reactive_energy_imp_varh % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kvarh");
            break;
        case DISP_PAGE_ANGLE_UR:
            obis_code = "14.81.0";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "35");
            snprintf(frame->main_value, sizeof(frame->main_value), "%u", (unsigned int)ctx->meas_buffer.angle_ur_deg);
            snprintf(frame->unit, sizeof(frame->unit), "DEG");
            break;
        case DISP_PAGE_ANGLE_US:
            obis_code = "14.81.1";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "36");
            snprintf(frame->main_value, sizeof(frame->main_value), "%u", (unsigned int)ctx->meas_buffer.angle_us_deg);
            snprintf(frame->unit, sizeof(frame->unit), "DEG");
            break;
        case DISP_PAGE_ANGLE_UT:
            obis_code = "14.81.2";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "37");
            snprintf(frame->main_value, sizeof(frame->main_value), "%u", (unsigned int)ctx->meas_buffer.angle_ut_deg);
            snprintf(frame->unit, sizeof(frame->unit), "DEG");
            break;
        case DISP_PAGE_ANGLE_IR:
            obis_code = "11.81.0";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "39");
            snprintf(frame->main_value, sizeof(frame->main_value), "%u", (unsigned int)ctx->meas_buffer.angle_ir_deg);
            snprintf(frame->unit, sizeof(frame->unit), "DEG");
            break;
        case DISP_PAGE_ANGLE_IS:
            obis_code = "11.81.1";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "40");
            snprintf(frame->main_value, sizeof(frame->main_value), "%u", (unsigned int)ctx->meas_buffer.angle_is_deg);
            snprintf(frame->unit, sizeof(frame->unit), "DEG");
            break;
        case DISP_PAGE_ANGLE_IT:
            obis_code = "11.81.2";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "41");
            snprintf(frame->main_value, sizeof(frame->main_value), "%u", (unsigned int)ctx->meas_buffer.angle_it_deg);
            snprintf(frame->unit, sizeof(frame->unit), "DEG");
            break;
        case DISP_PAGE_MAX_DEMAND_S:
            obis_code = "09.06.0";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "42");
            snprintf(frame->main_value, sizeof(frame->main_value), "%lu.%03lu",
                     (unsigned long)(ctx->meas_buffer.max_demand_s_va / 1000),
                     (unsigned long)(ctx->meas_buffer.max_demand_s_va % 1000));
            snprintf(frame->unit, sizeof(frame->unit), "kVA");
            break;
        case DISP_PAGE_MD_TIME_S: {
            obis_code = "09.06.0*0";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "43");
            frame->is_large_font = false;
            if (ctx->meas_buffer.max_demand_s_timestamp > 0) {
                uint16_t yr; uint8_t mo, dy, hr, mn, sc;
                display_epoch_to_date(ctx->meas_buffer.max_demand_s_timestamp, &yr, &mo, &dy, &hr, &mn, &sc);
                snprintf(frame->main_value, sizeof(frame->main_value), "%02d/%02d %02d:%02d", dy, mo, hr, mn);
            } else {
                snprintf(frame->main_value, sizeof(frame->main_value), "-");
            }
            frame->unit[0] = '\0';
            break;
        }
        case DISP_PAGE_EOB:
            obis_code = "00.09.01";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "44");
            frame->is_large_font = false;
            snprintf(frame->main_value, sizeof(frame->main_value), "%02u-%02u",
                     (unsigned int)ctx->meas_buffer.eob_day,
                     (unsigned int)ctx->meas_buffer.eob_hour);
            snprintf(frame->unit, sizeof(frame->unit), "EoB");
            break;
        case DISP_PAGE_PLN_UNIT:
            obis_code = "96.01.0";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "45");
            frame->is_large_font = false;
            snprintf(frame->main_value, sizeof(frame->main_value), "%s",
                     (ctx->meas_buffer.pln_unit_code[0] != '\0') ? ctx->meas_buffer.pln_unit_code : "PLN UID");
            snprintf(frame->unit, sizeof(frame->unit), "PLN");
            break;
        case DISP_PAGE_CHECKSUM: {
            obis_code = "96.20";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "38");
            uint32_t csum = (ctx->meas_buffer.firmware_checksum > 0) ? ctx->meas_buffer.firmware_checksum : 0xA5B6U;
            snprintf(frame->main_value, sizeof(frame->main_value), "%04lX", (unsigned long)csum);
            snprintf(frame->unit, sizeof(frame->unit), "CSUM");
            break;
        }
        case DISP_PAGE_TAMPER_ALARM: {
            obis_code = "96.50";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "AL");
            frame->is_large_font = false;

            char spln_text[16] = "RUSAK";
            char spln_code[10] = "-";

            display_get_combined_error_info(ctx->active_tamper_mask, ctx->active_alarm_mask, ctx->current_time_epoch,
                                            spln_text, sizeof(spln_text), spln_code, sizeof(spln_code));

            snprintf(frame->main_value, sizeof(frame->main_value), "%s", spln_text);
            snprintf(frame->unit, sizeof(frame->unit), "%s", spln_code);
            break;
        }
        case DISP_PAGE_RELAY_READY_ON: {
            obis_code = "00.00";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "on");
            frame->is_large_font = false;
            snprintf(frame->main_value, sizeof(frame->main_value), "READY ON");
            snprintf(frame->unit, sizeof(frame->unit), "RECONNECT");
            break;
        }
        default:
            obis_code = "00.00";
            snprintf(frame->scroll_index_zz, sizeof(frame->scroll_index_zz), "--");
            snprintf(frame->main_value, sizeof(frame->main_value), "---");
            frame->unit[0] = '\0';
            break;
    }

    /* Simpan obis_code dan reg_info untuk penataan pixel terpisah */
    frame->show_reg_alt = ((ctx->timer_accumulator_ms / 2500U) % 2 == 0);

    /* Evaluasi Simbol Baterai (SPLN Butir 6.11: Berkedip saat <= 2.7V / ERR03) */
    frame->is_battery_low = (ctx->active_alarm_mask & SPLN_ALARM_LOW_BATTERY) != 0;
    frame->is_battery_blink_on = ((ctx->timer_accumulator_ms / 500) % 2 == 0);

    /* Evaluasi Arah Arus / Reverse Power */
    frame->is_reverse_power = ((ctx->active_tamper_mask & TAMPER_VECTOR_REVERSE_POWER) != 0) ||
                              (ctx->meas_buffer.active_power_w < 0);

    /* Evaluasi Keberadaan Fasa Tegangan (UR, US, UT >= 110.0V) */
    frame->voltage_phases = 0x07;
    if (ctx->meas_buffer.voltage_r_dvolts < 1100U) frame->voltage_phases &= ~0x01;
    if (ctx->meas_buffer.voltage_s_dvolts < 1100U) frame->voltage_phases &= ~0x02;
    if (ctx->meas_buffer.voltage_t_dvolts < 1100U) frame->voltage_phases &= ~0x04;

    /* Evaluasi Keberadaan Fasa Arus (IR, IS, IT >= 20mA) */
    frame->current_phases = 0x07;
    if (ctx->meas_buffer.current_r_mamps < 20U) frame->current_phases &= ~0x01;
    if (ctx->meas_buffer.current_s_mamps < 20U) frame->current_phases &= ~0x02;
    if (ctx->meas_buffer.current_t_mamps < 20U) frame->current_phases &= ~0x04;

    /* Evaluasi Kuadran Daya (P vs Q) sesuai SPLN D3.006:2021 Butir 6.11 & Lampiran A */
    int32_t p = ctx->meas_buffer.active_power_w;
    int32_t q = ctx->meas_buffer.reactive_power_var;
    if (ctx->meas_buffer.current_r_mamps < 20U &&
        ctx->meas_buffer.current_s_mamps < 20U &&
        ctx->meas_buffer.current_t_mamps < 20U &&
        p == 0 && q == 0) {
        frame->power_quadrant = 0; /* Kondisi idle / tanpa beban: lingkaran silang kosong */
    } else if (p >= 0 && q >= 0) {
        frame->power_quadrant = 1; /* Kuadran 1 (+P, +Q): Kanan Atas (Beban Induktif) */
    } else if (p < 0 && q >= 0) {
        frame->power_quadrant = 2; /* Kuadran 2 (-P, +Q): Kiri Atas (Ekspor PLTS Lagging) */
    } else if (p < 0 && q < 0) {
        frame->power_quadrant = 3; /* Kuadran 3 (-P, -Q): Kiri Bawah (Ekspor PLTS Leading) */
    } else {
        frame->power_quadrant = 4; /* Kuadran 4 (+P, -Q): Kanan Bawah (Beban Kapasitif) */
    }

    /* Status Relai, Tamper, dan Modem untuk Grafis Layar OLED */
    frame->is_relay_ready_on = ctx->relay_ready_on;
    frame->is_relay_closed = !ctx->relay_ready_on &&
                             ((ctx->active_tamper_mask & TAMPER_MASK_RELAY_TRIP) == 0U) &&
                             ((ctx->active_alarm_mask & SPLN_ALARM_MASK_RELAY_TRIP) == 0U);
    frame->is_tamper_active = (ctx->active_tamper_mask != 0);
    frame->is_modem_active = ctx->is_modem_active;

    /* Evaluasi Zona Tarif ToU untuk Tampilan OLED */
    uint32_t hour_of_day = (ctx->current_time_epoch / 3600U) % 24U;
    if (hour_of_day >= 17U && hour_of_day < 22U) {
        frame->active_tariff_zone = 0; /* WBP */
    } else if (hour_of_day >= 22U || hour_of_day < 6U) {
        frame->active_tariff_zone = 1; /* LWBP1 */
    } else {
        frame->active_tariff_zone = 2; /* LWBP2 */
    }

    char bat_sym[4] = "[B]";
    if (frame->is_battery_low && !frame->is_battery_blink_on) {
        strcpy(bat_sym, "   ");
    }

    /* Indikasi Fasa Tegangan UR, US, UT (SPLN D3.006: 2021 Butir 6.11 Gambar 6) */
    char v_sym[5] = "U---";
    v_sym[1] = (frame->voltage_phases & 0x01) ? 'R' : '-';
    v_sym[2] = (frame->voltage_phases & 0x02) ? 'S' : '-';
    v_sym[3] = (frame->voltage_phases & 0x04) ? 'T' : '-';

    /* Indikasi Fasa Arus IR, IS, IT / Reverse Power (SPLN Butir 6.11 Gambar 6) */
    char i_sym[5] = "I---";
    if (frame->is_reverse_power) {
        strcpy(i_sym, " REV");
    } else {
        i_sym[1] = (frame->current_phases & 0x01) ? 'R' : '-';
        i_sym[2] = (frame->current_phases & 0x02) ? 'S' : '-';
        i_sym[3] = (frame->current_phases & 0x04) ? 'T' : '-';
    }

    /* Format Baris 1: Simbol & Kode OBIS gabungan */
    snprintf(frame->header_symbols, sizeof(frame->header_symbols),
             "%s %s %s %s %s",
             frame->alarm_code,
             bat_sym,
             v_sym,
             i_sym,
             obis_code);
}