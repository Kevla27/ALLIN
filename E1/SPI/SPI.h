#ifndef ADE9000_SPI_H
#define ADE9000_SPI_H

#include <stdint.h>
#include <stdbool.h>

/*
 * ADE9000 SPI command header
 *
 * [15:4] = Register address
 * [3]    = Read/Write
 * [2:0]  = Don't care / internal timing bits
 */

#define ADE9000_SPI_READ_BIT       (1U << 3)

#define ADE9000_SPI_WRITE          0U
#define ADE9000_SPI_READ           1U

#define ADE9000_SPI_DATA_16BIT     2U
#define ADE9000_SPI_DATA_32BIT     4U

/* SPI transaction status */
typedef enum
{
    ADE9000_SPI_OK = 0,
    ADE9000_SPI_ERROR = -1,
    ADE9000_SPI_CRC_ERROR = -2,
    ADE9000_SPI_INVALID_SIZE = -3,
    ADE9000_SPI_INVALID_ADDRESS = -4

} ADE9000_SPI_Status;

/* Initialize SPI layer */
void ADE9000_SPI_Init(void);

/* Build ADE9000 command header */
uint16_t ADE9000_SPI_BuildCommand(uint16_t address,
                                   uint8_t read);

/* Low-level transaction */
ADE9000_SPI_Status ADE9000_SPI_TransactionRead(
    uint16_t address,
    uint8_t data_size,
    uint32_t *data,
    uint16_t *crc);

ADE9000_SPI_Status ADE9000_SPI_TransactionWrite(
    uint16_t address,
    uint8_t data_size,
    uint32_t data);

/* Register-level API */
uint32_t ADE9000_SPI_ReadRegister(uint16_t address);

void ADE9000_SPI_WriteRegister(uint16_t address,
                               uint32_t value);

/* Burst read */
ADE9000_SPI_Status ADE9000_SPI_BurstRead(
    uint16_t start_address,
    uint32_t *buffer,
    uint16_t count);

/* CRC */
uint16_t ADE9000_SPI_CalculateCRC16(uint32_t data,
                                    uint8_t data_size);

bool ADE9000_SPI_CheckCRC16(uint32_t data,
                            uint8_t data_size,
                            uint16_t received_crc);

#endif