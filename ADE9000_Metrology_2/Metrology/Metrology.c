#include "Metrology.h"
#include <stdint.h>
#include <stddef.h>

/* =========================================================
 * CURRENT
 * ========================================================= */

float Metrology_CurrentFromAIRMS(uint32_t raw)
{
    return raw * CURRENT_CONVERSION_CONSTANT;
}

float Metrology_CurrentFromBIRMS(uint32_t raw)
{
    return raw * CURRENT_CONVERSION_CONSTANT;
}

float Metrology_CurrentFromCIRMS(uint32_t raw)
{
    return raw * CURRENT_CONVERSION_CONSTANT;
}

float Metrology_CurrentFromNIRMS(uint32_t raw)
{
    return raw * CURRENT_CONVERSION_CONSTANT;
}

/* =========================================================
 * VOLTAGE
 * ========================================================= */

float Metrology_VoltageFromAVRMS(uint32_t raw)
{
    return raw * VOLTAGE_CONVERSION_CONSTANT;
}

float Metrology_VoltageFromBVRMS(uint32_t raw)
{
    return raw * VOLTAGE_CONVERSION_CONSTANT;
}

float Metrology_VoltageFromCVRMS(uint32_t raw)
{
    return raw * VOLTAGE_CONVERSION_CONSTANT;
}

/* =========================================================
 * POWER
 * ========================================================= */
/*ACTIVE POWER*/
float Metrology_PowerFromAWATT(int32_t raw)
{
    return raw * POWER_CONVERSION_CONSTANT;
}

float Metrology_PowerFromBWATT(int32_t raw)
{
    return raw * POWER_CONVERSION_CONSTANT;
}

float Metrology_PowerFromCWATT(int32_t raw)
{
    return raw * POWER_CONVERSION_CONSTANT;
}

/* =========================
 * REACTIVE POWER (Q)
 * ========================= */

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

/* =========================
 * APPARENT POWER (S)
 * ========================= */

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

/* =========================
 * POWER FACTOR (PF)
 * ADE9000 signed 5.27 format
 * ========================= */

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

    /* Assumption: one raw count equals one microsecond. */
    const float period_seconds = (float)raw * 1.0e-6f;
    return 1.0f / period_seconds;
}

/* =========================================================
 * THD
 * ========================================================= */

float Metrology_THDFromRaw(uint32_t raw)
{
    /* Apply ADE9000 register scaling/calibration when established. */
    return (float)raw;
}


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
        (reactive_power >= 0.0
            ? reactive_power
            : -reactive_power)
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