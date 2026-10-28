/**
 * @file bsp_display_oled.c
 * @brief Implementasi Driver Konkret Layar OLED 128x32 SSD1306 Standar SPLN Gambar 4
 */

#include "bsp_display_oled.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include <string.h>

static void bsp_oled_init(void) {
    ssd1306_Init();
}

static void bsp_oled_show_boot_screen(const char *title, const char *subtitle) {
    ssd1306_Init();
    ssd1306_Fill(Black);
    ssd1306_SetCursor(11, 4);
    ssd1306_WriteString((char *)(title ? title : "STM32U575 START"), Font_7x10, White);
    ssd1306_SetCursor(11, 18);
    ssd1306_WriteString((char *)(subtitle ? subtitle : "Booting RTOS..."), Font_7x10, White);
    ssd1306_UpdateScreen();
}

static void bsp_oled_render_frame(const display_spln_frame_t *spln_frame) {
    if (spln_frame == NULL) return;

    ssd1306_Fill(Black);

    /* BARIS 1: Simbol & Kode OBIS (Font_6x8 di Y: 0..8) */
    if (spln_frame->is_alarm_active) {
        /* INDIKATOR ALARM PROMINEN (INVERTED SOLID BANNER):
         * Kotak putih terang dengan teks hitam "!ALM" di pojok kiri atas */
        ssd1306_FillRectangle(0, 0, 25, 8, White);
        ssd1306_SetCursor(1, 0);
        ssd1306_WriteString("!ALM", Font_6x8, Black);
    } else {
        /* Status Normal: Teks "OK" standar warna putih */
        ssd1306_SetCursor(2, 0);
        ssd1306_WriteString("OK", Font_6x8, White);
    }

    /* Simbol Baterai [B] di X = 26 (SPLN Butir 6.11: Berkedip saat tegangan <= 2.7V / ERR03) */
    if (!spln_frame->is_battery_low || spln_frame->is_battery_blink_on) {
        ssd1306_SetCursor(26, 0);
        ssd1306_WriteString("[B]", Font_6x8, White);
    }

    /* Indikator Fasa Tegangan L123 di X = 46 (SPLN Butir 6.11: Indikasi tegangan fase UR, US, UT) */
    char v_str[5] = "L---";
    v_str[1] = (spln_frame->voltage_phases & 0x01) ? '1' : '-';
    v_str[2] = (spln_frame->voltage_phases & 0x02) ? '2' : '-';
    v_str[3] = (spln_frame->voltage_phases & 0x04) ? '3' : '-';
    ssd1306_SetCursor(46, 0);
    ssd1306_WriteString(v_str, Font_6x8, White);

    /* Indikator Fasa Arus / Arah Arus di X = 72 (SPLN Butir 6.11: Arah energi balik REV) */
    if (spln_frame->is_reverse_power) {
        ssd1306_SetCursor(72, 0);
        ssd1306_WriteString("REV", Font_6x8, White);
    } else {
        char i_str[5] = "I---";
        i_str[1] = (spln_frame->current_phases & 0x01) ? '1' : '-';
        i_str[2] = (spln_frame->current_phases & 0x02) ? '2' : '-';
        i_str[3] = (spln_frame->current_phases & 0x04) ? '3' : '-';
        ssd1306_SetCursor(72, 0);
        ssd1306_WriteString(i_str, Font_6x8, White);
    }

    /* Kode Register OBIS rata kanan di X = 96..127 */
    int obis_len = (int)strlen(spln_frame->obis_code);
    int x_obis = 127 - (obis_len * 6);
    if (x_obis < 94) x_obis = 94;
    ssd1306_SetCursor((uint8_t)x_obis, 0);
    ssd1306_WriteString((char *)spln_frame->obis_code, Font_6x8, White);

    /* Garis Pemisah Horizontal (Y = 10)
     * - Normal: garis putus-putus halus
     * - Alarm Aktif: garis tebal padat (solid bright alert bar) */
    if (spln_frame->is_alarm_active) {
        ssd1306_Line(0, 10, 127, 10, White);
    } else {
        for (uint8_t x = 0; x < 128; x += 2) {
            ssd1306_DrawPixel(x, 10, White);
        }
    }

    /* BARIS 2 KIRI: zz (Indeks Urutan Scrolling, Font_6x8 di Y: 22) */
    ssd1306_SetCursor(1, 22);
    ssd1306_WriteString((char *)spln_frame->scroll_index_zz, Font_6x8, White);

    /* BARIS 2 TENGAH: Nilai Angka Utama */
    if (spln_frame->is_large_font) {
        /* Nilai angka besar (Font_11x18 di Y: 13..31) */
        int val_len = (int)strlen(spln_frame->main_value);
        int x_val = 18 + (76 - (val_len * 11)) / 2;
        if (x_val < 16) x_val = 16;
        ssd1306_SetCursor((uint8_t)x_val, 13);
        ssd1306_WriteString((char *)spln_frame->main_value, Font_11x18, White);
    } else {
        /* Teks khusus / IDPEL panjang / SABOTASE (Font_7x10 di Y: 17) */
        int val_len = (int)strlen(spln_frame->main_value);
        int x_val = 18 + (82 - (val_len * 7)) / 2;
        if (x_val < 16) x_val = 16;
        ssd1306_SetCursor((uint8_t)x_val, 17);
        ssd1306_WriteString((char *)spln_frame->main_value, Font_7x10, White);
    }

    /* BARIS 2 KANAN: Satuan Besaran Listrik / Kode Error (Font_6x8 di Y: 22) */
    if (spln_frame->unit[0] != '\0') {
        int unit_len = (int)strlen(spln_frame->unit);
        int x_unit = 127 - (unit_len * 6);
        ssd1306_SetCursor((uint8_t)x_unit, 22);
        ssd1306_WriteString((char *)spln_frame->unit, Font_6x8, White);
    }

    ssd1306_UpdateScreen();
}

static const display_driver_interface_t s_bsp_oled_driver = {
    .init              = bsp_oled_init,
    .render_frame      = bsp_oled_render_frame,
    .show_boot_screen  = bsp_oled_show_boot_screen
};

const display_driver_interface_t* bsp_display_oled_get_driver(void) {
    return &s_bsp_oled_driver;
}

