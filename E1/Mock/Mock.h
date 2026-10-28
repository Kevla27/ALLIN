#ifndef ADE9000_MOCK_H
#define ADE9000_MOCK_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Structure representing the ADE9000 registers
 * used for metrology simulation and Spin 1 blind bring-up.
 */
typedef struct
{
    /* =========================
     * PHASE A
     * ========================= */

    uint32_t AIRMS;
    uint32_t AVRMS;
    uint32_t AIFRMS;
    uint32_t AVFRMS;

    int32_t  AWATT;
    int32_t  AVAR;
    uint32_t AVA;
    int32_t  APF;

    uint32_t AVTHD;
    uint32_t AITHD;
    uint32_t APERIOD;

    /* Accumulated total active power & energy */
    uint32_t AWATT_ACC_LO;
    uint32_t AWATT_ACC_HI;
    uint32_t AWATTHR_LO;
    uint32_t AWATTHR_HI;
    uint32_t AVARHR_LO;
    uint32_t AVARHR_HI;

    /* Calibration Phase A */
    uint32_t AIGAIN;
    uint32_t AVGAIN;
    uint32_t APGAIN;
    uint32_t APHCAL0;
    uint32_t AIRMSOS;
    uint32_t AVRMSOS;
    uint32_t AWATTOS;
    uint32_t AVAROS;
    uint32_t AIFRMSOS;
    uint32_t AVFRMSOS;

    /* =========================
     * PHASE B
     * ========================= */

    uint32_t BIRMS;
    uint32_t BVRMS;
    uint32_t BIFRMS;
    uint32_t BVFRMS;

    int32_t  BWATT;
    int32_t  BVAR;
    uint32_t BVA;
    int32_t  BPF;

    uint32_t BVTHD;
    uint32_t BITHD;
    uint32_t BPERIOD;

    /* Accumulated total active power & energy */
    uint32_t BWATT_ACC_LO;
    uint32_t BWATT_ACC_HI;
    uint32_t BWATTHR_LO;
    uint32_t BWATTHR_HI;
    uint32_t BVARHR_LO;
    uint32_t BVARHR_HI;

    /* Calibration Phase B */
    uint32_t BIGAIN;
    uint32_t BVGAIN;
    uint32_t BPGAIN;
    uint32_t BPHCAL0;
    uint32_t BIRMSOS;
    uint32_t BVRMSOS;
    uint32_t BWATTOS;
    uint32_t BVAROS;
    uint32_t BIFRMSOS;
    uint32_t BVFRMSOS;

    /* =========================
     * PHASE C
     * ========================= */

    uint32_t CIRMS;
    uint32_t CVRMS;
    uint32_t CIFRMS;
    uint32_t CVFRMS;

    int32_t  CWATT;
    int32_t  CVAR;
    uint32_t CVA;
    int32_t  CPF;

    uint32_t CVTHD;
    uint32_t CITHD;
    uint32_t CPERIOD;

    /* Accumulated total active power & energy */
    uint32_t CWATT_ACC_LO;
    uint32_t CWATT_ACC_HI;
    uint32_t CWATTHR_LO;
    uint32_t CWATTHR_HI;
    uint32_t CVARHR_LO;
    uint32_t CVARHR_HI;

    /* Calibration Phase C */
    uint32_t CIGAIN;
    uint32_t CVGAIN;
    uint32_t CPGAIN;
    uint32_t CPHCAL0;
    uint32_t CIRMSOS;
    uint32_t CVRMSOS;
    uint32_t CWATTOS;
    uint32_t CVAROS;
    uint32_t CIFRMSOS;
    uint32_t CVFRMSOS;

    /* =========================
     * NEUTRAL & COMBINED
     * ========================= */

    uint32_t NIRMS;
    uint32_t NIGAIN;
    uint32_t NIRMSOS;
    uint32_t COM_PERIOD;

    /* =========================
     * CONTROL, STATUS & PQ THRESHOLDS (Desain Sistem Hal. 67-74)
     * ========================= */

    uint32_t PART_ID;
    uint32_t RUN;
    uint32_t CONFIG0;
    uint32_t CONFIG1;
    uint32_t PGA_GAIN;
    uint32_t ACCMODE;
    uint32_t EP_CFG;
    uint32_t EGY_TIME;
    uint32_t CFMODE;
    uint32_t CF_LCFG;
    uint32_t ACT_NL_LVL;
    uint32_t REACT_NL_LVL;
    uint32_t APP_NL_LVL;
    uint32_t DIP_LVL;
    uint32_t DIPA;
    uint32_t DIPB;
    uint32_t DIPC;
    uint32_t SWELL_LVL;
    uint32_t SWELLA;
    uint32_t SWELLB;
    uint32_t SWELLC;
    uint32_t OILVL;
    uint32_t OIA;
    uint32_t OIB;
    uint32_t OIC;
    uint32_t OIN;
    uint32_t STATUS0;
    uint32_t STATUS1;
    uint32_t EVENT_STATUS;
    uint32_t MASK0;
    uint32_t MASK1;
    uint32_t EVENT_MASK;
    uint32_t WR_LOCK;

} ADE9000_Mock_t;


/* =========================
 * INITIALIZATION
 * ========================= */

void ADE9000_Mock_Init(void);


/* =========================
 * REGISTER ACCESS
 * ========================= */

uint32_t ADE9000_Mock_ReadRegister(uint16_t address);

void ADE9000_Mock_WriteRegister(uint16_t address,
                                uint32_t value);


/* =========================
 * DEVICE ACCESS
 * ========================= */

ADE9000_Mock_t* ADE9000_Mock_GetDevice(void);


/* =========================
 * PHASE A SETTERS
 * ========================= */

void ADE9000_Mock_SetAIRMS(uint32_t value);
void ADE9000_Mock_SetAVRMS(uint32_t value);
void ADE9000_Mock_SetAIFRMS(uint32_t value);
void ADE9000_Mock_SetAWATT(int32_t value);
void ADE9000_Mock_SetAVAR(int32_t value);
void ADE9000_Mock_SetAVA(uint32_t value);
void ADE9000_Mock_SetAPF(int32_t value);
void ADE9000_Mock_SetAVTHD(uint32_t value);
void ADE9000_Mock_SetAITHD(uint32_t value);

void ADE9000_Mock_SetAWATT_ACC_LO(uint32_t value);
void ADE9000_Mock_SetAWATT_ACC_HI(uint32_t value);

void ADE9000_Mock_SetAWATTHR_LO(uint32_t value);
void ADE9000_Mock_SetAWATTHR_HI(uint32_t value);
void ADE9000_Mock_SetAVARHR_LO(uint32_t value);
void ADE9000_Mock_SetAVARHR_HI(uint32_t value);

void ADE9000_Mock_SetAPERIOD(uint32_t value);


/* =========================
 * PHASE B SETTERS
 * ========================= */

void ADE9000_Mock_SetBIRMS(uint32_t value);
void ADE9000_Mock_SetBVRMS(uint32_t value);
void ADE9000_Mock_SetBWATT(int32_t value);
void ADE9000_Mock_SetBVAR(int32_t value);
void ADE9000_Mock_SetBVA(uint32_t value);
void ADE9000_Mock_SetBPF(int32_t value);
void ADE9000_Mock_SetBVTHD(uint32_t value);
void ADE9000_Mock_SetBITHD(uint32_t value);

void ADE9000_Mock_SetBWATT_ACC_LO(uint32_t value);
void ADE9000_Mock_SetBWATT_ACC_HI(uint32_t value);

void ADE9000_Mock_SetBWATTHR_LO(uint32_t value);
void ADE9000_Mock_SetBWATTHR_HI(uint32_t value);
void ADE9000_Mock_SetBVARHR_LO(uint32_t value);
void ADE9000_Mock_SetBVARHR_HI(uint32_t value);

void ADE9000_Mock_SetBPERIOD(uint32_t value);


/* =========================
 * PHASE C SETTERS
 * ========================= */

void ADE9000_Mock_SetCIRMS(uint32_t value);
void ADE9000_Mock_SetCVRMS(uint32_t value);
void ADE9000_Mock_SetCWATT(int32_t value);
void ADE9000_Mock_SetCVAR(int32_t value);
void ADE9000_Mock_SetCVA(uint32_t value);
void ADE9000_Mock_SetCPF(int32_t value);
void ADE9000_Mock_SetCVTHD(uint32_t value);
void ADE9000_Mock_SetCITHD(uint32_t value);

void ADE9000_Mock_SetCWATT_ACC_LO(uint32_t value);
void ADE9000_Mock_SetCWATT_ACC_HI(uint32_t value);

void ADE9000_Mock_SetCWATTHR_LO(uint32_t value);
void ADE9000_Mock_SetCWATTHR_HI(uint32_t value);
void ADE9000_Mock_SetCVARHR_LO(uint32_t value);
void ADE9000_Mock_SetCVARHR_HI(uint32_t value);

void ADE9000_Mock_SetCPERIOD(uint32_t value);


/* =========================
 * NEUTRAL & STATUS SETTERS
 * ========================= */

void ADE9000_Mock_SetNIRMS(uint32_t value);
void ADE9000_Mock_SetSTATUS0(uint32_t value);
void ADE9000_Mock_SetSTATUS1(uint32_t value);

/* =========================
 * SPI TRANSACTION MOCK
 * ========================= */

bool ADE9000_Mock_SPI_Read(
    uint16_t command,
    uint8_t data_size,
    uint32_t *data,
    uint16_t *crc);

bool ADE9000_Mock_SPI_Write(
    uint16_t command,
    uint8_t data_size,
    uint32_t data);

bool ADE9000_Mock_SPI_BurstRead(
    uint16_t command,
    uint32_t *buffer,
    uint16_t count);

#endif
