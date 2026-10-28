#include <stdio.h>
#include <string.h>

#include "Mock.h"
#include "../RegisterMap/RegisterMap.h"


/*
 * Virtual ADE9000 device.
 */
static ADE9000_Mock_t device;


/*
 * Initialize the virtual ADE9000 device.
 */
void ADE9000_Mock_Init(void)
{
	memset(&device, 0, sizeof(device));

	printf("ADE9000 Mock initialized.\n\n");
}


/*
 * Return a pointer to the virtual ADE9000 device.
 */
ADE9000_Mock_t* ADE9000_Mock_GetDevice(void)
{
	return &device;
}


/*
 * Read a simulated register based on its address.
 */
uint32_t ADE9000_Mock_ReadRegister(uint16_t address)
{
	switch (address)
	{
		/* =========================
		 * PHASE A
		 * ========================= */

		case ADE9000_AIRMS:
			return device.AIRMS;

		case ADE9000_AVRMS:
			return device.AVRMS;

		case ADE9000_AIFRMS:
			return device.AIFRMS;

		case ADE9000_AWATT:
			return (uint32_t)device.AWATT;

		case ADE9000_AVAR:
			return (uint32_t)device.AVAR;

		case ADE9000_AVA:
			return device.AVA;

		case ADE9000_APF:
			return (uint32_t)device.APF;

		case ADE9000_AVTHD:
			return device.AVTHD;

		case ADE9000_AITHD:
			return device.AITHD;

		case ADE9000_APERIOD:
			return device.APERIOD;

		case ADE9000_AWATT_ACC_LO:
			return device.AWATT_ACC_LO;

		case ADE9000_AWATT_ACC_HI:
			return device.AWATT_ACC_HI;

		case ADE9000_AWATTHR_LO:
			return device.AWATTHR_LO;

		case ADE9000_AWATTHR_HI:
			return device.AWATTHR_HI;

        case ADE9000_AVARHR_LO:
            return device.AVARHR_LO;

        case ADE9000_AVARHR_HI:
            return device.AVARHR_HI;


		/* =========================
		 * PHASE B
		 * ========================= */

		case ADE9000_BIRMS:
			return device.BIRMS;

		case ADE9000_BVRMS:
			return device.BVRMS;

		case ADE9000_BWATT:
			return (uint32_t)device.BWATT;

		case ADE9000_BVAR:
			return (uint32_t)device.BVAR;

		case ADE9000_BVA:
			return device.BVA;

		case ADE9000_BPF:
			return (uint32_t)device.BPF;

		case ADE9000_BVTHD:
			return device.BVTHD;

		case ADE9000_BITHD:
			return device.BITHD;

		case ADE9000_BPERIOD:
			return device.BPERIOD;

		case ADE9000_BWATT_ACC_LO:
			return device.BWATT_ACC_LO;

		case ADE9000_BWATT_ACC_HI:
			return device.BWATT_ACC_HI;

		case ADE9000_BWATTHR_LO:
			return device.BWATTHR_LO;

		case ADE9000_BWATTHR_HI:
			return device.BWATTHR_HI;

        case ADE9000_BVARHR_LO:
            return device.BVARHR_LO;

        case ADE9000_BVARHR_HI:
            return device.BVARHR_HI;

		/* =========================
		 * PHASE C
		 * ========================= */

		case ADE9000_CIRMS:
			return device.CIRMS;

		case ADE9000_CVRMS:
			return device.CVRMS;

		case ADE9000_CWATT:
			return (uint32_t)device.CWATT;

		case ADE9000_CVAR:
			return (uint32_t)device.CVAR;

		case ADE9000_CVA:
			return device.CVA;

		case ADE9000_CPF:
			return (uint32_t)device.CPF;

		case ADE9000_CVTHD:
			return device.CVTHD;

		case ADE9000_CITHD:
			return device.CITHD;

		case ADE9000_CPERIOD:
			return device.CPERIOD;

		case ADE9000_CWATT_ACC_LO:
			return device.CWATT_ACC_LO;

		case ADE9000_CWATT_ACC_HI:
			return device.CWATT_ACC_HI;

		case ADE9000_CWATTHR_LO:
			return device.CWATTHR_LO;

		case ADE9000_CWATTHR_HI:
			return device.CWATTHR_HI;

        case ADE9000_CVARHR_LO:
            return device.CVARHR_LO;

        case ADE9000_CVARHR_HI:
            return device.CVARHR_HI;


		/* =========================
		 * NEUTRAL
		 * ========================= */

		case ADE9000_NIRMS:
			return device.NIRMS;


		/* =========================
		 * CONTROL
		 * ========================= */

		case ADE9000_RUN:
			return device.RUN;


		default:
			printf("Mock: unknown register 0x%03X\n", address);
			return 0;
	}
}


/*
 * Write a simulated register based on its address.
 */
void ADE9000_Mock_WriteRegister(uint16_t address,
								uint32_t value)
{
	switch (address)
	{
		case ADE9000_RUN:
			device.RUN = value;
			break;

		default:
			printf("Mock: write to unsupported register 0x%03X\n",
				   address);
			break;
	}
}


/* =========================================================
 * PHASE A SETTERS
 * ========================================================= */

void ADE9000_Mock_SetAIRMS(uint32_t value)
{
	device.AIRMS = value;
}


void ADE9000_Mock_SetAVRMS(uint32_t value)
{
	device.AVRMS = value;
}

void ADE9000_Mock_SetAIFRMS(uint32_t value)
{
    device.AIFRMS = value;
}

void ADE9000_Mock_SetAWATT(int32_t value)
{
	device.AWATT = value;
}

void ADE9000_Mock_SetAVAR(int32_t value)
{
    device.AVAR = value;
}

void ADE9000_Mock_SetAVA(uint32_t value)
{
    device.AVA = value;
}

void ADE9000_Mock_SetAPF(int32_t value)
{
    device.APF = value;
}

void ADE9000_Mock_SetAWATT_ACC_LO(uint32_t value)
{
	device.AWATT_ACC_LO = value;
}


void ADE9000_Mock_SetAWATT_ACC_HI(uint32_t value)
{
	device.AWATT_ACC_HI = value;
}


void ADE9000_Mock_SetAWATTHR_LO(uint32_t value)
{
	device.AWATTHR_LO = value;
}

void ADE9000_Mock_SetAVARHR_LO(uint32_t value)
{
    device.AVARHR_LO = value;
}

void ADE9000_Mock_SetAVARHR_HI(uint32_t value)
{
    device.AVARHR_HI = value;
}

void ADE9000_Mock_SetAWATTHR_HI(uint32_t value)
{
	device.AWATTHR_HI = value;
}


void ADE9000_Mock_SetAPERIOD(uint32_t value)
{
	device.APERIOD = value;
}


/* =========================================================
 * PHASE B SETTERS
 * ========================================================= */

void ADE9000_Mock_SetBIRMS(uint32_t value)
{
	device.BIRMS = value;
}


void ADE9000_Mock_SetBVRMS(uint32_t value)
{
	device.BVRMS = value;
}


void ADE9000_Mock_SetBWATT(int32_t value)
{
	device.BWATT = value;
}

void ADE9000_Mock_SetBVAR(int32_t value)
{
    device.BVAR = value;
}

void ADE9000_Mock_SetBVA(uint32_t value)
{
    device.BVA = value;
}

void ADE9000_Mock_SetBPF(int32_t value)
{
    device.BPF = value;
}

void ADE9000_Mock_SetBWATT_ACC_LO(uint32_t value)
{
	device.BWATT_ACC_LO = value;
}


void ADE9000_Mock_SetBWATT_ACC_HI(uint32_t value)
{
	device.BWATT_ACC_HI = value;
}


void ADE9000_Mock_SetBWATTHR_LO(uint32_t value)
{
	device.BWATTHR_LO = value;
}


void ADE9000_Mock_SetBWATTHR_HI(uint32_t value)
{
	device.BWATTHR_HI = value;
}


void ADE9000_Mock_SetBVARHR_LO(uint32_t value)
{
    device.BVARHR_LO = value;
}

void ADE9000_Mock_SetBVARHR_HI(uint32_t value)
{
    device.BVARHR_HI = value;
}


void ADE9000_Mock_SetBPERIOD(uint32_t value)
{
	device.BPERIOD = value;
}


/* =========================================================
 * PHASE C SETTERS
 * ========================================================= */

void ADE9000_Mock_SetCIRMS(uint32_t value)
{
	device.CIRMS = value;
}


void ADE9000_Mock_SetCVRMS(uint32_t value)
{
	device.CVRMS = value;
}


void ADE9000_Mock_SetCWATT(int32_t value)
{
	device.CWATT = value;
}

void ADE9000_Mock_SetCVAR(int32_t value)
{
    device.CVAR = value;
}

void ADE9000_Mock_SetCVA(uint32_t value)
{
    device.CVA = value;
}

void ADE9000_Mock_SetCPF(int32_t value)
{
    device.CPF = value;
}

void ADE9000_Mock_SetCWATT_ACC_LO(uint32_t value)
{
	device.CWATT_ACC_LO = value;
}

void ADE9000_Mock_SetCWATT_ACC_HI(uint32_t value)
{
	device.CWATT_ACC_HI = value;
}


void ADE9000_Mock_SetCWATTHR_LO(uint32_t value)
{
	device.CWATTHR_LO = value;
}

void ADE9000_Mock_SetCWATTHR_HI(uint32_t value)
{
	device.CWATTHR_HI = value;
}


void ADE9000_Mock_SetCVARHR_LO(uint32_t value)
{
    device.CVARHR_LO = value;
}

void ADE9000_Mock_SetCVARHR_HI(uint32_t value)
{
    device.CVARHR_HI = value;
}

void ADE9000_Mock_SetCPERIOD(uint32_t value)
{
	device.CPERIOD = value;
}


/* =========================================================
 * NEUTRAL SETTER
 * ========================================================= */

void ADE9000_Mock_SetNIRMS(uint32_t value)
{
	device.NIRMS = value;
}

/* =========================================================
 * SPI TRANSACTION MOCK
 * ========================================================= */

#include <stdio.h>

bool ADE9000_Mock_SPI_Read(
    uint16_t command,
    uint8_t data_size,
    uint32_t *data,
    uint16_t *crc)
{
    uint16_t address;
    uint32_t value;

    if (data == NULL || crc == NULL)
    {
        return false;
    }


    /*
     * Decode CMD_HDR
     *
     * CMD_HDR[15:4] = address
     * CMD_HDR[3]    = READ
     */

    address = (uint16_t)((command >> 4) & 0x0FFFU);


    if ((command & (1U << 3)) == 0)
    {
        return false;
    }


    /*
     * Get register value from mock register map
     */

    value = ADE9000_Mock_ReadRegister(address);


    /*
     * Respect register data size.
     */

    if (data_size == 2U)
    {
        value &= 0x0000FFFFUL;
    }
    else if (data_size == 4U)
    {
        /* 32-bit value */
    }
    else
    {
        return false;
    }


    *data = value;


    /*
     * Generate CRC exactly from the transmitted
     * register data.
     *
     * CRC calculation itself is implemented in
     * SPI.c.
     *
     * We duplicate the CRC algorithm here so
     * the mock behaves like the ADE9000 device.
     */

    {
        uint16_t crc_value = 0xFFFFU;

        uint8_t bytes[4];
        uint8_t byte_count;
        uint8_t i;
        uint8_t j;

        if (data_size == 4U)
        {
            bytes[0] = (uint8_t)((value >> 24) & 0xFFU);
            bytes[1] = (uint8_t)((value >> 16) & 0xFFU);
            bytes[2] = (uint8_t)((value >> 8) & 0xFFU);
            bytes[3] = (uint8_t)(value & 0xFFU);

            byte_count = 4U;
        }
        else
        {
            bytes[0] = (uint8_t)((value >> 8) & 0xFFU);
            bytes[1] = (uint8_t)(value & 0xFFU);

            byte_count = 2U;
        }


        for (i = 0; i < byte_count; i++)
        {
            crc_value ^= ((uint16_t)bytes[i] << 8);

            for (j = 0; j < 8; j++)
            {
                if (crc_value & 0x8000U)
                {
                    crc_value =
                        (uint16_t)((crc_value << 1) ^ 0x1021U);
                }
                else
                {
                    crc_value <<= 1;
                }
            }
        }

        *crc = crc_value;
    }


    return true;
}


/* =========================================================
 * SPI WRITE TRANSACTION MOCK
 * ========================================================= */

bool ADE9000_Mock_SPI_Write(
    uint16_t command,
    uint8_t data_size,
    uint32_t data)
{
    uint16_t address;


    /*
     * Decode address.
     */

    address = (uint16_t)((command >> 4) & 0x0FFFU);


    /*
     * Check that this is WRITE.
     */

    if ((command & (1U << 3)) != 0)
    {
        return false;
    }


    /*
     * Check data size.
     */

    if ((data_size != 2U) &&
        (data_size != 4U))
    {
        return false;
    }


    /*
     * Write into mock register map.
     */

    ADE9000_Mock_WriteRegister(
        address,
        data);


    return true;
}


/* =========================================================
 * SPI BURST READ TRANSACTION MOCK
 * ========================================================= */

bool ADE9000_Mock_SPI_BurstRead(
    uint16_t command,
    uint32_t *buffer,
    uint16_t count)
{
    uint16_t address;
    uint16_t i;


    if (buffer == NULL || count == 0)
    {
        return false;
    }


    /*
     * CMD_HDR must indicate READ.
     */

    if ((command & (1U << 3)) == 0)
    {
        return false;
    }


    /*
     * Extract starting address.
     */

    address = (uint16_t)((command >> 4) & 0x0FFFU);


    /*
     * Simulate consecutive register reads.
     *
     * In real hardware the ADE9000 keeps SS low
     * and automatically provides subsequent data
     * according to its burst-read rules.
     */

    for (i = 0; i < count; i++)
    {
        buffer[i] =
            ADE9000_Mock_ReadRegister(
                (uint16_t)(address + i));
    }


    return true;
}