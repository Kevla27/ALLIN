#ifndef ADE9000_MOCK_H
#define ADE9000_MOCK_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Structure representing the ADE9000 registers
 * used for metrology simulation.
 */
typedef struct
{
    /* =========================
     * PHASE A
     * ========================= */

    uint32_t AIRMS;
    uint32_t AVRMS;
    uint32_t AIFRMS;

    int32_t  AWATT;
    int32_t  AVAR;
    uint32_t AVA;
    int32_t  APF;

    uint32_t AVTHD;
    uint32_t AITHD;
    uint32_t APERIOD;

    /* Accumulated total active power */
    uint32_t AWATT_ACC_LO;
    uint32_t AWATT_ACC_HI;

    /* Accumulated total active energy */
    uint32_t AWATTHR_LO;
    uint32_t AWATTHR_HI;

    /* Accumulated total reactive energy */
    uint32_t AVARHR_LO;
    uint32_t AVARHR_HI;

    /* =========================
     * PHASE B
     * ========================= */

    uint32_t BIRMS;
    uint32_t BVRMS;

    int32_t  BWATT;
    int32_t  BVAR;
    uint32_t BVA;
    int32_t  BPF;

    uint32_t BVTHD;
    uint32_t BITHD;
    uint32_t BPERIOD;

    /* Accumulated total active power */
    uint32_t BWATT_ACC_LO;
    uint32_t BWATT_ACC_HI;

    /* Accumulated total active energy */
    uint32_t BWATTHR_LO;
    uint32_t BWATTHR_HI;

    /* Accumulated total reactive energy */
    uint32_t BVARHR_LO;
    uint32_t BVARHR_HI;

    /* =========================
     * PHASE C
     * ========================= */

    uint32_t CIRMS;
    uint32_t CVRMS;

    int32_t  CWATT;
    int32_t  CVAR;
    uint32_t CVA;
    int32_t  CPF;

    uint32_t CVTHD;
    uint32_t CITHD;
    uint32_t CPERIOD;

    /* Accumulated total active power */
    uint32_t CWATT_ACC_LO;
    uint32_t CWATT_ACC_HI;

    /* Accumulated total active energy */
    uint32_t CWATTHR_LO;
    uint32_t CWATTHR_HI;

    /* Accumulated total reactive energy */
    uint32_t CVARHR_LO;
    uint32_t CVARHR_HI;

    /* =========================
     * NEUTRAL
     * ========================= */

    uint32_t NIRMS;


    /* =========================
     * CONTROL
     * ========================= */

    uint32_t RUN;

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

void ADE9000_Mock_SetCWATT_ACC_LO(uint32_t value);
void ADE9000_Mock_SetCWATT_ACC_HI(uint32_t value);

void ADE9000_Mock_SetCWATTHR_LO(uint32_t value);
void ADE9000_Mock_SetCWATTHR_HI(uint32_t value);

void ADE9000_Mock_SetCVARHR_LO(uint32_t value);
void ADE9000_Mock_SetCVARHR_HI(uint32_t value);

void ADE9000_Mock_SetCPERIOD(uint32_t value);

/* =========================
 * NEUTRAL SETTER
 * ========================= */

void ADE9000_Mock_SetNIRMS(uint32_t value);

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


/* =========================
 * REACTIVE POWER, APPARENT POWER, POWER FACTOR SETTERS
 * ========================= */

/* Phase A */
void ADE9000_Mock_SetAVAR(int32_t value);
void ADE9000_Mock_SetAVA(uint32_t value);
void ADE9000_Mock_SetAPF(int32_t value);

/* Phase B */
void ADE9000_Mock_SetBVAR(int32_t value);
void ADE9000_Mock_SetBVA(uint32_t value);
void ADE9000_Mock_SetBPF(int32_t value);

/* Phase C */
void ADE9000_Mock_SetCVAR(int32_t value);
void ADE9000_Mock_SetCVA(uint32_t value);
void ADE9000_Mock_SetCPF(int32_t value);


#endif