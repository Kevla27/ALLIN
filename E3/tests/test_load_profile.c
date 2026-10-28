/**
 * @file test_load_profile.c
 * @brief Unit Test untuk Modul Profil Beban 15-Menit (Load Profile - ER-02) & Adapter A-XDR DLMS
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "load_profile.h"
#include "load_profile_dlms_adapter.h"

void test_load_profile_basic(void) {
    printf("[TEST] Testing Load Profile Init, Insert, & CRC-16 Checksum...\n");

    load_profile_ctx_t lp_ctx;
    load_profile_init(&lp_ctx);
    assert(load_profile_get_count(&lp_ctx) == 0);

    load_profile_entry_t entry1 = {
        .timestamp = 1727570400, // Sample epoch
        .voltage_r_dvolts = 2200,
        .voltage_s_dvolts = 2205,
        .voltage_t_dvolts = 2198,
        .current_r_mamps = 5000,
        .current_s_mamps = 5020,
        .current_t_mamps = 4980,
        .current_n_mamps = 20,
        .active_power_w = 3300,
        .reactive_power_var = 150,
        .power_factor_permille = 998,
        .active_energy_import_wh = 125000,
        .active_energy_export_wh = 15000,
        .reactive_energy_import_varh = 3000,
        .reactive_energy_export_varh = 500,
        .status_mask = 0
    };

    bool ok = load_profile_add_entry(&lp_ctx, &entry1);
    assert(ok);
    assert(load_profile_get_count(&lp_ctx) == 1);

    load_profile_entry_t read_back;
    ok = load_profile_get_entry(&lp_ctx, 0, &read_back);
    assert(ok);
    assert(read_back.voltage_r_dvolts == 2200);
    assert(load_profile_verify_integrity(&read_back) == true);

    printf("   [PASS] Insert & Integrity Verification OK!\n\n");
}

void test_load_profile_ring_buffer_wrap(void) {
    printf("[TEST] Testing Load Profile Ring Buffer Wrap-Around...\n");

    load_profile_ctx_t lp_ctx;
    load_profile_init(&lp_ctx);

    /* Isi melampaui kapasitas LOAD_PROFILE_MAX_ENTRIES (100 entri) */
    for (size_t i = 0; i < 120; i++) {
        load_profile_entry_t e = {
            .timestamp = 1727570400 + (uint32_t)(i * 900),
            .voltage_r_dvolts = (uint16_t)(2200 + (i % 10))
        };
        load_profile_add_entry(&lp_ctx, &e);
    }

    assert(load_profile_get_count(&lp_ctx) == LOAD_PROFILE_MAX_ENTRIES);

    /* Pastikan entri tertua adalah entri ke-20 (karena 0-19 tertimpa) */
    load_profile_entry_t oldest;
    bool ok = load_profile_get_entry(&lp_ctx, 0, &oldest);
    assert(ok);
    assert(oldest.timestamp == 1727570400 + (20 * 900));

    printf("   Oldest Timestamp: %u\n", oldest.timestamp);
    printf("   [PASS] Ring Buffer Wrap-Around Verified OK!\n\n");
}

void test_max_demand_tracker(void) {
    printf("[TEST] Testing Maximum Demand 15-Minute Tracker & Integration...\n");
    max_demand_tracker_t tracker;
    max_demand_init(&tracker, 10);

    for (int s = 0; s < 10; s++) {
        max_demand_process_second(&tracker, 6000, 1700000000 + s);
    }
    assert(tracker.current_demand_w == 6000);
    assert(tracker.max_demand_w == 6000);
    assert(tracker.max_demand_timestamp == 1700000009);

    for (int s = 0; s < 10; s++) {
        max_demand_process_second(&tracker, 11000, 1700000010 + s);
    }
    assert(tracker.current_demand_w == 11000);
    assert(tracker.max_demand_w == 11000);

    max_demand_reset(&tracker);
    assert(tracker.max_demand_w == 0);
    assert(tracker.cumulative_max_demand_w == 11000);
    assert(tracker.reset_count == 1);
    printf("   [PASS] Maximum Demand Tracker Verified OK!\n\n");
}

void test_billing_snapshot_ring(void) {
    printf("[TEST] Testing Monthly Billing Snapshot 12-Month Buffer & Integrity...\n");
    billing_snapshot_ctx_t b_ctx;
    billing_snapshot_init(&b_ctx);

    for (uint32_t i = 0; i < 20; i++) {
        billing_snapshot_entry_t e = {
            .billing_epoch_sec = 1700000000 + i * 2592000,
            .active_energy_import_wh = (i + 1) * 50000ULL,
            .active_energy_wbp_wh = (i + 1) * 15000ULL,
            .active_energy_lwbp_wh = (i + 1) * 35000ULL,
            .reactive_energy_varh = (i + 1) * 10000ULL,
            .max_demand_w = 4000 + i * 50,
            .max_demand_timestamp = 1700000000 + i * 2592000 + 3600
        };
        bool ok = billing_snapshot_record(&b_ctx, &e);
        assert(ok);
    }

    assert(billing_snapshot_get_count(&b_ctx) == BILLING_MAX_SNAPSHOTS);
    assert(b_ctx.total_snapshots == 20);

    billing_snapshot_entry_t oldest;
    bool ok = billing_snapshot_get_entry(&b_ctx, 0, &oldest);
    assert(ok);
    assert(oldest.billing_epoch_sec == 1700000000 + 8 * 2592000);
    assert(billing_snapshot_verify_integrity(&oldest) == true);

    printf("   [PASS] Billing Snapshot 12-Month Buffer & Integrity Verified OK!\n\n");
}

void test_load_profile_dlms_axdr_encoding(void) {
    printf("[TEST] Testing DLMS A-XDR Structure Encoding & Verification...\n");

    /* 1. Uji Load Profile Entry (11 Elemen Struktur Sempurna) */
    load_profile_entry_t lp_entry = {
        .timestamp = 1727570400,
        .voltage_r_dvolts = 2301,
        .voltage_s_dvolts = 2298,
        .voltage_t_dvolts = 2312,
        .current_r_mamps = 10500,
        .current_s_mamps = 10450,
        .current_t_mamps = 10600,
        .current_n_mamps = 80,
        .active_power_w = 7200,
        .reactive_power_var = 450,
        .power_factor_permille = 995,
        .active_energy_import_wh = 543210,
        .active_energy_export_wh = 12345,
        .reactive_energy_import_varh = 6789,
        .reactive_energy_export_varh = 100,
        .status_mask = 0x00000001
    };

    uint8_t lp_buf[128];
    memset(lp_buf, 0, sizeof(lp_buf));

    /* Validasi penolakan buffer terlalu kecil (< 72 byte) */
    size_t sz_small = load_profile_encode_dlms_entry(&lp_entry, lp_buf, 70);
    assert(sz_small == 0);

    /* Validasi encoding sukses */
    size_t sz_lp = load_profile_encode_dlms_entry(&lp_entry, lp_buf, sizeof(lp_buf));
    assert(sz_lp == 72);
    assert(lp_buf[0] == DLMS_AXDR_TAG_STRUCTURE);
    assert(lp_buf[1] == 11); /* Tepat 11 elemen, TIDAK ADA underflow */

    /* Verifikasi dengan decoder validator */
    bool valid_struct = dlms_decode_verify_structure(lp_buf, sz_lp, 11);
    assert(valid_struct == true);
    printf("   -> Load Profile A-XDR 11-Element Structure: %zu bytes (Valid Structure & Non-Truncated)\n", sz_lp);

    /* 2. Uji Tamper Log Entry (3 Elemen Struktur Sempurna) */
    tamper_event_entry_t tamp_entry = {
        .timestamp = 1727570500,
        .vector = TAMPER_VECTOR_TERMINAL_OPEN,
        .is_asserted = true,
        .duration_sec = 15,
        .active_mask = 0x00000002
    };

    uint8_t tamp_buf[64];
    memset(tamp_buf, 0, sizeof(tamp_buf));

    /* Buffer terlalu kecil (< 24 byte) */
    size_t tamp_small = tamper_log_encode_dlms_entry(&tamp_entry, tamp_buf, 20);
    assert(tamp_small == 0);

    size_t sz_tamp = tamper_log_encode_dlms_entry(&tamp_entry, tamp_buf, sizeof(tamp_buf));
    assert(sz_tamp == 24);
    assert(tamp_buf[0] == DLMS_AXDR_TAG_STRUCTURE);
    assert(tamp_buf[1] == 3); /* Tepat 3 elemen, telah diperbaiki dari '4' */

    valid_struct = dlms_decode_verify_structure(tamp_buf, sz_tamp, 3);
    assert(valid_struct == true);
    printf("   -> Tamper Log A-XDR 3-Element Structure: %zu bytes (Corrected tag 3, Verified OK)\n", sz_tamp);

    /* 3. Uji SPLN Standard Log Entry (4 Elemen Sesuai SPLN Butir 12.3) */
    spln_standard_log_entry_t spln_entry = {
        .timestamp_epoch = 1727570600,
        .event_code = SPLN_LOG_TAMPER_CASE_OPEN,
        .status_flags = 1,
        .parameter_val = 0x12345678
    };

    uint8_t spln_buf[64];
    memset(spln_buf, 0, sizeof(spln_buf));

    /* Buffer terlalu kecil (< 27 byte) */
    size_t spln_small = spln_log_encode_dlms_entry(&spln_entry, spln_buf, 25);
    assert(spln_small == 0);

    size_t sz_spln = spln_log_encode_dlms_entry(&spln_entry, spln_buf, sizeof(spln_buf));
    assert(sz_spln == 27);
    assert(spln_buf[0] == DLMS_AXDR_TAG_STRUCTURE);
    assert(spln_buf[1] == 4); /* Tepat 4 elemen */

    valid_struct = dlms_decode_verify_structure(spln_buf, sz_spln, 4);
    assert(valid_struct == true);
    printf("   -> SPLN Standard Log A-XDR 4-Element Structure: %zu bytes (Verified OK)\n", sz_spln);

    /* 4. Uji Validator Kekebalan Terhadap Malformed Stream */
    assert(dlms_decode_verify_structure(NULL, 10, 3) == false);
    assert(dlms_decode_verify_structure(lp_buf, 10, 11) == false); /* Stream terpotong */
    assert(dlms_decode_verify_structure(lp_buf, sz_lp, 10) == false); /* Elemen mismatch */

    /* 5. Uji Kekebalan Y2K38 (Pasca-2038: Tahun 2050 & Tahun 2100) pada dlms_encode_datetime */
    uint8_t dt_buf[12];
    dlms_encode_datetime(2524608000UL, dt_buf); /* 01-01-2050 00:00:00 */
    uint16_t yr_2050 = ((uint16_t)dt_buf[0] << 8) | dt_buf[1];
    assert(yr_2050 == 2050U && dt_buf[2] == 1U && dt_buf[3] == 1U);

    dlms_encode_datetime(4102444800UL, dt_buf); /* 01-01-2100 00:00:00 */
    uint16_t yr_2100 = ((uint16_t)dt_buf[0] << 8) | dt_buf[1];
    assert(yr_2100 == 2100U && dt_buf[2] == 1U && dt_buf[3] == 1U);
    printf("   -> Y2K38 Immunity Verified (2050 & 2100 A-XDR DateTime OK)\n");

    printf("   [PASS] DLMS A-XDR Structure Encoding & Verification Verified OK!\n\n");
}

int main(void) {
    printf("====================================================\n");
    printf(" RUNNING LOAD PROFILE & A-XDR UNIT TESTS (ER-02/E3) \n");
    printf("====================================================\n\n");

    test_load_profile_basic();
    test_load_profile_ring_buffer_wrap();
    test_max_demand_tracker();
    test_billing_snapshot_ring();
    test_load_profile_dlms_axdr_encoding();

    printf("====================================================\n");
    printf(" ALL LOAD PROFILE & A-XDR TESTS PASSED 100%%!         \n");
    printf("====================================================\n");
    return 0;
}
