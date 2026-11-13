/**
 * @file bsp_display_ips.c
 * @brief Implementasi Driver Konkret Layar IPS ST7789 Standar SPLN Gambar 4
 */

#include "bsp_display_ips.h"
#include "st7789.h"
#include "fonts.h"
#include <string.h>
#include <stdio.h>

static void bsp_ips_init(void) {
    ST7789_Init();
#ifdef ST7789_ROTATION
    ST7789_SetRotation(ST7789_ROTATION);
#endif
    ST7789_Fill_Color(BLACK);
}

static void bsp_ips_show_boot_screen(const char *title, const char *subtitle) {
    ST7789_Init();
#ifdef ST7789_ROTATION
    ST7789_SetRotation(ST7789_ROTATION);
#endif
    ST7789_Fill_Color(BLACK);

    /* Bingkai luar layar */
    ST7789_DrawRectangle(2, 2, ST7789_WIDTH - 3, ST7789_HEIGHT - 3, CYAN);

    const char *t = title ? title : "ALLIN SMART METER";
    const char *s = subtitle ? subtitle : "SPLN D3.006:2021";

    /* Judul Baris 1: Font 11x18 Rata Tengah */
    int t_len = (int)strlen(t);
    int t_x = (ST7789_WIDTH - (t_len * 11)) / 2;
    if (t_x < 10) t_x = 10;
    ST7789_WriteString((uint16_t)t_x, 35, t, Font_11x18, WHITE, BLACK);

    /* Subjudul Baris 2: Font 7x10 Rata Tengah */
    int s_len = (int)strlen(s);
    int s_x = (ST7789_WIDTH - (s_len * 7)) / 2;
    if (s_x < 10) s_x = 10;
    ST7789_WriteString((uint16_t)s_x, 75, s, Font_7x10, YELLOW, BLACK);
}

/**
 * @brief Gambar ikon status fisik relai kontaktor (Ukuran besar: 18x12 piksel)
 *        Sesuai SPLN D3.006:2021 Butir 6.11 Gambar 6 (Simbol Sakelar Terbuka/Tertutup)
 * - Tertutup/Normal: [--o===o--] (Terminal lingkaran tersambung kontak horizontal - Hijau)
 * - Trip/Buka:       [--o / o--] (Bilah sakelar terangkat miring ke atas - Merah)
 * - READY ON:        Bilah sakelar berkedip 1 Hz - Kuning
 */
static void ips_draw_relay_icon(uint16_t x, uint16_t y, bool closed, bool ready_on, bool blink) {
    uint16_t color = closed ? GREEN : (ready_on ? YELLOW : RED);

    /* Kawat terminal kiri (tebal 2px) */
    ST7789_DrawLine(x, y + 6, x + 2, y + 6, color);
    ST7789_DrawLine(x, y + 7, x + 2, y + 7, color);

    /* Terminal lingkaran kiri berlubang (Radius 2 di center x+4, y+6) */
    ST7789_DrawCircle(x + 4, y + 6, 2, color);

    /* Terminal lingkaran kanan berlubang (Radius 2 di center x+14, y+6) */
    ST7789_DrawCircle(x + 14, y + 6, 2, color);

    /* Kawat terminal kanan (tebal 2px) */
    ST7789_DrawLine(x + 16, y + 6, x + 18, y + 6, color);
    ST7789_DrawLine(x + 16, y + 7, x + 18, y + 7, color);

    if (closed && !ready_on) {
        /* Kontak tertutup menyambungkan kedua terminal (tebal 2px) */
        ST7789_DrawLine(x + 6, y + 6, x + 12, y + 6, color);
        ST7789_DrawLine(x + 6, y + 7, x + 12, y + 7, color);
    } else if (ready_on) {
        /* READY ON: bilah sakelar berkedip miring ke atas */
        if (blink) {
            ST7789_DrawLine(x + 13, y + 4, x + 7, y + 1, color);
            ST7789_DrawLine(x + 13, y + 5, x + 7, y + 2, color);
        }
    } else {
        /* Kontak terbuka / trip: bilah sakelar terangkat miring ke atas */
        ST7789_DrawLine(x + 13, y + 4, x + 7, y + 1, color);
        ST7789_DrawLine(x + 13, y + 5, x + 7, y + 2, color);
    }
}

/**
 * @brief Gambar ikon modem seluler / komunikasi (Ukuran besar: 13x13 piksel)
 *        Sesuai SPLN D3.006:2021 Butir 6.11 Gambar 6 (Gagang Telepon dalam Lingkaran)
 * - Aktif: Siluet gagang telepon menyala di dalam lingkaran (Cyan/Putih)
 * - Idle: Lingkaran luar standby (Gray)
 */
static void ips_draw_modem_icon(uint16_t x, uint16_t y, bool active, bool blink) {
    uint16_t ring_color = active ? CYAN : GRAY;

    /* 1. Lingkaran luar diameter 13 piksel (radius 6 di center x+6, y+6) tebal 2px */
    ST7789_DrawCircle(x + 6, y + 6, 6, ring_color);
    ST7789_DrawCircle(x + 6, y + 6, 5, ring_color);

    /* 2. Siluet gagang telepon klasik di dalam lingkaran */
    if (active && blink) {
        /* Earpiece atas */
        ST7789_Fill(x + 4, y + 3, x + 6, y + 4, WHITE);
        /* Gagang tebal */
        ST7789_DrawLine(x + 6, y + 4, x + 6, y + 8, WHITE);
        ST7789_DrawLine(x + 7, y + 4, x + 7, y + 8, WHITE);
        /* Mouthpiece bawah */
        ST7789_Fill(x + 4, y + 8, x + 6, y + 9, WHITE);
        /* Gelombang sinyal aktif */
        ST7789_DrawPixel(x + 9, y + 5, CYAN);
        ST7789_DrawPixel(x + 10, y + 6, CYAN);
        ST7789_DrawPixel(x + 9, y + 7, CYAN);
    } else {
        /* Standby dot tengah */
        ST7789_DrawPixel(x + 6, y + 6, GRAY);
    }
}

/**
 * @brief Gambar ikon indikator alarm/sabotase (Ukuran besar: 13x13 piksel)
 * - Alarm aktif: Segitiga peringatan tanda seru tebal [▲!] (Merah/Putih)
 * - Normal: Tanda centang halus tebal [✓] (Hijau)
 */
static void ips_draw_tamper_alarm_icon(uint16_t x, uint16_t y, bool alarm_active) {
    if (alarm_active) {
        /* Segitiga peringatan MERAH tebal (13x13 piksel) */
        ST7789_DrawTriangle(x + 6, y, x, y + 12, x + 12, y + 12, RED);
        ST7789_DrawTriangle(x + 6, y + 1, x + 1, y + 12, x + 11, y + 12, RED);
        /* Tanda seru PUTIH tebal di dalam segitiga */
        ST7789_Fill(x + 5, y + 4, x + 6, y + 8, WHITE);
        ST7789_Fill(x + 5, y + 10, x + 6, y + 11, WHITE);
    } else {
        /* Tanda centang OK HIJAU tebal */
        ST7789_DrawLine(x + 2, y + 7, x + 5, y + 11, GREEN);
        ST7789_DrawLine(x + 2, y + 8, x + 5, y + 12, GREEN);
        ST7789_DrawLine(x + 5, y + 11, x + 11, y + 2, GREEN);
        ST7789_DrawLine(x + 5, y + 12, x + 11, y + 3, GREEN);
    }
}

/**
 * @brief Gambar indikator baterai grafis (Ukuran besar: 16x12 piksel)
 * - Bodi baterai 13x11 dengan kutub positif
 * - Normal: 3 garis level isi (Hijau)
 * - Lemah (ERR03): berkedip 1 balok merah tebal (Merah)
 */
static void ips_draw_battery_gauge(uint16_t x, uint16_t y, bool low_battery, bool blink) {
    uint16_t color = low_battery ? RED : GREEN;

    /* Bodi luar baterai (tebal 2px) */
    ST7789_DrawRectangle(x, y + 1, x + 13, y + 11, color);
    ST7789_DrawRectangle(x + 1, y + 2, x + 12, y + 10, color);

    /* Kutub positif (tonjolan kanan) */
    ST7789_Fill(x + 14, y + 4, x + 15, y + 8, color);

    if (low_battery) {
        if (blink) {
            /* 1 balok merah tebal berkedip */
            ST7789_Fill(x + 3, y + 3, x + 6, y + 9, RED);
        }
    } else {
        /* 3 balok hijau solid */
        ST7789_Fill(x + 3, y + 4, x + 4, y + 8, GREEN);
        ST7789_Fill(x + 6, y + 4, x + 7, y + 8, GREEN);
        ST7789_Fill(x + 9, y + 4, x + 10, y + 8, GREEN);
    }
}

/**
 * @brief Gambar ikon panah arah aliran energi aktif (Ukuran besar: 14x12 piksel)
 * - Normal (Impor PLN / Beban): Panah solid segitiga mengarah ke kanan [►] (Hijau)
 * - Terbalik (Ekspor PLTS): Panah solid segitiga mengarah ke kiri [◄] (Kuning berkedip saat reverse)
 */
static void ips_draw_power_direction_arrow(uint16_t x, uint16_t y, bool is_reverse, bool blink) {
    if (is_reverse) {
        /* Aliran Terbalik / Ekspor PLTS: Panah Segitiga Padat Menghadap Kiri [◄] (Kuning) */
        if (blink) {
            ST7789_DrawFilledTriangle(x, y + 6, x + 8, y + 1, x + 8, y + 11, YELLOW);
            ST7789_Fill(x + 8, y + 4, x + 13, y + 8, YELLOW);
        }
    } else {
        /* Aliran Normal / Impor PLN: Panah Segitiga Padat Menghadap Kanan [►] (Hijau) */
        ST7789_DrawFilledTriangle(x + 13, y + 6, x + 5, y + 1, x + 5, y + 11, GREEN);
        ST7789_Fill(x, y + 4, x + 5, y + 8, GREEN);
    }
}

/**
 * @brief Gambar ikon indikator aliran daya 4-kuadran (Ukuran besar: 13x13 piksel)
 *        Berupa lingkaran 13x13 piksel dengan sumbu silang (+).
 *        Sektor kuadran aktif diarsir padat:
 *        - Kuadran 1 (Q1: +P, +Q): Kanan Atas (Beban Induktif)
 *        - Kuadran 2 (Q2: -P, +Q): Kiri Atas (Ekspor PLTS Lagging)
 *        - Kuadran 3 (Q3: -P, -Q): Kiri Bawah (Ekspor PLTS Leading)
 *        - Kuadran 4 (Q4: +P, -Q): Kanan Bawah (Beban Kapasitif)
 */
static void ips_draw_quadrant_icon(uint16_t x, uint16_t y, uint8_t quadrant) {
    uint16_t circle_color = GRAY;
    uint16_t active_color = CYAN;

    /* 1. Gambar lingkaran luar diameter 13 piksel (radius 6 di center x+6, y+6) */
    ST7789_DrawCircle(x + 6, y + 6, 6, circle_color);

    /* 2. Sumbu silang kuadran */
    ST7789_DrawLine(x, y + 6, x + 12, y + 6, circle_color); /* Sumbu horizontal (P) */
    ST7789_DrawLine(x + 6, y, x + 6, y + 12, circle_color); /* Sumbu vertikal (Q) */

    /* 3. Isi solid sektor kuadran yang sedang aktif */
    if (quadrant == 1) {
        /* Kuadran 1: Kanan Atas (+P, +Q) */
        ST7789_Fill(x + 7, y + 1, x + 11, y + 5, active_color);
    } else if (quadrant == 2) {
        /* Kuadran 2: Kiri Atas (-P, +Q) */
        ST7789_Fill(x + 1, y + 1, x + 5, y + 5, active_color);
    } else if (quadrant == 3) {
        /* Kuadran 3: Kiri Bawah (-P, -Q) */
        ST7789_Fill(x + 1, y + 7, x + 5, y + 11, active_color);
    } else if (quadrant == 4) {
        /* Kuadran 4: Kanan Bawah (+P, -Q) */
        ST7789_Fill(x + 7, y + 7, x + 11, y + 11, active_color);
    }
}

static void bsp_ips_render_frame(const display_spln_frame_t *spln_frame) {
    if (spln_frame == NULL) return;

    /* Bersihkan seluruh area layar dengan hitam */
    ST7789_Fill_Color(BLACK);

    /* BARIS 1 STATUS BAR: Ikon-Ikon Diperbesar (2x lebih besar, tinggi 12-14px)
     * 1. Ikon Relai Kontaktor (X=4..22)
     * 2. Ikon Modem Komunikasi (X=26..39)
     * 3. Ikon Alarm / Tamper (X=44..57)
     * 4. Ikon Baterai Grafis (X=62..78)
     * 5. Ikon Panah Arah Energi Aktif (X=83..97)
     * 6. Ikon Kuadran Aliran Daya (X=102..115)
     * 7. Badge Tarif ToU T1/T2/T3 (X=120..138)
     * 8. Indikator Fasa Tegangan & Arus Rata Kanan (X=150..236)
     */
    ips_draw_relay_icon(4, 2, spln_frame->is_relay_closed, spln_frame->is_relay_ready_on, spln_frame->is_battery_blink_on);
    ips_draw_modem_icon(26, 2, spln_frame->is_modem_active, spln_frame->is_battery_blink_on);
    ips_draw_tamper_alarm_icon(44, 2, spln_frame->is_alarm_active || spln_frame->is_tamper_active);
    ips_draw_battery_gauge(62, 2, spln_frame->is_battery_low, spln_frame->is_battery_blink_on);
    ips_draw_power_direction_arrow(83, 2, spln_frame->is_reverse_power, spln_frame->is_battery_blink_on);
    ips_draw_quadrant_icon(102, 2, spln_frame->power_quadrant);

    /* Badge Tarif ToU (T1, T2, T3) dengan kotak berbingkai */
    if (spln_frame->active_tariff_zone < 3) {
        static const char *tariff_labels[] = {"T1", "T2", "T3"};
        ST7789_DrawRectangle(120, 2, 138, 14, YELLOW);
        ST7789_WriteString(123, 3, tariff_labels[spln_frame->active_tariff_zone], Font_7x10, YELLOW, BLACK);
    }

    /* BARIS 1 STATUS BAR: Indikator Fasa & Kode OBIS Rata Kanan */
    char top_ind[20] = "";

    switch (spln_frame->page_id) {
        /* 1. Pengukuran Terkait Fasa R: Menampilkan indikator fasa R */
        case DISP_PAGE_VOLTAGE_R:
        case DISP_PAGE_CURRENT_R:
        case DISP_PAGE_ANGLE_UR:
        case DISP_PAGE_ANGLE_IR:
            if ((spln_frame->voltage_phases & 0x01) && (spln_frame->current_phases & 0x01)) {
                strcpy(top_ind, "UR IR");
            } else if (spln_frame->voltage_phases & 0x01) {
                strcpy(top_ind, "UR");
            } else if (spln_frame->current_phases & 0x01) {
                strcpy(top_ind, "IR");
            }
            break;

        /* 2. Pengukuran Terkait Fasa S: Menampilkan indikator fasa S */
        case DISP_PAGE_VOLTAGE_S:
        case DISP_PAGE_CURRENT_S:
        case DISP_PAGE_ANGLE_US:
        case DISP_PAGE_ANGLE_IS:
            if ((spln_frame->voltage_phases & 0x02) && (spln_frame->current_phases & 0x02)) {
                strcpy(top_ind, "US IS");
            } else if (spln_frame->voltage_phases & 0x02) {
                strcpy(top_ind, "US");
            } else if (spln_frame->current_phases & 0x02) {
                strcpy(top_ind, "IS");
            }
            break;

        /* 3. Pengukuran Terkait Fasa T: Menampilkan indikator fasa T */
        case DISP_PAGE_VOLTAGE_T:
        case DISP_PAGE_CURRENT_T:
        case DISP_PAGE_ANGLE_UT:
        case DISP_PAGE_ANGLE_IT:
            if ((spln_frame->voltage_phases & 0x04) && (spln_frame->current_phases & 0x04)) {
                strcpy(top_ind, "UT IT");
            } else if (spln_frame->voltage_phases & 0x04) {
                strcpy(top_ind, "UT");
            } else if (spln_frame->current_phases & 0x04) {
                strcpy(top_ind, "IT");
            }
            break;

        /* 4. Arus Netral */
        case DISP_PAGE_CURRENT_N:
            strcpy(top_ind, "IN");
            break;

        /* 5. Halaman Non-Metrologi / Informasi Umum: Tidak menampilkan teks di baris atas (kode dipindah ke bawah) */
        case DISP_PAGE_IDPEL:
        case DISP_PAGE_METER_ID:
        case DISP_PAGE_TIME:
        case DISP_PAGE_CHECKSUM:
        case DISP_PAGE_PLN_UNIT:
        case DISP_PAGE_TAMPER_ALARM:
        case DISP_PAGE_RELAY_READY_ON:
            top_ind[0] = '\0';
            break;

        /* 6. Halaman Metrologi Total 3-Fasa (Energi Aktif, Reaktif, Daya Total, PF, Freq, dll) */
        default: {
            char v_part[5];
            v_part[0] = 'U';
            v_part[1] = (spln_frame->voltage_phases & 0x01) ? 'R' : '-';
            v_part[2] = (spln_frame->voltage_phases & 0x02) ? 'S' : '-';
            v_part[3] = (spln_frame->voltage_phases & 0x04) ? 'T' : '-';
            v_part[4] = '\0';

            char i_part[5];
            if (spln_frame->is_reverse_power) {
                strcpy(i_part, "REV");
            } else {
                i_part[0] = 'I';
                i_part[1] = (spln_frame->current_phases & 0x01) ? 'R' : '-';
                i_part[2] = (spln_frame->current_phases & 0x02) ? 'S' : '-';
                i_part[3] = (spln_frame->current_phases & 0x04) ? 'T' : '-';
                i_part[4] = '\0';
            }

            snprintf(top_ind, sizeof(top_ind), "%s %s", v_part, i_part);
            break;
        }
    }

    /* Gambar Indikator Baris 1: Rata Kanan */
    if (top_ind[0] != '\0') {
        int text_len = (int)strlen(top_ind);
        int text_width = text_len * 7;
        int x_pos = (ST7789_WIDTH - 6) - text_width;
        if (x_pos < 100) x_pos = 100;
        ST7789_WriteString((uint16_t)x_pos, 2, top_ind, Font_7x10, WHITE, BLACK);
    }

    /* Garis Pemisah Horizontal Status Bar (Y = 16) */
    if (spln_frame->is_alarm_active || spln_frame->is_tamper_active) {
        ST7789_DrawLine(2, 16, ST7789_WIDTH - 3, 16, RED);
    } else {
        for (uint16_t x = 2; x < ST7789_WIDTH - 2; x += 3) {
            ST7789_DrawPixel(x, 16, GRAY);
        }
    }

    /* BARIS UTAMA (Y: 35..90):
     * Nilai Angka Utama (Font 16x26 / Font 11x18) dan Satuan Metrologi (Font 11x18)
     * Ditata rapi di tengah tanpa terhalang kode halaman.
     */
    int val_len = (int)strlen(spln_frame->main_value);
    int unit_len = (int)strlen(spln_frame->unit);
    uint16_t val_color = (spln_frame->is_alarm_active || spln_frame->is_tamper_active) ? RED : WHITE;

    if (spln_frame->is_large_font && (val_len * 16 <= 180)) {
        /* Mode Angka Besar (Font 16x26) */
        if (unit_len > 0) {
            /* Nilai + Satuan berdampingan terpusat di tengah */
            int total_w = (val_len * 16) + 8 + (unit_len * 11);
            int x_start = (ST7789_WIDTH - total_w) / 2;
            if (x_start < 8) x_start = 8;

            /* Nilai Utama */
            ST7789_WriteString((uint16_t)x_start, 48, spln_frame->main_value, Font_16x26, val_color, BLACK);

            /* Satuan di samping kanan nilai */
            int x_unit = x_start + (val_len * 16) + 8;
            ST7789_WriteString((uint16_t)x_unit, 54, spln_frame->unit, Font_11x18, CYAN, BLACK);
        } else {
            int x_val = (ST7789_WIDTH - (val_len * 16)) / 2;
            if (x_val < 8) x_val = 8;
            ST7789_WriteString((uint16_t)x_val, 48, spln_frame->main_value, Font_16x26, val_color, BLACK);
        }
    } else {
        /* Mode Teks Sedang / Panjang (Font 11x18), misal ID Pelanggan atau Kode Error */
        if (unit_len > 0 && ((val_len + unit_len) * 11 + 8 <= (ST7789_WIDTH - 16))) {
            int total_w = ((val_len + unit_len) * 11) + 8;
            int x_start = (ST7789_WIDTH - total_w) / 2;
            if (x_start < 8) x_start = 8;

            ST7789_WriteString((uint16_t)x_start, 52, spln_frame->main_value, Font_11x18, val_color, BLACK);
            int x_unit = x_start + (val_len * 11) + 8;
            ST7789_WriteString((uint16_t)x_unit, 52, spln_frame->unit, Font_11x18, CYAN, BLACK);
        } else {
            int x_val = (ST7789_WIDTH - (val_len * 11)) / 2;
            if (x_val < 8) x_val = 8;
            ST7789_WriteString((uint16_t)x_val, 52, spln_frame->main_value, Font_11x18, val_color, BLACK);

            if (unit_len > 0) {
                int x_unit = (ST7789_WIDTH - 8) - (unit_len * 11);
                ST7789_WriteString((uint16_t)x_unit, 52, spln_frame->unit, Font_11x18, CYAN, BLACK);
            }
        }
    }

    /* FOOTER BARIS BAWAH (Y: 98..125):
     * Garis pemisah bawah + Kode Halaman (scroll_index_zz) di KIRI + Kode OBIS/Register di KANAN
     */
    ST7789_DrawLine(2, 98, ST7789_WIDTH - 3, 98, GRAY);

    /* 1. KODE DARI SETIAP PAGE: Dipindah ke baris bawah sebelah kiri (Kuning, Font 11x18) */
    if (spln_frame->scroll_index_zz[0] != '\0') {
        char code_str[24];
        snprintf(code_str, sizeof(code_str), "[%s]", spln_frame->scroll_index_zz);
        ST7789_WriteString(6, 104, code_str, Font_11x18, YELLOW, BLACK);
    }

    /* 2. KODE OBIS & REGISTER: Di baris bawah sebelah kanan (Font 7x10) */
    if (spln_frame->is_tamper_active) {
        ST7789_WriteString((uint16_t)(ST7789_WIDTH - 8 - (19 * 7)), 108, "! PERIKSA SABOTASE !", Font_7x10, RED, BLACK);
    } else {
        char right_info[32] = "";
        if (spln_frame->obis_code[0] != '\0' && spln_frame->reg_info[0] != '\0') {
            snprintf(right_info, sizeof(right_info), "%s %s", spln_frame->obis_code, spln_frame->reg_info);
        } else if (spln_frame->obis_code[0] != '\0') {
            snprintf(right_info, sizeof(right_info), "OBIS: %s", spln_frame->obis_code);
        } else if (spln_frame->reg_info[0] != '\0') {
            snprintf(right_info, sizeof(right_info), "%s", spln_frame->reg_info);
        }

        if (right_info[0] != '\0') {
            int r_len = (int)strlen(right_info);
            int x_r = (ST7789_WIDTH - 8) - (r_len * 7);
            if (x_r < 90) x_r = 90;
            ST7789_WriteString((uint16_t)x_r, 108, right_info, Font_7x10, LGRAY, BLACK);
        }
    }
}

void bsp_display_ips_set_backlight(uint8_t on) {
    ST7789_SetBacklight(on);
}

uint8_t bsp_display_ips_get_backlight(void) {
    return ST7789_GetBacklight();
}

void bsp_display_ips_toggle_backlight(void) {
    ST7789_SetBacklight(!ST7789_GetBacklight());
}

static void bsp_ips_set_power_mode(bool sleep_mode) {
    if (sleep_mode) {
        /* Matikan backlight & masuk mode ultra-low power sleep (DISPOFF + SLPIN) */
        ST7789_SetDisplayOn(0);
    } else {
        /* Bangun dari mode sleep (SLPOUT + DISPON) & nyalakan backlight */
        ST7789_SetDisplayOn(1);
    }
}

static const display_driver_interface_t s_bsp_ips_driver = {
    .init              = bsp_ips_init,
    .render_frame      = bsp_ips_render_frame,
    .show_boot_screen  = bsp_ips_show_boot_screen,
    .set_power_mode    = bsp_ips_set_power_mode
};

const display_driver_interface_t* bsp_display_ips_get_driver(void) {
    return &s_bsp_ips_driver;
}
