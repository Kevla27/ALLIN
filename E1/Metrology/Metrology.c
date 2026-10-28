#include "Metrology.h"

/* =========================================================
 * CURRENT
 * ========================================================= */

float Metrology_CurrentFromAIRMS(uint32_t raw)
{
    return (float)raw * CURRENT_CONVERSION_CONSTANT;
}

float Metrology_CurrentFromBIRMS(uint32_t raw)
{
    return (float)raw * CURRENT_CONVERSION_CONSTANT;
}

float Metrology_CurrentFromCIRMS(uint32_t raw)
{
    return (float)raw * CURRENT_CONVERSION_CONSTANT;
}

float Metrology_CurrentFromNIRMS(uint32_t raw)
{
    return (float)raw * CURRENT_CONVERSION_CONSTANT;
}

/* =========================================================
 * VOLTAGE
 * ========================================================= */

float Metrology_VoltageFromAVRMS(uint32_t raw)
{
    return (float)raw * VOLTAGE_CONVERSION_CONSTANT;
}

float Metrology_VoltageFromBVRMS(uint32_t raw)
{
    return (float)raw * VOLTAGE_CONVERSION_CONSTANT;
}

float Metrology_VoltageFromCVRMS(uint32_t raw)
{
    return (float)raw * VOLTAGE_CONVERSION_CONSTANT;
}

/* =========================================================
 * ACTIVE, REACTIVE & APPARENT POWER
 * ========================================================= */

float Metrology_PowerFromAWATT(int32_t raw)
{
    return (float)raw * POWER_CONVERSION_CONSTANT;
}

float Metrology_PowerFromBWATT(int32_t raw)
{
    return (float)raw * POWER_CONVERSION_CONSTANT;
}

float Metrology_PowerFromCWATT(int32_t raw)
{
    return (float)raw * POWER_CONVERSION_CONSTANT;
}

float Metrology_ReactivePowerFromAVAR(int32_t raw)
{
    return (float)raw * POWER_CONVERSION_CONSTANT;
}

float Metrology_ReactivePowerFromBVAR(int32_t raw)
{
    return (float)raw * POWER_CONVERSION_CONSTANT;
}

float Metrology_ReactivePowerFromCVAR(int32_t raw)
{
    return (float)raw * POWER_CONVERSION_CONSTANT;
}

float Metrology_ApparentPowerFromAVA(uint32_t raw)
{
    return (float)raw * POWER_CONVERSION_CONSTANT;
}

float Metrology_ApparentPowerFromBVA(uint32_t raw)
{
    return (float)raw * POWER_CONVERSION_CONSTANT;
}

float Metrology_ApparentPowerFromCVA(uint32_t raw)
{
    return (float)raw * POWER_CONVERSION_CONSTANT;
}

/* =========================================================
 * POWER FACTOR & CALIBRATION GAIN
 * ========================================================= */

float Metrology_PowerFactorFromAPF(int32_t raw)
{
    return (float)raw / 134217728.0f;
}

float Metrology_PowerFactorFromBPF(int32_t raw)
{
    return (float)raw / 134217728.0f;
}

float Metrology_PowerFactorFromCPF(int32_t raw)
{
    return (float)raw / 134217728.0f;
}

uint16_t Metrology_PowerFactorPermilleFromRaw(int32_t raw)
{
    /* ADE9000 APF/BPF/CPF registers are signed Q1.27 fixed-point (2^27 = 1.000). */
    int64_t abs_raw = (raw < 0) ? -(int64_t)raw : (int64_t)raw;
    uint32_t permille = (uint32_t)(((uint64_t)abs_raw * 1000ULL) >> 27);
    if (permille > 1000U)
    {
        permille = 1000U;
    }
    return (uint16_t)permille;
}

int32_t Metrology_CalculateGainRegister(float expected_val, float measured_val)
{
    if (measured_val <= 0.0f || expected_val <= 0.0f)
    {
        return 0;
    }
    /* ADE9000 xGAIN = ((Expected / Measured) - 1) * 2^27 */
    float gain_adj = (expected_val / measured_val) - 1.0f;
    return (int32_t)(gain_adj * 134217728.0f);
}

/* =========================================================
 * ENERGY
 * ========================================================= */

int64_t Metrology_CombineEnergyRegister(uint32_t hi, uint32_t lo)
{
    /* ADE9000 energy registers are signed 45-bit values: HI[31:0]
     * contains bits [44:13], and LO[12:0] contains bits [12:0]. */
    uint64_t raw45 = ((uint64_t)hi << 13) | ((uint64_t)lo & 0x1FFFULL);

    /* Sign-extend bit 44 through the upper bits. */
    if ((raw45 & (1ULL << 44)) != 0)
    {
        raw45 |= 0xFFFFE00000000000ULL;
    }

    return (int64_t)raw45;
}

float Metrology_EnergyFromRaw(int64_t raw)
{
    return (float)raw * ENERGY_CONVERSION_CONSTANT;
}

float Metrology_EnergyKWhFromRaw(int64_t raw)
{
    return Metrology_EnergyFromRaw(raw) / 1000.0f;
}

/* =========================================================
 * FREQUENCY
 * ========================================================= */

float Metrology_FrequencyFromPeriod(uint32_t raw)
{
    if (raw == 0U)
    {
        return 0.0f;
    }

    /* ADE9000 hardware period register: f_line = (8000 * 65536) / (APERIOD + 1) */
    if (raw > 100000U)
    {
        return 524288000.0f / (float)(raw + 1U);
    }

    /* Legacy microsecond period count fallback for unit tests */
    const float period_seconds = (float)raw * 1.0e-6f;
    return 1.0f / period_seconds;
}

/* =========================================================
 * THD
 * ========================================================= */

float Metrology_THDFromRaw(uint32_t raw)
{
    /* ADE9000 VTHD/ITHD registers are Q5.27 format (100% = 2^27). */
    return ((float)raw / 134217728.0f) * 100.0f;
}

/* =========================================================
 * REACTIVE ENERGY QUADRANTS (SPLN D3.006:2021 Q1, Q2, Q3, Q4)
 * ========================================================= */

void Metrology_AccumulateReactiveQuadrant(
    ReactiveEnergyQuadrants_t *energy,
    double active_power,
    double reactive_power,
    double delta_time_seconds)
{
    if (energy == 0 || delta_time_seconds <= 0.0)
    {
        return;
    }

    /* Energi reaktif selama interval pengukuran, dalam varh */
    double reactive_energy_varh =
        (reactive_power >= 0.0 ? reactive_power : -reactive_power)
        * delta_time_seconds / 3600.0;

    /* Tentukan kuadran berdasarkan tanda P dan Q */
    if (active_power > 0.0 && reactive_power > 0.0)
    {
        energy->q1_varh += reactive_energy_varh;
    }
    else if (active_power < 0.0 && reactive_power > 0.0)
    {
        energy->q2_varh += reactive_energy_varh;
    }
    else if (active_power < 0.0 && reactive_power < 0.0)
    {
        energy->q3_varh += reactive_energy_varh;
    }
    else if (active_power > 0.0 && reactive_power < 0.0)
    {
        energy->q4_varh += reactive_energy_varh;
    }
}
