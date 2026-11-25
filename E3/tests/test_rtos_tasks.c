/**
 * @file test_rtos_tasks.c
 * @brief Pengujian Komprehensif Arsitektur 8-Task FreeRTOS, 3 Queue IPC,
 *        Kebijakan Tamper Ter-Latch Permanen & Watchdog Supervisor (0xEF)
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "rtos_tasks.h"
#include "wcet_profiler.h"

volatile bool g_tamper_alarm_active = false;

int main(void) {
    printf("===================================================\n");
    printf(" RUNNING 8-TASK RTOS IPC & LATCHED TAMPER TEST     \n");
    printf("===================================================\n\n");

    /* 1. Inisialisasi Sistem, 3 Queues, 3 Mutexes & EventGroup */
    rtos_system_init();

    /* 2. Uji Kirim & Terima xTamperQueue (Producer: TamperTask -> Consumer: DLMSTask) */
    tamper_event_msg_t tx_msg = {
        .timestamp = 1700000000U,
        .tamper_code = (uint8_t)DLMS_TAMPER_TERMINAL_COVER_OPEN,
        .is_active = true
    };
    assert(rtos_queue_send_tamper_event(&tx_msg) == true);

    tamper_event_msg_t rx_msg = {0};
    assert(rtos_queue_receive_tamper_event(&rx_msg, 100U) == true);
    assert(rx_msg.timestamp == 1700000000U);
    assert(rx_msg.tamper_code == (uint8_t)DLMS_TAMPER_TERMINAL_COVER_OPEN);
    assert(rx_msg.is_active == true);
    assert(rtos_queue_receive_tamper_event(&rx_msg, 100U) == false);
    printf("[TEST 1] xTamperQueue Producer/Consumer: PASSED\n");

    /* 3. Uji Pemrosesan Event ke DLMS Tamper Log (Class 7) & Kamus OBIS */
    uint32_t init_term_count = dlms_obis_get_tamper_counter(DLMS_TAMPER_TERMINAL_COVER_OPEN);
    rtos_dlms_process_tamper_event(&rx_msg);

    dlms_task_ctx_t *task_ctx = rtos_dlms_get_task_ctx();
    assert(task_ctx != NULL);
    assert(task_ctx->server.tamper_log.entries_in_use == 1U);

    dlms_tamper_record_t rec;
    assert(dlms_tamper_log_get_entry(&task_ctx->server.tamper_log, 0U, &rec) == DLMS_OK);
    assert(rec.timestamp_epoch == 1700000000U);
    assert(rec.event_code == DLMS_TAMPER_TERMINAL_COVER_OPEN);
    assert(rec.status == 1U);
    assert(dlms_obis_get_tamper_counter(DLMS_TAMPER_TERMINAL_COVER_OPEN) == init_term_count + 1U);
    printf("[TEST 2] DLMS Logbook Class 7 & OBIS Counter: PASSED\n");

    /* 4. Uji Kebijakan Sabotase Ter-Latch Permanen (RUSAK / ERR20) vs Auto-Recover (Magnet) */
    rtos_tamper_apply_latch((uint32_t)(TAMPER_VECTOR_CASE_OPEN | TAMPER_VECTOR_MAGNETIC_FIELD), 1700000100U);
    tamper_context_t *t_ctx = rtos_tamper_get_ctx();
    assert(t_ctx != NULL);
    assert((tamper_get_active_mask(t_ctx) & TAMPER_VECTOR_CASE_OPEN) != 0U);
    assert((tamper_get_active_mask(t_ctx) & TAMPER_VECTOR_MAGNETIC_FIELD) != 0U);
    assert((rtos_tamper_get_latched_mask() & TAMPER_VECTOR_CASE_OPEN) != 0U);

    /* Upaya pulih otomatis CASE_OPEN (tutup meter dipasang kembali) WAJIB DITOLAK! */
    assert(rtos_tamper_evaluate_clear(TAMPER_VECTOR_CASE_OPEN, false, 1700000200U) == false);
    assert((tamper_get_active_mask(t_ctx) & TAMPER_VECTOR_CASE_OPEN) != 0U);
    assert((rtos_tamper_get_latched_mask() & TAMPER_VECTOR_CASE_OPEN) != 0U);

    /* Medan magnet (non-latch) BOLEH pulih otomatis tanpa clear resmi */
    assert(rtos_tamper_evaluate_clear(TAMPER_VECTOR_MAGNETIC_FIELD, false, 1700000210U) == true);
    assert((tamper_get_active_mask(t_ctx) & TAMPER_VECTOR_MAGNETIC_FIELD) == 0U);

    /* Perintah Clear resmi PLN BARU boleh membuka latch CASE_OPEN */
    rtos_tamper_clear_authorized((uint32_t)TAMPER_VECTOR_CASE_OPEN);
    assert((tamper_get_active_mask(t_ctx) & TAMPER_VECTOR_CASE_OPEN) == 0U);
    assert((rtos_tamper_get_latched_mask() & TAMPER_VECTOR_CASE_OPEN) == 0U);
    printf("[TEST 3] Latched Tamper Irreversible Policy (RUSAK vs Magnet): PASSED\n");

    /* 5. Uji Thread-Safe Measurement Snapshot (xMeasMutex Publish & Get) */
    meter_measurements_t test_meas = {0};
    test_meas.voltage_r_dvolts = 2200U;
    test_meas.current_r_mamps  = 5000U;
    test_meas.active_power_w   = 1100;
    test_meas.active_energy_wh = 123456ULL;
    rtos_meter_data_publish(&test_meas);

    meter_measurements_t read_meas = {0};
    assert(rtos_meter_data_get_snapshot(&read_meas) == true);
    assert(read_meas.voltage_r_dvolts == 2200U);
    assert(read_meas.active_energy_wh == 123456ULL);
    printf("[TEST 4] Thread-Safe Measurement Snapshot (xMeasMutex): PASSED\n");

    /* 6. Uji Antrean StorageTask (xStorageQueue: Checkpoint & Device Config IDPEL) */
    storage_write_req_t sreq_cp = {0};
    sreq_cp.type = STORAGE_REQ_ENERGY_CHECKPOINT;
    sreq_cp.payload.checkpoint.magic = NVRAM_ENERGY_CHECKPOINT_MAGIC;
    sreq_cp.payload.checkpoint.active_energy_import_wh = 999999ULL;
    assert(rtos_queue_send_storage_req(&sreq_cp) == true);

    storage_write_req_t sreq_cfg = {0};
    sreq_cfg.type = STORAGE_REQ_DEV_CONFIG;
    sreq_cfg.payload.dev_config.magic = NVRAM_DEVICE_CONFIG_MAGIC;
    strcpy(sreq_cfg.payload.dev_config.customer_id, "543210000123");
    assert(rtos_queue_send_storage_req(&sreq_cfg) == true);

    storage_write_req_t srx = {0};
    assert(rtos_queue_receive_storage_req(&srx, 100U) == true);
    assert(srx.type == STORAGE_REQ_ENERGY_CHECKPOINT);
    assert(srx.payload.checkpoint.active_energy_import_wh == 999999ULL);
    assert(rtos_queue_receive_storage_req(&srx, 100U) == true);
    assert(srx.type == STORAGE_REQ_DEV_CONFIG);
    assert(strcmp(srx.payload.dev_config.customer_id, "543210000123") == 0);
    assert(rtos_queue_receive_storage_req(&srx, 100U) == false);
    printf("[TEST 5] xStorageQueue (StorageTask Async Worker Queue): PASSED\n");

    /* 7. Uji Antrean ModemTask (xModemTxQueue: Last-Gasp Push) */
    modem_push_msg_t mreq = {
        .type = MODEM_PUSH_LAST_GASP,
        .timestamp = 1700000500U,
        .active_energy_wh = 123456ULL,
        .tamper_code = 0x50U,
        .is_active = true
    };
    assert(rtos_queue_send_modem_push(&mreq) == true);
    modem_push_msg_t mrx = {0};
    assert(rtos_queue_receive_modem_push(&mrx, 100U) == true);
    assert(mrx.type == MODEM_PUSH_LAST_GASP);
    assert(mrx.active_energy_wh == 123456ULL);
    assert(rtos_queue_receive_modem_push(&mrx, 100U) == false);
    printf("[TEST 6] xModemTxQueue (ModemTask 4G Last-Gasp Push): PASSED\n");

    /* 8. Uji Watchdog Supervisor Liveness Bitmask (7 Worker Tasks -> 0xEF) & Identity Sync */
    bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_DLMS_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_UI_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_MODEM_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_HEALTH_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_STORAGE_BIT);
    assert((rtos_watchdog_get_kicked_mask() & WATCHDOG_ALL_TASKS_MASK) == WATCHDOG_ALL_TASKS_MASK);
    assert(rtos_watchdog_supervise_and_feed() == true);

    rtos_set_device_ids("543219876543", "530000999888");
    char cust[16] = {0}, mtr[16] = {0};
    rtos_get_device_ids(cust, sizeof(cust), mtr, sizeof(mtr));
    assert(strcmp(cust, "543219876543") == 0);
    assert(strcmp(mtr, "530000999888") == 0);

    rtos_power_notify_ac_fail(true);
    assert((rtos_power_get_event_bits() & PWR_EVT_AC_FAIL_BIT) != 0U);
    rtos_power_notify_ac_fail(false);
    assert((rtos_power_get_event_bits() & PWR_EVT_AC_FAIL_BIT) == 0U);
    printf("[TEST 7] Watchdog Liveness (0xEF), Identity Sync & Power EventGroup: PASSED\n");

    /* 9. Uji Klasifikasi Watchdog: Error Ringan Non-Kritis vs Glitch Sesaat vs ERR04 Fatal (SPLN Tabel 4 No 5) */
    rtos_watchdog_test_reset_persist();
    rtos_system_init();

    wdt_supervisor_status_t wstat = {0};
    rtos_watchdog_test_set_tick_ms(1000U);
    bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_DLMS_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_UI_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_MODEM_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_HEALTH_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_STORAGE_BIT);
    assert(rtos_watchdog_supervise_and_feed() == true);

    /* 9a. Error Ringan: OLEDTask, ModemTask & DLMSTask hang > timeout, task kritis tetap normal -> NON-ERR04, PD12 TETAP NYALA */
    rtos_watchdog_test_set_tick_ms(3000U);
    bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_HEALTH_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_STORAGE_BIT);
    assert(rtos_watchdog_supervise_and_feed() == true);
    rtos_watchdog_get_status(&wstat);
    assert(wstat.severity == WDT_SEVERITY_MINOR_DEGRADED);
    assert(wstat.err04_latched == false);
    assert((wstat.minor_degraded_mask & WATCHDOG_TASK_UI_BIT) != 0U);
    assert((wstat.minor_degraded_mask & WATCHDOG_TASK_MODEM_BIT) != 0U);

    /* Pemulihan otomatis saat OLED/Modem/DLMS berdetak lagi */
    bsp_watchdog_kick_task(WATCHDOG_TASK_DLMS_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_UI_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_MODEM_BIT);
    assert(rtos_watchdog_supervise_and_feed() == true);
    rtos_watchdog_get_status(&wstat);
    assert(wstat.minor_degraded_mask == 0U);
    assert(wstat.severity == WDT_SEVERITY_NORMAL);

    /* 9b. Glitch Sementara 1x pada Task Kritis (TamperTask macet > 1500 ms) -> Tahan PD12, setelah boot TIDAK ERR04 */
    rtos_watchdog_test_set_tick_ms(5000U);
    bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_HEALTH_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_STORAGE_BIT);
    assert(rtos_watchdog_supervise_and_feed() == false); /* Tahan PD12 agar reset sesaat */
    rtos_watchdog_get_status(&wstat);
    assert(wstat.reset_streak == 1U);
    assert(wstat.err04_latched == false);
    assert(wstat.pending_reset == true);

    /* Simulasikan MCU reboot sesudah glitch #1 -> Relai tidak boleh trip, bukan ERR04 */
    rtos_watchdog_test_set_tick_ms(100U);
    rtos_system_init();
    rtos_watchdog_get_status(&wstat);
    assert(wstat.reset_streak == 1U);
    assert(wstat.err04_latched == false);
    assert(wstat.pending_reset == false);
    assert(wstat.severity == WDT_SEVERITY_GLITCH_RECOVERING);

    /* Setelah uptime stabil 10 detik, reset_streak kembali 0 */
    rtos_watchdog_test_set_tick_ms(10200U);
    bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_HEALTH_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_STORAGE_BIT);
    assert(rtos_watchdog_supervise_and_feed() == true);
    rtos_watchdog_get_status(&wstat);
    assert(wstat.reset_streak == 0U);
    assert(wstat.severity == WDT_SEVERITY_NORMAL);

    /* 9c. Reset Loop Berulang >= 3x pada Task Kritis -> Latch ERR04 + Trip Relai */
    for (uint32_t cycle = 1U; cycle <= WDT_FATAL_RESET_THRESHOLD; cycle++) {
        uint32_t base = cycle * 1000U;
        rtos_watchdog_test_set_tick_ms(base);
        bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT);
        bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);
        bsp_watchdog_kick_task(WATCHDOG_TASK_HEALTH_BIT);
        bsp_watchdog_kick_task(WATCHDOG_TASK_STORAGE_BIT);
        rtos_watchdog_test_set_tick_ms(base + 2000U); /* TamperTask tidak menendang > 1500 ms */
        bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);
        assert(rtos_watchdog_supervise_and_feed() == false);
        rtos_watchdog_test_set_tick_ms(base + 2100U);
        rtos_system_init();
    }
    rtos_watchdog_get_status(&wstat);
    assert(wstat.reset_streak == WDT_FATAL_RESET_THRESHOLD);
    assert(wstat.err04_latched == true);
    assert(wstat.severity == WDT_SEVERITY_FATAL_ERR04);
    assert(spln_alarm_should_trip_relay(SPLN_ALARM_MCU_ERROR) == true);
    /* Saat task hidup kembali dalam mode ERR04, PD12 tetap disegarkan agar layar ERR04 & DLMS bisa dibaca */
    bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_HEALTH_BIT);
    bsp_watchdog_kick_task(WATCHDOG_TASK_STORAGE_BIT);
    assert(rtos_watchdog_supervise_and_feed() == true);

    /* Reboot ulang tanpa clear resmi wajib tetap mempertahankan latch ERR04 */
    rtos_system_init();
    assert(rtos_watchdog_is_err04_latched() == true);

    /* Clear resmi membuka latch ERR04, lalu uji Trap Fatal Clock NMI -> langsung ERR04 */
    rtos_watchdog_clear_err04();
    assert(rtos_watchdog_is_err04_latched() == false);
    assert((rtos_internal_alarm_get_mask() & SPLN_ALARM_MCU_ERROR) == 0U);
    rtos_watchdog_record_fatal_trap(WDT_SRC_CLOCK_NMI, 0U);
    rtos_system_init();
    rtos_watchdog_get_status(&wstat);
    assert(wstat.err04_latched == true);
    assert(wstat.last_fault_source == WDT_SRC_CLOCK_NMI);
    assert((rtos_internal_alarm_get_mask() & SPLN_ALARM_MCU_ERROR) != 0U);

    /* Uji konsistensi CLI non-kritis (WCET_TASK_UI=3) vs kritis (WCET_TASK_TAMPER=0) */
    rtos_watchdog_clear_err04();
    wcet_watchdog_inject_fault(WCET_TASK_UI);
    assert(rtos_watchdog_supervise_and_feed() == true); /* Non-kritis tidak menahan PD12 */
    assert(rtos_watchdog_is_err04_latched() == false);
    assert((rtos_watchdog_get_minor_fault_mask() & WATCHDOG_TASK_UI_BIT) != 0U);
    rtos_watchdog_clear_err04();
    assert(rtos_watchdog_is_err04_latched() == false);
    printf("[TEST 8] Watchdog Tiered Policy (Minor vs Glitch vs Fatal ERR04): PASSED\n");

    /* 9. Uji Sinkronisasi RTC Epoch Nyata pada RTOS Helper & Event Stamp */
    rtos_watchdog_test_set_tick_ms(0U);
    assert(rtos_get_rtc_epoch_sec() == 1793181600U);
    rtos_watchdog_test_set_tick_ms(65000U);
    assert(rtos_get_rtc_epoch_sec() == 1793181665U);
    tamper_event_msg_t dummy_ev;
    while (rtos_queue_receive_tamper_event(&dummy_ev, 0)) {}
    rtos_tamper_record_event(DLMS_TAMPER_MAGNETIC_INDUCTION, true);
    assert(rtos_queue_receive_tamper_event(&dummy_ev, 0) == true);
    assert(dummy_ev.timestamp >= 1793181665U);
    printf("[TEST 9] RTOS RTC Epoch Timestamp Sync (>= 1793181665): PASSED\n");

    printf("\n>>> ALL 8-TASK RTOS IPC & FLOW DIAGRAM TESTS PASSED 100%%! <<<\n");
    printf("===================================================\n");
    return 0;
}
