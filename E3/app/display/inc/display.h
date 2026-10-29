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
    /* 14 Halaman Layar Tampilan Otomatis (Auto Carousel) Sesuai Tabel E.1       */
    /* "Informasi layar tampilan otomatis" (No. 1 s.d 14 - Tanpa Kurang / Tambah)*/
    /* ========================================================================= */
    DISP_PAGE_IDPEL = 0,           /* 01: ID Pelanggan (IDPEL) */
    DISP_PAGE_ENERGY_WBP,          /* 02: Energi aktif kirim Tarif 1 WBP (EA_EXP1, kWh, 3 desimal) */
    DISP_PAGE_ENERGY_LWBP,         /* 03: Energi aktif kirim Tarif 2 LWBP1 (EA_EXP2, kWh, 3 desimal) */
    DISP_PAGE_ENERGY_LWBP2,        /* 04: Energi aktif kirim Tarif 3 LWBP2 (EA_EXP3, kWh, 3 desimal) */
    DISP_PAGE_ACTIVE_ENERGY,       /* 05: Energi aktif kirim (EA_EXP, kWh, 3 desimal) */
    DISP_PAGE_ENERGY_IMPORT,       /* 06: Energi aktif terima (EA_IMP, kWh, 3 desimal) */
    DISP_PAGE_REACTIVE_ENERGY,     /* 07: Energi reaktif billing kompensasi (ER_BIL, kVARh, 3 desimal) */
    DISP_PAGE_VOLTAGE_R,           /* 08: Tegangan fase R (UR, V, 1 desimal) */
    DISP_PAGE_VOLTAGE_S,           /* 09: Tegangan fase S (US, V, 1 desimal) */
    DISP_PAGE_VOLTAGE_T,           /* 10: Tegangan fase T (UT, V, 1 desimal) */
    DISP_PAGE_CURRENT_R,           /* 11: Arus fase R (IR, A, 3 desimal) */
    DISP_PAGE_CURRENT_S,           /* 12: Arus fase S (IS, A, 3 desimal) */
    DISP_PAGE_CURRENT_T,           /* 13: Arus fase T (IT, A, 3 desimal) */
    DISP_PAGE_CURRENT_N,           /* 14: Arus netral (IN, A, 3 desimal) */

    /* ========================================================================= */
    /* Parameter Tambahan Scrolling Manual (Tabel E.2 / Diagnostik / Rekayasa)   */
    /* ========================================================================= */
    DISP_PAGE_ACTIVE_POWER,        /* Daya Aktif Total (kW) */
    DISP_PAGE_REACTIVE_POWER,      /* Daya Reaktif Total (kvar) */
    DISP_PAGE_APPARENT_POWER,      /* Daya Semu Total (kVA) */
    DISP_PAGE_POWER_FACTOR,        /* Faktor Daya (PF) */
    DISP_PAGE_FREQUENCY,           /* Frekuensi Jala-Jala (Hz) */
    DISP_PAGE_MAX_DEMAND,          /* Kebutuhan Maksimum Aktif (P_MAX, kW) */
    DISP_PAGE_MD_TIME,             /* Waktu Kebutuhan Maksimum (TS_MAX) */
    DISP_PAGE_CMD,                 /* Kumulatif Max Demand (CMD, kW) */
    DISP_PAGE_MD_RESET_COUNT,      /* Counter Reset MD */
    DISP_PAGE_TIME,                /* Jam Waktu Nyata (TIME, WIB) */
    DISP_PAGE_DATE,                /* Tanggal Waktu Nyata (DATE, WIB) */
    DISP_PAGE_TEST_DISPLAY,        /* zz: 00, Uji Layar Segmen '88888888' - Tabel E.2 No 1 */
    DISP_PAGE_METER_ID,            /* zz: 02, OBIS: 96.02   (METERID) - Tabel E.2 No 2 */
    DISP_PAGE_ACTIVE_POWER_IMP,    /* zz: 27, OBIS: 02.07   (P_IMP Daya Aktif Terima) - Tabel E.2 No 4 */
    DISP_PAGE_REACTIVE_POWER_IMP,  /* zz: 28, OBIS: 04.07   (Q_IMP Daya Reaktif Terima) - Tabel E.2 No 6 */
    DISP_PAGE_ENERGY_IMP_WBP,      /* zz: 29, OBIS: 02.08.1 (EA_IMP1 Tarif 1 Terima) - Tabel E.2 No 11 */
    DISP_PAGE_ENERGY_IMP_LWBP1,    /* zz: 30, OBIS: 02.08.2 (EA_IMP2 Tarif 2 Terima) - Tabel E.2 No 12 */
    DISP_PAGE_ENERGY_IMP_LWBP2,    /* zz: 31, OBIS: 02.08.3 (EA_IMP3 Tarif 3 Terima) - Tabel E.2 No 13 */
    DISP_PAGE_REACTIVE_ENERGY_EXP, /* zz: 32, OBIS: 03.08.0 (ER_EXP Reaktif Kirim Murni) - Tabel E.2 No 15 */
    DISP_PAGE_REACTIVE_ENERGY_IMP, /* zz: 33, OBIS: 04.08.0 (ER_IMP Reaktif Terima Murni) - Tabel E.2 No 16 */
    DISP_PAGE_ANGLE_UR,            /* zz: 35, OBIS: 14.81.0 (Sdt_UR Sudut Tegangan R) - Tabel E.2 No 22 */
    DISP_PAGE_ANGLE_US,            /* zz: 36, OBIS: 14.81.1 (Sdt_US Sudut Tegangan S) - Tabel E.2 No 23 */
    DISP_PAGE_ANGLE_UT,            /* zz: 37, OBIS: 14.81.2 (Sdt_UT Sudut Tegangan T) - Tabel E.2 No 24 */
    DISP_PAGE_ANGLE_IR,            /* zz: 39, OBIS: 11.81.0 (Sdt_IR Sudut Arus R) - Tabel E.2 No 29 */
    DISP_PAGE_ANGLE_IS,            /* zz: 40, OBIS: 11.81.1 (Sdt_IS Sudut Arus S) - Tabel E.2 No 30 */
    DISP_PAGE_ANGLE_IT,            /* zz: 41, OBIS: 11.81.2 (Sdt_IT Sudut Arus T) - Tabel E.2 No 31 */
    DISP_PAGE_POWER_LIMITER,       /* zz: 34, OBIS: 00.01   (P_LIM kW) - Tabel E.2 No 34 */
    DISP_PAGE_MAX_DEMAND_S,        /* zz: 42, OBIS: 09.06.0 (S_MAX kVA MD Semu) - Tabel E.2 No 35 */
    DISP_PAGE_MD_TIME_S,           /* zz: 43, OBIS: 09.06.0*0 (TS_MAX Waktu MD Semu) - Tabel E.2 No 36 */
    DISP_PAGE_EOB,                 /* zz: 44, OBIS: 00.09.01(EoB End of Billing dd-hh) - Tabel E.2 No 37 */
    DISP_PAGE_CHECKSUM,            /* zz: 38, OBIS: 96.20   (CSUM Firmware) - Tabel E.2 No 38 */
    DISP_PAGE_PLN_UNIT,            /* zz: 45, OBIS: 96.01.0 (Unit PLN UP3) - Tabel E.2 No 39 */
    DISP_PAGE_HISTORICAL_BILLING,  /* zz: 24, OBIS: 01.08*01(Stand Billing Bulan Lalu) - Tabel E.2 No 17 */
    DISP_PAGE_TAMPER_ALARM,        /* zz: AL, OBIS: 96.50   (Status Sabotase) - Tabel E.2 No 39 */
    DISP_PAGE_RELAY_READY_ON,      /* zz: on, Status Relai Reconnect - Tabel E.2 No 40 */
    DISP_PAGE_COUNT
} display_page_id_t;

#define DISP_AUTO_PAGE_COUNT  14U
#define DISP_AUTO_PAGE_FIRST  DISP_PAGE_IDPEL
#define DISP_AUTO_PAGE_LAST   DISP_PAGE_CURRENT_N

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
    bool             alarm_icon_active;
    uint32_t         active_tamper_mask;  /* Bitmask tamper_vector_t (SPLN D3.006 Tabel 6) */
    uint32_t         active_alarm_mask;   /* Bitmask spln_internal_alarm_t (SPLN D3.006 Tabel 4) */
    uint32_t         current_time_epoch;  /* Timestamp Epoch UNIX / waktu RTC sesaat */
    bool             relay_ready_on;      /* Flag state relai READY ON (SPLN Tabel 5 No 12) */
    bool             asymmetry_warning;   /* Flag peringatan asimetri beban (SPLN Tabel 5 No 11) */
    bool             is_modem_active;     /* Flag meter sedang berkomunikasi dengan modem (SPLN Gambar 6) */
} display_context_t;

bool display_init(display_context_t *ctx, const char *customer_id, uint32_t carousel_interval_ms);
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
    char header_symbols[32];      /* Baris 1: Simbol indikator dinamis */
    char scroll_index_zz[4];      /* zz: Indeks scrolling (01..38, AL, on) */
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
