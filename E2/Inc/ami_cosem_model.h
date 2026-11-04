#ifndef AMI_COSEM_MODEL_H
#define AMI_COSEM_MODEL_H

#include <stddef.h>
#include <stdint.h>
#include "ami_obis_catalog.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AMI_COSEM_LOGICAL_NAME_LENGTH 6u
#define AMI_COSEM_DATA_CLASS_ID 1u
#define AMI_COSEM_REGISTER_CLASS_ID 3u
#define AMI_COSEM_EXTENDED_REGISTER_CLASS_ID 4u
#define AMI_COSEM_PROFILE_GENERIC_CLASS_ID 7u
#define AMI_COSEM_CLOCK_CLASS_ID 8u
#define AMI_COSEM_SECURITY_SETUP_CLASS_ID 64u
#define AMI_COSEM_DISCONNECT_CONTROL_CLASS_ID 70u
#define AMI_COSEM_LIMITER_CLASS_ID 71u
#define AMI_COSEM_REGISTER_SAMPLE_MAX 64u

#define AMI_COSEM_PROFILE_MAX_ENTRIES 8u
#define AMI_COSEM_AXDR_ENCODED_MAX 2048u
#define AMI_COSEM_PROFILE_SELECTOR_RANGE 1u
#define AMI_COSEM_PROFILE_SELECTOR_ENTRY 2u

#define AMI_COSEM_LOAD_PROFILE_CHANNELS 23u
#define AMI_COSEM_TAMPER_LOG_COLUMNS 3u

#define AMI_COSEM_VALID_EVENT_LOG (1u << 11)
#define AMI_COSEM_VALID_BILLING_PROFILE (1u << 12)
#define AMI_COSEM_VALID_SECURITY_CLIENT_TITLE (1u << 13)
#define AMI_COSEM_VALID_LOAD_PROFILE (1u << 14)
#define AMI_COSEM_VALID_TARIFF_REGISTERS (1u << 15)
#define AMI_COSEM_VALID_POWER_QUALITY    (1u << 16)
#define AMI_COSEM_VALID_LIMITER          (1u << 17)
#define AMI_COSEM_VALID_DISCONNECT_CTRL  (1u << 18)
#define AMI_COSEM_VALID_DEVICE_INFO      (1u << 19)

#define AMI_COSEM_VALID_CLOCK (1u << 10)

#define AMI_COSEM_VALID_VOLTAGE_A (1u << 0)
#define AMI_COSEM_VALID_VOLTAGE_B (1u << 1)
#define AMI_COSEM_VALID_VOLTAGE_C (1u << 2)
#define AMI_COSEM_VALID_CURRENT_A (1u << 3)
#define AMI_COSEM_VALID_CURRENT_B (1u << 4)
#define AMI_COSEM_VALID_CURRENT_C (1u << 5)
#define AMI_COSEM_VALID_ACTIVE_IMPORT (1u << 6)
#define AMI_COSEM_VALID_ACTIVE_EXPORT (1u << 7)
#define AMI_COSEM_VALID_REACTIVE_IMPORT (1u << 8)
#define AMI_COSEM_VALID_REACTIVE_EXPORT (1u << 9)

typedef struct {
    uint8_t year[2];
    uint8_t month;
    uint8_t day;
    uint8_t day_of_week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t hundredths;
    uint8_t deviation[2];
    uint8_t status;
} AmiCosemDateTime;

typedef struct {
    AmiCosemDateTime timestamp;
    uint8_t event_code;
    uint32_t status;
} AmiCosemEventLogEntry;

typedef struct {
    AmiCosemDateTime billing_timestamp;
    uint64_t active_import_wh;
    uint64_t active_export_wh;
    uint64_t reactive_import_varh;
    uint64_t reactive_export_varh;
} AmiCosemBillingEntry;

typedef struct {
    AmiCosemDateTime timestamp;
    uint32_t status;
    uint64_t active_import_wh;
    uint64_t active_export_wh;
    uint64_t reactive_import_varh;
    uint64_t reactive_export_varh;
    uint64_t reactive_q1_varh;
    uint64_t reactive_q2_varh;
    uint64_t reactive_q3_varh;
    uint64_t reactive_q4_varh;
    uint32_t phase_voltage_mv[3];
    uint32_t phase_current_ma[3];
    uint32_t neutral_current_ma;
    uint32_t active_power_watts;
    uint32_t reactive_power_vars;
    int32_t  power_factor;
    uint32_t frequency_mhz;
    uint32_t thd_voltage_percent_x10;
    uint32_t thd_current_percent_x10;
} AmiCosemLoadProfileEntry;

/* Raw register values supplied by metrology. The model only accepts logical
 * names present in its SPLN measurement catalog and supplies type/scaler/unit. */
typedef struct {
    uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH];
    uint64_t raw_value;
    int32_t signed_value;
    uint8_t valid;
} AmiCosemRegisterSample;

/* Snapshot values are supplied by the metrology/board layer. This module does
 * not synthesize measurements or access hardware. */
typedef struct {
    uint32_t phase_voltage_millivolts[3];
    uint32_t phase_current_milliamps[3];
    uint64_t active_import_wh;
    uint64_t active_export_wh;
    uint64_t reactive_import_varh;
    uint64_t reactive_export_varh;
    AmiCosemDateTime clock_date_time;
    int16_t clock_time_zone_minutes;
    uint8_t clock_status;
    uint8_t security_policy;
    uint8_t security_suite;
    uint8_t client_system_title[8];
    uint8_t server_system_title[8];
    uint32_t valid_mask;

    /* Multi-Tariff Energy Registers (Tariffs 1..4: LWBP1, WBP, LWBP2, T4) */
    uint64_t active_import_tariff_wh[4];
    uint64_t active_export_tariff_wh[4];
    uint64_t reactive_import_tariff_varh[4];

    /* Power Quality (PQ) & Instantaneous Metrics */
    uint32_t frequency_mhz;                    /* mHz: 50000 = 50.00 Hz (Scaler -3, Hz) */
    int32_t  power_factor[4];                  /* Scaler -3: 0=total, 1=L1, 2=L2, 3=L3 (985 = 0.985) */
    uint32_t thd_voltage_percent_x10[4];       /* Scaler -1: 0=total, 1=L1, 2=L2, 3=L3 (15 = 1.5%) */
    uint32_t thd_current_percent_x10[4];       /* Scaler -1: 0=total, 1=L1, 2=L2, 3=L3 (25 = 2.5%) */
    uint32_t active_power_watts;               /* Scaler 0: Total Active Power +P */
    uint32_t reactive_power_vars;              /* Scaler 0: Total Reactive Power +Q */

    /* Demand Limiter Class 71 */
    uint32_t limiter_threshold_active_watts;
    uint32_t limiter_threshold_normal_watts;
    uint32_t limiter_min_over_threshold_duration_sec;
    uint32_t limiter_min_under_threshold_duration_sec;
    uint8_t  limiter_emergency_profile_active;

    /* Disconnect Control Class 70 */
    uint8_t  disconnect_output_state;          /* 1 = connected, 0 = disconnected */
    uint8_t  disconnect_control_state;         /* 1 = connected, 0 = disconnected, 2 = ready */
    uint8_t  disconnect_control_mode;          /* e.g. 2 */

    /* Device Info Visible Strings */
    char meter_id[17];                         /* e.g. "PLN1234567890" */
    char meter_type[32];                       /* e.g. "3-PHASE-AMI-POSTPAID" */

    /* Bounded development/board-layer snapshots. Entries are oldest first;
     * the producer is responsible for ring-buffer rollover and timestamps. */
    AmiCosemEventLogEntry event_log[AMI_COSEM_PROFILE_MAX_ENTRIES];
    size_t event_log_count;
    AmiCosemBillingEntry billing_history[AMI_COSEM_PROFILE_MAX_ENTRIES];
    size_t billing_history_count;
    AmiCosemLoadProfileEntry load_profile[AMI_COSEM_PROFILE_MAX_ENTRIES];
    size_t load_profile_count;
    AmiCosemRegisterSample measurement_samples[AMI_COSEM_REGISTER_SAMPLE_MAX];
    size_t measurement_sample_count;
} AmiCosemRegisterSnapshot;

typedef enum {
    AMI_COSEM_OK = 0,
    AMI_COSEM_INVALID_ARGUMENT,
    AMI_COSEM_OBJECT_NOT_FOUND,
    AMI_COSEM_ATTRIBUTE_NOT_FOUND,
    AMI_COSEM_VALUE_UNAVAILABLE,
    AMI_COSEM_SCOPE_VIOLATED
} AmiCosemStatus;

/* A-XDR/COSEM data type tags used by this register model. */
typedef enum {
    AMI_COSEM_TYPE_ENCODED = 0, /* Internal: value already contains A-XDR. */
    AMI_COSEM_TYPE_STRUCTURE = 2,
    AMI_COSEM_TYPE_DOUBLE_LONG = 5,
    AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED = 6,
    AMI_COSEM_TYPE_OCTET_STRING = 9,
    AMI_COSEM_TYPE_VISIBLE_STRING = 10,
    AMI_COSEM_TYPE_UNSIGNED = 17,
    AMI_COSEM_TYPE_INTEGER = 15,
    AMI_COSEM_TYPE_LONG_UNSIGNED = 18,
    AMI_COSEM_TYPE_LONG64_UNSIGNED = 21,
    AMI_COSEM_TYPE_ENUM = 22
} AmiCosemDataType;

typedef struct {
    int8_t scaler;
    uint8_t unit;
} AmiCosemScalerUnit;

typedef struct {
    AmiCosemDataType type;
    size_t length;
    union {
        uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH];
        uint32_t double_long_unsigned;
        uint64_t long64_unsigned;
        AmiCosemScalerUnit scaler_unit;
        int32_t double_long;
        uint8_t unsigned_integer;
        uint8_t octets[32];
        AmiCosemDateTime date_time;
        uint8_t encoded[AMI_COSEM_AXDR_ENCODED_MAX];
    } value;
} AmiCosemData;

typedef struct {
    uint8_t selector;
    uint32_t from_entry;
    uint32_t to_entry; /* 0 means through the newest entry for selector 2. */
    AmiCosemDateTime from_date;
    AmiCosemDateTime to_date;
} AmiCosemProfileSelection;

/* Read-only Class-3 Register, Class-7 Profile Generic, and Class-8 Clock. */
AmiCosemStatus ami_cosem_register_read(
    const AmiCosemRegisterSnapshot *snapshot,
    uint16_t class_id,
    const uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH],
    uint8_t attribute_id,
    AmiCosemData *out);

/* Same as ami_cosem_register_read, with Profile Generic buffer selection.
 * A NULL selection reads the complete buffer. */
AmiCosemStatus ami_cosem_register_read_selected(
    const AmiCosemRegisterSnapshot *snapshot,
    uint16_t class_id,
    const uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH],
    uint8_t attribute_id,
    const AmiCosemProfileSelection *selection,
    AmiCosemData *out);

size_t ami_cosem_register_count(void);
size_t ami_cosem_object_count(void);

#ifdef __cplusplus
}
#endif

#endif
