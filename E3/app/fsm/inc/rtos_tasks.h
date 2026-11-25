/**
 * @file rtos_tasks.h
 * @brief Definisi Arsitektur 8-Task & Inter-Task Communication (IPC) FreeRTOS E3
 *        Mencakup 7 Worker Task (Tamper, Profile, DLMS, Modem, OLED, Health, Storage)
 *        + 1 Supervisor Task (defaultTask) sesuai Flow Diagram Final Tanpa Runtime WCET.
 */
#ifndef RTOS_TASKS_H
#define RTOS_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "display.h"
#include "tamper_manager.h"
#include "power_relay_manager.h"
#include "load_profile.h"
#include "nvram_storage.h"

/* ========================================================================== */
/*           PRIORITAS & ALOKASI STACK 8 TASK FREERTOS (CMSIS-RTOS V2)        */
/* ========================================================================== */

#define PRIORITY_TASK_TAMPER    32  /**< Task 1: Realtime Emergency Tamper & Last-Gasp */
#define PRIORITY_TASK_PROFILING 28  /**< Task 2: High Metrology, Voltage Prot, MD & Profiling */
#define PRIORITY_TASK_DLMS      24  /**< Task 3: Normal DLMS/COSEM HDLC Port Optik */
#define PRIORITY_TASK_MODEM     20  /**< Task 4: BelowNormal4 Modem Seluler 4G / TCP HES */
#define PRIORITY_TASK_UI        16  /**< Task 5: BelowNormal UI Display Carousel OLED */
#define PRIORITY_TASK_HEALTH    12  /**< Task 6: Low4 Telemetri ADC4 VBAT/VCAP, LSE & Self-Test */
#define PRIORITY_TASK_STORAGE   10  /**< Task 7: Low2 Worker Asinkron Flash W25Q64 */
/* Task 8 (defaultTask Supervisor & CLI) berjalan pada osPriorityLow (Prioritas 8) */

/* Ukuran Stack Task (Words - 1 word = 4 bytes; disesuaikan dgn -fstack-usage & margin >= 2x) */
#define STACK_SIZE_TAMPER       512     /**< 2.048 B: Last-Gasp NVRAM + Tamper NSC + Relay */
#define STACK_SIZE_PROFILING    768     /**< 3.072 B: ADE9000 SPI1 + Float Scaling + LP + MD */
#define STACK_SIZE_DLMS         1536    /**< 6.144 B: DLMS GET (2.16 KB frame) + AES-GCM + HDLC */
#define STACK_SIZE_MODEM        768     /**< 3.072 B: AT-Cmd 4G FSM + Last-Gasp Push UART2 */
#define STACK_SIZE_UI           768     /**< 3.072 B: SPLN Renderer + SSD1306 I2C + RTC Sync */
#define STACK_SIZE_HEALTH       512     /**< 2.048 B: ADC4 + RAM March + Flash + ADE9000 Check */
#define STACK_SIZE_STORAGE      768     /**< 3.072 B: Worker Async SPI2 Flash + CRC32 + Erase */

/* Kapasitas Antrean Pesan (Message Queues) */
#define TAMPER_QUEUE_MAX_ITEMS   10U
#define STORAGE_QUEUE_MAX_ITEMS  8U
#define MODEM_TX_QUEUE_MAX_ITEMS 4U

/* Bitmask Watchdog Supervisor (7 Worker Tasks -> 0xEF, Bit 4 dicadangkan APP_STEP legacy) */
#define WATCHDOG_TASK_TAMPER_BIT    (1U << 0)
#define WATCHDOG_TASK_METROLOGY_BIT (1U << 1)
#define WATCHDOG_TASK_DLMS_BIT      (1U << 2)
#define WATCHDOG_TASK_UI_BIT        (1U << 3)
#define WATCHDOG_TASK_MODEM_BIT     (1U << 5)
#define WATCHDOG_TASK_HEALTH_BIT    (1U << 6)
#define WATCHDOG_TASK_STORAGE_BIT   (1U << 7)
#define WATCHDOG_ALL_TASKS_MASK     (0xEFU)

/* Klasifikasi Tugas Kritis (Inti Meter) vs Non-Kritis (Antarmuka & Komunikasi)
 * Sesuai SPLN D3.006: 2021 Tabel 4 No. 5 (ERR04): hanya gangguan inti/fatal CPU yang membuka relai. */
#define WATCHDOG_CRITICAL_TASKS_MASK ( \
    WATCHDOG_TASK_TAMPER_BIT    | \
    WATCHDOG_TASK_METROLOGY_BIT | \
    WATCHDOG_TASK_HEALTH_BIT    | \
    WATCHDOG_TASK_STORAGE_BIT     \
)
#define WATCHDOG_NONCRITICAL_TASKS_MASK ( \
    WATCHDOG_TASK_DLMS_BIT  | \
    WATCHDOG_TASK_UI_BIT    | \
    WATCHDOG_TASK_MODEM_BIT   \
)

#define WDT_FATAL_RESET_THRESHOLD   3U      /**< >= 3x reset kritis beruntun -> Latch ERR04 + Buka Relai */
#define WDT_STABLE_UPTIME_CLEAR_MS  10000U  /**< Uptime stabil 10s mereset streak glitch sementara */

/* Format Register Persistensi Boot/Watchdog/Fault MCU (RTC_BKP_DR10 Domain VSW & Host Mirror) */
#ifndef WDT_BOOT_REG_MAGIC
#define WDT_BOOT_REG_MAGIC          0xA5500000U
#define WDT_BOOT_REG_MAGIC_MASK     0xFFF00000U
#define WDT_BOOT_REG_SRC_SHIFT      16U
#define WDT_BOOT_REG_SRC_MASK       0x000F0000U
#define WDT_BOOT_REG_TASK_SHIFT     8U
#define WDT_BOOT_REG_TASK_MASK      0x0000FF00U
#define WDT_BOOT_REG_ERR04_BIT      (1U << 7)
#define WDT_BOOT_REG_PENDING_BIT    (1U << 6)
#define WDT_BOOT_REG_STREAK_MASK    0x0000000FU
#endif

typedef enum {
    WDT_SEVERITY_NORMAL = 0,         /**< Seluruh task sehat normal */
    WDT_SEVERITY_MINOR_DEGRADED,     /**< Task non-kritis terganggu: TIDAK ERR04, TIDAK buka relai */
    WDT_SEVERITY_GLITCH_RECOVERING,  /**< Pasca glitch kritis < 3x: pulih tanpa ERR04/trip, menunggu stabil 10s */
    WDT_SEVERITY_FATAL_ERR04         /**< Kegagalan fatal MCU (>= 3x / Fault / Clock NMI): Latch ERR04 + Trip Relai */
} wdt_fault_severity_t;

typedef enum {
    WDT_SRC_NONE = 0,
    WDT_SRC_CRITICAL_TASK_HANG = 1,
    WDT_SRC_CPU_FAULT          = 2,
    WDT_SRC_STACK_OVERFLOW     = 3,
    WDT_SRC_CLOCK_NMI          = 4,
    WDT_SRC_CLI_INJECT         = 5
} wdt_fault_source_t;

typedef struct {
    wdt_fault_severity_t severity;
    wdt_fault_source_t   last_fault_source;
    uint32_t             offending_task_mask;
    uint32_t             minor_degraded_mask;
    uint32_t             minor_fault_count;
    uint32_t             reset_streak;
    bool                 err04_latched;
    bool                 pending_reset;
    uint32_t             stable_uptime_ms;
} wdt_supervisor_status_t;

/* Bitmask Kebijakan Tamper Ter-Latch Permanen (Irreversible Tanpa Clear Resmi PLN) */
#define TAMPER_LATCHED_MASK         (TAMPER_VECTOR_CASE_OPEN | TAMPER_VECTOR_TERMINAL_OPEN)

/* Flag Event Group Manajemen Listrik Padam & Mode Stop 2 */
#define PWR_EVT_AC_FAIL_BIT         (1U << 0)
#define PWR_EVT_EMERGENCY_SAVED     (1U << 1)
#define PWR_EVT_ENTER_STOP2         (1U << 2)
#define PWR_EVT_USER_WAKEUP_10S     (1U << 3)

/* ========================================================================== */
/*                    STRUKTUR PESAN IPC & KONTEKS TASK                       */
/* ========================================================================== */

/**
 * @brief Callback pengiriman byte UART dari DLMS Task
 */
typedef void (*dlms_uart_tx_fn_t)(const uint8_t *data, size_t len);

/**
 * @brief Konteks Server & Task DLMS E3 Bridge
 */
typedef struct {
    bool is_running;
    dlms_uart_tx_fn_t tx_cb;
    struct {
        dlms_tamper_log_t tamper_log;
    } server;
} dlms_task_ctx_t;

/**
 * @brief Konteks AMI Server Mirror E3
 */
typedef struct {
    meter_measurements_t latest_meas;
    uint32_t last_tamper_epoch;
    uint8_t  last_tamper_code;
    uint8_t  last_tamper_status;
} ami_server_t;

/**
 * @brief Struktur Pesan Queue Event Sabotase / Tamper
 */
typedef struct {
    uint32_t timestamp;   /**< Waktu kejadian (Epoch UNIX) */
    uint8_t  tamper_code; /**< Kode jenis sabotase */
    bool     is_active;   /**< Status: true = Terdeteksi, false = Pulih */
} tamper_event_msg_t;

/**
 * @brief Jenis Permintaan Penulisan Flash Asinkron ke StorageTask
 */
typedef enum {
    STORAGE_REQ_ENERGY_CHECKPOINT = 0, /**< Simpan Ping-Pong Checkpoint Energi + CRC32 */
    STORAGE_REQ_LOAD_PROFILE,          /**< Simpan Record 23-Kanal Profil Beban 15m */
    STORAGE_REQ_TAMPER_LOG,            /**< Simpan Snapshot Logbook Tamper ke Sektor 1 */
    STORAGE_REQ_DEV_CONFIG             /**< Simpan IDPEL & Meter ID ke Sektor 4 */
} storage_req_type_t;

/**
 * @brief Struktur Pesan Antrean xStorageQueue
 */
typedef struct {
    storage_req_type_t type;
    uint32_t           index; /**< Indeks entri (khusus STORAGE_REQ_LOAD_PROFILE) */
    union {
        nvram_energy_checkpoint_t checkpoint;
        load_profile_entry_t      load_profile;
        nvram_device_config_t     dev_config;
    } payload;
} storage_write_req_t;

/**
 * @brief Jenis Paket Push Darurat / Alert ke ModemTask (4G)
 */
typedef enum {
    MODEM_PUSH_LAST_GASP = 0,  /**< Notifikasi Listrik Padam (AC_FAIL_DET) */
    MODEM_PUSH_TAMPER_ALERT,   /**< Notifikasi Perubahan Sabotase Aktif/Latch */
    MODEM_PUSH_HEARTBEAT       /**< Sinyal Keep-Alive Periodik ke HES PLN */
} modem_push_type_t;

/**
 * @brief Status Finite State Machine Modem Seluler 4G
 */
typedef enum {
    MODEM_STATE_INIT = 0,
    MODEM_STATE_CONNECTING,
    MODEM_STATE_READY,
    MODEM_STATE_ERROR
} modem_fsm_state_t;

/**
 * @brief Struktur Pesan Antrean xModemTxQueue
 */
typedef struct {
    modem_push_type_t type;
    uint32_t          timestamp;
    uint64_t          active_energy_wh;
    uint8_t           tamper_code;
    bool              is_active;
} modem_push_msg_t;

/* ========================================================================== */
/*              CALLBACK INTEGRASI LAYANAN EKSTERNAL (E1 / E2 / E3)           */
/* ========================================================================== */

typedef void (*meter_sample_fn_t)(meter_measurements_t *out_meas);
typedef void (*rtos_meas_hook_fn_t)(const meter_measurements_t *meas);
typedef void (*rtos_checkpoint_build_fn_t)(nvram_energy_checkpoint_t *out_cp, const meter_measurements_t *meas);
typedef void (*rtos_simple_hook_fn_t)(void);
typedef void (*rtos_elapsed_hook_fn_t)(uint32_t elapsed_ms);
typedef bool (*rtos_modem_step_fn_t)(modem_fsm_state_t *state, const modem_push_msg_t *optional_push);

void rtos_metrology_set_sample_cb(meter_sample_fn_t cb);
void rtos_profile_set_1s_cb(rtos_meas_hook_fn_t cb);
void rtos_checkpoint_set_build_cb(rtos_checkpoint_build_fn_t cb);
void rtos_health_set_check_cb(rtos_simple_hook_fn_t cb);
void rtos_modem_set_step_cb(rtos_modem_step_fn_t cb);
void rtos_dlms_set_optical_poll_cb(rtos_simple_hook_fn_t cb);
void rtos_relay_set_actuate_cb(rtos_elapsed_hook_fn_t cb);
void rtos_power_set_stop2_cb(rtos_elapsed_hook_fn_t cb);

/* ========================================================================== */
/*                   INISIALISASI & LAYANAN IPC ANTAR-TASK                    */
/* ========================================================================== */

void rtos_system_init(void);

/* Queue xTamperQueue */
bool rtos_queue_send_tamper_event(const tamper_event_msg_t *msg);
bool rtos_queue_send_tamper_event_from_isr(const tamper_event_msg_t *msg);
bool rtos_queue_receive_tamper_event(tamper_event_msg_t *out_msg, uint32_t timeout_ms);

/* Queue xStorageQueue */
bool rtos_queue_send_storage_req(const storage_write_req_t *req);
bool rtos_queue_receive_storage_req(storage_write_req_t *out_req, uint32_t timeout_ms);

/* Queue xModemTxQueue */
bool rtos_queue_send_modem_push(const modem_push_msg_t *msg);
bool rtos_queue_send_modem_push_from_isr(const modem_push_msg_t *msg);
bool rtos_queue_receive_modem_push(modem_push_msg_t *out_msg, uint32_t timeout_ms);

/* Mutex Snapshot Metrologi (xMeasMutex) */
void rtos_meter_data_publish(const meter_measurements_t *meas);
bool rtos_meter_data_get_snapshot(meter_measurements_t *out_meas);
void rtos_meter_data_lock(void);
void rtos_meter_data_unlock(void);

/* Mutex State Aplikasi E3 (xAppStateMutex) & Bus Flash SPI2 (xSpiFlashMutex) */
void rtos_app_state_lock(void);
void rtos_app_state_unlock(void);
void rtos_flash_lock(void);
void rtos_flash_unlock(void);

/* Event Group Manajemen Daya (xPowerEventGroup) */
void     rtos_power_notify_ac_fail(bool is_failed);
void     rtos_power_set_event_bits(uint32_t bits);
void     rtos_power_clear_event_bits(uint32_t bits);
uint32_t rtos_power_get_event_bits(void);

/* ========================================================================== */
/*        KEBIJAKAN TAMPER TER-LATCH PERMANEN & SINKRONISASI IDENTITAS        */
/* ========================================================================== */

bool     rtos_tamper_is_latched_vector(tamper_vector_t vec);
void     rtos_tamper_apply_latch(uint32_t vector_mask, uint32_t timestamp_sec);
bool     rtos_tamper_evaluate_clear(tamper_vector_t vec, bool is_authorized_clear, uint32_t timestamp_sec);
void     rtos_tamper_clear_authorized(uint32_t clear_mask);
uint32_t rtos_tamper_get_latched_mask(void);

void rtos_set_device_ids(const char *customer_id, const char *meter_id);
void rtos_get_device_ids(char *out_customer_id, size_t cust_sz, char *out_meter_id, size_t meter_sz);

/* ========================================================================== */
/*                   ENTRY POINTS 7 WORKER TASK & PELUNCURAN                  */
/* ========================================================================== */

void task_tamper_emergency_entry(void *pvParameters);
void task_metrology_profiling_entry(void *pvParameters);
void task_dlms_entry(void *pvParameters);
void task_modem_cellular_entry(void *pvParameters);
void task_ui_display_entry(void *pvParameters);
void task_system_health_entry(void *pvParameters);
void task_storage_logger_entry(void *pvParameters);

bool rtos_start_all_tasks(void);

/* ========================================================================== */
/*                   API DLMS, RELAI, LOGBOOK & WATCHDOG                      */
/* ========================================================================== */

void rtos_dlms_task_set_tx_cb(dlms_uart_tx_fn_t tx_cb);
void rtos_dlms_task_notify_rx(const uint8_t *data, size_t len);
void rtos_dlms_process_tamper_event(const tamper_event_msg_t *msg);
uint32_t dlms_obis_get_tamper_counter(dlms_tamper_code_t code);

dlms_task_ctx_t*       rtos_dlms_get_task_ctx(void);
ami_server_t*          rtos_ami_get_server(void);
tamper_context_t*      rtos_tamper_get_ctx(void);
power_relay_context_t* rtos_power_relay_get_ctx(void);
load_profile_ctx_t*    rtos_load_profile_get_ctx(void);
modem_fsm_state_t      rtos_modem_get_state(void);

void rtos_power_relay_simulate_power_trip(void);
void rtos_power_relay_request_reconnect(void);
bool rtos_power_relay_confirm_reconnect(void);
void rtos_tamper_dump_log(void);
void rtos_tamper_record_event(dlms_tamper_code_t code, bool is_active);

void     bsp_watchdog_kick_task(uint32_t task_bit);
bool     rtos_watchdog_supervise_and_feed(void);
uint32_t rtos_watchdog_get_kicked_mask(void);
void     rtos_watchdog_on_boot(void);
void     rtos_watchdog_record_fatal_trap(wdt_fault_source_t src, uint32_t task_mask);
void     rtos_watchdog_clear_err04(void);
bool     rtos_watchdog_is_err04_latched(void);
uint32_t rtos_watchdog_get_minor_fault_mask(void);
void     rtos_watchdog_get_status(wdt_supervisor_status_t *out_status);
void     rtos_internal_alarm_set_mask(uint32_t alarm_mask);
uint32_t rtos_internal_alarm_get_mask(void);
const char* rtos_watchdog_severity_to_string(wdt_fault_severity_t sev);
const char* rtos_watchdog_source_to_string(wdt_fault_source_t src);
uint32_t rtos_get_rtc_epoch_sec(void);
#if !defined(EMBEDDED_HARDWARE_TARGET) && !defined(USE_FREERTOS)
void     rtos_watchdog_test_set_tick_ms(uint32_t tick_ms);
void     rtos_watchdog_test_reset_persist(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* RTOS_TASKS_H */
