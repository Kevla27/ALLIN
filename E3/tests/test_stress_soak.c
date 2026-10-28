/**
 * @file test_stress_soak.c
 * @brief Pengujian Ketahanan Jangka Panjang & Beban Ekstrem (Stress & Soak Test)
 * 
 * Skenario Pengujian:
 * - Skenario A: 90 Hari Load Profiling Non-Stop (8.640 Snapshot 15-Menit) & Akumulasi Energi 64-bit
 * - Skenario B: Badai Sabotase Ekstrem (Tamper Flood 1.000 Event Berturut-turut) & Validasi A-XDR
 * - Skenario C: 500x Siklus Pemadaman Mendadak (PVD Brownout Failover & Offline Tamper Recovery)
 * - Skenario D: Audit Integritas Memori, Boundary Limit, & Ketahanan Null Pointer
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "load_profile.h"
#include "load_profile_dlms_adapter.h"
#include "nvram_storage.h"
#include "dlms_types.h"
#include "dlms_apdu.h"
#include "dlms_tamper_log.h"
#include "tamper_manager.h"
#include "bsp_pvd.h"
#include "bsp_rtc_tamp.h"
#include "bsp_ade9000_irq.h"

#define SECONDS_PER_15_MIN  900U
#define SNAPSHOTS_PER_DAY   96U
#define SIMULATION_DAYS     90U
#define TOTAL_SNAPSHOTS     (SIMULATION_DAYS * SNAPSHOTS_PER_DAY) /* 8.640 Snapshot */

/* --- Skenario A: 90 Hari Continuous Load Profiling (8.640 Snapshot) --- */
void test_scenario_a_90_day_soak_load_profile(void) {
    printf("[SKENARIO A] Memulai Uji Rendam (Soak Test) Profil Beban 90 Hari (%u Snapshot 15-Menitan)...\n",
           TOTAL_SNAPSHOTS);

    load_profile_ctx_t lp_ctx;
    load_profile_init(&lp_ctx);
    assert(load_profile_get_count(&lp_ctx) == 0);

    /* Baseline Energi Awal: 1.000 kWh = 1.000.000 Wh */
    uint64_t simulated_energy_import_wh = 1000000ULL;
    uint64_t simulated_energy_export_wh = 50000ULL;
    uint32_t base_epoch = 1727570400U; /* 29 September 2026 */

    clock_t start_time = clock();

    for (uint32_t i = 0; i < TOTAL_SNAPSHOTS; i++) {
        uint32_t current_epoch = base_epoch + (i * SECONDS_PER_15_MIN);
        uint32_t hour_of_day = (i % SNAPSHOTS_PER_DAY) / 4;

        /* Profil Beban Dinamis Harian Realistis (Pagi, Siang, Puncak Malam, Tengah Malam) */
        int32_t active_w;
        if (hour_of_day >= 17 && hour_of_day <= 22) {
            active_w = 8500 + (int32_t)(i % 500);  /* Beban Puncak Malam: ~8.5 kW - 9.0 kW */
        } else if (hour_of_day >= 10 && hour_of_day < 17) {
            active_w = 4200 + (int32_t)(i % 300);  /* Siang Hari: ~4.2 kW - 4.5 kW */
        } else if (hour_of_day >= 6 && hour_of_day < 10) {
            active_w = 2800 + (int32_t)(i % 200);  /* Pagi Hari: ~2.8 kW - 3.0 kW */
        } else {
            active_w = 1200 + (int32_t)(i % 100);  /* Tengah Malam: ~1.2 kW - 1.3 kW */
        }

        /* Akumulasi Energi Wh per 15 Menit: P (W) * 900s / 3600s = P / 4 */
        uint32_t delta_wh = (uint32_t)(active_w / 4);
        simulated_energy_import_wh += delta_wh;

        /* 3-Fasa Tegangan & Arus Realistis */
        load_profile_entry_t entry = {
            .timestamp = current_epoch,
            .voltage_r_dvolts = (uint16_t)(2200 + (i % 30)),
            .voltage_s_dvolts = (uint16_t)(2205 + (i % 25)),
            .voltage_t_dvolts = (uint16_t)(2195 + (i % 35)),
            .current_r_mamps = (uint32_t)(active_w * 1000 / 3 / 220),
            .current_s_mamps = (uint32_t)(active_w * 1000 / 3 / 220),
            .current_t_mamps = (uint32_t)(active_w * 1000 / 3 / 220),
            .current_n_mamps = 25,
            .active_power_w = active_w,
            .reactive_power_var = (int32_t)(active_w * 0.2),
            .power_factor_permille = 980,
            .active_energy_import_wh = simulated_energy_import_wh,
            .active_energy_export_wh = simulated_energy_export_wh,
            .reactive_energy_import_varh = simulated_energy_import_wh / 5,
            .reactive_energy_export_varh = 0,
            .status_mask = (i % 500 == 0) ? 0x00000001U : 0x00000000U
        };

        /* Simpan ke Ring Buffer RAM */
        bool ok = load_profile_add_entry(&lp_ctx, &entry);
        assert(ok);

        /* Simpan ke NVRAM Flash Ring Buffer */
        ok = nvram_write_load_profile_entry(i % LOAD_PROFILE_MAX_ENTRIES, &entry);
        assert(ok);
    }

    clock_t end_time = clock();
    double duration_ms = (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC;

    /* 1. Verifikasi Kapasitas dan Counter Total */
    assert(load_profile_get_count(&lp_ctx) == LOAD_PROFILE_MAX_ENTRIES);
    assert(lp_ctx.total_written == TOTAL_SNAPSHOTS);

    /* Ring buffer kapasitas 100 telah berputar 86 kali penuh */
    printf("   -> 8.640 snapshot berhasil disimulasikan dalam %.2f ms (Wrap-around 86x).\n", duration_ms);

    /* 2. Verifikasi Entri Tertua (Oldest) dan Terbaru (Newest) */
    load_profile_entry_t oldest, newest;
    assert(load_profile_get_entry(&lp_ctx, 0, &oldest) == true);
    assert(load_profile_get_entry(&lp_ctx, LOAD_PROFILE_MAX_ENTRIES - 1, &newest) == true);

    uint32_t expected_oldest_epoch = base_epoch + ((TOTAL_SNAPSHOTS - LOAD_PROFILE_MAX_ENTRIES) * SECONDS_PER_15_MIN);
    uint32_t expected_newest_epoch = base_epoch + ((TOTAL_SNAPSHOTS - 1) * SECONDS_PER_15_MIN);

    assert(oldest.timestamp == expected_oldest_epoch);
    assert(newest.timestamp == expected_newest_epoch);
    assert(newest.active_energy_import_wh > oldest.active_energy_import_wh);

    /* 3. Verifikasi Integritas CRC-16 Pada Seluruh 100 Entri yang Ada */
    for (size_t idx = 0; idx < LOAD_PROFILE_MAX_ENTRIES; idx++) {
        load_profile_entry_t e;
        assert(load_profile_get_entry(&lp_ctx, idx, &e) == true);
        assert(load_profile_verify_integrity(&e) == true);

        /* Slot fisik di NVRAM Flash bersesuaian dengan (head + idx) % LOAD_PROFILE_MAX_ENTRIES */
        size_t phys_slot = (lp_ctx.head + idx) % LOAD_PROFILE_MAX_ENTRIES;
        load_profile_entry_t flash_e;
        assert(nvram_read_load_profile_entry((uint32_t)phys_slot, &flash_e) == true);
        assert(load_profile_verify_integrity(&flash_e) == true);
        assert(flash_e.crc16 == e.crc16);
        assert(flash_e.timestamp == e.timestamp);
    }

    printf("   -> Integritas CRC-16 & NVRAM Flash 100%% Valid pada seluruh entri.\n");
    printf("   -> Total Energi Akumulasi: %llu Wh (%.2f kWh) tanpa integer overflow.\n",
           (unsigned long long)newest.active_energy_import_wh,
           (double)newest.active_energy_import_wh / 1000.0);
    printf("   [PASS] Skenario A (90-Day Soak Test) LULUS 100%%!\n\n");
}

/* --- Skenario B: Badai Sabotase Ekstrem (Tamper Flood 1.000 Event Berturut-turut) --- */
void test_scenario_b_tamper_flood_stress(void) {
    printf("[SKENARIO B] Memulai Uji Badai Sabotase (Tamper Flood 1.000 Event Simultan)...\n");

    dlms_tamper_log_t log;
    assert(dlms_tamper_log_init(&log) == DLMS_OK);

    const dlms_tamper_code_t sample_codes[] = {
        DLMS_TAMPER_TERMINAL_COVER_OPEN,
        DLMS_TAMPER_METER_COVER_OPEN,
        DLMS_TAMPER_MAGNETIC_INDUCTION,
        DLMS_TAMPER_REVERSE_CURRENT,
        DLMS_TAMPER_MISSING_NEUTRAL,
        DLMS_TAMPER_WRONG_PHASE_SEQ,
        DLMS_TAMPER_MISSING_VOLTAGE
    };
    const size_t num_codes = sizeof(sample_codes) / sizeof(sample_codes[0]);

    uint32_t flood_base_epoch = 1727600000U;
    clock_t start_time = clock();

    /* Injeksi 1.000 event tamper berurutan */
    for (uint32_t i = 0; i < 1000; i++) {
        dlms_tamper_code_t code = sample_codes[i % num_codes];
        uint8_t status = (uint8_t)((i % 2 == 0) ? 1 : 0); /* Bergantian Terjadi & Pulih */
        uint32_t ts = flood_base_epoch + i;

        dlms_result_t res = dlms_tamper_log_add_event(&log, ts, code, status);
        assert(res == DLMS_OK);
    }

    clock_t end_time = clock();
    double duration_ms = (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC;

    /* 1. Verifikasi Kapasitas Maksimum dan Total Counter */
    assert(log.entries_in_use == DLMS_TAMPER_LOG_MAX_ENTRIES); /* Maks 30 entri */
    assert(log.total_events == 1000);
    printf("   -> 1.000 event diproses dalam %.2f ms tanpa buffer overflow.\n", duration_ms);

    /* 2. Verifikasi Entri Paling Baru (Index 0) dan Paling Lama (Index 29) */
    dlms_tamper_record_t newest_rec, oldest_rec;
    assert(dlms_tamper_log_get_entry(&log, 0, &newest_rec) == DLMS_OK);
    assert(dlms_tamper_log_get_entry(&log, 29, &oldest_rec) == DLMS_OK);

    assert(newest_rec.timestamp_epoch == flood_base_epoch + 999);
    assert(oldest_rec.timestamp_epoch == flood_base_epoch + (1000 - 30));

    /* 3. Validasi Serialisasi A-XDR (DLMS Class 7 Profile Generic Response) */
    uint8_t axdr_buffer[512];
    memset(axdr_buffer, 0, sizeof(axdr_buffer));
    size_t encoded_bytes = dlms_tamper_log_encode_axdr(&log, axdr_buffer, sizeof(axdr_buffer));

    /* Tag (1 byte) + Length (1 byte) + 30 entri * 6 byte = 182 bytes */
    size_t expected_bytes = 1 + 1 + (DLMS_TAMPER_LOG_MAX_ENTRIES * 6);
    assert(encoded_bytes == expected_bytes);
    assert(axdr_buffer[0] == DLMS_DATA_TYPE_OCTET_STRING);
    assert(axdr_buffer[1] == DLMS_TAMPER_LOG_MAX_ENTRIES);

    /* 4. Validasi Ketahanan Boundary Buffer A-XDR (Kapasitas Kurang) */
    uint8_t small_buf[50];
    size_t truncated_len = dlms_tamper_log_encode_axdr(&log, small_buf, sizeof(small_buf));
    assert(truncated_len <= sizeof(small_buf)); /* Harus mencegah buffer overrun */

    printf("   -> Serialisasi A-XDR: %zu bytes (Valid Format, Boundary Guard OK).\n", encoded_bytes);
    printf("   [PASS] Skenario B (Tamper Flood 1.000 Event) LULUS 100%%!\n\n");
}

/* --- Skenario C: 500x Pemadaman Mendadak (PVD Failover Cycle & Offline Tamper) --- */
void test_scenario_c_500x_pvd_power_loss_and_offline_tamper(void) {
    printf("[SKENARIO C] Memulai Uji 500x Siklus Pemadaman Listrik & Pemulihan (PVD Failover)...\n");

    bsp_pvd_init();
    bsp_rtc_tamp_init();

    tamper_context_t tamper_ctx;
    tamper_init(&tamper_ctx);

    uint32_t offline_tamper_count = 0;
    clock_t start_time = clock();

    for (uint32_t cycle = 0; cycle < 500; cycle++) {
        /* 1. Operasi Normal: Tambah event tamper ke konteks */
        tamper_process_signal(&tamper_ctx, TAMPER_VECTOR_MAGNETIC_FIELD, true, 1727700000U + cycle);

        /* 2. Tegangan Drop -> PVD Interupsi (Last-Gasp Sequence) */
        bsp_pvd_simulate_power_fail();
        assert(bsp_pvd_is_power_failed() == true);

        /* 3. Simpan Snapshot ke NVRAM Flash saat Last-Gasp */
        bool saved = nvram_save_tamper_log_snapshot(&tamper_ctx);
        assert(saved);

        /* 4. Kondisi Listrik Padam Total (VDD = 0V, VBAT Aktif) */
        /* Setiap kelipatan 10 siklus, terjadi sabotase fisik buka tutup meter saat padam */
        bool inject_offline = (cycle % 10 == 0);
        if (inject_offline) {
            bsp_rtc_tamp_simulate_offline_tamper(DLMS_TAMPER_METER_COVER_OPEN, 1727700000U + cycle + 50);
            offline_tamper_count++;
        }

        /* 5. Listrik Menyala Kembali (Power-On Reset / Booting Sequence) */
        bsp_pvd_simulate_power_restore();
        assert(bsp_pvd_is_power_failed() == false);

        bsp_rtc_tamp_init();

        /* 6. Periksa & Pulihkan Data dari NVRAM Flash */
        tamper_context_t restored_ctx;
        bool loaded = nvram_load_tamper_log_snapshot(&restored_ctx);
        assert(loaded);
        assert(restored_ctx.total_event_count == tamper_ctx.total_event_count);

        /* 7. Periksa Kejadian Offline Tamper */
        bsp_offline_tamper_info_t offline_info;
        bool has_offline = bsp_rtc_tamp_check_offline_tamper(&offline_info);

        if (inject_offline) {
            assert(has_offline == true);
            assert(offline_info.tamper_code == DLMS_TAMPER_METER_COVER_OPEN);
            assert(offline_info.occurred == true);

            /* Proses event dan bersihkan flag */
            bsp_rtc_tamp_process_offline_events();

            /* Pastikan flag telah bersih (single-shot recovery) */
            assert(bsp_rtc_tamp_check_offline_tamper(&offline_info) == false);
        } else {
            assert(has_offline == false);
        }
    }

    clock_t end_time = clock();
    double duration_ms = (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC;

    /* Verifikasi total offline tamper di register akumulator BKP */
    uint32_t bkp_counter = bsp_rtc_tamp_get_backup_reg(TAMP_BKP_REG_COUNTER);
    assert(bkp_counter == offline_tamper_count);
    assert(offline_tamper_count == 50);

    printf("   -> 500x siklus PVD & failover boot selesai dalam %.2f ms.\n", duration_ms);
    printf("   -> 50x kejadian offline tamper terdeteksi presisi & single-shot cleared.\n");
    printf("   [PASS] Skenario C (500x Power-Loss & Offline Tamper) LULUS 100%%!\n\n");
}

/* --- Skenario D: Audit Deterministik Memori, Boundary, & Null-Safety --- */
void test_scenario_d_memory_boundary_and_null_safety(void) {
    printf("[SKENARIO D] Memulai Audit Deterministik Memori, Batas Array, & Null-Safety...\n");

    /* 1. Audit Ukuran Struktur Data (Compact Memory Alignment) */
    printf("   Ukuran load_profile_entry_t: %zu bytes\n", sizeof(load_profile_entry_t));
    printf("   Ukuran dlms_tamper_record_t: %zu bytes\n", sizeof(dlms_tamper_record_t));
    printf("   Ukuran dlms_tamper_log_t   : %zu bytes\n", sizeof(dlms_tamper_log_t));

    assert(sizeof(load_profile_entry_t) <= 128); /* Tetap hemat memori RAM */
    assert(sizeof(dlms_tamper_record_t) <= 16);

    /* 2. Uji Boundary Array Load Profile */
    load_profile_ctx_t lp_ctx;
    load_profile_init(&lp_ctx);
    load_profile_entry_t dummy_out;

    assert(load_profile_get_entry(&lp_ctx, 0, &dummy_out) == false);
    assert(load_profile_get_entry(&lp_ctx, LOAD_PROFILE_MAX_ENTRIES, &dummy_out) == false);
    assert(load_profile_get_entry(&lp_ctx, 9999, &dummy_out) == false);

    /* 3. Uji Boundary Array Tamper Log */
    dlms_tamper_log_t log;
    dlms_tamper_log_init(&log);
    dlms_tamper_record_t rec_out;

    assert(dlms_tamper_log_get_entry(&log, 0, &rec_out) == DLMS_ERR_UNAUTHORIZED);
    assert(dlms_tamper_log_get_entry(&log, DLMS_TAMPER_LOG_MAX_ENTRIES, &rec_out) == DLMS_ERR_UNAUTHORIZED);
    assert(dlms_tamper_log_get_entry(&log, 100, &rec_out) == DLMS_ERR_UNAUTHORIZED);

    /* 4. Uji Null Pointer Protection pada Seluruh API Kritis */
    assert(load_profile_add_entry(NULL, &dummy_out) == false);
    assert(load_profile_add_entry(&lp_ctx, NULL) == false);
    assert(load_profile_get_entry(NULL, 0, &dummy_out) == false);
    assert(load_profile_get_entry(&lp_ctx, 0, NULL) == false);
    assert(load_profile_verify_integrity(NULL) == false);
    assert(load_profile_calc_crc16(NULL) == 0);

    assert(dlms_tamper_log_init(NULL) == DLMS_ERR_NULL_PTR);
    assert(dlms_tamper_log_add_event(NULL, 0, DLMS_TAMPER_NONE, 0) == DLMS_ERR_NULL_PTR);
    assert(dlms_tamper_log_get_entry(NULL, 0, &rec_out) == DLMS_ERR_NULL_PTR);
    assert(dlms_tamper_log_get_entry(&log, 0, NULL) == DLMS_ERR_NULL_PTR);
    assert(dlms_tamper_log_encode_axdr(NULL, NULL, 0) == 0);

    assert(nvram_write_load_profile_entry(0, NULL) == false);
    assert(nvram_read_load_profile_entry(0, NULL) == false);
    assert(nvram_save_tamper_log_snapshot(NULL) == false);
    assert(nvram_load_tamper_log_snapshot(NULL) == false);

    assert(bsp_rtc_tamp_check_offline_tamper(NULL) == false);

    printf("   [PASS] Seluruh Boundary Check & Null Pointer Protection Terbukti Aman 100%%!\n\n");
}

int main(void) {
    printf("======================================================================\n");
    printf("     RUNNING SUITE: LONG-TERM SOAK & TAMPER FLOOD STRESS TEST         \n");
    printf("     Standar Kepatuhan: SPLN D3.006: 2021 & IEC 62056 DLMS/COSEM      \n");
    printf("======================================================================\n\n");

    test_scenario_a_90_day_soak_load_profile();
    test_scenario_b_tamper_flood_stress();
    test_scenario_c_500x_pvd_power_loss_and_offline_tamper();
    test_scenario_d_memory_boundary_and_null_safety();

    printf("======================================================================\n");
    printf(" [HASIL] SELURUH 4 SKENARIO STRESS & SOAK TEST LULUS 100%% TANPA CACAT!  \n");
    printf("======================================================================\n");
    return 0;
}
