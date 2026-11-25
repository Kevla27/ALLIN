/**
 * @file display_task.c
 * @brief Implementasi FreeRTOS Task 5: Display Carousel Layar Standar SPLN Gambar 4 (Agnostik Hardware)
 *        Menyinkronkan IDPEL, State Relai READY ON, Tamper Ter-Latch (RUSAK) & Snapshot Metrologi
 */

#include "display_task.h"
#include "display.h"
#include "rtos_tasks.h"
#include "board_profile_config.h"
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#else
#ifndef pdMS_TO_TICKS
#define pdMS_TO_TICKS(ms) (ms)
#endif
static inline void vTaskDelay(uint32_t ticks) { (void)ticks; }
#endif
#include <string.h>
#include <stdio.h>

/* Pointer ke driver perangkat keras display aktif (Hardware Abstraction Layer) */
static const display_driver_interface_t *s_display_driver = NULL;

/* State internal display task */
static volatile bool s_tamper_alarm_active = false;
static volatile bool s_tamper_trigger_instant = false;

void display_task_set_driver(const display_driver_interface_t *driver)
{
    s_display_driver = driver;
}

void display_task_trigger_instant_refresh(bool alarm_active)
{
    s_tamper_alarm_active = alarm_active;
    s_tamper_trigger_instant = true;
}

bool display_task_is_alarm_active(void)
{
    return s_tamper_alarm_active;
}

void display_task_show_boot_screen(const char *title, const char *subtitle)
{
    if (s_display_driver != NULL && s_display_driver->show_boot_screen != NULL) {
        s_display_driver->show_boot_screen(title, subtitle);
    }
}

void task_oled128x32_carousel(void *pvParameters)
{
    (void)pvParameters;
#if defined(EMBEDDED_HARDWARE_TARGET)
    portALLOCATE_SECURE_CONTEXT(configMINIMAL_SECURE_STACK_SIZE);
#endif

    if (s_display_driver != NULL && s_display_driver->init != NULL) {
        s_display_driver->init();
    }

    char init_cust[16]  = METER_CONFIG_CUSTOMER_ID;
    char init_meter[16] = METER_CONFIG_METER_ID;
    rtos_get_device_ids(init_cust, sizeof(init_cust), init_meter, sizeof(init_meter));

    static display_context_t    disp_ctx;
    static meter_measurements_t meas;
    static display_spln_frame_t spln_frame;
    memset(&disp_ctx, 0, sizeof(disp_ctx));
    memset(&meas, 0, sizeof(meas));
    memset(&spln_frame, 0, sizeof(spln_frame));
    display_init(&disp_ctx, init_cust, init_meter, BOARD_CFG_DISPLAY_CAROUSEL_MS);
    uint32_t elapsed_accumulator_ms = BOARD_CFG_DISPLAY_CAROUSEL_MS; /* Render awal saat startup */
    uint32_t prev_active_tamp  = 0U;
    uint32_t prev_active_alarm = 0U;
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
    TickType_t xLastWakeTime = xTaskGetTickCount();
#endif

    for (;;) {
        bool need_render = false;

        /* 1. Sinkronisasi IDPEL, Relai READY ON, Asimetri, Tamper & Alarm Hardware (ERR00..ERR07) via Mutex */
        char cur_cust[16];
        char cur_meter[16];
        rtos_get_device_ids(cur_cust, sizeof(cur_cust), cur_meter, sizeof(cur_meter));
        if (strcmp(disp_ctx.customer_id, cur_cust) != 0) {
            display_update_customer_id(&disp_ctx, cur_cust);
            need_render = true;
        }
        if (strcmp(disp_ctx.meter_id, cur_meter) != 0) {
            display_update_meter_id(&disp_ctx, cur_meter);
            need_render = true;
        }

        rtos_app_state_lock();
        power_relay_context_t *pr_ctx = rtos_power_relay_get_ctx();
        tamper_context_t      *t_ctx  = rtos_tamper_get_ctx();
        bool ready_on     = (pr_ctx != NULL && power_relay_is_ready_on(pr_ctx));
        bool relay_closed = (pr_ctx == NULL || power_relay_get_state(pr_ctx) == RELAY_STATE_CONNECTED);
        uint32_t active_tamp  = ((t_ctx != NULL) ? tamper_get_active_mask(t_ctx) : 0U) | rtos_tamper_get_latched_mask();
        uint32_t active_alarm = rtos_internal_alarm_get_mask();

        disp_ctx.relay_ready_on = ready_on;
        if (pr_ctx != NULL) {
            disp_ctx.asymmetry_warning = pr_ctx->is_asymmetry_active;
        }
        rtos_app_state_unlock();

        if (s_tamper_alarm_active && active_tamp == 0U && active_alarm == 0U) {
            active_tamp = TAMPER_VECTOR_CASE_OPEN;
        }
        bool effective_alarm = (s_tamper_alarm_active || (active_tamp != 0U) || (active_alarm != 0U));
        if ((active_tamp != prev_active_tamp) || (active_alarm != prev_active_alarm)) {
            prev_active_tamp  = active_tamp;
            prev_active_alarm = active_alarm;
            s_tamper_trigger_instant = true;
        }

        /* 2. Tangani Trigger Instan (Sabotase / Alarm Hardware / Tombol) atau Rotasi Carousel 5s */
        if (s_tamper_trigger_instant) {
            s_tamper_trigger_instant = false;
            elapsed_accumulator_ms = 0U;
            disp_ctx.alarm_icon_active = effective_alarm;
            if (effective_alarm) {
                disp_ctx.active_tamper_mask = active_tamp;
                disp_ctx.active_alarm_mask  = active_alarm;
                disp_ctx.current_page       = DISP_PAGE_TAMPER_ALARM;
            } else if (ready_on) {
                disp_ctx.active_tamper_mask = 0U;
                disp_ctx.active_alarm_mask  = 0U;
                disp_ctx.current_page       = DISP_PAGE_RELAY_READY_ON;
            } else {
                disp_ctx.active_tamper_mask = 0U;
                disp_ctx.active_alarm_mask  = 0U;
                disp_ctx.current_page       = DISP_PAGE_IDPEL;
            }
            need_render = true;
        } else if (ready_on && disp_ctx.current_page != DISP_PAGE_RELAY_READY_ON && !effective_alarm) {
            disp_ctx.current_page = DISP_PAGE_RELAY_READY_ON;
            need_render = true;
        } else if (elapsed_accumulator_ms >= BOARD_CFG_DISPLAY_CAROUSEL_MS) {
            elapsed_accumulator_ms = 0U;
            if (!effective_alarm) {
                display_process_tick(&disp_ctx, BOARD_CFG_DISPLAY_CAROUSEL_MS);
                need_render = true;
            }
        }

        /* 3. Ambil Snapshot Metrologi & Render ke Hardware Display Agnostik */
        if (need_render) {
            if (rtos_meter_data_get_snapshot(&meas)) {
                display_update_measurements(&disp_ctx, &meas);
            }

            disp_ctx.alarm_icon_active  = effective_alarm;
            disp_ctx.active_tamper_mask = effective_alarm ? active_tamp : 0U;
            disp_ctx.active_alarm_mask  = effective_alarm ? active_alarm : 0U;
            display_set_time(&disp_ctx, rtos_get_rtc_epoch_sec());

            display_render_spln_frame(&disp_ctx, &spln_frame);
            spln_frame.is_relay_closed = (relay_closed && !ready_on &&
                                          ((active_tamp & TAMPER_MASK_RELAY_TRIP) == 0U) &&
                                          ((active_alarm & SPLN_ALARM_MASK_RELAY_TRIP) == 0U));

            if (s_display_driver != NULL && s_display_driver->render_frame != NULL) {
                s_display_driver->render_frame(&spln_frame);
            }
        }

        /* 4. Kick Watchdog (BIT 3) & Tunda Periodik Presisi 100 ms */
        bsp_watchdog_kick_task(WATCHDOG_TASK_UI_BIT);
#if defined(EMBEDDED_HARDWARE_TARGET) || defined(USE_FREERTOS)
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
#else
        vTaskDelay(pdMS_TO_TICKS(100));
#endif
        elapsed_accumulator_ms += 100U;
    }
}
