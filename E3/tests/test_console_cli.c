/**
 * @file test_console_cli.c
 * @brief Unit Test untuk Modul Interactive Serial CLI Parser (E3/Application Lead)
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "console_io.h"
#include "bsp_power_mode.h"

/* Variabel state tiruan untuk mock hooks */
static meter_measurements_t s_mock_meas;
static uint32_t             s_mock_alarm_mask = 0;
static tamper_context_t     s_mock_tamper_ctx;
static relay_state_t        s_mock_relay_state = RELAY_STATE_CONNECTED;
static relay_reason_t       s_mock_relay_reason = RELAY_REASON_NONE;
static max_demand_tracker_t s_mock_demand_tracker;

static uint32_t             s_mock_time_epoch = 1793181600;
static uint8_t              s_mock_tariff_zone = 0;
static bool                 s_mock_ac_failed = false;
static button_dir_t         s_mock_last_button = BUTTON_DIR_DOWN;
static int                  s_mock_button_count = 0;

/* Mock Hook Implementations */
static const meter_measurements_t* mock_get_meas(void) {
    return &s_mock_meas;
}

static uint32_t mock_get_alarm(void) {
    return s_mock_alarm_mask;
}

static void mock_set_alarm(uint32_t mask) {
    s_mock_alarm_mask = mask;
}

static const tamper_context_t* mock_get_tamper(void) {
    return &s_mock_tamper_ctx;
}

static void mock_inject_tamper(tamper_vector_t vec, bool asserted) {
    tamper_process_signal(&s_mock_tamper_ctx, vec, asserted, 1727570400);
}

static relay_state_t mock_get_relay_state(void) {
    return s_mock_relay_state;
}

static relay_reason_t mock_get_relay_reason(void) {
    return s_mock_relay_reason;
}

static bool mock_relay_disconnect(relay_reason_t reason) {
    s_mock_relay_state = RELAY_STATE_DISCONNECTED;
    s_mock_relay_reason = reason;
    return true;
}

static bool mock_relay_request_reconnect(void) {
    if (s_mock_relay_state == RELAY_STATE_DISCONNECTED) {
        s_mock_relay_state = RELAY_STATE_READY_ON;
        return true;
    }
    return false;
}

static bool mock_relay_confirm_reconnect(void) {
    if (s_mock_relay_state == RELAY_STATE_READY_ON) {
        s_mock_relay_state = RELAY_STATE_CONNECTED;
        s_mock_relay_reason = RELAY_REASON_NONE;
        return true;
    }
    return false;
}

static const max_demand_tracker_t* mock_get_demand(void) {
    return &s_mock_demand_tracker;
}

static void mock_reset_demand(void) {
    max_demand_reset(&s_mock_demand_tracker);
}

static uint32_t mock_get_time(void) {
    return s_mock_time_epoch;
}

static void mock_set_time(uint32_t epoch_sec) {
    s_mock_time_epoch = epoch_sec;
}

static uint8_t mock_get_tariff(void) {
    return s_mock_tariff_zone;
}

static void mock_button_press(button_dir_t dir) {
    s_mock_last_button = dir;
    s_mock_button_count++;
}

static void mock_set_ac_fail(bool is_fail) {
    s_mock_ac_failed = is_fail;
}

static bool mock_is_ac_failed(void) {
    return s_mock_ac_failed;
}

static uint8_t mock_get_power_mode(void) {
    return (uint8_t)METER_POWER_MODE_RUN;
}

static uint32_t mock_get_power_wakeup_count(void) {
    return 3U;
}

static uint8_t mock_get_power_last_wakeup_reason(void) {
    return (uint8_t)METER_WAKEUP_REASON_BUTTON_PRESS;
}

#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
static synthetic_load_config_t s_mock_synth_cfg;

static void mock_set_synthetic_load(const synthetic_load_config_t *cfg) {
    if (cfg != NULL) {
        s_mock_synth_cfg = *cfg;
    }
}

static void mock_get_synthetic_load(synthetic_load_config_t *out_cfg) {
    if (out_cfg != NULL) {
        *out_cfg = s_mock_synth_cfg;
    }
}
#endif

/* Inisialisasi Mock Environment */
static void init_mock_env(void) {
    memset(&s_mock_meas, 0, sizeof(s_mock_meas));
    s_mock_meas.voltage_r_dvolts = 2301;
    s_mock_meas.voltage_s_dvolts = 2298;
    s_mock_meas.voltage_t_dvolts = 2312;
    s_mock_meas.current_r_mamps = 10500;
    s_mock_meas.current_s_mamps = 10450;
    s_mock_meas.current_t_mamps = 10600;
    s_mock_meas.current_n_mamps = 80;
    s_mock_meas.active_power_w = 7200;
    s_mock_meas.reactive_power_var = 450;
    s_mock_meas.apparent_power_va = 7250;
    s_mock_meas.power_factor_ppm = 995;
    s_mock_meas.frequency_mhz = 50000;

    s_mock_meas.active_energy_wh = 543210;
    s_mock_meas.active_energy_import_wh = 12345;
    s_mock_meas.reactive_energy_varh = 6789;
    s_mock_meas.active_energy_wbp_wh = 150000;
    s_mock_meas.active_energy_lwbp_wh = 393210;
    s_mock_meas.active_energy_lwbp2_wh = 0;

    s_mock_alarm_mask = 0;
    tamper_init(&s_mock_tamper_ctx);
    s_mock_relay_state = RELAY_STATE_CONNECTED;
    s_mock_relay_reason = RELAY_REASON_NONE;
    max_demand_init(&s_mock_demand_tracker, 900);
    max_demand_process_second(&s_mock_demand_tracker, 5500, 1727570400);

    /* Perekaman sampel log SPLN */
    spln_log_record_alarm(&s_mock_tamper_ctx, SPLN_LOG_ALARM_CLOCK_LOSS, 1, 1727570410, 0x1234);
    spln_log_record_event(&s_mock_tamper_ctx, SPLN_LOG_EVENT_POWER_FAILURE, 1, 1727570420, 0);
    spln_log_record_tamper(&s_mock_tamper_ctx, SPLN_LOG_TAMPER_CASE_OPEN, 1, 1727570430, 0);

    s_mock_ac_failed = false;
    s_mock_last_button = BUTTON_DIR_DOWN;
    s_mock_button_count = 0;

#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
    memset(&s_mock_synth_cfg, 0, sizeof(s_mock_synth_cfg));
    s_mock_synth_cfg.voltage_r_dv = 2200;
    s_mock_synth_cfg.voltage_s_dv = 2200;
    s_mock_synth_cfg.voltage_t_dv = 2200;
    s_mock_synth_cfg.current_r_ma = 15000;
    s_mock_synth_cfg.current_s_ma = 15000;
    s_mock_synth_cfg.current_t_ma = 15000;
    s_mock_synth_cfg.current_n_ma = 50;
    s_mock_synth_cfg.active_power_w = 9900;
    s_mock_synth_cfg.reactive_power_var = 1485;
    s_mock_synth_cfg.power_factor_ppm = 990;
    s_mock_synth_cfg.active = false;
#endif
}

void test_cli_core_commands(void) {
    printf("[TEST] Testing CLI Core Commands Execution...\n");
    init_mock_env();

    console_meter_hooks_t hooks = {
        .get_measurements = mock_get_meas,
        .get_internal_alarm = mock_get_alarm,
        .set_internal_alarm = mock_set_alarm,
        .get_tamper_context = mock_get_tamper,
        .inject_tamper = mock_inject_tamper,
        .get_relay_state = mock_get_relay_state,
        .get_relay_disconnect_reason = mock_get_relay_reason,
        .relay_disconnect = mock_relay_disconnect,
        .relay_request_reconnect = mock_relay_request_reconnect,
        .relay_confirm_reconnect = mock_relay_confirm_reconnect,
        .get_max_demand = mock_get_demand,
        .reset_max_demand = mock_reset_demand,
        .get_time = mock_get_time,
        .set_time = mock_set_time,
        .get_active_tariff = mock_get_tariff,
        .button_press = mock_button_press,
        .set_ac_fail = mock_set_ac_fail,
        .is_ac_failed = mock_is_ac_failed,
        .get_power_mode = mock_get_power_mode,
        .get_power_wakeup_count = mock_get_power_wakeup_count,
        .get_power_last_wakeup_reason = mock_get_power_last_wakeup_reason,
#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
        .set_synthetic_load = mock_set_synthetic_load,
        .get_synthetic_load = mock_get_synthetic_load
#endif
    };

    console_cli_init(&hooks);

    /* 1. Uji Help dan Alias '?' */
    char cmd_buf[128];
    strcpy(cmd_buf, "help");
    assert(console_cli_execute_command(cmd_buf) == 0);

    strcpy(cmd_buf, "?");
    assert(console_cli_execute_command(cmd_buf) == 0);

    /* 2. Uji Status General & Manajemen Daya */
    strcpy(cmd_buf, "status");
    assert(console_cli_execute_command(cmd_buf) == 0);

    strcpy(cmd_buf, "power");
    assert(console_cli_execute_command(cmd_buf) == 0);

    strcpy(cmd_buf, "pwr");
    assert(console_cli_execute_command(cmd_buf) == 0);

    /* 3. Uji Pengukuran Listrik & Telemetri */
    strcpy(cmd_buf, "meas");
    assert(console_cli_execute_command(cmd_buf) == 0);

    /* 4. Uji Akumulasi Energi & ToU */
    strcpy(cmd_buf, "energy");
    assert(console_cli_execute_command(cmd_buf) == 0);

    /* 5. Uji Siklus Kontrol Relai */
    strcpy(cmd_buf, "relay status");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_relay_state == RELAY_STATE_CONNECTED);

    strcpy(cmd_buf, "relay trip 2"); /* RELAY_REASON_POWER_LIMITER */
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_relay_state == RELAY_STATE_DISCONNECTED);
    assert(s_mock_relay_reason == RELAY_REASON_POWER_LIMITER);

    strcpy(cmd_buf, "relay reconnect");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_relay_state == RELAY_STATE_READY_ON);

    strcpy(cmd_buf, "relay confirm");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_relay_state == RELAY_STATE_CONNECTED);

    /* 6. Uji Alarm Internal Hardware */
    strcpy(cmd_buf, "alarm status");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_alarm_mask == 0);

    strcpy(cmd_buf, "alarm set 0x05"); /* ERR00 Flash + ERR02 RTC */
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_alarm_mask == 0x05);

    strcpy(cmd_buf, "alarm clear");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_alarm_mask == 0);

    /* 7. Uji Tamper Injeksi */
    strcpy(cmd_buf, "tamper status");
    assert(console_cli_execute_command(cmd_buf) == 0);

    strcpy(cmd_buf, "tamper test 4 1"); /* TAMPER_VECTOR_MAGNETIC_FIELD */
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert((tamper_get_active_mask(&s_mock_tamper_ctx) & TAMPER_VECTOR_MAGNETIC_FIELD) != 0);

    /* 8. Uji Pembacaan 3 Log FIFO SPLN */
    strcpy(cmd_buf, "log alarm");
    assert(console_cli_execute_command(cmd_buf) == 0);

    strcpy(cmd_buf, "log event");
    assert(console_cli_execute_command(cmd_buf) == 0);

    strcpy(cmd_buf, "log tamper");
    assert(console_cli_execute_command(cmd_buf) == 0);

    /* 9. Uji Maximum Demand */
    strcpy(cmd_buf, "demand status");
    assert(console_cli_execute_command(cmd_buf) == 0);

    strcpy(cmd_buf, "demand reset");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_demand_tracker.reset_count == 1);

    /* 10. Uji Mute & Echo */
    strcpy(cmd_buf, "mute 1");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(console_io_is_muted() == true);

    strcpy(cmd_buf, "mute 0");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(console_io_is_muted() == false);

    strcpy(cmd_buf, "echo ALLIN Smart Meter CLI Ready");
    assert(console_cli_execute_command(cmd_buf) == 0);

    /* 11. Uji Time & Set_Time (RTC) */
    strcpy(cmd_buf, "time");
    assert(console_cli_execute_command(cmd_buf) == 0);

    strcpy(cmd_buf, "set_time 1793184000");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_time_epoch == 1793184000);

    /* 12. Uji AC Fail & Button (Off-Power Standby) */
    strcpy(cmd_buf, "acfail");
    assert(console_cli_execute_command(cmd_buf) == 0);

    strcpy(cmd_buf, "acfail 1");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_ac_failed == true);

    strcpy(cmd_buf, "button up");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_last_button == BUTTON_DIR_UP);
    assert(s_mock_button_count == 1);

    strcpy(cmd_buf, "button down");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_last_button == BUTTON_DIR_DOWN);
    assert(s_mock_button_count == 2);

    strcpy(cmd_buf, "button both");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_last_button == BUTTON_DIR_BOTH);
    assert(s_mock_button_count == 3);

    strcpy(cmd_buf, "acfail 0");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_ac_failed == false);

#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
    /* 13. Uji Perintah Synthetic Load (load, volt, curr, synth) */
    strcpy(cmd_buf, "synth status");
    assert(console_cli_execute_command(cmd_buf) == 0);

    strcpy(cmd_buf, "load 5000 500 990");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_synth_cfg.active == true);
    assert(s_mock_synth_cfg.active_power_w == 5000);
    assert(s_mock_synth_cfg.reactive_power_var == 500);
    assert(s_mock_synth_cfg.power_factor_ppm == 990);

    strcpy(cmd_buf, "volt 230");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_synth_cfg.voltage_r_dv == 2300);
    assert(s_mock_synth_cfg.voltage_s_dv == 2300);
    assert(s_mock_synth_cfg.voltage_t_dv == 2300);

    strcpy(cmd_buf, "curr 12000 12000 12000 50");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_synth_cfg.current_r_ma == 12000);
    assert(s_mock_synth_cfg.current_s_ma == 12000);
    assert(s_mock_synth_cfg.current_t_ma == 12000);
    assert(s_mock_synth_cfg.current_n_ma == 50);

    strcpy(cmd_buf, "synth off");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_synth_cfg.active == false);

    strcpy(cmd_buf, "synth on");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_synth_cfg.active == true);

    strcpy(cmd_buf, "synth reset");
    assert(console_cli_execute_command(cmd_buf) == 0);
    assert(s_mock_synth_cfg.active_power_w == 9900);
#endif

    /* 14. Uji Perintah Invalid dan Baris Kosong */
    strcpy(cmd_buf, "perintah_ngawur_xyz");
    assert(console_cli_execute_command(cmd_buf) == -1);

    strcpy(cmd_buf, "   ");
    assert(console_cli_execute_command(cmd_buf) == 0);

    printf("   [PASS] CLI Core Commands Execution OK!\n\n");
}

void test_cli_rx_stream_and_editing(void) {
    printf("[TEST] Testing Interactive RX Stream Parsing & Line Editing (Backspace)...\n");
    init_mock_env();

    console_meter_hooks_t hooks = {
        .get_measurements = mock_get_meas,
        .get_internal_alarm = mock_get_alarm,
        .set_internal_alarm = mock_set_alarm,
        .get_tamper_context = mock_get_tamper,
        .inject_tamper = mock_inject_tamper,
        .get_relay_state = mock_get_relay_state,
        .get_relay_disconnect_reason = mock_get_relay_reason,
        .relay_disconnect = mock_relay_disconnect,
        .relay_request_reconnect = mock_relay_request_reconnect,
        .relay_confirm_reconnect = mock_relay_confirm_reconnect,
        .get_max_demand = mock_get_demand,
        .reset_max_demand = mock_reset_demand,
        .get_time = mock_get_time,
        .set_time = mock_set_time,
        .get_active_tariff = mock_get_tariff,
        .button_press = mock_button_press,
        .set_ac_fail = mock_set_ac_fail,
        .is_ac_failed = mock_is_ac_failed,
        .get_power_mode = mock_get_power_mode,
        .get_power_wakeup_count = mock_get_power_wakeup_count,
        .get_power_last_wakeup_reason = mock_get_power_last_wakeup_reason,
#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
        .set_synthetic_load = mock_set_synthetic_load,
        .get_synthetic_load = mock_get_synthetic_load
#endif
    };

    console_cli_init(&hooks);

    /* 1. Uji kirim baris lengkap via stream buffer RX */
    const char *line1 = "alarm set 0x02\r\n";
    console_cli_process_rx_buffer((const uint8_t *)line1, strlen(line1));
    assert(s_mock_alarm_mask == 0x02);

    /* 2. Uji line editing dengan Backspace: ketik "alarm claer" -> backspace 3x -> "ear" -> "alarm clear" */
    const uint8_t edit_stream[] = {
        'a', 'l', 'a', 'r', 'm', ' ', 'c', 'l', 'a', 'e', 'r',
        '\b', '\b', '\b',
        'e', 'a', 'r', '\r'
    };
    console_cli_process_rx_buffer(edit_stream, sizeof(edit_stream));
    assert(s_mock_alarm_mask == 0x00);

    printf("   [PASS] Interactive RX Stream Parsing & Line Editing Verified OK!\n\n");
}

int main(void) {
    printf("====================================================\n");
    printf(" RUNNING INTERACTIVE SERIAL CLI UNIT TESTS (E3/CLI) \n");
    printf("====================================================\n\n");

    test_cli_core_commands();
    test_cli_rx_stream_and_editing();

    printf("====================================================\n");
    printf(" ALL INTERACTIVE SERIAL CLI TESTS PASSED 100%%!      \n");
    printf("====================================================\n");
    return 0;
}
