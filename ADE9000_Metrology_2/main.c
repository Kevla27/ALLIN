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
    ADE9000_Mock_SetAVAR(300000);
    ADE9000_Mock_SetAVA(700000);
    ADE9000_Mock_SetAPF(107374182);
    ADE9000_Mock_SetAVARHR_HI(0x00000001);
    ADE9000_Mock_SetAVARHR_LO(0x00001000);

    /* PHASE B */
    ADE9000_Mock_SetBIRMS(5962491);
    ADE9000_Mock_SetBVRMS(17136895);
    ADE9000_Mock_SetBWATT(636984);
    ADE9000_Mock_SetBPERIOD(20000);
    ADE9000_Mock_SetBWATTHR_HI(0x00000001);
    ADE9000_Mock_SetBWATTHR_LO(0x00001000);
    ADE9000_Mock_SetBVAR(300000);
    ADE9000_Mock_SetBVA(700000);
    ADE9000_Mock_SetBPF(107374182);
    ADE9000_Mock_SetBVARHR_HI(0x00000001);
    ADE9000_Mock_SetBVARHR_LO(0x00001000);

    /* PHASE C */
    ADE9000_Mock_SetCIRMS(5962491);
    ADE9000_Mock_SetCVRMS(17136895);
    ADE9000_Mock_SetCWATT(636984);
    ADE9000_Mock_SetCPERIOD(20000);
    ADE9000_Mock_SetCWATTHR_HI(0x00000001);
    ADE9000_Mock_SetCWATTHR_LO(0x00001000);
    ADE9000_Mock_SetCVAR(300000);
    ADE9000_Mock_SetCVA(700000);
    ADE9000_Mock_SetCPF(107374182);
    ADE9000_Mock_SetCVARHR_HI(0x00000001);
    ADE9000_Mock_SetCVARHR_LO(0x00001000);

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
    
    int32_t avar =
        (int32_t)ADE9000_SPI_ReadRegister(ADE9000_AVAR);

    uint32_t ava =
        ADE9000_SPI_ReadRegister(ADE9000_AVA);

    int32_t apf =
        (int32_t)ADE9000_SPI_ReadRegister(ADE9000_APF);

    uint32_t birms =
        ADE9000_SPI_ReadRegister(ADE9000_BIRMS);

    uint32_t bvrms =
        ADE9000_SPI_ReadRegister(ADE9000_BVRMS);

    int32_t bwatt =
        (int32_t)ADE9000_SPI_ReadRegister(ADE9000_BWATT);

    uint32_t bperiod =
        ADE9000_SPI_ReadRegister(ADE9000_BPERIOD);

    int32_t bvar =
        (int32_t)ADE9000_SPI_ReadRegister(ADE9000_BVAR);

    uint32_t bva =
        ADE9000_SPI_ReadRegister(ADE9000_BVA);

    int32_t bpf =
        (int32_t)ADE9000_SPI_ReadRegister(ADE9000_BPF);

    uint32_t cirms =
        ADE9000_SPI_ReadRegister(ADE9000_CIRMS);

    uint32_t cvrms =
        ADE9000_SPI_ReadRegister(ADE9000_CVRMS);

    int32_t cwatt =
        (int32_t)ADE9000_SPI_ReadRegister(ADE9000_CWATT);

    uint32_t cperiod =
        ADE9000_SPI_ReadRegister(ADE9000_CPERIOD);

    int32_t cvar =
        (int32_t)ADE9000_SPI_ReadRegister(ADE9000_CVAR);

    uint32_t cva =
        ADE9000_SPI_ReadRegister(ADE9000_CVA);

    int32_t cpf =
        (int32_t)ADE9000_SPI_ReadRegister(ADE9000_CPF);

    uint32_t nirms =
        ADE9000_SPI_ReadRegister(ADE9000_NIRMS);

    /* =========================
     * READ ENERGY REGISTERS
     * ========================= */

    uint32_t awatthr_hi =
        ADE9000_SPI_ReadRegister(ADE9000_AWATTHR_HI);

    uint32_t awatthr_lo =
        ADE9000_SPI_ReadRegister(ADE9000_AWATTHR_LO);

    uint32_t avarhr_hi =
        ADE9000_SPI_ReadRegister(ADE9000_AVARHR_HI);

    uint32_t avarhr_lo =
        ADE9000_SPI_ReadRegister(ADE9000_AVARHR_LO);

    uint32_t bwatthr_hi =
        ADE9000_SPI_ReadRegister(ADE9000_BWATTHR_HI);

    uint32_t bwatthr_lo =
        ADE9000_SPI_ReadRegister(ADE9000_BWATTHR_LO);

    uint32_t bvarhr_lo =
        ADE9000_SPI_ReadRegister(ADE9000_BVARHR_LO);

    uint32_t bvarhr_hi =
        ADE9000_SPI_ReadRegister(ADE9000_BVARHR_HI);

    uint32_t cwatthr_hi =
        ADE9000_SPI_ReadRegister(ADE9000_CWATTHR_HI);

    uint32_t cwatthr_lo =
        ADE9000_SPI_ReadRegister(ADE9000_CWATTHR_LO);

    uint32_t cvarhr_lo =
        ADE9000_SPI_ReadRegister(ADE9000_CVARHR_LO);

    uint32_t cvarhr_hi =
        ADE9000_SPI_ReadRegister(ADE9000_CVARHR_HI);

    /* =========================
     * COMBINE ENERGY REGISTERS
     * ========================= */
    /*Active Energy*/
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

    /* REACTIVE ENERGY: COMBINE HI AND LO */

    int64_t raw_reactive_energy_a =
        Metrology_CombineEnergyRegister(
            avarhr_hi,
            avarhr_lo);

    int64_t raw_reactive_energy_b =
        Metrology_CombineEnergyRegister(
            bvarhr_hi,
            bvarhr_lo);

    int64_t raw_reactive_energy_c =
        Metrology_CombineEnergyRegister(
            cvarhr_hi,
            cvarhr_lo);
        
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

    float qa =
        Metrology_ReactivePowerFromAVAR(avar);

    float sa =
        Metrology_ApparentPowerFromAVA(ava);

    float pfa =
        Metrology_PowerFactorFromAPF(apf);

    float energy_a_kwh =
        Metrology_EnergyKWhFromRaw(raw_energy_a);

    float reactive_energy_a_kvarh =
        Metrology_EnergyKWhFromRaw(raw_reactive_energy_a);

    float ib =
        Metrology_CurrentFromBIRMS(birms);

    float vb =
        Metrology_VoltageFromBVRMS(bvrms);

    float pb =
        Metrology_PowerFromBWATT(bwatt);

    float fb =
        Metrology_FrequencyFromPeriod(bperiod);

    float qb =
        Metrology_ReactivePowerFromBVAR(bvar);

    float sb =
        Metrology_ApparentPowerFromBVA(bva);
        
    float pfb =
        Metrology_PowerFactorFromBPF(bpf);

    float energy_b_kwh =
        Metrology_EnergyKWhFromRaw(raw_energy_b);

    float reactive_energy_b_kvarh =
        Metrology_EnergyKWhFromRaw(raw_reactive_energy_b);

    float ic =
        Metrology_CurrentFromCIRMS(cirms);

    float vc =
        Metrology_VoltageFromCVRMS(cvrms);

    float pc =
        Metrology_PowerFromCWATT(cwatt);

    float fc =
        Metrology_FrequencyFromPeriod(cperiod);

    float qc =
        Metrology_ReactivePowerFromCVAR(cvar);

    float sc =
        Metrology_ApparentPowerFromCVA(cva);
        
    float pfc =
        Metrology_PowerFactorFromCPF(cpf);

    float energy_c_kwh =
        Metrology_EnergyKWhFromRaw(raw_energy_c);

    float reactive_energy_c_kvarh =
        Metrology_EnergyKWhFromRaw(raw_reactive_energy_c);

    float in =
        Metrology_CurrentFromNIRMS(nirms);

    float total_energy_kwh =
        energy_a_kwh +
        energy_b_kwh +
        energy_c_kwh;

    float total_reactive_energy_kvarh =
        reactive_energy_a_kvarh +
        reactive_energy_b_kvarh +
        reactive_energy_c_kvarh;

    /* REACTIVE ENERGY QUADRANTS*/

    ReactiveEnergyQuadrants_t reactive_energy = {0};

    const double delta_time_seconds = 0.1;

    /* Phase A */
    Metrology_AccumulateReactiveQuadrant(
        &reactive_energy, pa, qa, delta_time_seconds);

    /* Phase B */
    Metrology_AccumulateReactiveQuadrant(
        &reactive_energy, pb, qb, delta_time_seconds);

    /* Phase C */
    Metrology_AccumulateReactiveQuadrant(
        &reactive_energy, pc, qc, delta_time_seconds);
    
    /* =========================
     * OUTPUT
     * ========================= */

    printf("=== METROLOGY RESULT ===\n\n");

    printf("Phase A\n");
    printf("I_RMS = %.3f A\n", ia);
    printf("V_RMS = %.3f V\n", va);
    printf("P = %.3f kW\n", pa / 1000.0f);
    printf("Q = %.3f kVAR\n", qa / 1000.0f);
    printf("S = %.3f kVA\n", sa / 1000.0f);
    printf("PF = %.4f\n", pfa);
    printf("f = %.3f Hz\n", fa);
    printf("Wa = %.6f kWh\n", energy_a_kwh);
    printf("Wr = %.6f kVARh\n\n", reactive_energy_a_kvarh);

    printf("Phase B\n");
    printf("I_RMS = %.3f A\n", ib);
    printf("V_RMS = %.3f V\n", vb);
    printf("P = %.3f kW\n", pb / 1000.0f);
    printf("Q = %.3f kVAR\n", qb / 1000.0f);
    printf("S = %.3f kVA\n", sb / 1000.0f);
    printf("PF = %.4f\n", pfb);
    printf("f = %.3f Hz\n", fb);
    printf("Wa = %.6f kWh\n", energy_b_kwh);
    printf("Wr = %.6f kVARh\n\n", reactive_energy_b_kvarh);

    printf("Phase C\n");
    printf("I_RMS = %.3f A\n", ic);
    printf("V_RMS = %.3f V\n", vc);
    printf("P = %.3f kW\n", pc / 1000.0f);
    printf("Q = %.3f kVAR\n", qc / 1000.0f);
    printf("S = %.3f kVA\n", sc / 1000.0f);
    printf("PF = %.4f\n", pfc);
    printf("f = %.3f Hz\n", fc);
    printf("Wa = %.6f kWh\n", energy_c_kwh);
    printf("Wr = %.6f kVARh\n\n", reactive_energy_c_kvarh);

    printf("Neutral Current\n");
    printf("I_RMS = %.3f A\n\n", in);

    printf("Total P = %.3f kW\n", (pa + pb + pc) / 1000.0f);
    printf("Total Wa = %.6f kWh\n", total_energy_kwh);
    printf("Total Wr = %.6f kVARh\n\n", total_reactive_energy_kvarh);

    printf("Reactive Energy by Quadrant\n");
    printf("Q1 = %.12f kVARh\n", reactive_energy.q1_varh / 1000.0);
    printf("Q2 = %.12f kVARh\n", reactive_energy.q2_varh / 1000.0);
    printf("Q3 = %.12f kVARh\n", reactive_energy.q3_varh / 1000.0);
    printf("Q4 = %.12f kVARh\n", reactive_energy.q4_varh / 1000.0);
    
    return 0;
}