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
    ssd1306_DrawRectangle(0, 0, 127, 31, White);
    ssd1306_SetCursor(10, 4);
    ssd1306_WriteString((char *)(title ? title : "ALLIN SMART METER"), Font_7x10, White);
    ssd1306_SetCursor(10, 17);
    ssd1306_WriteString((char *)(subtitle ? subtitle : "SPLN D3.006:2021"), Font_6x8, White);
    ssd1306_UpdateScreen();
}

/**
 * @brief Gambar ikon status fisik relai kontaktor (X: 1..10, Y: 1..8)
 * - Tertutup/Normal: [o---o] (Kontak nyambung)
 * - Trip/Buka:       [o/  o] (Kontak terangkat)
 * - READY ON:        [o/  o] berkedip 1 Hz
 */
static void oled_draw_relay_icon(uint8_t x, uint8_t y, bool closed, bool ready_on, bool blink) {
    /* Terminal kontak kiri (2x2 piksel) */
    ssd1306_DrawPixel(x, y + 3, White);
    ssd1306_DrawPixel(x + 1, y + 3, White);
    ssd1306_DrawPixel(x, y + 4, White);
    ssd1306_DrawPixel(x + 1, y + 4, White);

    /* Terminal kontak kanan (2x2 piksel) */
    ssd1306_DrawPixel(x + 8, y + 3, White);
    ssd1306_DrawPixel(x + 9, y + 3, White);
    ssd1306_DrawPixel(x + 8, y + 4, White);
    ssd1306_DrawPixel(x + 9, y + 4, White);

    if (closed && !ready_on) {
        /* Kontak tertutup menyambungkan terminal */
        ssd1306_Line(x + 2, y + 3, x + 7, y + 3, White);
        ssd1306_Line(x + 2, y + 4, x + 7, y + 4, White);
    } else if (ready_on) {
        /* READY ON: berkedip */
        if (blink) {
            ssd1306_Line(x + 2, y + 3, x + 6, y, White);
        }
    } else {
        /* Kontak terbuka / trip */
        ssd1306_Line(x + 2, y + 3, x + 6, y, White);
    }
}

/**
 * @brief Gambar ikon indikator alarm/sabotase (X: 13..21, Y: 0..8)
 * - Alarm aktif: Segitiga peringatan tanda seru [▲!]
 * - Normal: Tanda centang halus [✓]
 */
static void oled_draw_tamper_alarm_icon(uint8_t x, uint8_t y, bool alarm_active) {
    if (alarm_active) {
        /* Segitiga peringatan */
        ssd1306_Line(x + 4, y, x, y + 7, White);
        ssd1306_Line(x + 4, y, x + 8, y + 7, White);
        ssd1306_Line(x, y + 7, x + 8, y + 7, White);
        /* Tanda seru di dalam segitiga */
        ssd1306_Line(x + 4, y + 2, x + 4, y + 4, White);
        ssd1306_DrawPixel(x + 4, y + 6, White);
    } else {
        /* Tanda centang OK */
        ssd1306_Line(x + 1, y + 4, x + 3, y + 6, White);
        ssd1306_Line(x + 3, y + 6, x + 7, y + 1, White);
    }
}

/**
 * @brief Gambar indikator baterai grafis (X: 24..35, Y: 1..7)
 * - Bodi baterai 11x7 dengan kutub positif
 * - Normal: 3 garis isi penuh
 * - Lemah (ERR03): berkedip 1 garis
 */
static void oled_draw_battery_gauge(uint8_t x, uint8_t y, bool low_battery, bool blink) {
    /* Bodi luar baterai */
    ssd1306_DrawRectangle(x, y, x + 10, y + 6, White);
    /* Kutub positif */
    ssd1306_Line(x + 11, y + 2, x + 11, y + 4, White);

    if (low_battery) {
        if (blink) {
            ssd1306_FillRectangle(x + 2, y + 2, x + 3, y + 4, White);
        }
    } else {
        ssd1306_FillRectangle(x + 2, y + 2, x + 3, y + 4, White);
        ssd1306_FillRectangle(x + 5, y + 2, x + 6, y + 4, White);
        ssd1306_FillRectangle(x + 8, y + 2, x + 9, y + 4, White);
    }
}

/**
 * @brief Gambar badge tarif ToU aktif (X: 38..56, Y: 0..8)
 * - WBP (Tarif 1): Kotak solid putih dengan teks hitam "WBP"
 * - LWBP1 (Tarif 2): Teks putih "LW1"
 * - LWBP2 (Tarif 3): Teks putih "LW2"
 */
static void oled_draw_tariff_badge(uint8_t x, uint8_t y, uint8_t zone) {
    if (zone == 0) {
        /* WBP Prominen Inverted Badge */
        ssd1306_FillRectangle(x, y, x + 18, y + 7, White);
        ssd1306_SetCursor(x + 1, y);
        ssd1306_WriteString("WBP", Font_6x8, Black);
    } else if (zone == 1) {
        ssd1306_SetCursor(x, y);
        ssd1306_WriteString("LW1", Font_6x8, White);
    } else {
        ssd1306_SetCursor(x, y);
        ssd1306_WriteString("LW2", Font_6x8, White);
    }
}

static void bsp_oled_render_frame(const display_spln_frame_t *spln_frame) {
    if (spln_frame == NULL) return;

    ssd1306_Fill(Black);

    /* BARIS 1 STATUS BAR:
     * 1. Ikon Relai Kontaktor (X=1..11)
     * 2. Ikon Alarm / Tamper (X=13..21)
     * 3. Ikon Baterai Grafis (X=24..35)
     * 4. Badge Tarif ToU (X=38..56)
     * 5. Fasa Tegangan L123 (X=59..78)
     * 6. Fasa Arus I123 / REV (X=80..99)
     * 7. Kode OBIS (X=101..127)
     */
    oled_draw_relay_icon(1, 1, spln_frame->is_relay_closed, spln_frame->is_relay_ready_on, spln_frame->is_battery_blink_on);
    oled_draw_tamper_alarm_icon(13, 0, spln_frame->is_alarm_active || spln_frame->is_tamper_active);
    oled_draw_battery_gauge(24, 1, spln_frame->is_battery_low, spln_frame->is_battery_blink_on);
    oled_draw_tariff_badge(38, 0, spln_frame->active_tariff_zone);

    /* Indikator Fasa Tegangan L123 di X = 59 */
    char v_str[5] = "L---";
    v_str[1] = (spln_frame->voltage_phases & 0x01) ? '1' : '-';
    v_str[2] = (spln_frame->voltage_phases & 0x02) ? '2' : '-';
    v_str[3] = (spln_frame->voltage_phases & 0x04) ? '3' : '-';
    ssd1306_SetCursor(59, 0);
    ssd1306_WriteString(v_str, Font_6x8, White);

    /* Indikator Fasa Arus / Arah Arus di X = 80 */
    if (spln_frame->is_reverse_power) {
        ssd1306_SetCursor(80, 0);
        ssd1306_WriteString("REV", Font_6x8, White);
    } else {
        char i_str[5] = "I---";
        i_str[1] = (spln_frame->current_phases & 0x01) ? '1' : '-';
        i_str[2] = (spln_frame->current_phases & 0x02) ? '2' : '-';
        i_str[3] = (spln_frame->current_phases & 0x04) ? '3' : '-';
        ssd1306_SetCursor(80, 0);
        ssd1306_WriteString(i_str, Font_6x8, White);
    }

    /* Kode Register OBIS rata kanan di X = 101..127 */
    int obis_len = (int)strlen(spln_frame->obis_code);
    int x_obis = 127 - (obis_len * 6);
    if (x_obis < 101) x_obis = 101;
    ssd1306_SetCursor((uint8_t)x_obis, 0);
    ssd1306_WriteString((char *)spln_frame->obis_code, Font_6x8, White);

    /* Garis Pemisah Horizontal (Y = 9) */
    if (spln_frame->is_alarm_active || spln_frame->is_tamper_active) {
        ssd1306_Line(0, 9, 127, 9, White);
    } else {
        for (uint8_t x = 0; x < 128; x += 2) {
            ssd1306_DrawPixel(x, 9, White);
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

static void bsp_oled_set_power_mode(bool sleep_mode) {
    if (sleep_mode) {
        /* Segera matikan internal DC-DC charge pump & matriks panel OLED via I2C */
        ssd1306_SetDisplayOn(0);
    } else {
        ssd1306_Init();
        ssd1306_SetDisplayOn(1);
    }
}

static const display_driver_interface_t s_bsp_oled_driver = {
    .init              = bsp_oled_init,
    .render_frame      = bsp_oled_render_frame,
    .show_boot_screen  = bsp_oled_show_boot_screen,
    .set_power_mode    = bsp_oled_set_power_mode
};

const display_driver_interface_t* bsp_display_oled_get_driver(void) {
    return &s_bsp_oled_driver;
}

