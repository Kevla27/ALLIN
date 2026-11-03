/**
 * @file bsp_display_oled.h
 * @brief Driver Konkret Layar OLED 128x32 SSD1306 Standar SPLN Gambar 4 (BSP Level)
 */

#ifndef BSP_DISPLAY_OLED_H
#define BSP_DISPLAY_OLED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "display_task.h"

/**
 * @brief Mengambil pointer instansiasi driver interface OLED 128x32 SSD1306
 * @return Pointer konstan ke struktur interface display driver
 */
const display_driver_interface_t* bsp_display_oled_get_driver(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_DISPLAY_OLED_H */

