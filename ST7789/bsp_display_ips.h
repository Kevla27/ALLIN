/**
 * @file bsp_display_ips.h
 * @brief Driver Konkret Layar IPS ST7789 Standar SPLN Gambar 4 (BSP Level)
 */

#ifndef BSP_DISPLAY_IPS_H
#define BSP_DISPLAY_IPS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "display_task.h"

/**
 * @brief Mengambil pointer instansiasi driver interface IPS ST7789
 * @return Pointer konstan ke struktur interface display driver
 */
const display_driver_interface_t* bsp_display_ips_get_driver(void);

/**
 * @brief Atur status lampu latar Backlight (BLK) layar IPS
 * @param on 1 untuk menyalakan, 0 untuk mematikan
 */
void bsp_display_ips_set_backlight(uint8_t on);

/**
 * @brief Baca status Backlight saat ini
 * @return 1 jika menyala, 0 jika mati
 */
uint8_t bsp_display_ips_get_backlight(void);

/**
 * @brief Toggle status Backlight (ON <-> OFF)
 */
void bsp_display_ips_toggle_backlight(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_DISPLAY_IPS_H */
