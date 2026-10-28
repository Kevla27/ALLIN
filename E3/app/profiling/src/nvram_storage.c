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
        (void)HAL_SPI_Transmit(&hspi2, (uint8_t *)data, len, 10U);
    }
    HAL_GPIO_WritePin(FLASH_SPI2_CS_GPIO_Port, FLASH_SPI2_CS_Pin, GPIO_PIN_SET);
}
#endif

/* Buffer NVRAM Backing Store untuk menjamin integritas baca/tulis pada Spin 1 & Unit Test */
static load_profile_entry_t simulated_load_profile_flash[LOAD_PROFILE_MAX_ENTRIES];
static tamper_context_t     simulated_tamper_log_flash;

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

    *tamper_ctx = simulated_tamper_log_flash;
    return true;
}
