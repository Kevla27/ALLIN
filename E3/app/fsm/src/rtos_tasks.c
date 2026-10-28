/**
* @file rtos_tasks.c
* @brief Implementasi Task & Inter-Task Communication FreeRTOS E3
*/
#include "rtos_tasks.h"
#include "tamper_manager.h"
#include "load_profile.h"
#include "display.h"
#include "nvram_storage.h"
#include "dlms_task.h"
#include "dlms_obis.h"
#include "ami_server.h"
#include "display_task.h"
#include "bsp_alarm_buzzer.h"
#include "bsp_ade9000_irq.h"
#include "bsp_watchdog.h"
#include <stdio.h>
#include <string.h>

/* Stub/Simulasi untuk PC (CTest) agar tidak mencari file header FreeRTOS asli */
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
static QueueHandle_t xTamperQueue = NULL;
static SemaphoreHandle_t xMeasMutex = NULL;
#else
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) (ms)
#endif
#ifndef configTICK_RATE_HZ
#define configTICK_RATE_HZ 1000
#endif
static inline void vTaskDelay(uint32_t ticks) { (void)ticks; }
static inline uint32_t xTaskGetTickCount(void) { return 0; }
/* Ring Buffer Simulasi Queue di RAM PC */
static tamper_event_msg_t simulated_tamper_queue[TAMPER_QUEUE_MAX_ITEMS];
static size_t q_head = 0;
static size_t q_tail = 0;
static size_t q_count = 0;
#endif

/* Global Context Handlers */
static tamper_context_t g_tamper_ctx;
static load_profile_ctx_t g_load_profile_buf;
static display_context_t g_display_ctx;
static dlms_task_ctx_t s_dlms_task_ctx;
static ami_server_t s_ami_server;
static dlms_uart_tx_fn_t s_dlms_uart_tx_cb = NULL;
static power_relay_context_t g_power_relay_ctx;

/* Thread-safe Measurement Buffer & Callback */
static meter_measurements_t s_latest_measurements;
static bool s_measurements_valid = false;
static meter_sample_fn_t s_meter_sample_cb = NULL;

void rtos_system_init(void) {
    /* 1. Inisialisasi Seluruh Modul Subsystem E3, BSP & Power Relay */
    bsp_alarm_buzzer_init();
    tamper_init(&g_tamper_ctx);
    load_profile_init(&g_load_profile_buf);
    display_init(&g_display_ctx, "530000000001", 5000);
    power_relay_init(&g_power_relay_ctx, 6600, 5, 30);

    /* 2. Inisialisasi Queue & Mutex FreeRTOS / Simulasi */
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    xTamperQueue = xQueueCreate(TAMPER_QUEUE_MAX_ITEMS, sizeof(tamper_event_msg_t));
    xMeasMutex = xSemaphoreCreateMutex();
#else
    q_head = 0;
    q_tail = 0;
    q_count = 0;
#endif

    /* 3. Memuat Snapshot NVRAM Terakhir dari Flash */
    nvram_load_tamper_log_snapshot(&g_tamper_ctx);

    /* 4. Inisialisasi Stack DLMS Task & Tamper Log */
    dlms_task_init(&s_dlms_task_ctx, s_dlms_uart_tx_cb);
    ami_server_init(&s_ami_server);

    printf("[RTOS] All E3 Firmware Subsystems, Queues, Mutexes & NVRAM Initialized Successfully!\n");
}
bool rtos_queue_send_tamper_event(const tamper_event_msg_t *msg) {
 if (msg == NULL) return false;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
 if (xTamperQueue == NULL) return false;
 return (xQueueSend(xTamperQueue, msg, pdMS_TO_TICKS(10)) == pdPASS);
#else
 if (q_count >= TAMPER_QUEUE_MAX_ITEMS) return false; /* Queue Penuh */
 simulated_tamper_queue[q_head] = *msg;
 q_head = (q_head + 1) % TAMPER_QUEUE_MAX_ITEMS;
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
 if (q_count == 0) return false; /* Queue Kosong */
 *out_msg = simulated_tamper_queue[q_tail];
 q_tail = (q_tail + 1) % TAMPER_QUEUE_MAX_ITEMS;
 q_count--;
return true;
#endif
}

extern volatile bool g_tamper_alarm_active;

/* ========================================================================== */
/*           THREAD-SAFE MEASUREMENT & METROLOGY SERVICE (IPC)                */
/* ========================================================================== */

void rtos_metrology_set_sample_cb(meter_sample_fn_t cb) {
    s_meter_sample_cb = cb;
}

void rtos_meter_data_publish(const meter_measurements_t *meas) {
    if (meas == NULL) return;

#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xMeasMutex != NULL) {
        xSemaphoreTake(xMeasMutex, portMAX_DELAY);
    }
#endif

    s_latest_measurements = *meas;
    s_measurements_valid = true;

    /* Sinkronkan nilai terbaru ke kamus register OBIS DLMS */
    dlms_obis_update_from_meter(meas);
    ami_server_update_from_meter(&s_ami_server, meas);

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
    if (xMeasMutex != NULL) {
        xSemaphoreTake(xMeasMutex, portMAX_DELAY);
    }
#endif
}

void rtos_meter_data_unlock(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    if (xMeasMutex != NULL) {
        xSemaphoreGive(xMeasMutex);
    }
#endif
}

/* ========================================================================== */
/*                             TASK ENTRY POINTS                              */
/* ========================================================================== */

void task_tamper_emergency_entry(void *pvParameters) {
    (void)pvParameters;
    /* Task Darurat Sabotase (Priority 4 - High) */
    printf("[TAMPER] Task started. Menunggu event sabotase...\r\n");
    bool last_alarm_state = false;
    for (;;) {
        bool current_alarm = display_task_is_alarm_active();
        if (current_alarm != last_alarm_state) {
            last_alarm_state = current_alarm;
            uint32_t now_sec = (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ);
            if (current_alarm) {
                printf("[TAMPER] *** DARURAT! SABOTASE TERDETEKSI (CASE OPEN) ***\r\n");
                power_relay_disconnect(&g_power_relay_ctx, RELAY_REASON_MANUAL, now_sec);
            } else {
                printf("[TAMPER] Sabotase dipulihkan. Sistem kembali normal.\r\n");
                bsp_alarm_buzzer_set(false);
            }
            tamper_event_msg_t msg = {
                .timestamp = now_sec,
                .tamper_code = (uint8_t)DLMS_TAMPER_METER_COVER_OPEN,
                .is_active = current_alarm
            };
            rtos_queue_send_tamper_event(&msg);
        }

        /* Aktuasi Buzzer & LED Alarm PB7 saat alarm aktif atau relai READY ON */
        if (current_alarm) {
            bsp_alarm_buzzer_toggle(); /* 200 ms cadence untuk tamper alarm */
        } else if (power_relay_is_ready_on(&g_power_relay_ctx)) {
            static uint8_t s_ready_blink = 0;
            s_ready_blink++;
            if (s_ready_blink >= 2) { /* 400 ms cadence untuk peringatan READY ON (SPLN Tabel 5 No 12) */
                s_ready_blink = 0;
                bsp_alarm_buzzer_toggle();
            }
        } else {
            bsp_alarm_buzzer_set(false);
        }

        bsp_watchdog_kick_task(WATCHDOG_TASK_TAMPER_BIT);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void task_metrology_profiling_entry(void *pvParameters) {
    (void)pvParameters;
    /* Task Sampling Metrologi (1 Hz) & Pembekuan Profil Beban 15-Menit (Priority 3 - Medium) */
    printf("[METROLOGY] Task started. Sampling: 1000 ms. Profiling: 15 menit.\r\n");

    meter_measurements_t meas;
    memset(&meas, 0, sizeof(meas));
    uint32_t profile_interval_sec = 0;

    for (;;) {
        /* 1. Eksekusi sampling register metrologi E1 jika callback terdaftar */
        if (s_meter_sample_cb != NULL) {
            s_meter_sample_cb(&meas);
            rtos_meter_data_publish(&meas);
        }

        /* 2. Evaluasi Asimetri Arus 3-Fase (SPLN Tabel 5 No 11) */
        bool asym = power_relay_check_asymmetry(&g_power_relay_ctx, meas.current_r_mamps, meas.current_s_mamps, meas.current_t_mamps);
        if (asym) {
            printf("[ASYMMETRIC] Peringatan Asimetri Beban! Deviasi: %lu%% (Threshold: %lu%%)\r\n",
                   (unsigned long)g_power_relay_ctx.current_deviation_pct,
                   (unsigned long)g_power_relay_ctx.asymmetry_threshold_pct);
        }

        /* 3. Evaluasi Pembatas Daya (Power Limiter - SPLN Tabel 5 No 13) */
        uint32_t now_sec = (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ);
        bool tripped = power_relay_process_power(&g_power_relay_ctx, meas.active_power_w, now_sec);
        if (tripped) {
            printf("[LIMITER] *** DARURAT! Daya aktif (%ld W) melebihi batas (%lu W). Relai TRIP/PUTUS! ***\r\n",
                   (long)meas.active_power_w, (unsigned long)g_power_relay_ctx.power_limit_w);
        }

        /* 4. Penghitung interval pembekuan Profil Beban 15-Menit (900 detik) */
        profile_interval_sec++;
        if (profile_interval_sec >= 900) {
            profile_interval_sec = 0;
            load_profile_entry_t entry;
            memset(&entry, 0, sizeof(entry));
            entry.timestamp = (uint32_t)(meas.frequency_mhz);
            entry.voltage_r_dvolts = (uint16_t)meas.voltage_r_dvolts;
            entry.voltage_s_dvolts = (uint16_t)meas.voltage_s_dvolts;
            entry.voltage_t_dvolts = (uint16_t)meas.voltage_t_dvolts;
            entry.current_r_mamps = meas.current_r_mamps;
            entry.current_s_mamps = meas.current_s_mamps;
            entry.current_t_mamps = meas.current_t_mamps;
            entry.current_n_mamps = meas.current_n_mamps;
            entry.active_power_w = meas.active_power_w;
            entry.reactive_power_var = meas.reactive_power_var;
            entry.power_factor_permille = meas.power_factor_ppm;
            entry.active_energy_import_wh = meas.active_energy_wh;
            load_profile_add_entry(&g_load_profile_buf, &entry);
            printf("[PROFILE] Snapshot profil beban 15-menit berhasil dibekukan.\r\n");
        }

        bsp_watchdog_kick_task(WATCHDOG_TASK_METROLOGY_BIT);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void rtos_dlms_task_set_tx_cb(dlms_uart_tx_fn_t tx_cb) {
    s_dlms_uart_tx_cb = tx_cb;
}

void rtos_dlms_task_notify_rx(const uint8_t *data, size_t len) {
    dlms_task_notify_rx(&s_dlms_task_ctx, data, len);
}

void rtos_dlms_process_tamper_event(const tamper_event_msg_t *msg) {
    if (!msg) return;

    dlms_tamper_code_t dcode = (dlms_tamper_code_t)msg->tamper_code;
    uint8_t status = msg->is_active ? 1 : 0;

    /* 1. Tambahkan ke DLMS Tamper Log Profile Generic (0.0.99.98.0.255) */
    dlms_tamper_log_add_event(&s_dlms_task_ctx.server.tamper_log, msg->timestamp, dcode, status);
    ami_server_add_tamper_event(&s_ami_server, msg->timestamp, (uint8_t)dcode, status);

    /* 2. Update akumulasi register OBIS Tamper Counter */
    if (msg->is_active) {
        dlms_obis_increment_tamper_counter(dcode);
    }

    /* 3. Update status tamper context E3 & rekam snapshot ke Flash NVRAM */
    tamper_vector_t vec = dlms_code_to_tamper_vector(dcode);
    tamper_process_signal(&g_tamper_ctx, vec, msg->is_active, msg->timestamp);
    nvram_save_tamper_log_snapshot(&g_tamper_ctx);

    printf("[DLMS] Tamper Event dicatat! Code: 0x%02X, Status: %d, Time: %lu s (Total Log: %u)\r\n",
           msg->tamper_code, status, (unsigned long)msg->timestamp,
           (unsigned int)s_dlms_task_ctx.server.tamper_log.total_events);
}

dlms_task_ctx_t* rtos_dlms_get_task_ctx(void) {
    return &s_dlms_task_ctx;
}

ami_server_t* rtos_ami_get_server(void) {
    return &s_ami_server;
}

tamper_context_t* rtos_tamper_get_ctx(void) {
    return &g_tamper_ctx;
}

void task_dlms_entry(void *pvParameters) {
    (void)pvParameters;
    /* Task Protokol DLMS/COSEM (Priority 2 - Normal) */
    printf("[DLMS] Task started. Server DLMS/COSEM HDLC Active!\r\n");

    if (!s_dlms_task_ctx.is_running) {
        dlms_task_init(&s_dlms_task_ctx, s_dlms_uart_tx_cb);
    ami_server_init(&s_ami_server);
    }

    tamper_event_msg_t event_msg;

    for (;;) {
        /* 1. Eksekusi pemrosesan paket DLMS HDLC / APDU dari ring buffer RX */
        dlms_task_step(&s_dlms_task_ctx);

        /* 2. Tangani event sabotase dari Queue dan catat ke Log DLMS */
        if (rtos_queue_receive_tamper_event(&event_msg, 0)) {
            rtos_dlms_process_tamper_event(&event_msg);
        }

        bsp_watchdog_kick_task(WATCHDOG_TASK_DLMS_BIT);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void task_ui_display_entry(void *pvParameters) {
    (void)pvParameters;
    /* Diimplementasikan oleh task_oled128x32_carousel di display_task.c */
    for (;;) {
        bsp_watchdog_kick_task(WATCHDOG_TASK_UI_BIT);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

bool rtos_start_all_tasks(void) {
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    BaseType_t res = pdPASS;
    if (xTaskCreate(task_tamper_emergency_entry, "TamperTask", STACK_SIZE_TAMPER, NULL, PRIORITY_TASK_TAMPER, NULL) != pdPASS) {
        printf("[ERROR] Gagal membuat TamperTask!\r\n");
        res = pdFAIL;
    }
    if (xTaskCreate(task_metrology_profiling_entry, "ProfileTask", STACK_SIZE_PROFILING, NULL, PRIORITY_TASK_PROFILING, NULL) != pdPASS) {
        printf("[ERROR] Gagal membuat ProfileTask!\r\n");
        res = pdFAIL;
    }
    if (xTaskCreate(task_dlms_entry, "DLMSTask", STACK_SIZE_DLMS, NULL, PRIORITY_TASK_DLMS, NULL) != pdPASS) {
        printf("[ERROR] Gagal membuat DLMSTask!\r\n");
        res = pdFAIL;
    }
    if (xTaskCreate(task_oled128x32_carousel, "OLEDTask", STACK_SIZE_UI, NULL, PRIORITY_TASK_UI, NULL) != pdPASS) {
        printf("[ERROR] Gagal membuat OLEDTask!\r\n");
        res = pdFAIL;
    }
    if (!bsp_ade9000_irq_start_worker_task()) {
        printf("[ERROR] Gagal membuat ADE9000 IRQ Worker Task!\r\n");
        res = pdFAIL;
    }
    if (!bsp_watchdog_start_supervisor_task()) {
        printf("[ERROR] Gagal membuat Watchdog Supervisor Task!\r\n");
        res = pdFAIL;
    }
    return (res == pdPASS);
#else
    return true;
#endif
}

power_relay_context_t* rtos_power_relay_get_ctx(void) {
    return &g_power_relay_ctx;
}

void rtos_power_relay_simulate_power_trip(void) {
    power_relay_disconnect(&g_power_relay_ctx, RELAY_REASON_POWER_LIMITER, (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ));
    printf("[SIMULASI] Power Limiter tripped via konsol! Relai terbuka (DISCONNECTED).\r\n");
}

void rtos_power_relay_request_reconnect(void) {
    power_relay_request_reconnect(&g_power_relay_ctx, (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ));
    printf("[SIMULASI] Perintah Remote Reconnect DLMS diterima! Status Relai: READY ON (Menunggu Aksi Lokal).\r\n");
}

bool rtos_power_relay_confirm_reconnect(void) {
    bool ok = power_relay_confirm_reconnect(&g_power_relay_ctx, (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ));
    if (ok) {
        printf("[RELAY] Konfirmasi lokal berhasil! Relai tersambung kembali (CONNECTED).\r\n");
    } else {
        printf("[RELAY] Relai tidak dalam status READY ON (Status saat ini: %s).\r\n",
               power_relay_state_to_string(g_power_relay_ctx.relay_state));
    }
    return ok;
}

void rtos_tamper_dump_log(void) {
    dlms_tamper_log_t *log = &s_dlms_task_ctx.server.tamper_log;
    printf("\r\n======================================================================\r\n");
    printf("              TAMPER EVENT LOG (DLMS PROFILE GENERIC CLASS 7)         \r\n");
    printf("                 OBIS: 0.0.99.98.0.255 | Standar SPLN D3.006          \r\n");
    printf("======================================================================\r\n");
    printf(" Total Akumulasi Kejadian: %lu | Rekaman Aktif: %zu / %u (FIFO)\r\n",
           (unsigned long)log->total_events, log->entries_in_use, (unsigned int)DLMS_TAMPER_LOG_MAX_ENTRIES);
    printf(" Active Tamper Mask: 0x%08lX\r\n", (unsigned long)g_tamper_ctx.active_tamper_mask);
    printf("----------------------------------------------------------------------\r\n");
    printf(" No | Epoch (s)  | Kode | Deskripsi Tamper SPLN           | Status\r\n");
    printf("----+------------+------+---------------------------------+-----------\r\n");

    if (log->entries_in_use == 0) {
        printf("    (Belum ada kejadian sabotase yang tercatat di dalam log)\r\n");
    } else {
        for (size_t i = 0; i < log->entries_in_use; i++) {
            dlms_tamper_record_t rec;
            if (dlms_tamper_log_get_entry(log, i, &rec) == DLMS_OK) {
                const char *desc;
                switch ((dlms_tamper_code_t)rec.event_code) {
                    case DLMS_TAMPER_TERMINAL_COVER_OPEN: desc = "Tutup Terminal Buka (ERR20)"; break;
                    case DLMS_TAMPER_METER_COVER_OPEN:    desc = "Tutup Meter Buka (RUSAK)";    break;
                    case DLMS_TAMPER_MAGNETIC_INDUCTION:  desc = "Medan Magnet >500mT (ERR25)"; break;
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
    uint32_t now_sec;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    now_sec = (uint32_t)(xTaskGetTickCount() / configTICK_RATE_HZ);
#else
    now_sec = 1700000000;
#endif

    tamper_event_msg_t msg = {
        .timestamp = now_sec,
        .tamper_code = (uint8_t)code,
        .is_active = is_active
    };
    rtos_queue_send_tamper_event(&msg);
}


