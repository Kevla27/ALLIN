/**
 * @file nvram_storage.h
 * @brief Pengelola Penyimpanan Non-Volatile (NVRAM) Flash Internal STM32U575
 */

#ifndef NVRAM_STORAGE_H
#define NVRAM_STORAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "load_profile.h"
#include "tamper_manager.h"

/* Alokasi Alamat Memori Non-Volatile Flash / EEPROM (Default: STM32U575 Bank 2) */
#ifndef NVRAM_CALIBRATION_START_ADDR
#define NVRAM_CALIBRATION_START_ADDR  0x081F0000U
#endif

#ifndef NVRAM_TAMPER_LOG_START_ADDR
#define NVRAM_TAMPER_LOG_START_ADDR   0x081F2000U
#endif

#ifndef NVRAM_LOAD_PROFILE_START_ADDR
#define NVRAM_LOAD_PROFILE_START_ADDR 0x081F4000U
#endif

#ifndef FLASH_PAGE_SIZE_BYTES
#define FLASH_PAGE_SIZE_BYTES          8192U  /* Default: 8 KB per Page */
#endif

/**
 * @brief Menyimpan 1 entri Load Profile terbaru ke dalam Flash Non-Volatile
 */
bool nvram_write_load_profile_entry(uint32_t entry_index, const load_profile_entry_t *entry);

/**
 * @brief Membaca 1 entri Load Profile dari Flash berdasarkan indeks
 */
bool nvram_read_load_profile_entry(uint32_t entry_index, load_profile_entry_t *out_entry);

/**
 * @brief Menyimpan seluruh FIFO Tamper Log ke Flash saat pemadaman (Last-Gasp)
 */
bool nvram_save_tamper_log_snapshot(const tamper_context_t *tamper_ctx);

/**
 * @brief Memuat kembali FIFO Tamper Log dari Flash saat booting (Power-On)
 */
bool nvram_load_tamper_log_snapshot(tamper_context_t *tamper_ctx);

#ifdef __cplusplus
}
#endif

#endif /* NVRAM_STORAGE_H */