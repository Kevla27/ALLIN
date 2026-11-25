/**
 * @file test_nvram_storage.c
 * @brief Unit Test untuk Pengujian Penyimpanan Memori NVRAM Flash STM32U575
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
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

void test_nvram_device_config(void) {
    printf("[TEST] Testing NVRAM Device Config (IDPEL & Meter ID)...\n");

    nvram_device_config_t cfg_in = {
        .customer_id = "54321999888",
        .meter_id    = "530000000099"
    };

    bool save_ok = nvram_save_device_config(&cfg_in);
    assert(save_ok == true);

    nvram_device_config_t cfg_out = {0};
    bool load_ok = nvram_load_device_config(&cfg_out);
    assert(load_ok == true);
    assert(cfg_out.magic == NVRAM_DEVICE_CONFIG_MAGIC);
    assert(strcmp(cfg_out.customer_id, "54321999888") == 0);
    assert(strcmp(cfg_out.meter_id, "530000000099") == 0);

    assert(nvram_save_device_config(NULL) == false);
    assert(nvram_load_device_config(NULL) == false);

    printf("   [PASS] NVRAM Device Config Magic Stamp 0x43464944 & Persistence Verified!\n\n");
}

void test_nvram_calibration_save_load(void) {
    printf("[TEST] Testing NVRAM Calibration Data Save/Load (ADE9000 Parameters)...\n");

    nvram_calibration_data_t cal_in = {
        .vgain = {1000, 1005, 998},
        .igain = {2000, 2002, 1995, 2001},
        .pgain = {500, 502, 498},
        .phase_cal = {12, 11, 13}
    };

    bool save_ok = nvram_save_calibration(&cal_in);
    assert(save_ok == true);

    nvram_calibration_data_t cal_out = {0};
    bool load_ok = nvram_load_calibration(&cal_out);
    assert(load_ok == true);
    assert(cal_out.magic == NVRAM_CALIBRATION_MAGIC);
    assert(cal_out.vgain[0] == 1000);
    assert(cal_out.vgain[1] == 1005);
    assert(cal_out.vgain[2] == 998);
    assert(cal_out.igain[0] == 2000);
    assert(cal_out.igain[3] == 2001);
    assert(cal_out.pgain[0] == 500);
    assert(cal_out.phase_cal[2] == 13);

    assert(nvram_save_calibration(NULL) == false);
    assert(nvram_load_calibration(NULL) == false);

    printf("   [PASS] NVRAM Calibration Data Magic Stamp 0x43414C42 & Integrity Verified!\n\n");
}

void test_nvram_sector_erase(void) {
    printf("[TEST] Testing NVRAM Sector Erase (4 KB Sector Opcode 0x20)...\n");

    bool erase_ok = nvram_erase_sector(SPI_FLASH_SECTOR_CHECKPOINT_SLOT_A);
    assert(erase_ok == true);

    bool erase_slot_b = nvram_erase_sector(SPI_FLASH_SECTOR_CHECKPOINT_SLOT_B);
    assert(erase_slot_b == true);

    printf("   [PASS] NVRAM Sector Erase Alignment & Execution Verified!\n\n");
}

void test_nvram_w25q64_spec_and_jedec(void) {
    printf("[TEST] Testing Winbond W25Q64 (64 Mbit / 8 MB) Specification & JEDEC ID...\n");

    /* Verifikasi spesifikasi kapasitas 8 MByte dan 2048 Sektor */
    assert(W25Q64_TOTAL_SIZE_BYTES == (8U * 1024U * 1024U));
    assert(W25Q64_TOTAL_SECTORS == 2048U);
    assert(W25Q64_TOTAL_BLOCKS_64K == 128U);

    /* Verifikasi pembacaan JEDEC ID (0xEF 0x40 0x17) */
    uint8_t mfg = 0, type = 0, cap = 0;
    bool read_ok = nvram_w25q64_read_jedec_id(&mfg, &type, &cap);
    assert(read_ok == true);
    assert(mfg == W25Q64_JEDEC_MANUFACTURER_ID); /* 0xEF = Winbond */
    assert(type == W25Q64_JEDEC_MEMORY_TYPE);     /* 0x40 = SPI/QSPI */
    assert(cap == W25Q64_JEDEC_CAPACITY_ID);     /* 0x17 = 64 Mbit */

    bool verify_ok = nvram_w25q64_verify();
    assert(verify_ok == true);

    printf("   [PASS] Winbond W25Q64 JEDEC ID (0x%02X 0x%02X 0x%02X) & 8MB Capacity Verified!\n\n",
           mfg, type, cap);
}

void test_nvram_virgin_flash_0xff_and_crc32(void) {
    printf("[TEST] Testing Virgin 0xFF Flash Protection & CRC32 Integrity...\n");

    nvram_test_simulate_virgin_flash();
    assert(nvram_flash_verify() == true);
    assert(nvram_has_hardware_fault() == false);

    nvram_energy_checkpoint_t cp;
    nvram_device_config_t cfg;
    nvram_calibration_data_t cal;
    tamper_context_t tctx;

    assert(nvram_load_energy_checkpoint(&cp) == false);
    assert(nvram_load_device_config(&cfg) == false);
    assert(nvram_load_calibration(&cal) == false);
    assert(nvram_load_tamper_log_snapshot(&tctx) == false);

    printf("   [PASS] Virgin 0xFF Flash safely rejected across all NVRAM blocks!\n\n");
}

void test_nvram_checkpoint_ring_wear_leveling(void) {
    printf("[TEST] Testing 8-Sector x 16-Page (128-Slot) Checkpoint Ring Wear-Leveling...\n");

    nvram_test_simulate_virgin_flash();

    for (uint32_t i = 1U; i <= 140U; i++) {
        nvram_energy_checkpoint_t cp = {0};
        cp.magic = NVRAM_ENERGY_CHECKPOINT_MAGIC;
        cp.timestamp = 1793181600U + (i * 300U);
        cp.active_energy_import_wh = 1000ULL + i;
        cp.active_energy_export_wh = 200ULL + i;
        cp.reactive_energy_varh = 50ULL + i;
        assert(nvram_save_energy_checkpoint(&cp) == true);
    }

    uint32_t erases = nvram_test_get_checkpoint_erase_count();
    assert(erases == 9U);

    nvram_energy_checkpoint_t loaded = {0};
    assert(nvram_load_energy_checkpoint(&loaded) == true);
    assert(loaded.active_energy_import_wh == 1140ULL);
    assert(loaded.timestamp == (1793181600U + (140U * 300U)));

    printf("   [PASS] 140 checkpoints triggered only %lu sector erases (Latest=1140 Wh)!\n\n",
           (unsigned long)erases);
}

void test_nvram_92_day_load_profile_slots(void) {
    printf("[TEST] Testing 92-Day (8832-Slot, 128B-Aligned) Load Profile Flash Ring...\n");

    assert(NVRAM_LOAD_PROFILE_FLASH_SLOTS == 8832U);
    nvram_test_simulate_virgin_flash();

    const uint32_t test_indices[5] = { 0U, 31U, 32U, 8831U, 8832U };
    for (size_t i = 0; i < 5U; i++) {
        load_profile_entry_t e = {0};
        e.timestamp = 1793181600U + (test_indices[i] * 900U);
        e.voltage_r_dvolts = (uint16_t)(2200U + i);
        e.active_power_w = (int32_t)(1000U + test_indices[i]);
        e.active_energy_import_wh = 5000ULL + test_indices[i];
        assert(nvram_write_load_profile_flash_slot(test_indices[i], &e) == true);
    }

    load_profile_entry_t r = {0};
    assert(nvram_read_load_profile_flash_slot(31U, &r) == true);
    assert(r.active_energy_import_wh == (5000ULL + 31U));

    assert(nvram_read_load_profile_flash_slot(32U, &r) == true);
    assert(r.active_energy_import_wh == (5000ULL + 32U));

    assert(nvram_read_load_profile_flash_slot(8831U, &r) == true);
    assert(r.active_energy_import_wh == (5000ULL + 8831U));

    /* Slot 8832 harus menimpa Slot 0 (wrap circular 92 hari) */
    assert(nvram_read_load_profile_flash_slot(0U, &r) == true);
    assert(r.active_energy_import_wh == (5000ULL + 8832U));

    printf("   [PASS] 92-Day Load Profile 8832-Slot Ring Verified!\n\n");
}

/* FUNGSI MAIN UTAMA HARUS ADA DI SINI */
int main(void) {
    printf("=========================================\n");
    printf("     RUNNING NVRAM STORAGE UNIT TESTS    \n");
    printf("=========================================\n\n");

    test_nvram_w25q64_spec_and_jedec();
    test_nvram_sector_erase();
    test_nvram_calibration_save_load();
    test_nvram_load_profile_write_read();
    test_nvram_tamper_log_snapshot();
    test_nvram_energy_checkpoint_magic_stamp();
    test_nvram_device_config();
    test_nvram_virgin_flash_0xff_and_crc32();
    test_nvram_checkpoint_ring_wear_leveling();
    test_nvram_92_day_load_profile_slots();

    printf(">>> ALL NVRAM UNIT TESTS PASSED 100%%! <<<\n");
    printf("=========================================\n");
    return 0;
}
