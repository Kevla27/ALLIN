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

    /* Set default ADE9000 PART_ID bit 20 (Desain Sistem Hal. 70) */
    device.PART_ID = ADE9000_PART_ID_MASK;

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
        case ADE9000_AIRMS:         return device.AIRMS;
        case ADE9000_AVRMS:         return device.AVRMS;
        case ADE9000_AIFRMS:        return device.AIFRMS;
        case ADE9000_AVFRMS:        return device.AVFRMS;
        case ADE9000_AWATT:         return (uint32_t)device.AWATT;
        case ADE9000_AVAR:          return (uint32_t)device.AVAR;
        case ADE9000_AVA:           return device.AVA;
        case ADE9000_APF:           return (uint32_t)device.APF;
        case ADE9000_AVTHD:         return device.AVTHD;
        case ADE9000_AITHD:         return device.AITHD;
        case ADE9000_APERIOD:       return device.APERIOD;
        case ADE9000_AWATT_ACC_LO:  return device.AWATT_ACC_LO;
        case ADE9000_AWATT_ACC_HI:  return device.AWATT_ACC_HI;
        case ADE9000_AWATTHR_LO:    return device.AWATTHR_LO;
        case ADE9000_AWATTHR_HI:    return device.AWATTHR_HI;
        case 0x2EA:
        case ADE9000_AVARHR_LO:     return device.AVARHR_LO;
        case 0x2EB:
        case ADE9000_AVARHR_HI:     return device.AVARHR_HI;
        case ADE9000_AIGAIN:        return device.AIGAIN;
        case ADE9000_AVGAIN:        return device.AVGAIN;
        case ADE9000_APGAIN:        return device.APGAIN;
        case ADE9000_APHCAL0:       return device.APHCAL0;
        case ADE9000_AIRMSOS:       return device.AIRMSOS;
        case ADE9000_AVRMSOS:       return device.AVRMSOS;
        case ADE9000_AWATTOS:       return device.AWATTOS;
        case ADE9000_AVAROS:        return device.AVAROS;
        case ADE9000_AIFRMSOS:      return device.AIFRMSOS;
        case ADE9000_AVFRMSOS:      return device.AVFRMSOS;

        /* =========================
         * PHASE B
         * ========================= */
        case ADE9000_BIRMS:         return device.BIRMS;
        case ADE9000_BVRMS:         return device.BVRMS;
        case ADE9000_BIFRMS:        return device.BIFRMS;
        case ADE9000_BVFRMS:        return device.BVFRMS;
        case ADE9000_BWATT:         return (uint32_t)device.BWATT;
        case ADE9000_BVAR:          return (uint32_t)device.BVAR;
        case ADE9000_BVA:           return device.BVA;
        case ADE9000_BPF:           return (uint32_t)device.BPF;
        case ADE9000_BVTHD:         return device.BVTHD;
        case ADE9000_BITHD:         return device.BITHD;
        case ADE9000_BPERIOD:       return device.BPERIOD;
        case ADE9000_BWATT_ACC_LO:  return device.BWATT_ACC_LO;
        case ADE9000_BWATT_ACC_HI:  return device.BWATT_ACC_HI;
        case ADE9000_BWATTHR_LO:    return device.BWATTHR_LO;
        case ADE9000_BWATTHR_HI:    return device.BWATTHR_HI;
        case 0x326:
        case ADE9000_BVARHR_LO:     return device.BVARHR_LO;
        case 0x327:
        case ADE9000_BVARHR_HI:     return device.BVARHR_HI;
        case ADE9000_BIGAIN:        return device.BIGAIN;
        case ADE9000_BVGAIN:        return device.BVGAIN;
        case ADE9000_BPGAIN:        return device.BPGAIN;
        case ADE9000_BPHCAL0:       return device.BPHCAL0;
        case ADE9000_BIRMSOS:       return device.BIRMSOS;
        case ADE9000_BVRMSOS:       return device.BVRMSOS;
        case ADE9000_BWATTOS:       return device.BWATTOS;
        case ADE9000_BVAROS:        return device.BVAROS;
        case ADE9000_BIFRMSOS:      return device.BIFRMSOS;
        case ADE9000_BVFRMSOS:      return device.BVFRMSOS;

        /* =========================
         * PHASE C
         * ========================= */
        case ADE9000_CIRMS:         return device.CIRMS;
        case ADE9000_CVRMS:         return device.CVRMS;
        case ADE9000_CIFRMS:        return device.CIFRMS;
        case ADE9000_CVFRMS:        return device.CVFRMS;
        case ADE9000_CWATT:         return (uint32_t)device.CWATT;
        case ADE9000_CVAR:          return (uint32_t)device.CVAR;
        case ADE9000_CVA:           return device.CVA;
        case ADE9000_CPF:           return (uint32_t)device.CPF;
        case ADE9000_CVTHD:         return device.CVTHD;
        case ADE9000_CITHD:         return device.CITHD;
        case ADE9000_CPERIOD:       return device.CPERIOD;
        case ADE9000_CWATT_ACC_LO:  return device.CWATT_ACC_LO;
        case ADE9000_CWATT_ACC_HI:  return device.CWATT_ACC_HI;
        case ADE9000_CWATTHR_LO:    return device.CWATTHR_LO;
        case ADE9000_CWATTHR_HI:    return device.CWATTHR_HI;
        case 0x362:
        case ADE9000_CVARHR_LO:     return device.CVARHR_LO;
        case 0x363:
        case ADE9000_CVARHR_HI:     return device.CVARHR_HI;
        case ADE9000_CIGAIN:        return device.CIGAIN;
        case ADE9000_CVGAIN:        return device.CVGAIN;
        case ADE9000_CPGAIN:        return device.CPGAIN;
        case ADE9000_CPHCAL0:       return device.CPHCAL0;
        case ADE9000_CIRMSOS:       return device.CIRMSOS;
        case ADE9000_CVRMSOS:       return device.CVRMSOS;
        case ADE9000_CWATTOS:       return device.CWATTOS;
        case ADE9000_CVAROS:        return device.CVAROS;
        case ADE9000_CIFRMSOS:      return device.CIFRMSOS;
        case ADE9000_CVFRMSOS:      return device.CVFRMSOS;

        /* =========================
         * NEUTRAL & COMBINED
         * ========================= */
        case ADE9000_NIRMS:         return device.NIRMS;
        case ADE9000_NIGAIN:        return device.NIGAIN;
        case ADE9000_NIRMSOS:       return device.NIRMSOS;
        case ADE9000_COM_PERIOD:    return device.COM_PERIOD;

        /* =========================
         * CONTROL & STATUS
         * ========================= */
        case ADE9000_PART_ID:       return device.PART_ID;
        case ADE9000_RUN:           return device.RUN;
        case ADE9000_CONFIG0:       return device.CONFIG0;
        case ADE9000_CONFIG1:       return device.CONFIG1;
        case ADE9000_PGA_GAIN:      return device.PGA_GAIN;
        case ADE9000_ACCMODE:       return device.ACCMODE;
        case ADE9000_EP_CFG:        return device.EP_CFG;
        case ADE9000_EGY_TIME:      return device.EGY_TIME;
        case ADE9000_CFMODE:        return device.CFMODE;
        case ADE9000_CF_LCFG:       return device.CF_LCFG;
        case ADE9000_ACT_NL_LVL:    return device.ACT_NL_LVL;
        case ADE9000_REACT_NL_LVL:  return device.REACT_NL_LVL;
        case ADE9000_APP_NL_LVL:    return device.APP_NL_LVL;
        case ADE9000_DIP_LVL:       return device.DIP_LVL;
        case ADE9000_DIPA:          return device.DIPA;
        case ADE9000_DIPB:          return device.DIPB;
        case ADE9000_DIPC:          return device.DIPC;
        case ADE9000_SWELL_LVL:     return device.SWELL_LVL;
        case ADE9000_SWELLA:        return device.SWELLA;
        case ADE9000_SWELLB:        return device.SWELLB;
        case ADE9000_SWELLC:        return device.SWELLC;
        case ADE9000_OILVL:         return device.OILVL;
        case ADE9000_OIA:           return device.OIA;
        case ADE9000_OIB:           return device.OIB;
        case ADE9000_OIC:           return device.OIC;
        case ADE9000_OIN:           return device.OIN;
        case ADE9000_STATUS0:       return device.STATUS0;
        case ADE9000_STATUS1:       return device.STATUS1;
        case ADE9000_EVENT_STATUS:  return device.EVENT_STATUS;
        case ADE9000_MASK0:         return device.MASK0;
        case ADE9000_MASK1:         return device.MASK1;
        case ADE9000_EVENT_MASK:    return device.EVENT_MASK;
        case ADE9000_WR_LOCK:       return device.WR_LOCK;

        default:
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
        case ADE9000_RUN:           device.RUN = value; break;
        case ADE9000_CONFIG0:       device.CONFIG0 = value; break;
        case ADE9000_CONFIG1:       device.CONFIG1 = value; break;
        case ADE9000_PGA_GAIN:      device.PGA_GAIN = value; break;
        case ADE9000_ACCMODE:       device.ACCMODE = value; break;
        case ADE9000_EP_CFG:        device.EP_CFG = value; break;
        case ADE9000_EGY_TIME:      device.EGY_TIME = value; break;
        case ADE9000_CFMODE:        device.CFMODE = value; break;
        case ADE9000_CF_LCFG:       device.CF_LCFG = value; break;
        case ADE9000_ACT_NL_LVL:    device.ACT_NL_LVL = value; break;
        case ADE9000_REACT_NL_LVL:  device.REACT_NL_LVL = value; break;
        case ADE9000_APP_NL_LVL:    device.APP_NL_LVL = value; break;
        case ADE9000_DIP_LVL:       device.DIP_LVL = value; break;
        case ADE9000_SWELL_LVL:     device.SWELL_LVL = value; break;
        case ADE9000_OILVL:         device.OILVL = value; break;
        case ADE9000_STATUS0:       device.STATUS0 &= ~value; break; /* Write-1-to-clear */
        case ADE9000_STATUS1:       device.STATUS1 &= ~value; break; /* Write-1-to-clear */
        case ADE9000_EVENT_STATUS:  device.EVENT_STATUS = value; break;
        case ADE9000_MASK0:         device.MASK0 = value; break;
        case ADE9000_MASK1:         device.MASK1 = value; break;
        case ADE9000_EVENT_MASK:    device.EVENT_MASK = value; break;
        case ADE9000_WR_LOCK:       device.WR_LOCK = value; break;

        /* Calibration Phase A */
        case ADE9000_AIGAIN:        device.AIGAIN = value; break;
        case ADE9000_AVGAIN:        device.AVGAIN = value; break;
        case ADE9000_APGAIN:        device.APGAIN = value; break;
        case ADE9000_APHCAL0:       device.APHCAL0 = value; break;
        case ADE9000_AIRMSOS:       device.AIRMSOS = value; break;
        case ADE9000_AVRMSOS:       device.AVRMSOS = value; break;
        case ADE9000_AWATTOS:       device.AWATTOS = value; break;
        case ADE9000_AVAROS:        device.AVAROS = value; break;
        case ADE9000_AIFRMSOS:      device.AIFRMSOS = value; break;
        case ADE9000_AVFRMSOS:      device.AVFRMSOS = value; break;

        /* Calibration Phase B */
        case ADE9000_BIGAIN:        device.BIGAIN = value; break;
        case ADE9000_BVGAIN:        device.BVGAIN = value; break;
        case ADE9000_BPGAIN:        device.BPGAIN = value; break;
        case ADE9000_BPHCAL0:       device.BPHCAL0 = value; break;
        case ADE9000_BIRMSOS:       device.BIRMSOS = value; break;
        case ADE9000_BVRMSOS:       device.BVRMSOS = value; break;
        case ADE9000_BWATTOS:       device.BWATTOS = value; break;
        case ADE9000_BVAROS:        device.BVAROS = value; break;
        case ADE9000_BIFRMSOS:      device.BIFRMSOS = value; break;
        case ADE9000_BVFRMSOS:      device.BVFRMSOS = value; break;

        /* Calibration Phase C */
        case ADE9000_CIGAIN:        device.CIGAIN = value; break;
        case ADE9000_CVGAIN:        device.CVGAIN = value; break;
        case ADE9000_CPGAIN:        device.CPGAIN = value; break;
        case ADE9000_CPHCAL0:       device.CPHCAL0 = value; break;
        case ADE9000_CIRMSOS:       device.CIRMSOS = value; break;
        case ADE9000_CVRMSOS:       device.CVRMSOS = value; break;
        case ADE9000_CWATTOS:       device.CWATTOS = value; break;
        case ADE9000_CVAROS:        device.CVAROS = value; break;
        case ADE9000_CIFRMSOS:      device.CIFRMSOS = value; break;
        case ADE9000_CVFRMSOS:      device.CVFRMSOS = value; break;

        /* Neutral Calibration */
        case ADE9000_NIGAIN:        device.NIGAIN = value; break;
        case ADE9000_NIRMSOS:       device.NIRMSOS = value; break;

        default:
            break;
    }
}


/* =========================================================
 * PHASE A SETTERS
 * ========================================================= */

void ADE9000_Mock_SetAIRMS(uint32_t value) { device.AIRMS = value; }
void ADE9000_Mock_SetAVRMS(uint32_t value) { device.AVRMS = value; }
void ADE9000_Mock_SetAIFRMS(uint32_t value) { device.AIFRMS = value; }
void ADE9000_Mock_SetAWATT(int32_t value) { device.AWATT = value; }
void ADE9000_Mock_SetAVAR(int32_t value) { device.AVAR = value; }
void ADE9000_Mock_SetAVA(uint32_t value) { device.AVA = value; }
void ADE9000_Mock_SetAPF(int32_t value) { device.APF = value; }
void ADE9000_Mock_SetAVTHD(uint32_t value) { device.AVTHD = value; }
void ADE9000_Mock_SetAITHD(uint32_t value) { device.AITHD = value; }
void ADE9000_Mock_SetAWATT_ACC_LO(uint32_t value) { device.AWATT_ACC_LO = value; }
void ADE9000_Mock_SetAWATT_ACC_HI(uint32_t value) { device.AWATT_ACC_HI = value; }
void ADE9000_Mock_SetAWATTHR_LO(uint32_t value) { device.AWATTHR_LO = value; }
void ADE9000_Mock_SetAWATTHR_HI(uint32_t value) { device.AWATTHR_HI = value; }
void ADE9000_Mock_SetAVARHR_LO(uint32_t value) { device.AVARHR_LO = value; }
void ADE9000_Mock_SetAVARHR_HI(uint32_t value) { device.AVARHR_HI = value; }
void ADE9000_Mock_SetAPERIOD(uint32_t value) { device.APERIOD = value; }


/* =========================================================
 * PHASE B SETTERS
 * ========================================================= */

void ADE9000_Mock_SetBIRMS(uint32_t value) { device.BIRMS = value; }
void ADE9000_Mock_SetBVRMS(uint32_t value) { device.BVRMS = value; }
void ADE9000_Mock_SetBWATT(int32_t value) { device.BWATT = value; }
void ADE9000_Mock_SetBVAR(int32_t value) { device.BVAR = value; }
void ADE9000_Mock_SetBVA(uint32_t value) { device.BVA = value; }
void ADE9000_Mock_SetBPF(int32_t value) { device.BPF = value; }
void ADE9000_Mock_SetBVTHD(uint32_t value) { device.BVTHD = value; }
void ADE9000_Mock_SetBITHD(uint32_t value) { device.BITHD = value; }
void ADE9000_Mock_SetBWATT_ACC_LO(uint32_t value) { device.BWATT_ACC_LO = value; }
void ADE9000_Mock_SetBWATT_ACC_HI(uint32_t value) { device.BWATT_ACC_HI = value; }
void ADE9000_Mock_SetBWATTHR_LO(uint32_t value) { device.BWATTHR_LO = value; }
void ADE9000_Mock_SetBWATTHR_HI(uint32_t value) { device.BWATTHR_HI = value; }
void ADE9000_Mock_SetBVARHR_LO(uint32_t value) { device.BVARHR_LO = value; }
void ADE9000_Mock_SetBVARHR_HI(uint32_t value) { device.BVARHR_HI = value; }
void ADE9000_Mock_SetBPERIOD(uint32_t value) { device.BPERIOD = value; }


/* =========================================================
 * PHASE C SETTERS
 * ========================================================= */

void ADE9000_Mock_SetCIRMS(uint32_t value) { device.CIRMS = value; }
void ADE9000_Mock_SetCVRMS(uint32_t value) { device.CVRMS = value; }
void ADE9000_Mock_SetCWATT(int32_t value) { device.CWATT = value; }
void ADE9000_Mock_SetCVAR(int32_t value) { device.CVAR = value; }
void ADE9000_Mock_SetCVA(uint32_t value) { device.CVA = value; }
void ADE9000_Mock_SetCPF(int32_t value) { device.CPF = value; }
void ADE9000_Mock_SetCVTHD(uint32_t value) { device.CVTHD = value; }
void ADE9000_Mock_SetCITHD(uint32_t value) { device.CITHD = value; }
void ADE9000_Mock_SetCWATT_ACC_LO(uint32_t value) { device.CWATT_ACC_LO = value; }
void ADE9000_Mock_SetCWATT_ACC_HI(uint32_t value) { device.CWATT_ACC_HI = value; }
void ADE9000_Mock_SetCWATTHR_LO(uint32_t value) { device.CWATTHR_LO = value; }
void ADE9000_Mock_SetCWATTHR_HI(uint32_t value) { device.CWATTHR_HI = value; }
void ADE9000_Mock_SetCVARHR_LO(uint32_t value) { device.CVARHR_LO = value; }
void ADE9000_Mock_SetCVARHR_HI(uint32_t value) { device.CVARHR_HI = value; }
void ADE9000_Mock_SetCPERIOD(uint32_t value) { device.CPERIOD = value; }


/* =========================================================
 * NEUTRAL & STATUS SETTERS
 * ========================================================= */

void ADE9000_Mock_SetNIRMS(uint32_t value) { device.NIRMS = value; }
void ADE9000_Mock_SetSTATUS0(uint32_t value) { device.STATUS0 = value; }
void ADE9000_Mock_SetSTATUS1(uint32_t value) { device.STATUS1 = value; }


/* =========================================================
 * SPI TRANSACTION MOCK
 * ========================================================= */

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

    address = (uint16_t)((command >> 4) & 0x0FFFU);

    if ((command & (1U << 3)) == 0)
    {
        return false;
    }

    value = ADE9000_Mock_ReadRegister(address);

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
                    crc_value = (uint16_t)((crc_value << 1) ^ 0x1021U);
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

    address = (uint16_t)((command >> 4) & 0x0FFFU);

    if ((command & (1U << 3)) != 0)
    {
        return false;
    }

    if ((data_size != 2U) && (data_size != 4U))
    {
        return false;
    }

    ADE9000_Mock_WriteRegister(address, data);
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

    if ((command & (1U << 3)) == 0)
    {
        return false;
    }

    address = (uint16_t)((command >> 4) & 0x0FFFU);

    for (i = 0; i < count; i++)
    {
        buffer[i] = ADE9000_Mock_ReadRegister((uint16_t)(address + i));
    }

    return true;
}
