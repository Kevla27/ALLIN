/**
 * @file meter_system_bridge.c
 * @brief Implementasi Jembatan Integrasi Sistem (System Bridge / Glue Code)
 *        Memusatkan seluruh callback HAL STM32, routing interrupt, inisialisasi
 *        tiga pilar (E1, E2, E3), dan loop task RTOS di luar main.c.
 * 
 * Target: STM32U585VIT6 (ARM Cortex-M33 TrustZone Non-Secure)
 */

#include "meter_system_bridge.h"
#include "cmsis_os2.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* Header Layanan Modular Tiga Pilar (E1, E2, E3) */
#include "e1_metrology_service.h"
#include "RegisterMap.h"
#include "SPI.h"
#include "e2_dlms_service.h"
#include "e3_meter_app.h"
#include "console_io.h"
#include "nvram_storage.h"
#include "wcet_profiler.h"
#include "rtos_tasks.h"
#include "display_task.h"
#include "bsp_display_oled.h"

/* -------------------------------------------------------------------------- */
/* Variabel Statis Internal                                                  */
/* -------------------------------------------------------------------------- */

/* Handle UART untuk transmisi serial konsol / modem (USART2) */
static UART_HandleTypeDef *s_console_uart = NULL;
extern UART_HandleTypeDef huart1;
static uint8_t s_vcp_rx_byte = 0U;

#define VCP_RX_FIFO_SIZE 256U
static volatile uint8_t  s_vcp_fifo[VCP_RX_FIFO_SIZE];
static volatile uint16_t s_vcp_head = 0U;
static volatile uint16_t s_vcp_tail = 0U;

static inline void vcp_fifo_push(uint8_t b)
{
    uint16_t next = (s_vcp_head + 1U) % VCP_RX_FIFO_SIZE;
    if (next != s_vcp_tail)
    {
        s_vcp_fifo[s_vcp_head] = b;
        s_vcp_head = next;
    }
}

static inline bool vcp_fifo_pop(uint8_t *b)
{
    if (s_vcp_head == s_vcp_tail) return false;
    *b = s_vcp_fifo[s_vcp_tail];
    s_vcp_tail = (s_vcp_tail + 1U) % VCP_RX_FIFO_SIZE;
    return true;
}

/* Context data pengukuran listrik yang dikoordinasikan antar pilar */
static meter_measurements_t s_meas;

/* -------------------------------------------------------------------------- */
/* Callback Console I/O Transmit Retargeting                                  */
/* -------------------------------------------------------------------------- */

static void bridge_console_uart_tx(const uint8_t *data, size_t len)
{
    if ((data != NULL) && (len > 0U))
    {
        /* 1. Transmit ke ST-LINK Virtual COM Port (USART1 @ 115200 bps) jika terpasang */
        if (huart1.Instance == USART1)
        {
            HAL_UART_Transmit(&huart1, (uint8_t *)data, (uint16_t)len, 100U);
        }
        else if ((s_console_uart != NULL) && (s_console_uart->Instance != NULL))
        {
            /* Fallback jika USART1 tidak aktif */
            HAL_UART_Transmit(s_console_uart, (uint8_t *)data, (uint16_t)len, 100U);
        }
    }
}

/* -------------------------------------------------------------------------- */
/* Implementasi API Bridge                                                    */
/* -------------------------------------------------------------------------- */

typedef enum {
    MODEM_AT_IDLE = 0,
    MODEM_AT_WAIT_PING,
    MODEM_AT_WAIT_SIM,
    MODEM_AT_WAIT_REG,
    MODEM_AT_WAIT_ATTACH,
    MODEM_AT_ONLINE
} modem_at_substep_t;

static bool bridge_modem_step_callback(modem_fsm_state_t *state, const modem_push_msg_t *optional_push)
{
    static modem_at_substep_t s_at_step = MODEM_AT_IDLE;
    static uint32_t s_wait_ticks = 0U;
    static uint8_t  s_retries = 0U;
    static bool     s_cmd_sent = false;
    static bool     s_sim_seen = false;
    static bool     s_bench_fallback_logged = false;
    static char     s_at_line[64];
    static size_t   s_at_line_len = 0U;

    bool saw_ok = false;
    bool saw_err = false;
    bool saw_net_reg = false;
    uint8_t rx_b = 0U;

    /* 1. Kuras balasan AT non-blocking dari ring USART2 */
    while (e2_dlms_service_pop_modem_rx_byte(&rx_b))
    {
        if (rx_b == (uint8_t)'\r' || rx_b == (uint8_t)'\n')
        {
            if (s_at_line_len > 0U)
            {
                s_at_line[s_at_line_len] = '\0';
                if (strstr(s_at_line, "OK") != NULL) { saw_ok = true; }
                if (strstr(s_at_line, "ERROR") != NULL) { saw_err = true; }
                if (strstr(s_at_line, "+CPIN: READY") != NULL) { s_sim_seen = true; }
                if ((strstr(s_at_line, "+CREG: 0,1") != NULL) ||
                    (strstr(s_at_line, "+CREG: 0,5") != NULL) ||
                    (strstr(s_at_line, "+CREG: 1,1") != NULL) ||
                    (strstr(s_at_line, "+CREG: 1,5") != NULL))
                {
                    saw_net_reg = true;
                }
                s_at_line_len = 0U;
            }
        }
        else if (s_at_line_len < (sizeof(s_at_line) - 1U))
        {
            s_at_line[s_at_line_len++] = (char)rx_b;
        }
    }

    if (state != NULL)
    {
        if (*state == MODEM_STATE_INIT)
        {
            s_at_step = MODEM_AT_WAIT_PING;
            s_wait_ticks = 0U;
            s_retries = 0U;
            s_cmd_sent = false;
            s_sim_seen = false;
            s_at_line_len = 0U;
            *state = MODEM_STATE_CONNECTING;
        }
        else if (*state == MODEM_STATE_CONNECTING)
        {
            if (!s_cmd_sent)
            {
                const char *cmd = "AT\r\n";
                if (s_at_step == MODEM_AT_WAIT_SIM)         cmd = "AT+CPIN?\r\n";
                else if (s_at_step == MODEM_AT_WAIT_REG)    cmd = "AT+CREG?\r\n";
                else if (s_at_step == MODEM_AT_WAIT_ATTACH) cmd = "AT+CGATT=1\r\n";
                (void)e2_dlms_service_send_modem_at(cmd);
                s_cmd_sent = true;
                s_wait_ticks = 0U;
            }

            if (s_at_step == MODEM_AT_WAIT_PING && saw_ok)
            {
                s_at_step = MODEM_AT_WAIT_SIM;
                s_cmd_sent = false;
                s_retries = 0U;
            }
            else if (s_at_step == MODEM_AT_WAIT_SIM && s_sim_seen && saw_ok)
            {
                s_at_step = MODEM_AT_WAIT_REG;
                s_cmd_sent = false;
                s_retries = 0U;
            }
            else if (s_at_step == MODEM_AT_WAIT_REG && saw_net_reg)
            {
                s_at_step = MODEM_AT_WAIT_ATTACH;
                s_cmd_sent = false;
                s_retries = 0U;
            }
            else if (s_at_step == MODEM_AT_WAIT_ATTACH && saw_ok)
            {
                s_at_step = MODEM_AT_ONLINE;
                e2_dlms_service_set_modem_at_active(false);
                *state = MODEM_STATE_READY;
                printf("[MODEM] Jaringan 4G terdaftar & GPRS/PDP aktif.\r\n");
            }
            else if (saw_err || (++s_wait_ticks >= 4U))
            {
                s_cmd_sent = false;
                s_wait_ticks = 0U;
                s_sim_seen = false;
                s_retries++;
                if (s_retries >= 3U)
                {
                    e2_dlms_service_set_modem_at_active(false);
                    if (board_cfg_is_synthetic_load_enabled())
                    {
                        if (!s_bench_fallback_logged)
                        {
                            s_bench_fallback_logged = true;
                            printf("[MODEM] Tidak ada modul 4G fisik di USART2; mode fallback bench aktif (READY).\r\n");
                        }
                        s_at_step = MODEM_AT_ONLINE;
                        *state = MODEM_STATE_READY;
                    }
                    else
                    {
                        s_at_step = MODEM_AT_IDLE;
                        s_wait_ticks = 0U;
                        *state = MODEM_STATE_ERROR;
                    }
                }
            }
        }
        else if (*state == MODEM_STATE_ERROR)
        {
            if (++s_wait_ticks >= 20U)
            {
                *state = MODEM_STATE_INIT;
            }
        }
    }

    if (optional_push != NULL)
    {
        e3_meter_app_notify_modem_activity();
        return e2_dlms_send_last_gasp_push(optional_push->timestamp, optional_push->active_energy_wh);
    }
    return true;
}

static void bridge_optical_poll_callback(void)
{
    /* Layanan jabat tangan IEC 62056-21 Mode E & pemrosesan ring DLMS RX/TX oleh DLMSTask tiap 50 ms */
    e2_dlms_service_poll();
}

void meter_system_bringup(UART_HandleTypeDef *huart_opt, UART_HandleTypeDef *huart_modem, ADC_HandleTypeDef *hadc_telemetry)
{
    s_console_uart = huart_modem;
    memset(&s_meas, 0, sizeof(s_meas));

    /* 1. Inisialisasi Console I/O & printf retargeting ke ST-LINK VCP (USART1) & USART2 */
    console_io_init(bridge_console_uart_tx);
    console_io_print_boot_banner("SMART METER 3-PHASE STM32U575 (NUCLEO)", "USART1 @ 115200 bps / USART2 @ 19200 bps", 160U);

    /* Inisialisasi WCET Profiler & Watchdog Supervisor (DWT @ 160 MHz & Pin PD12) */
    wcet_profiler_init(160000000U);

    /* Aktifkan interrupt penerimaan karakter serial CLI pada ST-LINK Virtual COM Port (USART1) */
    if (huart1.Instance == USART1)
    {
        HAL_UART_Receive_IT(&huart1, &s_vcp_rx_byte, 1U);
    }

    /* 2. Inisialisasi Layanan Terpadu Pilar E1 (Metrologi), E2 (DLMS/COSEM), dan E3 (Aplikasi) */
    e1_metrology_service_init();
    e2_dlms_service_init(huart_opt);
    e2_dlms_service_set_modem_uart(huart_modem);
    e3_meter_app_init(METER_CONFIG_CUSTOMER_ID, hadc_telemetry);

    /* Verifikasi Chip SPI2 Flash Eksternal: Winbond W25Q64 (64 Mbit / 8 MB) */
    uint8_t flash_mfg = 0, flash_type = 0, flash_cap = 0;
    if (nvram_w25q64_read_jedec_id(&flash_mfg, &flash_type, &flash_cap))
    {
        printf("[NVRAM] SPI2 Flash Target: Winbond W25Q64 (JEDEC: 0x%02X 0x%02X 0x%02X, Cap: 8 MB / 2048 Sectors)\n",
               flash_mfg, flash_type, flash_cap);
    }

    /* 3. Pulihkan Stand Akumulasi Energi dari NVRAM Checkpoint ke Context Pengukuran Sistem */
    nvram_energy_checkpoint_t cp = {0};
    if (nvram_load_energy_checkpoint(&cp))
    {
        s_meas.active_energy_wh        = cp.active_energy_import_wh;
        s_meas.active_energy_import_wh = cp.active_energy_export_wh;
        s_meas.reactive_energy_varh    = cp.reactive_energy_varh;
    }

    /* 4. Pulihkan Parameter Kalibrasi ADE9000 dari NVRAM Flash (jika ada) */
    nvram_calibration_data_t cal = {0};
    if (nvram_load_calibration(&cal))
    {
        printf("[BRIDGE] Restored ADE9000 Calibration from NVRAM Flash (VGain: 0x%08lX, IGain: 0x%08lX)\n",
               (unsigned long)cal.vgain[0], (unsigned long)cal.igain[0]);
        ADE9000_SPI_TransactionWrite(ADE9000_WR_LOCK, ADE9000_SPI_DATA_16BIT, ADE9000_WR_UNLOCK_KEY);
        ADE9000_SPI_TransactionWrite(ADE9000_AVGAIN, ADE9000_SPI_DATA_32BIT, cal.vgain[0]);
        ADE9000_SPI_TransactionWrite(ADE9000_BVGAIN, ADE9000_SPI_DATA_32BIT, cal.vgain[1]);
        ADE9000_SPI_TransactionWrite(ADE9000_CVGAIN, ADE9000_SPI_DATA_32BIT, cal.vgain[2]);
        ADE9000_SPI_TransactionWrite(ADE9000_AIGAIN, ADE9000_SPI_DATA_32BIT, cal.igain[0]);
        ADE9000_SPI_TransactionWrite(ADE9000_BIGAIN, ADE9000_SPI_DATA_32BIT, cal.igain[1]);
        ADE9000_SPI_TransactionWrite(ADE9000_CIGAIN, ADE9000_SPI_DATA_32BIT, cal.igain[2]);
        ADE9000_SPI_TransactionWrite(ADE9000_NIGAIN, ADE9000_SPI_DATA_32BIT, cal.igain[3]);
        ADE9000_SPI_TransactionWrite(ADE9000_APGAIN, ADE9000_SPI_DATA_32BIT, cal.pgain[0]);
        ADE9000_SPI_TransactionWrite(ADE9000_BPGAIN, ADE9000_SPI_DATA_32BIT, cal.pgain[1]);
        ADE9000_SPI_TransactionWrite(ADE9000_CPGAIN, ADE9000_SPI_DATA_32BIT, cal.pgain[2]);
        ADE9000_SPI_TransactionWrite(ADE9000_WR_LOCK, ADE9000_SPI_DATA_16BIT, ADE9000_WR_LOCK_KEY);
    }

    /* 5. Inisialisasi FreeRTOS Subsystem IPC (3 Queues, 3 Mutex, EventGroup) dan Integrasikan 8 Task */
    rtos_set_device_ids(e3_meter_app_get_customer_id(), e3_meter_app_get_meter_id());
    rtos_system_init();
    display_task_set_driver(bsp_display_oled_get_driver());
    extern void bridge_metrology_sample_callback(meter_measurements_t *out_meas);
    rtos_metrology_set_sample_cb(bridge_metrology_sample_callback);
    rtos_profile_set_1s_cb(e3_meter_app_service_profile_1s);
    rtos_checkpoint_set_build_cb(e3_meter_app_build_checkpoint);
    rtos_health_set_check_cb(e3_meter_app_run_health_check);
    rtos_relay_set_actuate_cb(e3_meter_app_actuate_hardware);
    rtos_modem_set_step_cb(bridge_modem_step_callback);
    rtos_dlms_set_optical_poll_cb(bridge_optical_poll_callback);
    rtos_power_set_stop2_cb(e3_meter_app_service_stop2_cycle);

    /* Muat stand energi awal ke dalam published measurement snapshot secara thread-safe */
    rtos_meter_data_publish(&s_meas);

    /* Jika boot berasal dari kegagalan fatal CPU / Watchdog Loop (ERR04), kunci relai & LED kuning SPLN */
    if (rtos_watchdog_is_err04_latched())
    {
        e3_meter_app_set_internal_alarm(SPLN_ALARM_MCU_ERROR);
        e3_meter_app_actuate_hardware(100U);
    }

    printf("[BRIDGE] All Pillars (E1 Metrology, E2 DLMS, E3 8-Task Preemptive) initialized.\n");
}

const meter_measurements_t *meter_system_get_measurements(void)
{
    return &s_meas;
}

#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
static uint32_t isqrt64(uint64_t val)
{
    if (val == 0) return 0;
    uint64_t op = val;
    uint64_t res = 0;
    uint64_t one = 1ULL << 62;
    while (one > op) one >>= 2;
    while (one != 0) {
        if (op >= res + one) {
            op -= res + one;
            res = (res >> 1) + one;
        } else {
            res >>= 1;
        }
        one >>= 2;
    }
    return (uint32_t)res;
}
#endif

void bridge_metrology_sample_callback(meter_measurements_t *out_meas)
{
    if (out_meas == NULL) return;

    /* 1. Lakukan akuisisi data metrologi ADE9000 untuk durasi 1000 ms */
    e1_metrology_service_step(out_meas, 1000U);

#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
    if (e3_meter_app_is_synthetic_load_active())
    {
        synthetic_load_config_t synth;
        e3_meter_app_get_synthetic_load(&synth);

        out_meas->voltage_r_dvolts = synth.voltage_r_dv;
        out_meas->voltage_s_dvolts = synth.voltage_s_dv;
        out_meas->voltage_t_dvolts = synth.voltage_t_dv;

        out_meas->current_r_mamps = synth.current_r_ma;
        out_meas->current_s_mamps = synth.current_s_ma;
        out_meas->current_t_mamps = synth.current_t_ma;
        out_meas->current_n_mamps = synth.current_n_ma;

        out_meas->active_power_w     = synth.active_power_w;
        out_meas->reactive_power_var = synth.reactive_power_var;
        out_meas->power_factor_ppm   = synth.power_factor_ppm;

        int64_t p2 = (int64_t)out_meas->active_power_w * out_meas->active_power_w;
        int64_t q2 = (int64_t)out_meas->reactive_power_var * out_meas->reactive_power_var;
        out_meas->apparent_power_va = isqrt64((uint64_t)(p2 + q2));

        if (out_meas->active_power_w > 0)
        {
            static uint32_t s_synth_wh_rem_mj = 0;
            uint64_t mj = (uint64_t)out_meas->active_power_w * 1000U + s_synth_wh_rem_mj;
            out_meas->active_energy_wh += (mj / 3600000ULL);
            s_synth_wh_rem_mj = (uint32_t)(mj % 3600000ULL);
        }
        else if (out_meas->active_power_w < 0)
        {
            static uint32_t s_synth_imp_wh_rem_mj = 0;
            uint64_t mj_imp = (uint64_t)(-out_meas->active_power_w) * 1000U + s_synth_imp_wh_rem_mj;
            out_meas->active_energy_import_wh += (mj_imp / 3600000ULL);
            s_synth_imp_wh_rem_mj = (uint32_t)(mj_imp % 3600000ULL);
        }

        if (out_meas->reactive_power_var != 0)
        {
            static uint32_t s_synth_varh_rem_mj = 0;
            uint32_t abs_q = (uint32_t)((out_meas->reactive_power_var > 0) ? out_meas->reactive_power_var : -out_meas->reactive_power_var);
            uint64_t mvar_s = (uint64_t)abs_q * 1000U + s_synth_varh_rem_mj;
            out_meas->reactive_energy_varh += (mvar_s / 3600000ULL);
            s_synth_varh_rem_mj = (uint32_t)(mvar_s % 3600000ULL);
        }
    }
#endif

    s_meas = *out_meas;
}

void app_metering_step(void)
{
    static uint32_t last_step_tick_ms = 0;
    uint32_t current_tick_ms = HAL_GetTick();
    uint32_t delta_ms = (last_step_tick_ms == 0) ? 100U : (current_tick_ms - last_step_tick_ms);
    last_step_tick_ms = current_tick_ms;
    if (delta_ms == 0U)
    {
        delta_ms = 1U;
    }

    /* 0. Proses antrean byte karakter Interactive Serial CLI dari USART1 (VCP) & USART2 (Modem)
     * Dilakukan sebelum pencatatan WCET superloop agar transmisi respons teks CLI tidak mendistorsi
     * waktu komputasi riil siklus pengukuran meter. */
    uint8_t cli_byte = 0U;
    while (vcp_fifo_pop(&cli_byte))
    {
        console_cli_process_rx_byte(cli_byte);
    }
    e2_dlms_service_poll();
    while (e2_dlms_service_pop_cli_rx_byte(&cli_byte))
    {
        console_cli_process_rx_byte(cli_byte);
    }

    /* 1. Pilar E1: Akuisisi Data ADC/SPI ADE9000, Konversi Fisik & Akumulasi Energi Wh */
    e1_metrology_service_step(&s_meas, delta_ms);

#if (BOARD_CFG_ENABLE_SYNTHETIC_LOAD == 1)
    if (e3_meter_app_is_synthetic_load_active())
    {
        synthetic_load_config_t synth;
        e3_meter_app_get_synthetic_load(&synth);

        s_meas.voltage_r_dvolts = synth.voltage_r_dv;
        s_meas.voltage_s_dvolts = synth.voltage_s_dv;
        s_meas.voltage_t_dvolts = synth.voltage_t_dv;

        s_meas.current_r_mamps = synth.current_r_ma;
        s_meas.current_s_mamps = synth.current_s_ma;
        s_meas.current_t_mamps = synth.current_t_ma;
        s_meas.current_n_mamps = synth.current_n_ma;

        s_meas.active_power_w     = synth.active_power_w;
        s_meas.reactive_power_var = synth.reactive_power_var;
        s_meas.power_factor_ppm   = synth.power_factor_ppm;

        int64_t p2 = (int64_t)s_meas.active_power_w * s_meas.active_power_w;
        int64_t q2 = (int64_t)s_meas.reactive_power_var * s_meas.reactive_power_var;
        s_meas.apparent_power_va = isqrt64((uint64_t)(p2 + q2));

        if (s_meas.active_power_w > 0)
        {
            static uint32_t s_synth_wh_rem_mj = 0;
            uint64_t mj = (uint64_t)s_meas.active_power_w * delta_ms + s_synth_wh_rem_mj;
            s_meas.active_energy_wh += (mj / 3600000ULL);
            s_synth_wh_rem_mj = (uint32_t)(mj % 3600000ULL);
        }
        else if (s_meas.active_power_w < 0)
        {
            static uint32_t s_synth_imp_wh_rem_mj = 0;
            uint64_t mj_imp = (uint64_t)(-s_meas.active_power_w) * delta_ms + s_synth_imp_wh_rem_mj;
            s_meas.active_energy_import_wh += (mj_imp / 3600000ULL);
            s_synth_imp_wh_rem_mj = (uint32_t)(mj_imp % 3600000ULL);
        }

        if (s_meas.reactive_power_var != 0)
        {
            static uint32_t s_synth_varh_rem_mj = 0;
            uint32_t abs_q = (uint32_t)((s_meas.reactive_power_var > 0) ? s_meas.reactive_power_var : -s_meas.reactive_power_var);
            uint64_t mvar_s = (uint64_t)abs_q * delta_ms + s_synth_varh_rem_mj;
            s_meas.reactive_energy_varh += (mvar_s / 3600000ULL);
            s_synth_varh_rem_mj = (uint32_t)(mvar_s % 3600000ULL);
        }
    }
#endif

    /* 2. Pilar E2: Sinkronisasi Register Data Model OBIS DLMS/COSEM */
    e2_dlms_service_sync_snapshot(e2_dlms_service_get_snapshot(), &s_meas);

    /* 3. Pilar E3: Evaluasi Tamper, Relai Shunt Trip, Telemetri Baterai, Carousel Display & Profiling */
    e3_meter_app_step(&s_meas, delta_ms);

    /* 4. Pengawasan Kepatuhan Heartbeat Task & Penyegaran WDT (PD12) */
    (void)rtos_watchdog_supervise_and_feed();
}

void app_metering_task_loop(void)
{
    /* 1. Luncurkan seluruh 7 Worker Task Preemptive FreeRTOS E3 (+ 1 defaultTask Supervisor = 8 Tasks) */
    bool tasks_ok = rtos_start_all_tasks();
    printf("[BRIDGE] FreeRTOS 8-Task Preemptive Multi-Tasking: %s\n",
           tasks_ok ? "7 Worker Tasks Successfully Launched!" : "WARNING: Task Creation Failed!");

    /* 2. defaultTask bertindak sebagai Supervisor, CLI Handler, Transisi Stop 2 & Pengawas Watchdog PD12 */
    for (;;)
    {
        /* Proses antrean byte karakter Interactive Serial CLI dari USART1 (VCP) & USART2 (Modem) */
        uint8_t cli_byte = 0U;
        while (vcp_fifo_pop(&cli_byte))
        {
            console_cli_process_rx_byte(cli_byte);
        }
        while (e2_dlms_service_pop_cli_rx_byte(&cli_byte))
        {
            console_cli_process_rx_byte(cli_byte);
        }

        /* Evaluasi manajemen mode hemat daya Stop 2 saat pasokan AC PLN padam */
        e3_meter_app_service_stop2_cycle(100U);

        /* Evaluasi keaktifan 7 worker task (Mask 0xEF) dan segarkan pulsa Watchdog hardware (PD12) */
        (void)rtos_watchdog_supervise_and_feed();

        osDelay(100U);
    }
}

/* -------------------------------------------------------------------------- */
/* Callback Interrupt Hardware Abstraction Layer (STM32 HAL)                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief Callback interrupt penerimaan karakter UART (Optik & Modem).
 *        Meng-override deklarasi __weak pada stm32u5xx_hal_uart.c.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != NULL && huart->Instance == USART1)
    {
        /* Simpan karakter CLI dari ST-LINK VCP ke FIFO non-blocking */
        vcp_fifo_push(s_vcp_rx_byte);
        HAL_UART_Receive_IT(&huart1, &s_vcp_rx_byte, 1U);
        return;
    }

    /* Serahkan pemrosesan byte optik (USART3) maupun modem (USART2) ke Pilar E2 */
    e2_dlms_service_uart_rx_callback(huart);
}

/**
 * @brief Callback interrupt penanganan error UART (Overrun, Noise, Framing).
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart != NULL && huart->Instance == USART1)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        huart->ErrorCode = HAL_UART_ERROR_NONE;
        HAL_UART_Receive_IT(&huart1, &s_vcp_rx_byte, 1U);
        return;
    }

    e2_dlms_service_uart_error_callback(huart);
}

/**
 * @brief Callback interrupt falling-edge EXTI GPIO.
 *        Meng-override deklarasi __weak pada stm32u5xx_hal_gpio.c.
 */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BTN_SCROLL_UP_Pin)
    {
        /* Sesuai SPLN Tabel 5 No 12: Deteksi tombol simultan (▲ + ▼) */
        if (HAL_GPIO_ReadPin(BTN_SCROLL_DN_GPIO_Port, BTN_SCROLL_DN_Pin) == GPIO_PIN_RESET)
        {
            e3_meter_app_on_button(BUTTON_DIR_BOTH);
        }
        else
        {
            e3_meter_app_on_button(BUTTON_DIR_UP);
        }
    }
    else if (GPIO_Pin == BTN_SCROLL_DN_Pin)
    {
        /* Sesuai SPLN Tabel 5 No 12: Deteksi tombol simultan (▲ + ▼) */
        if (HAL_GPIO_ReadPin(BTN_SCROLL_UP_GPIO_Port, BTN_SCROLL_UP_Pin) == GPIO_PIN_RESET)
        {
            e3_meter_app_on_button(BUTTON_DIR_BOTH);
        }
        else
        {
            e3_meter_app_on_button(BUTTON_DIR_DOWN);
        }
    }
    else if (GPIO_Pin == AC_FAIL_DET_Pin)
    {
        /* Deteksi Listrik Padam (Last Gasp): Matikan beban tinggi (E3) & ADE9000 ke Sleep (E1) */
        e3_meter_app_on_ac_fail(true);
        e1_metrology_set_power_mode(true);
    }
    else if (GPIO_Pin == ADE_CF1_Pin)
    {
        e1_metrology_on_active_pulse();
    }
    else if (GPIO_Pin == ADE_CF2_Pin)
    {
        e1_metrology_on_reactive_pulse();
    }
    else if (GPIO_Pin == ADE_IRQ0_Pin)
    {
        e3_meter_app_on_tamper(TAMPER_VECTOR_REVERSE_POWER);
    }
    else if (GPIO_Pin == ADE_IRQ1_Pin)
    {
        e3_meter_app_on_tamper(TAMPER_VECTOR_NEUTRAL_BYPASS);
    }
    else if (GPIO_Pin == ADE_DREADY_Pin)
    {
        /* ADE9000 DREADY (Data Ready ADC 8 kSPS) - Sinkronisasi Metrologi Pilar E1 */
        e1_metrology_on_data_ready();
    }
    else if (GPIO_Pin == ADE_CF_ZX_Pin)
    {
        /* Zero-Crossing pulse */
        e3_meter_app_on_zero_crossing();
    }
    else if (GPIO_Pin == AUX_CTRL_IN1_Pin)
    {
        e3_meter_app_on_aux_input(1U, true);
    }
    else if (GPIO_Pin == AUX_CTRL_IN2_Pin)
    {
        e3_meter_app_on_aux_input(2U, true);
    }
}

/**
 * @brief Callback interrupt rising-edge EXTI GPIO.
 *        Meng-override deklarasi __weak pada stm32u5xx_hal_gpio.c.
 */
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == AC_FAIL_DET_Pin)
    {
        /* Deteksi Listrik PLN Normal Kembali: Hidupkan periferal (E3) & ADE9000 ke Run (E1) */
        e3_meter_app_on_ac_fail(false);
        e1_metrology_set_power_mode(false);
    }
    else if (GPIO_Pin == ADE_CF_ZX_Pin)
    {
        e3_meter_app_on_zero_crossing();
    }
    else if (GPIO_Pin == AUX_CTRL_IN1_Pin)
    {
        e3_meter_app_on_aux_input(1U, false);
    }
    else if (GPIO_Pin == AUX_CTRL_IN2_Pin)
    {
        e3_meter_app_on_aux_input(2U, false);
    }
}

bool meter_system_bridge_set_customer_id(const char *customer_id)
{
    return e3_meter_app_set_customer_id(customer_id);
}

const char* meter_system_bridge_get_customer_id(void)
{
    return e3_meter_app_get_customer_id();
}

bool meter_system_bridge_set_meter_id(const char *meter_id)
{
    return e3_meter_app_set_meter_id(meter_id);
}

const char* meter_system_bridge_get_meter_id(void)
{
    return e3_meter_app_get_meter_id();
}

