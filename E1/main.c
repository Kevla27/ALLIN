#include <stdio.h>
#include <stdint.h>

#include "RegisterMap/RegisterMap.h"
#include "Mock/Mock.h"
#include "Metrology/Metrology.h"
#include "SPI/SPI.h"

int main(void)
{
    /* =========================
     * INITIALIZATION
     * ========================= */

    ADE9000_Mock_Init();
    ADE9000_SPI_Init();

    /* =========================
     * CONFIGURE MOCK DATA
     * ========================= */

    /* PHASE A */
    ADE9000_Mock_SetAIRMS(5962491);
    ADE9000_Mock_SetAVRMS(17136895);
	ADE9000_Mock_SetAIFRMS(26347436);
    ADE9000_Mock_SetAWATT(636984);
    ADE9000_Mock_SetAPERIOD(20000);
    ADE9000_Mock_SetAWATTHR_HI(0x00000001);
    ADE9000_Mock_SetAWATTHR_LO(0x00001000);

    /* PHASE B */
    ADE9000_Mock_SetBIRMS(5962491);
    ADE9000_Mock_SetBVRMS(17136895);
    ADE9000_Mock_SetBWATT(636984);
    ADE9000_Mock_SetBPERIOD(20000);
    ADE9000_Mock_SetBWATTHR_HI(0x00000001);
    ADE9000_Mock_SetBWATTHR_LO(0x00001000);

    /* PHASE C */
    ADE9000_Mock_SetCIRMS(5962491);
    ADE9000_Mock_SetCVRMS(17136895);
    ADE9000_Mock_SetCWATT(636984);
    ADE9000_Mock_SetCPERIOD(20000);
    ADE9000_Mock_SetCWATTHR_HI(0x00000001);
    ADE9000_Mock_SetCWATTHR_LO(0x00001000);

    /* NEUTRAL */
    ADE9000_Mock_SetNIRMS(1192498);

    /* =========================
     * SPI TRANSACTION TEST
     * ========================= */

    printf("=== SPI TRANSACTION TEST ===\n\n");

    /* SPI WRITE TEST */
    if (ADE9000_SPI_TransactionWrite(
            ADE9000_RUN,
            4,
            1) == ADE9000_SPI_OK)
    {
        printf("SPI Write Test : PASS\n");
    }
    else
    {
        printf("SPI Write Test : FAIL\n");
    }

    /* SPI READ TEST */
    uint32_t run_value = 0;
	uint16_t run_crc = 0;

    if (ADE9000_SPI_TransactionRead(
            ADE9000_RUN,
            4,
            &run_value,
			&run_crc) == ADE9000_SPI_OK)
    {
        printf("SPI Read Test  : PASS\n");
        printf("RUN Register   : %lu\n",
               (unsigned long)run_value);
        printf("RUN CRC        : %u\n",
               run_crc);
    }
    else
    {
        printf("SPI Read Test  : FAIL\n");
    }

    /* SPI BURST READ TEST */
    uint32_t burst_data[3];

	ADE9000_SPI_BurstRead(
		ADE9000_AIRMS,
		burst_data,
		3
	);

    if (ADE9000_SPI_BurstRead(
            ADE9000_AIRMS,
            burst_data,
            3) == ADE9000_SPI_OK)
    {
        printf("SPI Burst Read : PASS\n");

        printf("Data[0]        : %lu\n",
               (unsigned long)burst_data[0]);

        printf("Data[1]        : %lu\n",
               (unsigned long)burst_data[1]);

        printf("Data[2]        : %lu\n",
               (unsigned long)burst_data[2]);
    }
    else
    {
        printf("SPI Burst Read : FAIL\n");
    }

    printf("\n");

    /* =========================
     * READ METROLOGY REGISTERS
     * ========================= */

    uint32_t airms =
        ADE9000_SPI_ReadRegister(ADE9000_AIRMS);

    uint32_t avrms =
        ADE9000_SPI_ReadRegister(ADE9000_AVRMS);

    int32_t awatt =
        (int32_t)ADE9000_SPI_ReadRegister(ADE9000_AWATT);

    uint32_t aperiod =
        ADE9000_SPI_ReadRegister(ADE9000_APERIOD);

    uint32_t birms =
        ADE9000_SPI_ReadRegister(ADE9000_BIRMS);

    uint32_t bvrms =
        ADE9000_SPI_ReadRegister(ADE9000_BVRMS);

    int32_t bwatt =
        (int32_t)ADE9000_SPI_ReadRegister(ADE9000_BWATT);

    uint32_t bperiod =
        ADE9000_SPI_ReadRegister(ADE9000_BPERIOD);

    uint32_t cirms =
        ADE9000_SPI_ReadRegister(ADE9000_CIRMS);

    uint32_t cvrms =
        ADE9000_SPI_ReadRegister(ADE9000_CVRMS);

    int32_t cwatt =
        (int32_t)ADE9000_SPI_ReadRegister(ADE9000_CWATT);

    uint32_t cperiod =
        ADE9000_SPI_ReadRegister(ADE9000_CPERIOD);

    uint32_t nirms =
        ADE9000_SPI_ReadRegister(ADE9000_NIRMS);

    /* =========================
     * READ ENERGY REGISTERS
     * ========================= */

    uint32_t awatthr_hi =
        ADE9000_SPI_ReadRegister(ADE9000_AWATTHR_HI);

    uint32_t awatthr_lo =
        ADE9000_SPI_ReadRegister(ADE9000_AWATTHR_LO);

    uint32_t bwatthr_hi =
        ADE9000_SPI_ReadRegister(ADE9000_BWATTHR_HI);

    uint32_t bwatthr_lo =
        ADE9000_SPI_ReadRegister(ADE9000_BWATTHR_LO);

    uint32_t cwatthr_hi =
        ADE9000_SPI_ReadRegister(ADE9000_CWATTHR_HI);

    uint32_t cwatthr_lo =
        ADE9000_SPI_ReadRegister(ADE9000_CWATTHR_LO);

    /* =========================
     * COMBINE ENERGY REGISTERS
     * ========================= */

    int64_t raw_energy_a =
        Metrology_CombineEnergyRegister(
            awatthr_hi,
            awatthr_lo);

    int64_t raw_energy_b =
        Metrology_CombineEnergyRegister(
            bwatthr_hi,
            bwatthr_lo);

    int64_t raw_energy_c =
        Metrology_CombineEnergyRegister(
            cwatthr_hi,
            cwatthr_lo);

    /* =========================
     * METROLOGY CALCULATIONS
     * ========================= */

    float ia =
        Metrology_CurrentFromAIRMS(airms);

    float va =
        Metrology_VoltageFromAVRMS(avrms);

    float pa =
        Metrology_PowerFromAWATT(awatt);

    float fa =
        Metrology_FrequencyFromPeriod(aperiod);

    float ib =
        Metrology_CurrentFromBIRMS(birms);

    float vb =
        Metrology_VoltageFromBVRMS(bvrms);

    float pb =
        Metrology_PowerFromBWATT(bwatt);

    float fb =
        Metrology_FrequencyFromPeriod(bperiod);

    float ic =
        Metrology_CurrentFromCIRMS(cirms);

    float vc =
        Metrology_VoltageFromCVRMS(cvrms);

    float pc =
        Metrology_PowerFromCWATT(cwatt);

    float fc =
        Metrology_FrequencyFromPeriod(cperiod);

    float in =
        Metrology_CurrentFromNIRMS(nirms);

    float energy_a_kwh =
        Metrology_EnergyKWhFromRaw(raw_energy_a);

    float energy_b_kwh =
        Metrology_EnergyKWhFromRaw(raw_energy_b);

    float energy_c_kwh =
        Metrology_EnergyKWhFromRaw(raw_energy_c);

    float total_energy_kwh =
        energy_a_kwh +
        energy_b_kwh +
        energy_c_kwh;

    /* =========================
     * OUTPUT
     * ========================= */

    printf("=== METROLOGY RESULT ===\n\n");

    printf("Phase A: I_RMS = %.3f A, "
           "V_RMS = %.3f V, "
           "P = %.3f W, "
           "f = %.3f Hz, "
           "E = %.6f kWh\n",
           ia, va, pa, fa, energy_a_kwh);
    printf("Phase B: I_RMS = %.3f A, "
           "V_RMS = %.3f V, "
           "P = %.3f W, "
           "f = %.3f Hz, "
           "E = %.6f kWh\n",
           ib, vb, pb, fb, energy_b_kwh);
    printf("Phase C: I_RMS = %.3f A, "
           "V_RMS = %.3f V, "
           "P = %.3f W, "
           "f = %.3f Hz, "
           "E = %.6f kWh\n",
           ic, vc, pc, fc, energy_c_kwh);
    printf("Neutral: I_RMS = %.3f A\n", in);

    printf("Total Power: %.3f W\n",
           pa + pb + pc);
    printf("Total Energy: %.6f kWh\n",
           total_energy_kwh);

    return 0;
}