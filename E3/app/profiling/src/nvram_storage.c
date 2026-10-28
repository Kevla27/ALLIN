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

#define SPI_FLASH_CMD_WREN  0x06U
#define SPI_FLASH_CMD_PP    0x02U
#define SPI_FLASH_CMD_READ  0x03U

static void spi2_flash_write_bytes(uint32_t flash_offset, const uint8_t *data, uint16_t len)
{
    uint8_t wren = SPI_FLASH_CMD_WREN;
    uint8_t hdr[4] = {
        SPI_FLASH_CMD_PP,
        (uint8_t)((flash_offset >> 16) & 0xFFU),
        (uint8_t)((flash_offset >> 8) & 0xFFU),
        (uint8_t)(flash_offset & 0xFFU)
    };

    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
    (void)HAL_SPI_Transmit(&hspi2, &wren, 1U, 5U);
    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(&hspi2, hdr, 4U, 5U) == HAL_OK)
    {
        (void)HAL_SPI_Transmit(&hspi2, (uint8_t *)data, len, 20U);
    }
    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);
}

static void spi2_flash_read_bytes(uint32_t flash_offset, uint8_t *data, uint16_t len)
{
    if (data == NULL || len == 0U) return;

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

/* Buffer NVRAM Backing Store untuk menjamin integritas baca/tulis pada Spin 1 & Unit Test */
static load_profile_entry_t     simulated_load_profile_flash[LOAD_PROFILE_MAX_ENTRIES];
static tamper_context_t         simulated_tamper_log_flash;
static nvram_energy_checkpoint_t simulated_energy_checkpoint_flash = {0};

bool nvram_write_load_profile_entry(uint32_t entry_index, const load_profile_entry_t *entry)
{
    if (entry == NULL || entry_index >= LOAD_PROFILE_MAX_ENTRIES) return false;

    simulated_load_profile_flash[entry_index] = *entry;

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t offset = 0x004000U + (entry_index * (uint32_t)sizeof(load_profile_entry_t));
    spi2_flash_write_bytes(offset, (const uint8_t *)entry, (uint16_t)sizeof(load_profile_entry_t));
#endif
    return true;
}

bool nvram_read_load_profile_entry(uint32_t entry_index, load_profile_entry_t *out_entry)
{
    if (out_entry == NULL || entry_index >= LOAD_PROFILE_MAX_ENTRIES) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    uint32_t offset = 0x004000U + (entry_index * (uint32_t)sizeof(load_profile_entry_t));
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
    spi2_flash_write_bytes(0x002000U, (const uint8_t *)tamper_ctx, (uint16_t)sizeof(tamper_context_t));
#endif
    return true;
}

bool nvram_load_tamper_log_snapshot(tamper_context_t *tamper_ctx)
{
    if (tamper_ctx == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    tamper_context_t flash_tamper = {0};
    spi2_flash_read_bytes(0x002000U, (uint8_t *)&flash_tamper, (uint16_t)sizeof(tamper_context_t));
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
    spi2_flash_write_bytes(0x001000U, (const uint8_t *)&simulated_energy_checkpoint_flash, (uint16_t)sizeof(nvram_energy_checkpoint_t));
#endif
    return true;
}

bool nvram_load_energy_checkpoint(nvram_energy_checkpoint_t *out_checkpoint)
{
    if (out_checkpoint == NULL) return false;

#if defined(EMBEDDED_HARDWARE_TARGET)
    nvram_energy_checkpoint_t flash_cp = {0};
    spi2_flash_read_bytes(0x001000U, (uint8_t *)&flash_cp, (uint16_t)sizeof(nvram_energy_checkpoint_t));
    /* Verifikasi stempel magic sebelum memulihkan data dari SPI2 Flash fisik ke RAM mirror */
    if (flash_cp.magic == NVRAM_ENERGY_CHECKPOINT_MAGIC)
    {
        simulated_energy_checkpoint_flash = flash_cp;
    }
#endif

    *out_checkpoint = simulated_energy_checkpoint_flash;
    return (out_checkpoint->magic == NVRAM_ENERGY_CHECKPOINT_MAGIC);
}
