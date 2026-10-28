/**
 * @file test_nvram_storage.c
 * @brief Unit Test untuk Pengujian Penyimpanan Memori NVRAM Flash STM32U575
 */

#include <stdio.h>
#include <assert.h>
#include "nvram_storage.h"

void test_nvram_load_profile_write_read(void) {
    printf("[TEST] Testing NVRAM Load Profile Storage Write/Read...\n");

    load_profile_entry_t entry_to_write = {
        .timestamp = 1727570400,
        .active_energy_import_wh = 50000,
        .voltage_r_dvolts = 2215,
        .current_r_mamps = 5230,
        .status_mask = 0x00000000,
        .crc16 = 0xF8E1
    };

    bool write_ok = nvram_write_load_profile_entry(0, &entry_to_write);
    assert(write_ok == true);

    load_profile_entry_t entry_read;
    bool read_ok = nvram_read_load_profile_entry(0, &entry_read);
    assert(read_ok == true);
    assert(entry_read.timestamp == 1727570400);
    assert(entry_read.active_energy_import_wh == 50000);
    assert(entry_read.voltage_r_dvolts == 2215);
    assert(entry_read.current_r_mamps == 5230);
    assert(entry_read.crc16 == 0xF8E1);

    bool invalid_write = nvram_write_load_profile_entry(LOAD_PROFILE_MAX_ENTRIES, &entry_to_write);
    assert(invalid_write == false);
    assert(nvram_write_load_profile_entry(0, NULL) == false);
    assert(nvram_read_load_profile_entry(0, NULL) == false);
    assert(nvram_read_load_profile_entry(LOAD_PROFILE_MAX_ENTRIES, &entry_read) == false);
    assert(nvram_save_tamper_log_snapshot(NULL) == false);
    assert(nvram_load_tamper_log_snapshot(NULL) == false);
    assert(nvram_save_energy_checkpoint(NULL) == false);
    assert(nvram_load_energy_checkpoint(NULL) == false);

    printf("   [PASS] NVRAM Load Profile Storage Read/Write Valid!\n\n");
}

void test_nvram_tamper_log_snapshot(void) {
    printf("[TEST] Testing NVRAM Tamper Log Snapshot Save/Load...\n");

    tamper_context_t tamper_ctx;
    tamper_init(&tamper_ctx);
    tamper_process_signal(&tamper_ctx, TAMPER_VECTOR_CASE_OPEN, true, 1727570400);

    bool save_ok = nvram_save_tamper_log_snapshot(&tamper_ctx);
    assert(save_ok == true);

    tamper_context_t restored_ctx;
    bool load_ok = nvram_load_tamper_log_snapshot(&restored_ctx);
    assert(load_ok == true);
    assert(restored_ctx.active_tamper_mask == tamper_ctx.active_tamper_mask);
    assert(restored_ctx.total_event_count == 1);
    assert(restored_ctx.tamper_log_fifo.count == 1);

    printf("   [PASS] NVRAM Tamper Log Snapshot Valid!\n\n");
}

void test_nvram_energy_checkpoint_magic_stamp(void) {
    printf("[TEST] Testing NVRAM Energy Checkpoint Save/Load & Magic Stamp...\n");

    nvram_energy_checkpoint_t cp_in = {0};
    cp_in.timestamp = 1727570400;
    cp_in.active_energy_import_wh = 12345678;
    cp_in.active_energy_export_wh = 87654321;
    cp_in.reactive_energy_varh = 55555;
    cp_in.tou_tariff_wh[0] = 5000000;
    cp_in.tou_tariff_wh[1] = 7345678;
    cp_in.tou_import_tariff_wh[0] = 1500000;
    cp_in.tou_import_tariff_wh[1] = 2500000;
    cp_in.tou_import_tariff_wh[2] = 4765432;
    cp_in.vcap_mv_at_fail = 3250;
    cp_in.active_tamper_mask = 0;

    /* Uji auto-stamping: cp_in.magic sengaja bernilai 0 */
    assert(cp_in.magic == 0);

    bool save_ok = nvram_save_energy_checkpoint(&cp_in);
    assert(save_ok == true);

    nvram_energy_checkpoint_t cp_out = {0};
    bool load_ok = nvram_load_energy_checkpoint(&cp_out);
    assert(load_ok == true);
    assert(cp_out.magic == NVRAM_ENERGY_CHECKPOINT_MAGIC);
    assert(cp_out.timestamp == 1727570400);
    assert(cp_out.active_energy_import_wh == 12345678);
    assert(cp_out.active_energy_export_wh == 87654321);
    assert(cp_out.reactive_energy_varh == 55555);
    assert(cp_out.tou_tariff_wh[0] == 5000000);
    assert(cp_out.tou_tariff_wh[1] == 7345678);
    assert(cp_out.tou_import_tariff_wh[0] == 1500000);
    assert(cp_out.tou_import_tariff_wh[1] == 2500000);
    assert(cp_out.tou_import_tariff_wh[2] == 4765432);
    assert(cp_out.vcap_mv_at_fail == 3250);

    printf("   [PASS] NVRAM Energy Checkpoint Magic Stamp 0x454E4759 Verified!\n\n");
}

/* FUNGSI MAIN UTAMA HARUS ADA DI SINI */
int main(void) {
    printf("=========================================\n");
    printf("     RUNNING NVRAM STORAGE UNIT TESTS    \n");
    printf("=========================================\n\n");

    test_nvram_load_profile_write_read();
    test_nvram_tamper_log_snapshot();
    test_nvram_energy_checkpoint_magic_stamp();

    printf(">>> ALL NVRAM UNIT TESTS PASSED 100%%! <<<\n");
    printf("=========================================\n");
    return 0;
}
