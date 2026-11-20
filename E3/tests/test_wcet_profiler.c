/**
 * @file test_wcet_profiler.c
 * @brief Unit Test untuk Modul Profiler WCET & Watchdog Supervisor (DWT / WDT)
 */

#include <stdio.h>
#include <assert.h>
#include "wcet_profiler.h"

int main(void)
{
    printf("====================================================\n");
    printf(" RUNNING WCET PROFILER & WDT SUPERVISOR UNIT TESTS  \n");
    printf("====================================================\n\n");

    /* 1. Uji Inisialisasi */
    wcet_profiler_init(160000000U);
    const wcet_profiler_ctx_t *ctx = wcet_get_context();
    assert(ctx != NULL);
    assert(ctx->sysclk_hz == 160000000U);
    assert(ctx->tasks[WCET_TASK_TAMPER].period_ms == 100U);
    assert(ctx->tasks[WCET_TASK_TAMPER].wcet_budget_us == 1850U);
    assert(ctx->tasks[WCET_TASK_METROLOGY].period_ms == 1000U);
    assert(ctx->tasks[WCET_TASK_METROLOGY].wcet_budget_us == 14200U);
    assert(ctx->tasks[WCET_TASK_DLMS].period_ms == 50U);
    assert(ctx->tasks[WCET_TASK_DLMS].wcet_budget_us == 6400U);
    assert(ctx->tasks[WCET_TASK_UI].period_ms == 500U);
    assert(ctx->tasks[WCET_TASK_UI].wcet_budget_us == 18500U);
    printf("[TEST] Profiler Initialization & Table 3.3.2 Parameters: SUCCESS\n");

    /* 2. Uji Pencatatan Eksekusi Task Normal */
    wcet_record_start(WCET_TASK_TAMPER);
    volatile uint32_t dummy = 0;
    for (int i = 0; i < 500; i++) { dummy += (uint32_t)i; }
    (void)dummy;
    wcet_record_stop(WCET_TASK_TAMPER);

    const wcet_task_stats_t *tamper_stats = wcet_get_task_stats(WCET_TASK_TAMPER);
    assert(tamper_stats != NULL);
    assert(tamper_stats->exec_count == 1U);
    assert(tamper_stats->last_cycles > 0U);
    assert(tamper_stats->max_cycles >= tamper_stats->min_cycles);
    assert(tamper_stats->deadline_violations == 0U);
    printf("[TEST] Single Task Execution Recording (Tamper Task): SUCCESS (Duration: %lu us, %lu cycles)\n",
           (unsigned long)tamper_stats->last_us, (unsigned long)tamper_stats->last_cycles);

    /* 3. Uji Eksekusi Berulang & Metrik Rata-Rata */
    for (int k = 0; k < 5; k++)
    {
        wcet_record_start(WCET_TASK_METROLOGY);
        for (int i = 0; i < 1000; i++) { dummy += (uint32_t)i; }
        (void)dummy;
        wcet_record_stop(WCET_TASK_METROLOGY);
    }
    const wcet_task_stats_t *metrology_stats = wcet_get_task_stats(WCET_TASK_METROLOGY);
    assert(metrology_stats->exec_count == 5U);
    assert(metrology_stats->max_cycles >= metrology_stats->min_cycles);
    printf("[TEST] Multi-Iteration Execution Statistics (Metrology Task): SUCCESS (Avg: %lu us, Max/WCET: %lu us)\n",
           (unsigned long)metrology_stats->avg_us, (unsigned long)metrology_stats->max_us);

    /* 4. Uji Watchdog Feeding Saat Seluruh Tugas Normal */
    bool feed_ok = wcet_watchdog_supervise_and_feed();
    assert(feed_ok == true);
    assert(wcet_watchdog_get_feed_count() >= 1U);
    assert(wcet_watchdog_get_suppressed_count() == 0U);
    assert(wcet_watchdog_is_healthy() == true);
    printf("[TEST] Watchdog Supervisor Feed Under Normal Health: SUCCESS\n");

    /* 5. Uji Deteksi Pelanggaran Tenggat Waktu (WCET Fault Injection) */
    wcet_watchdog_inject_fault(WCET_TASK_DLMS);
    assert(wcet_watchdog_is_healthy() == false);

    bool feed_suppressed = wcet_watchdog_supervise_and_feed();
    assert(feed_suppressed == false);
    assert(wcet_watchdog_get_suppressed_count() == 1U);
    printf("[TEST] Watchdog Suppression On Deadline Violation: SUCCESS\n");

    /* 6. Uji Pemulihan Fault Watchdog */
    wcet_watchdog_clear_fault();
    assert(wcet_watchdog_is_healthy() == true);
    bool feed_recovered = wcet_watchdog_supervise_and_feed();
    assert(feed_recovered == true);
    printf("[TEST] Watchdog Fault Recovery & Reset: SUCCESS\n");

    /* 7. Uji Tampilan Konsol Tabel 3.3.2 */
    printf("\n[TEST] Output Ringkasan Profiling Tabel 3.3.2:\n");
    wcet_print_summary();

    printf(">>> ALL WCET PROFILER & WDT SUPERVISOR TESTS PASSED 100%%! <<<\n");
    printf("====================================================\n");
    return 0;
}
