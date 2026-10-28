#ifndef AMI_COSEM_MODEL_H
#define AMI_COSEM_MODEL_H

#include <stddef.h>
#include <stdint.h>
#include "ami_obis_catalog.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AMI_COSEM_LOGICAL_NAME_LENGTH 6u
#define AMI_COSEM_REGISTER_CLASS_ID 3u
#define AMI_COSEM_PROFILE_GENERIC_CLASS_ID 7u
#define AMI_COSEM_CLOCK_CLASS_ID 8u
#define AMI_COSEM_SECURITY_SETUP_CLASS_ID 64u
#define AMI_COSEM_REGISTER_SAMPLE_MAX 64u

#define AMI_COSEM_PROFILE_MAX_ENTRIES 8u
#define AMI_COSEM_AXDR_ENCODED_MAX 512u
#define AMI_COSEM_PROFILE_SELECTOR_RANGE 1u
#define AMI_COSEM_PROFILE_SELECTOR_ENTRY 2u

#define AMI_COSEM_VALID_EVENT_LOG (1u << 11)
#define AMI_COSEM_VALID_BILLING_PROFILE (1u << 12)
#define AMI_COSEM_VALID_SECURITY_CLIENT_TITLE (1u << 13)

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
} AmiCosemEventLogEntry;

typedef struct {
    AmiCosemDateTime billing_timestamp;
    uint64_t active_import_wh;
    uint64_t active_export_wh;
} AmiCosemBillingEntry;

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
    /* Bounded development/board-layer snapshots. Entries are oldest first;
     * the producer is responsible for ring-buffer rollover and timestamps. */
    AmiCosemEventLogEntry event_log[AMI_COSEM_PROFILE_MAX_ENTRIES];
    size_t event_log_count;
    AmiCosemBillingEntry billing_history[AMI_COSEM_PROFILE_MAX_ENTRIES];
    size_t billing_history_count;
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
