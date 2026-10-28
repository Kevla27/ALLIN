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

/* Tabel pendaftaran perintah CLI */
static const console_cmd_entry_t s_cmd_table[] = {
    {"help",   "Menampilkan daftar perintah dan sintaks", cmd_help},
    {"?",      "Alias untuk bantuan perintah",           cmd_help},
    {"status", "Status umum meter, relai, alarm & tamper",cmd_status},
    {"meas",   "Pengukuran listrik 3-fasa instan",       cmd_meas},
    {"energy", "Akumulasi energi Wh/varh & ToU",         cmd_energy},
    {"relay",  "Kontrol/inspeksi status relai kontaktor", cmd_relay},
    {"tamper", "Status sabotase atau simulasi injeksi",  cmd_tamper},
    {"alarm",  "Status alarm internal hardware ERR00..07",cmd_alarm},
    {"log",    "Membaca isi 3 register log FIFO SPLN",   cmd_log},
    {"demand", "Status Maximum Demand 15-menit & reset", cmd_demand},
    {"mute",   "Mode senyap konsol proteksi biner DLMS", cmd_mute},
    {"echo",   "Mencetak kembali teks yang diinputkan",  cmd_echo},
    {"clear",  "Membersihkan layar terminal konsol",     cmd_clear},
    {"cls",    "Alias untuk membersihkan layar",         cmd_clear}
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
