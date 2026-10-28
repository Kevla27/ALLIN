#include <stdio.h>
#include <stdint.h>

#include "SPI.h"
#include "../Mock/Mock.h"

/* =========================================================
 * SPI INITIALIZATION
 * ========================================================= */

void ADE9000_SPI_Init(void)
{
    printf("ADE9000 SPI transaction layer initialized.\n");
    printf("Mode : HOST MOCK\n");
    printf("SPI  : ADE9000 protocol simulation\n\n");
}


/* =========================================================
 * BUILD COMMAND HEADER
 * ========================================================= */

/*
 * ADE9000 CMD_HDR:
 *
 * bit 15 ........ 4 : Address
 * bit 3             : Read/Write
 * bit 2 ........ 0  : Don't care
 *
 * Read  -> bit 3 = 1
 * Write -> bit 3 = 0
 */

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

/*
 * ADE9000 uses CRC-16-CCITT.
 *
 * Polynomial:
 *
 * G(x) = x^16 + x^12 + x^5 + 1
 *
 * Polynomial = 0x1021
 *
 * Initial state:
 * all 16 bits = 1
 * therefore:
 *
 * CRC initial value = 0xFFFF
 *
 * Data is processed MSB first.
 */

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
        crc = CRC16_CCITT_Update(
            crc,
            (uint8_t)((data >> 24) & 0xFFU));

        crc = CRC16_CCITT_Update(
            crc,
            (uint8_t)((data >> 16) & 0xFFU));

        crc = CRC16_CCITT_Update(
            crc,
            (uint8_t)((data >> 8) & 0xFFU));

        crc = CRC16_CCITT_Update(
            crc,
            (uint8_t)(data & 0xFFU));
    }
    else if (data_size == ADE9000_SPI_DATA_16BIT)
    {
        crc = CRC16_CCITT_Update(
            crc,
            (uint8_t)((data >> 8) & 0xFFU));

        crc = CRC16_CCITT_Update(
            crc,
            (uint8_t)(data & 0xFFU));
    }

    return crc;
}


bool ADE9000_SPI_CheckCRC16(uint32_t data,
                            uint8_t data_size,
                            uint16_t received_crc)
{
    uint16_t calculated_crc;

    calculated_crc =
        ADE9000_SPI_CalculateCRC16(data, data_size);

    return calculated_crc == received_crc;
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
    uint32_t mock_data;
    uint16_t mock_crc;

    if (data == NULL)
    {
        return ADE9000_SPI_ERROR;
    }

    if ((data_size != ADE9000_SPI_DATA_16BIT) &&
        (data_size != ADE9000_SPI_DATA_32BIT))
    {
        return ADE9000_SPI_INVALID_SIZE;
    }


    /*
     * STEP 1
     * Build CMD_HDR
     */

    command = ADE9000_SPI_BuildCommand(
        address,
        ADE9000_SPI_READ);


    printf("[SPI READ]\n");
    printf("  SS   = LOW\n");
    printf("  CMD  = 0x%04X\n", command);
    printf("  ADDR = 0x%03X\n", address);


    /*
     * STEP 2
     * Send command to mock device
     */

    if (!ADE9000_Mock_SPI_Read(
            command,
            data_size,
            &mock_data,
            &mock_crc))
    {
        printf("  Transaction failed.\n");
        printf("  SS   = HIGH\n\n");

        return ADE9000_SPI_ERROR;
    }


    /*
     * STEP 3
     * Receive register data
     */

    *data = mock_data;

    printf("  DATA = 0x%08lX\n",
           (unsigned long)mock_data);


    /*
     * STEP 4
     * Receive CRC
     */

    if (crc != NULL)
    {
        *crc = mock_crc;

        printf("  CRC  = 0x%04X\n",
               mock_crc);


        /*
         * Check CRC
         */

        if (!ADE9000_SPI_CheckCRC16(
                mock_data,
                data_size,
                mock_crc))
        {
            printf("  CRC  = ERROR\n");
            printf("  SS   = HIGH\n\n");

            return ADE9000_SPI_CRC_ERROR;
        }

        printf("  CRC  = OK\n");
    }


    /*
     * STEP 5
     * Finish transaction
     */

    printf("  SS   = HIGH\n\n");

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


    /*
     * Build WRITE command
     */

    command = ADE9000_SPI_BuildCommand(
        address,
        ADE9000_SPI_WRITE);


    printf("[SPI WRITE]\n");
    printf("  SS   = LOW\n");
    printf("  CMD  = 0x%04X\n", command);
    printf("  ADDR = 0x%03X\n", address);
    printf("  DATA = 0x%08lX\n",
           (unsigned long)data);


    /*
     * Send transaction to mock
     */

    if (!ADE9000_Mock_SPI_Write(
            command,
            data_size,
            data))
    {
        printf("  Transaction failed.\n");
        printf("  SS   = HIGH\n\n");

        return ADE9000_SPI_ERROR;
    }


    /*
     * ADE9000 does not perform CRC checking
     * as part of SPI write.
     *
     * Verification is done by read-back.
     */

    printf("  SS   = HIGH\n");
    printf("  WRITE = OK\n\n");

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


    /*
     * Build only ONE command header.
     *
     * In a real ADE9000 burst transaction,
     * SS remains LOW while multiple register
     * values are clocked out.
     */

    command = ADE9000_SPI_BuildCommand(
        start_address,
        ADE9000_SPI_READ);


    printf("[SPI BURST READ]\n");
    printf("  SS   = LOW\n");
    printf("  CMD  = 0x%04X\n", command);
    printf("  START= 0x%03X\n", start_address);


    /*
     * Host-side mock:
     *
     * simulate consecutive register data.
     */

    if (!ADE9000_Mock_SPI_BurstRead(
            command,
            buffer,
            count))
    {
        printf("  Burst transaction failed.\n");
        printf("  SS   = HIGH\n\n");

        return ADE9000_SPI_ERROR;
    }


    for (i = 0; i < count; i++)
    {
        printf("  DATA[%u] = 0x%08lX\n",
               i,
               (unsigned long)buffer[i]);
    }


    printf("  SS   = HIGH\n");
    printf("  BURST  = OK\n\n");

    return ADE9000_SPI_OK;
}