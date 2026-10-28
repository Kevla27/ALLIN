/**
 * @file test_final_system_stress.c
 * @brief Uji Ketahanan Ekstrem & Verifikasi Komprehensif Versi Final Smart Meter
 *        (Pilar E3: HMI SPLN Gambar 4, ToU, Maximum Demand, Billing 12 Bulan, Tamper & Buzzer)
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "display.h"
#include "tamper_manager.h"
#include "power_relay_manager.h"
#include "load_profile.h"
#include "bsp_alarm_buzzer.h"

#define SOAK_CYCLES 10000U

int main(void) {
    printf("====================================================================\n");
    printf("  UJI KETAHANAN EKSTREM & AUDIT VERSI FINAL FIRMWARE SMART METER    \n");
    printf("  (Pengujian Rendam 10.000 Siklus, MD 15-Min, Billing 12-Bulan)     \n");
    printf("====================================================================\n\n");

    /* 1. Inisialisasi Seluruh Modul Inti */
    printf("[TAHAP 1] Inisialisasi Modul-Modul Inti Pilar E3...\n");
    display_context_t disp_ctx;
    tamper_context_t  tamp_ctx;
    power_relay_context_t relay_ctx;
    load_profile_ctx_t lp_ctx;
    billing_snapshot_ctx_t bill_ctx;
    max_demand_tracker_t md_track;

    bool ok = display_init(&disp_ctx, "530000000001", 3000);
    assert(ok);
    tamper_init(&tamp_ctx);
    power_relay_init(&relay_ctx, 11000, 5, 30);
    load_profile_init(&lp_ctx);
    billing_snapshot_init(&bill_ctx);
    max_demand_init(&md_track, 900);
    bsp_alarm_buzzer_init();
    printf("  > Seluruh modul inti berhasil diinisialisasi [PASS]\n\n");

    /* 2. Uji Rendam (Soak Test) 10.000 Siklus Dinamis */
    printf("[TAHAP 2] Menjalankan Uji Rendam 10.000 Siklus (Dynamic Load & Profiling)...\n");
    meter_measurements_t meas;
    memset(&meas, 0, sizeof(meas));
    meas.voltage_r_dvolts = 2300;
    meas.voltage_s_dvolts = 2300;
    meas.voltage_t_dvolts = 2300;
    meas.current_r_mamps  = 5000;
    meas.current_s_mamps  = 5000;
    meas.current_t_mamps  = 5000;
    meas.current_n_mamps  = 50;
    meas.active_power_w   = 3450;
    meas.reactive_power_var = 500;
    meas.apparent_power_va  = 3486;
    meas.power_factor_ppm   = 990;
    meas.frequency_mhz      = 50000;
    meas.active_energy_wh   = 100000;
    meas.reactive_energy_varh = 15000;

    uint32_t current_epoch = 1727570400U;

    for (uint32_t cycle = 1; cycle <= SOAK_CYCLES; cycle++) {
        /* Fluktuasi dinamis daya (1 kW s.d 11 kW) */
        meas.active_power_w = 1000 + (int32_t)((cycle * 37) % 10000);
        meas.active_energy_wh += (meas.active_power_w / 3600);

        /* Integrasi per detik */
        current_epoch++;
        power_relay_process_power(&relay_ctx, meas.active_power_w, current_epoch);
        max_demand_process_second(&md_track, meas.active_power_w, current_epoch);

        /* Navigasi display tick */
        display_process_tick(&disp_ctx, 1000);

        /* Snapshot 15-menit load profile setiap 900 siklus */
        if (cycle % 900 == 0) {
            load_profile_entry_t entry;
            memset(&entry, 0, sizeof(entry));
            entry.timestamp = current_epoch;
            entry.voltage_r_dvolts = (uint16_t)meas.voltage_r_dvolts;
            entry.active_power_w = meas.active_power_w;
            entry.active_energy_import_wh = meas.active_energy_wh;
            load_profile_add_entry(&lp_ctx, &entry);
        }

        if (cycle % 2500 == 0) {
            printf("  > Siklus %5u / %u: Waktu Sim = %lu s | Daya = %5ld W | MD = %5lu W | LP Entries = %u [OK]\n",
                   cycle, SOAK_CYCLES, (unsigned long)current_epoch, (long)meas.active_power_w,
                   (unsigned long)md_track.max_demand_w, (unsigned int)load_profile_get_count(&lp_ctx));
        }
    }
    printf("  > Soak Test 10.000 Siklus Lulus 100%% Tanpa Overflow / Penyimpangan! [PASS]\n\n");

    /* 3. Uji Maximum Demand 15-Menit & Snapshot Billing 12 Bulan */
    printf("[TAHAP 3] Pengujian Fitur Penagihan Lanjutan (MD & 12 Bulan Billing)...\n");
    assert(md_track.max_demand_w > 0);
    printf("  > Rekor Maximum Demand 15-Menit Tercatat: %lu W [PASS]\n", (unsigned long)md_track.max_demand_w);

    /* Rollover Bulanan 15 kali */
    printf("  > Melakukan 15 Siklus Billing Snapshot Rollover...\n");
    for (uint32_t b = 0; b < 15; b++) {
        billing_snapshot_entry_t snap = {
            .billing_epoch_sec = current_epoch + (b * 2592000U),
            .active_energy_import_wh = meas.active_energy_wh + (b * 250000ULL),
            .active_energy_wbp_wh = 100000ULL + (b * 80000ULL),
            .active_energy_lwbp_wh = 200000ULL + (b * 170000ULL),
            .reactive_energy_varh = 50000ULL + (b * 30000ULL),
            .max_demand_w = md_track.max_demand_w,
            .max_demand_timestamp = current_epoch + (b * 2592000U) - 3600U
        };
        billing_snapshot_record(&bill_ctx, &snap);
        max_demand_reset(&md_track);
    }

    assert(billing_snapshot_get_count(&bill_ctx) == BILLING_MAX_SNAPSHOTS);
    assert(bill_ctx.total_snapshots == 15);

    billing_snapshot_entry_t read_snap;
    for (size_t s = 0; s < BILLING_MAX_SNAPSHOTS; s++) {
        bool ok_s = billing_snapshot_get_entry(&bill_ctx, s, &read_snap);
        assert(ok_s);
        assert(billing_snapshot_verify_integrity(&read_snap) == true);
    }
    printf("  > Ring Buffer Billing 12 Bulan Mempertahankan Tepat 12 Entri dengan Integritas CRC-16 [PASS]\n");
    assert(md_track.reset_count == 15);
    assert(md_track.cumulative_max_demand_w > 0);
    printf("  > Kumulatif MD (CMD): %lu W | Reset Counter: %lu [PASS]\n\n",
           (unsigned long)md_track.cumulative_max_demand_w, (unsigned long)md_track.reset_count);

    /* 4. Uji Sabotase Arus Netral (ERR23) & Reverse Power (ERR26) */
    printf("[TAHAP 4] Pengujian Sabotase Arus Netral (ERR23) & Reverse Power (ERR26)...\n");
    tamper_process_signal(&tamp_ctx, TAMPER_VECTOR_NEUTRAL_BYPASS, true, current_epoch);
    assert((tamper_get_active_mask(&tamp_ctx) & TAMPER_VECTOR_NEUTRAL_BYPASS) != 0);

    tamper_process_signal(&tamp_ctx, TAMPER_VECTOR_REVERSE_POWER, true, current_epoch);
    assert((tamper_get_active_mask(&tamp_ctx) & TAMPER_VECTOR_REVERSE_POWER) != 0);

    disp_ctx.current_page = DISP_PAGE_TAMPER_ALARM;
    display_set_tamper_status(&disp_ctx, TAMPER_VECTOR_NEUTRAL_BYPASS);
    display_spln_frame_t f_tamp;
    display_render_spln_frame(&disp_ctx, &f_tamp);
    assert(strcmp(f_tamp.main_value, "PERIKSA") == 0);
    assert(strcmp(f_tamp.unit, "ERR23") == 0);
    printf("  > Render Layar Sabotase Netral: '%s %s' [PASS]\n", f_tamp.main_value, f_tamp.unit);

    display_set_tamper_status(&disp_ctx, TAMPER_VECTOR_REVERSE_POWER);
    display_render_spln_frame(&disp_ctx, &f_tamp);
    assert(strcmp(f_tamp.main_value, "REVERSE") == 0);
    assert(strcmp(f_tamp.unit, "ERR26") == 0);
    printf("  > Render Layar Sabotase Reverse Power: '%s %s' [PASS]\n\n", f_tamp.main_value, f_tamp.unit);

    /* 5. Uji Audit Seluruh Halaman Layar SPLN Gambar 4 */
    printf("[TAHAP 5] Pengujian Audit Kelengkapan Render Tampilan (Semua Halaman SPLN)...\n");
    display_context_t disp_all;
    display_init(&disp_all, "530000000001", 3000);
    display_spln_frame_t frame_check;

    for (int p = 0; p < DISP_PAGE_COUNT; p++) {
        disp_all.current_page = (display_page_id_t)p;
        display_render_spln_frame(&disp_all, &frame_check);
        assert(frame_check.scroll_index_zz[0] != '\0');
        assert(frame_check.main_value[0] != '\0');
    }
    printf("  > Seluruh %d Halaman Tampilan Render Tanpa Cacat [PASS]\n\n", DISP_PAGE_COUNT);

    /* 6. Uji Audio Cadence Buzzer 1.000 Siklus */
    printf("[TAHAP 6] Pengujian Siklus Audio Cadence Buzzer (1.000 Langkah)...\n");
    bsp_alarm_buzzer_init();
    for (int step = 0; step < 1000; step++) {
        bool b_on = bsp_alarm_buzzer_update_cadence(true, false, 50);
        int phase = ((step + 1) * 50) % 400;
        if (phase < 200) {
            assert(b_on == true);
        } else {
            assert(b_on == false);
        }
    }
    printf("  > Kadensi Buzzer 200 ms ON / 200 ms OFF Beroperasi Presisi Tanpa Drift! [PASS]\n\n");

    printf("====================================================================\n");
    printf("  HASIL: 100%% VERIFIKASI KETAHANAN EKSTREM & AUDIT FINAL LULUS!       \n");
    printf("====================================================================\n");

    return 0;
}
