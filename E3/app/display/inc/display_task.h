/**
 * @file display_task.h
 * @brief FreeRTOS Task Display Carousel Layar OLED 128x32 Standar SPLN Gambar 4 (E3 Lead)
 */

#ifndef DISPLAY_TASK_H
#define DISPLAY_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include "display.h"

/**
 * @brief Antarmuka abstraksi driver perangkat keras display (Hardware Abstraction Layer)
 */
typedef struct {
    void (*init)(void);
    void (*render_frame)(const display_spln_frame_t *frame);
    void (*show_boot_screen)(const char *title, const char *subtitle);
} display_driver_interface_t;

/**
 * @brief Mendaftarkan driver perangkat keras display aktif (Dependency Injection)
 * @param driver Pointer ke interface driver display (misal OLED SSD1306, LCD, TFT, dll)
 */
void display_task_set_driver(const display_driver_interface_t *driver);

/**
 * @brief Task FreeRTOS untuk menangani render carousel layar OLED 128x32
 * @param pvParameters Parameter task (tidak digunakan)
 */
void task_oled128x32_carousel(void *pvParameters);

/**
 * @brief Memicu refresh tampilan seketika saat terjadi event penting (misal: sabotase tamper)
 * @param alarm_active Status apakah alarm sabotase aktif atau pulih
 */
void display_task_trigger_instant_refresh(bool alarm_active);

/**
 * @brief Memeriksa apakah status alarm sabotase saat ini aktif di tampilan
 * @return true jika alarm sabotase aktif, false jika normal
 */
bool display_task_is_alarm_active(void);

/**
 * @brief Inisialisasi hardware layar OLED dan menampilkan splash screen awal saat booting
 * @param title Teks judul baris 1 (NULL untuk default "STM32U575 START")
 * @param subtitle Teks subjudul baris 2 (NULL untuk default "Booting RTOS...")
 */
void display_task_show_boot_screen(const char *title, const char *subtitle);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_TASK_H */

