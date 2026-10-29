/**
 * @file test_spln_display_tamper.c
 * @brief Unit Test untuk Tampilan Standar Layar PLN (Gambar 4) dan Alarm Sabotase
 *        Sesuai Standar SPLN D3.006: 2021 + SUP-1: 2022 + SUP-2: 2023
 *        - Tabel 4: Respons meter terhadap alarm internal / hardware (ERR00..ERR07)
 *        - Tabel 6: Respons meter terhadap ketidaknormalan dan tampering (RUSAK, PERIKSA ERR20..ERR27, REVERSE ERR26)
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "display.h"
#include "tamper_manager.h"
#include "bsp_alarm_buzzer.h"
#include "load_profile.h"

int main(void) {
    printf("====================================================================\n");
    printf("  TEST VERIFIKASI TATA LETAK SPLN GAMBAR 4 & KODE ERROR SPLN D3.006   \n");
    printf("====================================================================\n\n");

    display_context_t ctx;
    bool ok = display_init(&ctx, "530000000001", 2000);
    assert(ok);

    display_spln_frame_t frame;

    /* --------------------------------------------------------------------- */
    /* UJI 1: KONDISI NORMAL (Tidak Ada Sabotase / Alarm)                   */
    /* --------------------------------------------------------------------- */
    printf("[UJI 1] Verifikasi Tampilan Normal (alarm_icon_active = false)...\n");
    ctx.alarm_icon_active = false;
    ctx.current_page = DISP_PAGE_IDPEL;

    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.alarm_code, "OK") == 0);
    assert(strcmp(frame.scroll_index_zz, "01") == 0);
    assert(strcmp(frame.main_value, "530000000001") == 0);
    assert(strcmp(frame.obis_code, "96.01") == 0);
    printf("  > Page 01 (IDPEL): OK | zz: %s | Val: %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.obis_code);

    /* Cek 14 Halaman Tampilan Otomatis (Tabel E.1) */
    /* 1. ID Pelanggan (IDPEL) */
    ctx.current_page = DISP_PAGE_IDPEL;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.alarm_code, "OK") == 0);
    assert(strcmp(frame.scroll_index_zz, "01") == 0);
    assert(strcmp(frame.main_value, "530000000001") == 0);
    assert(strcmp(frame.obis_code, "96.01") == 0);
    printf("  > Page 01 (IDPEL): OK | zz: %s | Val: %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.obis_code);

    /* 2. Energi aktif kirim Tarif 1 WBP (EA_EXP1) */
    ctx.current_page = DISP_PAGE_ENERGY_WBP;
    ctx.meas_buffer.active_energy_wbp_wh = 10500; /* 10.500 kWh */
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.scroll_index_zz, "02") == 0);
    assert(strcmp(frame.main_value, "10.500") == 0);
    assert(strcmp(frame.unit, "kWh") == 0);
    assert(strcmp(frame.obis_code, "01.08.1") == 0);
    assert(strcmp(frame.reg_info, "EA_EXP1") == 0);
    printf("  > Page 02 (EA_EXP1 Tarif 1 - 3 Desimal): OK | zz: %s | Val: %s %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* 3. Energi aktif kirim Tarif 2 LWBP1 (EA_EXP2) */
    ctx.current_page = DISP_PAGE_ENERGY_LWBP;
    ctx.meas_buffer.active_energy_lwbp_wh = 25250; /* 25.250 kWh */
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.scroll_index_zz, "03") == 0);
    assert(strcmp(frame.main_value, "25.250") == 0);
    assert(strcmp(frame.unit, "kWh") == 0);
    assert(strcmp(frame.obis_code, "01.08.2") == 0);
    assert(strcmp(frame.reg_info, "EA_EXP2") == 0);
    printf("  > Page 03 (EA_EXP2 Tarif 2 - 3 Desimal): OK | zz: %s | Val: %s %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* 4. Energi aktif kirim Tarif 3 LWBP2 (EA_EXP3) */
    ctx.current_page = DISP_PAGE_ENERGY_LWBP2;
    ctx.meas_buffer.active_energy_lwbp2_wh = 14750; /* 14.750 kWh */
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.scroll_index_zz, "04") == 0);
    assert(strcmp(frame.main_value, "14.750") == 0);
    assert(strcmp(frame.unit, "kWh") == 0);
    assert(strcmp(frame.obis_code, "01.08.3") == 0);
    assert(strcmp(frame.reg_info, "EA_EXP3") == 0);
    printf("  > Page 04 (EA_EXP3 Tarif 3 - 3 Desimal): OK | zz: %s | Val: %s %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* 5. Energi aktif kirim (EA_EXP) */
    ctx.current_page = DISP_PAGE_ACTIVE_ENERGY;
    ctx.meas_buffer.active_energy_wh = 50500; /* 50.500 kWh */
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.scroll_index_zz, "05") == 0);
    assert(strcmp(frame.main_value, "50.500") == 0);
    assert(strcmp(frame.unit, "kWh") == 0);
    assert(strcmp(frame.obis_code, "01.08") == 0);
    assert(strcmp(frame.reg_info, "EA_EXP") == 0);
    printf("  > Page 05 (EA_EXP Kirim - 3 Desimal): OK | zz: %s | Val: %s %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* 6. Energi aktif terima (EA_IMP) */
    ctx.current_page = DISP_PAGE_ENERGY_IMPORT;
    ctx.meas_buffer.active_energy_import_wh = 5200; /* 5.200 kWh */
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.scroll_index_zz, "06") == 0);
    assert(strcmp(frame.main_value, "5.200") == 0);
    assert(strcmp(frame.unit, "kWh") == 0);
    assert(strcmp(frame.obis_code, "02.08") == 0);
    assert(strcmp(frame.reg_info, "EA_IMP") == 0);
    printf("  > Page 06 (EA_IMP Terima - 3 Desimal): OK | zz: %s | Val: %s %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* 7. Energi reaktif billing kompensasi (ER_BIL) */
    ctx.current_page = DISP_PAGE_REACTIVE_ENERGY;
    ctx.meas_buffer.reactive_energy_varh = 125450; /* 125.450 kVARh */
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.scroll_index_zz, "07") == 0);
    assert(strcmp(frame.main_value, "125.450") == 0);
    assert(strcmp(frame.unit, "kVARh") == 0);
    assert(strcmp(frame.obis_code, "03.08") == 0);
    assert(strcmp(frame.reg_info, "ER_BIL") == 0);
    printf("  > Page 07 (ER_BIL Billing - 3 Desimal): OK | zz: %s | Val: %s %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* 8. Tegangan fase R (UR) */
    ctx.current_page = DISP_PAGE_VOLTAGE_R;
    ctx.meas_buffer.voltage_r_dvolts = 2300; /* 230.0 V */
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.scroll_index_zz, "08") == 0);
    assert(strcmp(frame.main_value, "230.0") == 0);
    assert(strcmp(frame.unit, "V") == 0);
    assert(strcmp(frame.obis_code, "32.07") == 0);
    assert(strcmp(frame.reg_info, "UR") == 0);
    printf("  > Page 08 (Volt R - UR - 1 Desimal): OK | zz: %s | Val: %s %s | Reg: %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.reg_info, frame.obis_code);

    /* 9. Tegangan fase S (US) */
    ctx.current_page = DISP_PAGE_VOLTAGE_S;
    ctx.meas_buffer.voltage_s_dvolts = 2300; /* 230.0 V */
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "09") == 0);
    assert(strcmp(frame.main_value, "230.0") == 0);
    assert(strcmp(frame.unit, "V") == 0);
    assert(strcmp(frame.obis_code, "52.07") == 0);
    assert(strcmp(frame.reg_info, "US") == 0);
    printf("  > Page 09 (Volt S - US - 1 Desimal): OK | zz: %s | Val: %s %s | Reg: %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.reg_info, frame.obis_code);

    /* 10. Tegangan fase T (UT) */
    ctx.current_page = DISP_PAGE_VOLTAGE_T;
    ctx.meas_buffer.voltage_t_dvolts = 2300; /* 230.0 V */
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "10") == 0);
    assert(strcmp(frame.main_value, "230.0") == 0);
    assert(strcmp(frame.unit, "V") == 0);
    assert(strcmp(frame.obis_code, "72.07") == 0);
    assert(strcmp(frame.reg_info, "UT") == 0);
    printf("  > Page 10 (Volt T - UT - 1 Desimal): OK | zz: %s | Val: %s %s | Reg: %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.reg_info, frame.obis_code);

    /* 11. Arus fase R (IR) */
    ctx.current_page = DISP_PAGE_CURRENT_R;
    ctx.meas_buffer.current_r_mamps = 11946; /* 11.946 A */
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "11") == 0);
    assert(strcmp(frame.main_value, "11.946") == 0);
    assert(strcmp(frame.unit, "A") == 0);
    assert(strcmp(frame.obis_code, "31.07") == 0);
    assert(strcmp(frame.reg_info, "IR") == 0);
    printf("  > Page 11 (Arus R - IR - 3 Desimal): OK | zz: %s | Val: %s %s | Reg: %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.reg_info, frame.obis_code);

    /* 12. Arus fase S (IS) */
    ctx.current_page = DISP_PAGE_CURRENT_S;
    ctx.meas_buffer.current_s_mamps = 11946; /* 11.946 A */
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "12") == 0);
    assert(strcmp(frame.main_value, "11.946") == 0);
    assert(strcmp(frame.unit, "A") == 0);
    assert(strcmp(frame.obis_code, "51.07") == 0);
    assert(strcmp(frame.reg_info, "IS") == 0);
    printf("  > Page 12 (Arus S - IS - 3 Desimal): OK | zz: %s | Val: %s %s | Reg: %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.reg_info, frame.obis_code);

    /* 13. Arus fase T (IT) */
    ctx.current_page = DISP_PAGE_CURRENT_T;
    ctx.meas_buffer.current_t_mamps = 11946; /* 11.946 A */
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "13") == 0);
    assert(strcmp(frame.main_value, "11.946") == 0);
    assert(strcmp(frame.unit, "A") == 0);
    assert(strcmp(frame.obis_code, "71.07") == 0);
    assert(strcmp(frame.reg_info, "IT") == 0);
    printf("  > Page 13 (Arus T - IT - 3 Desimal): OK | zz: %s | Val: %s %s | Reg: %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.reg_info, frame.obis_code);

    /* 14. Arus netral (IN) */
    ctx.current_page = DISP_PAGE_CURRENT_N;
    ctx.meas_buffer.current_n_mamps = 2389; /* 2.389 A */
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "14") == 0);
    assert(strcmp(frame.main_value, "2.389") == 0);
    assert(strcmp(frame.unit, "A") == 0);
    assert(strcmp(frame.obis_code, "91.07") == 0);
    assert(strcmp(frame.reg_info, "IN") == 0);
    printf("  > Page 14 (Arus Netral - IN - 3 Desimal): OK | zz: %s | Val: %s %s | Reg: %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.reg_info, frame.obis_code);

    /* Cek Halaman Maximum Demand (DISP_PAGE_MAX_DEMAND) - SPLN Tabel E.1 No 4 */
    ctx.current_page = DISP_PAGE_MAX_DEMAND;
    ctx.meas_buffer.max_demand_w = 12500; /* 12.500 kW */
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.scroll_index_zz, "17") == 0);
    assert(strcmp(frame.main_value, "12.500") == 0);
    assert(strcmp(frame.unit, "kW") == 0);
    assert(strcmp(frame.obis_code, "01.06") == 0);
    assert(frame.is_large_font == true);
    printf("  > Page 17 (Max Demand P_MAX - 3 Desimal): OK | zz: %s | Val: %s %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* Cek Halaman MD Timestamp (DISP_PAGE_MD_TIME) - SPLN Tabel E.1 No 5 */
    ctx.current_page = DISP_PAGE_MD_TIME;
    ctx.meas_buffer.max_demand_timestamp = 1793083500; /* 27 Okt 2026 13:45 UTC */
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "18") == 0);
    assert(strcmp(frame.obis_code, "01.06.0") == 0);
    printf("  > Page 18 (MD Timestamp): OK | zz: %s | Val: %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.obis_code);

    /* Cek Halaman Kumulatif MD (DISP_PAGE_CMD) - SPLN Tabel E.1 No 6 */
    ctx.current_page = DISP_PAGE_CMD;
    ctx.meas_buffer.cumulative_max_demand_w = 34500;
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "19") == 0);
    assert(strcmp(frame.main_value, "34.500") == 0);
    assert(strcmp(frame.unit, "kW") == 0);
    assert(strcmp(frame.obis_code, "01.02") == 0);
    printf("  > Page 19 (Cumulative MD): OK | zz: %s | Val: %s %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* Cek Halaman Counter Reset MD (DISP_PAGE_MD_RESET_COUNT) - SPLN Tabel E.1 No 7 */
    ctx.current_page = DISP_PAGE_MD_RESET_COUNT;
    ctx.meas_buffer.max_demand_reset_count = 3;
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "20") == 0);
    assert(strcmp(frame.main_value, "0003") == 0);
    assert(strcmp(frame.unit, "RST") == 0);
    assert(strcmp(frame.obis_code, "01.01") == 0);
    printf("  > Page 20 (MD Reset Counter): OK | zz: %s | Val: %s %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* Cek Halaman Jam dan Waktu (DISP_PAGE_TIME) - SPLN Butir 6.11 & Tabel E.1 No 9 */
    ctx.current_page = DISP_PAGE_TIME;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.scroll_index_zz, "22") == 0);
    assert(strcmp(frame.obis_code, "01.00") == 0);
    assert(strcmp(frame.unit, "WIB") == 0);
    assert(frame.is_large_font == false);
    printf("  > Page 22 (Time): OK | zz: %s | Val: %s %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* Cek Halaman Tanggal (DISP_PAGE_DATE) - SPLN Tabel E.1 No 10 */
    ctx.current_page = DISP_PAGE_DATE;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.scroll_index_zz, "23") == 0);
    assert(strcmp(frame.obis_code, "00.09.02") == 0);
    assert(strcmp(frame.unit, "WIB") == 0);
    assert(strcmp(frame.main_value, "12-10-2026") == 0);
    printf("  > Page 23 (Date): OK | zz: %s | Val: %s %s | OBIS: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* Cek Parameter Tambahan Manual Scroll (SPLN Tabel E.2): TEST, HIST_BILLING, P_LIM, METERID, CSUM */
    ctx.current_page = DISP_PAGE_TEST_DISPLAY;
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "00") == 0);
    assert(strcmp(frame.main_value, "88888888") == 0);
    printf("  > Manual Scroll Test Segment (Tabel E.2 No 1): OK | Val: %s [PASS]\n", frame.main_value);

    ctx.current_page = DISP_PAGE_HISTORICAL_BILLING;
    ctx.meas_buffer.billing_last_month_wh = 150250; /* 150.25 kWh */
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "24") == 0);
    assert(strcmp(frame.obis_code, "01.08*01") == 0);
    printf("  > Manual Scroll Billing Stand M-1 (Tabel E.2 No 17): OK | Val: %s %s [PASS]\n",
           frame.main_value, frame.unit);

    ctx.current_page = DISP_PAGE_POWER_LIMITER;
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "34") == 0);
    assert(strcmp(frame.main_value, "11.000") == 0);
    assert(strcmp(frame.unit, "kW") == 0);
    assert(strcmp(frame.obis_code, "00.01") == 0);
    printf("  > Manual Scroll P_LIM (Tabel E.2 No 34): OK | zz: %s | Val: %s %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit);

    ctx.current_page = DISP_PAGE_METER_ID;
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "02") == 0);
    assert(strcmp(frame.main_value, "530000000001") == 0);
    assert(strcmp(frame.obis_code, "96.02") == 0);
    printf("  > Manual Scroll METERID (Tabel E.2 No 2): OK | zz: %s | Val: %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value);

    ctx.current_page = DISP_PAGE_CHECKSUM;
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "38") == 0);
    assert(strcmp(frame.main_value, "A5B6") == 0);
    assert(strcmp(frame.unit, "CSUM") == 0);
    assert(strcmp(frame.obis_code, "96.20") == 0);
    printf("  > Manual Scroll CSUM (Tabel E.2 No 38): OK | zz: %s | Val: %s %s [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit);

    /* Cek Halaman-Halaman Baru Tambahan Tabel E.2 (Daya Impor, Sudut Fasa, MD Semu, EoB, PLN Unit) */
    ctx.current_page = DISP_PAGE_ACTIVE_POWER_IMP;
    ctx.meas_buffer.active_power_imp_w = 4500; /* 4.500 kW */
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "27") == 0);
    assert(strcmp(frame.main_value, "4.500") == 0);
    assert(strcmp(frame.unit, "kW") == 0);
    assert(strcmp(frame.obis_code, "02.07") == 0);

    ctx.current_page = DISP_PAGE_REACTIVE_POWER_IMP;
    ctx.meas_buffer.reactive_power_imp_var = 1250; /* 1.250 kvar */
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "28") == 0);
    assert(strcmp(frame.main_value, "1.250") == 0);
    assert(strcmp(frame.unit, "kvar") == 0);
    assert(strcmp(frame.obis_code, "04.07") == 0);

    ctx.current_page = DISP_PAGE_ANGLE_UR;
    ctx.meas_buffer.angle_ur_deg = 0;
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "35") == 0);
    assert(strcmp(frame.main_value, "0") == 0);
    assert(strcmp(frame.unit, "DEG") == 0);
    assert(strcmp(frame.obis_code, "14.81.0") == 0);

    ctx.current_page = DISP_PAGE_ANGLE_US;
    ctx.meas_buffer.angle_us_deg = 120;
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "36") == 0);
    assert(strcmp(frame.main_value, "120") == 0);
    assert(strcmp(frame.unit, "DEG") == 0);
    assert(strcmp(frame.obis_code, "14.81.1") == 0);

    ctx.current_page = DISP_PAGE_ANGLE_UT;
    ctx.meas_buffer.angle_ut_deg = 240;
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "37") == 0);
    assert(strcmp(frame.main_value, "240") == 0);
    assert(strcmp(frame.unit, "DEG") == 0);
    assert(strcmp(frame.obis_code, "14.81.2") == 0);

    ctx.current_page = DISP_PAGE_ANGLE_IR;
    ctx.meas_buffer.angle_ir_deg = 15;
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "39") == 0);
    assert(strcmp(frame.main_value, "15") == 0);
    assert(strcmp(frame.unit, "DEG") == 0);
    assert(strcmp(frame.obis_code, "11.81.0") == 0);

    ctx.current_page = DISP_PAGE_MAX_DEMAND_S;
    ctx.meas_buffer.max_demand_s_va = 15250; /* 15.250 kVA */
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "42") == 0);
    assert(strcmp(frame.main_value, "15.250") == 0);
    assert(strcmp(frame.unit, "kVA") == 0);
    assert(strcmp(frame.obis_code, "09.06.0") == 0);

    ctx.current_page = DISP_PAGE_EOB;
    ctx.meas_buffer.eob_day = 1;
    ctx.meas_buffer.eob_hour = 0;
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "44") == 0);
    assert(strcmp(frame.main_value, "01-00") == 0);
    assert(strcmp(frame.unit, "EoB") == 0);
    assert(strcmp(frame.obis_code, "00.09.01") == 0);

    ctx.current_page = DISP_PAGE_PLN_UNIT;
    strncpy(ctx.meas_buffer.pln_unit_code, "UID JAYA", sizeof(ctx.meas_buffer.pln_unit_code) - 1);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.scroll_index_zz, "45") == 0);
    assert(strcmp(frame.main_value, "UID JAYA") == 0);
    assert(strcmp(frame.unit, "PLN") == 0);
    assert(strcmp(frame.obis_code, "96.01.0") == 0);
    printf("  > Seluruh Halaman Tambahan Tabel E.2 (Daya Impor, Sudut Fasa, MD Semu, EoB, Unit PLN) Lulus Verifikasi! [PASS]\n");

    /* Cek Carousel Wrap saat Normal: Dari Page 14 (IN) harus langsung ke Page 01 (IDPEL) */
    ctx.current_page = DISP_PAGE_CURRENT_N;
    display_process_tick(&ctx, 2000);
    assert(ctx.current_page == DISP_PAGE_IDPEL);
    printf("  > Carousel Skip Test (Normal tidak menampilkan halaman sabotase): [PASS]\n\n");

    /* --------------------------------------------------------------------- */
    /* UJI 2: KONDISI ALARM SABOTASE AKTIF (Default / Tombol PC13 Ditekan)   */
    /* --------------------------------------------------------------------- */
    printf("[UJI 2] Verifikasi Tampilan Alarm Sabotase Case Open (Tombol PC13)...\n");
    ctx.alarm_icon_active = true;
    ctx.active_tamper_mask = TAMPER_VECTOR_CASE_OPEN;

    /* Cek banner alarm pada halaman pengukuran normal */
    ctx.current_page = DISP_PAGE_VOLTAGE_R;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == true);
    assert(strcmp(frame.alarm_code, "!ALM") == 0);
    printf("  > Banner Alarm Tersemat di Page 08: Respon Alarm = '%s' [PASS]\n", frame.alarm_code);

    /* Cek Halaman Khusus Darurat Sabotase (DISP_PAGE_TAMPER_ALARM) - SPLN Tabel 6 No 1 */
    ctx.current_page = DISP_PAGE_TAMPER_ALARM;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == true);
    assert(strcmp(frame.scroll_index_zz, "AL") == 0);
    assert(strcmp(frame.main_value, "RUSAK") == 0);
    assert(strcmp(frame.unit, "-") == 0);
    assert(strcmp(frame.obis_code, "96.50") == 0);
    assert(frame.is_large_font == false);
    printf("  > SPLN Tabel 6 No 1 (Case Open): Header: '%s' | zz: '%s' | Teks: '%s' | Kode: '%s' | OBIS: '%s' [PASS]\n",
           frame.alarm_code, frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* Cek Siklus Carousel saat Alarm: Dari Page 14 (IN) harus masuk ke DISP_PAGE_TAMPER_ALARM */
    ctx.current_page = DISP_PAGE_CURRENT_N;
    display_process_tick(&ctx, 2000);
    assert(ctx.current_page == DISP_PAGE_TAMPER_ALARM);
    printf("  > Carousel Inclusion Test (Halaman sabotase masuk dalam siklus carousel): [PASS]\n");

    /* Dari DISP_PAGE_TAMPER_ALARM lanjut ke DISP_PAGE_IDPEL */
    display_process_tick(&ctx, 2000);
    assert(ctx.current_page == DISP_PAGE_IDPEL);
    printf("  > Carousel Loop Completion Test (Kembali ke Page 01): [PASS]\n\n");

    /* --------------------------------------------------------------------- */
    /* UJI 3: PEMULIHAN SABOTASE (Kembali Normal)                            */
    /* --------------------------------------------------------------------- */
    printf("[UJI 3] Verifikasi Pemulihan Sabotase (Kembali Normal)...\n");
    display_set_tamper_status(&ctx, 0);
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.alarm_code, "OK") == 0);
    printf("  > Status Layar Berhasil Pulih ke OK [PASS]\n\n");

    /* --------------------------------------------------------------------- */
    /* UJI 4: VERIFIKASI LENGKAP 9 VEKTOR SABOTASE SPLN D3.006 TABEL 6       */
    /* --------------------------------------------------------------------- */
    printf("[UJI 4] Verifikasi Seluruh 9 Vektor Sabotase SPLN D3.006: 2021 Tabel 6...\n");
    ctx.current_page = DISP_PAGE_TAMPER_ALARM;

    /* 1. Case Open -> RUSAK, Kode: - */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_CASE_OPEN);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "RUSAK") == 0);
    assert(strcmp(frame.unit, "-") == 0);
    printf("  > 4.1 Case Open: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 2. Terminal Cover Open -> PERIKSA, Kode: ERR20 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_TERMINAL_OPEN);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR20") == 0);
    printf("  > 4.2 Terminal Open: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 3. Wrong Phase Sequence -> PERIKSA, Kode: ERR21 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_WRONG_SEQUENCE);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR21") == 0);
    printf("  > 4.3 Wrong Sequence: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 4. Cross Phase Current/Voltage -> PERIKSA, Kode: ERR22 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_CROSS_PHASE);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR22") == 0);
    printf("  > 4.4 Cross Phase: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 5. Neutral Missing / Broken -> PERIKSA, Kode: ERR23 */
    ctx.current_page = DISP_PAGE_TAMPER_ALARM;
    display_set_tamper_status(&ctx, TAMPER_VECTOR_NEUTRAL_BYPASS);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR23") == 0);
    printf("  > 4.5 Neutral Missing: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 6. Missing 1/2 Phase Voltage -> PERIKSA, Kode: ERR24 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_PHASE_LOSS);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR24") == 0);
    printf("  > 4.6 Voltage Loss: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 7. External Magnetic Field Induction -> PERIKSA, Kode: ERR25 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_MAGNETIC_FIELD);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR25") == 0);
    printf("  > 4.7 Magnetic Field: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 8. Reverse Power -> REVERSE, Kode: ERR26 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_REVERSE_POWER);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "REVERSE") == 0);
    assert(strcmp(frame.unit, "ERR26") == 0);
    printf("  > 4.8 Reverse Power: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 9. Quadrant 4 PF < 0.85 -> PERIKSA, Kode: ERR27 */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_LOW_PF_QUAD4);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR27") == 0);
    printf("  > 4.9 Quad 4 (Low PF): Teks='%s' | Kode='%s' [PASS]\n\n", frame.main_value, frame.unit);

    /* --------------------------------------------------------------------- */
    /* UJI 5: VERIFIKASI LENGKAP 8 ALARM INTERNAL SPLN D3.006 TABEL 4        */
    /* --------------------------------------------------------------------- */
    printf("[UJI 5] Verifikasi Seluruh 8 Alarm Hardware Internal SPLN D3.006: 2021 Tabel 4...\n");
    display_set_tamper_status(&ctx, 0);

    /* 0. Flash Memory Rusak -> Teks: -, Kode: ERR00 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_FLASH_ERROR);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR00") == 0);
    printf("  > 5.0 Flash Error: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 1. RAM Rusak -> Teks: -, Kode: ERR01 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_RAM_ERROR);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR01") == 0);
    printf("  > 5.1 RAM Error: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 2. Clock Loss / RTC -> Teks: -, Kode: ERR02 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_RTC_ERROR);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR02") == 0);
    printf("  > 5.2 Clock Loss RTC: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 3. Low Battery -> Teks: -, Kode: ERR03 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_LOW_BATTERY);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR03") == 0);
    printf("  > 5.3 Low Battery: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 4. MCU Error -> Teks: -, Kode: ERR04 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_MCU_ERROR);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR04") == 0);
    printf("  > 5.4 MCU Error: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 5. ADC Sampling Error -> Teks: -, Kode: ERR05 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_ADC_ERROR);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR05") == 0);
    printf("  > 5.5 ADC Sampling: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 6. Supercapacitor Fail -> Teks: -, Kode: ERR06 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_SUPERCAP_FAIL);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR06") == 0);
    printf("  > 5.6 Supercap Fail: Teks='%s' | Kode='%s' [PASS]\n", frame.main_value, frame.unit);

    /* 7. Relay Fail -> Teks: -, Kode: ERR07 */
    display_set_internal_alarm(&ctx, SPLN_ALARM_RELAY_FAIL);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "-") == 0);
    assert(strcmp(frame.unit, "ERR07") == 0);
    printf("  > 5.7 Relay Fail: Teks='%s' | Kode='%s' [PASS]\n\n", frame.main_value, frame.unit);

    /* --------------------------------------------------------------------- */
    /* UJI 6: VERIFIKASI FORMAT TAMPILAN ASCII CLI (display_render_frame)    */
    /* --------------------------------------------------------------------- */
    printf("[UJI 6] Verifikasi Format Tampilan ASCII CLI (display_render_frame)...\n");
    char l1[64], l2[64], l3[64];

    display_set_internal_alarm(&ctx, 0);
    display_set_tamper_status(&ctx, TAMPER_VECTOR_CASE_OPEN);
    ctx.current_page = DISP_PAGE_TAMPER_ALARM;
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l2, "STATUS SABOTASE") == 0);
    assert(strcmp(l3, "RUSAK") == 0);
    printf("  > Case Open CLI Frame: L2='%s' | L3='%s' [PASS]\n", l2, l3);

    display_set_tamper_status(&ctx, TAMPER_VECTOR_TERMINAL_OPEN);
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l3, "PERIKSA ERR20") == 0);
    printf("  > Terminal Open CLI Frame: L3='%s' [PASS]\n", l3);

    display_set_tamper_status(&ctx, TAMPER_VECTOR_REVERSE_POWER);
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l3, "REVERSE ERR26") == 0);
    printf("  > Reverse Power CLI Frame: L3='%s' [PASS]\n", l3);

    ctx.alarm_icon_active = false;
    ctx.current_page = DISP_PAGE_ENERGY_WBP;
    ctx.meas_buffer.active_energy_wbp_wh = 10500;
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l2, "ENERGY WBP (T1)") == 0);
    assert(strstr(l3, "10.500 kWh") != NULL);
    printf("  > Energy WBP CLI Frame: L2='%s' | L3='%s' [PASS]\n", l2, l3);

    ctx.current_page = DISP_PAGE_ENERGY_LWBP;
    ctx.meas_buffer.active_energy_lwbp_wh = 25250;
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l2, "ENERGY LWBP (T2)") == 0);
    assert(strstr(l3, "25.250 kWh") != NULL);
    printf("  > Energy LWBP CLI Frame: L2='%s' | L3='%s' [PASS]\n", l2, l3);

    ctx.current_page = DISP_PAGE_ENERGY_LWBP2;
    ctx.meas_buffer.active_energy_lwbp2_wh = 14750;
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l2, "ENERGY LWBP2 (T3)") == 0);
    assert(strstr(l3, "14.750 kWh") != NULL);
    printf("  > Energy LWBP2 CLI Frame: L2='%s' | L3='%s' [PASS]\n", l2, l3);

    ctx.current_page = DISP_PAGE_ENERGY_IMPORT;
    ctx.meas_buffer.active_energy_import_wh = 5200;
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l2, "ENERGY IMPORT (EA_IMP)") == 0);
    assert(strstr(l3, "5.200 kWh") != NULL);
    printf("  > Energy Import CLI Frame: L2='%s' | L3='%s' [PASS]\n", l2, l3);

    ctx.current_page = DISP_PAGE_REACTIVE_ENERGY;
    ctx.meas_buffer.reactive_energy_varh = 89123; /* 89.123 kvarh */
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l2, "REACTIVE ENERGY (ER_BIL)") == 0);
    assert(strstr(l3, "89.123 kvarh") != NULL);
    printf("  > Reactive Energy CLI Frame: L2='%s' | L3='%s' [PASS]\n", l2, l3);

    ctx.current_page = DISP_PAGE_MAX_DEMAND;
    ctx.meas_buffer.max_demand_w = 8750;
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l2, "MAX DEMAND (P_MAX)") == 0);
    assert(strstr(l3, "8.750 kW") != NULL);
    printf("  > Max Demand CLI Frame: L2='%s' | L3='%s' [PASS]\n", l2, l3);

    ctx.current_page = DISP_PAGE_POWER_LIMITER;
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l2, "POWER LIMITER (P_LIM)") == 0);
    assert(strstr(l3, "11.000 kW") != NULL);
    printf("  > Power Limiter CLI Frame: L2='%s' | L3='%s' [PASS]\n", l2, l3);

    ctx.current_page = DISP_PAGE_CHECKSUM;
    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strcmp(l2, "FIRMWARE CHECKSUM") == 0);
    assert(strstr(l3, "CSUM A5B6") != NULL);
    printf("  > Checksum CLI Frame: L2='%s' | L3='%s' [PASS]\n", l2, l3);

    /* Verifikasi Simbol Dinamis Baris 1 (SPLN Butir 6.11) */
    printf("[UJI 7] Verifikasi Simbol Dinamis Baris 1 (SPLN Butir 6.11)...\n");
    ctx.active_alarm_mask = SPLN_ALARM_LOW_BATTERY;
    ctx.active_tamper_mask = 0;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_battery_low == true);
    printf("  > Simbol Baterai Low (ERR03): is_battery_low=true [PASS]\n");

    ctx.active_alarm_mask = 0;
    ctx.active_tamper_mask = TAMPER_VECTOR_REVERSE_POWER;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_reverse_power == true);
    assert(strstr(frame.header_symbols, "REV") != NULL);
    printf("  > Simbol Arah Arus Balik: is_reverse_power=true | Header '%s' [PASS]\n", frame.header_symbols);

    ctx.active_tamper_mask = 0;
    ctx.meas_buffer.voltage_r_dvolts = 500; /* Drop fasa R < 110V */
    ctx.meas_buffer.voltage_s_dvolts = 2300;
    ctx.meas_buffer.voltage_t_dvolts = 2300;
    display_render_spln_frame(&ctx, &frame);
    assert((frame.voltage_phases & 0x01) == 0);
    assert((frame.voltage_phases & 0x02) != 0);
    assert(strstr(frame.header_symbols, "U-ST") != NULL);

    /* Uji fasa R mati total (0 Volt) harus mematikan indikator fasa R (URST -> U-ST) */
    ctx.meas_buffer.voltage_r_dvolts = 0;
    display_render_spln_frame(&ctx, &frame);
    assert((frame.voltage_phases & 0x01) == 0);
    assert(strstr(frame.header_symbols, "U-ST") != NULL);
    printf("  > Indikator Fasa Tegangan Dinamis (Fasa R Drop & 0V Disconnect): Header '%s' [PASS]\n\n", frame.header_symbols);

    /* --------------------------------------------------------------------- */
    /* UJI 8: VERIFIKASI AUDIO CADENCE BUZZER (SPLN D3.006 Butir 6.14)      */
    /* --------------------------------------------------------------------- */
    printf("[UJI 8] Verifikasi Audio Cadence Buzzer (SPLN D3.006 Butir 6.14)...\n");
    bsp_alarm_buzzer_init();
    assert(bsp_alarm_buzzer_get() == false);

    /* 1. Kondisi normal tanpa alarm & tanpa tombol: Buzzer OFF */
    bool b_state = bsp_alarm_buzzer_update_cadence(false, false, 100);
    assert(b_state == false);
    assert(bsp_alarm_buzzer_get() == false);

    /* 2. Alarm aktif: Denyut 200 ms ON / 200 ms OFF */
    b_state = bsp_alarm_buzzer_update_cadence(true, false, 100); /* accum = 100 ms (< 200 ms) */
    assert(b_state == true);
    assert(bsp_alarm_buzzer_get() == true);
    printf("  > Alarm Cadence (t = 100 ms): Output = ON [PASS]\n");

    b_state = bsp_alarm_buzzer_update_cadence(true, false, 100); /* accum = 200 ms (>= 200 ms) */
    assert(b_state == false);
    assert(bsp_alarm_buzzer_get() == false);
    printf("  > Alarm Cadence (t = 200 ms): Output = OFF [PASS]\n");

    b_state = bsp_alarm_buzzer_update_cadence(true, false, 100); /* accum = 300 ms */
    assert(b_state == false);
    assert(bsp_alarm_buzzer_get() == false);

    b_state = bsp_alarm_buzzer_update_cadence(true, false, 100); /* accum = 400 ms -> wrap 0 ms */
    assert(b_state == true);
    assert(bsp_alarm_buzzer_get() == true);
    printf("  > Alarm Cadence (t = 400 ms Wrap): Output = ON [PASS]\n");

    /* 3. Override Beep Tombol saat Alarm berada pada fase OFF */
    bsp_alarm_buzzer_update_cadence(true, false, 100); /* 100 ms (fase ON) */
    bsp_alarm_buzzer_update_cadence(true, false, 100); /* 200 ms (fase OFF) */
    assert(bsp_alarm_buzzer_get() == false);

    /* Tekan tombol: beep_override = true */
    b_state = bsp_alarm_buzzer_update_cadence(true, true, 50);
    assert(b_state == true);
    assert(bsp_alarm_buzzer_get() == true);
    printf("  > Button Beep Override (Dipaksa ON saat fase OFF alarm): Output = ON [PASS]\n");

    /* 4. Pemulihan Alarm (Normal): Buzzer langsung padam */
    b_state = bsp_alarm_buzzer_update_cadence(false, false, 50);
    assert(b_state == false);
    assert(bsp_alarm_buzzer_get() == false);
    printf("  > Alarm Recovered: Output = OFF, Timer Reset [PASS]\n\n");
    /* --------------------------------------------------------------------- */
    /* UJI 9: VERIFIKASI MAXIMUM DEMAND & BILLING SNAPSHOT 12 BULAN          */
    /* --------------------------------------------------------------------- */
    printf("[UJI 9] Verifikasi Maximum Demand 15-Menit & Billing Snapshot 12 Bulan...\n");
    max_demand_tracker_t md_track;
    max_demand_init(&md_track, 10); /* Gunakan interval 10 detik untuk unit test */

    /* Interval 1: 5000 W selama 10 detik -> MD = 5000 W */
    for (int sec = 0; sec < 10; sec++) {
        max_demand_process_second(&md_track, 5000, 1727570400 + sec);
    }
    assert(md_track.current_demand_w == 5000);
    assert(md_track.max_demand_w == 5000);
    assert(md_track.max_demand_timestamp == 1727570409);
    printf("  > MD Interval 1 (5000 W): max_demand = %lu W [PASS]\n", (unsigned long)md_track.max_demand_w);

    /* Interval 2: 9500 W selama 10 detik -> MD naik menjadi 9500 W */
    for (int sec = 0; sec < 10; sec++) {
        max_demand_process_second(&md_track, 9500, 1727570410 + sec);
    }
    assert(md_track.current_demand_w == 9500);
    assert(md_track.max_demand_w == 9500);
    printf("  > MD Interval 2 Peak (9500 W): max_demand = %lu W [PASS]\n", (unsigned long)md_track.max_demand_w);

    /* Interval 3: 2000 W selama 10 detik -> MD tetap bertahan di peak 9500 W */
    for (int sec = 0; sec < 10; sec++) {
        max_demand_process_second(&md_track, 2000, 1727570420 + sec);
    }
    assert(md_track.current_demand_w == 2000);
    assert(md_track.max_demand_w == 9500);
    printf("  > MD Interval 3 Hold Peak: max_demand = %lu W [PASS]\n", (unsigned long)md_track.max_demand_w);

    /* Verifikasi Snapshot Billing 12 Bulan */
    billing_snapshot_ctx_t b_ctx;
    billing_snapshot_init(&b_ctx);
    assert(billing_snapshot_get_count(&b_ctx) == 0);

    for (uint32_t m = 0; m < 15; m++) {
        billing_snapshot_entry_t snap = {
            .billing_epoch_sec = 1727570400 + (m * 2592000U),
            .active_energy_import_wh = (m + 1) * 100000ULL,
            .active_energy_wbp_wh = (m + 1) * 30000ULL,
            .active_energy_lwbp_wh = (m + 1) * 70000ULL,
            .reactive_energy_varh = (m + 1) * 20000ULL,
            .max_demand_w = 9500 + m * 100,
            .max_demand_timestamp = 1727570400 + (m * 2592000U) + 1800U
        };
        billing_snapshot_record(&b_ctx, &snap);
    }
    /* Kapasitas ring buffer tertahan tepat pada 12 bulan terakhir */
    assert(billing_snapshot_get_count(&b_ctx) == BILLING_MAX_SNAPSHOTS);
    assert(b_ctx.total_snapshots == 15);

    billing_snapshot_entry_t read_snap;
    for (size_t idx = 0; idx < BILLING_MAX_SNAPSHOTS; idx++) {
        bool ok_read = billing_snapshot_get_entry(&b_ctx, idx, &read_snap);
        assert(ok_read);
        assert(billing_snapshot_verify_integrity(&read_snap) == true);
    }
    printf("  > Ring Buffer Billing 12 Bulan & Verifikasi CRC-16 Integrity: [PASS]\n");

    /* Reset MD saat billing rollover */
    max_demand_reset(&md_track);
    assert(md_track.max_demand_w == 0);
    assert(md_track.cumulative_max_demand_w == 9500);
    assert(md_track.reset_count == 1);
    printf("  > Reset MD Billing Rollover (CMD = %lu W, Counter = %lu): [PASS]\n\n",
           (unsigned long)md_track.cumulative_max_demand_w, (unsigned long)md_track.reset_count);

    /* --------------------------------------------------------------------- */
    /* UJI 10: VERIFIKASI DETEKSI SABOTASE ARUS NETRAL & REVERSE POWER       */
    /* --------------------------------------------------------------------- */
    printf("[UJI 10] Verifikasi Integritas Tamper Vektor Arus Netral & Reverse Power...\n");
    assert(TAMPER_VECTOR_NEUTRAL_BYPASS == (1 << 3));
    assert(TAMPER_VECTOR_REVERSE_POWER == (1 << 4));

    ctx.current_page = DISP_PAGE_TAMPER_ALARM;
    display_set_tamper_status(&ctx, TAMPER_VECTOR_NEUTRAL_BYPASS);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "PERIKSA") == 0);
    assert(strcmp(frame.unit, "ERR23") == 0);

    display_set_tamper_status(&ctx, TAMPER_VECTOR_REVERSE_POWER);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "REVERSE") == 0);
    assert(strcmp(frame.unit, "ERR26") == 0);
    printf("  > Deteksi ERR23 & ERR26: [PASS]\n\n");

    /* --------------------------------------------------------------------- */
    /* UJI 11: VERIFIKASI ROTASI MULTI-ERROR PER 3 DETIK (SPLN BUTIR 6.11)   */
    /* --------------------------------------------------------------------- */
    printf("[UJI 11] Verifikasi Rotasi Multi-Error Bergantian per 3 Detik (SPLN Butir 6.11)...\n");
    ctx.current_page = DISP_PAGE_TAMPER_ALARM;
    /* Aktifkan 2 sabotase bersamaan: Terminal Open (ERR20) & Reverse Power (ERR26) */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_TERMINAL_OPEN | TAMPER_VECTOR_REVERSE_POWER);
    display_set_internal_alarm(&ctx, 0);

    /* t = 0s -> Interval pertama: ERR20 */
    display_set_time(&ctx, 0);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.unit, "ERR20") == 0);
    printf("  > t = 0s (Siklus 1): Kode = %s [PASS]\n", frame.unit);

    /* t = 2s -> Masih interval pertama: ERR20 */
    display_set_time(&ctx, 2);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.unit, "ERR20") == 0);
    printf("  > t = 2s (Tahan 3s): Kode = %s [PASS]\n", frame.unit);

    /* t = 3s -> Masuk interval kedua: ERR26 */
    display_set_time(&ctx, 3);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.unit, "ERR26") == 0);
    printf("  > t = 3s (Siklus 2 berganti): Kode = %s [PASS]\n", frame.unit);

    /* t = 5s -> Masih interval kedua: ERR26 */
    display_set_time(&ctx, 5);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.unit, "ERR26") == 0);

    /* t = 6s -> Kembali berotasi ke interval pertama: ERR20 */
    display_set_time(&ctx, 6);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.unit, "ERR20") == 0);
    printf("  > t = 6s (Siklus 1 berulang): Kode = %s [PASS]\n", frame.unit);

    /* Uji kombinasi Sabotase (ERR20) + Hardware Internal Alarm (ERR03) */
    display_set_tamper_status(&ctx, TAMPER_VECTOR_TERMINAL_OPEN);
    display_set_internal_alarm(&ctx, SPLN_ALARM_LOW_BATTERY);

    display_set_time(&ctx, 0);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.unit, "ERR20") == 0);

    display_set_time(&ctx, 3);
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.unit, "ERR03") == 0);
    printf("  > Kombinasi Tamper ERR20 + Alarm ERR03 berotasi presisi per 3 detik [PASS]\n\n");

    /* --------------------------------------------------------------------- */
    /* UJI 12: VERIFIKASI INDIKATOR LO VOLT & HI VOLT (SPLN BUTIR 10.3 TABEL 9)*/
    /* --------------------------------------------------------------------- */
    printf("[UJI 12] Verifikasi Indikator Lo Volt & Hi Volt (SPLN Butir 10.3 Tabel 9)...\n");
    assert(display_get_voltage_status(2300U) == DISPLAY_VOLT_NORMAL);
    assert(display_get_voltage_status(1100U) == DISPLAY_VOLT_LOW);
    assert(display_get_voltage_status(1149U) == DISPLAY_VOLT_LOW);
    assert(display_get_voltage_status(1150U) == DISPLAY_VOLT_NORMAL);
    assert(display_get_voltage_status(2760U) == DISPLAY_VOLT_NORMAL);
    assert(display_get_voltage_status(2800U) == DISPLAY_VOLT_HIGH);

    /* Bersihkan alarm & sabotase untuk menguji efek murni anomali tegangan */
    display_set_tamper_status(&ctx, 0);
    display_set_internal_alarm(&ctx, 0);

    /* Simulasi Fasa R Tegangan Rendah (98.0 V < 115.0 V) */
    ctx.meas_buffer.voltage_r_dvolts = 980U;
    ctx.current_page = DISP_PAGE_VOLTAGE_R;
    ctx.timer_accumulator_ms = 0;

    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "Lo Volt") == 0);
    assert(strcmp(frame.unit, "V") == 0);
    assert(frame.is_alarm_active == true);
    assert(strcmp(frame.alarm_code, "!ALM") == 0);
    printf("  > Phase R Lo Volt (<115V): main_value='%s' | unit='%s' | alarm='%s' [PASS]\n",
           frame.main_value, frame.unit, frame.alarm_code);

    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strstr(l2, "[Lo Volt]") != NULL);
    assert(strstr(l3, "Lo Volt (98.0 V)") != NULL);
    printf("  > CLI Frame Lo Volt: L2='%s' | L3='%s' [PASS]\n", l2, l3);

    /* Simulasi Fasa S Tegangan Tinggi (285.0 V > 276.0 V) */
    ctx.meas_buffer.voltage_r_dvolts = 2300U;
    ctx.meas_buffer.voltage_s_dvolts = 2850U;
    ctx.current_page = DISP_PAGE_VOLTAGE_S;
    ctx.timer_accumulator_ms = 0;

    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "Hi Volt") == 0);
    assert(strcmp(frame.unit, "V") == 0);
    assert(frame.is_alarm_active == true);
    assert(strcmp(frame.alarm_code, "!ALM") == 0);
    printf("  > Phase S Hi Volt (>276V): main_value='%s' | unit='%s' | alarm='%s' [PASS]\n",
           frame.main_value, frame.unit, frame.alarm_code);

    display_render_frame(&ctx, l1, l2, l3, sizeof(l1));
    assert(strstr(l2, "[Hi Volt]") != NULL);
    assert(strstr(l3, "Hi Volt (285.0 V)") != NULL);
    printf("  > CLI Frame Hi Volt: L2='%s' | L3='%s' [PASS]\n", l2, l3);

    /* Pemulihan ke tegangan normal (230.0 V) */
    ctx.meas_buffer.voltage_s_dvolts = 2300U;
    display_render_spln_frame(&ctx, &frame);
    assert(strcmp(frame.main_value, "230.0") == 0);
    assert(frame.is_alarm_active == false);
    assert(strcmp(frame.alarm_code, "OK") == 0);
    printf("  > Pemulihan Normal (230V): main_value='%s' | alarm='%s' [PASS]\n\n",
           frame.main_value, frame.alarm_code);

    /* --------------------------------------------------------------------- */
    /* UJI 13: VERIFIKASI KUADRAN DAYA LISTRIK 4-KUADRAN (SPLN GAMBAR 6)     */
    /* --------------------------------------------------------------------- */
    printf("[UJI 13] Verifikasi Aliran Daya 4-Kuadran (SPLN Butir 6.11 Gambar 6 & Lampiran A)...\n");
    /* 1. Kondisi Idle / Tanpa Beban (Arus 0, P=0, Q=0) -> Kuadran 0 (Simbol silang kosong) */
    ctx.meas_buffer.current_r_mamps = 0;
    ctx.meas_buffer.current_s_mamps = 0;
    ctx.meas_buffer.current_t_mamps = 0;
    ctx.meas_buffer.active_power_w = 0;
    ctx.meas_buffer.reactive_power_var = 0;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.power_quadrant == 0);
    printf("  > Idle / No Load (P=0, Q=0): power_quadrant = %d (Lingkaran Silang Kosong) [PASS]\n", frame.power_quadrant);

    /* 2. Kuadran 1 (+P, +Q): Beban Induktif Normal (Konsumsi / Impor dari Grid) */
    ctx.meas_buffer.current_r_mamps = 10000;
    ctx.meas_buffer.active_power_w = 6900;
    ctx.meas_buffer.reactive_power_var = 1500;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.power_quadrant == 1);
    printf("  > Kuadran 1 (+P, +Q): power_quadrant = %d (Sektor Kanan-Atas Aktif) [PASS]\n", frame.power_quadrant);

    /* 3. Kuadran 2 (-P, +Q): Ekspor PLTS Lagging (Injeksi Daya Balik ke Grid) */
    ctx.meas_buffer.active_power_w = -5000;
    ctx.meas_buffer.reactive_power_var = 1200;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.power_quadrant == 2);
    printf("  > Kuadran 2 (-P, +Q): power_quadrant = %d (Sektor Kiri-Atas Aktif) [PASS]\n", frame.power_quadrant);

    /* 4. Kuadran 3 (-P, -Q): Ekspor PLTS Leading (Injeksi Daya Balik Kapasitif) */
    ctx.meas_buffer.active_power_w = -5000;
    ctx.meas_buffer.reactive_power_var = -1200;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.power_quadrant == 3);
    printf("  > Kuadran 3 (-P, -Q): power_quadrant = %d (Sektor Kiri-Bawah Aktif) [PASS]\n", frame.power_quadrant);

    /* 5. Kuadran 4 (+P, -Q): Beban Kapasitif (Kapasitor Bank Konsumsi) */
    ctx.meas_buffer.active_power_w = 6900;
    ctx.meas_buffer.reactive_power_var = -1500;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.power_quadrant == 4);
    printf("  > Kuadran 4 (+P, -Q): power_quadrant = %d (Sektor Kanan-Bawah Aktif) [PASS]\n\n", frame.power_quadrant);

    /* --------------------------------------------------------------------- */
    /* UJI 14: VERIFIKASI IKON MODEM TELEPON & RELAI SAKELAR (SPLN GAMBAR 6) */
    /* --------------------------------------------------------------------- */
    printf("[UJI 14] Verifikasi Ikon Modem & Sakelar Relai (SPLN Butir 6.11 Gambar 6)...\n");
    /* 1. Kondisi Modem Idle (Default) */
    display_set_modem_status(&ctx, false);
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_modem_active == false);
    printf("  > Modem Idle: is_modem_active = false [PASS]\n");

    /* 2. Kondisi Modem Berkomunikasi Aktif (Gagang Telepon Menyala) */
    display_set_modem_status(&ctx, true);
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_modem_active == true);
    printf("  > Modem Aktif: is_modem_active = true (Simbol Telepon Menyala) [PASS]\n");

    /* 3. Verifikasi Status Sakelar Terbuka / Trip */
    ctx.active_tamper_mask = TAMPER_VECTOR_CASE_OPEN; /* Memicu trip relai */
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_relay_closed == false);
    printf("  > Relai Trip (Case Open): is_relay_closed = false (Simbol Sakelar Terbuka Menyala) [PASS]\n");

    /* 4. Verifikasi Status Sakelar Tertutup / Normal */
    ctx.active_tamper_mask = 0;
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_relay_closed == true);
    printf("  > Relai Normal: is_relay_closed = true (Simbol Sakelar Menghubungkan Terminal) [PASS]\n\n");

    /* --------------------------------------------------------------------- */
    /* UJI 15: VERIFIKASI PENORMALAN INDIKASI BATERAI TANPA RESET (SPLN HAL 39) */
    /* --------------------------------------------------------------------- */
    printf("[UJI 15] Verifikasi Penormalan Indikasi Baterai Tanpa Reset (SPLN Hal 39)...\n");
    /* 1. Kondisi Baterai Drop <= 2.7V (ERR03 Aktif, Simbol Baterai Berkedip) */
    ctx.current_page = DISP_PAGE_TAMPER_ALARM;
    display_set_internal_alarm(&ctx, SPLN_ALARM_LOW_BATTERY);
    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_battery_low == true);
    assert(strcmp(frame.unit, "ERR03") == 0);
    printf("  > 1. Baterai Drop (<=2.7V): is_battery_low=true (Berkedip), Kode=ERR03 [PASS]\n");

    /* 2. Simulasi Penggantian Baterai Normal Tanpa Me-reset Display Context / CPU */
    /* Sesuai SPLN D3.006:2021 Butir 6.11 Hal 39 Catatan:
     * "CATATAN: *) Indikasi baterai kembali normal dapat dilakukan secara otomatis tanpa me-reset." */
    uint32_t active_alarms = display_get_internal_alarm(&ctx);
    active_alarms &= ~SPLN_ALARM_LOW_BATTERY;
    display_set_internal_alarm(&ctx, active_alarms);

    display_render_spln_frame(&ctx, &frame);
    assert(frame.is_battery_low == false);
    assert(strcmp(frame.unit, "ERR03") != 0);
    assert(ctx.alarm_icon_active == false);
    printf("  > 2. Penormalan Otomatis Tanpa Reset: is_battery_low=false (Solid), ERR03 Hilang [PASS]\n\n");

    printf("====================================================================\n");
    printf("  HASIL: 100%% SELURUH KODE ERROR SPLN D3.006 TABEL 4 & 6 LULUS!      \n");
    printf("====================================================================\n");

    return 0;
}
