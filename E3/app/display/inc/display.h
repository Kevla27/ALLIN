/**
 * @file display.h
 * @brief Modul Tampilan LCD & Navigasi Carousel Smart Meter 3-Fasa (E3/ENG-3)
 *        Memenuhi Standar Lengkap SPLN D3.006: 2021 Lampiran E (Tabel E.1 & Tabel E.2)
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "tamper_manager.h"

typedef enum {
    DISPLAY_MODE_AUTO_CAROUSEL = 0,
    DISPLAY_MODE_MANUAL_SCROLL
} display_mode_t;

typedef enum {
    BUTTON_DIR_UP = 0,
    BUTTON_DIR_DOWN,
    BUTTON_DIR_BOTH               /* Tekan tombol atas & bawah secara bersamaan (SPLN Tabel 5 No 12) */
} button_dir_t;

typedef enum {
    /* ========================================================================= */
    /* Urutan Halaman Scrolling Manual (Sesuai Tombol Button Scroll)             */
    /* ========================================================================= */
    DISP_PAGE_IDPEL = 0,           /* 01: ID Pelanggan */
    DISP_PAGE_METER_ID,            /* 02: Meter ID */
    DISP_PAGE_ACTIVE_POWER,        /* 03: Daya aktif kirim */
    DISP_PAGE_ACTIVE_POWER_IMP,    /* 04: Daya aktif terima */
    DISP_PAGE_REACTIVE_POWER,      /* 05: Daya reaktif kirim */
    DISP_PAGE_REACTIVE_POWER_IMP,  /* 06: Daya reaktif terima */
    DISP_PAGE_ENERGY_WBP,          /* 07: WBP (Kirim) */
    DISP_PAGE_ENERGY_LWBP,         /* 08: LWBP1 (Kirim) */
    DISP_PAGE_ENERGY_LWBP2,        /* 09: LWBP2 (Kirim) */
    DISP_PAGE_ACTIVE_ENERGY,       /* 10: Energi aktif kirim */
    DISP_PAGE_ENERGY_IMP_WBP,      /* 11: WBP (Terima) */
    DISP_PAGE_ENERGY_IMP_LWBP1,    /* 12: LWBP1 (Terima) */
    DISP_PAGE_ENERGY_IMP_LWBP2,    /* 13: LWBP2 (Terima) */
    DISP_PAGE_ENERGY_IMPORT,       /* 14: Energi aktif terima */
    DISP_PAGE_REACTIVE_ENERGY_EXP, /* 15: Energi reaktif kirim */
    DISP_PAGE_REACTIVE_ENERGY_IMP, /* 16: Energi reaktif terima */
    DISP_PAGE_TOTAL_ENERGY,        /* 17: Energi aktif total */
    DISP_PAGE_REACTIVE_ENERGY,     /* 18: Energi reaktif billing */
    DISP_PAGE_VOLTAGE_R,           /* 19: Tegangan fase R */
    DISP_PAGE_VOLTAGE_S,           /* 20: Tegangan fase S */
    DISP_PAGE_VOLTAGE_T,           /* 21: Tegangan fase T */
    DISP_PAGE_ANGLE_UR,            /* 22: Sudut tegangan fase R */
    DISP_PAGE_ANGLE_US,            /* 23: Sudut tegangan fase S */
    DISP_PAGE_ANGLE_UT,            /* 24: Sudut tegangan fase T */
    DISP_PAGE_CURRENT_R,           /* 25: Arus tegangan/fase R */
    DISP_PAGE_CURRENT_S,           /* 26: Arus tegangan/fase S */
    DISP_PAGE_CURRENT_T,           /* 27: Arus tegangan/fase T */
    DISP_PAGE_CURRENT_N,           /* 28: Arus netral */
    DISP_PAGE_ANGLE_IR,            /* 29: Sudut fase R */
    DISP_PAGE_ANGLE_IS,            /* 30: Sudut fase S */
    DISP_PAGE_ANGLE_IT,            /* 31: Sudut fase T */
    DISP_PAGE_POWER_FACTOR,        /* 32: Faktor daya */
    DISP_PAGE_TIME,                /* 33: Waktu */
    DISP_PAGE_POWER_LIMITER,       /* 34: Power limiter setting */
    DISP_PAGE_MAX_DEMAND_S,        /* 35: Maksimum demand daya semu kirim */
    DISP_PAGE_MD_TIME_S,           /* 36: Waktu saat maksimum demand daya semu kirim */
    DISP_PAGE_EOB,                 /* 37: EoB */
    DISP_PAGE_CHECKSUM,            /* 38: Checksum software */
    DISP_PAGE_PLN_UNIT,            /* 39: Unit PLN */

    /* ========================================================================= */
    /* Halaman Sistem / Notifikasi Paksaan (WAJIB DI PALING BAWAH)               */
    /* ========================================================================= */
    DISP_PAGE_TAMPER_ALARM,        /* Status Sabotase (Harus di sini agar siklus manual bekerja) */
    DISP_PAGE_RELAY_READY_ON,      /* Status Relai Reconnect */
    DISP_PAGE_COUNT                /* Batas akhir perhitungan jumlah halaman */
} display_page_id_t;

#define DISP_AUTO_PAGE_COUNT  15U
extern const display_page_id_t g_disp_auto_pages[DISP_AUTO_PAGE_COUNT];
#define DISP_AUTO_PAGE_FIRST  g_disp_auto_pages[0]
#define DISP_AUTO_PAGE_LAST   g_disp_auto_pages[DISP_AUTO_PAGE_COUNT - 1]

typedef struct {
    uint32_t voltage_r_dvolts;
    uint32_t voltage_s_dvolts;
    uint32_t voltage_t_dvolts;
    
    uint32_t current_r_mamps;
    uint32_t current_s_mamps;
    uint32_t current_t_mamps;
    uint32_t current_n_mamps;
    
    int32_t  active_power_w;
    int32_t  reactive_power_var;
    uint32_t apparent_power_va;
    
    uint16_t power_factor_ppm;
    uint16_t frequency_mhz;
    
    uint64_t active_energy_wh;
    uint64_t active_energy_wbp_wh;
    uint64_t active_energy_lwbp_wh;
    uint64_t active_energy_lwbp2_wh;    /* Tarif 3 LWBP2 (Wh) - Tabel E.1 No 4 (EA_EXP3) */
    uint64_t active_energy_import_wh;   /* Energi Terima / Impor PLTS (Wh) - Tabel E.1 No 6 (EA_IMP) */
    uint64_t reactive_energy_varh;

    /* Maximum Demand (MD 15-Menit) (SPLN Tabel E.1 No 4-7) */
    uint32_t max_demand_w;
    uint32_t max_demand_timestamp;
    uint32_t cumulative_max_demand_w;
    uint32_t max_demand_reset_count;

    /* Stand Penagihan Historis Bulan Lalu (SPLN Tabel E.2 No 17) */
    uint64_t billing_last_month_wh;

    /* Daya Aliran Balik / Impor PLTS (SPLN Tabel E.2 No 4 & 6) */
    int32_t  active_power_imp_w;
    int32_t  reactive_power_imp_var;

    /* Energi Impor per Tarif & Reaktif Murni (SPLN Tabel E.2 No 11-16) */
    uint64_t active_energy_imp_wbp_wh;
    uint64_t active_energy_imp_lwbp1_wh;
    uint64_t active_energy_imp_lwbp2_wh;
    uint64_t reactive_energy_exp_varh;
    uint64_t reactive_energy_imp_varh;

    /* Sudut Fasa Tegangan & Arus (SPLN Tabel E.2 No 22-24, 29-31) (0..360 Derajat) */
    uint16_t angle_ur_deg;
    uint16_t angle_us_deg;
    uint16_t angle_ut_deg;
    uint16_t angle_ir_deg;
    uint16_t angle_is_deg;
    uint16_t angle_it_deg;

    /* Maksimum Demand Daya Semu (kVA) (SPLN Tabel E.2 No 35-36) */
    uint32_t max_demand_s_va;
    uint32_t max_demand_s_timestamp;

    /* Konfigurasi Billing & Utilitas (SPLN Tabel E.2 No 37 & 39) */
    uint8_t  eob_day;
    uint8_t  eob_hour;
    char     pln_unit_code[16];
    
    /* Parameter Konfigurasi & Diagnostik Sistem (SPLN Tabel E.2) */
    uint32_t power_limit_w;
    uint32_t firmware_checksum;
} meter_measurements_t;

typedef struct {
    display_mode_t   mode;
    display_page_id_t current_page;
    uint32_t         carousel_interval_ms;
    uint32_t         timer_accumulator_ms;
    uint32_t         manual_timeout_ms;
    meter_measurements_t meas_buffer;
    char             customer_id[16];
    char            meter_id[16];
    bool             alarm_icon_active;
    uint32_t         active_tamper_mask;  /* Bitmask tamper_vector_t (SPLN D3.006 Tabel 6) */
    uint32_t         active_alarm_mask;   /* Bitmask spln_internal_alarm_t (SPLN D3.006 Tabel 4) */
    uint32_t         current_time_epoch;  /* Timestamp Epoch UNIX / waktu RTC sesaat */
    uint32_t         subsecond_accum_ms;  /* Akumulator ms untuk update current_time_epoch yang presisi */
    bool             relay_ready_on;      /* Flag state relai READY ON (SPLN Tabel 5 No 12) */
    bool             asymmetry_warning;   /* Flag peringatan asimetri beban (SPLN Tabel 5 No 11) */
    bool             is_modem_active;     /* Flag meter sedang berkomunikasi dengan modem (SPLN Gambar 6) */
} display_context_t;

bool display_init(display_context_t *ctx, const char *customer_id, const char *meter_id, uint32_t carousel_interval_ms);
void display_update_customer_id(display_context_t *ctx, const char *customer_id);
void display_update_meter_id(display_context_t *ctx, const char *meter_id);
void display_render(display_context_t *ctx);
void display_update_measurements(display_context_t *ctx, const meter_measurements_t *meas);
void display_process_tick(display_context_t *ctx, uint32_t elapsed_ms);
void display_handle_button_press(display_context_t *ctx, button_dir_t dir);
void display_render_frame(const display_context_t *ctx, char *out_line1, char *out_line2, char *out_line3, size_t max_len);
void display_set_time(display_context_t *ctx, uint32_t epoch_sec);
void display_set_relay_ready_on(display_context_t *ctx, bool ready_on);
void display_set_asymmetry_warning(display_context_t *ctx, bool warning);
void display_set_modem_status(display_context_t *ctx, bool active);

/**
 * @brief Helper pengaturan dan pembacaan teks / kode error resmi SPLN D3.006:2021
 */
void display_set_tamper_status(display_context_t *ctx, uint32_t tamper_mask);
void display_set_internal_alarm(display_context_t *ctx, uint32_t alarm_mask);
uint32_t display_get_internal_alarm(const display_context_t *ctx);
void display_get_spln_tamper_info(uint32_t tamper_mask, char *out_text, size_t text_len, char *out_code, size_t code_len);
void display_get_spln_alarm_info(uint32_t alarm_mask, char *out_text, size_t text_len, char *out_code, size_t code_len);
void display_get_combined_error_info(uint32_t tamper_mask, uint32_t alarm_mask, uint32_t epoch_sec, char *out_text, size_t text_sz, char *out_code, size_t code_sz);

/**
 * @brief Status julat batas operasi tegangan pasok (SPLN D3.006: 2021 Butir 10.3 Tabel 9)
 * - Normal: 0.5 Un s/d 1.2 Un (115.0 V s/d 276.0 V)
 * - Lo Volt: < 0.5 Un (< 115.0 V)
 * - Hi Volt: > 1.2 Un (> 276.0 V)
 */
typedef enum {
    DISPLAY_VOLT_NORMAL = 0,
    DISPLAY_VOLT_LOW,     /* < 0.5 Un (< 115.0 V) -> "Lo Volt" */
    DISPLAY_VOLT_HIGH     /* > 1.2 Un (> 276.0 V) -> "Hi Volt" */
} display_voltage_status_t;

display_voltage_status_t display_get_voltage_status(uint32_t v_dvolts);

/**
 * @brief Struktur data representasi frame tampilan fisik LCD SPLN D3.006:2021 Gambar 4
 */
typedef struct {
    display_page_id_t page_id;    /* ID Halaman SPLN yang sedang aktif */
    char header_symbols[32];      /* Baris 1: Simbol indikator dinamis */
    char scroll_index_zz[10];     /* zz: Indeks scrolling (01..38, AL, on, IDPEL, dll) */
    char main_value[32];          /* Baris 2: Nilai utama numerik / teks status */
    char unit[16];                /* Baris 3: Satuan metrologi (V, A, kW, kvarh, kWh) / kode ERR */
    char obis_code[16];           /* OBIS identifier sesuai SPLN */
    char reg_info[12];            /* Informasi Register SPLN Lampiran E (UR, US, UT, IR, dll) */
    char alarm_code[8];           /* Indikator "!ALM" atau "OK" */
    bool is_large_font;           /* true: Font 11x18 (nilai numerik), false: Font 7x10 */
    bool is_alarm_active;         /* Status icon alarm aktif */
    bool is_battery_low;          /* Simbol baterai lemah */
    bool is_battery_blink_on;     /* Siklus kedip simbol baterai (500 ms) */
    bool is_reverse_power;        /* Simbol arah arus balik */
    uint8_t voltage_phases;       /* Bitmask fasa aktif: bit 0: R, bit 1: S, bit 2: T */
    uint8_t current_phases;       /* Bitmask fasa arus aktif: bit 0: R, bit 1: S, bit 2: T */
    uint8_t power_quadrant;       /* Kuadran daya aktif (1..4) */
    bool is_relay_closed;         /* Status fisik relai kontaktor (true: Terhubung/Closed, false: Trip/Buka) */
    bool is_relay_ready_on;       /* Status relai READY ON (siap sambung kembali dengan tombol ▲+▼) */
    bool is_tamper_active;        /* Status sabotase terdeteksi (case open, terminal open, magnet, dll) */
    bool is_modem_active;         /* Simbol komunikasi modem aktif (SPLN D3.006:2021 Butir 6.11 Gambar 6) */
    uint8_t active_tariff_zone;   /* Zona tarif aktif (0: WBP, 1: LWBP1, 2: LWBP2) */
    bool show_reg_alt;            /* True: fase tampilan reg_info (UR, US, UT), False: fase obis_code (32.07) */
} display_spln_frame_t;

/**
 * @brief Merender data konteks tampilan ke frame layout standar SPLN Gambar 4
 */
void display_render_spln_frame(const display_context_t *ctx, display_spln_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */
