/**
 * @file nvram_storage.c
 * @brief Implementasi Penyimpanan Non-Volatile (SPI2 External NOR Flash / FRAM + RAM Backing Mirror)
 *        Sesuai Desain Sistem Hal. 26 & PIN.docx (PB12=FLASH_SPI2_CS, PB13=SCK, PB14=MISO, PB15=MOSI)
 */

#include "nvram_storage.h"
#include <string.h>

#if defined(EMBEDDED_HARDWARE_TARGET)
#include "main.h"
extern SPI_HandleTypeDef hspi2;

static void spi2_flash_wait_busy(uint32_t timeout_ms)
{
    uint8_t cmd = SPI_FLASH_CMD_RDSR1;
    uint8_t status = 0x01U;
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < timeout_ms)
    {
        HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_Transmit(&hspi2, &cmd, 1U, 5U) == HAL_OK)
        {
            (void)HAL_SPI_Receive(&hspi2, &status, 1U, 5U);
        }
        HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);
        if (status == 0xFFU || (status & 0x01U) == 0U)
        {
            break; /* Idle atau bus open-circuit */
        }
    }
}

static bool spi2_flash_erase_sector(uint32_t sector_addr)
{
    /* 1. Pastikan flash idle sebelum memulai siklus hapus */
    spi2_flash_wait_busy(100U);

    /* 2. Kirim Write Enable (0x06) */
    uint8_t wren = SPI_FLASH_CMD_WREN;
    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
    (void)HAL_SPI_Transmit(&hspi2, &wren, 1U, 5U);
    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);

    /* 3. Kirim Sector Erase (0x20) + 24-bit Sector Address (Aligned 4KB) */
    uint32_t aligned_addr = sector_addr & ~(SPI_FLASH_SECTOR_SIZE_BYTES - 1U);
    uint8_t hdr[4] = {
        SPI_FLASH_CMD_SE,
        (uint8_t)((aligned_addr >> 16) & 0xFFU),
        (uint8_t)((aligned_addr >> 8) & 0xFFU),
        (uint8_t)(aligned_addr & 0xFFU)
    };

    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
    bool tx_ok = (HAL_SPI_Transmit(&hspi2, hdr, 4U, 10U) == HAL_OK);
    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);

    if (!tx_ok) return false;

    /* 4. Tunggu proses penghapusan sektor selesai (W25Qxx typical 45ms, timeout 500ms) */
    spi2_flash_wait_busy(500U);
    return true;
}

static void spi2_flash_write_bytes(uint32_t flash_offset, const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U) return;

    while (len > 0U)
    {
        /* Hitung kapasitas tersisa pada halaman 256-byte saat ini agar data tidak wrap-around */
        uint16_t page_offset = (uint16_t)(flash_offset & (SPI_FLASH_PAGE_SIZE_BYTES - 1U));
        uint16_t chunk = SPI_FLASH_PAGE_SIZE_BYTES - page_offset;
        if (chunk > len)
        {
            chunk = len;
        }

        /* Pastikan flash idle sebelum memulai siklus tulis berikutnya */
        spi2_flash_wait_busy(10U);

        /* 1. Kirim Write Enable (0x06) */
        uint8_t wren = SPI_FLASH_CMD_WREN;
        HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
        (void)HAL_SPI_Transmit(&hspi2, &wren, 1U, 5U);
        HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);

        /* 2. Kirim Page Program (0x02) + 24-bit Flash Address */
        uint8_t hdr[4] = {
            SPI_FLASH_CMD_PP,
            (uint8_t)((flash_offset >> 16) & 0xFFU),
            (uint8_t)((flash_offset >> 8) & 0xFFU),
            (uint8_t)(flash_offset & 0xFFU)
        };

        HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_Transmit(&hspi2, hdr, 4U, 5U) == HAL_OK)
        {
            (void)HAL_SPI_Transmit(&hspi2, (uint8_t *)data, chunk, 20U);
        }
        HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);

        flash_offset += chunk;
        data         += chunk;
        len          -= chunk;
    }

    /* Pastikan halaman terakhir selesai diprogram */
    spi2_flash_wait_busy(10U);
}

static void spi2_flash_read_bytes(uint32_t flash_offset, uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U) return;

    /* Tunggu jika ada siklus tulis yang sedang berlangsung */
    spi2_flash_wait_busy(50U);

    uint8_t hdr[4] = {
        SPI_FLASH_CMD_READ,
        (uint8_t)((flash_offset >> 16) & 0xFFU),
        (uint8_t)((flash_offset >> 8) & 0xFFU),
        (uint8_t)(flash_offset & 0xFFU)
    };

    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(&hspi2, hdr, 4U, 10U) == HAL_OK)
    {
        (void)HAL_SPI_Receive(&hspi2, data, len, 50U);
    }
    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);
}
#endif

/* Buffer NVRAM Backing Store untuk menjamin integritas baca/tulis pada CTest & RAM Mirror */
static load_profile_entry_t      simulated_load_profile_flash[LOAD_PROFILE_MAX_ENTRIES];
static tamper_context_t          simulated_tamper_log_flash;
static nvram_energy_checkpoint_t  simulated_energy_checkpoint_flash = {0};
static nvram_device_config_t      simulated_device_config_flash = {0};
static nvram_calibration_data_t   simulated_calibration_flash = {0};
#if defined(EMBEDDED_HARDWARE_TARGET)
static uint8_t                   s_checkpoint_active_slot = 0U; /* 0 = Slot A, 1 = Slot B */
#endif

bool nvram_erase_sector(uint32_t sector_addr)
{
#if defined(EMBEDDED_HARDWARE_TARGET)
    return spi2_flash_erase_sector(sector_addr);
#else
    (void)sector_addr;
    return true;
#endif
}

bool nvram_write_load_profile_entry(uint32_t entry_index, const load_profile_entry_t *entry)
{
    if (entry == NULL || entry_index >= LOAD_PROFILE_MAX_ENTRIES) return false;

    simulated_load_profile_flash[entry_index] = *entry;

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t offset = SPI_FLASH_SECTOR_LOAD_PROFILE + (entry_index * (uint32_t)sizeof(load_profile_entry_t));
    /* Jika offset berada tepat di awal batas sektor 4 KB, hapus sektor tersebut sebelum entri pertama ditulis */
    if ((offset & (SPI_FLASH_SECTOR_SIZE_BYTES - 1U)) == 0U)
    {
        spi2_flash_erase_sector(offset);
    }
    spi2_flash_write_bytes(offset, (const uint8_t *)entry, (uint16_t)sizeof(load_profile_entry_t));
#endif
    return true;
}

bool nvram_read_load_profile_entry(uint32_t entry_index, load_profile_entry_t *out_entry)
{
    if (out_entry == NULL || entry_index >= LOAD_PROFILE_MAX_ENTRIES) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t offset = SPI_FLASH_SECTOR_LOAD_PROFILE + (entry_index * (uint32_t)sizeof(load_profile_entry_t));
    load_profile_entry_t flash_entry = {0};
    spi2_flash_read_bytes(offset, (uint8_t *)&flash_entry, (uint16_t)sizeof(load_profile_entry_t));
    if (flash_entry.timestamp != 0xFFFFFFFFU && flash_entry.timestamp != 0U)
    {
        simulated_load_profile_flash[entry_index] = flash_entry;
    }
#endif

    *out_entry = simulated_load_profile_flash[entry_index];
    return true;
}

bool nvram_save_tamper_log_snapshot(const tamper_context_t *tamper_ctx)
{
    if (tamper_ctx == NULL) return false;

    simulated_tamper_log_flash = *tamper_ctx;

#if defined(EMBEDDED_HARDWARE_TARGET)
    /* Hapus Sektor 2 sebelum menulis snapshot tamper terbaru (4 KB) */
    spi2_flash_erase_sector(SPI_FLASH_SECTOR_TAMPER_LOG);
    spi2_flash_write_bytes(SPI_FLASH_SECTOR_TAMPER_LOG, (const uint8_t *)tamper_ctx, (uint16_t)sizeof(tamper_context_t));
#endif
    return true;
}

bool nvram_load_tamper_log_snapshot(tamper_context_t *tamper_ctx)
{
    if (tamper_ctx == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    tamper_context_t flash_tamper = {0};
    spi2_flash_read_bytes(SPI_FLASH_SECTOR_TAMPER_LOG, (uint8_t *)&flash_tamper, (uint16_t)sizeof(tamper_context_t));
    if (flash_tamper.total_event_count > 0 || (flash_tamper.active_tamper_mask != 0xFFFFFFFFU && flash_tamper.active_tamper_mask != 0U))
    {
        simulated_tamper_log_flash = flash_tamper;
    }
#endif

    *tamper_ctx = simulated_tamper_log_flash;
    return true;
}

bool nvram_save_energy_checkpoint(const nvram_energy_checkpoint_t *checkpoint)
{
    if (checkpoint == NULL) return false;

    simulated_energy_checkpoint_flash = *checkpoint;
    /* Jamin stempel magic resmi NVRAM_ENERGY_CHECKPOINT_MAGIC ("ENGY" = 0x454E4759) selalu valid terisi */
    simulated_energy_checkpoint_flash.magic = NVRAM_ENERGY_CHECKPOINT_MAGIC;

#if defined(EMBEDDED_HARDWARE_TARGET)
    /* Ping-Pong Dual-Slot Protection (Power-Fail Safe):
     * Tulis secara bergantian antara Slot A (0x001000) dan Slot B (0x003000).
     * Sektor tujuan selalu di-erase (0x20) terlebih dahulu sebelum penulisan. */
    uint32_t target_sector = (s_checkpoint_active_slot == 0U) 
                           ? SPI_FLASH_SECTOR_CHECKPOINT_SLOT_B 
                           : SPI_FLASH_SECTOR_CHECKPOINT_SLOT_A;

    spi2_flash_erase_sector(target_sector);
    spi2_flash_write_bytes(target_sector, (const uint8_t *)&simulated_energy_checkpoint_flash, (uint16_t)sizeof(nvram_energy_checkpoint_t));

    s_checkpoint_active_slot = (s_checkpoint_active_slot == 0U) ? 1U : 0U;
#endif
    return true;
}

bool nvram_load_energy_checkpoint(nvram_energy_checkpoint_t *out_checkpoint)
{
    if (out_checkpoint == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    nvram_energy_checkpoint_t cp_a = {0};
    nvram_energy_checkpoint_t cp_b = {0};

    spi2_flash_read_bytes(SPI_FLASH_SECTOR_CHECKPOINT_SLOT_A, (uint8_t *)&cp_a, (uint16_t)sizeof(nvram_energy_checkpoint_t));
    spi2_flash_read_bytes(SPI_FLASH_SECTOR_CHECKPOINT_SLOT_B, (uint8_t *)&cp_b, (uint16_t)sizeof(nvram_energy_checkpoint_t));

    bool valid_a = (cp_a.magic == NVRAM_ENERGY_CHECKPOINT_MAGIC);
    bool valid_b = (cp_b.magic == NVRAM_ENERGY_CHECKPOINT_MAGIC);

    if (valid_a && valid_b)
    {
        /* Pilih checkpoint yang memiliki stempel waktu terbaru */
        if (cp_b.timestamp >= cp_a.timestamp)
        {
            simulated_energy_checkpoint_flash = cp_b;
            s_checkpoint_active_slot = 1U;
        }
        else
        {
            simulated_energy_checkpoint_flash = cp_a;
            s_checkpoint_active_slot = 0U;
        }
    }
    else if (valid_a)
    {
        simulated_energy_checkpoint_flash = cp_a;
        s_checkpoint_active_slot = 0U;
    }
    else if (valid_b)
    {
        simulated_energy_checkpoint_flash = cp_b;
        s_checkpoint_active_slot = 1U;
    }
#endif

    *out_checkpoint = simulated_energy_checkpoint_flash;
    return (out_checkpoint->magic == NVRAM_ENERGY_CHECKPOINT_MAGIC);
}

bool nvram_save_device_config(const nvram_device_config_t *config)
{
    if (config == NULL) return false;

    simulated_device_config_flash = *config;
    simulated_device_config_flash.magic = NVRAM_DEVICE_CONFIG_MAGIC;

#if defined(EMBEDDED_HARDWARE_TARGET)
    /* Alokasi sektor konfigurasi perangkat pada Sektor 4 (0x004000U) */
    spi2_flash_erase_sector(SPI_FLASH_SECTOR_DEVICE_CONFIG);
    spi2_flash_write_bytes(SPI_FLASH_SECTOR_DEVICE_CONFIG, (const uint8_t *)&simulated_device_config_flash, (uint16_t)sizeof(nvram_device_config_t));
#endif
    return true;
}

bool nvram_load_device_config(nvram_device_config_t *out_config)
{
    if (out_config == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    nvram_device_config_t flash_cfg = {0};
    spi2_flash_read_bytes(SPI_FLASH_SECTOR_DEVICE_CONFIG, (uint8_t *)&flash_cfg, (uint16_t)sizeof(nvram_device_config_t));
    if (flash_cfg.magic == NVRAM_DEVICE_CONFIG_MAGIC)
    {
        simulated_device_config_flash = flash_cfg;
    }
#endif

    *out_config = simulated_device_config_flash;
    return (out_config->magic == NVRAM_DEVICE_CONFIG_MAGIC);
}

bool nvram_save_calibration(const nvram_calibration_data_t *cal)
{
    if (cal == NULL) return false;

    nvram_calibration_data_t cal_copy = *cal;
    cal_copy.magic = NVRAM_CALIBRATION_MAGIC;

    /* Hitung Checksum sederhana untuk verifikasi integritas */
    uint32_t csum = cal_copy.magic;
    for (int i = 0; i < 3; i++)
    {
        csum += (uint32_t)cal_copy.vgain[i];
        csum += (uint32_t)cal_copy.pgain[i];
        csum += (uint32_t)cal_copy.phase_cal[i];
    }
    for (int i = 0; i < 4; i++)
    {
        csum += (uint32_t)cal_copy.igain[i];
    }
    cal_copy.checksum = csum;

    simulated_calibration_flash = cal_copy;

#if defined(EMBEDDED_HARDWARE_TARGET)
    /* Hapus Sektor 0 (0x000000U) dan tulis parameter kalibrasi ADE9000 */
    spi2_flash_erase_sector(SPI_FLASH_SECTOR_CALIBRATION);
    spi2_flash_write_bytes(SPI_FLASH_SECTOR_CALIBRATION, (const uint8_t *)&cal_copy, (uint16_t)sizeof(nvram_calibration_data_t));
#endif
    return true;
}

bool nvram_load_calibration(nvram_calibration_data_t *out_cal)
{
    if (out_cal == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    nvram_calibration_data_t flash_cal = {0};
    spi2_flash_read_bytes(SPI_FLASH_SECTOR_CALIBRATION, (uint8_t *)&flash_cal, (uint16_t)sizeof(nvram_calibration_data_t));
    if (flash_cal.magic == NVRAM_CALIBRATION_MAGIC)
    {
        uint32_t csum = flash_cal.magic;
        for (int i = 0; i < 3; i++)
        {
            csum += (uint32_t)flash_cal.vgain[i];
            csum += (uint32_t)flash_cal.pgain[i];
            csum += (uint32_t)flash_cal.phase_cal[i];
        }
        for (int i = 0; i < 4; i++)
        {
            csum += (uint32_t)flash_cal.igain[i];
        }
        if (csum == flash_cal.checksum)
        {
            simulated_calibration_flash = flash_cal;
        }
    }
#endif

    *out_cal = simulated_calibration_flash;
    return (out_cal->magic == NVRAM_CALIBRATION_MAGIC);
}

bool nvram_w25q64_read_jedec_id(uint8_t *out_mfg, uint8_t *out_type, uint8_t *out_cap)
{
    if (out_mfg == NULL || out_type == NULL || out_cap == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    spi2_flash_wait_busy(50U);

    uint8_t cmd = SPI_FLASH_CMD_JEDEC_ID;
    uint8_t rx_buf[3] = {0};

    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef tx_st = HAL_SPI_Transmit(&hspi2, &cmd, 1U, 10U);
    HAL_StatusTypeDef rx_st = HAL_SPI_Receive(&hspi2, rx_buf, 3U, 10U);
    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);

    if (tx_st != HAL_OK || rx_st != HAL_OK)
    {
        return false;
    }

    *out_mfg  = rx_buf[0];
    *out_type = rx_buf[1];
    *out_cap  = rx_buf[2];
    return true;
#else
    /* Simulasi host / CTest */
    *out_mfg  = W25Q64_JEDEC_MANUFACTURER_ID;
    *out_type = W25Q64_JEDEC_MEMORY_TYPE;
    *out_cap  = W25Q64_JEDEC_CAPACITY_ID;
    return true;
#endif
}

bool nvram_w25q64_verify(void)
{
    uint8_t mfg = 0;
    uint8_t type = 0;
    uint8_t cap = 0;

    if (!nvram_w25q64_read_jedec_id(&mfg, &type, &cap))
    {
        return false;
    }

    return (mfg == W25Q64_JEDEC_MANUFACTURER_ID &&
            type == W25Q64_JEDEC_MEMORY_TYPE &&
            cap == W25Q64_JEDEC_CAPACITY_ID);
}
