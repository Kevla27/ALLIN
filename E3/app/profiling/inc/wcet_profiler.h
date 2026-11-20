/**
 * @file wcet_profiler.h
 * @brief High-Precision Worst-Case Execution Time (WCET) Profiler & Watchdog Supervisor (DWT / WDT)
 * 
 * Target: STM32U575 / STM32U585 (ARM Cortex-M33 TrustZone @ 160 MHz) & Host Simulation
 * Standard: SPLN D3.006: 2021 & Arsitektur Sistem Bagian 3.3.1.2 (Tabel 3.3.2)
 * 
 * Modul ini menyediakan:
 * 1. Pengukuran waktu eksekusi berpresisi siklus clock (DWT CYCCNT @ 160 MHz, 1 cycle = 6.25 ns).
 * 2. Analisis WCET (Worst-Case Execution Time), Min, Max, Rata-rata, dan Utilisasi CPU (Ui = Ci / Ti).
 * 3. Pengawas Watchdog Terpadu (WDT Supervisor) yang memantau batas tenggat waktu (deadline)
 *    setiap task dan hanya mengizinkan sinyal penyegaran (toggle pin EXT_WDI_PULSE PD12)
 *    apabila seluruh tugas aktif dan memenuhi batas WCET yang dipersyaratkan.
 */

#ifndef WCET_PROFILER_H
#define WCET_PROFILER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ========================================================================== */
/* Definisi Task ID Sesuai Desain Sistem Tabel 3.3.2                          */
/* ========================================================================== */
typedef enum {
    WCET_TASK_TAMPER = 0,       /**< Task 1: Task_Tamper_Emergency (100 ms, WCET 1.85 ms) */
    WCET_TASK_METROLOGY,        /**< Task 2: Task_Metrology_Profiling (1000 ms, WCET 14.20 ms) */
    WCET_TASK_DLMS,             /**< Task 3: Task_DLMS_Server (50 ms, WCET 6.40 ms) */
    WCET_TASK_UI,               /**< Task 4: Task_UI_Display (500 ms, WCET 18.50 ms) */
    WCET_TASK_APP_STEP,         /**< Task 5: Task_System_Metering Superloop (100 ms, WCET 35.00 ms) */
    WCET_TASK_COUNT
} wcet_task_id_t;

/* Bitmask Watchdog Task */
#define WCET_WDT_TASK_TAMPER_BIT    (1U << 0)
#define WCET_WDT_TASK_METROLOGY_BIT (1U << 1)
#define WCET_WDT_TASK_DLMS_BIT      (1U << 2)
#define WCET_WDT_TASK_UI_BIT        (1U << 3)
#define WCET_WDT_TASK_APP_STEP_BIT  (1U << 4)
#define WCET_WDT_ALL_TASKS_MASK     (0x1FU)

/**
 * @brief Statistik Profiling & WCET per Task
 */
typedef struct {
    const char *name;              /**< Nama resmi tugas */
    uint32_t   period_ms;          /**< Periode kedatangan Ti dalam milidetik */
    uint32_t   wcet_budget_us;     /**< Batas teoretis WCET Ci dalam mikrodetik */
    uint32_t   exec_count;         /**< Jumlah total eksekusi */
    uint32_t   last_cycles;        /**< Durasi siklus clock eksekusi terakhir */
    uint32_t   min_cycles;         /**< Durasi siklus clock terpendek */
    uint32_t   max_cycles;         /**< Durasi siklus clock terpanjang (WCET fisik terukur) */
    uint64_t   total_cycles;       /**< Akumulasi siklus clock untuk perhitungan rata-rata */
    uint32_t   last_us;            /**< Waktu eksekusi terakhir (mikrodetik) */
    uint32_t   min_us;             /**< Waktu eksekusi minimum (mikrodetik) */
    uint32_t   max_us;             /**< WCET terukur (mikrodetik) */
    uint32_t   avg_us;             /**< Waktu eksekusi rata-rata (mikrodetik) */
    uint32_t   start_cycle_stamp;  /**< Penanda siklus saat start */
    uint32_t   last_kick_tick_ms;  /**< Waktu HAL tick saat task check-in terakhir */
    uint32_t   deadline_violations;/**< Jumlah pelanggaran batas waktu WCET / period */
    bool       is_active;          /**< Status eksekusi saat ini (true = sedang berjalan) */
} wcet_task_stats_t;

/**
 * @brief Konteks Global Profiler WCET & Pengawas Watchdog (WDT)
 */
typedef struct {
    uint32_t          sysclk_hz;            /**< Frekuensi CPU Clock (160 MHz default) */
    bool              dwt_hardware_active;  /**< Status aktif DWT CYCCNT hardware */
    uint32_t          watchdog_kicked_mask; /**< Bitmask task yang telah melapor */
    uint32_t          watchdog_required_mask;/**< Bitmask task wajib melapor */
    uint32_t          watchdog_feed_count;  /**< Jumlah berhasil menyegarkan external WDT */
    uint32_t          watchdog_suppressed_count; /**< Jumlah pulsa WDT ditahan akibat anomali */
    uint32_t          last_feed_tick_ms;    /**< Timestamp penyegaran WDT terakhir */
    bool              fault_latched;        /**< Indikasi terjadi kegagalan tenggat waktu */
    wcet_task_id_t    fault_task_id;        /**< ID tugas yang memicu fault */
    wcet_task_stats_t tasks[WCET_TASK_COUNT];
} wcet_profiler_ctx_t;

/* ========================================================================== */
/* Antarmuka Fungsi API Profiler WCET & Watchdog Supervisor                   */
/* ========================================================================== */

/**
 * @brief Inisialisasi modul profiler WCET, register ARM DWT CYCCNT, dan WDT supervisor.
 * @param sysclk_hz Frekuensi clock inti prosesor (misal 160000000 untuk 160 MHz).
 */
void wcet_profiler_init(uint32_t sysclk_hz);

/**
 * @brief Memulai pencatatan durasi eksekusi task (start timestamping DWT).
 * @param task_id ID tugas yang akan diukur.
 */
void wcet_record_start(wcet_task_id_t task_id);

/**
 * @brief Menghentikan pencatatan durasi eksekusi task, memperbarui metrik WCET,
 *        dan mengevaluasi kepatuhan terhadap batas waktu teoretis.
 * @param task_id ID tugas yang diukur.
 */
void wcet_record_stop(wcet_task_id_t task_id);

/**
 * @brief Mereset seluruh rekaman min/max/avg WCET dan counter eksekusi.
 */
void wcet_profiler_reset(void);

/**
 * @brief Mengambil pointer statistik dari suatu task.
 * @param task_id ID tugas.
 * @return Pointer ke struktur statistik task atau NULL jika tidak valid.
 */
const wcet_task_stats_t* wcet_get_task_stats(wcet_task_id_t task_id);

/**
 * @brief Mengambil pointer ke seluruh konteks profiler.
 */
const wcet_profiler_ctx_t* wcet_get_context(void);

/**
 * @brief Melaporkan keaktifan tugas (kick / check-in) ke Watchdog Supervisor.
 * @param task_mask Bitmask task yang melapor (misal WCET_WDT_TASK_TAMPER_BIT).
 */
void wcet_watchdog_kick(uint32_t task_mask);

/**
 * @brief Supervisi periodik seluruh tugas dan penyegaran fisik sirkuit Watchdog Eksternal.
 *        Sinyal toggle pin EXT_WDI_PULSE (PD12) HANYA dibangkitkan jika seluruh tugas
 *        terbukti aktif dan tidak ada yang melanggar batas WCET.
 * @return true jika watchdog berhasil disegarkan, false jika pulsa ditahan (fault).
 */
bool wcet_watchdog_supervise_and_feed(void);

/**
 * @brief Memeriksa apakah seluruh tugas beroperasi dalam kondisi sehat (tidak ada fault).
 */
bool wcet_watchdog_is_healthy(void);

/**
 * @brief Mengambil total jumlah sinyal toggle watchdog yang berhasil dibangkitkan.
 */
uint32_t wcet_watchdog_get_feed_count(void);

/**
 * @brief Mengambil total jumlah sinyal toggle watchdog yang ditahan akibat anomali.
 */
uint32_t wcet_watchdog_get_suppressed_count(void);

/**
 * @brief Menyuntikkan simulasi anomali/pelanggaran deadline pada task untuk keperluan pengujian.
 * @param task_id ID tugas yang disimulasikan melanggar batas waktu.
 */
void wcet_watchdog_inject_fault(wcet_task_id_t task_id);

/**
 * @brief Membersihkan status latched fault pada pengawas watchdog.
 */
void wcet_watchdog_clear_fault(void);

/**
 * @brief Menampilkan ringkasan tabel analisis WCET dan beban prosesor ke konsol UART
 *        sesuai format Tabel 3.3.2 Spesifikasi Sistem Smart Meter 3-Fasa.
 */
void wcet_print_summary(void);

#ifdef __cplusplus
}
#endif

#endif /* WCET_PROFILER_H */
