/**
 * @file wcet_profiler.c
 * @brief Implementasi High-Precision WCET Profiler & Watchdog Supervisor (DWT / WDT)
 * 
 * Standar: SPLN D3.006: 2021 & Arsitektur Sistem Bab 3.3.1.2 (Tabel 3.3.2)
 * Target: STM32U575 / STM32U585 (ARM Cortex-M33 @ 160 MHz) & Host CTest
 */

#include "wcet_profiler.h"
#include <stdio.h>
#include <string.h>

#if defined(EMBEDDED_HARDWARE_TARGET)
#include "main.h"
#elif defined(_WIN32)
#include <windows.h>
#else
#include <time.h>
#endif

/* -------------------------------------------------------------------------- */
/* Konteks Statis Global Profiler & Watchdog                                  */
/* -------------------------------------------------------------------------- */
static wcet_profiler_ctx_t s_ctx;
static bool s_initialized = false;

/* -------------------------------------------------------------------------- */
/* Abstraksi Hardware Pewaktu Siklus (DWT CYCCNT @ 160 MHz / Host High-Res)    */
/* -------------------------------------------------------------------------- */

static inline uint32_t profiler_read_raw_cycles(void)
{
#if defined(EMBEDDED_HARDWARE_TARGET)
    return DWT->CYCCNT;
#elif defined(_WIN32)
    static LARGE_INTEGER freq;
    static int init_qpc = 0;
    if (!init_qpc)
    {
        QueryPerformanceFrequency(&freq);
        init_qpc = 1;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    /* Konversi frekuensi QPC ke ekuivalen 160 MHz clock cycles */
    return (uint32_t)((counter.QuadPart * 160000000ULL) / freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ns = (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
    return (uint32_t)((ns * 160ULL) / 1000ULL);
#endif
}

static inline uint32_t profiler_get_tick_ms(void)
{
#if defined(EMBEDDED_HARDWARE_TARGET)
    return HAL_GetTick();
#elif defined(_WIN32)
    return (uint32_t)GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

static inline uint32_t cycles_to_microseconds(uint32_t cycles, uint32_t sysclk_hz)
{
    if (sysclk_hz == 0U) sysclk_hz = 160000000U;
    return (uint32_t)(((uint64_t)cycles * 1000000ULL + (sysclk_hz / 2U)) / sysclk_hz);
}

static inline uint32_t microseconds_to_cycles(uint32_t us, uint32_t sysclk_hz)
{
    if (sysclk_hz == 0U) sysclk_hz = 160000000U;
    return (uint32_t)(((uint64_t)us * sysclk_hz) / 1000000ULL);
}

/* -------------------------------------------------------------------------- */
/* Inisialisasi Profiler & Pengawas Watchdog                                  */
/* -------------------------------------------------------------------------- */

void wcet_profiler_init(uint32_t sysclk_hz)
{
    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.sysclk_hz = (sysclk_hz > 0U) ? sysclk_hz : 160000000U;

#if defined(EMBEDDED_HARDWARE_TARGET)
    /* 1. Aktifkan DWT Cycle Counter pada ARM Cortex-M33 STM32U5 */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0U;
    s_ctx.dwt_hardware_active = ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) != 0U);
#else
    s_ctx.dwt_hardware_active = true;
#endif

    /* 2. Konfigurasi 4 Tugas Pokok & 1 Superloop Sesuai Tabel 3.3.2 */
    s_ctx.tasks[WCET_TASK_TAMPER].name           = "Task_Tamper_Emergency";
    s_ctx.tasks[WCET_TASK_TAMPER].period_ms      = 100U;   /* T1 = 100 ms */
    s_ctx.tasks[WCET_TASK_TAMPER].wcet_budget_us = 1850U;  /* C1 = 1.85 ms = 1850 us */

    s_ctx.tasks[WCET_TASK_METROLOGY].name           = "Task_Metrology_Profiling";
    s_ctx.tasks[WCET_TASK_METROLOGY].period_ms      = 1000U;  /* T2 = 1000 ms */
    s_ctx.tasks[WCET_TASK_METROLOGY].wcet_budget_us = 14200U; /* C2 = 14.20 ms = 14200 us */

    s_ctx.tasks[WCET_TASK_DLMS].name           = "Task_DLMS_Server";
    s_ctx.tasks[WCET_TASK_DLMS].period_ms      = 50U;    /* T3 = 50 ms */
    s_ctx.tasks[WCET_TASK_DLMS].wcet_budget_us = 6400U;  /* C3 = 6.40 ms = 6400 us */

    s_ctx.tasks[WCET_TASK_UI].name           = "Task_UI_Display";
    s_ctx.tasks[WCET_TASK_UI].period_ms      = 500U;   /* T4 = 500 ms */
    s_ctx.tasks[WCET_TASK_UI].wcet_budget_us = 18500U; /* C4 = 18.50 ms = 18500 us */

    s_ctx.tasks[WCET_TASK_APP_STEP].name           = "Task_System_Metering";
    s_ctx.tasks[WCET_TASK_APP_STEP].period_ms      = 100U;   /* Superloop = 100 ms */
    s_ctx.tasks[WCET_TASK_APP_STEP].wcet_budget_us = 35000U; /* Step Budget = 35.00 ms */

    /* Inisialisasi batas min_cycles ke nilai tertinggi */
    for (size_t i = 0; i < WCET_TASK_COUNT; i++)
    {
        s_ctx.tasks[i].min_cycles = 0xFFFFFFFFU;
        s_ctx.tasks[i].min_us     = 0xFFFFFFFFU;
    }

    s_ctx.watchdog_required_mask = (WCET_WDT_TASK_TAMPER_BIT |
                                    WCET_WDT_TASK_METROLOGY_BIT |
                                    WCET_WDT_TASK_DLMS_BIT |
                                    WCET_WDT_TASK_UI_BIT |
                                    WCET_WDT_TASK_APP_STEP_BIT);
    s_ctx.last_feed_tick_ms = profiler_get_tick_ms();
    s_ctx.fault_latched     = false;
    s_initialized           = true;
}

/* -------------------------------------------------------------------------- */
/* Pengukuran Waktu Eksekusi (Start & Stop)                                   */
/* -------------------------------------------------------------------------- */

void wcet_record_start(wcet_task_id_t task_id)
{
    if ((size_t)task_id >= (size_t)WCET_TASK_COUNT) return;
    if (!s_initialized) wcet_profiler_init(160000000U);

    s_ctx.tasks[task_id].start_cycle_stamp = profiler_read_raw_cycles();
    s_ctx.tasks[task_id].is_active = true;
}

void wcet_record_stop(wcet_task_id_t task_id)
{
    if ((size_t)task_id >= (size_t)WCET_TASK_COUNT) return;
    if (!s_ctx.tasks[task_id].is_active) return;

    uint32_t stop_cyc = profiler_read_raw_cycles();
    uint32_t elapsed_cycles = stop_cyc - s_ctx.tasks[task_id].start_cycle_stamp;
    s_ctx.tasks[task_id].is_active = false;

    wcet_task_stats_t *t = &s_ctx.tasks[task_id];
    t->exec_count++;
    t->last_cycles = elapsed_cycles;
    t->total_cycles += elapsed_cycles;

    if (elapsed_cycles > t->max_cycles)
    {
        t->max_cycles = elapsed_cycles;
    }
    if (elapsed_cycles < t->min_cycles)
    {
        t->min_cycles = elapsed_cycles;
    }

    uint32_t elapsed_us = cycles_to_microseconds(elapsed_cycles, s_ctx.sysclk_hz);
    t->last_us = elapsed_us;
    if (elapsed_us > t->max_us)
    {
        t->max_us = elapsed_us;
    }
    if (elapsed_us < t->min_us)
    {
        t->min_us = elapsed_us;
    }

    if (t->exec_count > 0U)
    {
        t->avg_us = (uint32_t)cycles_to_microseconds((uint32_t)(t->total_cycles / t->exec_count), s_ctx.sysclk_hz);
    }

    /* Evaluasi batas anggaran WCET teoretis */
    if (elapsed_us > t->wcet_budget_us)
    {
        t->deadline_violations++;
        s_ctx.fault_latched = true;
        s_ctx.fault_task_id = task_id;
    }

    /* Set kick bitmask untuk watchdog supervisor */
    s_ctx.watchdog_kicked_mask |= (1U << (uint32_t)task_id);
    t->last_kick_tick_ms = profiler_get_tick_ms();
}

void wcet_profiler_reset(void)
{
    for (size_t i = 0; i < WCET_TASK_COUNT; i++)
    {
        s_ctx.tasks[i].exec_count          = 0U;
        s_ctx.tasks[i].last_cycles         = 0U;
        s_ctx.tasks[i].min_cycles          = 0xFFFFFFFFU;
        s_ctx.tasks[i].max_cycles          = 0U;
        s_ctx.tasks[i].total_cycles        = 0ULL;
        s_ctx.tasks[i].last_us             = 0U;
        s_ctx.tasks[i].min_us              = 0xFFFFFFFFU;
        s_ctx.tasks[i].max_us              = 0U;
        s_ctx.tasks[i].avg_us              = 0U;
        s_ctx.tasks[i].deadline_violations = 0U;
        s_ctx.tasks[i].is_active           = false;
        s_ctx.tasks[i].last_kick_tick_ms   = profiler_get_tick_ms();
    }
    s_ctx.watchdog_feed_count            = 0U;
    s_ctx.watchdog_suppressed_count      = 0U;
    s_ctx.fault_latched                  = false;
}

const wcet_task_stats_t* wcet_get_task_stats(wcet_task_id_t task_id)
{
    if (!s_initialized) wcet_profiler_init(160000000U);
    if ((size_t)task_id >= (size_t)WCET_TASK_COUNT) return NULL;
    return &s_ctx.tasks[task_id];
}

const wcet_profiler_ctx_t* wcet_get_context(void)
{
    if (!s_initialized) wcet_profiler_init(160000000U);
    return &s_ctx;
}

/* -------------------------------------------------------------------------- */
/* Watchdog Supervisor (WDT) Implementation                                   */
/* -------------------------------------------------------------------------- */

void wcet_watchdog_kick(uint32_t task_mask)
{
    if (!s_initialized) wcet_profiler_init(160000000U);
    s_ctx.watchdog_kicked_mask |= task_mask;
}

bool wcet_watchdog_supervise_and_feed(void)
{
    if (!s_initialized) wcet_profiler_init(160000000U);
    uint32_t now_ms = profiler_get_tick_ms();

    /* 1. Jika terdeteksi latched fault akibat pelanggaran WCET, tahan penyegaran */
    if (s_ctx.fault_latched)
    {
        s_ctx.watchdog_suppressed_count++;
        return false;
    }

    /* 2. Periksa apakah ada task yang macet (melebihi batas toleransi periode atau batas pengawas 1.6 s) */
    for (size_t i = 0; i < WCET_TASK_COUNT; i++)
    {
        if (s_ctx.tasks[i].exec_count > 0U)
        {
            uint32_t max_allowed_gap_ms = s_ctx.tasks[i].period_ms * 10U;
            if (max_allowed_gap_ms > 1400U)
            {
                max_allowed_gap_ms = 1400U; /* Batas keras di bawah 1.6 s external watchdog */
            }
            if (max_allowed_gap_ms < 1200U)
            {
                max_allowed_gap_ms = 1200U; /* Minimal 1200 ms toleransi penjadwalan */
            }

            uint32_t elapsed_since_last_ms = now_ms - s_ctx.tasks[i].last_kick_tick_ms;
            if (elapsed_since_last_ms > max_allowed_gap_ms)
            {
                /* Tugas macet / starved */
                s_ctx.fault_latched = true;
                s_ctx.fault_task_id = (wcet_task_id_t)i;
                s_ctx.watchdog_suppressed_count++;
                return false;
            }
        }
    }

    /* 3. Seluruh task normal: Bangkitkan pulsa penyegaran WDT (Toggle Pin EXT_WDI_PULSE PD12) */
#if defined(EMBEDDED_HARDWARE_TARGET)
    HAL_GPIO_TogglePin(EXT_WDI_PULSE_GPIO_Port, EXT_WDI_PULSE_Pin);
#endif

    s_ctx.watchdog_feed_count++;
    s_ctx.last_feed_tick_ms = now_ms;
    return true;
}

bool wcet_watchdog_is_healthy(void)
{
    if (!s_initialized) wcet_profiler_init(160000000U);
    return (!s_ctx.fault_latched);
}

uint32_t wcet_watchdog_get_feed_count(void)
{
    if (!s_initialized) wcet_profiler_init(160000000U);
    return s_ctx.watchdog_feed_count;
}

uint32_t wcet_watchdog_get_suppressed_count(void)
{
    if (!s_initialized) wcet_profiler_init(160000000U);
    return s_ctx.watchdog_suppressed_count;
}

void wcet_watchdog_inject_fault(wcet_task_id_t task_id)
{
    if (!s_initialized) wcet_profiler_init(160000000U);
    if ((size_t)task_id < (size_t)WCET_TASK_COUNT)
    {
        s_ctx.tasks[task_id].deadline_violations++;
        s_ctx.fault_task_id = task_id;
    }
    s_ctx.fault_latched = true;
}

void wcet_watchdog_clear_fault(void)
{
    if (!s_initialized) wcet_profiler_init(160000000U);
    s_ctx.fault_latched = false;
    for (size_t i = 0; i < WCET_TASK_COUNT; i++)
    {
        s_ctx.tasks[i].last_kick_tick_ms = profiler_get_tick_ms();
    }
}

/* -------------------------------------------------------------------------- */
/* Visualisasi Konsol Sesuai Tabel 3.3.2 Arsitektur Sistem                    */
/* -------------------------------------------------------------------------- */

void wcet_print_summary(void)
{
    if (!s_initialized) wcet_profiler_init(160000000U);
    printf("\r\n===================================================================================================\r\n");
    printf("         WORST-CASE EXECUTION TIME (WCET) & CPU UTILIZATION PROFILER (DWT / WDT)                   \r\n");
    printf("         Target: STM32U575 @ %lu MHz | Rate-Monotonic Scheduling (RMS Theoretical Bound: 75.68%%)   \r\n",
           (unsigned long)(s_ctx.sysclk_hz / 1000000U));
    printf("===================================================================================================\r\n");
    printf(" ID | Nama Tugas                | Periode (T) | Budget (C) | Execs  | Min (us) | Avg (us) | WCET/Max | Rasio (U) | Status\r\n");
    printf("----+---------------------------+-------------+------------+--------+----------+----------+----------+-----------+---------\r\n");

    uint32_t total_util_x10000 = 0U;

    for (size_t i = 0; i < WCET_TASK_COUNT; i++)
    {
        const wcet_task_stats_t *t = &s_ctx.tasks[i];
        uint32_t min_disp = (t->min_us == 0xFFFFFFFFU) ? 0U : t->min_us;
        uint32_t max_disp = t->max_us;

        /* Hitung Rasio Utilisasi Ui = Ci / Ti dalam satuan basis 10,000 (0.01%) */
        uint32_t util_ratio_x10000 = 0U;
        if (t->period_ms > 0U && max_disp > 0U)
        {
            util_ratio_x10000 = (uint32_t)(((uint64_t)max_disp * 10000ULL) / ((uint64_t)t->period_ms * 1000ULL));
        }
        else if (t->period_ms > 0U && t->wcet_budget_us > 0U)
        {
            /* Jika belum dieksekusi, tampilkan estimasi desain teoretis */
            util_ratio_x10000 = (uint32_t)(((uint64_t)t->wcet_budget_us * 10000ULL) / ((uint64_t)t->period_ms * 1000ULL));
        }

        /* Akumulasikan ke Utilisasi Total prosesor */
        if (i < 4U) /* 4 Core periodic tasks */
        {
            total_util_x10000 += util_ratio_x10000;
        }

        const char *status_str = (t->deadline_violations > 0U) ? "DEADLINE!" : (t->exec_count > 0U ? "PASS" : "IDLE");

        printf("  %u  | %-25s | %5lu ms   | %5lu us  | %6lu | %8lu | %8lu | %8lu | %3lu.%02lu %%  | %s\r\n",
               (unsigned int)(i + 1),
               t->name,
               (unsigned long)t->period_ms,
               (unsigned long)t->wcet_budget_us,
               (unsigned long)t->exec_count,
               (unsigned long)min_disp,
               (unsigned long)t->avg_us,
               (unsigned long)max_disp,
               (unsigned long)(util_ratio_x10000 / 100U),
               (unsigned long)(util_ratio_x10000 % 100U),
               status_str);
    }

    printf("---------------------------------------------------------------------------------------------------\r\n");
    uint32_t headroom_x10000 = (total_util_x10000 < 10000U) ? (10000U - total_util_x10000) : 0U;
    printf(" Pemanfaatan Prosesor Total (U)    : %lu.%02lu %% (Batas RMS Teoretis: 75.68 %%)\r\n",
           (unsigned long)(total_util_x10000 / 100U), (unsigned long)(total_util_x10000 % 100U));
    printf(" Margin Cadangan Komputasi CPU     : %lu.%02lu %%\r\n",
           (unsigned long)(headroom_x10000 / 100U), (unsigned long)(headroom_x10000 % 100U));
    printf(" Status Pengawas Hardware (WDT)    : %s\r\n",
           s_ctx.fault_latched ? "*** FAULT DETECTED! PULSA DITAHAN ***" : "NORMAL (SUPERVISI AKTIF)");
    printf(" Metrik Watchdog Feed              : Berhasil Toggle: %lu | Ditahan: %lu\r\n",
           (unsigned long)s_ctx.watchdog_feed_count, (unsigned long)s_ctx.watchdog_suppressed_count);
    printf(" Hardware Watchdog Pin             : EXT_WDI_PULSE (GPIOD Pin 12, Timeout 1.6 s)\r\n");
    printf("===================================================================================================\r\n\r\n");
}
