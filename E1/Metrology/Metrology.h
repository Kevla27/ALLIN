#ifndef METROLOGY_H
#define METROLOGY_H

#include <stdint.h>

typedef struct
{
    double q1_varh;
    double q2_varh;
    double q3_varh;
    double q4_varh;
} ReactiveEnergyQuadrants_t;

void Metrology_AccumulateReactiveQuadrant(
    ReactiveEnergyQuadrants_t *energy,
    double active_power,
    double reactive_power,
    double delta_time_seconds);


/* =========================================================
 * CONVERSION CONSTANTS
 * =========================================================
 *
 * These values are example conversion constants based on
 * the ADE9000 UG-1098 reference system.
 *
 * Final values must be recalculated according to the actual
 * voltage/current sensing circuit and calibration results.
 */


/*
 * Current conversion:
 *
 * Example:
 * 2.0036 uArms/LSB = 2.0036e-6 Arms/LSB
 */
#define CURRENT_CONVERSION_CONSTANT 2.0036e-6f


/*
 * Voltage conversion:
 *
 * Example:
 * 13.4225 uVrms/LSB = 13.4225e-6 Vrms/LSB
 */
#define VOLTAGE_CONVERSION_CONSTANT 13.4225e-6f


/*
 * Active/Reactive/Apparent power conversion:
 *
 * Example:
 * 3.6097 mW/LSB = 3.6097e-3 W/LSB
 */
#define POWER_CONVERSION_CONSTANT 3.6097e-3f


/*
 * Energy conversion:
 *
 * Example:
 * 1.0268 uWh/LSB = 1.0268e-6 Wh/LSB
 */
#define ENERGY_CONVERSION_CONSTANT 1.0268e-6f


/* =========================================================
 * CURRENT
 * ========================================================= */

float Metrology_CurrentFromAIRMS(uint32_t raw);
float Metrology_CurrentFromBIRMS(uint32_t raw);
float Metrology_CurrentFromCIRMS(uint32_t raw);
float Metrology_CurrentFromNIRMS(uint32_t raw);


/* =========================================================
 * VOLTAGE
 * ========================================================= */

float Metrology_VoltageFromAVRMS(uint32_t raw);
float Metrology_VoltageFromBVRMS(uint32_t raw);
float Metrology_VoltageFromCVRMS(uint32_t raw);


/* =========================================================
 * ACTIVE, REACTIVE & APPARENT POWER
 * ========================================================= */

float Metrology_PowerFromAWATT(int32_t raw);
float Metrology_PowerFromBWATT(int32_t raw);
float Metrology_PowerFromCWATT(int32_t raw);

float Metrology_ReactivePowerFromAVAR(int32_t raw);
float Metrology_ReactivePowerFromBVAR(int32_t raw);
float Metrology_ReactivePowerFromCVAR(int32_t raw);

float Metrology_ApparentPowerFromAVA(uint32_t raw);
float Metrology_ApparentPowerFromBVA(uint32_t raw);
float Metrology_ApparentPowerFromCVA(uint32_t raw);


/* =========================================================
 * POWER FACTOR & CALIBRATION GAIN
 * ========================================================= */

float    Metrology_PowerFactorFromAPF(int32_t raw);
float    Metrology_PowerFactorFromBPF(int32_t raw);
float    Metrology_PowerFactorFromCPF(int32_t raw);
uint16_t Metrology_PowerFactorPermilleFromRaw(int32_t raw);
int32_t  Metrology_CalculateGainRegister(float expected_val, float measured_val);


/* =========================================================
 * ENERGY
 * ========================================================= */

/*
 * Combine the 32-bit HI register and 13-bit LO register
 * into the signed 45-bit ADE9000 energy accumulator.
 */
int64_t Metrology_CombineEnergyRegister(uint32_t hi,
                                        uint32_t lo);


/*
 * Convert a raw ADE9000 energy accumulator value
 * into Wh.
 */
float Metrology_EnergyFromRaw(int64_t raw);


/*
 * Convert a raw ADE9000 energy accumulator value
 * into kWh.
 */
float Metrology_EnergyKWhFromRaw(int64_t raw);


/* =========================================================
 * FREQUENCY
 * ========================================================= */

float Metrology_FrequencyFromPeriod(uint32_t raw);


/* =========================================================
 * THD
 * ========================================================= */

float Metrology_THDFromRaw(uint32_t raw);


#endif
