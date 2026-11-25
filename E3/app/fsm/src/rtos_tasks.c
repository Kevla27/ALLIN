/**
 * @file rtos_tasks.c
 * @brief Implementasi Arsitektur 8-Task & Inter-Task Communication (IPC) FreeRTOS E3
 *        - Task 1: TamperTask (Pri 32, 100 ms) + Last-Gasp + Latched Tamper Policy
 *        - Task 2: ProfileTask (Pri 28, 1000 ms) + Voltage Prot + Max Demand + StorageQueue
 *        - Task 3: DLMSTask (Pri 24, 50 ms) + Tamper Log Class 7 + Port Optik
 *        - Task 4: ModemTask [BARU] (Pri 20, 500 ms) + AT-Cmd 4G + ModemTxQueue
 *        - Task 5: OLEDTask (Pri 16, 100 ms / Rotasi 5s) -> display_task.c
 *        - Task 6: HealthTask [BARU] (Pri 12, 5000 ms) + ADC4 VBAT/VCAP + LSE + Self-Test
 *        - Task 7: StorageTask [BARU] (Pri 10, Queue 1000 ms) + Worker Flash W25Q64
 *        - Task 8: defaultTask (Pri 8, 100 ms) + CLI + Stop 2 + Watchdog Supervisor PD12
 */
#include "rtos_tasks.h"
#include "tamper_manager.h"
#include "load_profile.h"
#include "display.h"
#include "nvram_storage.h"
#include "display_task.h"
#include "wcet_profiler.h"
#include "board_profile_config.h"
#include <stdio.h>
#include <string.h>

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#if defined(EMBEDDED_HARDWARE_TARGET)
#include "main.h"
#include "secure_nsc.h"
#include "e2_dlms_service.h"
#include "e3_meter_app.h"
#endif

static QueueHandle_t      xTamperQueue    = NULL;
static QueueHandle_t      xStorageQueue   = NULL;
static QueueHandle_t      xModemTxQueue   = NULL;
static SemaphoreHandle_t  xMeasMutex      = NULL;
static SemaphoreHandle_t  xAppStateMutex  = NULL;
static SemaphoreHandle_t  xSpiFlashMutex  = NULL;
static EventGroupHandle_t xPowerEventGroup = NULL;

#else
/* Stub Simulasi Host PC (CTest) tanpa dependensi header kernel FreeRTOS */
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) (ms)
#endif
#ifndef configTICK_RATE_HZ
#define configTICK_RATE_HZ 1000
#endif
static inline void vTaskDelay(uint32_t ticks) { (void)ticks; }
static inline uint32_t xTaskGetTickCount(void) { return 0U; }

/* Ring Buffer Simulasi 3 Queue di RAM Host PC */
static tamper_event_msg_t simulated_tamper_queue[TAMPER_QUEUE_MAX_ITEMS];
static size_t q_head = 0U, q_tail = 0U, q_count = 0U;

static storage_write_req_t simulated_storage_queue[STORAGE_QUEUE_MAX_ITEMS];
static size_t sq_head = 0U, sq_tail = 0U, sq_count = 0U;

static modem_push_msg_t simulated_modem_queue[MODEM_TX_QUEUE_MAX_ITEMS];
static size_t mq_head = 0U, mq_tail = 0U, mq_count = 0U;
#endif

/* ========================================================================== */
/*                        KONTEKS GLOBAL & STATUS IPC                         */
/* ========================================================================== */

static tamper_context_t      g_tamper_ctx;
static load_profile_ctx_t    g_load_profile_buf;
static display_context_t     g_display_ctx;
static dlms_task_ctx_t       s_dlms_task_ctx;
static ami_server_t          s_ami_server;
static dlms_uart_tx_fn_t     s_dlms_uart_tx_cb = NULL;
static power_relay_context_t g_power_relay_ctx;
static uint32_t              s_obis_tamper_counters[16] = {0};

/* Status Identitas Meter Sinkron */
static char s_rtos_customer_id[16] = METER_CONFIG_CUSTOMER_ID;
static char s_rtos_meter_id[16]    = METER_CONFIG_METER_ID;

/* Kebijakan Tamper Ter-Latch Permanen & Status Modem FSM */
static volatile uint32_t s_latched_tamper_mask = 0U;
static volatile uint32_t s_internal_alarm_mask_mirror = 0U;
static volatile uint32_t s_power_event_bits_mirror = 0U;
static modem_fsm_state_t s_modem_state = MODEM_STATE_INIT;

/* Thread-safe Measurement Snapshot & Registered External Service Hooks */
static meter_measurements_t       s_latest_measurements;
static bool                       s_measurements_valid = false;
static meter_sample_fn_t          s_meter_sample_cb = NULL;
static rtos_meas_hook_fn_t        s_profile_1s_cb = NULL;
static rtos_checkpoint_build_fn_t s_checkpoint_build_cb = NULL;
static rtos_simple_hook_fn_t      s_health_check_cb = NULL;
static rtos_modem_step_fn_t       s_modem_step_cb = NULL;
static rtos_simple_hook_fn_t      s_optical_poll_cb = NULL;
static rtos_elapsed_hook_fn_t     s_relay_actuate_cb = NULL;
static rtos_elapsed_hook_fn_t     s_power_stop2_cb = NULL;

/* ========================================================================== */
/*     WATCHDOG SUPERVISOR & KLASIFIKASI FAULT ERR04 VS ERROR RINGAN (PD12)   */
/* ========================================================================== */

typedef struct {
    uint32_t bit_mask;
    uint32_t max_timeout_ms;
    bool     is_critical;
    uint32_t last_kick_tick_ms;
    bool     has_kicked_once;
    bool     is_timed_out;
} task_liveness_t;

static volatile uint32_t s_watchdog_kicked_mask = 0U;
static task_liveness_t s_liveness_table[7] = {
    { WATCHDOG_TASK_TAMPER_BIT,    1500U,  true,  0U, false, false }, /* Period 100 ms (Kritis) */
    { WATCHDOG_TASK_METROLOGY_BIT, 2500U,  true,  0U, false, false }, /* Period 1000 ms (Kritis) */
    { WATCHDOG_TASK_DLMS_BIT,      1500U,  false, 0U, false, false }, /* Period 50 ms (Non-Kritis) */
    { WATCHDOG_TASK_UI_BIT,        1500U,  false, 0U, false, false }, /* Period 100 ms (Non-Kritis) */
    { WATCHDOG_TASK_MODEM_BIT,     1800U,  false, 0U, false, false }, /* Period 500 ms (Non-Kritis) */
    { WATCHDOG_TASK_HEALTH_BIT,    11500U, true,  0U, false, false }, /* Period 5000 ms (Kritis) */
    { WATCHDOG_TASK_STORAGE_BIT,   2500U,  true,  0U, false, false }  /* Queue 1000 ms (Kritis) */
};

static wdt_supervisor_status_t s_wdt_status = {
    .severity = WDT_SEVERITY_NORMAL,
    .last_fault_source = WDT_SRC_NONE,
    .offending_task_mask = 0U,
    .minor_degraded_mask = 0U,
    .minor_fault_count = 0U,
    .reset_streak = 0U,
    .err04_latched = false,
    .pending_reset = false,
    .stable_uptime_ms = 0U
};
static uint32_t s_wdt_boot_stable_start_ms = 0U;
static bool     s_wdt_pending_reset_armed  = false;
#if !defined(EMBEDDED_HARDWARE_TARGET)
static uint32_t s_host_boot_fault_reg      = WDT_BOOT_REG_MAGIC;
#endif

#if !defined(EMBEDDED_HARDWARE_TARGET) && !defined(USE_FREERTOS)
static uint32_t s_host_test_tick_ms = 0U;
#endif

static uint32_t rtos_get_tick_ms(void) {
#if defined(EMBEDDED_HARDWARE_TARGET)
    return HAL_GetTick();
#elif defined(USE_FREERTOS)
    return (uint32_t)((xTaskGetTickCount() * 1000U) / configTICK_RATE_HZ);
#else
    return s_host_test_tick_ms;
#endif
}

uint32_t rtos_get_rtc_epoch_sec(void) {
#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t rtc_now = SECURE_GetRTCDateTime();
    if (rtc_now > 1700000000U) {
        return rtc_now;
    }
    uint32_t app_epoch = e3_meter_app_get_epoch();
    if (app_epoch > 1700000000U) {
        return app_epoch;
    }
    return 1793181600U + (HAL_GetTick() / 1000U);
#else
    return 1793181600U + (rtos_get_tick_ms() / 1000U);
#endif
}

static uint32_t wdt_read_persist_reg(void) {
    uint32_t reg;
#if defined(EMBEDDED_HARDWARE_TARGET)
    reg = SECURE_ReadBootFaultReg();
#else
    reg = s_host_boot_fault_reg;
#endif
    if ((reg & WDT_BOOT_REG_MAGIC_MASK) != WDT_BOOT_REG_MAGIC) {
        reg = WDT_BOOT_REG_MAGIC;
    }
    return reg;
}

static void wdt_write_persist_reg(uint32_t reg) {
    uint32_t val = (reg & ~WDT_BOOT_REG_MAGIC_MASK) | WDT_BOOT_REG_MAGIC;
#if defined(EMBEDDED_HARDWARE_TARGET)
    SECURE_WriteBootFaultReg(val);
#else
    s_host_boot_fault_reg = val;
#endif
}

static uint32_t wdt_pack_persist_reg(wdt_fault_source_t src,
                                     uint32_t task_mask,
                                     uint32_t streak,
                                     bool err04_latched,
                                     bool pending_reset) {
    if (streak > 15U) {
        streak = 15U;
    }
    uint32_t reg = WDT_BOOT_REG_MAGIC;
    reg |= (((uint32_t)src & 0x0FU) << WDT_BOOT_REG_SRC_SHIFT);
    reg |= ((task_mask & 0xFFU) << WDT_BOOT_REG_TASK_SHIFT);
    reg |= (streak & WDT_BOOT_REG_STREAK_MASK);
    if (err04_latched) {
        reg |= WDT_BOOT_REG_ERR04_BIT;
    }
    if (pending_reset) {
        reg |= WDT_BOOT_REG_PENDING_BIT;
    }
    return reg;
}

static void wdt_sync_status_from_reg(uint32_t reg) {
    s_wdt_status.last_fault_source =
        (wdt_fault_source_t)((reg & WDT_BOOT_REG_SRC_MASK) >> WDT_BOOT_REG_SRC_SHIFT);
    s_wdt_status.offending_task_mask =
        (reg & WDT_BOOT_REG_TASK_MASK) >> WDT_BOOT_REG_TASK_SHIFT;
    s_wdt_status.reset_streak  = reg & WDT_BOOT_REG_STREAK_MASK;
    s_wdt_status.err04_latched = ((reg & WDT_BOOT_REG_ERR04_BIT) != 0U);
    s_wdt_status.pending_reset = ((reg & WDT_BOOT_REG_PENDING_BIT) != 0U);
}

static void wdt_refresh_severity(void) {
    if (s_wdt_status.err04_latched) {
        s_wdt_status.severity = WDT_SEVERITY_FATAL_ERR04;
    } else if (s_wdt_status.minor_degraded_mask != 0U) {
        s_wdt_status.severity = WDT_SEVERITY_MINOR_DEGRADED;
    } else if (s_wdt_status.reset_streak > 0U) {
        s_wdt_status.severity = WDT_SEVERITY_GLITCH_RECOVERING;
    } else {
        s_wdt_status.severity = WDT_SEVERITY_NORMAL;
    }
}

void rtos_watchdog_on_boot(void) {
    uint32_t reg = wdt_read_persist_reg();
    bool was_pending = ((reg & WDT_BOOT_REG_PENDING_BIT) != 0U);
    bool was_err04   = ((reg & WDT_BOOT_REG_ERR04_BIT) != 0U);
    wdt_fault_source_t src =
        (wdt_fault_source_t)((reg & WDT_BOOT_REG_SRC_MASK) >> WDT_BOOT_REG_SRC_SHIFT);
    uint32_t task_mask = (reg & WDT_BOOT_REG_TASK_MASK) >> WDT_BOOT_REG_TASK_SHIFT;
    uint32_t streak    = reg & WDT_BOOT_REG_STREAK_MASK;

    bool fatal_src = (src == WDT_SRC_CPU_FAULT) ||
                     (src == WDT_SRC_STACK_OVERFLOW) ||
                     (src == WDT_SRC_CLOCK_NMI) ||
                     (src == WDT_SRC_CLI_INJECT);
    bool latch_err04 = was_err04 || (was_pending && (fatal_src || (streak >= WDT_FATAL_RESET_THRESHOLD)));

    /* Hapus bit pending setelah MCU selesai restart agar reset manual/power-off berikutnya tidak salah hitung */
    uint32_t updated = wdt_pack_persist_reg(src, task_mask, streak, latch_err04, false);
    wdt_write_persist_reg(updated);

    s_wdt_status.minor_degraded_mask = 0U;
    s_wdt_boot_stable_start_ms = rtos_get_tick_ms();
    s_wdt_pending_reset_armed  = false;
    s_wdt_status.stable_uptime_ms = 0U;
    wdt_sync_status_from_reg(updated);
    wdt_refresh_severity();

    if (s_wdt_status.err04_latched) {
        s_internal_alarm_mask_mirror |= SPLN_ALARM_MCU_ERROR;
        printf("[WDT] *** FATAL MCU FAULT (ERR04 LATCHED) Src=%s Streak=%lu/%u -> Buka Relai & LED Kuning! ***\r\n",
               rtos_watchdog_source_to_string(s_wdt_status.last_fault_source),
               (unsigned long)s_wdt_status.reset_streak,
               (unsigned int)WDT_FATAL_RESET_THRESHOLD);
    } else if (was_pending && (streak > 0U)) {
        printf("[WDT] Reset sementara pulih (Glitch %lu/%u, Mask=0x%02lX). Relai tetap TERTUTUP (Non-ERR04).\r\n",
               (unsigned long)s_wdt_status.reset_streak,
               (unsigned int)WDT_FATAL_RESET_THRESHOLD,
               (unsigned long)s_wdt_status.offending_task_mask);
    }
}

void rtos_watchdog_record_fatal_trap(wdt_fault_source_t src, uint32_t task_mask) {
    uint32_t reg = wdt_read_persist_reg();
    uint32_t streak = reg & WDT_BOOT_REG_STREAK_MASK;
    if (streak < 15U) {
        streak++;
    }
    uint32_t updated = wdt_pack_persist_reg(src, task_mask, streak, true, true);
    wdt_write_persist_reg(updated);
    wdt_sync_status_from_reg(updated);
    wdt_refresh_severity();
    s_wdt_pending_reset_armed = true;
}

void rtos_watchdog_clear_err04(void) {
    uint32_t clean = wdt_pack_persist_reg(WDT_SRC_NONE, 0U, 0U, false, false);
    wdt_write_persist_reg(clean);
    s_wdt_pending_reset_armed = false;
    s_wdt_boot_stable_start_ms = rtos_get_tick_ms();
    s_wdt_status.stable_uptime_ms = 0U;
    s_wdt_status.minor_degraded_mask = 0U;
    s_internal_alarm_mask_mirror &= ~SPLN_ALARM_MCU_ERROR;
    for (size_t i = 0; i < (sizeof(s_liveness_table) / sizeof(s_liveness_table[0])); i++) {
        s_liveness_table[i].is_timed_out = false;
    }
    wdt_sync_status_from_reg(clean);
    wdt_refresh_severity();
    wcet_watchdog_clear_fault();
}

bool rtos_watchdog_is_err04_latched(void) {
    return s_wdt_status.err04_latched;
}

uint32_t rtos_watchdog_get_minor_fault_mask(void) {
    return s_wdt_status.minor_degraded_mask;
}

void rtos_watchdog_get_status(wdt_supervisor_status_t *out_status) {
    if (out_status == NULL) return;
    *out_status = s_wdt_status;
}

void rtos_internal_alarm_set_mask(uint32_t alarm_mask) {
    rtos_app_state_lock();
    s_internal_alarm_mask_mirror = alarm_mask;
    rtos_app_state_unlock();
}

uint32_t rtos_internal_alarm_get_mask(void) {
    return s_internal_alarm_mask_mirror;
}

const char* rtos_watchdog_severity_to_string(wdt_fault_severity_t sev) {
    switch (sev) {
        case WDT_SEVERITY_NORMAL:            return "NORMAL";
        case WDT_SEVERITY_MINOR_DEGRADED:    return "MINOR_DEGRADED (NON-TRIP)";
        case WDT_SEVERITY_GLITCH_RECOVERING: return "GLITCH_RECOVERING (NON-ERR04)";
        case WDT_SEVERITY_FATAL_ERR04:       return "FATAL_ERR04 (RELAI PUTUS)";
        default:                             return "UNKNOWN";
    }
}

const char* rtos_watchdog_source_to_string(wdt_fault_source_t src) {
    switch (src) {
        case WDT_SRC_NONE:               return "NONE";
        case WDT_SRC_CRITICAL_TASK_HANG: return "CRITICAL_TASK_HANG";
        case WDT_SRC_CPU_FAULT:          return "CPU_HARDFAULT";
        case WDT_SRC_STACK_OVERFLOW:     return "RTOS_STACK_OVERFLOW";
        case WDT_SRC_CLOCK_NMI:          return "CLOCK_LOSS_NMI";
        case WDT_SRC_CLI_INJECT:         return "CLI_FAULT_INJECT";
        default:                         return "UNKNOWN";
    }
}

#if !defined(EMBEDDED_HARDWARE_TARGET) && !defined(USE_FREERTOS)
void rtos_watchdog_test_set_tick_ms(uint32_t tick_ms) {
    s_host_test_tick_ms = tick_ms;
}

void rtos_watchdog_test_reset_persist(void) {
    s_host_boot_fault_reg = WDT_BOOT_REG_MAGIC;
    s_host_test_tick_ms   = 0U;
    memset(&s_wdt_status, 0, sizeof(s_wdt_status));
    s_wdt_status.severity = WDT_SEVERITY_NORMAL;
    s_wdt_boot_stable_start_ms = 0U;
    s_wdt_pending_reset_armed  = false;
    s_watchdog_kicked_mask     = 0U;
    for (size_t i = 0; i < (sizeof(s_liveness_table) / sizeof(s_liveness_table[0])); i++) {
        s_liveness_table[i].last_kick_tick_ms = 0U;
        s_liveness_table[i].has_kicked_once   = false;
        s_liveness_table[i].is_timed_out      = false;
    }
}
#endif

void bsp_watchdog_kick_task(uint32_t task_bit) {
    uint32_t now_ms = rtos_get_tick_ms();
    s_watchdog_kicked_mask |= task_bit;

    for (size_t i = 0; i < (sizeof(s_liveness_table) / sizeof(s_liveness_table[0])); i++) {
        if (s_liveness_table[i].bit_mask == task_bit) {
            s_liveness_table[i].last_kick_tick_ms = now_ms;
            s_liveness_table[i].has_kicked_once   = true;
            if (s_liveness_table[i].is_timed_out && !s_liveness_table[i].is_critical) {
                s_liveness_table[i].is_timed_out = false;
                s_wdt_status.minor_degraded_mask &= ~task_bit;
                wdt_refresh_severity();
            }
            break;
        }
    }
    wcet_watchdog_kick(task_bit);
}

uint32_t rtos_watchdog_get_kicked_mask(void) {
    return s_watchdog_kicked_mask;
}

bool rtos_watchdog_supervise_and_feed(void) {
    uint32_t now_ms = rtos_get_tick_ms();

    /* 1. Evaluasi liveness 7 worker task dengan pemisahan Kritis vs Non-Kritis */
    for (size_t i = 0; i < (sizeof(s_liveness_table) / sizeof(s_liveness_table[0])); i++) {
        if (s_liveness_table[i].has_kicked_once) {
            uint32_t elapsed = now_ms - s_liveness_table[i].last_kick_tick_ms;
            if (elapsed > s_liveness_table[i].max_timeout_ms) {
                if (!s_liveness_table[i].is_critical) {
                    /* Error Ringan (OLED/Modem/DLMS): degradasi lokal, JANGAN ERR04 / trip relai / reset loop */
                    if (!s_liveness_table[i].is_timed_out) {
                        s_liveness_table[i].is_timed_out = true;
                        s_wdt_status.minor_fault_count++;
                        s_wdt_status.minor_degraded_mask |= s_liveness_table[i].bit_mask;
                        wdt_refresh_severity();
                        printf("[WDT] Error Ringan Task Non-Kritis (Mask=0x%02lX): mode degradasi tanpa trip relai.\r\n",
                               (unsigned long)s_liveness_table[i].bit_mask);
                    }
                    continue;
                }

                /* Task Kritis macet -> catat streak reset sebelum menahan toggle PD12 */
                s_liveness_table[i].is_timed_out = true;
                if (!s_wdt_pending_reset_armed) {
                    s_wdt_pending_reset_armed = true;
                    uint32_t reg = wdt_read_persist_reg();
                    uint32_t streak = reg & WDT_BOOT_REG_STREAK_MASK;
                    if (streak < 15U) {
                        streak++;
                    }
                    bool err04_now = ((reg & WDT_BOOT_REG_ERR04_BIT) != 0U) ||
                                     (streak >= WDT_FATAL_RESET_THRESHOLD);
                    uint32_t updated = wdt_pack_persist_reg(WDT_SRC_CRITICAL_TASK_HANG,
                                                            s_liveness_table[i].bit_mask,
                                                            streak,
                                                            err04_now,
                                                            true);
                    wdt_write_persist_reg(updated);
                    wdt_sync_status_from_reg(updated);
                    wdt_refresh_severity();
                }
                s_wdt_boot_stable_start_ms = now_ms;
                s_wdt_status.stable_uptime_ms = 0U;
                return false;
            }
        }
    }

    /* 2. Periksa injeksi fault diagnostik (misal CLI `wdt test`) */
    const wcet_profiler_ctx_t *wctx = wcet_get_context();
    if (wctx != NULL && wctx->fault_latched) {
        uint32_t inj_bit = (1U << (uint32_t)wctx->fault_task_id);
        if ((inj_bit & WATCHDOG_NONCRITICAL_TASKS_MASK) != 0U) {
            /* Task non-kritis (DLMS/UI): isolasi sebagai gangguan ringan tanpa reset/ERR04 */
            if ((s_wdt_status.minor_degraded_mask & inj_bit) == 0U) {
                s_wdt_status.minor_fault_count++;
                s_wdt_status.minor_degraded_mask |= inj_bit;
                wdt_refresh_severity();
            }
            wcet_watchdog_clear_fault();
        } else {
            if (!s_wdt_pending_reset_armed) {
                rtos_watchdog_record_fatal_trap(WDT_SRC_CLI_INJECT, inj_bit);
            }
            return wcet_watchdog_supervise_and_feed();
        }
    }

    /* 3. Seluruh task kritis sehat: hitung uptime stabil untuk reset streak glitch sementara */
    s_wdt_status.stable_uptime_ms = now_ms - s_wdt_boot_stable_start_ms;
    if ((s_wdt_status.reset_streak > 0U) && !s_wdt_status.err04_latched &&
        (s_wdt_status.stable_uptime_ms >= WDT_STABLE_UPTIME_CLEAR_MS)) {
        uint32_t clean = wdt_pack_persist_reg(WDT_SRC_NONE, 0U, 0U, false, false);
        wdt_write_persist_reg(clean);
        wdt_sync_status_from_reg(clean);
        wdt_refresh_severity();
        printf("[WDT] Sistem stabil %u ms: reset streak glitch dibersihkan.\r\n",
               (unsigned int)WDT_STABLE_UPTIME_CLEAR_MS);
    }

    /* Meski ERR04 sudah ter-latch pasca reboot, tetap segarkan PD12 agar layar/DLMS bisa dibaca & di-clear */
    return wcet_watchdog_supervise_and_feed();
}

/* ========================================================================== */
/*                   DLMS TAMPER LOG PROFILE & OBIS HELPERS                   */
/* ========================================================================== */

dlms_result_t dlms_tamper_log_init(dlms_tamper_log_t *log) {
    if (log == NULL) return DLMS_ERR_NULL_PTR;
    memset(log, 0, sizeof(*log));
    return DLMS_OK;
}

dlms_result_t dlms_tamper_log_add_event(dlms_tamper_log_t *log, uint32_t timestamp_epoch, dlms_tamper_code_t code, uint8_t status) {
    if (log == NULL) return DLMS_ERR_NULL_PTR;
    log->records[log->head].timestamp_epoch = timestamp_epoch;
    log->records[log->head].event_code      = (uint8_t)code;
    log->records[log->head].status          = status;
    log->head = (log->head + 1U) % DLMS_TAMPER_LOG_MAX_ENTRIES;
    if (log->entries_in_use < DLMS_TAMPER_LOG_MAX_ENTRIES) {
        log->entries_in_use++;
    }
    log->total_events++;
    return DLMS_OK;
}

dlms_result_t dlms_tamper_log_get_entry(const dlms_tamper_log_t *log, size_t index, dlms_tamper_record_t *out_record) {
    if (log == NULL || out_record == NULL) return DLMS_ERR_NULL_PTR;
    if (index >= log->entries_in_use)      return DLMS_ERR_BUFFER_EMPTY;

    size_t oldest_idx = 0U;
    if (log->entries_in_use == DLMS_TAMPER_LOG_MAX_ENTRIES) {
        oldest_idx = log->head;
    }
    size_t physical_idx = (oldest_idx + index) % DLMS_TAMPER_LOG_MAX_ENTRIES;
    *out_record = log->records[physical_idx];
    return DLMS_OK;
}

uint32_t dlms_obis_get_tamper_counter(dlms_tamper_code_t code) {
    if ((uint8_t)code < 16U) {
        return s_obis_tamper_counters[(uint8_t)code];
    }
    return 0U;
}

static void dlms_obis_increment_tamper_counter(dlms_tamper_code_t code) {
    if ((uint8_t)code < 16U) {
        s_obis_tamper_counters[(uint8_t)code]++;
    }
}

/* ========================================================================== */
/*            KEBIJAKAN TAMPER TER-LATCH PERMANEN & IDENTITAS METER           */
/* ========================================================================== */

bool rtos_tamper_is_latched_vector(tamper_vector_t vec) {
    return (((uint32_t)vec & TAMPER_LATCHED_MASK) != 0U);
}

void rtos_tamper_apply_latch(uint32_t vector_mask, uint32_t timestamp_sec) {
    rtos_app_state_lock();
    for (uint32_t bit = 0U; bit < 9U; bit++) {
        uint32_t vmask = (1U << bit);
        if ((vector_mask & vmask) != 0U) {
            tamper_vector_t vec = (tamper_vector_t)vmask;
            tamper_process_signal(&g_tamper_ctx, vec, true, timestamp_sec);
            if (rtos_tamper_is_latched_vector(vec)) {
                s_latched_tamper_mask |= vmask;
            }
        }
    }
    rtos_app_state_unlock();
}

bool rtos_tamper_evaluate_clear(tamper_vector_t vec, bool is_authorized_clear, uint32_t timestamp_sec) {
    bool cleared = false;
    uint32_t vmask = (uint32_t)vec;

    rtos_app_state_lock();
    if (!is_authorized_clear && ((vmask & TAMPER_LATCHED_MASK) != 0U) &&
        ((s_latched_tamper_mask & vmask) != 0U)) {
        /* Kebijakan SPLN & Latch Hardware: Tutup Meter (RUSAK) / Terminal (ERR20)
         * TIDAK BOLEH pulih otomatis tanpa otorisasi Clear Resmi PLN! */
        cleared = false;
    } else {
        s_latched_tamper_mask &= ~vmask;
        tamper_process_signal(&g_tamper_ctx, vec, false, timestamp_sec);
        cleared = true;
    }
    rtos_app_state_unlock();

    return cleared;
}

void rtos_tamper_clear_authorized(uint32_t clear_mask) {
    uint32_t now_sec = rtos_get_rtc_epoch_sec();

    rtos_app_state_lock();
    s_latched_tamper_mask &= ~clear_mask;
    for (uint32_t bit = 0U; bit < 9U; bit++) {
        uint32_t vmask = (1U << bit);
        if ((clear_mask & vmask) != 0U) {
            tamper_process_signal(&g_tamper_ctx, (tamper_vector_t)vmask, false, now_sec);
        }
    }
    if ((g_tamper_ctx.active_tamper_mask) == 0U) {
        display_task_trigger_instant_refresh(false);
    }
    rtos_app_state_unlock();

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t hw_clr = clear_mask & 0x07U;
    if (hw_clr != 0U) {
        SECURE_ClearTamperLatch(hw_clr);
    }
#endif
}

uint32_t rtos_tamper_get_latched_mask(void) {
    return s_latched_tamper_mask;
}

void rtos_set_device_ids(const char *customer_id, const char *meter_id) {
    rtos_app_state_lock();
    if (customer_id != NULL && customer_id[0] != '\0') {
        strncpy(s_rtos_customer_id, customer_id, sizeof(s_rtos_customer_id) - 1U);
        s_rtos_customer_id[sizeof(s_rtos_customer_id) - 1U] = '\0';
    }
    if (meter_id != NULL && meter_id[0] != '\0') {
        strncpy(s_rtos_meter_id, meter_id, sizeof(s_rtos_meter_id) - 1U);
        s_rtos_meter_id[sizeof(s_rtos_meter_id) - 1U] = '\0';
    }
    rtos_app_state_unlock();
}

void rtos_get_device_ids(char *out_customer_id, size_t cust_sz, char *out_meter_id, size_t meter_sz) {
    rtos_app_state_lock();
    if (out_customer_id != NULL && cust_sz > 0U) {
        strncpy(out_customer_id, s_rtos_customer_id, cust_sz - 1U);
        out_customer_id[cust_sz - 1U] = '\0';
    }
    if (out_meter_id != NULL && meter_sz > 0U) {
        strncpy(out_meter_id, s_rtos_meter_id, meter_sz - 1U);
        out_meter_id[meter_sz - 1U] = '\0';
    }
    rtos_app_state_unlock();
}

/* ========================================================================== */
/*                           INISIALISASI KERNEL IPC                          */
/* ========================================================================== */

void rtos_system_init(void) {
    tamper_init(&g_tamper_ctx);
    load_profile_init(&g_load_profile_buf);
    display_init(&g_display_ctx, s_rtos_customer_id, s_rtos_meter_id, 5000U);
    power_relay_init(&g_power_relay_ctx, 11000U, 5U, 30U);

    s_latched_tamper_mask = 0U;
    s_internal_alarm_mask_mirror = 0U;
    s_power_event_bits_mirror = 0U;
    s_modem_state = MODEM_STATE_INIT;
    s_watchdog_kicked_mask = 0U;

    for (size_t i = 0; i < (sizeof(s_liveness_table) / sizeof(s_liveness_table[0])); i++) {
        s_liveness_table[i].last_kick_tick_ms = 0U;
        s_liveness_table[i].has_kicked_once   = false;
        s_liveness_table[i].is_timed_out      = false;
    }

    rtos_watchdog_on_boot();

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    xTamperQueue     = xQueueCreate(TAMPER_QUEUE_MAX_ITEMS, sizeof(tamper_event_msg_t));
    xStorageQueue    = xQueueCreate(STORAGE_QUEUE_MAX_ITEMS, sizeof(storage_write_req_t));
    xModemTxQueue    = xQueueCreate(MODEM_TX_QUEUE_MAX_ITEMS, sizeof(modem_push_msg_t));
    xMeasMutex       = xSemaphoreCreateMutex();
    xAppStateMutex   = xSemaphoreCreateMutex();
    xSpiFlashMutex   = xSemaphoreCreateMutex();
    xPowerEventGroup = xEventGroupCreate();
#else
    q_head = 0U; q_tail = 0U; q_count = 0U;
    sq_head = 0U; sq_tail = 0U; sq_count = 0U;
    mq_head = 0U; mq_tail = 0U; mq_count = 0U;
#endif

    nvram_load_tamper_log_snapshot(&g_tamper_ctx);
    /* Sinkronkan latch mask jika ada tamper fatal yang terbaca dari snapshot NVRAM */
    if ((g_tamper_ctx.active_tamper_mask & TAMPER_LATCHED_MASK) != 0U) {
        s_latched_tamper_mask |= (g_tamper_ctx.active_tamper_mask & TAMPER_LATCHED_MASK);
    }

    memset(&s_dlms_task_ctx, 0, sizeof(s_dlms_task_ctx));
    (void)dlms_tamper_log_init(&s_dlms_task_ctx.server.tamper_log);
    s_dlms_task_ctx.is_running = true;
    s_dlms_task_ctx.tx_cb = s_dlms_uart_tx_cb;
    memset(&s_ami_server, 0, sizeof(s_ami_server));
    memset(s_obis_tamper_counters, 0, sizeof(s_obis_tamper_counters));

    printf("[RTOS] 8-Task Preemptive System, 3 Queues, 3 Mutexes & EventGroup Initialized!\r\n");
}

/* ========================================================================== */
/*                           QUEUE & SYNCHRONIZATION APIS                     */
/* ========================================================================== */

bool rtos_queue_send_tamper_event(const tamper_event_msg_t *msg) {
    if (msg == NULL) return false;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xTamperQueue == NULL) return false;
    return (xQueueSend(xTamperQueue, msg, pdMS_TO_TICKS(10)) == pdPASS);
#else
    if (q_count >= TAMPER_QUEUE_MAX_ITEMS) return false;
    simulated_tamper_queue[q_head] = *msg;
    q_head = (q_head + 1U) % TAMPER_QUEUE_MAX_ITEMS;
    q_count++;
    return true;
#endif
}

bool rtos_queue_send_tamper_event_from_isr(const tamper_event_msg_t *msg) {
    if (msg == NULL) return false;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xTamperQueue == NULL) return false;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t res = xQueueSendFromISR(xTamperQueue, msg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return (res == pdPASS);
#else
    return rtos_queue_send_tamper_event(msg);
#endif
}

bool rtos_queue_receive_tamper_event(tamper_event_msg_t *out_msg, uint32_t timeout_ms) {
    (void)timeout_ms;
    if (out_msg == NULL) return false;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xTamperQueue == NULL) return false;
    return (xQueueReceive(xTamperQueue, out_msg, pdMS_TO_TICKS(timeout_ms)) == pdPASS);
#else
    if (q_count == 0U) return false;
    *out_msg = simulated_tamper_queue[q_tail];
    q_tail = (q_tail + 1U) % TAMPER_QUEUE_MAX_ITEMS;
    q_count--;
    return true;
#endif
}

bool rtos_queue_send_storage_req(const storage_write_req_t *req) {
    if (req == NULL) return false;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xStorageQueue == NULL) return false;
    return (xQueueSend(xStorageQueue, req, pdMS_TO_TICKS(10)) == pdPASS);
#else
    if (sq_count >= STORAGE_QUEUE_MAX_ITEMS) return false;
    simulated_storage_queue[sq_head] = *req;
    sq_head = (sq_head + 1U) % STORAGE_QUEUE_MAX_ITEMS;
    sq_count++;
    return true;
#endif
}

bool rtos_queue_receive_storage_req(storage_write_req_t *out_req, uint32_t timeout_ms) {
    (void)timeout_ms;
    if (out_req == NULL) return false;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xStorageQueue == NULL) return false;
    return (xQueueReceive(xStorageQueue, out_req, pdMS_TO_TICKS(timeout_ms)) == pdPASS);
#else
    if (sq_count == 0U) return false;
    *out_req = simulated_storage_queue[sq_tail];
    sq_tail = (sq_tail + 1U) % STORAGE_QUEUE_MAX_ITEMS;
    sq_count--;
    return true;
#endif
}

bool rtos_queue_send_modem_push(const modem_push_msg_t *msg) {
    if (msg == NULL) return false;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xModemTxQueue == NULL) return false;
    return (xQueueSend(xModemTxQueue, msg, pdMS_TO_TICKS(10)) == pdPASS);
#else
    if (mq_count >= MODEM_TX_QUEUE_MAX_ITEMS) return false;
    simulated_modem_queue[mq_head] = *msg;
    mq_head = (mq_head + 1U) % MODEM_TX_QUEUE_MAX_ITEMS;
    mq_count++;
    return true;
#endif
}

bool rtos_queue_send_modem_push_from_isr(const modem_push_msg_t *msg) {
    if (msg == NULL) return false;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xModemTxQueue == NULL) return false;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t res = xQueueSendFromISR(xModemTxQueue, msg, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    return (res == pdPASS);
#else
    return rtos_queue_send_modem_push(msg);
#endif
}

bool rtos_queue_receive_modem_push(modem_push_msg_t *out_msg, uint32_t timeout_ms) {
    (void)timeout_ms;
    if (out_msg == NULL) return false;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xModemTxQueue == NULL) return false;
    return (xQueueReceive(xModemTxQueue, out_msg, pdMS_TO_TICKS(timeout_ms)) == pdPASS);
#else
    if (mq_count == 0U) return false;
    *out_msg = simulated_modem_queue[mq_tail];
    mq_tail = (mq_tail + 1U) % MODEM_TX_QUEUE_MAX_ITEMS;
    mq_count--;
    return true;
#endif
}

/* ========================================================================== */
/*             THREAD-SAFE MUTEXES & POWER EVENT GROUP (IPC)                  */
/* ========================================================================== */

void rtos_metrology_set_sample_cb(meter_sample_fn_t cb)         { s_meter_sample_cb = cb; }
void rtos_profile_set_1s_cb(rtos_meas_hook_fn_t cb)             { s_profile_1s_cb = cb; }
void rtos_checkpoint_set_build_cb(rtos_checkpoint_build_fn_t cb){ s_checkpoint_build_cb = cb; }
void rtos_health_set_check_cb(rtos_simple_hook_fn_t cb)         { s_health_check_cb = cb; }
void rtos_modem_set_step_cb(rtos_modem_step_fn_t cb)            { s_modem_step_cb = cb; }
void rtos_dlms_set_optical_poll_cb(rtos_simple_hook_fn_t cb)    { s_optical_poll_cb = cb; }
void rtos_relay_set_actuate_cb(rtos_elapsed_hook_fn_t cb)       { s_relay_actuate_cb = cb; }
void rtos_power_set_stop2_cb(rtos_elapsed_hook_fn_t cb)         { s_power_stop2_cb = cb; }

void rtos_meter_data_publish(const meter_measurements_t *meas) {
    if (meas == NULL) return;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xMeasMutex != NULL) {
        xSemaphoreTake(xMeasMutex, portMAX_DELAY);
    }
#endif
    s_latest_measurements = *meas;
    s_measurements_valid = true;
    s_ami_server.latest_meas = *meas;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xMeasMutex != NULL) {
        xSemaphoreGive(xMeasMutex);
    }
#endif
}

bool rtos_meter_data_get_snapshot(meter_measurements_t *out_meas) {
    if (out_meas == NULL || !s_measurements_valid) return false;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xMeasMutex != NULL) {
        xSemaphoreTake(xMeasMutex, portMAX_DELAY);
    }
#endif
    *out_meas = s_latest_measurements;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xMeasMutex != NULL) {
        xSemaphoreGive(xMeasMutex);
    }
#endif
    return true;
}

void rtos_meter_data_lock(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xMeasMutex != NULL) xSemaphoreTake(xMeasMutex, portMAX_DELAY);
#endif
}

void rtos_meter_data_unlock(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xMeasMutex != NULL) xSemaphoreGive(xMeasMutex);
#endif
}

void rtos_app_state_lock(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xAppStateMutex != NULL && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        xSemaphoreTake(xAppStateMutex, portMAX_DELAY);
    }
#endif
}

void rtos_app_state_unlock(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xAppStateMutex != NULL && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        xSemaphoreGive(xAppStateMutex);
    }
#endif
}

void rtos_flash_lock(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xSpiFlashMutex != NULL && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        xSemaphoreTake(xSpiFlashMutex, portMAX_DELAY);
    }
#endif
}

void rtos_flash_unlock(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xSpiFlashMutex != NULL && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        xSemaphoreGive(xSpiFlashMutex);
    }
#endif
}

void rtos_power_set_event_bits(uint32_t bits) {
    s_power_event_bits_mirror |= bits;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xPowerEventGroup != NULL) {
#if defined(EMBEDDED_HARDWARE_TARGET)
        if (__get_IPSR() != 0U) {
            BaseType_t woken = pdFALSE;
            xEventGroupSetBitsFromISR(xPowerEventGroup, (EventBits_t)bits, &woken);
            portYIELD_FROM_ISR(woken);
            return;
        }
#endif
        xEventGroupSetBits(xPowerEventGroup, (EventBits_t)bits);
    }
#endif
}

void rtos_power_clear_event_bits(uint32_t bits) {
    s_power_event_bits_mirror &= ~bits;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xPowerEventGroup != NULL) {
#if defined(EMBEDDED_HARDWARE_TARGET)
        if (__get_IPSR() != 0U) {
            xEventGroupClearBitsFromISR(xPowerEventGroup, (EventBits_t)bits);
            return;
        }
#endif
        xEventGroupClearBits(xPowerEventGroup, (EventBits_t)bits);
    }
#endif
}

uint32_t rtos_power_get_event_bits(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xPowerEventGroup != NULL) {
        return (uint32_t)xEventGroupGetBits(xPowerEventGroup);
    }
#endif
    return s_power_event_bits_mirror;
}

void rtos_power_notify_ac_fail(bool is_failed) {
    if (is_failed) {
        rtos_power_clear_event_bits(PWR_EVT_EMERGENCY_SAVED | PWR_EVT_ENTER_STOP2);
        rtos_power_set_event_bits(PWR_EVT_AC_FAIL_BIT);
    } else {
        rtos_power_clear_event_bits(PWR_EVT_AC_FAIL_BIT | PWR_EVT_EMERGENCY_SAVED | PWR_EVT_ENTER_STOP2 | PWR_EVT_USER_WAKEUP_10S);
    }
}

/* ========================================================================== */
/*                    TASK 1: TAMPER EMERGENCY & LAST-GASP                    */
/* ========================================================================== */

static void dispatch_tamper_transition(uint32_t now_sec, dlms_tamper_code_t code, bool is_active) {
    static storage_write_req_t s_tamper_sreq;
    tamper_event_msg_t msg = {
        .timestamp   = now_sec,
        .tamper_code = (uint8_t)code,
        .is_active   = is_active
    };
    (void)rtos_queue_send_tamper_event(&msg);

    modem_push_msg_t push = {
        .type             = MODEM_PUSH_TAMPER_ALERT,
        .timestamp        = now_sec,
        .active_energy_wh = s_latest_measurements.active_energy_wh,
        .tamper_code      = (uint8_t)code,
        .is_active        = is_active
    };
    (void)rtos_queue_send_modem_push(&push);

    memset(&s_tamper_sreq, 0, sizeof(s_tamper_sreq));
    s_tamper_sreq.type = STORAGE_REQ_TAMPER_LOG;
    (void)rtos_queue_send_storage_req(&s_tamper_sreq);
}

void task_tamper_emergency_entry(void *pvParameters) {
    (void)pvParameters;
#if defined(EMBEDDED_HARDWARE_TARGET)
    portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);
#endif
    printf("[TAMPER] Task 1 (Pri 32, 100 ms) aktif. Latch hardware & Last-Gasp siap.\r\n");

    uint32_t last_effective_mask = 0U;
    static bool     s_hw_mag_asserted = false;
    static uint32_t s_mag_clear_ms    = 0U;
    static nvram_energy_checkpoint_t s_lg_cp;
    static meter_measurements_t      s_lg_snap;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    TickType_t xLastWakeTime = xTaskGetTickCount();
#endif

    for (;;) {
        uint32_t now_sec = rtos_get_rtc_epoch_sec();
        uint32_t pwr_bits = rtos_power_get_event_bits();

        /* 1. Cek Darurat PLN Padam (AC_FAIL_DET -> Prosedur Last-Gasp < 15 ms) */
        if (((pwr_bits & PWR_EVT_AC_FAIL_BIT) != 0U) && ((pwr_bits & PWR_EVT_EMERGENCY_SAVED) == 0U)) {
            memset(&s_lg_cp, 0, sizeof(s_lg_cp));
            if (s_checkpoint_build_cb != NULL) {
                if (rtos_meter_data_get_snapshot(&s_lg_snap)) {
                    s_checkpoint_build_cb(&s_lg_cp, &s_lg_snap);
                }
            } else {
                s_lg_cp.magic     = NVRAM_ENERGY_CHECKPOINT_MAGIC;
                s_lg_cp.timestamp = now_sec;
                s_lg_cp.active_energy_import_wh = s_latest_measurements.active_energy_wh;
            }
            rtos_flash_lock();
            (void)nvram_save_energy_checkpoint(&s_lg_cp);
            (void)nvram_save_tamper_log_snapshot(&g_tamper_ctx);
            rtos_flash_unlock();

            modem_push_msg_t lg_push = {
                .type             = MODEM_PUSH_LAST_GASP,
                .timestamp        = now_sec,
                .active_energy_wh = s_lg_cp.active_energy_import_wh,
                .tamper_code      = 0x50U,
                .is_active        = true
            };
            (void)rtos_queue_send_modem_push(&lg_push);
            rtos_power_set_event_bits(PWR_EVT_EMERGENCY_SAVED | PWR_EVT_ENTER_STOP2);
        }

        /* 2. Baca Pin Fisik & Register Latch Hardware via TrustZone NSC */
        uint32_t raw_hw_mask = 0U;
        bool btn_alarm = display_task_is_alarm_active();
        if (btn_alarm) {
            raw_hw_mask |= (uint32_t)TAMPER_VECTOR_CASE_OPEN;
        }

#if defined(EMBEDDED_HARDWARE_TARGET)
        uint32_t sec_tamp_mask = SECURE_GetTamperStatusMask();
        if ((sec_tamp_mask & (1U << 0)) != 0U) raw_hw_mask |= (uint32_t)TAMPER_VECTOR_CASE_OPEN;
        if ((sec_tamp_mask & (1U << 1)) != 0U) raw_hw_mask |= (uint32_t)TAMPER_VECTOR_TERMINAL_OPEN;
        if ((sec_tamp_mask & (1U << 2)) != 0U) raw_hw_mask |= (uint32_t)TAMPER_VECTOR_MAGNETIC_FIELD;
#endif

        /* Terapkan kebijakan Latch Permanen (Tutup Meter & Terminal tidak pernah auto-recover) */
        if ((raw_hw_mask & (uint32_t)TAMPER_VECTOR_CASE_OPEN) != 0U) {
            rtos_tamper_apply_latch((uint32_t)TAMPER_VECTOR_CASE_OPEN, now_sec);
        }
        if ((raw_hw_mask & (uint32_t)TAMPER_VECTOR_TERMINAL_OPEN) != 0U) {
            rtos_tamper_apply_latch((uint32_t)TAMPER_VECTOR_TERMINAL_OPEN, now_sec);
        }

        /* Evaluasi Medan Magnet (ERR25): Boleh auto-recover 10 detik setelah magnet dijauhkan */
        if ((raw_hw_mask & (uint32_t)TAMPER_VECTOR_MAGNETIC_FIELD) != 0U) {
            s_hw_mag_asserted = true;
            s_mag_clear_ms = 0U;
            rtos_tamper_apply_latch((uint32_t)TAMPER_VECTOR_MAGNETIC_FIELD, now_sec);
        } else if (s_hw_mag_asserted) {
            s_mag_clear_ms += 100U;
            if (s_mag_clear_ms >= BOARD_CFG_TAMPER_MAG_CLEAR_MS) {
                s_hw_mag_asserted = false;
                s_mag_clear_ms = 0U;
                (void)rtos_tamper_evaluate_clear(TAMPER_VECTOR_MAGNETIC_FIELD, false, now_sec);
            }
        }

        /* Gabungkan dengan state tamper sinkron (termasuk vektor elektrikal & Clear resmi) */
        rtos_app_state_lock();
        uint32_t curr_mask = tamper_get_active_mask(&g_tamper_ctx) | s_latched_tamper_mask;
#if defined(EMBEDDED_HARDWARE_TARGET)
        const tamper_context_t *e3_tctx = e3_meter_app_get_tamper_context();
        if (e3_tctx != NULL) {
            curr_mask |= tamper_get_active_mask(e3_tctx);
        }
#endif
        rtos_app_state_unlock();

        if (curr_mask != last_effective_mask) {
            uint32_t rising  = curr_mask & (~last_effective_mask);
            uint32_t falling = last_effective_mask & (~curr_mask);

            /* Sabotase baru terdeteksi */
            if (rising != 0U) {
                bool fatal_trip = ((rising & TAMPER_MASK_RELAY_TRIP) != 0U);
                if (fatal_trip) {
                    printf("[TAMPER] *** DARURAT! SABOTASE LATCH/TRIP (Mask: 0x%04lX) -> RELAI DIPUTUS! ***\r\n",
                           (unsigned long)rising);
                    rtos_app_state_lock();
                    power_relay_disconnect(&g_power_relay_ctx, RELAY_REASON_MANUAL, now_sec);
                    rtos_app_state_unlock();
                } else {
                    printf("[TAMPER] Sabotase Non-Trip terdeteksi (Mask: 0x%04lX). Relai tetap tertutup.\r\n",
                           (unsigned long)rising);
                }
                display_task_trigger_instant_refresh(true);

                for (uint32_t bit = 0U; bit < 9U; bit++) {
                    uint32_t vmask = (1U << bit);
                    if ((rising & vmask) != 0U) {
                        dispatch_tamper_transition(now_sec, tamper_vector_to_dlms_code((tamper_vector_t)vmask), true);
                    }
                }
            }

            /* Pemulihan hanya terjadi untuk non-latch atau setelah Clear resmi PLN */
            if (falling != 0U) {
                for (uint32_t bit = 0U; bit < 9U; bit++) {
                    uint32_t vmask = (1U << bit);
                    if ((falling & vmask) != 0U) {
                        tamper_vector_t vec = (tamper_vector_t)vmask;
                        if (rtos_tamper_is_latched_vector(vec) && ((s_latched_tamper_mask & vmask) != 0U)) {
                            printf("[TAMPER] Penolakan auto-recover pada vektor ter-latch 0x%04lX!\r\n",
                                   (unsigned long)vmask);
                        } else {
                            dispatch_tamper_transition(now_sec, tamper_vector_to_dlms_code(vec), false);
                        }
                    }
                }
                if (curr_mask == 0U) {
                    printf("[TAMPER] Seluruh sabotase pulih / dibersihkan secara resmi.\r\n");
                    display_task_trigger_instant_refresh(false);
                }
            }
            last_effective_mask = curr_mask;
        }

        /* Eksekusi penggerak fisik pulsa 100 ms relai & LED kuning SPLN */
        if (s_relay_actuate_cb != NULL) {
            s_relay_actuate_cb(100U);
        }

        bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT);
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
#else
        vTaskDelay(pdMS_TO_TICKS(100));
#endif
    }
}

/* ========================================================================== */
/*       TASK 2: METROLOGY, PROTEKSI TEGANGAN, MD & PROFILING (1000 ms)       */
/* ========================================================================== */

void task_metrology_profiling_entry(void *pvParameters) {
    (void)pvParameters;
#if defined(EMBEDDED_HARDWARE_TARGET)
    portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);
#endif
    printf("[PROFILE] Task 2 (Pri 28, 1000 ms) aktif. ADE9000, Prot V 184-276V, MD & LP.\r\n");

    static meter_measurements_t s_prof_meas;
    static storage_write_req_t  s_prof_req;
    static load_profile_entry_t s_prof_lp_entry;
    memset(&s_prof_meas, 0, sizeof(s_prof_meas));
    uint32_t profile_interval_sec = 0U;
    uint32_t checkpoint_timer_sec = 0U;
    uint32_t last_lp_boundary = (rtos_get_rtc_epoch_sec() + BOARD_CFG_LOCAL_TIMEZONE_SEC) / 900U;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    TickType_t xLastWakeTime = xTaskGetTickCount();
#endif

    for (;;) {
        /* 1. Akuisisi ADE9000 & Synthetic Load Callback */
        if (s_meter_sample_cb != NULL) {
            s_meter_sample_cb(&s_prof_meas);
            rtos_meter_data_publish(&s_prof_meas);
        }

        uint32_t now_sec = rtos_get_rtc_epoch_sec();

        /* 2. Evaluasi ToU, Asimetri, Proteksi Tegangan 184-276V, Power Limiter & Max Demand */
        rtos_app_state_lock();
        bool asym = power_relay_check_asymmetry(&g_power_relay_ctx, s_prof_meas.current_r_mamps, s_prof_meas.current_s_mamps, s_prof_meas.current_t_mamps);
        if (asym) {
            printf("[PROFILE] Peringatan Asimetri Beban! Deviasi: %lu%%\r\n",
                   (unsigned long)g_power_relay_ctx.current_deviation_pct);
        }

        (void)power_relay_process_voltage(&g_power_relay_ctx,
                                          s_prof_meas.voltage_r_dvolts,
                                          s_prof_meas.voltage_s_dvolts,
                                          s_prof_meas.voltage_t_dvolts,
                                          now_sec);

        bool tripped = power_relay_process_power(&g_power_relay_ctx, s_prof_meas.active_power_w, now_sec);
        if (tripped) {
            printf("[LIMITER] *** OVERLOAD (%ld W > %lu W)! Relai TRIP! ***\r\n",
                   (long)s_prof_meas.active_power_w, (unsigned long)g_power_relay_ctx.power_limit_w);
        }

        if (s_profile_1s_cb != NULL) {
            s_profile_1s_cb(&s_prof_meas);
        }
        rtos_app_state_unlock();

        /* 3. Timer 300 Detik (5 Menit): Kirim Checkpoint Energi ke StorageTask (Non-Blocking, Wear-Leveled >120 Tahun) */
        checkpoint_timer_sec++;
        if (checkpoint_timer_sec >= 300U) {
            checkpoint_timer_sec = 0U;
            memset(&s_prof_req, 0, sizeof(s_prof_req));
            s_prof_req.type = STORAGE_REQ_ENERGY_CHECKPOINT;
            if (s_checkpoint_build_cb != NULL) {
                s_checkpoint_build_cb(&s_prof_req.payload.checkpoint, &s_prof_meas);
            } else {
                s_prof_req.payload.checkpoint.magic = NVRAM_ENERGY_CHECKPOINT_MAGIC;
                s_prof_req.payload.checkpoint.timestamp = now_sec;
                s_prof_req.payload.checkpoint.active_energy_import_wh = s_prof_meas.active_energy_wh;
                s_prof_req.payload.checkpoint.active_energy_export_wh = s_prof_meas.active_energy_import_wh;
                s_prof_req.payload.checkpoint.reactive_energy_varh    = s_prof_meas.reactive_energy_varh;
            }
            (void)rtos_queue_send_storage_req(&s_prof_req);
        }

        /* 4. Timer 15 Menit (900 Detik Tersinkron RTC): Bekukan 23-Kanal Load Profile ke StorageTask */
        uint32_t cur_lp_boundary = (now_sec + BOARD_CFG_LOCAL_TIMEZONE_SEC) / 900U;
        profile_interval_sec++;
        if ((cur_lp_boundary > last_lp_boundary) || (profile_interval_sec >= 900U)) {
            last_lp_boundary = cur_lp_boundary;
            profile_interval_sec = 0U;
            memset(&s_prof_lp_entry, 0, sizeof(s_prof_lp_entry));
            s_prof_lp_entry.timestamp                 = now_sec;
            s_prof_lp_entry.voltage_r_dvolts          = (uint16_t)s_prof_meas.voltage_r_dvolts;
            s_prof_lp_entry.voltage_s_dvolts          = (uint16_t)s_prof_meas.voltage_s_dvolts;
            s_prof_lp_entry.voltage_t_dvolts          = (uint16_t)s_prof_meas.voltage_t_dvolts;
            s_prof_lp_entry.current_r_mamps           = s_prof_meas.current_r_mamps;
            s_prof_lp_entry.current_s_mamps           = s_prof_meas.current_s_mamps;
            s_prof_lp_entry.current_t_mamps           = s_prof_meas.current_t_mamps;
            s_prof_lp_entry.current_n_mamps           = s_prof_meas.current_n_mamps;
            s_prof_lp_entry.active_power_w            = s_prof_meas.active_power_w;
            s_prof_lp_entry.reactive_power_var        = s_prof_meas.reactive_power_var;
            s_prof_lp_entry.power_factor_permille     = s_prof_meas.power_factor_ppm;
            s_prof_lp_entry.active_energy_import_wh   = s_prof_meas.active_energy_wh;
            s_prof_lp_entry.active_energy_export_wh   = s_prof_meas.active_energy_import_wh;
            s_prof_lp_entry.reactive_energy_import_varh = s_prof_meas.reactive_energy_varh;

            rtos_app_state_lock();
            s_prof_lp_entry.status_mask = tamper_get_active_mask(&g_tamper_ctx) | s_latched_tamper_mask;
            load_profile_add_entry(&g_load_profile_buf, &s_prof_lp_entry);
            uint32_t rec_idx = (g_load_profile_buf.total_written > 0U)
                               ? (g_load_profile_buf.total_written - 1U)
                               : 0U;
            rtos_app_state_unlock();

            memset(&s_prof_req, 0, sizeof(s_prof_req));
            s_prof_req.type = STORAGE_REQ_LOAD_PROFILE;
            s_prof_req.index = rec_idx;
            s_prof_req.payload.load_profile = s_prof_lp_entry;
            (void)rtos_queue_send_storage_req(&s_prof_req);
            printf("[PROFILE] Record Profil Beban 15-Menit dikirim ke StorageTask.\r\n");
        }

        bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
#else
        vTaskDelay(pdMS_TO_TICKS(1000));
#endif
    }
}

/* ========================================================================== */
/*             TASK 3: SERVER DLMS/COSEM & PORT OPTIK (50 ms)                 */
/* ========================================================================== */

void rtos_dlms_task_set_tx_cb(dlms_uart_tx_fn_t tx_cb) {
    s_dlms_uart_tx_cb = tx_cb;
    s_dlms_task_ctx.tx_cb = tx_cb;
}

void rtos_dlms_task_notify_rx(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
}

void rtos_dlms_process_tamper_event(const tamper_event_msg_t *msg) {
    if (!msg) return;

    dlms_tamper_code_t dcode = (dlms_tamper_code_t)msg->tamper_code;
    uint8_t status = msg->is_active ? 1U : 0U;

    /* 1. Catat ke DLMS Tamper Log Profile Generic Class 7 (0.0.99.98.0.255) */
    (void)dlms_tamper_log_add_event(&s_dlms_task_ctx.server.tamper_log, msg->timestamp, dcode, status);
    s_ami_server.last_tamper_epoch  = msg->timestamp;
    s_ami_server.last_tamper_code   = (uint8_t)dcode;
    s_ami_server.last_tamper_status = status;

    /* 2. Inkremen Register OBIS Tamper Counter */
    if (msg->is_active) {
        dlms_obis_increment_tamper_counter(dcode);
    }

    /* 3. Sinkronkan ke tamper_context_t (tanpa menimpa kebijakan latch) */
    tamper_vector_t vec = dlms_code_to_tamper_vector(dcode);
    if (msg->is_active) {
        rtos_tamper_apply_latch((uint32_t)vec, msg->timestamp);
    } else {
        (void)rtos_tamper_evaluate_clear(vec, false, msg->timestamp);
    }
}

dlms_task_ctx_t* rtos_dlms_get_task_ctx(void) { return &s_dlms_task_ctx; }
ami_server_t* rtos_ami_get_server(void) { return &s_ami_server; }
tamper_context_t* rtos_tamper_get_ctx(void) { return &g_tamper_ctx; }
power_relay_context_t* rtos_power_relay_get_ctx(void) { return &g_power_relay_ctx; }
load_profile_ctx_t* rtos_load_profile_get_ctx(void) { return &g_load_profile_buf; }
modem_fsm_state_t rtos_modem_get_state(void) { return s_modem_state; }

void task_dlms_entry(void *pvParameters) {
    (void)pvParameters;
#if defined(EMBEDDED_HARDWARE_TARGET)
    portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);
#endif
    printf("[DLMS] Task 3 (Pri 24, 50 ms) aktif. Class 7 Logbook, OBIS & Port Optik.\r\n");

    if (!s_dlms_task_ctx.is_running) {
        s_dlms_task_ctx.is_running = true;
    }

    tamper_event_msg_t event_msg;
#if defined(EMBEDDED_HARDWARE_TARGET)
    static meter_measurements_t s_dlms_meas_snap;
#endif
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    TickType_t xLastWakeTime = xTaskGetTickCount();
#endif
    for (;;) {
        /* 1. Konsumsi seluruh event sabotase dari xTamperQueue */
        while (rtos_queue_receive_tamper_event(&event_msg, 0)) {
            rtos_dlms_process_tamper_event(&event_msg);
        }

#if defined(EMBEDDED_HARDWARE_TARGET)
        /* 2. Sinkronisasi snapshot data metrologi ke Kamus OBIS */
        if (rtos_meter_data_get_snapshot(&s_dlms_meas_snap)) {
            e2_dlms_service_sync_snapshot(e2_dlms_service_get_snapshot(), &s_dlms_meas_snap);
        }
#endif

        /* 3. Layanan Jabat Tangan Mode E (300->9600 bps) & HDLC Port Optik */
        if (s_optical_poll_cb != NULL) {
            s_optical_poll_cb();
        }

        bsp_watchdog_kick_task(WATCHDOG_TASK_DLMS_BIT);
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
#else
        vTaskDelay(pdMS_TO_TICKS(50));
#endif
    }
}

/* ========================================================================== */
/*       TASK 4: MODEM SELULER 4G & TCP WRAPPER HES [BARU] (500 ms)           */
/* ========================================================================== */

void task_modem_cellular_entry(void *pvParameters) {
    (void)pvParameters;
    printf("[MODEM] Task 4 (Pri 20, 500 ms) aktif. FSM AT-Cmd 4G & xModemTxQueue.\r\n");

    s_modem_state = MODEM_STATE_INIT;
    modem_push_msg_t push_msg;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    TickType_t xLastWakeTime = xTaskGetTickCount();
#endif

    for (;;) {
        bool has_push = rtos_queue_receive_modem_push(&push_msg, 0);

        if (s_modem_step_cb != NULL) {
            (void)s_modem_step_cb(&s_modem_state, has_push ? &push_msg : NULL);
        } else {
            /* Default lightweight state progression saat modem eksternal tidak terhubung */
            if (s_modem_state == MODEM_STATE_INIT) {
                s_modem_state = MODEM_STATE_CONNECTING;
            } else if (s_modem_state == MODEM_STATE_CONNECTING) {
                s_modem_state = MODEM_STATE_READY;
            }
#if defined(EMBEDDED_HARDWARE_TARGET)
            if (has_push) {
                (void)e2_dlms_send_last_gasp_push(push_msg.timestamp, push_msg.active_energy_wh);
            }
#endif
        }

        bsp_watchdog_kick_task(WATCHDOG_TASK_MODEM_BIT);
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(500));
#else
        vTaskDelay(pdMS_TO_TICKS(500));
#endif
    }
}

/* ========================================================================== */
/*                  TASK 5: UI DISPLAY WRAPPER (SPLN 128x32)                  */
/* ========================================================================== */

void task_ui_display_entry(void *pvParameters) {
    task_oled128x32_carousel(pvParameters);
}

/* ========================================================================== */
/*        TASK 6: TELEMETRI ADC4, LSE & SELF-TEST [BARU] (5000 ms)            */
/* ========================================================================== */

void task_system_health_entry(void *pvParameters) {
    (void)pvParameters;
#if defined(EMBEDDED_HARDWARE_TARGET)
    portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);
#endif
    printf("[HEALTH] Task 6 (Pri 12, 5000 ms) aktif. Monitor ADC4 VBAT/VCAP, LSE & Relai FB.\r\n");
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    TickType_t xLastWakeTime = xTaskGetTickCount();
#endif

    for (;;) {
        if (s_health_check_cb != NULL) {
            s_health_check_cb();
        }
        bsp_watchdog_kick_task(WATCHDOG_TASK_HEALTH_BIT);
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(5000));
#else
        vTaskDelay(pdMS_TO_TICKS(5000));
#endif
    }
}

/* ========================================================================== */
/*     TASK 7: WORKER ASYNCHRONOUS SPI FLASH W25Q64 [BARU] (Queue 1000 ms)    */
/* ========================================================================== */

void task_storage_logger_entry(void *pvParameters) {
    (void)pvParameters;
    printf("[STORAGE] Task 7 (Pri 10, Queue 1000 ms) aktif. Worker Flash Winbond W25Q64.\r\n");

    static storage_write_req_t s_storage_req;
    static tamper_context_t    s_storage_tamper_snap;
    for (;;) {
        /* Tendang detak hidup sebelum menunggu antrean agar saat Flash idle watchdog aman */
        bsp_watchdog_kick_task(WATCHDOG_TASK_STORAGE_BIT);

        if (rtos_queue_receive_storage_req(&s_storage_req, 1000U)) {
            rtos_flash_lock();
            switch (s_storage_req.type) {
                case STORAGE_REQ_ENERGY_CHECKPOINT:
                    (void)nvram_save_energy_checkpoint(&s_storage_req.payload.checkpoint);
                    break;
                case STORAGE_REQ_LOAD_PROFILE:
                    (void)nvram_write_load_profile_flash_slot(s_storage_req.index, &s_storage_req.payload.load_profile);
                    break;
                case STORAGE_REQ_TAMPER_LOG: {
                    rtos_app_state_lock();
#if defined(EMBEDDED_HARDWARE_TARGET)
                    const tamper_context_t *auth = e3_meter_app_get_tamper_context();
                    s_storage_tamper_snap = (auth != NULL) ? *auth : g_tamper_ctx;
#else
                    s_storage_tamper_snap = g_tamper_ctx;
#endif
                    rtos_app_state_unlock();
                    (void)nvram_save_tamper_log_snapshot(&s_storage_tamper_snap);
                    break;
                }
                case STORAGE_REQ_DEV_CONFIG:
                    (void)nvram_save_device_config(&s_storage_req.payload.dev_config);
                    break;
                default:
                    break;
            }
            rtos_flash_unlock();
            bsp_watchdog_kick_task(WATCHDOG_TASK_STORAGE_BIT);
        }
    }
}

/* ========================================================================== */
/*                 PELUNCURAN SEMUA 7 WORKER TASK FREERTOS                    */
/* ========================================================================== */

bool rtos_start_all_tasks(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    BaseType_t res = pdPASS;
    if (xTaskCreate(task_tamper_emergency_entry,  "TamperTask",  STACK_SIZE_TAMPER,    NULL, PRIORITY_TASK_TAMPER,    NULL) != pdPASS) res = pdFAIL;
    if (xTaskCreate(task_metrology_profiling_entry,"ProfileTask",STACK_SIZE_PROFILING, NULL, PRIORITY_TASK_PROFILING, NULL) != pdPASS) res = pdFAIL;
    if (xTaskCreate(task_dlms_entry,              "DLMSTask",    STACK_SIZE_DLMS,      NULL, PRIORITY_TASK_DLMS,      NULL) != pdPASS) res = pdFAIL;
    if (xTaskCreate(task_modem_cellular_entry,    "ModemTask",   STACK_SIZE_MODEM,     NULL, PRIORITY_TASK_MODEM,     NULL) != pdPASS) res = pdFAIL;
    if (xTaskCreate(task_oled128x32_carousel,     "OLEDTask",    STACK_SIZE_UI,        NULL, PRIORITY_TASK_UI,        NULL) != pdPASS) res = pdFAIL;
    if (xTaskCreate(task_system_health_entry,     "HealthTask",  STACK_SIZE_HEALTH,    NULL, PRIORITY_TASK_HEALTH,    NULL) != pdPASS) res = pdFAIL;
    if (xTaskCreate(task_storage_logger_entry,    "StorageTask", STACK_SIZE_STORAGE,   NULL, PRIORITY_TASK_STORAGE,   NULL) != pdPASS) res = pdFAIL;
    return (res == pdPASS);
#else
    return true;
#endif
}

/* ========================================================================== */
/*                  API KONSOL SIMULASI, RELAI & TAMPER LOG                   */
/* ========================================================================== */

void rtos_power_relay_simulate_power_trip(void) {
    rtos_app_state_lock();
    power_relay_disconnect(&g_power_relay_ctx, RELAY_REASON_POWER_LIMITER, rtos_get_rtc_epoch_sec());
    rtos_app_state_unlock();
    printf("[SIMULASI] Power Limiter tripped! Relai terbuka (DISCONNECTED).\r\n");
}

void rtos_power_relay_request_reconnect(void) {
    rtos_app_state_lock();
    power_relay_request_reconnect(&g_power_relay_ctx, rtos_get_rtc_epoch_sec());
    rtos_app_state_unlock();
    printf("[SIMULASI] Remote Reconnect DLMS diterima -> READY ON.\r\n");
}

bool rtos_power_relay_confirm_reconnect(void) {
    rtos_app_state_lock();
    bool ok = power_relay_confirm_reconnect(&g_power_relay_ctx, rtos_get_rtc_epoch_sec());
    rtos_app_state_unlock();
    if (ok) {
        printf("[RELAY] Konfirmasi lokal berhasil -> CONNECTED.\r\n");
    } else {
        printf("[RELAY] Gagal: status saat ini %s.\r\n", power_relay_state_to_string(g_power_relay_ctx.relay_state));
    }
    return ok;
}

void rtos_tamper_dump_log(void) {
    dlms_tamper_log_t *log = &s_dlms_task_ctx.server.tamper_log;
    printf("\r\n======================================================================\r\n");
    printf("              TAMPER EVENT LOG (DLMS PROFILE GENERIC CLASS 7)         \r\n");
    printf("                 OBIS: 0.0.99.98.0.255 | Standar SPLN D3.006          \r\n");
    printf("======================================================================\r\n");
    printf(" Total Events: %lu | Aktif: %zu / %u | Latched Mask: 0x%08lX\r\n",
           (unsigned long)log->total_events, log->entries_in_use,
           (unsigned int)DLMS_TAMPER_LOG_MAX_ENTRIES, (unsigned long)s_latched_tamper_mask);
    printf("----------------------------------------------------------------------\r\n");

    if (log->entries_in_use == 0) {
        printf("    (Belum ada kejadian sabotase yang tercatat)\r\n");
    } else {
        for (size_t i = 0; i < log->entries_in_use; i++) {
            dlms_tamper_record_t rec;
            if (dlms_tamper_log_get_entry(log, i, &rec) == DLMS_OK) {
                const char *desc;
                switch ((dlms_tamper_code_t)rec.event_code) {
                    case DLMS_TAMPER_TERMINAL_COVER_OPEN: desc = "Tutup Terminal Buka (ERR20)"; break;
                    case DLMS_TAMPER_METER_COVER_OPEN:    desc = "Tutup Meter Buka (RUSAK)";    break;
                    case DLMS_TAMPER_MAGNETIC_INDUCTION:  desc = "Medan Magnet <=500mT (ERR25)";break;
                    case DLMS_TAMPER_REVERSE_CURRENT:     desc = "Arus Balik / Reverse (ERR26)"; break;
                    case DLMS_TAMPER_MISSING_NEUTRAL:     desc = "Kawat Netral Putus (ERR23)";  break;
                    case DLMS_TAMPER_WRONG_PHASE_SEQ:     desc = "Urutan Fase Balik (ERR21)";   break;
                    case DLMS_TAMPER_CROSS_PHASE:         desc = "Tegangan/Arus Silang (ERR22)";break;
                    case DLMS_TAMPER_MISSING_VOLTAGE:     desc = "Hilang Tegangan Fase (ERR24)";break;
                    case DLMS_TAMPER_LOW_PF_QUAD4:        desc = "Faktor Daya < 0.85 (ERR27)";  break;
                    default:                              desc = "Event Tidak Dikenal";        break;
                }
                printf(" %2zu | %10lu | 0x%02X | %-31s | %s\r\n",
                       i + 1,
                       (unsigned long)rec.timestamp_epoch,
                       rec.event_code,
                       desc,
                       rec.status ? "TERDETEKSI" : "PULIH / NORMAL");
            }
        }
    }
    printf("======================================================================\r\n\r\n");
}

void rtos_tamper_record_event(dlms_tamper_code_t code, bool is_active) {
    uint32_t now_sec = rtos_get_rtc_epoch_sec();
    tamper_event_msg_t msg = {
        .timestamp   = now_sec,
        .tamper_code = (uint8_t)code,
        .is_active   = is_active
    };
    (void)rtos_queue_send_tamper_event(&msg);
}
