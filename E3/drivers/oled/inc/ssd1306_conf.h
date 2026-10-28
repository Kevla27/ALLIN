/**
 * @file ssd1306_conf.h
 * @brief Konfigurasi Library Driver OLED SSD1306 128x32 untuk STM32U585 (ALLIN Smart Meter)
 */

#ifndef __SSD1306_CONF_H__
#define __SSD1306_CONF_H__

/* Mode komunikasi bus display: I2C3 (PC0=SCL, PC1=SDA) */
#define SSD1306_USE_I2C

/* Target MCU Family */
#ifndef STM32U5
#define STM32U5
#endif

/* Resolusi layar fisik standar SPLN Gambar 4: 128x32 */
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  32

/* Font yang digunakan oleh tata letak standar SPLN Gambar 4 */
#define SSD1306_INCLUDE_FONT_6x8
#define SSD1306_INCLUDE_FONT_7x10
#define SSD1306_INCLUDE_FONT_11x18

#endif /* __SSD1306_CONF_H__ */

