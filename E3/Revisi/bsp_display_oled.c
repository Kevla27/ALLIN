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
 * @brief Gambar ikon status fisik relai kontaktor (X: 0..8, Y: 1..7)
 *        Sesuai SPLN D3.006:2021 Butir 6.11 Gambar 6 (Simbol Sakelar Terbuka/Tertutup)
 * - Tertutup/Normal: [--o---o--] (Terminal lingkaran tersambung kontak horizontal)
 * - Trip/Buka:       [--o / o--] (Bilah sakelar terangkat miring ke atas)
 * - READY ON:        Bilah sakelar berkedip 1 Hz
 */
static void oled_draw_relay_icon(uint8_t x, uint8_t y, bool closed, bool ready_on, bool blink) {
    /* Terminal lingkaran kiri berlubang di (x+1, y+5) */
    ssd1306_DrawPixel(x, y + 5, White);         /* Ekor kiri */
    ssd1306_DrawPixel(x + 1, y + 4, White);     /* Busur atas */
    ssd1306_DrawPixel(x + 1, y + 6, White);     /* Busur bawah */
    ssd1306_DrawPixel(x + 2, y + 5, White);     /* Busur kanan */

    /* Terminal lingkaran kanan berlubang di (x+7, y+5) */
    ssd1306_DrawPixel(x + 6, y + 5, White);     /* Busur kiri */
    ssd1306_DrawPixel(x + 7, y + 4, White);     /* Busur atas */
    ssd1306_DrawPixel(x + 7, y + 6, White);     /* Busur bawah */
    ssd1306_DrawPixel(x + 8, y + 5, White);     /* Ekor kanan */

    if (closed && !ready_on) {
        /* Kontak tertutup menyambungkan kedua terminal */
        ssd1306_Line(x + 2, y + 5, x + 6, y + 5, White);
    } else if (ready_on) {
        /* READY ON: bilah sakelar berkedip */
        if (blink) {
            ssd1306_Line(x + 7, y + 4, x + 3, y + 1, White);
        }
    } else {
        /* Kontak terbuka / trip: bilah sakelar terangkat miring ke kiri atas */
        ssd1306_Line(x + 7, y + 4, x + 3, y + 1, White);
    }
}

/**
 * @brief Gambar ikon modem seluler / komunikasi (X: 10..16, Y: 1..7)
 *        Sesuai SPLN D3.006:2021 Butir 6.11 Gambar 6 (Gagang Telepon dalam Lingkaran)
 * - Aktif: Siluet gagang telepon (earpiece, handle, mouthpiece) menyala di dalam lingkaran
 * - Idle: Lingkaran luar standby
 */
static void oled_draw_modem_icon(uint8_t x, uint8_t y, bool active, bool blink) {
    /* 1. Lingkaran luar diameter 7 piksel */
    ssd1306_DrawPixel(x + 2, y + 0, White);
    ssd1306_DrawPixel(x + 3, y + 0, White);
    ssd1306_DrawPixel(x + 4, y + 0, White);

    ssd1306_DrawPixel(x + 1, y + 1, White);
    ssd1306_DrawPixel(x + 5, y + 1, White);

    ssd1306_DrawPixel(x + 0, y + 2, White);
    ssd1306_DrawPixel(x + 6, y + 2, White);
    ssd1306_DrawPixel(x + 0, y + 3, White);
    ssd1306_DrawPixel(x + 6, y + 3, White);
    ssd1306_DrawPixel(x + 0, y + 4, White);
    ssd1306_DrawPixel(x + 6, y + 4, White);

    ssd1306_DrawPixel(x + 1, y + 5, White);
    ssd1306_DrawPixel(x + 5, y + 5, White);

    ssd1306_DrawPixel(x + 2, y + 6, White);
    ssd1306_DrawPixel(x + 3, y + 6, White);
    ssd1306_DrawPixel(x + 4, y + 6, White);

    /* 2. Siluet gagang telepon klasik di dalam lingkaran */
    if (active && blink) {
        /* Earpiece atas */
        ssd1306_DrawPixel(x + 2, y + 1, White);
        ssd1306_DrawPixel(x + 3, y + 1, White);
        /* Gagang lengkung */
        ssd1306_DrawPixel(x + 3, y + 2, White);
        ssd1306_DrawPixel(x + 3, y + 3, White);
        ssd1306_DrawPixel(x + 3, y + 4, White);
        /* Mouthpiece bawah */
        ssd1306_DrawPixel(x + 2, y + 5, White);
        ssd1306_DrawPixel(x + 3, y + 5, White);
    }
}

/**
 * @brief Gambar ikon indikator alarm/sabotase (X: 8..14, Y: 0..8)
 * - Alarm aktif: Segitiga peringatan tanda seru [▲!]
 * - Normal: Tanda centang halus [✓]
 */
static void oled_draw_tamper_alarm_icon(uint8_t x, uint8_t y, bool alarm_active) {
    if (alarm_active) {
        /* Segitiga peringatan */
        ssd1306_Line(x + 3, y, x, y + 7, White);
        ssd1306_Line(x + 3, y, x + 6, y + 7, White);
        ssd1306_Line(x, y + 7, x + 6, y + 7, White);
        /* Tanda seru di dalam segitiga */
        ssd1306_Line(x + 3, y + 2, x + 3, y + 4, White);
        ssd1306_DrawPixel(x + 3, y + 6, White);
    } else {
        /* Tanda centang OK */
        ssd1306_Line(x + 1, y + 4, x + 3, y + 6, White);
        ssd1306_Line(x + 3, y + 6, x + 6, y + 2, White);
    }
}

/**
 * @brief Gambar indikator baterai grafis (X: 16..23, Y: 1..7)
 * - Bodi baterai 7x7 dengan kutub positif
 * - Normal: 2 garis level isi
 * - Lemah (ERR03): berkedip 1 garis
 */
static void oled_draw_battery_gauge(uint8_t x, uint8_t y, bool low_battery, bool blink) {
    /* Bodi luar baterai */
    ssd1306_DrawRectangle(x, y, x + 6, y + 6, White);
    /* Kutub positif */
    ssd1306_Line(x + 7, y + 2, x + 7, y + 4, White);

    if (low_battery) {
        if (blink) {
            ssd1306_FillRectangle(x + 2, y + 2, x + 3, y + 4, White);
        }
    } else {
        ssd1306_Line(x + 2, y + 2, x + 2, y + 4, White);
        ssd1306_Line(x + 4, y + 2, x + 4, y + 4, White);
    }
}

/**
 * @brief Gambar ikon panah arah aliran energi aktif (SPLN D3.006:2021 Butir 6.11 Gambar 6)
 * - Normal (Impor PLN / Beban): Panah solid mengarah ke kanan [►]
 * - Terbalik (Ekspor PLTS): Panah solid mengarah ke kiri [◄] (berkedip saat reverse)
 *
 * @param x Koordinat kiri ikon (X: 25..29)
 * @param y Koordinat atas ikon (Y: 1..7)
 * @param is_reverse True jika daya aktif bernilai negatif (aliran terbalik)
 * @param blink Flag kedip 1 Hz saat kondisi terbalik
 */
static void oled_draw_power_direction_arrow(uint8_t x, uint8_t y, bool is_reverse, bool blink) {
    if (is_reverse) {
        /* Aliran Terbalik / Ekspor PLTS: Panah Menghadap Kiri [◄] (berkedip) */
        if (blink) {
            for (uint8_t row = 0; row < 7; row++) {
                uint8_t width = (row <= 3) ? (row + 2) : (8 - row);
                if (width > 5) width = 5;
                ssd1306_Line(x + 5 - width, y + row, x + 4, y + row, White);
            }
        }
    } else {
        /* Aliran Normal / Impor PLN: Panah Menghadap Kanan [►] (solid) */
        for (uint8_t row = 0; row < 7; row++) {
            uint8_t width = (row <= 3) ? (row + 2) : (8 - row);
            if (width > 5) width = 5;
            ssd1306_Line(x, y + row, x + width - 1, y + row, White);
        }
    }
}

/**
 * @brief Gambar ikon indikator aliran daya 4-kuadran (SPLN D3.006:2021 Butir 6.11 Gambar 6 & Lampiran A)
 *        Berupa lingkaran 7x7 piksel dengan sumbu silang (+).
 *        Sektor kuadran aktif diarsir padat:
 *        - Kuadran 1 (Q1: +P, +Q): Kanan Atas (Beban Induktif)
 *        - Kuadran 2 (Q2: -P, +Q): Kiri Atas (Ekspor PLTS Lagging)
 *        - Kuadran 3 (Q3: -P, -Q): Kiri Bawah (Ekspor PLTS Leading)
 *        - Kuadran 4 (Q4: +P, -Q): Kanan Bawah (Beban Kapasitif)
 *
 * @param x Koordinat kiri ikon (X: 31..37)
 * @param y Koordinat atas ikon (Y: 1..7)
 * @param quadrant Nilai kuadran daya (1..4, atau 0 saat idle)
 */
static void oled_draw_quadrant_icon(uint8_t x, uint8_t y, uint8_t quadrant) {
    /* 1. Gambar lingkaran luar (Outer Circle) diameter 7 piksel (radius 3) */
    ssd1306_DrawPixel(x + 2, y + 0, White);
    ssd1306_DrawPixel(x + 3, y + 0, White);
    ssd1306_DrawPixel(x + 4, y + 0, White);

    ssd1306_DrawPixel(x + 1, y + 1, White);
    ssd1306_DrawPixel(x + 5, y + 1, White);

    ssd1306_DrawPixel(x + 0, y + 2, White);
    ssd1306_DrawPixel(x + 6, y + 2, White);

    ssd1306_DrawPixel(x + 0, y + 3, White);
    ssd1306_DrawPixel(x + 6, y + 3, White);

    ssd1306_DrawPixel(x + 0, y + 4, White);
    ssd1306_DrawPixel(x + 6, y + 4, White);

    ssd1306_DrawPixel(x + 1, y + 5, White);
    ssd1306_DrawPixel(x + 5, y + 5, White);

    ssd1306_DrawPixel(x + 2, y + 6, White);
    ssd1306_DrawPixel(x + 3, y + 6, White);
    ssd1306_DrawPixel(x + 4, y + 6, White);

    /* 2. Gambar sumbu silang kuadran (Crosshair Axes) */
    ssd1306_Line(x, y + 3, x + 6, y + 3, White); /* Sumbu horizontal (P) */
    ssd1306_Line(x + 3, y, x + 3, y + 6, White); /* Sumbu vertikal (Q) */

    /* 3. Isi solid sektor kuadran yang sedang aktif */
    if (quadrant == 1) {
        /* Kuadran 1: Kanan Atas (+P, +Q) */
        ssd1306_FillRectangle(x + 4, y + 1, x + 5, y + 2, White);
    } else if (quadrant == 2) {
        /* Kuadran 2: Kiri Atas (-P, +Q) */
        ssd1306_FillRectangle(x + 1, y + 1, x + 2, y + 2, White);
    } else if (quadrant == 3) {
        /* Kuadran 3: Kiri Bawah (-P, -Q) */
        ssd1306_FillRectangle(x + 1, y + 4, x + 2, y + 5, White);
    } else if (quadrant == 4) {
        /* Kuadran 4: Kanan Bawah (+P, -Q) */
        ssd1306_FillRectangle(x + 4, y + 4, x + 5, y + 5, White);
    }
}

/**
 * @brief Gambar badge tarif ToU aktif (X: 49..54, Y: 1..7)
 * - WBP (Tarif 1): Kotak solid putih dengan angka '1' hitam (prominen WBP)
 * - LWBP1 (Tarif 2): Kotak outline dengan angka '2' putih
 * - LWBP2 (Tarif 3): Kotak outline dengan angka '3' putih
 */
static void oled_draw_tariff_badge(uint8_t x, uint8_t y, uint8_t zone) {
    if (zone == 0) {
        /* WBP Prominen Inverted Badge (Kotak solid putih dengan angka '1' hitam) */
        ssd1306_FillRectangle(x, y, x + 5, y + 6, White);
        /* Angka '1' hitam */
        ssd1306_DrawPixel(x + 2, y + 2, Black);
        for (uint8_t row = 1; row <= 5; row++) {
            ssd1306_DrawPixel(x + 3, y + row, Black);
        }
        for (uint8_t col = 2; col <= 4; col++) {
            ssd1306_DrawPixel(x + col, y + 5, Black);
        }
    } else if (zone == 1) {
        /* LWBP1 (Tarif 2): Kotak outline dengan angka '2' */
        ssd1306_DrawRectangle(x, y, x + 5, y + 6, White);
        ssd1306_DrawPixel(x + 2, y + 2, White);
        ssd1306_DrawPixel(x + 3, y + 2, White);
        ssd1306_DrawPixel(x + 3, y + 3, White);
        ssd1306_DrawPixel(x + 2, y + 4, White);
        for (uint8_t col = 2; col <= 4; col++) {
            ssd1306_DrawPixel(x + col, y + 5, White);
        }
    } else {
        /* LWBP2 (Tarif 3): Kotak outline dengan angka '3' */
        ssd1306_DrawRectangle(x, y, x + 5, y + 6, White);
        for (uint8_t col = 2; col <= 4; col++) {
            ssd1306_DrawPixel(x + col, y + 2, White);
            ssd1306_DrawPixel(x + col, y + 4, White);
            ssd1306_DrawPixel(x + col, y + 5, White);
        }
        ssd1306_DrawPixel(x + 3, y + 3, White);
    }
}

static void bsp_oled_render_frame(const display_spln_frame_t *spln_frame) {
    if (spln_frame == NULL) return;

    ssd1306_Fill(Black);

    /* BARIS 1 STATUS BAR (SPLN D3.006:2021 Butir 6.11 Gambar 6):
     * 1. Ikon Relai Kontaktor (X=0..8) [--o / o--]
     * 2. Ikon Modem Komunikasi (X=10..16) [(☎)]
     * 3. Ikon Alarm / Tamper (X=18..24) [▲!]
     * 4. Ikon Baterai Grafis (X=26..33) [B]
     * 5. Ikon Panah Arah Energi Aktif (X=35..39) [►] Maju / [◄] Terbalik
     * 6. Ikon Kuadran Aliran Daya (X=41..47) [⊕] 4-Kuadran
     * 7. Badge Tarif ToU T1/T2/T3 (X=49..54)
     * 8. Fasa Tegangan URST (SPLN Gambar 6) (X=56..79)
     * 9. Fasa Arus IRST (SPLN Gambar 6) (X=81..104)
     * 10. Kode Register SPLN (UR/US/UT) / OBIS (X=106..127)
     */
    oled_draw_relay_icon(0, 1, spln_frame->is_relay_closed, spln_frame->is_relay_ready_on, spln_frame->is_battery_blink_on);
    oled_draw_modem_icon(10, 1, spln_frame->is_modem_active, spln_frame->is_battery_blink_on);
    oled_draw_tamper_alarm_icon(18, 0, spln_frame->is_alarm_active || spln_frame->is_tamper_active);
    oled_draw_battery_gauge(26, 1, spln_frame->is_battery_low, spln_frame->is_battery_blink_on);
    oled_draw_power_direction_arrow(35, 1, spln_frame->is_reverse_power, spln_frame->is_battery_blink_on);
    oled_draw_quadrant_icon(41, 1, spln_frame->power_quadrant);
    oled_draw_tariff_badge(49, 1, spln_frame->active_tariff_zone);

    /* BARIS 1 STATUS BAR: Indikator Fasa Tegangan & Arus Sesuai Opsi A (Mengikuti Halaman Aktif) */
    char v_str[5] = "";
    char i_str[5] = "";

    switch (spln_frame->page_id) {
        /* 1. Pengukuran Terkait Fasa R: Hanya fasa R yang menyala */
        case DISP_PAGE_VOLTAGE_R:
        case DISP_PAGE_CURRENT_R:
        case DISP_PAGE_ANGLE_UR:
        case DISP_PAGE_ANGLE_IR:
            if (spln_frame->voltage_phases & 0x01) {
                strcpy(v_str, "UR  ");
            }
            if (spln_frame->current_phases & 0x01) {
                strcpy(i_str, "IR  ");
            }
            break;

        /* 2. Pengukuran Terkait Fasa S: Hanya fasa S yang menyala */
        case DISP_PAGE_VOLTAGE_S:
        case DISP_PAGE_CURRENT_S:
        case DISP_PAGE_ANGLE_US:
        case DISP_PAGE_ANGLE_IS:
            if (spln_frame->voltage_phases & 0x02) {
                strcpy(v_str, "U S ");
            }
            if (spln_frame->current_phases & 0x02) {
                strcpy(i_str, "I S ");
            }
            break;

        /* 3. Pengukuran Terkait Fasa T: Hanya fasa T yang menyala */
        case DISP_PAGE_VOLTAGE_T:
        case DISP_PAGE_CURRENT_T:
        case DISP_PAGE_ANGLE_UT:
        case DISP_PAGE_ANGLE_IT:
            if (spln_frame->voltage_phases & 0x04) {
                strcpy(v_str, "U  T");
            }
            if (spln_frame->current_phases & 0x04) {
                strcpy(i_str, "I  T");
            }
            break;

        /* 4. Arus Netral */
        case DISP_PAGE_CURRENT_N:
            strcpy(i_str, "IN");
            break;

        /* 5. Halaman Non-Metrologi / Informasi Umum: Tidak menampilkan fasa agar baris 1 lapang */
        case DISP_PAGE_IDPEL:
        case DISP_PAGE_METER_ID:
        case DISP_PAGE_TIME:
        case DISP_PAGE_DATE:
        case DISP_PAGE_TEST_DISPLAY:
        case DISP_PAGE_CHECKSUM:
        case DISP_PAGE_PLN_UNIT:
        case DISP_PAGE_TAMPER_ALARM:
        case DISP_PAGE_RELAY_READY_ON:
            v_str[0] = '\0';
            i_str[0] = '\0';
            break;

        /* 6. Halaman Metrologi Total 3-Fasa (Energi Aktif, Reaktif, Daya Total, PF, Freq, dll) */
        default:
            v_str[0] = 'U';
            v_str[1] = (spln_frame->voltage_phases & 0x01) ? 'R' : '-';
            v_str[2] = (spln_frame->voltage_phases & 0x02) ? 'S' : '-';
            v_str[3] = (spln_frame->voltage_phases & 0x04) ? 'T' : '-';
            v_str[4] = '\0';

            if (spln_frame->is_reverse_power) {
                strcpy(i_str, " REV");
            } else {
                i_str[0] = 'I';
                i_str[1] = (spln_frame->current_phases & 0x01) ? 'R' : '-';
                i_str[2] = (spln_frame->current_phases & 0x02) ? 'S' : '-';
                i_str[3] = (spln_frame->current_phases & 0x04) ? 'T' : '-';
                i_str[4] = '\0';
            }
            break;
    }

    if (v_str[0] != '\0') {
        ssd1306_SetCursor(56, 0);
        ssd1306_WriteString(v_str, Font_6x8, White);
    }

    if (i_str[0] != '\0') {
        ssd1306_SetCursor(81, 0);
        ssd1306_WriteString(i_str, Font_6x8, White);
    }

    /* Kode Register SPLN Lampiran E (UR, US, UT, dll) bergantian dengan OBIS (32.07, dll) di X = 106..127 */
    const char *code_str = spln_frame->obis_code;
    if (spln_frame->reg_info[0] != '\0') {
        code_str = spln_frame->show_reg_alt ? spln_frame->reg_info : spln_frame->obis_code;
    }
    int code_len = (int)strlen(code_str);
    int x_code = 127 - (code_len * 6);
    int min_x_code = (i_str[0] != '\0') ? 106 : 56;
    if (x_code < min_x_code) x_code = min_x_code;
    ssd1306_SetCursor((uint8_t)x_code, 0);
    ssd1306_WriteString((char *)code_str, Font_6x8, White);

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
        int x_val = 46 + (50 - (val_len * 11)) / 2;
        if (x_val < 46) x_val = 46;
        ssd1306_SetCursor((uint8_t)x_val, 13);
        ssd1306_WriteString((char *)spln_frame->main_value, Font_11x18, White);
    } else {
        /* Teks khusus / IDPEL panjang / SABOTASE (Font_7x10 di Y: 17) */
        int val_len = (int)strlen(spln_frame->main_value);
        int text_width = val_len * 7;
        int zz_len = (int)strlen(spln_frame->scroll_index_zz);
        int zz_end_x = (zz_len > 0) ? (1 + zz_len * 6) : 0;
        int unit_len = (int)strlen(spln_frame->unit);
        int unit_start_x = (unit_len > 0) ? (127 - unit_len * 6) : 127;
        int available_width = unit_start_x - zz_end_x;

        int x_val;
        if (text_width <= (available_width - 4)) {
            /* Posisikan di tengah area antara label zz dan unit agar tidak tertimpa */
            x_val = zz_end_x + 3 + (available_width - 4 - text_width) / 2;
        } else {
            /* Jika teks sangat panjang, geser ke kanan label zz dengan batas kanan */
            x_val = zz_end_x + 3;
            if (x_val + text_width > 127) {
                x_val = 127 - text_width;
            }
        }
        if (x_val < zz_end_x + 2) {
            x_val = zz_end_x + 2;
        }

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

