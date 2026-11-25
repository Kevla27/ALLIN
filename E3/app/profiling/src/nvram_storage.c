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

static volatile bool s_nvram_hw_fault = false;

static bool spi2_flash_wait_busy(uint32_t timeout_ms)
{
    uint8_t cmd = SPI_FLASH_CMD_RDSR1;
    uint8_t status = 0x01U;
    uint32_t start = HAL_GetTick();
    while ((HAL_GetTick() - start) < timeout_ms)
    {
        HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
        HAL_StatusTypeDef tx_st = HAL_SPI_Transmit(&hspi2, &cmd, 1U, 5U);
        HAL_StatusTypeDef rx_st = (tx_st == HAL_OK) ? HAL_SPI_Receive(&hspi2, &status, 1U, 5U) : HAL_ERROR;
        HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);
        if (tx_st != HAL_OK || rx_st != HAL_OK)
        {
            s_nvram_hw_fault = true;
            return false;
        }
        if (status == 0xFFU)
        {
            return false; /* Bus open-circuit / tidak terpasang */
        }
        if ((status & 0x01U) == 0U)
        {
            return true; /* Flash siap */
        }
    }
    s_nvram_hw_fault = true;
    return false;
}

static bool spi2_flash_erase_sector(uint32_t sector_addr)
{
    /* 1. Pastikan flash idle sebelum memulai siklus hapus */
    (void)spi2_flash_wait_busy(100U);

    /* 2. Kirim Write Enable (0x06) */
    uint8_t wren = SPI_FLASH_CMD_WREN;
    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef we_st = HAL_SPI_Transmit(&hspi2, &wren, 1U, 5U);
    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);
    if (we_st != HAL_OK)
    {
        s_nvram_hw_fault = true;
        return false;
    }

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

    if (!tx_ok)
    {
        s_nvram_hw_fault = true;
        return false;
    }

    /* 4. Tunggu proses penghapusan sektor selesai (W25Qxx typical 45ms, timeout 500ms) */
    return spi2_flash_wait_busy(500U);
}

static bool spi2_flash_write_bytes(uint32_t flash_offset, const uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U) return false;

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
        if (!spi2_flash_wait_busy(20U))
        {
            return false;
        }

        /* 1. Kirim Write Enable (0x06) */
        uint8_t wren = SPI_FLASH_CMD_WREN;
        HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
        HAL_StatusTypeDef we_st = HAL_SPI_Transmit(&hspi2, &wren, 1U, 5U);
        HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);
        if (we_st != HAL_OK)
        {
            s_nvram_hw_fault = true;
            return false;
        }

        /* 2. Kirim Page Program (0x02) + 24-bit Flash Address */
        uint8_t hdr[4] = {
            SPI_FLASH_CMD_PP,
            (uint8_t)((flash_offset >> 16) & 0xFFU),
            (uint8_t)((flash_offset >> 8) & 0xFFU),
            (uint8_t)(flash_offset & 0xFFU)
        };

        HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
        bool ok = (HAL_SPI_Transmit(&hspi2, hdr, 4U, 5U) == HAL_OK) &&
                  (HAL_SPI_Transmit(&hspi2, (uint8_t *)data, chunk, 20U) == HAL_OK);
        HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);

        if (!ok)
        {
            s_nvram_hw_fault = true;
            return false;
        }

        flash_offset += chunk;
        data         += chunk;
        len          -= chunk;
    }

    /* Pastikan halaman terakhir selesai diprogram */
    return spi2_flash_wait_busy(20U);
}

static bool spi2_flash_read_bytes(uint32_t flash_offset, uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U) return false;

    /* Tunggu jika ada siklus tulis yang sedang berlangsung */
    (void)spi2_flash_wait_busy(50U);

    uint8_t hdr[4] = {
        SPI_FLASH_CMD_READ,
        (uint8_t)((flash_offset >> 16) & 0xFFU),
        (uint8_t)((flash_offset >> 8) & 0xFFU),
        (uint8_t)(flash_offset & 0xFFU)
    };

    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
    bool ok = (HAL_SPI_Transmit(&hspi2, hdr, 4U, 10U) == HAL_OK) &&
              (HAL_SPI_Receive(&hspi2, data, len, 50U) == HAL_OK);
    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);
    if (!ok)
    {
        s_nvram_hw_fault = true;
    }
    return ok;
}
#else
static volatile bool s_nvram_hw_fault = false;
#endif

static uint32_t nvram_calc_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFU;
    if (data == NULL) return 0U;
    for (size_t i = 0U; i < len; i++)
    {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
        {
            crc = (crc & 1U) ? ((crc >> 1) ^ 0xEDB88320U) : (crc >> 1);
        }
    }
    return ~crc;
}

#define NVRAM_CHECKPOINT_TOTAL_SLOTS (NVRAM_CHECKPOINT_RING_SECTORS * NVRAM_CHECKPOINT_PAGES_PER_SECTOR)

typedef struct {
    uint32_t                 magic;
    uint32_t                 seq;
    nvram_energy_checkpoint_t cp;
    uint32_t                 crc32;
} nvram_checkpoint_flash_slot_t;

typedef struct {
    uint32_t         magic;
    uint32_t         seq;
    tamper_context_t ctx;
    uint32_t         crc32;
} nvram_tamper_flash_slot_t;

typedef struct {
    uint32_t              magic;
    uint32_t              seq;
    nvram_device_config_t cfg;
    uint32_t              crc32;
} nvram_dev_cfg_flash_slot_t;

typedef struct {
    uint32_t                 magic;
    uint32_t                 seq;
    nvram_calibration_data_t cal;
    uint32_t                 crc32;
} nvram_cal_flash_slot_t;

#if !defined(EMBEDDED_HARDWARE_TARGET)
/* Buffer Simulasi NVRAM HANYA untuk Host PC / CTest (tidak masuk binary STM32U5) */
static load_profile_entry_t          simulated_load_profile_flash[NVRAM_LOAD_PROFILE_FLASH_SLOTS];
static nvram_tamper_flash_slot_t     simulated_tamper_slots[2];
static nvram_checkpoint_flash_slot_t simulated_checkpoint_ring[NVRAM_CHECKPOINT_TOTAL_SLOTS];
static nvram_dev_cfg_flash_slot_t    simulated_dev_cfg_slots[2];
static nvram_cal_flash_slot_t        simulated_cal_slots[2];
static uint32_t s_cp_erase_count = 0U;
#else
static nvram_tamper_flash_slot_t     s_tamper_hw_scratch;
#endif

static uint32_t s_cp_seq            = 0U;
static uint32_t s_cp_next_slot      = 0U;
static uint32_t s_tamper_seq        = 0U;
static uint32_t s_dev_cfg_seq       = 0U;
static uint32_t s_cal_seq           = 0U;

bool nvram_erase_sector(uint32_t sector_addr)
{
#if defined(EMBEDDED_HARDWARE_TARGET)
    return spi2_flash_erase_sector(sector_addr);
#else
    (void)sector_addr;
    return true;
#endif
}

bool nvram_write_load_profile_flash_slot(uint32_t seq_index, const load_profile_entry_t *entry)
{
    if (entry == NULL) return false;
    uint32_t slot = seq_index % NVRAM_LOAD_PROFILE_FLASH_SLOTS;

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t offset = SPI_FLASH_SECTOR_LOAD_PROFILE + (slot * NVRAM_LOAD_PROFILE_SLOT_STRIDE);
    if ((slot & 31U) == 0U)
    {
        if (!spi2_flash_erase_sector(offset))
        {
            return false;
        }
    }
    return spi2_flash_write_bytes(offset, (const uint8_t *)entry, (uint16_t)sizeof(load_profile_entry_t));
#else
    simulated_load_profile_flash[slot] = *entry;
    return true;
#endif
}

bool nvram_read_load_profile_flash_slot(uint32_t seq_index, load_profile_entry_t *out_entry)
{
    if (out_entry == NULL) return false;
    uint32_t slot = seq_index % NVRAM_LOAD_PROFILE_FLASH_SLOTS;

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t offset = SPI_FLASH_SECTOR_LOAD_PROFILE + (slot * NVRAM_LOAD_PROFILE_SLOT_STRIDE);
    load_profile_entry_t flash_entry = {0};
    if (!spi2_flash_read_bytes(offset, (uint8_t *)&flash_entry, (uint16_t)sizeof(load_profile_entry_t)) ||
        flash_entry.timestamp == 0xFFFFFFFFU || flash_entry.timestamp == 0U)
    {
        memset(out_entry, 0, sizeof(*out_entry));
        return false;
    }
    *out_entry = flash_entry;
    return true;
#else
    *out_entry = simulated_load_profile_flash[slot];
    return true;
#endif
}

bool nvram_write_load_profile_entry(uint32_t entry_index, const load_profile_entry_t *entry)
{
    if (entry == NULL || entry_index >= LOAD_PROFILE_MAX_ENTRIES) return false;
    return nvram_write_load_profile_flash_slot(entry_index, entry);
}

bool nvram_read_load_profile_entry(uint32_t entry_index, load_profile_entry_t *out_entry)
{
    if (out_entry == NULL || entry_index >= LOAD_PROFILE_MAX_ENTRIES) return false;
    return nvram_read_load_profile_flash_slot(entry_index, out_entry);
}

bool nvram_save_tamper_log_snapshot(const tamper_context_t *tamper_ctx)
{
    if (tamper_ctx == NULL) return false;

    s_tamper_seq++;
    uint32_t bank = s_tamper_seq & 1U;

#if defined(EMBEDDED_HARDWARE_TARGET)
    memset(&s_tamper_hw_scratch, 0, sizeof(s_tamper_hw_scratch));
    s_tamper_hw_scratch.magic = NVRAM_TAMPER_SNAP_MAGIC;
    s_tamper_hw_scratch.seq   = s_tamper_seq;
    s_tamper_hw_scratch.ctx   = *tamper_ctx;
    s_tamper_hw_scratch.crc32 = nvram_calc_crc32((const uint8_t *)&s_tamper_hw_scratch, offsetof(nvram_tamper_flash_slot_t, crc32));

    uint32_t sector = (bank == 0U) ? SPI_FLASH_SECTOR_TAMPER_LOG : SPI_FLASH_SECTOR_TAMPER_LOG_B;
    if (!spi2_flash_erase_sector(sector))
    {
        return false;
    }
    return spi2_flash_write_bytes(sector, (const uint8_t *)&s_tamper_hw_scratch, (uint16_t)sizeof(s_tamper_hw_scratch));
#else
    nvram_tamper_flash_slot_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.magic = NVRAM_TAMPER_SNAP_MAGIC;
    rec.seq   = s_tamper_seq;
    rec.ctx   = *tamper_ctx;
    rec.crc32 = nvram_calc_crc32((const uint8_t *)&rec, offsetof(nvram_tamper_flash_slot_t, crc32));
    simulated_tamper_slots[bank] = rec;
    return true;
#endif
}

bool nvram_load_tamper_log_snapshot(tamper_context_t *tamper_ctx)
{
    if (tamper_ctx == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    static const uint32_t banks[2] = { SPI_FLASH_SECTOR_TAMPER_LOG, SPI_FLASH_SECTOR_TAMPER_LOG_B };
    uint32_t best_seq = 0U;
    bool found = false;

    for (size_t i = 0U; i < 2U; i++)
    {
        memset(&s_tamper_hw_scratch, 0xFF, sizeof(s_tamper_hw_scratch));
        if (spi2_flash_read_bytes(banks[i], (uint8_t *)&s_tamper_hw_scratch, (uint16_t)sizeof(s_tamper_hw_scratch)) &&
            s_tamper_hw_scratch.magic == NVRAM_TAMPER_SNAP_MAGIC &&
            s_tamper_hw_scratch.crc32 == nvram_calc_crc32((const uint8_t *)&s_tamper_hw_scratch, offsetof(nvram_tamper_flash_slot_t, crc32)))
        {
            if (!found || s_tamper_hw_scratch.seq >= best_seq)
            {
                found       = true;
                best_seq    = s_tamper_hw_scratch.seq;
                *tamper_ctx = s_tamper_hw_scratch.ctx;
            }
        }
    }
    if (!found) return false;
    s_tamper_seq = best_seq;
    return true;
#else
    const nvram_tamper_flash_slot_t *best = NULL;
    for (size_t i = 0U; i < 2U; i++)
    {
        const nvram_tamper_flash_slot_t *s = &simulated_tamper_slots[i];
        if (s->magic == NVRAM_TAMPER_SNAP_MAGIC &&
            s->crc32 == nvram_calc_crc32((const uint8_t *)s, offsetof(nvram_tamper_flash_slot_t, crc32)))
        {
            if (best == NULL || s->seq >= best->seq)
            {
                best = s;
            }
        }
    }
    if (best == NULL) return false;
    s_tamper_seq = best->seq;
    *tamper_ctx  = best->ctx;
    return true;
#endif
}

bool nvram_save_energy_checkpoint(const nvram_energy_checkpoint_t *checkpoint)
{
    if (checkpoint == NULL) return false;

    uint32_t slot = s_cp_next_slot % NVRAM_CHECKPOINT_TOTAL_SLOTS;
    uint32_t sector_idx = slot / NVRAM_CHECKPOINT_PAGES_PER_SECTOR;
    uint32_t page_idx   = slot % NVRAM_CHECKPOINT_PAGES_PER_SECTOR;

    s_cp_seq++;
    nvram_checkpoint_flash_slot_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.magic     = NVRAM_ENERGY_CHECKPOINT_MAGIC;
    rec.seq       = s_cp_seq;
    rec.cp        = *checkpoint;
    rec.cp.magic  = NVRAM_ENERGY_CHECKPOINT_MAGIC;
    rec.crc32     = nvram_calc_crc32((const uint8_t *)&rec, offsetof(nvram_checkpoint_flash_slot_t, crc32));

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t sector_addr = SPI_FLASH_SECTOR_CHECKPOINT_RING + (sector_idx * SPI_FLASH_SECTOR_SIZE_BYTES);
    uint32_t write_addr  = sector_addr + (page_idx * SPI_FLASH_PAGE_SIZE_BYTES);
    if (page_idx == 0U)
    {
        if (!spi2_flash_erase_sector(sector_addr))
        {
            return false;
        }
    }
    if (!spi2_flash_write_bytes(write_addr, (const uint8_t *)&rec, (uint16_t)sizeof(rec)))
    {
        return false;
    }
#else
    if (page_idx == 0U)
    {
        memset(&simulated_checkpoint_ring[sector_idx * NVRAM_CHECKPOINT_PAGES_PER_SECTOR],
               0xFF,
               NVRAM_CHECKPOINT_PAGES_PER_SECTOR * sizeof(nvram_checkpoint_flash_slot_t));
        s_cp_erase_count++;
    }
    simulated_checkpoint_ring[slot] = rec;
#endif
    s_cp_next_slot = (slot + 1U) % NVRAM_CHECKPOINT_TOTAL_SLOTS;
    return true;
}

bool nvram_load_energy_checkpoint(nvram_energy_checkpoint_t *out_checkpoint)
{
    if (out_checkpoint == NULL) return false;

    uint32_t best_seq  = 0U;
    uint32_t best_slot = 0U;
    bool     found     = false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    nvram_energy_checkpoint_t best_cp = {0};
    for (uint32_t s = 0U; s < NVRAM_CHECKPOINT_TOTAL_SLOTS; s++)
    {
        uint32_t addr = SPI_FLASH_SECTOR_CHECKPOINT_RING +
                        ((s / NVRAM_CHECKPOINT_PAGES_PER_SECTOR) * SPI_FLASH_SECTOR_SIZE_BYTES) +
                        ((s % NVRAM_CHECKPOINT_PAGES_PER_SECTOR) * SPI_FLASH_PAGE_SIZE_BYTES);
        nvram_checkpoint_flash_slot_t rd = {0};
        if (spi2_flash_read_bytes(addr, (uint8_t *)&rd, (uint16_t)sizeof(rd)) &&
            rd.magic == NVRAM_ENERGY_CHECKPOINT_MAGIC &&
            rd.cp.magic == NVRAM_ENERGY_CHECKPOINT_MAGIC &&
            rd.crc32 == nvram_calc_crc32((const uint8_t *)&rd, offsetof(nvram_checkpoint_flash_slot_t, crc32)))
        {
            if (!found || rd.seq >= best_seq)
            {
                found     = true;
                best_seq  = rd.seq;
                best_slot = s;
                best_cp   = rd.cp;
            }
        }
    }
    if (!found) return false;

    s_cp_seq = best_seq;
    uint32_t next = (best_slot + 1U) % NVRAM_CHECKPOINT_TOTAL_SLOTS;
    if ((next % NVRAM_CHECKPOINT_PAGES_PER_SECTOR) != 0U)
    {
        uint32_t next_addr = SPI_FLASH_SECTOR_CHECKPOINT_RING +
                             ((next / NVRAM_CHECKPOINT_PAGES_PER_SECTOR) * SPI_FLASH_SECTOR_SIZE_BYTES) +
                             ((next % NVRAM_CHECKPOINT_PAGES_PER_SECTOR) * SPI_FLASH_PAGE_SIZE_BYTES);
        nvram_checkpoint_flash_slot_t next_rd = {0};
        if (!spi2_flash_read_bytes(next_addr, (uint8_t *)&next_rd, (uint16_t)sizeof(next_rd)) ||
            (next_rd.magic != 0xFFFFFFFFU && next_rd.magic != 0U))
        {
            next = (((next / NVRAM_CHECKPOINT_PAGES_PER_SECTOR) + 1U) * NVRAM_CHECKPOINT_PAGES_PER_SECTOR) % NVRAM_CHECKPOINT_TOTAL_SLOTS;
        }
    }
    s_cp_next_slot = next;
    *out_checkpoint = best_cp;
    return true;
#else
    for (uint32_t s = 0U; s < NVRAM_CHECKPOINT_TOTAL_SLOTS; s++)
    {
        const nvram_checkpoint_flash_slot_t *rec = &simulated_checkpoint_ring[s];
        if (rec->magic == NVRAM_ENERGY_CHECKPOINT_MAGIC &&
            rec->cp.magic == NVRAM_ENERGY_CHECKPOINT_MAGIC &&
            rec->crc32 == nvram_calc_crc32((const uint8_t *)rec, offsetof(nvram_checkpoint_flash_slot_t, crc32)))
        {
            if (!found || rec->seq >= best_seq)
            {
                found     = true;
                best_seq  = rec->seq;
                best_slot = s;
            }
        }
    }
    if (!found) return false;

    s_cp_seq = best_seq;
    uint32_t next = (best_slot + 1U) % NVRAM_CHECKPOINT_TOTAL_SLOTS;
    if ((next % NVRAM_CHECKPOINT_PAGES_PER_SECTOR) != 0U &&
        simulated_checkpoint_ring[next].magic != 0xFFFFFFFFU &&
        simulated_checkpoint_ring[next].magic != 0U)
    {
        next = (((next / NVRAM_CHECKPOINT_PAGES_PER_SECTOR) + 1U) * NVRAM_CHECKPOINT_PAGES_PER_SECTOR) % NVRAM_CHECKPOINT_TOTAL_SLOTS;
    }
    s_cp_next_slot = next;
    *out_checkpoint = simulated_checkpoint_ring[best_slot].cp;
    return true;
#endif
}

bool nvram_save_device_config(const nvram_device_config_t *config)
{
    if (config == NULL) return false;

    s_dev_cfg_seq++;
    uint32_t bank = s_dev_cfg_seq & 1U;
    nvram_dev_cfg_flash_slot_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.magic     = NVRAM_DEVICE_CONFIG_MAGIC;
    rec.seq       = s_dev_cfg_seq;
    rec.cfg       = *config;
    rec.cfg.magic = NVRAM_DEVICE_CONFIG_MAGIC;
    rec.crc32     = nvram_calc_crc32((const uint8_t *)&rec, offsetof(nvram_dev_cfg_flash_slot_t, crc32));

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t sector = (bank == 0U) ? SPI_FLASH_SECTOR_DEVICE_CONFIG : SPI_FLASH_SECTOR_DEVICE_CONFIG_B;
    if (!spi2_flash_erase_sector(sector))
    {
        return false;
    }
    return spi2_flash_write_bytes(sector, (const uint8_t *)&rec, (uint16_t)sizeof(rec));
#else
    simulated_dev_cfg_slots[bank] = rec;
    return true;
#endif
}

bool nvram_load_device_config(nvram_device_config_t *out_config)
{
    if (out_config == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    static const uint32_t banks[2] = { SPI_FLASH_SECTOR_DEVICE_CONFIG, SPI_FLASH_SECTOR_DEVICE_CONFIG_B };
    nvram_dev_cfg_flash_slot_t best = {0};
    bool found = false;

    for (size_t i = 0U; i < 2U; i++)
    {
        nvram_dev_cfg_flash_slot_t rd = {0};
        if (spi2_flash_read_bytes(banks[i], (uint8_t *)&rd, (uint16_t)sizeof(rd)) &&
            rd.magic == NVRAM_DEVICE_CONFIG_MAGIC &&
            rd.cfg.magic == NVRAM_DEVICE_CONFIG_MAGIC &&
            rd.crc32 == nvram_calc_crc32((const uint8_t *)&rd, offsetof(nvram_dev_cfg_flash_slot_t, crc32)))
        {
            if (!found || rd.seq >= best.seq)
            {
                found = true;
                best  = rd;
            }
        }
    }
    if (!found) return false;
    s_dev_cfg_seq = best.seq;
    *out_config   = best.cfg;
    return true;
#else
    const nvram_dev_cfg_flash_slot_t *best = NULL;
    for (size_t i = 0U; i < 2U; i++)
    {
        const nvram_dev_cfg_flash_slot_t *s = &simulated_dev_cfg_slots[i];
        if (s->magic == NVRAM_DEVICE_CONFIG_MAGIC &&
            s->cfg.magic == NVRAM_DEVICE_CONFIG_MAGIC &&
            s->crc32 == nvram_calc_crc32((const uint8_t *)s, offsetof(nvram_dev_cfg_flash_slot_t, crc32)))
        {
            if (best == NULL || s->seq >= best->seq)
            {
                best = s;
            }
        }
    }
    if (best == NULL) return false;
    s_dev_cfg_seq = best->seq;
    *out_config = best->cfg;
    return true;
#endif
}

bool nvram_save_calibration(const nvram_calibration_data_t *cal)
{
    if (cal == NULL) return false;

    nvram_calibration_data_t cal_copy = *cal;
    cal_copy.magic = NVRAM_CALIBRATION_MAGIC;

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

    s_cal_seq++;
    uint32_t bank = s_cal_seq & 1U;
    nvram_cal_flash_slot_t rec;
    memset(&rec, 0, sizeof(rec));
    rec.magic = NVRAM_CALIBRATION_MAGIC;
    rec.seq   = s_cal_seq;
    rec.cal   = cal_copy;
    rec.crc32 = nvram_calc_crc32((const uint8_t *)&rec, offsetof(nvram_cal_flash_slot_t, crc32));

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t sector = (bank == 0U) ? SPI_FLASH_SECTOR_CALIBRATION : SPI_FLASH_SECTOR_CALIBRATION_B;
    if (!spi2_flash_erase_sector(sector))
    {
        return false;
    }
    return spi2_flash_write_bytes(sector, (const uint8_t *)&rec, (uint16_t)sizeof(rec));
#else
    simulated_cal_slots[bank] = rec;
    return true;
#endif
}

bool nvram_load_calibration(nvram_calibration_data_t *out_cal)
{
    if (out_cal == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    static const uint32_t banks[2] = { SPI_FLASH_SECTOR_CALIBRATION, SPI_FLASH_SECTOR_CALIBRATION_B };
    nvram_cal_flash_slot_t best = {0};
    bool found = false;

    for (size_t i = 0U; i < 2U; i++)
    {
        nvram_cal_flash_slot_t rd = {0};
        if (spi2_flash_read_bytes(banks[i], (uint8_t *)&rd, (uint16_t)sizeof(rd)) &&
            rd.magic == NVRAM_CALIBRATION_MAGIC &&
            rd.cal.magic == NVRAM_CALIBRATION_MAGIC &&
            rd.crc32 == nvram_calc_crc32((const uint8_t *)&rd, offsetof(nvram_cal_flash_slot_t, crc32)))
        {
            if (!found || rd.seq >= best.seq)
            {
                found = true;
                best  = rd;
            }
        }
    }
    if (!found) return false;
    s_cal_seq = best.seq;
    *out_cal  = best.cal;
    return true;
#else
    const nvram_cal_flash_slot_t *best = NULL;
    for (size_t i = 0U; i < 2U; i++)
    {
        const nvram_cal_flash_slot_t *s = &simulated_cal_slots[i];
        if (s->magic == NVRAM_CALIBRATION_MAGIC &&
            s->cal.magic == NVRAM_CALIBRATION_MAGIC &&
            s->crc32 == nvram_calc_crc32((const uint8_t *)s, offsetof(nvram_cal_flash_slot_t, crc32)))
        {
            if (best == NULL || s->seq >= best->seq)
            {
                best = s;
            }
        }
    }
    if (best == NULL) return false;
    s_cal_seq = best->seq;
    *out_cal = best->cal;
    return true;
#endif
}

bool nvram_w25q64_read_jedec_id(uint8_t *out_mfg, uint8_t *out_type, uint8_t *out_cap)
{
    if (out_mfg == NULL || out_type == NULL || out_cap == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    (void)spi2_flash_wait_busy(50U);

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
    *out_mfg  = W25Q64_JEDEC_MANUFACTURER_ID;
    *out_type = W25Q64_JEDEC_MEMORY_TYPE;
    *out_cap  = W25Q64_JEDEC_CAPACITY_ID;
    return true;
#endif
}

bool nvram_flash_verify(void)
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
            (cap == W25Q64_JEDEC_CAPACITY_ID || cap == W25Q128_JEDEC_CAPACITY_ID));
}

bool nvram_w25q64_verify(void)
{
    return nvram_flash_verify();
}

bool nvram_has_hardware_fault(void)
{
    return s_nvram_hw_fault;
}

void nvram_clear_hardware_fault(void)
{
    s_nvram_hw_fault = false;
}

#if !defined(EMBEDDED_HARDWARE_TARGET)
void nvram_test_simulate_virgin_flash(void)
{
    memset(simulated_load_profile_flash, 0xFF, sizeof(simulated_load_profile_flash));
    memset(simulated_tamper_slots,       0xFF, sizeof(simulated_tamper_slots));
    memset(simulated_checkpoint_ring,    0xFF, sizeof(simulated_checkpoint_ring));
    memset(simulated_dev_cfg_slots,      0xFF, sizeof(simulated_dev_cfg_slots));
    memset(simulated_cal_slots,          0xFF, sizeof(simulated_cal_slots));
    s_cp_seq = 0U;
    s_cp_next_slot = 0U;
    s_cp_erase_count = 0U;
    s_tamper_seq = 0U;
    s_dev_cfg_seq = 0U;
    s_cal_seq = 0U;
    s_nvram_hw_fault = false;
}

uint32_t nvram_test_get_checkpoint_erase_count(void)
{
    return s_cp_erase_count;
}
#endif
