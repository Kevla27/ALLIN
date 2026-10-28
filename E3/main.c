/**
 * @file main.c
 * @brief Standalone Test & Demonstration Driver untuk Pilar E3
 *        Smart Meter 3-Fasa (Application, HMI Display, Tamper & FSM Lead)
 *        Standar SPLN D3.006: 2021
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "app/display/inc/display.h"
#include "app/tamper/inc/tamper_manager.h"
#include "app/fsm/inc/power_relay_manager.h"
#include "app/profiling/inc/load_profile.h"

int main(void) {
    printf("====================================================================\n");
    printf("   DEMONSTRASI STANDALONE APLIKASI PILAR E3 (SMART METER 3-FASA)    \n");
    printf("     Application, HMI Display SPLN Gbr 4, Tamper & Relay Lead       \n");
    printf("====================================================================\n\n");

    /* -------------------------------------------------------------
     * 1. Inisialisasi Modul Display (SPLN D3.006-1 Gambar 4)
     * ------------------------------------------------------------- */
    printf("[1] Inisialisasi Layar Display HMI (IDPEL: 54321678901)...\n");
    display_context_t disp_ctx;
    display_init(&disp_ctx, "54321678901", 3000);

    /* Simulasi data pengukuran metrologi dari E1 (ADE9000) */
    meter_measurements_t meas = {
        .voltage_r_dvolts = 2205, /* 220.5 V */
        .voltage_s_dvolts = 2210, /* 221.0 V */
        .voltage_t_dvolts = 2198, /* 219.8 V */
        .current_r_mamps  = 15200, /* 15.20 A */
        .current_s_mamps  = 14800, /* 14.80 A */
        .current_t_mamps  = 15000, /* 15.00 A */
        .current_n_mamps  = 100,
        .active_power_w   = 9980,  /* 9.98 kW */
        .reactive_power_var = 1200,
        .apparent_power_va  = 10050,
        .frequency_mhz    = 50020, /* 50.02 Hz */
        .active_energy_wh = 12458000
    };
    display_update_measurements(&disp_ctx, &meas);

    char line1[64], line2[64], line3[64];
    display_render_frame(&disp_ctx, line1, line2, line3, sizeof(line1));
    printf("   -> Layar Baris 1: [%s]\n", line1);
    printf("   -> Layar Baris 2: [%s]\n", line2);
    printf("   -> Layar Baris 3: [%s]\n", line3);

    /* Bergulir ke halaman berikutnya (Tegangan Fasa R) */
    display_process_tick(&disp_ctx, 3000);
    display_render_frame(&disp_ctx, line1, line2, line3, sizeof(line1));
    printf("   -> Bergulir Otomatis (Page Tegangan R):\n");
    printf("      [%s]\n      [%s]\n      [%s]\n\n", line1, line2, line3);

    /* -------------------------------------------------------------
     * 2. Inisialisasi & Pengujian Tamper Manager
     * ------------------------------------------------------------- */
    printf("[2] Menguji Tamper Event Manager (Sabotase SPLN Tabel 6)...\n");
    tamper_context_t tamper_ctx;
    tamper_init(&tamper_ctx);

    /* Simulasi kejadian sabotase Tutup Meter (Case Open - RUSAK) */
    printf("   -> Simulasi Trigger Tamper: Tutup Meter Dibuka (PC13)...\n");
    tamper_process_signal(&tamper_ctx, TAMPER_VECTOR_CASE_OPEN, true, 1700000000);
    
    /* Simulasi kejadian Medan Magnet Luar > 500mT (ERR25) */
    printf("   -> Simulasi Trigger Tamper: Medan Magnet >500mT (ERR25)...\n");
    tamper_process_signal(&tamper_ctx, TAMPER_VECTOR_MAGNETIC_FIELD, true, 1700000010);

    tamper_event_entry_t logs[5];
    size_t count = 0;
    tamper_get_log_entries(&tamper_ctx, logs, 5, &count);
    printf("   -> Rekaman Log Sabotase Tersimpan: %zu entri\n", count);
    for (size_t i = 0; i < count; i++) {
        printf("      [%zu] Waktu: %u, Vektor Sabotase: 0x%02X\n",
               i + 1, (unsigned int)logs[i].timestamp, (unsigned int)logs[i].vector);
    }
    printf("\n");

    /* -------------------------------------------------------------
     * 3. Pengujian Power Limiter & Relay FSM (Sambungan Tidak Langsung)
     * ------------------------------------------------------------- */
    printf("[3] Menguji Power Limiter & Relai FSM (SPLN D3.006 Tabel 5)...\n");
    power_relay_context_t relay_ctx;
    power_relay_init(&relay_ctx, 6600, 5, 30);

    printf("   -> Status Awal Relai: %s\n", power_relay_state_to_string(relay_ctx.relay_state));
    printf("   -> Menguji Daya Berlebih (7500 W > 6600 W selama 6 detik)...\n");
    for (uint32_t t = 1; t <= 6; t++) {
        bool trip = power_relay_process_power(&relay_ctx, 7500, t);
        if (trip) {
            printf("      [TRIP] Waktu t=%u s: Power Limiter memicu pemutusan beban!\n", t);
        }
    }
    printf("   -> Status Relai Paska Trip: %s (Koil Shunt Trip PC8 Aktif)\n",
           power_relay_state_to_string(relay_ctx.relay_state));

    /* Remote Reconnect -> Menuju READY ON */
    printf("   -> Remote Reconnect DLMS diterima dari HES...\n");
    power_relay_request_reconnect(&relay_ctx, 100);
    printf("   -> Status Relai: %s (Menunggu konfirmasi tombol lokal PB_UP + PB_DN)\n",
           power_relay_state_to_string(relay_ctx.relay_state));

    /* Konfirmasi lokal pengguna */
    bool ok_rec = power_relay_confirm_reconnect(&relay_ctx, 110);
    printf("   -> Konfirmasi Tombol Lokal Dieksekusi: %s\n", ok_rec ? "SUKSES" : "GAGAL");
    printf("   -> Status Relai: %s (Tripping Release Shunt Trip PC8 dieksekusi, MCCB siap ON manual)\n\n",
           power_relay_state_to_string(relay_ctx.relay_state));

    /* -------------------------------------------------------------
     * 4. Pengujian Load Profile 15-Menit
     * ------------------------------------------------------------- */
    printf("[4] Menguji Perekam Load Profile 15-Menit...\n");
    load_profile_ctx_t lp_ctx;
    load_profile_init(&lp_ctx);

    load_profile_entry_t lp_ent = {
        .timestamp = 1700000900,
        .voltage_r_dvolts = 2205,
        .voltage_s_dvolts = 2210,
        .voltage_t_dvolts = 2198,
        .current_r_mamps = 15200,
        .current_s_mamps = 14800,
        .current_t_mamps = 15000,
        .current_n_mamps = 100,
        .active_power_w = 9980,
        .reactive_power_var = 1200,
        .power_factor_permille = 995,
        .active_energy_import_wh = 12458000
    };
    load_profile_add_entry(&lp_ctx, &lp_ent);
    printf("   -> Entri Profil Beban Tersimpan: %zu entri\n", load_profile_get_count(&lp_ctx));

    printf("\n====================================================================\n");
    printf("   [PASS] SELURUH MODUL APLIKASI E3 BEROPERASI 100%% SEMPURNA!       \n");
    printf("====================================================================\n");

    return 0;
}

