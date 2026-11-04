#ifndef AMI_ADE9000_H
#define AMI_ADE9000_H

#include <stddef.h>
#include <stdint.h>
#include "ami_cosem_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * ADE9000 Register Addresses (12-bit address mapped to SPI 16-bit word)
 * -------------------------------------------------------------------------- */
#define ADE9000_REG_AIRMS          0x20Cu
#define ADE9000_REG_AVRMS          0x20Du
#define ADE9000_REG_APERIOD        0x20Eu
#define ADE9000_REG_AWATT          0x210u
#define ADE9000_REG_AWATTHR_HI     0x213u
#define ADE9000_REG_AVAR           0x214u
#define ADE9000_REG_AFVARHR_HI     0x217u
#define ADE9000_REG_AVA            0x218u
#define ADE9000_REG_APF            0x21Au
#define ADE9000_REG_AVAHR_HI       0x21Bu

#define ADE9000_REG_BIRMS          0x22Cu
#define ADE9000_REG_BVRMS          0x22Du
#define ADE9000_REG_BPERIOD        0x22Eu
#define ADE9000_REG_BWATT          0x230u
#define ADE9000_REG_BWATTHR_HI     0x233u
#define ADE9000_REG_BVAR           0x234u
#define ADE9000_REG_BFVARHR_HI     0x237u
#define ADE9000_REG_BVA            0x238u
#define ADE9000_REG_BPF            0x23Au
#define ADE9000_REG_BVAHR_HI       0x23Bu

#define ADE9000_REG_CIRMS          0x24Cu
#define ADE9000_REG_CVRMS          0x24Du
#define ADE9000_REG_CPERIOD        0x24Eu
#define ADE9000_REG_CWATT          0x250u
#define ADE9000_REG_CWATTHR_HI     0x253u
#define ADE9000_REG_CVAR           0x254u
#define ADE9000_REG_CFVARHR_HI     0x257u
#define ADE9000_REG_CVA            0x258u
#define ADE9000_REG_CPF            0x25Au
#define ADE9000_REG_CVAHR_HI       0x25Bu

#define ADE9000_REG_NIRMS          0x26Cu
#define ADE9000_REG_STATUS0        0x402u
#define ADE9000_REG_STATUS1        0x403u
#define ADE9000_REG_MASK0          0x405u
#define ADE9000_REG_MASK1          0x406u

/* Status0 / Status1 Tamper & Anomaly Event Bits (SPLN D3.006:2021) */
#define ADE9000_STATUS0_REVAPA     (1u << 16)  /* Reverse active power phase A */
#define ADE9000_STATUS0_REVAPB     (1u << 17)  /* Reverse active power phase B */
#define ADE9000_STATUS0_REVAPC     (1u << 18)  /* Reverse active power phase C */
#define ADE9000_STATUS0_PHASE_SEQ  (1u << 19)  /* Phase sequence mismatch (ERR21) */
#define ADE9000_STATUS1_ZXTOA      (1u << 0)   /* Zero crossing timeout phase A */
#define ADE9000_STATUS1_ZXTOB      (1u << 1)   /* Zero crossing timeout phase B */
#define ADE9000_STATUS1_ZXTOC      (1u << 2)   /* Zero crossing timeout phase C */

/* --------------------------------------------------------------------------
 * Calibration & Conversion Parameters (Defaults for 230V, 5(10)A Class 0.5S)
 * -------------------------------------------------------------------------- */
#define ADE9000_DEFAULT_V_SCALE_NUM    230000u /* millivolts */
#define ADE9000_DEFAULT_V_SCALE_DEN    52702092u /* raw code nominal */

#define ADE9000_DEFAULT_I_SCALE_NUM    5000u   /* milliamps */
#define ADE9000_DEFAULT_I_SCALE_DEN    26351046u /* raw code nominal */

#define ADE9000_DEFAULT_W_SCALE_NUM    1150u   /* milliwatts (230V * 5A = 1150W) */
#define ADE9000_DEFAULT_W_SCALE_DEN    20638835u

#define ADE9000_DEFAULT_WH_PER_PULSE   1u      /* 1 Wh per accumulation unit */

typedef struct {
    uint32_t v_scale_num;
    uint32_t v_scale_den;
    uint32_t i_scale_num;
    uint32_t i_scale_den;
    uint32_t wh_per_code;
    uint32_t varh_per_code;
} AmiAde9000Calibration;

typedef struct {
    uint32_t vrms_raw[3];     /* A, B, C */
    uint32_t irms_raw[3];     /* A, B, C */
    uint32_t irms_neutral_raw;
    int32_t  watt_raw[3];
    int32_t  var_raw[3];
    int32_t  va_raw[3];
    int16_t  pf_raw[3];
    uint32_t period_raw[3];
    uint32_t watthr_hi_raw[3];
    uint32_t fvarhr_hi_raw[3];
    uint32_t vahr_hi_raw[3];
    uint32_t status0;
    uint32_t status1;
} AmiAde9000RawRegisters;

#define AMI_ADE9000_OK                  0
#define AMI_ADE9000_ERR_PARAM          -1
#define AMI_ADE9000_ERR_SPI            -2
#define AMI_ADE9000_ERR_NOT_INIT       -3
#define AMI_ADE9000_ERR_INVALID_DATA   -4

typedef struct {
    uint32_t phase_voltage_mv[3];      /* Phase voltages in millivolts */
    uint32_t phase_current_ma[3];      /* Phase currents in milliamps */
    uint32_t neutral_current_ma;       /* Neutral current in milliamps */
    int32_t  active_power_w[3];        /* Active power per phase (W) */
    int32_t  total_active_power_w;     /* Total active power (W) */
    int32_t  reactive_power_var[3];    /* Reactive power per phase (var) */
    int32_t  total_reactive_power_var; /* Total reactive power (var) */
    uint32_t apparent_power_va[3];     /* Apparent power per phase (VA) */
    uint32_t total_apparent_power_va;  /* Total apparent power (VA) */
    int16_t  power_factor[3];          /* Power factor * 1000 */
    uint32_t frequency_mhz;            /* Frequency in mHz (e.g. 50000 = 50.00 Hz) */
    uint64_t active_import_wh;         /* Cumulative active import Wh */
    uint64_t active_export_wh;         /* Cumulative active export Wh */
    uint64_t reactive_import_varh;     /* Cumulative reactive import varh */
    uint64_t reactive_export_varh;     /* Cumulative reactive export varh */
    uint8_t  reverse_power_flags;      /* Bit 0: A, Bit 1: B, Bit 2: C */
    uint8_t  phase_sequence_error;     /* 1 if reverse phase sequence */
    uint8_t  loss_of_phase_flags;      /* Bit 0: A, Bit 1: B, Bit 2: C */
    uint8_t  is_valid;                 /* 1 if all registers successfully acquired */
    int32_t  last_error_code;          /* 0 if healthy, or AMI_ADE9000_ERR_* */
} AmiAde9000ScaledReadings;

/**
 * @brief Hardware SPI read/write callback interface.
 * @return 0 on success, negative error code on failure.
 */
typedef int (*AmiAde9000SpiTransferFunc)(uint16_t reg_addr,
                                        uint8_t is_read,
                                        uint32_t *data,
                                        void *user_data);

typedef struct {
    AmiAde9000Calibration cal;
    AmiAde9000RawRegisters raw;
    AmiAde9000ScaledReadings scaled;
    AmiAde9000SpiTransferFunc spi_transfer;
    void *user_data;
    int is_initialized;
} AmiAde9000Context;

/**
 * @brief Initialize ADE9000 metering driver with calibration and SPI transport.
 */
int ami_ade9000_init(AmiAde9000Context *ctx,
                     const AmiAde9000Calibration *cal,
                     AmiAde9000SpiTransferFunc spi_func,
                     void *user_data);

/**
 * @brief Perform a 32-bit register read over SPI.
 */
int ami_ade9000_read_reg(AmiAde9000Context *ctx, uint16_t reg_addr, uint32_t *val);

/**
 * @brief Perform a 32-bit register write over SPI.
 */
int ami_ade9000_write_reg(AmiAde9000Context *ctx, uint16_t reg_addr, uint32_t val);

/**
 * @brief Acquire a full snapshot of instantaneous and energy registers from ADE9000.
 */
int ami_ade9000_acquire_all(AmiAde9000Context *ctx);

/**
 * @brief Convert raw ADE9000 registers to physical scaled values.
 */
void ami_ade9000_process_scaling(AmiAde9000Context *ctx);

/**
 * @brief Feed converted ADE9000 measurements directly into COSEM snapshot buffer.
 */
int ami_ade9000_feed_cosem_snapshot(const AmiAde9000Context *ctx,
                                   AmiCosemRegisterSnapshot *snapshot);

#ifdef __cplusplus
}
#endif

#endif /* AMI_ADE9000_H */

