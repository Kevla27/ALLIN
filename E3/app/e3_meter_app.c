/**
 * @file e3_meter_app.c
 * @brief Implementasi Layanan Terpadu Pilar E3: HMI, Tamper, Relai Pembatas, ToU 8-Zona, Telemetri & Profiling
 */

#include "e3_meter_app.h"
#include "bsp_alarm_buzzer.h"
#include "nvram_storage.h"
#include "e2_dlms_service.h"
#include "console_io.h"
#include <stdio.h>
#include <string.h>

#if defined(EMBEDDED_HARDWARE_TARGET)
#include "secure_nsc.h"
#include "bsp_display_oled.h"
#endif

static display_context_t      s_disp_ctx;
static tamper_context_t       s_tamper_ctx;
static power_relay_context_t  s_relay_ctx;
static load_profile_ctx_t     s_lp_ctx;
static max_demand_tracker_t   s_md_tracker;
static billing_snapshot_ctx_t s_billing_ctx;
static ADC_HandleTypeDef     *s_hadc_telemetry = NULL;

static uint32_t s_now_epoch_sec         = 1791794500U;
static uint32_t s_ms_accumulator        = 0U;
static uint32_t s_last_lp_record_sec    = 0U;
static uint32_t s_last_disp_print_ms    = 0U;
static uint32_t s_last_telemetry_ms     = 0U;
static uint32_t s_vbat_millivolts       = 3600U;
static uint32_t s_vcap_millivolts       = 3300U;
static uint32_t s_internal_alarm_mask   = 0U; /* Bitmask spln_internal_alarm_t (SPLN D3.006 Tabel 4) */

/* Status Pemadaman PLN (AC Fail) & Timer Wake-Up Layar 10 Detik (SPLN D3.006 & Desain Sistem Hal. 37) */
static volatile bool     s_is_ac_failed         = false;
static volatile uint32_t s_lcd_wakeup_timer_ms  = 0U;
static volatile uint32_t s_button_beep_ms         = 0U;
static uint64_t          s_last_active_energy_wh = 0ULL;

/* Akumulator Energi Time-of-Use (ToU) 8 Zona Tarif (SP-13 & Desain Sistem Hal. 30) */
static uint64_t s_tou_tariff_wh[E3_TOU_MAX_ZONES] = {0};
static uint8_t  s_active_tariff_zone = 1U; /* Default: Zona 1 (LWBP) */

/* Counter Zero-Crossing (PE8) & Status Kontrol Bantu (PD10/PD14 -> PD8) */
static volatile uint32_t s_zero_crossing_count  = 0U;
static volatile uint8_t  s_aux_input_state_mask = 0U;

static uint8_t evaluate_tou_tariff_zone(uint32_t epoch_sec)
{
    uint32_t hour_of_day = (epoch_sec / 3600U) % 24U;
    /* Sesuai SPLN D3.006: WBP = 17:00 s.d 22:00 (Zona 0), LWBP1 = 22:00 s.d 06:00 (Zona 1), LWBP2 = 06:00 s.d 17:00 (Zona 2) */
    if (hour_of_day >= 17U && hour_of_day < 22U)
    {
        return 0U; /* Tarif 1: WBP */
    }
    else if (hour_of_day >= 22U || hour_of_day < 6U)
    {
        return 1U; /* Tarif 2: LWBP Malam */
    }
    return 2U;     /* Tarif 3: LWBP Siang */
}

static void sample_battery_and_supercap(uint32_t *out_vbat_mv, uint32_t *out_vcap_mv)
{
    if (s_hadc_telemetry == NULL || !out_vbat_mv || !out_vcap_mv) return;

    /* Channel 3: PC2 (VCAP_SENSE) */
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_3;
    sConfig.Rank = ADC4_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC4_SAMPLINGTIME_COMMON_1;
    HAL_ADC_ConfigChannel(s_hadc_telemetry, &sConfig);

    HAL_ADC_Start(s_hadc_telemetry);
    if (HAL_ADC_PollForConversion(s_hadc_telemetry, 5) == HAL_OK)
    {
        uint32_t raw_vcap = HAL_ADC_GetValue(s_hadc_telemetry);
        *out_vcap_mv = (raw_vcap * 3300U) / 4095U;
    }
    HAL_ADC_Stop(s_hadc_telemetry);

    /* Channel 15: PD11 (VBAT_SENSE) */
    sConfig.Channel = ADC_CHANNEL_15;
    HAL_ADC_ConfigChannel(s_hadc_telemetry, &sConfig);

    HAL_ADC_Start(s_hadc_telemetry);
    if (HAL_ADC_PollForConversion(s_hadc_telemetry, 5) == HAL_OK)
    {
        uint32_t raw_vbat = HAL_ADC_GetValue(s_hadc_telemetry);
        *out_vbat_mv = (raw_vbat * 3300U) / 4095U;
    }
    HAL_ADC_Stop(s_hadc_telemetry);
}

/* --- Implementasi Hook Pelayanan Konsol CLI Interaktif --- */
static const meter_measurements_t* cli_get_measurements(void) {
    return &s_disp_ctx.meas_buffer;
}

static uint32_t cli_get_internal_alarm(void) {
    return s_internal_alarm_mask;
}

static void cli_set_internal_alarm(uint32_t mask) {
    if (mask == 0) {
        s_internal_alarm_mask = 0;
        display_set_internal_alarm(&s_disp_ctx, 0);
    } else {
        e3_meter_app_set_internal_alarm(mask);
    }
}

static const tamper_context_t* cli_get_tamper_context(void) {
    return &s_tamper_ctx;
}

static void cli_inject_tamper(tamper_vector_t vec, bool asserted) {
    tamper_process_signal(&s_tamper_ctx, vec, asserted, s_now_epoch_sec);
}

static relay_state_t cli_get_relay_state(void) {
    return power_relay_get_state(&s_relay_ctx);
}

static relay_reason_t cli_get_relay_disconnect_reason(void) {
    return s_relay_ctx.last_reason;
}

static bool cli_relay_disconnect(relay_reason_t reason) {
    power_relay_disconnect(&s_relay_ctx, reason, s_now_epoch_sec);
    return true;
}

static bool cli_relay_request_reconnect(void) {
    power_relay_request_reconnect(&s_relay_ctx, s_now_epoch_sec);
    display_set_relay_ready_on(&s_disp_ctx, true);
    return true;
}

static bool cli_relay_confirm_reconnect(void) {
    bool ok = power_relay_confirm_reconnect(&s_relay_ctx, s_now_epoch_sec);
    if (ok) {
        display_set_relay_ready_on(&s_disp_ctx, false);
    }
    return ok;
}

static const max_demand_tracker_t* cli_get_max_demand(void) {
    return &s_md_tracker;
}

static void cli_reset_max_demand(void) {
    max_demand_reset(&s_md_tracker);
}

static uint32_t cli_get_time(void) {
    return e3_meter_app_get_epoch();
}

static void cli_set_time(uint32_t epoch_sec) {
    e3_meter_app_set_epoch(epoch_sec);
}

static uint8_t cli_get_active_tariff(void) {
    return e3_meter_app_get_active_tariff_zone();
}

void e3_meter_app_init(const char *customer_id, ADC_HandleTypeDef *hadc_telemetry)
{
    s_hadc_telemetry = hadc_telemetry;

    /* 0. Kalibrasi Hardware ADC4 jika tersedia */
    if (s_hadc_telemetry != NULL)
    {
        HAL_ADCEx_Calibration_Start(s_hadc_telemetry, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);
    }

    /* 1. Inisialisasi Power Rail Periferal Aktif & Chip Select SPI */
    HAL_GPIO_WritePin(LCD_PWR_EN_GPIO_Port, LCD_PWR_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(RS232_PWR_EN_GPIO_Port, RS232_PWR_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(MODEM_12V_EN_GPIO_Port, MODEM_12V_EN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);

    /* 2. Inisialisasi Modul-Modul Inti Pilar E3 */
    s_internal_alarm_mask = 0U;
    display_init(&s_disp_ctx, customer_id ? customer_id : "54321678901", 3000);
#if defined(EMBEDDED_HARDWARE_TARGET)
    const display_driver_interface_t *oled_drv = bsp_display_oled_get_driver();
    if (oled_drv != NULL && oled_drv->init != NULL)
    {
        oled_drv->init();
        if (oled_drv->show_boot_screen != NULL)
        {
            oled_drv->show_boot_screen("ALLIN SMART METER", "SPLN D3.006:2021");
        }
    }
#endif
    tamper_init(&s_tamper_ctx);
    (void)nvram_load_tamper_log_snapshot(&s_tamper_ctx);
    power_relay_init(&s_relay_ctx, 11000, 5, 30);
    load_profile_init(&s_lp_ctx);
    max_demand_init(&s_md_tracker, LOAD_PROFILE_INTERVAL_SEC);
    billing_snapshot_init(&s_billing_ctx);
    bsp_alarm_buzzer_init();

    /* 3. Inisialisasi & Pendaftaran Hooks Interactive Serial CLI */
    console_meter_hooks_t cli_hooks = {
        .get_measurements = cli_get_measurements,
        .get_internal_alarm = cli_get_internal_alarm,
        .set_internal_alarm = cli_set_internal_alarm,
        .get_tamper_context = cli_get_tamper_context,
        .inject_tamper = cli_inject_tamper,
        .get_relay_state = cli_get_relay_state,
        .get_relay_disconnect_reason = cli_get_relay_disconnect_reason,
        .relay_disconnect = cli_relay_disconnect,
        .relay_request_reconnect = cli_relay_request_reconnect,
        .relay_confirm_reconnect = cli_relay_confirm_reconnect,
        .get_max_demand = cli_get_max_demand,
        .reset_max_demand = cli_reset_max_demand,
        .get_time = cli_get_time,
        .set_time = cli_set_time,
        .get_active_tariff = cli_get_active_tariff
    };
    console_cli_init(&cli_hooks);

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t rtc_epoch = SECURE_GetRTCDateTime();
    if (rtc_epoch > 1700000000U)
    {
        s_now_epoch_sec = rtc_epoch;
    }
#endif

    s_active_tariff_zone = evaluate_tou_tariff_zone(s_now_epoch_sec);
    s_last_lp_record_sec = s_now_epoch_sec;
    s_last_disp_print_ms = HAL_GetTick();
}

void e3_meter_app_step(const meter_measurements_t *meas, uint32_t delta_ms)
{
    if (meas == NULL) return;

    uint32_t current_tick_ms = HAL_GetTick();

    /* 0. Heartbeat External Watchdog (PD12) */
    HAL_GPIO_TogglePin(EXT_WDI_PULSE_GPIO_Port, EXT_WDI_PULSE_Pin);

    /* 0b. Manajemen Timer Wake-Up Layar 10 Detik saat Listrik PLN Padam (Desain Sistem Hal. 37) */
    if (s_is_ac_failed && s_lcd_wakeup_timer_ms > 0U)
    {
        if (delta_ms >= s_lcd_wakeup_timer_ms)
        {
            s_lcd_wakeup_timer_ms = 0U;
            HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LCD_PWR_EN_GPIO_Port, LCD_PWR_EN_Pin, GPIO_PIN_RESET);
        }
        else
        {
            s_lcd_wakeup_timer_ms -= delta_ms;
        }
    }

    /* 1. Akumulasi detik epoch & distribusi energi Time-of-Use (ToU) 8-Zona */
    if (meas->active_energy_wh > s_last_active_energy_wh)
    {
        uint64_t delta_wh = meas->active_energy_wh - s_last_active_energy_wh;
        s_last_active_energy_wh = meas->active_energy_wh;
        s_active_tariff_zone = evaluate_tou_tariff_zone(s_now_epoch_sec);
        if (s_active_tariff_zone < E3_TOU_MAX_ZONES)
        {
            s_tou_tariff_wh[s_active_tariff_zone] += delta_wh;
        }
    }

    s_ms_accumulator += delta_ms;
    bool is_one_sec_tick = false;
    if (s_ms_accumulator >= 1000U)
    {
        uint32_t elapsed_sec = s_ms_accumulator / 1000U;
        s_ms_accumulator %= 1000U;
        is_one_sec_tick = true;

#if defined(EMBEDDED_HARDWARE_TARGET)
        /* Sinkronisasi presisi waktu langsung dari peripheral Secure Hardware RTC STM32U5 */
        uint32_t rtc_now = SECURE_GetRTCDateTime();
        if (rtc_now > 1700000000U)
        {
            s_now_epoch_sec = rtc_now;
        }
        else
        {
            s_now_epoch_sec += elapsed_sec;
        }
#else
        s_now_epoch_sec += elapsed_sec;
#endif

        s_active_tariff_zone = evaluate_tou_tariff_zone(s_now_epoch_sec);
        display_set_time(&s_disp_ctx, s_now_epoch_sec);

        /* Pemicu Otomatis End-of-Billing (EoB) Tanggal 1 Pukul 10:00:00 (SPLN Butir 12.4 Hal. 34) */
        uint32_t days_since_epoch = s_now_epoch_sec / 86400U;
        uint32_t sec_of_day = s_now_epoch_sec % 86400U;
        if ((days_since_epoch % 30U) == 0U && sec_of_day == 36000U)
        {
            e3_meter_app_trigger_billing_snapshot();
        }
    }

    /* 2. Periksa Input Tamper Hardware (PC13, PE6, PE4) & Latch TrustZone Secure TAMP */
    bool tamp_case = (HAL_GPIO_ReadPin(TAMP_CASE_OPEN_GPIO_Port, TAMP_CASE_OPEN_Pin) == (BOARD_CFG_TAMPER_CASE_OPEN_LVL ? GPIO_PIN_SET : GPIO_PIN_RESET));
    bool tamp_term = (HAL_GPIO_ReadPin(TAMP_TERM_COVER_GPIO_Port, TAMP_TERM_COVER_Pin) == (BOARD_CFG_TAMPER_TERM_OPEN_LVL ? GPIO_PIN_SET : GPIO_PIN_RESET));
    bool tamp_mag  = (HAL_GPIO_ReadPin(SENSOR_MAGNET_GPIO_Port, SENSOR_MAGNET_Pin) == (BOARD_CFG_TAMPER_MAGNET_LVL ? GPIO_PIN_SET : GPIO_PIN_RESET));

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t sec_tamp_mask = SECURE_GetTamperStatusMask();
    if (sec_tamp_mask & 0x01U) tamp_case = true;
    if (sec_tamp_mask & 0x02U) tamp_term = true;
    if (sec_tamp_mask & 0x04U) tamp_mag  = true;
#endif

    if (tamp_case) tamper_process_signal(&s_tamper_ctx, TAMPER_VECTOR_CASE_OPEN, true, s_now_epoch_sec);
    if (tamp_term) tamper_process_signal(&s_tamper_ctx, TAMPER_VECTOR_TERMINAL_OPEN, true, s_now_epoch_sec);
    if (tamp_mag)  tamper_process_signal(&s_tamper_ctx, TAMPER_VECTOR_MAGNETIC_FIELD, true, s_now_epoch_sec);

    /* Deteksi Hilang Tegangan 1 atau 2 Fasa (ERR24 - SPLN Tabel 6 No 6) saat PLN aktif */
    if (!s_is_ac_failed)
    {
        uint8_t low_v_phases = 0U;
        if (meas->voltage_r_dvolts < 1100U) low_v_phases++;
        if (meas->voltage_s_dvolts < 1100U) low_v_phases++;
        if (meas->voltage_t_dvolts < 1100U) low_v_phases++;
        if (low_v_phases == 1U || low_v_phases == 2U)
        {
            tamper_process_signal(&s_tamper_ctx, TAMPER_VECTOR_PHASE_LOSS, true, s_now_epoch_sec);
        }

        /* Deteksi Kawat Netral Hilang / Putus / Bypass (ERR23 - SPLN Tabel 6 No 5) */
        bool phase_loaded = (meas->current_r_mamps > 500U || meas->current_s_mamps > 500U || meas->current_t_mamps > 500U);
        int32_t diff_rs = (int32_t)meas->current_r_mamps - (int32_t)meas->current_s_mamps;
        if (diff_rs < 0) diff_rs = -diff_rs;
        int32_t diff_st = (int32_t)meas->current_s_mamps - (int32_t)meas->current_t_mamps;
        if (diff_st < 0) diff_st = -diff_st;

        if (phase_loaded && (diff_rs > 400 || diff_st > 400) && meas->current_n_mamps < 50U)
        {
            tamper_process_signal(&s_tamper_ctx, TAMPER_VECTOR_NEUTRAL_BYPASS, true, s_now_epoch_sec);
        }
        else if (meas->current_n_mamps > (meas->current_r_mamps + meas->current_s_mamps + meas->current_t_mamps + 1000U) && meas->current_n_mamps > 1500U)
        {
            tamper_process_signal(&s_tamper_ctx, TAMPER_VECTOR_NEUTRAL_BYPASS, true, s_now_epoch_sec);
        }

        /* Deteksi Urutan Fase Tegangan Tidak Sesuai / Wrong Phase Sequence (ERR21 - SPLN Tabel 6 No 3) */
        if (meas->angle_us_deg > 0U && meas->angle_ut_deg > 0U && meas->angle_us_deg > meas->angle_ut_deg)
        {
            tamper_process_signal(&s_tamper_ctx, TAMPER_VECTOR_WRONG_SEQUENCE, true, s_now_epoch_sec);
        }

        /* Deteksi Pengawatan Terbalik / Arus & Tegangan Tidak Sefasa (ERR22 - SPLN Tabel 6 No 4) */
        if (phase_loaded && (meas->angle_ir_deg > 90U && meas->angle_ir_deg < 270U) && meas->active_power_w > 50)
        {
            tamper_process_signal(&s_tamper_ctx, TAMPER_VECTOR_CROSS_PHASE, true, s_now_epoch_sec);
        }
    }

    /* Deteksi Reverse Power (ERR26 - SPLN Tabel 6 No 8) */
    if (meas->active_power_w < -50)
    {
        tamper_process_signal(&s_tamper_ctx, TAMPER_VECTOR_REVERSE_POWER, true, s_now_epoch_sec);
    }

    /* Deteksi Pengukuran Kuadran 4 dengan Faktor Daya < 0,85 (ERR27 - SPLN Tabel 6 No 9) */
    if (meas->active_power_w > 50 && meas->reactive_power_var < -50 &&
        meas->power_factor_ppm > 0U && meas->power_factor_ppm < 850U)
    {
        tamper_process_signal(&s_tamper_ctx, TAMPER_VECTOR_LOW_PF_QUAD4, true, s_now_epoch_sec);
    }

    display_set_tamper_status(&s_disp_ctx, tamper_get_active_mask(&s_tamper_ctx));

    /* 3. Periksa Ketidakseimbangan Fasa (Asymmetry) */
    bool is_asym = power_relay_check_asymmetry(&s_relay_ctx, meas->current_r_mamps, meas->current_s_mamps, meas->current_t_mamps);
    display_set_asymmetry_warning(&s_disp_ctx, is_asym);
    if (is_asym && is_one_sec_tick)
    {
        spln_log_record_event(&s_tamper_ctx, SPLN_LOG_EVENT_ASYMMETRIC_POWER, true, s_now_epoch_sec, s_relay_ctx.current_deviation_pct);
    }

    /* 4. Telemetri Tegangan Baterai (PD11), Superkapasitor (PC2) & Kesehatan LSE Clock (ERR02/ERR03/ERR06) */
    if (current_tick_ms - s_last_telemetry_ms >= 5000U)
    {
        s_last_telemetry_ms = current_tick_ms;
        sample_battery_and_supercap(&s_vbat_millivolts, &s_vcap_millivolts);

        uint32_t telemetry_alarms = 0U;
        if (s_vbat_millivolts < 2700U && s_vbat_millivolts > 500U)
        {
            telemetry_alarms |= SPLN_ALARM_LOW_BATTERY; /* ERR03: Low Battery */
        }
        if (s_vcap_millivolts < 2000U && s_vcap_millivolts > 100U)
        {
            telemetry_alarms |= SPLN_ALARM_SUPERCAP_FAIL; /* ERR06: Supercap anomaly */
        }
#if defined(EMBEDDED_HARDWARE_TARGET)
        if (SECURE_IsLseClockHealthy() == 0U)
        {
            telemetry_alarms |= SPLN_ALARM_RTC_ERROR; /* ERR02: LSE 32.768 kHz failure */
        }
#endif
        if (telemetry_alarms != 0U)
        {
            e3_meter_app_set_internal_alarm(telemetry_alarms);
        }
    }

    /* 5. Jalankan State Machine Pembatas Beban & Relai (PC8) serta Tracker Maximum Demand (MD) */
    bool relay_closed_fb = (HAL_GPIO_ReadPin(RELAY_STATUS_GPIO_Port, RELAY_STATUS_Pin) == (BOARD_CFG_RELAY_FB_CLOSED_LVL ? GPIO_PIN_SET : GPIO_PIN_RESET));
    power_relay_process_voltage(&s_relay_ctx, meas->voltage_r_dvolts, meas->voltage_s_dvolts, meas->voltage_t_dvolts, s_now_epoch_sec);
    if (is_one_sec_tick)
    {
        bool p_tripped = power_relay_process_power(&s_relay_ctx, (int32_t)meas->active_power_w, s_now_epoch_sec);
        if (p_tripped)
        {
            spln_log_record_event(&s_tamper_ctx, SPLN_LOG_EVENT_POWER_LIMIT_EXC, true, s_now_epoch_sec, (uint32_t)meas->active_power_w);
            spln_log_record_event(&s_tamper_ctx, SPLN_LOG_EVENT_RELAY_CHANGE, false, s_now_epoch_sec, RELAY_REASON_POWER_LIMITER);
        }
        max_demand_process_second(&s_md_tracker, (int32_t)meas->active_power_w, s_now_epoch_sec);
    }

    /* Sinkronisasi status relai READY ON ke HMI Display (Halaman 17 SPLN Gambar 4) */
    bool is_ready_on = power_relay_is_ready_on(&s_relay_ctx);
    if (is_ready_on && !s_disp_ctx.relay_ready_on)
    {
        spln_log_record_event(&s_tamper_ctx, SPLN_LOG_EVENT_READY_RECONNECT, true, s_now_epoch_sec, 0);
    }
    display_set_relay_ready_on(&s_disp_ctx, is_ready_on);

    /* Sesuai SPLN D3.006: 2021:
     * 1. Relai membuka (TRIP) jika:
     *    a. Overload / FSM Disconnected (power_relay_get_state(&s_relay_ctx) == RELAY_STATE_DISCONNECTED)
     *    b. Sabotase/Tamper yang berhak trip (Tabel 6: Case Open, Terminal Open, Urutan Salah, Cross Phase, Hilang Fasa)
     *    c. Alarm Internal Kritis Hardware (Tabel 4: ERR00 Flash, ERR01 RAM, ERR02 Clock, ERR04 MCU, ERR05 ADC)
     * 2. LED Kuning aktif jika:
     *    a. Overload / FSM Disconnected
     *    b. Sabotase/Tamper yang menyalakan LED (Tabel 6: Terminal Open, Urutan Salah, Cross Phase, Netral Putus, Hilang Fasa, Reverse Power, Quad 4)
     *    c. Alarm Internal Hardware aktif (Tabel 4: ERR00 s.d ERR07) */
    bool is_hardware_alarm_trip = spln_alarm_should_trip_relay(s_internal_alarm_mask);
    bool is_hardware_alarm_led  = spln_alarm_should_turn_on_led(s_internal_alarm_mask);

    bool is_trip_commanded = (power_relay_get_state(&s_relay_ctx) == RELAY_STATE_DISCONNECTED ||
                              tamper_should_trip_relay(&s_tamper_ctx) ||
                              is_hardware_alarm_trip);
    bool is_yellow_led_on  = tamper_should_turn_on_led(&s_tamper_ctx) ||
                             (power_relay_get_state(&s_relay_ctx) == RELAY_STATE_DISCONNECTED) ||
                             is_hardware_alarm_led;

    if (is_trip_commanded)
    {
        HAL_GPIO_WritePin(RELAY_TRIP_GPIO_Port, RELAY_TRIP_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_ALM_GPIO_Port, LED_ALM_Pin, is_yellow_led_on ? GPIO_PIN_SET : GPIO_PIN_RESET);

        /* Validasi Feedback Relai: Jika sudah di-trip tapi kontak fisik masih terhubung -> Alarm ERR07 */
        if (relay_closed_fb)
        {
            s_internal_alarm_mask |= SPLN_ALARM_RELAY_FAIL;
            display_set_internal_alarm(&s_disp_ctx, s_internal_alarm_mask);
        }
    }
    else if (is_ready_on)
    {
        /* Sesuai Desain Sistem Hal. 10 & 36: Saat READY ON, LED_ALM (Kuning) berkedip periodik 1 Hz */
        HAL_GPIO_WritePin(RELAY_TRIP_GPIO_Port, RELAY_TRIP_Pin, GPIO_PIN_RESET);
        GPIO_PinState blink_state = ((current_tick_ms / 500U) % 2U == 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET;
        HAL_GPIO_WritePin(LED_ALM_GPIO_Port, LED_ALM_Pin, blink_state);
    }
    else
    {
        HAL_GPIO_WritePin(RELAY_TRIP_GPIO_Port, RELAY_TRIP_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_ALM_GPIO_Port, LED_ALM_Pin, is_yellow_led_on ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    /* Non-blocking buzzer button beep timer */
    bool beep_override = false;
    if (s_button_beep_ms > 0U)
    {
        beep_override = true;
        if (delta_ms >= s_button_beep_ms)
        {
            s_button_beep_ms = 0U;
        }
        else
        {
            s_button_beep_ms -= delta_ms;
        }
    }

    /* Audio Cadence Buzzer PB7 (SPLN D3.006: 2021 Butir 6.14):
     * - Override penekanan tombol: Bunyi Beep 50 ms
     * - Alarm sabotase / Trip aktif: Kadensi berdenyut 200 ms ON / 200 ms OFF
     * - Kondisi normal: Buzzer MATI */
    bsp_alarm_buzzer_update_cadence(is_trip_commanded, beep_override, delta_ms);

    /* 6. Perbarui Tampilan Carousel UI (SPLN Gambar 4) */
    meter_measurements_t disp_meas = *meas;
    disp_meas.active_energy_wbp_wh = s_tou_tariff_wh[0];
    disp_meas.active_energy_lwbp_wh = s_tou_tariff_wh[1];
    disp_meas.active_energy_lwbp2_wh = s_tou_tariff_wh[2];
    disp_meas.active_energy_import_wh = meas->active_energy_import_wh;
    disp_meas.max_demand_w = s_md_tracker.max_demand_w;
    disp_meas.max_demand_timestamp = s_md_tracker.max_demand_timestamp;
    disp_meas.cumulative_max_demand_w = s_md_tracker.cumulative_max_demand_w;
    disp_meas.max_demand_reset_count = s_md_tracker.reset_count;

    /* Ambil stand penagihan bulan lalu jika tersedia */
    billing_snapshot_entry_t last_bill;
    if (billing_snapshot_get_count(&s_billing_ctx) > 0 &&
        billing_snapshot_get_entry(&s_billing_ctx, billing_snapshot_get_count(&s_billing_ctx) - 1, &last_bill)) {
        disp_meas.billing_last_month_wh = last_bill.active_energy_import_wh;
    } else {
        disp_meas.billing_last_month_wh = 0;
    }

    display_update_measurements(&s_disp_ctx, &disp_meas);
    display_process_tick(&s_disp_ctx, delta_ms);

    if (current_tick_ms - s_last_disp_print_ms >= 3000U)
    {
        s_last_disp_print_ms = current_tick_ms;
        char line1[32], line2[32], line3[32];
        display_render_frame(&s_disp_ctx, line1, line2, line3, sizeof(line1));
        printf("[RTOS Task - HMI Page %d]\n  L1: [%s]\n  L2: [%s]\n  L3: [%s]\n",
               s_disp_ctx.current_page, line1, line2, line3);

#if defined(EMBEDDED_HARDWARE_TARGET)
        display_spln_frame_t spln_frame;
        display_render_spln_frame(&s_disp_ctx, &spln_frame);
        const display_driver_interface_t *oled_drv = bsp_display_oled_get_driver();
        if (oled_drv != NULL && oled_drv->render_frame != NULL)
        {
            oled_drv->render_frame(&spln_frame);
        }
#endif
    }

    /* 7. Rekam 15-Minute Load Profile ke NVRAM (SPI2 External Flash + RAM Mirror) */
    if (s_now_epoch_sec - s_last_lp_record_sec >= LOAD_PROFILE_INTERVAL_SEC)
    {
        s_last_lp_record_sec = s_now_epoch_sec;
        load_profile_entry_t lp_entry = {0};
        lp_entry.timestamp = s_now_epoch_sec;
        lp_entry.voltage_r_dvolts = (uint16_t)meas->voltage_r_dvolts;
        lp_entry.voltage_s_dvolts = (uint16_t)meas->voltage_s_dvolts;
        lp_entry.voltage_t_dvolts = (uint16_t)meas->voltage_t_dvolts;
        lp_entry.current_r_mamps = meas->current_r_mamps;
        lp_entry.current_s_mamps = meas->current_s_mamps;
        lp_entry.current_t_mamps = meas->current_t_mamps;
        lp_entry.current_n_mamps = meas->current_n_mamps;
        lp_entry.active_power_w = (int32_t)meas->active_power_w;
        lp_entry.reactive_power_var = (int32_t)meas->reactive_power_var;
        lp_entry.power_factor_permille = meas->power_factor_ppm;
        lp_entry.active_energy_import_wh = meas->active_energy_wh;
        lp_entry.active_energy_export_wh = meas->active_energy_import_wh; /* Stand Ekspor/Terima PLTS */
        lp_entry.reactive_energy_import_varh = meas->reactive_energy_varh;
        lp_entry.status_mask = tamper_get_active_mask(&s_tamper_ctx);
        load_profile_add_entry(&s_lp_ctx, &lp_entry);
        (void)nvram_write_load_profile_entry((uint32_t)(load_profile_get_count(&s_lp_ctx) - 1U), &lp_entry);
        printf("[PROFILING] 15-min Load Profile entry saved. Total entries: %u\n",
               (unsigned int)load_profile_get_count(&s_lp_ctx));
    }
}

void e3_meter_app_on_button(button_dir_t dir)
{
    /* 1. Jika PLN sedang padam (Off-Power), penekanan tombol menyalakan layar LCD selama 10 detik (Desain Sistem Hal. 37) */
    if (s_is_ac_failed)
    {
        HAL_GPIO_WritePin(LCD_PWR_EN_GPIO_Port, LCD_PWR_EN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);
        s_lcd_wakeup_timer_ms = BOARD_CFG_DISPLAY_WAKE_MS;
    }

    /* 2. Sesuai SPLN Tabel 5 No 12: Jika relai dalam status READY ON,
     * konfirmasi penyambungan kembali (reconnect) HANYA terjadi bila
     * tombol Atas dan Bawah ditekan secara bersamaan (BUTTON_DIR_BOTH). */
    if (power_relay_is_ready_on(&s_relay_ctx))
    {
        if (dir == BUTTON_DIR_BOTH)
        {
            power_relay_confirm_reconnect(&s_relay_ctx, s_now_epoch_sec);
            display_set_relay_ready_on(&s_disp_ctx, false);
            HAL_GPIO_WritePin(RELAY_TRIP_GPIO_Port, RELAY_TRIP_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_ALM_GPIO_Port, LED_ALM_Pin, GPIO_PIN_RESET);
        }
    }

    display_handle_button_press(&s_disp_ctx, dir);
    s_last_disp_print_ms = 0; /* Refresh tampilan seketika saat tombol ditekan */
    if (!s_is_ac_failed)
    {
        s_button_beep_ms = 50U;   /* Feedback beep 50ms (hanya saat suplai daya aktif) */
        bsp_alarm_buzzer_set(true);
    }
}

void e3_meter_app_on_ac_fail(bool is_power_cut)
{
    if (is_power_cut)
    {
        s_is_ac_failed = true;
        s_lcd_wakeup_timer_ms = 0U;

        /* Catat kejadian ke Event Log FIFO (SPLN Tabel 18 No. 9: Power supply failure) */
        spln_log_record_event(&s_tamper_ctx, SPLN_LOG_EVENT_POWER_FAILURE, true, s_now_epoch_sec, 0);

        /* 1. Simpan snapshot 3 log FIFO (Alarm, Event, Tamper) ke NVRAM (SPI2 Flash) & Kirim paket darurat Last-Gasp */
        (void)nvram_save_tamper_log_snapshot(&s_tamper_ctx);
        (void)e2_dlms_send_last_gasp_push(s_now_epoch_sec, s_last_active_energy_wh);

        /* 2. Matikan beban berdaya tinggi saat padam (Power Gating: LCD, Backlight, RS-232, Modem 12V, Buzzer) */
        HAL_GPIO_WritePin(LCD_PWR_EN_GPIO_Port, LCD_PWR_EN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(RS232_PWR_EN_GPIO_Port, RS232_PWR_EN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(MODEM_12V_EN_GPIO_Port, MODEM_12V_EN_Pin, GPIO_PIN_RESET);
        bsp_alarm_buzzer_set(false);
        s_button_beep_ms = 0U;
    }
    else
    {
        s_is_ac_failed = false;
        s_lcd_wakeup_timer_ms = 0U;

        /* Catat kejadian ke Event Log FIFO (SPLN Tabel 18 No. 10: Power supply up) */
        spln_log_record_event(&s_tamper_ctx, SPLN_LOG_EVENT_POWER_UP, true, s_now_epoch_sec, 0);

        /* Hidupkan kembali seluruh rail periferal saat PLN pulih normal */
        HAL_GPIO_WritePin(LCD_PWR_EN_GPIO_Port, LCD_PWR_EN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_Port, LCD_BL_CTRL_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(RS232_PWR_EN_GPIO_Port, RS232_PWR_EN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MODEM_12V_EN_GPIO_Port, MODEM_12V_EN_Pin, GPIO_PIN_SET);
    }
}

void e3_meter_app_on_tamper(tamper_vector_t vector)
{
    tamper_process_signal(&s_tamper_ctx, vector, true, s_now_epoch_sec);
}

void e3_meter_app_on_zero_crossing(void)
{
    s_zero_crossing_count++;
}

void e3_meter_app_on_aux_input(uint8_t channel, bool is_active)
{
    if (channel == 1U)
    {
        if (is_active) s_aux_input_state_mask |= 0x01U;
        else           s_aux_input_state_mask &= (uint8_t)~0x01U;
    }
    else if (channel == 2U)
    {
        if (is_active) s_aux_input_state_mask |= 0x02U;
        else           s_aux_input_state_mask &= (uint8_t)~0x02U;
    }

    /* Sinkronkan output kontrol bantu AUX_CTRL_OUT1 (PD8) dengan status interlok input bantu */
    HAL_GPIO_WritePin(AUX_CTRL_OUT1_GPIO_Port, AUX_CTRL_OUT1_Pin,
                      (s_aux_input_state_mask != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

uint32_t e3_meter_app_get_epoch(void)
{
    return s_now_epoch_sec;
}

void e3_meter_app_set_epoch(uint32_t epoch_sec)
{
    s_now_epoch_sec = epoch_sec;
#if defined(EMBEDDED_HARDWARE_TARGET)
    SECURE_SetRTCDateTime(epoch_sec);
#endif
    s_active_tariff_zone = evaluate_tou_tariff_zone(s_now_epoch_sec);
    display_set_time(&s_disp_ctx, s_now_epoch_sec);
    s_last_disp_print_ms = 0U; /* Segera perbarui layar OLED */
}

uint64_t e3_meter_app_get_tou_energy_wh(uint8_t zone_idx)
{
    if (zone_idx >= E3_TOU_MAX_ZONES) return 0ULL;
    return s_tou_tariff_wh[zone_idx];
}

uint8_t e3_meter_app_get_active_tariff_zone(void)
{
    return s_active_tariff_zone;
}

void e3_meter_app_trigger_billing_snapshot(void)
{
    billing_snapshot_entry_t snap = {
        .billing_epoch_sec = s_now_epoch_sec,
        .active_energy_import_wh = s_last_active_energy_wh,
        .active_energy_wbp_wh = s_tou_tariff_wh[0],
        .active_energy_lwbp_wh = s_tou_tariff_wh[1] + s_tou_tariff_wh[2],
        .reactive_energy_varh = s_disp_ctx.meas_buffer.reactive_energy_varh,
        .max_demand_w = s_md_tracker.max_demand_w,
        .max_demand_timestamp = s_md_tracker.max_demand_timestamp
    };
    billing_snapshot_record(&s_billing_ctx, &snap);
    max_demand_reset(&s_md_tracker);
}

const billing_snapshot_ctx_t* e3_meter_app_get_billing_snapshots(void)
{
    return &s_billing_ctx;
}

const max_demand_tracker_t* e3_meter_app_get_max_demand(void)
{
    return &s_md_tracker;
}

static void log_internal_alarm_events(uint32_t newly_asserted_alarms, uint32_t epoch_sec)
{
    if (newly_asserted_alarms == 0U) return;

    if (newly_asserted_alarms & SPLN_ALARM_FLASH_ERROR)
        spln_log_record_alarm(&s_tamper_ctx, SPLN_LOG_ALARM_FLASH_ERROR, true, epoch_sec, 0);
    if (newly_asserted_alarms & SPLN_ALARM_RAM_ERROR)
        spln_log_record_alarm(&s_tamper_ctx, SPLN_LOG_ALARM_RAM_ERROR, true, epoch_sec, 0);
    if (newly_asserted_alarms & SPLN_ALARM_RTC_ERROR)
        spln_log_record_alarm(&s_tamper_ctx, SPLN_LOG_ALARM_CLOCK_LOSS, true, epoch_sec, 0);
    if (newly_asserted_alarms & SPLN_ALARM_LOW_BATTERY)
        spln_log_record_alarm(&s_tamper_ctx, SPLN_LOG_ALARM_LOW_BATTERY, true, epoch_sec, s_vbat_millivolts);
    if (newly_asserted_alarms & SPLN_ALARM_MCU_ERROR)
        spln_log_record_alarm(&s_tamper_ctx, SPLN_LOG_ALARM_MCU_ERROR, true, epoch_sec, 0);
    if (newly_asserted_alarms & SPLN_ALARM_ADC_ERROR)
        spln_log_record_alarm(&s_tamper_ctx, SPLN_LOG_ALARM_ADC_ERROR, true, epoch_sec, 0);
    if (newly_asserted_alarms & SPLN_ALARM_SUPERCAP_FAIL)
        spln_log_record_alarm(&s_tamper_ctx, SPLN_LOG_ALARM_SUPERCAP_FAIL, true, epoch_sec, s_vcap_millivolts);
    if (newly_asserted_alarms & SPLN_ALARM_RELAY_FAIL)
        spln_log_record_alarm(&s_tamper_ctx, SPLN_LOG_ALARM_RELAY_FAIL, true, epoch_sec, 0);
}

void e3_meter_app_set_internal_alarm(uint32_t alarm_flags)
{
    uint32_t new_alarms = alarm_flags & (~s_internal_alarm_mask);
    s_internal_alarm_mask |= alarm_flags;
    log_internal_alarm_events(new_alarms, s_now_epoch_sec);
    display_set_internal_alarm(&s_disp_ctx, s_internal_alarm_mask);
}

uint32_t e3_meter_app_get_internal_alarm(void)
{
    return s_internal_alarm_mask;
}

const tamper_context_t* e3_meter_app_get_tamper_context(void)
{
    return &s_tamper_ctx;
}
