/**
 * @file display_task.c
 * @brief Implementasi FreeRTOS Task Display Carousel Layar Standar SPLN Gambar 4 (Agnostik Hardware)
 */

#include "display_task.h"
#include "display.h"
#include "rtos_tasks.h"
#include "bsp_watchdog.h"
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

    if (s_display_driver != NULL && s_display_driver->init != NULL) {
        s_display_driver->init();
    }

    display_context_t disp_ctx;
    display_init(&disp_ctx, "530000000001", 5000);

    meter_measurements_t meas;
    memset(&meas, 0, sizeof(meas));

    display_spln_frame_t spln_frame;
    uint32_t elapsed_accumulator_ms = 5000; /* Langsung render saat startup */

    for (;;) {
        bool need_render = false;

        /* Sinkronisasi status Relai READY ON & Asimetri Beban (SPLN Tabel 5 No 11 & 12) */
        power_relay_context_t *pr_ctx = rtos_power_relay_get_ctx();
        bool ready_on = (pr_ctx != NULL && power_relay_is_ready_on(pr_ctx));
        disp_ctx.relay_ready_on = ready_on;
        if (pr_ctx != NULL) {
            disp_ctx.asymmetry_warning = pr_ctx->is_asymmetry_active;
        }

        /* Tangani event tombol darurat sabotase PC13 */
        if (s_tamper_trigger_instant) {
            s_tamper_trigger_instant = false;
            elapsed_accumulator_ms = 0;
            disp_ctx.alarm_icon_active = s_tamper_alarm_active;
            if (s_tamper_alarm_active) {
                disp_ctx.active_tamper_mask = TAMPER_VECTOR_CASE_OPEN;
                disp_ctx.current_page = DISP_PAGE_TAMPER_ALARM;
            } else if (ready_on) {
                disp_ctx.current_page = DISP_PAGE_RELAY_READY_ON;
            } else {
                disp_ctx.active_tamper_mask = 0;
                disp_ctx.current_page = DISP_PAGE_IDPEL;
            }
            need_render = true;
        } else if (ready_on && disp_ctx.current_page != DISP_PAGE_RELAY_READY_ON && !s_tamper_alarm_active) {
            disp_ctx.current_page = DISP_PAGE_RELAY_READY_ON;
            need_render = true;
        } else if (elapsed_accumulator_ms >= 5000) {
            elapsed_accumulator_ms = 0;
            if (!s_tamper_alarm_active) {
                display_process_tick(&disp_ctx, 5000);
                need_render = true;
            }
        }

        if (need_render) {
            /* 1. Ambil snapshot data pengukuran terbaru secara thread-safe dari Mutex */
            rtos_meter_data_get_snapshot(&meas);
            display_update_measurements(&disp_ctx, &meas);

            /* 2. Format frame sesuai Standar Layar Meter PLN (SPLN D3.006-1 Gambar 4) */
            disp_ctx.alarm_icon_active = s_tamper_alarm_active;
            if (s_tamper_alarm_active && disp_ctx.active_tamper_mask == 0) {
                disp_ctx.active_tamper_mask = TAMPER_VECTOR_CASE_OPEN;
            } else if (!s_tamper_alarm_active) {
                disp_ctx.active_tamper_mask = 0;
            }
            display_render_spln_frame(&disp_ctx, &spln_frame);

            /* 3. Delegasikan render piksel ke driver display aktif secara agnostik */
            if (s_display_driver != NULL && s_display_driver->render_frame != NULL) {
                s_display_driver->render_frame(&spln_frame);
            }

            printf("[OLED DISP] Page %d [%s] -> OBIS: %s | %s %s | Status: %s\r\n",
                   disp_ctx.current_page,
                   spln_frame.scroll_index_zz,
                   spln_frame.obis_code,
                   spln_frame.main_value,
                   spln_frame.unit,
                   spln_frame.is_alarm_active ? "!ALM (SABOTASE)" : "NORMAL");
        }

        bsp_watchdog_kick_task(WATCHDOG_TASK_UI_BIT);
        vTaskDelay(pdMS_TO_TICKS(100));
        elapsed_accumulator_ms += 100;
    }
}
