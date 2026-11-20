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
#define NVRAM_CALIBRATION_START_ADDR       0x081F0000U
#endif

#ifndef NVRAM_ENERGY_CHECKPOINT_START_ADDR
#define NVRAM_ENERGY_CHECKPOINT_START_ADDR 0x081F1000U
#endif

#ifndef NVRAM_TAMPER_LOG_START_ADDR
#define NVRAM_TAMPER_LOG_START_ADDR        0x081F2000U
#endif

#ifndef NVRAM_LOAD_PROFILE_START_ADDR
#define NVRAM_LOAD_PROFILE_START_ADDR      0x081F4000U
#endif

#ifndef FLASH_PAGE_SIZE_BYTES
#define FLASH_PAGE_SIZE_BYTES              8192U  /* Default: 8 KB per Page */
#endif

/* ========================================================================= */
/*  Konfigurasi & Alokasi Sektor Memori SPI Flash Eksternal (W25Qxx)         */
/* ========================================================================= */
#define SPI_FLASH_SECTOR_SIZE_BYTES        4096U  /**< Ukuran 1 Sektor Standar NOR Flash (4 KB) */
#define SPI_FLASH_PAGE_SIZE_BYTES          256U   /**< Ukuran 1 Halaman Pemrograman (256 Byte) */

/* Perintah SPI Flash (Opcodes W25Qxx / Winbond / Macronix) */
#define SPI_FLASH_CMD_WREN                 0x06U  /**< Write Enable */
#define SPI_FLASH_CMD_WRDI                 0x04U  /**< Write Disable */
#define SPI_FLASH_CMD_PP                   0x02U  /**< Page Program (256 Byte) */
#define SPI_FLASH_CMD_SE                   0x20U  /**< Sector Erase (4 KB) */
#define SPI_FLASH_CMD_BE32                 0x52U  /**< Block Erase (32 KB) */
#define SPI_FLASH_CMD_BE64                 0xD8U  /**< Block Erase (64 KB) */
#define SPI_FLASH_CMD_CE                   0xC7U  /**< Chip Erase */
#define SPI_FLASH_CMD_READ                 0x03U  /**< Read Data */
#define SPI_FLASH_CMD_RDSR1                0x05U  /**< Read Status Register 1 */

/* Peta Alokasi Sektor Fisik SPI2 Flash Eksternal (4 KB per Sektor) */
#define SPI_FLASH_SECTOR_CALIBRATION       0x000000U /**< Sektor 0 (0..4KB): Parameter Kalibrasi ADE9000 */
#define SPI_FLASH_SECTOR_CHECKPOINT_SLOT_A 0x001000U /**< Sektor 1 (4..8KB): Energy Checkpoint Slot A */
#define SPI_FLASH_SECTOR_TAMPER_LOG        0x002000U /**< Sektor 2 (8..12KB): Snapshot Tamper Context FIFO */
#define SPI_FLASH_SECTOR_CHECKPOINT_SLOT_B 0x003000U /**< Sektor 3 (12..16KB): Energy Checkpoint Slot B (Ping-Pong) */
#define SPI_FLASH_SECTOR_DEVICE_CONFIG     0x004000U /**< Sektor 4 (16..20KB): Device Config (IDPEL & Meter ID) */
#define SPI_FLASH_SECTOR_LOAD_PROFILE      0x005000U /**< Sektor 5..N (20KB+): Load Profile Ring Buffer */

#define NVRAM_ENERGY_CHECKPOINT_MAGIC      0x454E4759U /**< "ENGY" */
#define NVRAM_DEVICE_CONFIG_MAGIC          0x43464944U /**< "CFID" (Config ID) */
#define NVRAM_CALIBRATION_MAGIC            0x43414C42U /**< "CALB" (Calibration) */

/**
 * @brief Struktur Data Parameter Kalibrasi ADE9000 Non-Volatile
 */
typedef struct {
    uint32_t magic;                    /**< 0x43414C42 ("CALB") */
    int32_t  vgain[3];                 /**< Register AVGAIN, BVGAIN, CVGAIN */
    int32_t  igain[4];                 /**< Register AIGAIN, BIGAIN, CIGAIN, NIRMS/gain */
    int32_t  pgain[3];                 /**< Register APGAIN, BPGAIN, CPGAIN */
    int32_t  phase_cal[3];             /**< Register APHCAL, BPHCAL, CPHCAL */
    uint32_t checksum;                 /**< Checksum Validasi Integritas */
} nvram_calibration_data_t;

/**
 * @brief Struktur Data Konfigurasi Identitas Meter Non-Volatile
 */
typedef struct {
    uint32_t magic;                    /**< 0x43464944 ("CFID") */
    char customer_id[16];              /**< ID Pelanggan PLN (IDPEL, 11-12 digit) */
    char meter_id[16];                 /**< Nomor Seri Meter (Meter ID, 12 digit) */
} nvram_device_config_t;

/**
 * @brief Struktur Data Checkpoint Energi & Status Darurat (Last-Gasp Backup)
 */
typedef struct {
    uint32_t magic;                    /**< 0x454E4759 ("ENGY") */
    uint32_t timestamp;                /**< Epoch timestamp saat pemadaman */
    uint64_t active_energy_import_wh;  /**< Stand energi aktif impor/kirim total (Wh) */
    uint64_t active_energy_export_wh;  /**< Stand energi aktif ekspor/terima PLTS total (Wh) */
    uint64_t reactive_energy_varh;     /**< Stand energi reaktif (varh) */
    uint64_t tou_tariff_wh[8];         /**< Stand kirim per zona tarif ToU (WBP, LWBP1, LWBP2, dll) */
    uint64_t tou_import_tariff_wh[8];  /**< Stand terima PLTS per zona tarif ToU (EA_IMP1..3 Tabel E.2 No 11-13) */
    uint32_t vcap_mv_at_fail;          /**< Tegangan superkapasitor VCAP saat AC Fail (mV) */
    uint32_t active_tamper_mask;       /**< Bitmask vektor sabotase yang sedang aktif */
} nvram_energy_checkpoint_t;

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

/**
 * @brief Menyimpan checkpoint stand energi darurat ke memori non-volatile (Last-Gasp)
 */
bool nvram_save_energy_checkpoint(const nvram_energy_checkpoint_t *checkpoint);

/**
 * @brief Memuat kembali checkpoint stand energi darurat saat booting
 */
bool nvram_load_energy_checkpoint(nvram_energy_checkpoint_t *out_checkpoint);

/**
 * @brief Menyimpan konfigurasi ID Pelanggan & Meter ID ke memori non-volatile (SPI2 Flash)
 */
bool nvram_save_device_config(const nvram_device_config_t *config);

/**
 * @brief Memuat konfigurasi ID Pelanggan & Meter ID dari memori non-volatile
 */
bool nvram_load_device_config(nvram_device_config_t *out_config);

/**
 * @brief Menghapus 1 sektor 4 KB pada memori Flash eksternal SPI (W25Qxx)
 * @param sector_addr Alamat awal sektor (akan disejajarkan ke batas 4 KB)
 * @return true jika perintah penghapusan berhasil dikirim dan selesai
 */
bool nvram_erase_sector(uint32_t sector_addr);

/**
 * @brief Menyimpan konfigurasi parameter kalibrasi ADE9000 ke Flash non-volatile
 */
bool nvram_save_calibration(const nvram_calibration_data_t *cal);

/**
 * @brief Memuat konfigurasi parameter kalibrasi ADE9000 dari Flash non-volatile
 */
bool nvram_load_calibration(nvram_calibration_data_t *out_cal);

#ifdef __cplusplus
}
#endif

#endif /* NVRAM_STORAGE_H */
