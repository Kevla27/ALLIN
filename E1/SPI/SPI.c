#include <stdio.h>
#include <stdint.h>

#include "SPI.h"
#include "../Mock/Mock.h"
#include "../RegisterMap/RegisterMap.h"

#if defined(EMBEDDED_HARDWARE_TARGET)
#include "main.h"
extern SPI_HandleTypeDef hspi1;
static bool s_use_hw_spi = false;
#endif

#if defined(EMBEDDED_HARDWARE_TARGET) && !defined(ADE9000_SPI_VERBOSE_DEBUG)
#define SPI_DEBUG_PRINTF(...) ((void)0)
#else
#define SPI_DEBUG_PRINTF(...) printf(__VA_ARGS__)
#endif

/* =========================================================
 * BUILD COMMAND HEADER
 * ========================================================= */

uint16_t ADE9000_SPI_BuildCommand(uint16_t address,
                                  uint8_t read)
{
    uint16_t command;

    command = (uint16_t)((address & 0x0FFFU) << 4);

    if (read)
    {
        command |= ADE9000_SPI_READ_BIT;
    }

    return command;
}


/* =========================================================
 * CRC-16-CCITT
 * ========================================================= */

static uint16_t CRC16_CCITT_Update(uint16_t crc,
                                   uint8_t data)
{
    uint8_t i;

    crc ^= ((uint16_t)data << 8);

    for (i = 0; i < 8; i++)
    {
        if (crc & 0x8000U)
        {
            crc = (uint16_t)((crc << 1) ^ 0x1021U);
        }
        else
        {
            crc <<= 1;
        }
    }

    return crc;
}


uint16_t ADE9000_SPI_CalculateCRC16(uint32_t data,
                                    uint8_t data_size)
{
    uint16_t crc = 0xFFFFU;

    if (data_size == ADE9000_SPI_DATA_32BIT)
    {
        crc = CRC16_CCITT_Update(crc, (uint8_t)((data >> 24) & 0xFFU));
        crc = CRC16_CCITT_Update(crc, (uint8_t)((data >> 16) & 0xFFU));
        crc = CRC16_CCITT_Update(crc, (uint8_t)((data >> 8) & 0xFFU));
        crc = CRC16_CCITT_Update(crc, (uint8_t)(data & 0xFFU));
    }
    else if (data_size == ADE9000_SPI_DATA_16BIT)
    {
        crc = CRC16_CCITT_Update(crc, (uint8_t)((data >> 8) & 0xFFU));
        crc = CRC16_CCITT_Update(crc, (uint8_t)(data & 0xFFU));
    }

    return crc;
}


bool ADE9000_SPI_CheckCRC16(uint32_t data,
                            uint8_t data_size,
                            uint16_t received_crc)
{
    uint16_t calculated_crc = ADE9000_SPI_CalculateCRC16(data, data_size);
    return calculated_crc == received_crc;
}


#if defined(EMBEDDED_HARDWARE_TARGET)
/* =========================================================
 * HARDWARE SPI1 LOW-LEVEL HELPERS (PA4=SS, PA5=SCK, PA6=MISO, PA7=MOSI)
 * ========================================================= */
static bool ADE9000_HW_SPI_Read(uint16_t command, uint8_t data_size, uint32_t *data, uint16_t *crc)
{
    uint8_t cmd_buf[2] = { (uint8_t)(command >> 8), (uint8_t)(command & 0xFFU) };
    uint8_t rx_buf[6]  = { 0 };
    uint16_t rx_len    = (uint16_t)(data_size + 2U); /* data (2 or 4 bytes) + 16-bit CRC */

    HAL_GPIO_WritePin(ADE_NSS_GPIO_Port, ADE_NSS_Pin, GPIO_PIN_RESET);
    if (HAL_SPI_Transmit(&hspi1, cmd_buf, 2, 10) != HAL_OK ||
        HAL_SPI_Receive(&hspi1, rx_buf, rx_len, 10) != HAL_OK)
    {
        HAL_GPIO_WritePin(ADE_NSS_GPIO_Port, ADE_NSS_Pin, GPIO_PIN_SET);
        return false;
    }
    HAL_GPIO_WritePin(ADE_NSS_GPIO_Port, ADE_NSS_Pin, GPIO_PIN_SET);

    if (data_size == ADE9000_SPI_DATA_32BIT)
    {
        *data = ((uint32_t)rx_buf[0] << 24) |
                ((uint32_t)rx_buf[1] << 16) |
                ((uint32_t)rx_buf[2] << 8)  |
                (uint32_t)rx_buf[3];
        if (crc != NULL)
        {
            *crc = (uint16_t)(((uint16_t)rx_buf[4] << 8) | rx_buf[5]);
        }
    }
    else
    {
        *data = ((uint32_t)rx_buf[0] << 8) | (uint32_t)rx_buf[1];
        if (crc != NULL)
        {
            *crc = (uint16_t)(((uint16_t)rx_buf[2] << 8) | rx_buf[3]);
        }
    }
    return true;
}

static bool ADE9000_HW_SPI_Write(uint16_t command, uint8_t data_size, uint32_t data)
{
    uint8_t tx_buf[6];
    uint16_t tx_len;

    tx_buf[0] = (uint8_t)(command >> 8);
    tx_buf[1] = (uint8_t)(command & 0xFFU);

    if (data_size == ADE9000_SPI_DATA_32BIT)
    {
        tx_buf[2] = (uint8_t)((data >> 24) & 0xFFU);
        tx_buf[3] = (uint8_t)((data >> 16) & 0xFFU);
        tx_buf[4] = (uint8_t)((data >> 8) & 0xFFU);
        tx_buf[5] = (uint8_t)(data & 0xFFU);
        tx_len = 6U;
    }
    else
    {
        tx_buf[2] = (uint8_t)((data >> 8) & 0xFFU);
        tx_buf[3] = (uint8_t)(data & 0xFFU);
        tx_len = 4U;
    }

    HAL_GPIO_WritePin(ADE_NSS_GPIO_Port, ADE_NSS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef st = HAL_SPI_Transmit(&hspi1, tx_buf, tx_len, 10);
    HAL_GPIO_WritePin(ADE_NSS_GPIO_Port, ADE_NSS_Pin, GPIO_PIN_SET);

    return (st == HAL_OK);
}
#endif


/* =========================================================
 * SPI INITIALIZATION
 * ========================================================= */

void ADE9000_SPI_Init(void)
{
#if defined(EMBEDDED_HARDWARE_TARGET)
    HAL_GPIO_WritePin(ADE_NSS_GPIO_Port, ADE_NSS_Pin, GPIO_PIN_SET);
    uint32_t part_id = 0;
    uint16_t crc     = 0;
    uint16_t cmd     = ADE9000_SPI_BuildCommand(ADE9000_PART_ID, ADE9000_SPI_READ);

    if (ADE9000_HW_SPI_Read(cmd, ADE9000_SPI_DATA_32BIT, &part_id, &crc) &&
        part_id != 0x00000000UL && part_id != 0xFFFFFFFFUL &&
        (part_id & ADE9000_PART_ID_MASK) != 0UL)
    {
        s_use_hw_spi = true;
        printf("ADE9000 SPI transaction layer initialized.\n");
        printf("Mode : HARDWARE SPI1 (PART_ID=0x%08lX)\n\n", (unsigned long)part_id);
        return;
    }
    s_use_hw_spi = false;
#endif
    printf("ADE9000 SPI transaction layer initialized.\n");
    printf("Mode : HOST MOCK\n");
    printf("SPI  : ADE9000 protocol simulation\n\n");
}


/* =========================================================
 * SPI READ TRANSACTION
 * ========================================================= */

ADE9000_SPI_Status ADE9000_SPI_TransactionRead(
    uint16_t address,
    uint8_t data_size,
    uint32_t *data,
    uint16_t *crc)
{
    uint16_t command;
    uint32_t rx_data = 0;
    uint16_t rx_crc  = 0;

    if (data == NULL)
    {
        return ADE9000_SPI_ERROR;
    }

    if ((data_size != ADE9000_SPI_DATA_16BIT) &&
        (data_size != ADE9000_SPI_DATA_32BIT))
    {
        return ADE9000_SPI_INVALID_SIZE;
    }

    command = ADE9000_SPI_BuildCommand(address, ADE9000_SPI_READ);

    SPI_DEBUG_PRINTF("[SPI READ]\n");
    SPI_DEBUG_PRINTF("  SS   = LOW\n");
    SPI_DEBUG_PRINTF("  CMD  = 0x%04X\n", command);
    SPI_DEBUG_PRINTF("  ADDR = 0x%03X\n", address);

#if defined(EMBEDDED_HARDWARE_TARGET)
    bool ok = s_use_hw_spi
              ? ADE9000_HW_SPI_Read(command, data_size, &rx_data, &rx_crc)
              : ADE9000_Mock_SPI_Read(command, data_size, &rx_data, &rx_crc);
#else
    bool ok = ADE9000_Mock_SPI_Read(command, data_size, &rx_data, &rx_crc);
#endif

    if (!ok)
    {
        SPI_DEBUG_PRINTF("  Transaction failed.\n");
        SPI_DEBUG_PRINTF("  SS   = HIGH\n\n");
        return ADE9000_SPI_ERROR;
    }

    *data = rx_data;
    SPI_DEBUG_PRINTF("  DATA = 0x%08lX\n", (unsigned long)rx_data);

    if (crc != NULL)
    {
        *crc = rx_crc;
        SPI_DEBUG_PRINTF("  CRC  = 0x%04X\n", rx_crc);

        if (!ADE9000_SPI_CheckCRC16(rx_data, data_size, rx_crc))
        {
            SPI_DEBUG_PRINTF("  CRC  = ERROR\n");
            SPI_DEBUG_PRINTF("  SS   = HIGH\n\n");
            return ADE9000_SPI_CRC_ERROR;
        }
        SPI_DEBUG_PRINTF("  CRC  = OK\n");
    }

    SPI_DEBUG_PRINTF("  SS   = HIGH\n\n");
    return ADE9000_SPI_OK;
}


/* =========================================================
 * SPI WRITE TRANSACTION
 * ========================================================= */

ADE9000_SPI_Status ADE9000_SPI_TransactionWrite(
    uint16_t address,
    uint8_t data_size,
    uint32_t data)
{
    uint16_t command;

    if ((data_size != ADE9000_SPI_DATA_16BIT) &&
        (data_size != ADE9000_SPI_DATA_32BIT))
    {
        return ADE9000_SPI_INVALID_SIZE;
    }

    command = ADE9000_SPI_BuildCommand(address, ADE9000_SPI_WRITE);

    SPI_DEBUG_PRINTF("[SPI WRITE]\n");
    SPI_DEBUG_PRINTF("  SS   = LOW\n");
    SPI_DEBUG_PRINTF("  CMD  = 0x%04X\n", command);
    SPI_DEBUG_PRINTF("  ADDR = 0x%03X\n", address);
    SPI_DEBUG_PRINTF("  DATA = 0x%08lX\n", (unsigned long)data);

#if defined(EMBEDDED_HARDWARE_TARGET)
    bool ok = s_use_hw_spi
              ? ADE9000_HW_SPI_Write(command, data_size, data)
              : ADE9000_Mock_SPI_Write(command, data_size, data);
#else
    bool ok = ADE9000_Mock_SPI_Write(command, data_size, data);
#endif

    if (!ok)
    {
        SPI_DEBUG_PRINTF("  Transaction failed.\n");
        SPI_DEBUG_PRINTF("  SS   = HIGH\n\n");
        return ADE9000_SPI_ERROR;
    }

    SPI_DEBUG_PRINTF("  SS   = HIGH\n");
    SPI_DEBUG_PRINTF("  WRITE = OK\n\n");
    return ADE9000_SPI_OK;
}


/* =========================================================
 * REGISTER READ
 * ========================================================= */

uint32_t ADE9000_SPI_ReadRegister(uint16_t address)
{
    uint32_t data;
    uint16_t crc;

    if (ADE9000_SPI_TransactionRead(
            address,
            ADE9000_SPI_DATA_32BIT,
            &data,
            &crc) != ADE9000_SPI_OK)
    {
        return 0;
    }

    return data;
}


/* =========================================================
 * REGISTER WRITE
 * ========================================================= */

void ADE9000_SPI_WriteRegister(uint16_t address,
                               uint32_t value)
{
    ADE9000_SPI_TransactionWrite(
        address,
        ADE9000_SPI_DATA_32BIT,
        value);
}


/* =========================================================
 * BURST READ
 * ========================================================= */

ADE9000_SPI_Status ADE9000_SPI_BurstRead(
    uint16_t start_address,
    uint32_t *buffer,
    uint16_t count)
{
    uint16_t command;
    uint16_t i;

    if (buffer == NULL || count == 0)
    {
        return ADE9000_SPI_ERROR;
    }

    command = ADE9000_SPI_BuildCommand(start_address, ADE9000_SPI_READ);

    SPI_DEBUG_PRINTF("[SPI BURST READ]\n");
    SPI_DEBUG_PRINTF("  SS   = LOW\n");
    SPI_DEBUG_PRINTF("  CMD  = 0x%04X\n", command);
    SPI_DEBUG_PRINTF("  START= 0x%03X\n", start_address);

#if defined(EMBEDDED_HARDWARE_TARGET)
    if (s_use_hw_spi)
    {
        uint8_t cmd_buf[2] = { (uint8_t)(command >> 8), (uint8_t)(command & 0xFFU) };
        HAL_GPIO_WritePin(ADE_NSS_GPIO_Port, ADE_NSS_Pin, GPIO_PIN_RESET);
        if (HAL_SPI_Transmit(&hspi1, cmd_buf, 2, 10) != HAL_OK)
        {
            HAL_GPIO_WritePin(ADE_NSS_GPIO_Port, ADE_NSS_Pin, GPIO_PIN_SET);
            return ADE9000_SPI_ERROR;
        }
        for (i = 0; i < count; i++)
        {
            uint8_t rx_word[4] = {0};
            if (HAL_SPI_Receive(&hspi1, rx_word, 4, 10) != HAL_OK)
            {
                HAL_GPIO_WritePin(ADE_NSS_GPIO_Port, ADE_NSS_Pin, GPIO_PIN_SET);
                return ADE9000_SPI_ERROR;
            }
            buffer[i] = ((uint32_t)rx_word[0] << 24) |
                        ((uint32_t)rx_word[1] << 16) |
                        ((uint32_t)rx_word[2] << 8)  |
                        (uint32_t)rx_word[3];
        }
        HAL_GPIO_WritePin(ADE_NSS_GPIO_Port, ADE_NSS_Pin, GPIO_PIN_SET);
        return ADE9000_SPI_OK;
    }
#endif

    if (!ADE9000_Mock_SPI_BurstRead(command, buffer, count))
    {
        SPI_DEBUG_PRINTF("  Burst transaction failed.\n");
        SPI_DEBUG_PRINTF("  SS   = HIGH\n\n");
        return ADE9000_SPI_ERROR;
    }

    for (i = 0; i < count; i++)
    {
        SPI_DEBUG_PRINTF("  DATA[%u] = 0x%08lX\n", i, (unsigned long)buffer[i]);
    }

    SPI_DEBUG_PRINTF("  SS   = HIGH\n");
    SPI_DEBUG_PRINTF("  BURST  = OK\n\n");

    return ADE9000_SPI_OK;
}
