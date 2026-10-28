/**
 * @file console_io.c
 * @brief Implementasi Modul Abstraksi Konsol Universal & Interactive Serial CLI Parser
 *        Menyediakan Syscall Newlib _write() & Antarmuka Perintah Diagnostik Meter
 */

#include "console_io.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* Pointer callback driver pengiriman hardware */
static console_tx_fn_t s_console_tx_driver = NULL;

/* Status proteksi mute konsol (misal saat transaksi biner DLMS berlangsung) */
static volatile bool s_console_is_muted = false;

/* Hooks kontrol dan telemetri meter */
static console_meter_hooks_t s_hooks;
static bool s_hooks_registered = false;

/* Buffer penerimaan baris perintah CLI interaktif */
static char   s_rx_line_buf[CONSOLE_CMD_MAX_LEN];
static size_t s_rx_line_idx = 0;

/* Prototipe handler perintah CLI */
static int cmd_help(int argc, char *argv[]);
static int cmd_status(int argc, char *argv[]);
static int cmd_meas(int argc, char *argv[]);
static int cmd_energy(int argc, char *argv[]);
static int cmd_relay(int argc, char *argv[]);
static int cmd_tamper(int argc, char *argv[]);
static int cmd_alarm(int argc, char *argv[]);
static int cmd_log(int argc, char *argv[]);
static int cmd_demand(int argc, char *argv[]);
static int cmd_mute(int argc, char *argv[]);
static int cmd_echo(int argc, char *argv[]);
static int cmd_clear(int argc, char *argv[]);
static int cmd_time(int argc, char *argv[]);
static int cmd_set_time(int argc, char *argv[]);
static int cmd_acfail(int argc, char *argv[]);
static int cmd_button(int argc, char *argv[]);
#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
static int cmd_load(int argc, char *argv[]);
static int cmd_volt(int argc, char *argv[]);
static int cmd_curr(int argc, char *argv[]);
static int cmd_synth(int argc, char *argv[]);
#endif

/* Tabel pendaftaran perintah CLI */
static const console_cmd_entry_t s_cmd_table[] = {
    {"help",     "Menampilkan daftar perintah dan sintaks", cmd_help},
    {"?",        "Alias untuk bantuan perintah",           cmd_help},
    {"status",   "Status umum meter, relai, alarm & tamper",cmd_status},
    {"meas",     "Pengukuran listrik 3-fasa instan",       cmd_meas},
    {"energy",   "Akumulasi energi Wh/varh & ToU",         cmd_energy},
    {"relay",    "Kontrol/inspeksi status relai kontaktor", cmd_relay},
    {"tamper",   "Status sabotase atau simulasi injeksi",  cmd_tamper},
    {"alarm",    "Status alarm internal hardware ERR00..07",cmd_alarm},
    {"log",      "Membaca isi 3 register log FIFO SPLN",   cmd_log},
    {"demand",   "Status Maximum Demand 15-menit & reset", cmd_demand},
    {"time",     "Status kalender RTC & zona tarif ToU",   cmd_time},
    {"set_time", "Atur waktu RTC (sintaks: set_time <epoch>)", cmd_set_time},
    {"acfail",   "Simulasi listrik PLN padam/pulih (sintaks: acfail [0|1])", cmd_acfail},
    {"button",   "Simulasi tombol fisik OLED (sintaks: button <up|down|both>)", cmd_button},
#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
    {"load",     "Set daya tiruan aktif/reaktif (sintaks: load <P_W> [Q_var] [pf])", cmd_load},
    {"volt",     "Set tegangan 3-fasa tiruan (sintaks: volt <Vr_V> [Vs] [Vt])",      cmd_volt},
    {"curr",     "Set arus 3-fasa tiruan (sintaks: curr <Ir_mA> [Is] [It] [In])",    cmd_curr},
    {"synth",    "Sakelar status generator beban tiruan (sintaks: synth [on|off])",  cmd_synth},
#endif
    {"mute",     "Mode senyap konsol proteksi biner DLMS", cmd_mute},
    {"echo",     "Mencetak kembali teks yang diinputkan",  cmd_echo},
    {"clear",    "Membersihkan layar terminal konsol",     cmd_clear},
    {"cls",      "Alias untuk membersihkan layar",         cmd_clear}
};

#define CMD_TABLE_SIZE (sizeof(s_cmd_table) / sizeof(s_cmd_table[0]))

/* ========================================================================= */
/*  1. Core Console I/O & Retargeting printf                                 */
/* ========================================================================= */

void console_io_init(console_tx_fn_t tx_func) {
    s_console_tx_driver = tx_func;
    s_console_is_muted = false;
}

void console_io_set_mute(bool mute) {
    s_console_is_muted = mute;
}

bool console_io_is_muted(void) {
    return s_console_is_muted;
}

int _write(int file, char *ptr, int len) {
    (void)file;

    if (s_console_is_muted) {
        return len;
    }

    if (s_console_tx_driver != NULL && ptr != NULL && len > 0) {
        s_console_tx_driver((const uint8_t *)ptr, (size_t)len);
    }

    return len;
}

void console_io_print_boot_banner(const char *system_title, const char *uart_desc, uint32_t sysclk_mhz) {
    printf("\n==================================================\n");
    printf("   %-44s   \n", system_title ? system_title : "SYSTEM HARDWARE BRING-UP INITIALIZED");
    printf("   Console UART: %-32s\n", uart_desc ? uart_desc : "USART2 @ 115200 bps");
    printf("   System Clock: %lu MHz | GPDMA1 ACTIVE          \n", (unsigned long)sysclk_mhz);
    printf("==================================================\n\n");
}

/* ========================================================================= */
/*  2. Helper Perbandingan String Agnostik (Case-Insensitive)                */
/* ========================================================================= */

static bool str_equals_ignore_case(const char *s1, const char *s2) {
    if (s1 == NULL || s2 == NULL) return false;
    while (*s1 && *s2) {
        if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2)) {
            return false;
        }
        s1++;
        s2++;
    }
    return (*s1 == '\0' && *s2 == '\0');
}

/* ========================================================================= */
/*  3. Command Handlers Implementation                                       */
/* ========================================================================= */

static int cmd_help(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    printf("\n===================================================================\n");
    printf("           SMART METER 3-PHASE INTERACTIVE CLI COMMANDS            \n");
    printf("===================================================================\n");
    for (size_t i = 0; i < CMD_TABLE_SIZE; i++) {
        /* Sembunyikan alias duplikat pada listing utama */
        if (strcmp(s_cmd_table[i].name, "?") == 0 || strcmp(s_cmd_table[i].name, "cls") == 0) {
            continue;
        }
        printf("  %-12s : %s\n", s_cmd_table[i].name, s_cmd_table[i].desc);
    }
    printf("===================================================================\n");
    printf("Format perintah parameter:\n");
    printf("  relay trip <alasan>   : Trip relai (1:Manual, 2:Limiter, 3:Remote, 4:UnderVolt, 5:OverVolt)\n");
    printf("  relay reconnect       : Request reconnect (masuk state READY_ON)\n");
    printf("  relay confirm         : Konfirmasi rekoneksi fisik kontaktor\n");
    printf("  tamper test <vec> <1|0>: Injeksi simulasi tamper (vector bitmask)\n");
    printf("  alarm set <hex_mask>  : Injeksi alarm internal (ERR00..ERR07)\n");
    printf("  alarm clear           : Reset semua alarm internal ke nol\n");
    printf("  log <alarm|event|tamper>: Baca rekaman FIFO non-volatile\n");
    printf("  demand reset          : Reset MD 15-menit berjalan ke CMD\n");
    printf("  time                  : Baca status RTC & zona tarif ToU aktif\n");
    printf("  set_time <epoch>      : Set waktu hardware RTC (UNIX epoch)\n");
    printf("  acfail <1|0>          : Simulasi padam (1) atau pulih (0) listrik PLN\n");
    printf("  button <up|down|both> : Simulasi penekanan tombol navigasi fisik\n");
#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
    printf("  load <P_W> [Q] [pf]   : Set daya tiruan aktif/reaktif (misal: load 5000)\n");
    printf("  volt <Vr_V> [Vs] [Vt] : Set tegangan 3-fasa tiruan (misal: volt 230 atau volt 90)\n");
    printf("  curr <Ir_mA> [Is] [It]: Set arus 3-fasa tiruan (misal: curr 10000)\n");
    printf("  synth <on|off|status> : Kontrol sakelar generator beban tiruan\n");
#endif
    printf("  mute <1|0>            : 1=Senyap (proteksi DLMS), 0=Aktif\n");
    printf("===================================================================\n\n");
    return 0;
}

static int cmd_status(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    printf("\n--- STATUS SISTEM SMART METER ---\n");

    /* Status Relai */
    if (s_hooks_registered && s_hooks.get_relay_state != NULL) {
        relay_state_t st = s_hooks.get_relay_state();
        printf("  Relai Utama     : %s\n", power_relay_state_to_string(st));
        if (st == RELAY_STATE_DISCONNECTED && s_hooks.get_relay_disconnect_reason != NULL) {
            relay_reason_t reason = s_hooks.get_relay_disconnect_reason();
            printf("  Alasan Pemutusan: %s\n", power_relay_reason_to_string(reason));
        }
    } else {
        printf("  Relai Utama     : [HOOKS BELUM TERPASANG]\n");
    }

    /* Status Alarm Internal Hardware */
    if (s_hooks_registered && s_hooks.get_internal_alarm != NULL) {
        uint32_t alarm = s_hooks.get_internal_alarm();
        printf("  Alarm Hardware  : 0x%04lX ", (unsigned long)alarm);
        if (alarm == 0) {
            printf("(NORMAL / SEHAT)\n");
        } else {
            printf("[AKTIF: ");
            if (alarm & SPLN_ALARM_FLASH_ERROR)   printf("ERR00_Flash ");
            if (alarm & SPLN_ALARM_RAM_ERROR)     printf("ERR01_RAM ");
            if (alarm & SPLN_ALARM_RTC_ERROR)     printf("ERR02_Clock ");
            if (alarm & SPLN_ALARM_LOW_BATTERY)   printf("ERR03_Batt ");
            if (alarm & SPLN_ALARM_MCU_ERROR)     printf("ERR04_MCU ");
            if (alarm & SPLN_ALARM_ADC_ERROR)     printf("ERR05_ADC ");
            if (alarm & SPLN_ALARM_SUPERCAP_FAIL) printf("ERR06_Supercap ");
            if (alarm & SPLN_ALARM_RELAY_FAIL)    printf("ERR07_Relay ");
            printf("]\n");
        }
    }

    /* Status Tamper Sabotase */
    if (s_hooks_registered && s_hooks.get_tamper_context != NULL) {
        const tamper_context_t *t_ctx = s_hooks.get_tamper_context();
        if (t_ctx != NULL) {
            uint32_t active = tamper_get_active_mask(t_ctx);
            printf("  Sabotase Fisik  : 0x%04lX ", (unsigned long)active);
            if (active == 0) {
                printf("(AMAN / TIADA SABOTASE)\n");
            } else {
                printf("[TERDETEKSI: ");
                if (active & TAMPER_VECTOR_CASE_OPEN)      printf("TutupMeter(RUSAK) ");
                if (active & TAMPER_VECTOR_TERMINAL_OPEN)  printf("Terminal(ERR20) ");
                if (active & TAMPER_VECTOR_WRONG_SEQUENCE) printf("UrutanFasa(ERR21) ");
                if (active & TAMPER_VECTOR_CROSS_PHASE)    printf("CrossPhase(ERR22) ");
                if (active & TAMPER_VECTOR_NEUTRAL_BYPASS) printf("Netral(ERR23) ");
                if (active & TAMPER_VECTOR_PHASE_LOSS)     printf("PhaseLoss(ERR24) ");
                if (active & TAMPER_VECTOR_MAGNETIC_FIELD) printf("Magnet(ERR25) ");
                if (active & TAMPER_VECTOR_REVERSE_POWER)  printf("Reverse(ERR26) ");
                if (active & TAMPER_VECTOR_LOW_PF_QUAD4)   printf("PF_Quad4(ERR27) ");
                printf("]\n");
            }
        }
    }

    /* Status Tegangan Fasa */
    if (s_hooks_registered && s_hooks.get_measurements != NULL) {
        const meter_measurements_t *m = s_hooks.get_measurements();
        if (m != NULL) {
            display_voltage_status_t vr_st = display_get_voltage_status(m->voltage_r_dvolts);
            display_voltage_status_t vs_st = display_get_voltage_status(m->voltage_s_dvolts);
            display_voltage_status_t vt_st = display_get_voltage_status(m->voltage_t_dvolts);

            printf("  Status Tegangan : ");
            if (vr_st == DISPLAY_VOLT_LOW || vs_st == DISPLAY_VOLT_LOW || vt_st == DISPLAY_VOLT_LOW) {
                printf("PERINGATAN: Lo Volt (<115V)\n");
            } else if (vr_st == DISPLAY_VOLT_HIGH || vs_st == DISPLAY_VOLT_HIGH || vt_st == DISPLAY_VOLT_HIGH) {
                printf("PERINGATAN: Hi Volt (>276V)\n");
            } else {
                printf("NORMAL (115V .. 276V)\n");
            }
        }
    }

    /* Status Suplai Daya PLN (AC Power) */
    if (s_hooks_registered && s_hooks.is_ac_failed != NULL) {
        printf("  Suplai Daya PLN : %s\n", s_hooks.is_ac_failed() ? "PADAM / MATI LAMPU (AC FAIL - EMERGENCY STANDBY)" : "NORMAL (AC MAINS ACTIVE)");
    }

#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
    if (s_hooks_registered && s_hooks.get_synthetic_load != NULL) {
        synthetic_load_config_t synth;
        s_hooks.get_synthetic_load(&synth);
        printf("  Mode Metrologi  : %s\n", synth.active ? "SYNTHETIC SIMULATION (DIAGNOSTIC OVERRIDE ON)" : "PHYSICAL ADE9000 HARDWARE (LIVE SENSOR)");
    }
#endif

    printf("  Proteksi Konsol : %s\n\n", s_console_is_muted ? "MUTED (DLMS SAFE)" : "ACTIVE");
    return 0;
}

static int cmd_meas(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    if (!s_hooks_registered || s_hooks.get_measurements == NULL) {
        printf("[CLI] Error: Hook pengukuran metrologi belum terdaftar.\n");
        return -1;
    }

    const meter_measurements_t *m = s_hooks.get_measurements();
    if (m == NULL) {
        printf("[CLI] Error: Data pengukuran bernilai NULL.\n");
        return -2;
    }

    printf("\n--- PENGUKURAN KELISTRIKAN 3-FASA (INSTANTANEOUS) ---\n");
    printf("  Tegangan (V)   : R=%lu.%lu V | S=%lu.%lu V | T=%lu.%lu V\n",
           (unsigned long)(m->voltage_r_dvolts / 10), (unsigned long)(m->voltage_r_dvolts % 10),
           (unsigned long)(m->voltage_s_dvolts / 10), (unsigned long)(m->voltage_s_dvolts % 10),
           (unsigned long)(m->voltage_t_dvolts / 10), (unsigned long)(m->voltage_t_dvolts % 10));

    printf("  Arus (A)       : R=%lu.%03lu A | S=%lu.%03lu A | T=%lu.%03lu A | N=%lu.%03lu A\n",
           (unsigned long)(m->current_r_mamps / 1000), (unsigned long)(m->current_r_mamps % 1000),
           (unsigned long)(m->current_s_mamps / 1000), (unsigned long)(m->current_s_mamps % 1000),
           (unsigned long)(m->current_t_mamps / 1000), (unsigned long)(m->current_t_mamps % 1000),
           (unsigned long)(m->current_n_mamps / 1000), (unsigned long)(m->current_n_mamps % 1000));

    printf("  Daya Aktif (W) : %ld W\n", (long)m->active_power_w);
    printf("  Daya Reaktif   : %ld var\n", (long)m->reactive_power_var);
    printf("  Daya Semu (VA) : %lu VA\n", (unsigned long)m->apparent_power_va);
    printf("  Faktor Daya    : %u.%03u\n", m->power_factor_ppm / 1000, m->power_factor_ppm % 1000);
    printf("  Frekuensi      : %u.%03u Hz\n\n", m->frequency_mhz / 1000, m->frequency_mhz % 1000);
    return 0;
}

static int cmd_energy(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    if (!s_hooks_registered || s_hooks.get_measurements == NULL) {
        printf("[CLI] Error: Hook pengukuran energi belum terdaftar.\n");
        return -1;
    }

    const meter_measurements_t *m = s_hooks.get_measurements();
    if (m == NULL) {
        printf("[CLI] Error: Data pengukuran bernilai NULL.\n");
        return -2;
    }

    printf("\n--- STAND AKUMULASI ENERGI TRANSAKSI ---\n");
    printf("  Energi Aktif Total  (01.08) : %llu Wh (%.3f kWh)\n",
           (unsigned long long)m->active_energy_wh, (double)m->active_energy_wh / 1000.0);
    printf("  Energi Terima PLTS  (02.08) : %llu Wh (%.3f kWh)\n",
           (unsigned long long)m->active_energy_import_wh, (double)m->active_energy_import_wh / 1000.0);
    printf("  Energi Reaktif      (03.08) : %llu varh (%.3f kvarh)\n",
           (unsigned long long)m->reactive_energy_varh, (double)m->reactive_energy_varh / 1000.0);
    printf("  ToU Tarif 1 WBP     (01.08.1): %llu Wh\n", (unsigned long long)m->active_energy_wbp_wh);
    printf("  ToU Tarif 2 LWBP1   (01.08.2): %llu Wh\n", (unsigned long long)m->active_energy_lwbp_wh);
    printf("  ToU Tarif 3 LWBP2   (01.08.3): %llu Wh\n\n", (unsigned long long)m->active_energy_lwbp2_wh);
    return 0;
}

static int cmd_relay(int argc, char *argv[]) {
    if (!s_hooks_registered || s_hooks.get_relay_state == NULL) {
        printf("[CLI] Error: Hook relai belum terdaftar.\n");
        return -1;
    }

    if (argc < 2 || str_equals_ignore_case(argv[1], "status") || str_equals_ignore_case(argv[1], "get")) {
        relay_state_t st = s_hooks.get_relay_state();
        printf("[RELAY] State: %s\n", power_relay_state_to_string(st));
        if (st == RELAY_STATE_DISCONNECTED && s_hooks.get_relay_disconnect_reason != NULL) {
            printf("[RELAY] Reason: %s\n", power_relay_reason_to_string(s_hooks.get_relay_disconnect_reason()));
        }
        return 0;
    }

    if (str_equals_ignore_case(argv[1], "trip")) {
        relay_reason_t r = RELAY_REASON_MANUAL;
        if (argc >= 3) {
            int val = atoi(argv[2]);
            if (val >= 1 && val <= 7) {
                r = (relay_reason_t)val;
            }
        }
        if (s_hooks.relay_disconnect != NULL) {
            bool ok = s_hooks.relay_disconnect(r);
            printf("[RELAY] Trip command executed: %s (Reason: %s)\n",
                   ok ? "SUCCESS" : "IGNORED/FAILED", power_relay_reason_to_string(r));
            return ok ? 0 : -2;
        }
    }

    if (str_equals_ignore_case(argv[1], "reconnect")) {
        if (s_hooks.relay_request_reconnect != NULL) {
            bool ok = s_hooks.relay_request_reconnect();
            printf("[RELAY] Request reconnect executed: %s (New State: %s)\n",
                   ok ? "SUCCESS -> READY_ON" : "FAILED", power_relay_state_to_string(s_hooks.get_relay_state()));
            return ok ? 0 : -3;
        }
    }

    if (str_equals_ignore_case(argv[1], "confirm")) {
        if (s_hooks.relay_confirm_reconnect != NULL) {
            bool ok = s_hooks.relay_confirm_reconnect();
            printf("[RELAY] Confirm reconnect executed: %s (New State: %s)\n",
                   ok ? "SUCCESS -> CONNECTED" : "FAILED (Must be in READY_ON)",
                   power_relay_state_to_string(s_hooks.get_relay_state()));
            return ok ? 0 : -4;
        }
    }

    printf("[CLI] Sintaks relay salah. Gunakan: relay [status | trip <1..7> | reconnect | confirm]\n");
    return -1;
}

static int cmd_tamper(int argc, char *argv[]) {
    if (!s_hooks_registered || s_hooks.get_tamper_context == NULL) {
        printf("[CLI] Error: Hook tamper belum terdaftar.\n");
        return -1;
    }

    const tamper_context_t *ctx = s_hooks.get_tamper_context();
    if (ctx == NULL) {
        printf("[CLI] Error: Konteks tamper bernilai NULL.\n");
        return -2;
    }

    if (argc < 2 || str_equals_ignore_case(argv[1], "status")) {
        uint32_t active = tamper_get_active_mask(ctx);
        printf("\n--- STATUS SABOTASE (TAMPER) ---\n");
        printf("  Active Mask  : 0x%04lX\n", (unsigned long)active);
        printf("  Alarm Pending: %s\n", tamper_is_alarm_pending(ctx) ? "YA" : "TIDAK");
        printf("  Relay Trip   : %s\n", tamper_should_trip_relay(ctx) ? "TRIP" : "TIDAK");
        printf("  LED Kuning   : %s\n\n", tamper_should_turn_on_led(ctx) ? "MENYALA" : "PADAM");
        return 0;
    }

    if (str_equals_ignore_case(argv[1], "test")) {
        if (argc < 4) {
            printf("[CLI] Sintaks: tamper test <vector_id> <1|0>\n");
            printf("  Contoh: tamper test 4 1  (Injeksi Medan Magnet Aktif)\n");
            return -1;
        }
        uint32_t vec_id = (uint32_t)strtoul(argv[2], NULL, 0);
        bool asserted = (atoi(argv[3]) != 0);

        if (s_hooks.inject_tamper != NULL) {
            s_hooks.inject_tamper((tamper_vector_t)vec_id, asserted);
            printf("[CLI] Injeksi tamper vector 0x%04lX %s BERHASIL. Mask baru: 0x%04lX\n",
                   (unsigned long)vec_id, asserted ? "ASSERT" : "CLEAR",
                   (unsigned long)tamper_get_active_mask(ctx));
            return 0;
        }
    }

    printf("[CLI] Sintaks tamper salah. Gunakan: tamper [status | test <vec> <1|0>]\n");
    return -1;
}

static int cmd_alarm(int argc, char *argv[]) {
    if (!s_hooks_registered || s_hooks.get_internal_alarm == NULL) {
        printf("[CLI] Error: Hook alarm internal belum terdaftar.\n");
        return -1;
    }

    if (argc < 2 || str_equals_ignore_case(argv[1], "status")) {
        uint32_t alarm = s_hooks.get_internal_alarm();
        printf("\n--- STATUS ALARM INTERNAL HARDWARE (SPLN TABEL 4) ---\n");
        printf("  Alarm Mask : 0x%04lX\n", (unsigned long)alarm);
        printf("  Relay Trip : %s\n", spln_alarm_should_trip_relay(alarm) ? "TRIP" : "TIDAK");
        printf("  LED Kuning : %s\n", spln_alarm_should_turn_on_led(alarm) ? "MENYALA" : "PADAM");
        return 0;
    }

    if (str_equals_ignore_case(argv[1], "set")) {
        if (argc < 3) {
            printf("[CLI] Sintaks: alarm set <mask_hex>\n");
            return -1;
        }
        uint32_t mask = (uint32_t)strtoul(argv[2], NULL, 0);
        if (s_hooks.set_internal_alarm != NULL) {
            s_hooks.set_internal_alarm(mask);
            printf("[CLI] Alarm mask diatur ke: 0x%04lX\n", (unsigned long)mask);
            return 0;
        }
    }

    if (str_equals_ignore_case(argv[1], "clear")) {
        if (s_hooks.set_internal_alarm != NULL) {
            s_hooks.set_internal_alarm(0);
            printf("[CLI] Semua alarm internal di-reset ke 0 (SEHAT).\n");
            return 0;
        }
    }

    printf("[CLI] Sintaks alarm salah. Gunakan: alarm [status | set <hex> | clear]\n");
    return -1;
}

static int cmd_log(int argc, char *argv[]) {
    if (!s_hooks_registered || s_hooks.get_tamper_context == NULL) {
        printf("[CLI] Error: Hook log konteks belum terdaftar.\n");
        return -1;
    }

    const tamper_context_t *ctx = s_hooks.get_tamper_context();
    if (ctx == NULL) {
        printf("[CLI] Error: Konteks log bernilai NULL.\n");
        return -2;
    }

    if (argc < 2) {
        printf("[CLI] Sintaks: log <alarm | event | tamper>\n");
        return -1;
    }

    const spln_log_fifo_t *target_fifo = NULL;
    const char *log_name = "";

    if (str_equals_ignore_case(argv[1], "alarm")) {
        target_fifo = &ctx->alarm_log_fifo;
        log_name = "REGISTER 1: ALARM LOG (SPLN TABEL 18 NO 1..8)";
    } else if (str_equals_ignore_case(argv[1], "event")) {
        target_fifo = &ctx->event_log_fifo;
        log_name = "REGISTER 2: EVENT LOG (SPLN TABEL 18 NO 9..19 & 29)";
    } else if (str_equals_ignore_case(argv[1], "tamper")) {
        target_fifo = &ctx->tamper_log_fifo;
        log_name = "REGISTER 3: TAMPER LOG (SPLN TABEL 18 NO 20..28)";
    } else {
        printf("[CLI] Jenis log tidak dikenal: '%s'. Pilih: alarm, event, tamper.\n", argv[1]);
        return -1;
    }

    printf("\n--- REKAMAN %s ---\n", log_name);
    printf("Total Rekaman Tersimpan: %zu / %u (Kumulatif: %lu)\n",
           target_fifo->count, SPLN_LOG_FIFO_MAX_ENTRIES, (unsigned long)target_fifo->total_events);
    printf("---------------------------------------------------------------\n");
    printf(" No |  Timestamp (Epoch)  | Event Code | Status | Parameter    \n");
    printf("----+---------------------+------------+--------+--------------\n");

    for (size_t i = 0; i < target_fifo->count; i++) {
        spln_standard_log_entry_t entry;
        bool ok = spln_log_get_entry_by_index(target_fifo, i, &entry);
        if (ok) {
            printf(" %02zu | %19lu | 0x%04X     | %-6s | 0x%08lX\n",
                   i + 1, (unsigned long)entry.timestamp_epoch,
                   entry.event_code, entry.status_flags ? "START" : "RESTORE",
                   (unsigned long)entry.parameter_val);
        }
    }
    printf("---------------------------------------------------------------\n\n");
    return 0;
}

static int cmd_demand(int argc, char *argv[]) {
    if (!s_hooks_registered || s_hooks.get_max_demand == NULL) {
        printf("[CLI] Error: Hook Maximum Demand belum terdaftar.\n");
        return -1;
    }

    const max_demand_tracker_t *tracker = s_hooks.get_max_demand();
    if (tracker == NULL) {
        printf("[CLI] Error: Tracker MD bernilai NULL.\n");
        return -2;
    }

    if (argc < 2 || str_equals_ignore_case(argv[1], "status")) {
        printf("\n--- MAXIMUM DEMAND 15-MENIT TRACKER (SPLN TABEL E.1) ---\n");
        printf("  Current Demand (15-min) : %lu W\n", (unsigned long)tracker->current_demand_w);
        printf("  Peak Max Demand (MD)    : %lu W\n", (unsigned long)tracker->max_demand_w);
        printf("  Waktu Kejadian (Epoch)  : %lu\n", (unsigned long)tracker->max_demand_timestamp);
        printf("  Cumulative MD (CMD)     : %lu W\n", (unsigned long)tracker->cumulative_max_demand_w);
        printf("  Jumlah Reset Counter    : %lu kali\n\n", (unsigned long)tracker->reset_count);
        return 0;
    }

    if (str_equals_ignore_case(argv[1], "reset")) {
        if (s_hooks.reset_max_demand != NULL) {
            s_hooks.reset_max_demand();
            printf("[CLI] Maximum demand berhasil di-reset ke CMD.\n");
            return 0;
        }
    }

    printf("[CLI] Sintaks demand salah. Gunakan: demand [status | reset]\n");
    return -1;
}

static int cmd_mute(int argc, char *argv[]) {
    if (argc < 2) {
        printf("[CLI] Status konsol saat ini: %s\n", s_console_is_muted ? "MUTED" : "ACTIVE");
        return 0;
    }

    int val = atoi(argv[1]);
    console_io_set_mute(val != 0);
    printf("[CLI] Konsol diatur ke: %s\n", (val != 0) ? "MUTED (DLMS SAFE)" : "ACTIVE");
    return 0;
}

static int cmd_echo(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        printf("%s%s", argv[i], (i < argc - 1) ? " " : "");
    }
    printf("\n");
    return 0;
}

static int cmd_clear(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    printf("\033[2J\033[H");
    return 0;
}

static int cmd_time(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    if (!s_hooks_registered || s_hooks.get_time == NULL) {
        printf("[CLI] Error: Hook waktu RTC belum terdaftar.\n");
        return -1;
    }

    uint32_t epoch = s_hooks.get_time();
    uint32_t rem_sec = epoch % 86400U;
    uint8_t sec  = (uint8_t)(rem_sec % 60U);
    uint8_t min  = (uint8_t)((rem_sec / 60U) % 60U);
    uint8_t hour = (uint8_t)(rem_sec / 3600U);

    uint32_t z = (epoch / 86400U) + 719468U;
    uint32_t era = z / 146097U;
    uint32_t doe = z - era * 146097U;
    uint32_t yoe = (doe - doe / 1460U + doe / 36524U - doe / 146096U) / 365U;
    uint32_t y = yoe + era * 400U;
    uint32_t doy = doe - (365U * yoe + yoe / 4U - yoe / 100U);
    uint32_t mp = (5U * doy + 2U) / 153U;
    uint32_t d = doy - (153U * mp + 2U) / 5U + 1U;
    uint32_t m = (mp < 10U) ? (mp + 3U) : (mp - 9U);
    if (m <= 2U) y += 1U;

    uint8_t zone = (s_hooks.get_active_tariff != NULL) ? s_hooks.get_active_tariff() : 1U;
    const char *tariff_names[] = {
        "Tarif 1: WBP (17:00 - 22:00)",
        "Tarif 2: LWBP Malam (22:00 - 06:00)",
        "Tarif 3: LWBP Siang (06:00 - 17:00)",
        "Tarif 4 (Reserved)", "Tarif 5 (Reserved)", "Tarif 6 (Reserved)",
        "Tarif 7 (Reserved)", "Tarif 8 (Reserved)"
    };
    const char *t_str = (zone < 8U) ? tariff_names[zone] : "Unknown";

    printf("\n--- STATUS REAL-TIME CLOCK (RTC) & TARIF TOU ---\n");
    printf("  Tanggal Kalender: %02u-%02u-%04u\n", (unsigned)d, (unsigned)m, (unsigned)y);
    printf("  Jam / Waktu     : %02u:%02u:%02u WIB\n", (unsigned)hour, (unsigned)min, (unsigned)sec);
    printf("  UNIX Epoch      : %lu detik\n", (unsigned long)epoch);
    printf("  Zona Tarif Aktif: [%u] %s\n\n", (unsigned)zone, t_str);
    return 0;
}

static int cmd_set_time(int argc, char *argv[]) {
    if (argc < 2) {
        printf("[CLI] Penggunaan: set_time <epoch_sec>\n");
        printf("       Contoh: set_time 1793181600 (28-10-2026 10:00:00)\n");
        return -1;
    }

    if (!s_hooks_registered || s_hooks.set_time == NULL) {
        printf("[CLI] Error: Hook set_time belum terdaftar.\n");
        return -2;
    }

    uint32_t new_epoch = (uint32_t)strtoul(argv[1], NULL, 10);
    if (new_epoch < 1700000000UL) {
        printf("[CLI] Error: Epoch tidak valid (harus >= 1700000000).\n");
        return -3;
    }

    s_hooks.set_time(new_epoch);
    printf("[CLI] Waktu hardware RTC & OLED berhasil diatur ke epoch: %lu\n", (unsigned long)new_epoch);
    return cmd_time(1, argv);
}

static int cmd_acfail(int argc, char *argv[]) {
    if (!s_hooks_registered) {
        printf("[CLI] Error: Hooks meter belum terdaftar.\n");
        return -1;
    }

    if (argc < 2) {
        bool is_fail = (s_hooks.is_ac_failed != NULL) ? s_hooks.is_ac_failed() : false;
        printf("\n--- STATUS SUPLAI DAYA LISTRIK PLN (AC POWER) ---\n");
        printf("  Status: %s\n", is_fail ? "PADAM / MATI LAMPU (AC FAIL - EMERGENCY STANDBY)" : "NORMAL (AC MAINS ACTIVE)");
        printf("  Petunjuk: Ketik 'acfail 1' untuk simulasi padam, atau 'acfail 0' untuk pemulihan normal.\n\n");
        return 0;
    }

    if (s_hooks.set_ac_fail == NULL) {
        printf("[CLI] Error: Hook set_ac_fail belum terdaftar.\n");
        return -2;
    }

    if (str_equals_ignore_case(argv[1], "1") || str_equals_ignore_case(argv[1], "on") ||
        str_equals_ignore_case(argv[1], "cut") || str_equals_ignore_case(argv[1], "fail")) {
        s_hooks.set_ac_fail(true);
        printf("[CLI] SIMULASI: Listrik PLN PADAM (AC FAIL) diaktifkan!\n");
        printf("      - Checkpoint energi tersimpan di NVRAM Flash\n");
        printf("      - Snapshot 3 Log FIFO tersimpan di NVRAM\n");
        printf("      - Paket DLMS Last-Gasp terkirim\n");
        printf("      - Layar OLED dimatikan (Sleep 0xAE)\n");
        printf("      - Power gating periferal aktif (LCD_PWR_EN=0, RS232=0, Modem=0)\n");
    } else if (str_equals_ignore_case(argv[1], "0") || str_equals_ignore_case(argv[1], "off") ||
               str_equals_ignore_case(argv[1], "restore") || str_equals_ignore_case(argv[1], "normal")) {
        s_hooks.set_ac_fail(false);
        printf("[CLI] SIMULASI: Listrik PLN PULIH NORMAL (AC RESTORED)!\n");
        printf("      - Log event Power Up tercatat\n");
        printf("      - Rail daya diaktifkan kembali\n");
        printf("      - Layar OLED diaktifkan kembali\n");
    } else {
        printf("[CLI] Penggunaan: acfail [0|1|off|on|restore]\n");
        return -3;
    }
    return 0;
}

static int cmd_button(int argc, char *argv[]) {
    if (!s_hooks_registered || s_hooks.button_press == NULL) {
        printf("[CLI] Error: Hook button_press belum terdaftar.\n");
        return -1;
    }

    if (argc < 2) {
        printf("[CLI] Penggunaan: button <up|down|both>\n");
        return -1;
    }

    button_dir_t dir;
    if (str_equals_ignore_case(argv[1], "up")) {
        dir = BUTTON_DIR_UP;
    } else if (str_equals_ignore_case(argv[1], "down")) {
        dir = BUTTON_DIR_DOWN;
    } else if (str_equals_ignore_case(argv[1], "both")) {
        dir = BUTTON_DIR_BOTH;
    } else {
        printf("[CLI] Tombol tidak valid: '%s'. Gunakan: up, down, atau both.\n", argv[1]);
        return -2;
    }

    s_hooks.button_press(dir);
    bool is_fail = (s_hooks.is_ac_failed != NULL) ? s_hooks.is_ac_failed() : false;
    if (is_fail) {
        printf("[CLI] Tombol '%s' ditekan saat PLN PADAM -> Layar OLED aktif sementara (10 detik standby).\n", argv[1]);
    } else {
        printf("[CLI] Tombol '%s' ditekan -> Halaman UI OLED diperbarui.\n", argv[1]);
    }
    return 0;
}

#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
static int cmd_synth(int argc, char *argv[]) {
    if (!s_hooks_registered || s_hooks.get_synthetic_load == NULL || s_hooks.set_synthetic_load == NULL) {
        printf("[CLI] Error: Hooks generator beban tiruan belum terdaftar.\n");
        return -1;
    }

    synthetic_load_config_t cfg;
    s_hooks.get_synthetic_load(&cfg);

    if (argc < 2 || str_equals_ignore_case(argv[1], "status")) {
        printf("\n--- STATUS GENERATOR BEBAN METROLOGI TIRUAN (SYNTHETIC LOAD) ---\n");
        printf("  Status Generator : %s\n", cfg.active ? "AKTIF (DIAGNOSTIC OVERRIDE ON)" : "NON-AKTIF (MENGGUNAKAN ADE9000 FISIK)");
        printf("  Daya Aktif (P)   : %ld Watt\n", (long)cfg.active_power_w);
        printf("  Daya Reaktif (Q) : %ld var\n", (long)cfg.reactive_power_var);
        printf("  Faktor Daya (PF) : %u.%03u\n", (unsigned)(cfg.power_factor_ppm / 1000U), (unsigned)(cfg.power_factor_ppm % 1000U));
        printf("  Tegangan 3-Fasa  : R=%lu.%u V | S=%lu.%u V | T=%lu.%u V\n",
               (unsigned long)(cfg.voltage_r_dv / 10U), (unsigned)(cfg.voltage_r_dv % 10U),
               (unsigned long)(cfg.voltage_s_dv / 10U), (unsigned)(cfg.voltage_s_dv % 10U),
               (unsigned long)(cfg.voltage_t_dv / 10U), (unsigned)(cfg.voltage_t_dv % 10U));
        printf("  Arus 3-Fasa      : R=%lu.%03u A | S=%lu.%03u A | T=%lu.%03u A | N=%lu.%03u A\n\n",
               (unsigned long)(cfg.current_r_ma / 1000U), (unsigned)(cfg.current_r_ma % 1000U),
               (unsigned long)(cfg.current_s_ma / 1000U), (unsigned)(cfg.current_s_ma % 1000U),
               (unsigned long)(cfg.current_t_ma / 1000U), (unsigned)(cfg.current_t_ma % 1000U),
               (unsigned long)(cfg.current_n_ma / 1000U), (unsigned)(cfg.current_n_ma % 1000U));
        printf("  Petunjuk: Ketik 'synth on' untuk mengaktifkan, 'synth off' untuk kembali ke sensor fisik ADE9000,\n");
        printf("            atau 'synth reset' untuk mereset ke beban nominal (220V, 15A, 9.9kW).\n\n");
        return 0;
    }

    if (str_equals_ignore_case(argv[1], "on") || str_equals_ignore_case(argv[1], "1")) {
        cfg.active = true;
        s_hooks.set_synthetic_load(&cfg);
        printf("[CLI] Generator beban tiruan DIAKTIFKAN. Layar OLED & DLMS membaca data tiruan.\n");
    } else if (str_equals_ignore_case(argv[1], "off") || str_equals_ignore_case(argv[1], "0")) {
        cfg.active = false;
        s_hooks.set_synthetic_load(&cfg);
        printf("[CLI] Generator beban tiruan DINONAKTIFKAN. Kembali ke pembacaan sensor fisik ADE9000.\n");
    } else if (str_equals_ignore_case(argv[1], "reset")) {
        cfg.active = true;
        cfg.active_power_w = 9900;
        cfg.reactive_power_var = 1485;
        cfg.power_factor_ppm = 990;
        cfg.voltage_r_dv = 2200;
        cfg.voltage_s_dv = 2200;
        cfg.voltage_t_dv = 2200;
        cfg.current_r_ma = 15000;
        cfg.current_s_ma = 15000;
        cfg.current_t_ma = 15000;
        cfg.current_n_ma = 50;
        s_hooks.set_synthetic_load(&cfg);
        printf("[CLI] Generator beban tiruan di-reset ke nilai nominal (220.0V, 15.0A, 9.9kW).\n");
    } else {
        printf("[CLI] Parameter tidak valid: '%s'. Gunakan: synth <on|off|status|reset>\n", argv[1]);
        return -2;
    }
    return 0;
}

static int cmd_load(int argc, char *argv[]) {
    if (argc < 2) {
        printf("[CLI] Penggunaan: load <P_Watt> [Q_var] [pf_permille]\n");
        printf("       Contoh: load 5000          -> Set daya aktif 5000 W\n");
        printf("       Contoh: load 15000         -> Uji trip pembatas daya (Overload >11kW)\n");
        printf("       Contoh: load 3500 500 990  -> Set P=3500W, Q=500var, PF=0.990\n");
        return -1;
    }

    if (!s_hooks_registered || s_hooks.get_synthetic_load == NULL || s_hooks.set_synthetic_load == NULL) {
        printf("[CLI] Error: Hooks generator beban tiruan belum terdaftar.\n");
        return -2;
    }

    synthetic_load_config_t cfg;
    s_hooks.get_synthetic_load(&cfg);

    cfg.active_power_w = (int32_t)strtol(argv[1], NULL, 10);
    if (argc >= 3) {
        cfg.reactive_power_var = (int32_t)strtol(argv[2], NULL, 10);
    } else {
        cfg.reactive_power_var = (int32_t)(cfg.active_power_w * 15 / 100);
    }

    if (argc >= 4) {
        cfg.power_factor_ppm = (uint16_t)strtoul(argv[3], NULL, 10);
    } else {
        cfg.power_factor_ppm = 990U;
    }

    uint32_t v_avg = (cfg.voltage_r_dv > 0) ? (cfg.voltage_r_dv / 10U) : 220U;
    if (v_avg == 0) v_avg = 220U;
    uint32_t pf = (cfg.power_factor_ppm > 0) ? cfg.power_factor_ppm : 1000U;

    int32_t abs_p = (cfg.active_power_w >= 0) ? cfg.active_power_w : -cfg.active_power_w;
    uint32_t i_calc_ma = (uint32_t)(((uint64_t)abs_p * 1000ULL * 1000ULL) / ((uint64_t)3U * v_avg * pf));
    cfg.current_r_ma = i_calc_ma;
    cfg.current_s_ma = i_calc_ma;
    cfg.current_t_ma = i_calc_ma;

    cfg.active = true;
    s_hooks.set_synthetic_load(&cfg);

    printf("[CLI] Beban tiruan diperbarui: P=%ld W, Q=%ld var, PF=%u.%03u, Arus Fasa=%lu.%03u A (Mode: AKTIF)\n",
           (long)cfg.active_power_w, (long)cfg.reactive_power_var,
           (unsigned)(cfg.power_factor_ppm / 1000U), (unsigned)(cfg.power_factor_ppm % 1000U),
           (unsigned long)(i_calc_ma / 1000U), (unsigned)(i_calc_ma % 1000U));
    if (cfg.active_power_w > 11000) {
        printf("      [PERINGATAN] Daya melebihi batas 11.000 Watt -> Bersiap menguji trip relai pembatas beban!\n");
    }
    return 0;
}

static int cmd_volt(int argc, char *argv[]) {
    if (argc < 2) {
        printf("[CLI] Penggunaan: volt <Vr_V> [Vs_V] [Vt_V]\n");
        printf("       Contoh: volt 230           -> Set tegangan seimbang 230.0 V ke semua fasa\n");
        printf("       Contoh: volt 90            -> Uji trip proteksi tegangan rendah (<110V)\n");
        printf("       Contoh: volt 290           -> Uji trip proteksi tegangan lebih (>280V)\n");
        printf("       Contoh: volt 230 0 230     -> Uji deteksi hilang fasa S (ERR24)\n");
        return -1;
    }

    if (!s_hooks_registered || s_hooks.get_synthetic_load == NULL || s_hooks.set_synthetic_load == NULL) {
        printf("[CLI] Error: Hooks generator beban tiruan belum terdaftar.\n");
        return -2;
    }

    synthetic_load_config_t cfg;
    s_hooks.get_synthetic_load(&cfg);

    uint32_t vr_v = (uint32_t)strtoul(argv[1], NULL, 10);
    cfg.voltage_r_dv = vr_v * 10U;

    if (argc >= 3) {
        cfg.voltage_s_dv = (uint32_t)strtoul(argv[2], NULL, 10) * 10U;
    } else {
        cfg.voltage_s_dv = cfg.voltage_r_dv;
    }

    if (argc >= 4) {
        cfg.voltage_t_dv = (uint32_t)strtoul(argv[3], NULL, 10) * 10U;
    } else {
        cfg.voltage_t_dv = cfg.voltage_r_dv;
    }

    cfg.active = true;
    s_hooks.set_synthetic_load(&cfg);

    printf("[CLI] Tegangan tiruan 3-fasa diperbarui: Vr=%lu.%u V, Vs=%lu.%u V, Vt=%lu.%u V (Mode: AKTIF)\n",
           (unsigned long)(cfg.voltage_r_dv / 10U), (unsigned)(cfg.voltage_r_dv % 10U),
           (unsigned long)(cfg.voltage_s_dv / 10U), (unsigned)(cfg.voltage_s_dv % 10U),
           (unsigned long)(cfg.voltage_t_dv / 10U), (unsigned)(cfg.voltage_t_dv % 10U));

    if (cfg.voltage_r_dv < 1100U || cfg.voltage_s_dv < 1100U || cfg.voltage_t_dv < 1100U) {
        printf("      [PERINGATAN] Terdeteksi tegangan di bawah ambang batas (<110V) -> Lo Volt trip aktif!\n");
    } else if (cfg.voltage_r_dv > 2800U || cfg.voltage_s_dv > 2800U || cfg.voltage_t_dv > 2800U) {
        printf("      [PERINGATAN] Terdeteksi tegangan di atas ambang batas (>280V) -> Hi Volt trip aktif!\n");
    }
    return 0;
}

static int cmd_curr(int argc, char *argv[]) {
    if (argc < 2) {
        printf("[CLI] Penggunaan: curr <Ir_mA> [Is_mA] [It_mA] [In_mA]\n");
        printf("       Contoh: curr 10000         -> Set arus seimbang 10.000 A\n");
        printf("       Contoh: curr 20000 5000 5000 -> Uji ketidakseimbangan fasa (Asymmetry)\n");
        return -1;
    }

    if (!s_hooks_registered || s_hooks.get_synthetic_load == NULL || s_hooks.set_synthetic_load == NULL) {
        printf("[CLI] Error: Hooks generator beban tiruan belum terdaftar.\n");
        return -2;
    }

    synthetic_load_config_t cfg;
    s_hooks.get_synthetic_load(&cfg);

    cfg.current_r_ma = (uint32_t)strtoul(argv[1], NULL, 10);
    if (argc >= 3) {
        cfg.current_s_ma = (uint32_t)strtoul(argv[2], NULL, 10);
    } else {
        cfg.current_s_ma = cfg.current_r_ma;
    }

    if (argc >= 4) {
        cfg.current_t_ma = (uint32_t)strtoul(argv[3], NULL, 10);
    } else {
        cfg.current_t_ma = cfg.current_r_ma;
    }

    if (argc >= 5) {
        cfg.current_n_ma = (uint32_t)strtoul(argv[4], NULL, 10);
    }

    uint64_t p_va = ((uint64_t)cfg.current_r_ma * cfg.voltage_r_dv +
                     (uint64_t)cfg.current_s_ma * cfg.voltage_s_dv +
                     (uint64_t)cfg.current_t_ma * cfg.voltage_t_dv) / 10000ULL;
    cfg.active_power_w = (int32_t)((p_va * cfg.power_factor_ppm) / 1000ULL);

    cfg.active = true;
    s_hooks.set_synthetic_load(&cfg);

    printf("[CLI] Arus tiruan 3-fasa diperbarui: Ir=%lu.%03u A, Is=%lu.%03u A, It=%lu.%03u A, In=%lu.%03u A\n",
           (unsigned long)(cfg.current_r_ma / 1000U), (unsigned)(cfg.current_r_ma % 1000U),
           (unsigned long)(cfg.current_s_ma / 1000U), (unsigned)(cfg.current_s_ma % 1000U),
           (unsigned long)(cfg.current_t_ma / 1000U), (unsigned)(cfg.current_t_ma % 1000U),
           (unsigned long)(cfg.current_n_ma / 1000U), (unsigned)(cfg.current_n_ma % 1000U));
    printf("      Daya aktif terhitung: %ld Watt (Mode: AKTIF)\n", (long)cfg.active_power_w);
    return 0;
}
#endif

/* ========================================================================= */
/*  4. Interactive Serial CLI Engine & Dispatcher                           */
/* ========================================================================= */

void console_cli_init(const console_meter_hooks_t *hooks) {
    if (hooks != NULL) {
        s_hooks = *hooks;
        s_hooks_registered = true;
    } else {
        memset(&s_hooks, 0, sizeof(s_hooks));
        s_hooks_registered = false;
    }
    s_rx_line_idx = 0;
    s_rx_line_buf[0] = '\0';
}

void console_cli_print_prompt(void) {
    printf("SMART-METER> ");
    fflush(stdout);
}

int console_cli_execute_command(char *cmd_line) {
    if (cmd_line == NULL) return -1;

    while (*cmd_line && isspace((unsigned char)*cmd_line)) {
        cmd_line++;
    }

    if (*cmd_line == '\0') {
        return 0;
    }

    char *argv[CONSOLE_MAX_ARGS];
    int   argc = 0;

    char *token = strtok(cmd_line, " \t\r\n");
    while (token != NULL && argc < CONSOLE_MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\r\n");
    }

    if (argc == 0) return 0;

    for (size_t i = 0; i < CMD_TABLE_SIZE; i++) {
        if (str_equals_ignore_case(argv[0], s_cmd_table[i].name)) {
            return s_cmd_table[i].handler(argc, argv);
        }
    }

    printf("Perintah tidak dikenal: '%s'. Ketik 'help' untuk melihat daftar perintah.\n", argv[0]);
    return -1;
}

void console_cli_process_rx_byte(uint8_t byte) {
    if (byte == '\r' || byte == '\n') {
        if (s_rx_line_idx > 0) {
            s_rx_line_buf[s_rx_line_idx] = '\0';
            printf("\r\n");
            console_cli_execute_command(s_rx_line_buf);
            s_rx_line_idx = 0;
            s_rx_line_buf[0] = '\0';
            console_cli_print_prompt();
        } else {
            printf("\r\n");
            console_cli_print_prompt();
        }
        return;
    }

    if (byte == '\b' || byte == 0x7F) {
        if (s_rx_line_idx > 0) {
            s_rx_line_idx--;
            s_rx_line_buf[s_rx_line_idx] = '\0';
            if (!s_console_is_muted) {
                printf("\b \b");
                fflush(stdout);
            }
        }
        return;
    }

    if (byte >= 0x20 && byte <= 0x7E) {
        if (s_rx_line_idx < CONSOLE_CMD_MAX_LEN - 1) {
            s_rx_line_buf[s_rx_line_idx++] = (char)byte;
            s_rx_line_buf[s_rx_line_idx] = '\0';
            if (!s_console_is_muted) {
                putchar((int)byte);
                fflush(stdout);
            }
        }
    }
}

void console_cli_process_rx_buffer(const uint8_t *buf, size_t len) {
    if (buf == NULL || len == 0) return;
    for (size_t i = 0; i < len; i++) {
        console_cli_process_rx_byte(buf[i]);
    }
}
