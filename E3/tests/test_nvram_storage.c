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

    bool invalid_write = nvram_write_load_profile_entry(LOAD_PROFILE_MAX_ENTRIES + 1, &entry_to_write);
    assert(invalid_write == false);

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

    printf("   [PASS] NVRAM Tamper Log Snapshot Valid!\n\n");
}

/* FUNGSI MAIN UTAMA HARUS ADA DI SINI */
int main(void) {
    printf("=========================================\n");
    printf("     RUNNING NVRAM STORAGE UNIT TESTS    \n");
    printf("=========================================\n\n");

    test_nvram_load_profile_write_read();
    test_nvram_tamper_log_snapshot();

    printf(">>> ALL NVRAM UNIT TESTS PASSED 100%! <<<\n");
    printf("=========================================\n");
    return 0;
}