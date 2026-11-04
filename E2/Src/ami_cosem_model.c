#include "ami_cosem_model.h"

#include <string.h>

_Static_assert(sizeof(AmiCosemDateTime) == 12u,
               "COSEM Clock date-time representation must be 12 bytes");

enum {
    COSEM_UNIT_WATT = 27,
    COSEM_UNIT_VOLT_AMPERE = 28,
    COSEM_UNIT_VAR = 29,
    COSEM_UNIT_ACTIVE_ENERGY_WH = 30,
    COSEM_UNIT_VOLT_AMPERE_HOUR = 31,
    COSEM_UNIT_REACTIVE_ENERGY_VARH = 32,
    COSEM_UNIT_CURRENT_AMPERE = 33,
    COSEM_UNIT_VOLT = 35,
    COSEM_UNIT_HERTZ = 44,
    COSEM_UNIT_DIMENSIONLESS = 255
};

typedef enum {
    VALUE_VOLTAGE,
    VALUE_CURRENT,
    VALUE_ACTIVE_IMPORT,
    VALUE_ACTIVE_EXPORT,
    VALUE_REACTIVE_IMPORT,
    VALUE_REACTIVE_EXPORT,
    VALUE_ACTIVE_IMPORT_TARIFF,
    VALUE_ACTIVE_EXPORT_TARIFF,
    VALUE_REACTIVE_IMPORT_TARIFF,
    VALUE_FREQUENCY,
    VALUE_POWER_FACTOR,
    VALUE_THD_VOLTAGE,
    VALUE_THD_CURRENT,
    VALUE_ACTIVE_POWER_TOTAL,
    VALUE_REACTIVE_POWER_TOTAL
} ValueSource;

typedef struct {
    uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH];
    ValueSource source;
    uint8_t phase;
    AmiCosemDataType value_type;
    int8_t scaler;
    uint8_t unit;
    uint32_t valid_mask;
} RegisterDefinition;

typedef struct {
    uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH];
    AmiCosemDataType value_type;
    int8_t scaler;
    uint8_t unit;
} ExtendedRegisterDefinition;

#define OBIS(a, b, c, d, e, f) {a, b, c, d, e, f}

static const RegisterDefinition registers[] = {
    /* Instantaneous Voltage (L1, L2, L3) */
    {OBIS(1, 0, 32, 7, 0, 255), VALUE_VOLTAGE, 0, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -3, COSEM_UNIT_VOLT, AMI_COSEM_VALID_VOLTAGE_A},
    {OBIS(1, 0, 52, 7, 0, 255), VALUE_VOLTAGE, 1, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -3, COSEM_UNIT_VOLT, AMI_COSEM_VALID_VOLTAGE_B},
    {OBIS(1, 0, 72, 7, 0, 255), VALUE_VOLTAGE, 2, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -3, COSEM_UNIT_VOLT, AMI_COSEM_VALID_VOLTAGE_C},

    /* Instantaneous Current (L1, L2, L3) */
    {OBIS(1, 0, 31, 7, 0, 255), VALUE_CURRENT, 0, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -3, COSEM_UNIT_CURRENT_AMPERE, AMI_COSEM_VALID_CURRENT_A},
    {OBIS(1, 0, 51, 7, 0, 255), VALUE_CURRENT, 1, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -3, COSEM_UNIT_CURRENT_AMPERE, AMI_COSEM_VALID_CURRENT_B},
    {OBIS(1, 0, 71, 7, 0, 255), VALUE_CURRENT, 2, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -3, COSEM_UNIT_CURRENT_AMPERE, AMI_COSEM_VALID_CURRENT_C},

    /* Cumulative Energy Totals (Active & Reactive) */
    {OBIS(1, 0, 1, 8, 0, 255), VALUE_ACTIVE_IMPORT, 0, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH, AMI_COSEM_VALID_ACTIVE_IMPORT},
    {OBIS(1, 0, 2, 8, 0, 255), VALUE_ACTIVE_EXPORT, 0, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH, AMI_COSEM_VALID_ACTIVE_EXPORT},
    {OBIS(1, 0, 3, 8, 0, 255), VALUE_REACTIVE_IMPORT, 0, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH, AMI_COSEM_VALID_REACTIVE_IMPORT},
    {OBIS(1, 0, 4, 8, 0, 255), VALUE_REACTIVE_EXPORT, 0, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH, AMI_COSEM_VALID_REACTIVE_EXPORT},

    /* Multi-Tarif Active Energy Import (+A Tariffs 1..4: LWBP1, WBP, LWBP2, T4) */
    {OBIS(1, 0, 1, 8, 1, 255), VALUE_ACTIVE_IMPORT_TARIFF, 0, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH, AMI_COSEM_VALID_TARIFF_REGISTERS},
    {OBIS(1, 0, 1, 8, 2, 255), VALUE_ACTIVE_IMPORT_TARIFF, 1, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH, AMI_COSEM_VALID_TARIFF_REGISTERS},
    {OBIS(1, 0, 1, 8, 3, 255), VALUE_ACTIVE_IMPORT_TARIFF, 2, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH, AMI_COSEM_VALID_TARIFF_REGISTERS},
    {OBIS(1, 0, 1, 8, 4, 255), VALUE_ACTIVE_IMPORT_TARIFF, 3, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH, AMI_COSEM_VALID_TARIFF_REGISTERS},

    /* Multi-Tarif Active Energy Export (-A Tariffs 1..4) */
    {OBIS(1, 0, 2, 8, 1, 255), VALUE_ACTIVE_EXPORT_TARIFF, 0, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH, AMI_COSEM_VALID_TARIFF_REGISTERS},
    {OBIS(1, 0, 2, 8, 2, 255), VALUE_ACTIVE_EXPORT_TARIFF, 1, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH, AMI_COSEM_VALID_TARIFF_REGISTERS},
    {OBIS(1, 0, 2, 8, 3, 255), VALUE_ACTIVE_EXPORT_TARIFF, 2, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH, AMI_COSEM_VALID_TARIFF_REGISTERS},
    {OBIS(1, 0, 2, 8, 4, 255), VALUE_ACTIVE_EXPORT_TARIFF, 3, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH, AMI_COSEM_VALID_TARIFF_REGISTERS},

    /* Multi-Tarif Reactive Energy Import (+R Tariffs 1..4) */
    {OBIS(1, 0, 3, 8, 1, 255), VALUE_REACTIVE_IMPORT_TARIFF, 0, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH, AMI_COSEM_VALID_TARIFF_REGISTERS},
    {OBIS(1, 0, 3, 8, 2, 255), VALUE_REACTIVE_IMPORT_TARIFF, 1, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH, AMI_COSEM_VALID_TARIFF_REGISTERS},
    {OBIS(1, 0, 3, 8, 3, 255), VALUE_REACTIVE_IMPORT_TARIFF, 2, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH, AMI_COSEM_VALID_TARIFF_REGISTERS},
    {OBIS(1, 0, 3, 8, 4, 255), VALUE_REACTIVE_IMPORT_TARIFF, 3, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH, AMI_COSEM_VALID_TARIFF_REGISTERS},

    /* Power Quality: Frequency & Power Factor */
    {OBIS(1, 0, 14, 7, 0, 255), VALUE_FREQUENCY, 0, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -3, COSEM_UNIT_HERTZ, AMI_COSEM_VALID_POWER_QUALITY},
    {OBIS(1, 0, 13, 7, 0, 255), VALUE_POWER_FACTOR, 0, AMI_COSEM_TYPE_DOUBLE_LONG, -3, COSEM_UNIT_DIMENSIONLESS, AMI_COSEM_VALID_POWER_QUALITY},
    {OBIS(1, 0, 33, 7, 0, 255), VALUE_POWER_FACTOR, 1, AMI_COSEM_TYPE_DOUBLE_LONG, -3, COSEM_UNIT_DIMENSIONLESS, AMI_COSEM_VALID_POWER_QUALITY},
    {OBIS(1, 0, 53, 7, 0, 255), VALUE_POWER_FACTOR, 2, AMI_COSEM_TYPE_DOUBLE_LONG, -3, COSEM_UNIT_DIMENSIONLESS, AMI_COSEM_VALID_POWER_QUALITY},
    {OBIS(1, 0, 73, 7, 0, 255), VALUE_POWER_FACTOR, 3, AMI_COSEM_TYPE_DOUBLE_LONG, -3, COSEM_UNIT_DIMENSIONLESS, AMI_COSEM_VALID_POWER_QUALITY},

    /* Power Quality: Voltage THD (Total, L1, L2, L3) */
    {OBIS(1, 0, 12, 7, 124, 255), VALUE_THD_VOLTAGE, 0, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -1, COSEM_UNIT_DIMENSIONLESS, AMI_COSEM_VALID_POWER_QUALITY},
    {OBIS(1, 0, 32, 7, 124, 255), VALUE_THD_VOLTAGE, 1, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -1, COSEM_UNIT_DIMENSIONLESS, AMI_COSEM_VALID_POWER_QUALITY},
    {OBIS(1, 0, 52, 7, 124, 255), VALUE_THD_VOLTAGE, 2, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -1, COSEM_UNIT_DIMENSIONLESS, AMI_COSEM_VALID_POWER_QUALITY},
    {OBIS(1, 0, 72, 7, 124, 255), VALUE_THD_VOLTAGE, 3, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -1, COSEM_UNIT_DIMENSIONLESS, AMI_COSEM_VALID_POWER_QUALITY},

    /* Power Quality: Current THD (Total, L1, L2, L3) */
    {OBIS(1, 0, 11, 7, 124, 255), VALUE_THD_CURRENT, 0, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -1, COSEM_UNIT_DIMENSIONLESS, AMI_COSEM_VALID_POWER_QUALITY},
    {OBIS(1, 0, 31, 7, 124, 255), VALUE_THD_CURRENT, 1, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -1, COSEM_UNIT_DIMENSIONLESS, AMI_COSEM_VALID_POWER_QUALITY},
    {OBIS(1, 0, 51, 7, 124, 255), VALUE_THD_CURRENT, 2, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -1, COSEM_UNIT_DIMENSIONLESS, AMI_COSEM_VALID_POWER_QUALITY},
    {OBIS(1, 0, 71, 7, 124, 255), VALUE_THD_CURRENT, 3, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -1, COSEM_UNIT_DIMENSIONLESS, AMI_COSEM_VALID_POWER_QUALITY},

    /* Instantaneous Power (Total Active & Reactive Power) */
    {OBIS(1, 0, 1, 7, 0, 255), VALUE_ACTIVE_POWER_TOTAL, 0, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_WATT, AMI_COSEM_VALID_ACTIVE_IMPORT},
    {OBIS(1, 0, 3, 7, 0, 255), VALUE_REACTIVE_POWER_TOTAL, 0, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VAR, AMI_COSEM_VALID_REACTIVE_IMPORT}
};


#define EXT(a, b, c, d, e, f, type, scale, unit_value) \
    {OBIS(a, b, c, d, e, f), type, scale, unit_value}
static const ExtendedRegisterDefinition extended_registers[] = {
    EXT(1,0,14,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -3, COSEM_UNIT_HERTZ),
    EXT(1,0,91,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -3, COSEM_UNIT_CURRENT_AMPERE),
    EXT(1,0,92,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, -3, COSEM_UNIT_VOLT),
    EXT(1,0,13,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG, -3, COSEM_UNIT_DIMENSIONLESS),
    EXT(1,0,33,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG, -3, COSEM_UNIT_DIMENSIONLESS),
    EXT(1,0,53,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG, -3, COSEM_UNIT_DIMENSIONLESS),
    EXT(1,0,73,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG, -3, COSEM_UNIT_DIMENSIONLESS),
    EXT(1,0,1,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_WATT),
    EXT(1,0,2,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_WATT),
    EXT(1,0,3,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VAR),
    EXT(1,0,4,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VAR),
    EXT(1,0,9,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE),
    EXT(1,0,10,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE),
    EXT(1,0,21,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_WATT),
    EXT(1,0,41,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_WATT),
    EXT(1,0,61,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_WATT),
    EXT(1,0,22,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_WATT),
    EXT(1,0,42,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_WATT),
    EXT(1,0,62,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_WATT),
    EXT(1,0,23,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VAR),
    EXT(1,0,43,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VAR),
    EXT(1,0,63,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VAR),
    EXT(1,0,24,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VAR),
    EXT(1,0,44,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VAR),
    EXT(1,0,64,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VAR),
    EXT(1,0,29,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE),
    EXT(1,0,49,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE),
    EXT(1,0,69,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE),
    EXT(1,0,30,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE),
    EXT(1,0,50,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE),
    EXT(1,0,70,7,0,255, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE),
    EXT(1,0,21,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH),
    EXT(1,0,41,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH),
    EXT(1,0,61,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH),
    EXT(1,0,22,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH),
    EXT(1,0,42,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH),
    EXT(1,0,62,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_ACTIVE_ENERGY_WH),
    EXT(1,0,23,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,43,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,63,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,24,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,44,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,64,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,29,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE_HOUR),
    EXT(1,0,49,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE_HOUR),
    EXT(1,0,69,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE_HOUR),
    EXT(1,0,30,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE_HOUR),
    EXT(1,0,50,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE_HOUR),
    EXT(1,0,70,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_VOLT_AMPERE_HOUR),
    EXT(1,0,25,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,45,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,65,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,26,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,66,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,27,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,47,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,67,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,28,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,48,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,68,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),

    /* Total Reactive Energy per Quadrant Q1..Q4 */
    EXT(1,0,5,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,6,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,7,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH),
    EXT(1,0,8,8,0,255, AMI_COSEM_TYPE_LONG64_UNSIGNED, 0, COSEM_UNIT_REACTIVE_ENERGY_VARH)
};
#undef EXT
#undef OBIS

static const uint8_t clock_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {0, 0, 1, 0, 0, 255};
/* Standard event-log and tamper-log instances. Billing profile and Load profile. */
static const uint8_t event_log_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {0, 0, 99, 98, 0, 255};
static const uint8_t tamper_log_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {0, 0, 99, 98, 1, 255};
static const uint8_t billing_profile_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {0, 0, 98, 1, 0, 255};
static const uint8_t billing_profile_alt_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 98, 1, 0, 255};
static const uint8_t load_profile_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 99, 1, 0, 255};

static const uint8_t event_code_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {0, 0, 96, 11, 0, 255};
static const uint8_t status_reg_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {0, 0, 96, 10, 1, 255};
static const uint8_t active_import_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 1, 8, 0, 255};
static const uint8_t active_export_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 2, 8, 0, 255};
static const uint8_t reactive_import_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 3, 8, 0, 255};
static const uint8_t reactive_export_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 4, 8, 0, 255};
static const uint8_t reactive_q1_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 5, 8, 0, 255};
static const uint8_t reactive_q2_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 6, 8, 0, 255};
static const uint8_t reactive_q3_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 7, 8, 0, 255};
static const uint8_t reactive_q4_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 8, 8, 0, 255};
static const uint8_t voltage_a_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 32, 7, 0, 255};
static const uint8_t voltage_b_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 52, 7, 0, 255};
static const uint8_t voltage_c_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 72, 7, 0, 255};
static const uint8_t current_a_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 31, 7, 0, 255};
static const uint8_t current_b_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 51, 7, 0, 255};
static const uint8_t current_c_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 71, 7, 0, 255};
static const uint8_t current_neutral_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 91, 7, 0, 255};
static const uint8_t active_power_total_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 1, 7, 0, 255};
static const uint8_t reactive_power_total_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 3, 7, 0, 255};
static const uint8_t power_factor_total_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 13, 7, 0, 255};
static const uint8_t frequency_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 14, 7, 0, 255};
static const uint8_t thd_voltage_l1_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 32, 7, 124, 255};
static const uint8_t thd_current_l1_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {1, 0, 31, 7, 124, 255};

static const uint8_t security_setup_logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH] =
    {0, 0, 43, 0, 0, 255};

typedef struct {
    uint16_t class_id;
    const uint8_t *logical_name;
    int8_t attribute_id;
    uint16_t data_index;
} CaptureObject;

static const CaptureObject event_capture_objects[] = {
    {AMI_COSEM_CLOCK_CLASS_ID, clock_logical_name, 2, 0},
    {1u, event_code_logical_name, 2, 0},
    {1u, status_reg_logical_name, 2, 0}
};
static const CaptureObject billing_capture_objects[] = {
    {AMI_COSEM_CLOCK_CLASS_ID, clock_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, active_import_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, active_export_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, reactive_import_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, reactive_export_logical_name, 2, 0}
};
static const CaptureObject load_profile_capture_objects[] = {
    {AMI_COSEM_CLOCK_CLASS_ID, clock_logical_name, 2, 0},
    {1u, status_reg_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, active_import_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, active_export_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, reactive_import_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, reactive_export_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, reactive_q1_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, reactive_q2_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, reactive_q3_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, reactive_q4_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, voltage_a_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, voltage_b_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, voltage_c_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, current_a_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, current_b_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, current_c_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, current_neutral_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, active_power_total_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, reactive_power_total_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, power_factor_total_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, frequency_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, thd_voltage_l1_logical_name, 2, 0},
    {AMI_COSEM_REGISTER_CLASS_ID, thd_current_l1_logical_name, 2, 0}
};

static int put_byte(AmiCosemData *out, size_t *offset, uint8_t value)
{
    if (*offset >= sizeof(out->value.encoded)) return 0;
    out->value.encoded[(*offset)++] = value;
    return 1;
}

static int put_bytes(AmiCosemData *out, size_t *offset,
                     const uint8_t *bytes, size_t length)
{
    if (length > sizeof(out->value.encoded) - *offset) return 0;
    memcpy(out->value.encoded + *offset, bytes, length);
    *offset += length;
    return 1;
}

static int put_u16(AmiCosemData *out, size_t *offset, uint16_t value)
{
    return put_byte(out, offset, (uint8_t)(value >> 8)) &&
           put_byte(out, offset, (uint8_t)value);
}

static int put_u32(AmiCosemData *out, size_t *offset, uint32_t value)
{
    return put_byte(out, offset, (uint8_t)(value >> 24)) &&
           put_byte(out, offset, (uint8_t)(value >> 16)) &&
           put_byte(out, offset, (uint8_t)(value >> 8)) &&
           put_byte(out, offset, (uint8_t)value);
}

static int put_u64(AmiCosemData *out, size_t *offset, uint64_t value)
{
    for (int shift = 56; shift >= 0; shift -= 8) {
        if (!put_byte(out, offset, (uint8_t)(value >> shift))) return 0;
    }
    return 1;
}

static int put_datetime(AmiCosemData *out, size_t *offset,
                        const AmiCosemDateTime *date_time)
{
    const uint8_t *bytes = (const uint8_t *)date_time;
    return put_byte(out, offset, AMI_COSEM_TYPE_OCTET_STRING) &&
           put_byte(out, offset, (uint8_t)sizeof(*date_time)) &&
           put_bytes(out, offset, bytes, sizeof(*date_time));
}

static int put_capture_object(AmiCosemData *out, size_t *offset,
                              const CaptureObject *object)
{
    if (!put_byte(out, offset, AMI_COSEM_TYPE_STRUCTURE) ||
        !put_byte(out, offset, 4u) ||
        !put_byte(out, offset, AMI_COSEM_TYPE_LONG_UNSIGNED) ||
        !put_u16(out, offset, object->class_id) ||
        !put_byte(out, offset, AMI_COSEM_TYPE_OCTET_STRING) ||
        !put_byte(out, offset, AMI_COSEM_LOGICAL_NAME_LENGTH) ||
        !put_bytes(out, offset, object->logical_name, AMI_COSEM_LOGICAL_NAME_LENGTH) ||
        !put_byte(out, offset, AMI_COSEM_TYPE_INTEGER) ||
        !put_byte(out, offset, (uint8_t)object->attribute_id) ||
        !put_byte(out, offset, AMI_COSEM_TYPE_LONG_UNSIGNED) ||
        !put_u16(out, offset, object->data_index)) return 0;
    return 1;
}

static int put_capture_array(AmiCosemData *out, size_t *offset,
                             const CaptureObject *objects, size_t count)
{
    if (!put_byte(out, offset, 1u) || !put_byte(out, offset, (uint8_t)count)) return 0;
    for (size_t i = 0; i < count; ++i) {
        if (!put_capture_object(out, offset, &objects[i])) return 0;
    }
    return 1;
}

static int compare_profile_datetime(const AmiCosemDateTime *left,
                                    const AmiCosemDateTime *right)
{
    const uint8_t left_fields[] = {
        left->year[0], left->year[1], left->month, left->day,
        left->hour, left->minute, left->second, left->hundredths
    };
    const uint8_t right_fields[] = {
        right->year[0], right->year[1], right->month, right->day,
        right->hour, right->minute, right->second, right->hundredths
    };
    /* Handle DLMS wildcards: 0xFF in fields indicates unspecified/wildcard */
    for (size_t i = 0u; i < sizeof(left_fields); ++i) {
        if (i < 2u && ((left->year[0] == 0xFFu && left->year[1] == 0xFFu) ||
                       (right->year[0] == 0xFFu && right->year[1] == 0xFFu))) {
            i = 1u; /* skip both year bytes */
            continue;
        }
        if (left_fields[i] == 0xFFu || right_fields[i] == 0xFFu) continue;
        if (left_fields[i] < right_fields[i]) return -1;
        if (left_fields[i] > right_fields[i]) return 1;
    }
    return 0;
}

static AmiCosemStatus read_profile(const AmiCosemRegisterSnapshot *snapshot,
                                   const uint8_t *logical_name,
                                   uint8_t attribute_id,
                                   const AmiCosemProfileSelection *selection,
                                   AmiCosemData *out)
{
    int specific_tamper_code = -1;
    const int is_generic_event = (memcmp(logical_name, event_log_logical_name,
                                         AMI_COSEM_LOGICAL_NAME_LENGTH) == 0) ||
                                 (memcmp(logical_name, tamper_log_logical_name,
                                         AMI_COSEM_LOGICAL_NAME_LENGTH) == 0);
    const int is_specific_tamper = (logical_name[0] == 1 && logical_name[1] == 97 &&
                                    logical_name[2] == 99 && logical_name[3] == 98 &&
                                    logical_name[5] == 255);
    if (is_specific_tamper) {
        specific_tamper_code = (int)logical_name[4];
    }
    const int is_event = is_generic_event || is_specific_tamper;
    const int is_billing = (memcmp(logical_name, billing_profile_logical_name,
                                   AMI_COSEM_LOGICAL_NAME_LENGTH) == 0) ||
                           (memcmp(logical_name, billing_profile_alt_logical_name,
                                   AMI_COSEM_LOGICAL_NAME_LENGTH) == 0);
    const int is_load = (memcmp(logical_name, load_profile_logical_name,
                                AMI_COSEM_LOGICAL_NAME_LENGTH) == 0);

    if (!is_event && !is_billing && !is_load) return AMI_COSEM_OBJECT_NOT_FOUND;
    if (selection != NULL &&
        selection->selector != AMI_COSEM_PROFILE_SELECTOR_ENTRY &&
        selection->selector != AMI_COSEM_PROFILE_SELECTOR_RANGE) {
        return AMI_COSEM_SCOPE_VIOLATED;
    }
    if (selection != NULL && selection->selector == AMI_COSEM_PROFILE_SELECTOR_RANGE &&
        compare_profile_datetime(&selection->from_date, &selection->to_date) > 0) {
        return AMI_COSEM_SCOPE_VIOLATED;
    }

    size_t count = 0u;
    uint32_t valid_bit = 0u;
    const CaptureObject *captures = NULL;
    size_t capture_count = 0u;
    uint32_t capture_period = 0u;

    if (is_load) {
        count = snapshot->load_profile_count;
        valid_bit = AMI_COSEM_VALID_LOAD_PROFILE;
        captures = load_profile_capture_objects;
        capture_count = sizeof(load_profile_capture_objects) / sizeof(load_profile_capture_objects[0]);
        capture_period = 900u;
    } else if (is_billing) {
        count = snapshot->billing_history_count;
        valid_bit = AMI_COSEM_VALID_BILLING_PROFILE;
        captures = billing_capture_objects;
        capture_count = sizeof(billing_capture_objects) / sizeof(billing_capture_objects[0]);
        capture_period = 0u;
    } else {
        count = snapshot->event_log_count;
        valid_bit = AMI_COSEM_VALID_EVENT_LOG;
        captures = event_capture_objects;
        capture_count = sizeof(event_capture_objects) / sizeof(event_capture_objects[0]);
        capture_period = 0u;
    }

    const size_t capacity = AMI_COSEM_PROFILE_MAX_ENTRIES;
    size_t offset = 0u;
    memset(out, 0, sizeof(*out));

    if (attribute_id == 1u) {
        if (selection != NULL) return AMI_COSEM_SCOPE_VIOLATED;
        out->type = AMI_COSEM_TYPE_OCTET_STRING;
        out->length = AMI_COSEM_LOGICAL_NAME_LENGTH;
        memcpy(out->value.logical_name, logical_name, AMI_COSEM_LOGICAL_NAME_LENGTH);
        return AMI_COSEM_OK;
    }
    if (count > capacity) return AMI_COSEM_INVALID_ARGUMENT;
    if (selection != NULL && attribute_id != 2u) return AMI_COSEM_SCOPE_VIOLATED;

    switch (attribute_id) {
    case 2: /* buffer: ARRAY of row STRUCTURE values */
        if ((snapshot->valid_mask & valid_bit) == 0u) return AMI_COSEM_VALUE_UNAVAILABLE;
        {
        size_t selected_count = 0u;
        size_t first_entry = 0u;
        size_t last_entry = count;
        if (selection != NULL && selection->selector == AMI_COSEM_PROFILE_SELECTOR_ENTRY) {
            uint32_t to_entry = selection->to_entry == 0u
                ? (uint32_t)count : selection->to_entry;
            if (selection->from_entry == 0u || selection->from_entry > to_entry ||
                to_entry > count) return AMI_COSEM_SCOPE_VIOLATED;
            first_entry = (size_t)(selection->from_entry - 1u);
            last_entry = (size_t)to_entry;
        }
        if (!put_byte(out, &offset, 1u) || !put_byte(out, &offset, 0u))
            return AMI_COSEM_INVALID_ARGUMENT;
        int encode_failed = 0;
        for (size_t i = 0; i < count; ++i) {
            if (is_event && specific_tamper_code >= 0 &&
                snapshot->event_log[i].event_code != (uint8_t)specific_tamper_code) {
                continue;
            }
            if (selection != NULL && selection->selector == AMI_COSEM_PROFILE_SELECTOR_ENTRY &&
                (i < first_entry || i >= last_entry)) continue;
            const AmiCosemDateTime *timestamp = is_load ? &snapshot->load_profile[i].timestamp :
                                               (is_billing ? &snapshot->billing_history[i].billing_timestamp :
                                                             &snapshot->event_log[i].timestamp);
            if (selection != NULL && selection->selector == AMI_COSEM_PROFILE_SELECTOR_RANGE &&
                (compare_profile_datetime(timestamp, &selection->from_date) < 0 ||
                 compare_profile_datetime(timestamp, &selection->to_date) > 0)) continue;

            if (is_load) {
                if (!put_byte(out, &offset, AMI_COSEM_TYPE_STRUCTURE) ||
                    !put_byte(out, &offset, (uint8_t)AMI_COSEM_LOAD_PROFILE_CHANNELS) ||
                    !put_datetime(out, &offset, timestamp) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].status) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_LONG64_UNSIGNED) ||
                    !put_u64(out, &offset, snapshot->load_profile[i].active_import_wh) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_LONG64_UNSIGNED) ||
                    !put_u64(out, &offset, snapshot->load_profile[i].active_export_wh) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_LONG64_UNSIGNED) ||
                    !put_u64(out, &offset, snapshot->load_profile[i].reactive_import_varh) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_LONG64_UNSIGNED) ||
                    !put_u64(out, &offset, snapshot->load_profile[i].reactive_export_varh) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_LONG64_UNSIGNED) ||
                    !put_u64(out, &offset, snapshot->load_profile[i].reactive_q1_varh) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_LONG64_UNSIGNED) ||
                    !put_u64(out, &offset, snapshot->load_profile[i].reactive_q2_varh) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_LONG64_UNSIGNED) ||
                    !put_u64(out, &offset, snapshot->load_profile[i].reactive_q3_varh) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_LONG64_UNSIGNED) ||
                    !put_u64(out, &offset, snapshot->load_profile[i].reactive_q4_varh) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].phase_voltage_mv[0]) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].phase_voltage_mv[1]) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].phase_voltage_mv[2]) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].phase_current_ma[0]) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].phase_current_ma[1]) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].phase_current_ma[2]) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].neutral_current_ma) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].active_power_watts) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].reactive_power_vars) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG) ||
                    !put_u32(out, &offset, (uint32_t)snapshot->load_profile[i].power_factor) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].frequency_mhz) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].thd_voltage_percent_x10) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->load_profile[i].thd_current_percent_x10)) {
                    encode_failed = 1;
                    break;
                }
            } else if (is_billing) {
                if (!put_byte(out, &offset, AMI_COSEM_TYPE_STRUCTURE) ||
                    !put_byte(out, &offset, 5u) ||
                    !put_datetime(out, &offset, timestamp) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_LONG64_UNSIGNED) ||
                    !put_u64(out, &offset, snapshot->billing_history[i].active_import_wh) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_LONG64_UNSIGNED) ||
                    !put_u64(out, &offset, snapshot->billing_history[i].active_export_wh) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_LONG64_UNSIGNED) ||
                    !put_u64(out, &offset, snapshot->billing_history[i].reactive_import_varh) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_LONG64_UNSIGNED) ||
                    !put_u64(out, &offset, snapshot->billing_history[i].reactive_export_varh)) {
                    encode_failed = 1;
                    break;
                }
            } else {
                if (!put_byte(out, &offset, AMI_COSEM_TYPE_STRUCTURE) ||
                    !put_byte(out, &offset, (uint8_t)AMI_COSEM_TAMPER_LOG_COLUMNS) ||
                    !put_datetime(out, &offset, timestamp) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_UNSIGNED) ||
                    !put_byte(out, &offset, snapshot->event_log[i].event_code) ||
                    !put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                    !put_u32(out, &offset, snapshot->event_log[i].status)) {
                    encode_failed = 1;
                    break;
                }
            }
            ++selected_count;
        }
        if (encode_failed || offset == 0u || selected_count > UINT8_MAX)
            return AMI_COSEM_INVALID_ARGUMENT;
        out->value.encoded[1] = (uint8_t)selected_count;
        goto encoded;
        }
    case 3:
        if (!put_capture_array(out, &offset, captures, capture_count)) break;
        goto encoded;
    case 4: /* capture_period: 900 for load profile, 0 for trigger-based profiles */
        if (!put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
            !put_u32(out, &offset, capture_period)) break;
        goto encoded;
    case 5: /* FIFO */
        if (!put_byte(out, &offset, 22u) || !put_byte(out, &offset, 1u)) break;
        goto encoded;
    case 6: /* sort_object: timestamp capture object */
        if (!put_capture_object(out, &offset, &captures[0])) break;
        goto encoded;
    case 7:
        if ((snapshot->valid_mask & valid_bit) == 0u) return AMI_COSEM_VALUE_UNAVAILABLE;
        {
            uint32_t active_entries = (uint32_t)count;
            if (is_event && specific_tamper_code >= 0) {
                active_entries = 0u;
                for (size_t k = 0; k < count; ++k) {
                    if (snapshot->event_log[k].event_code == (uint8_t)specific_tamper_code) {
                        active_entries++;
                    }
                }
            }
            if (!put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
                !put_u32(out, &offset, active_entries)) break;
        }
        goto encoded;
    case 8:
        if (!put_byte(out, &offset, AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) ||
            !put_u32(out, &offset, (uint32_t)capacity)) break;
        goto encoded;
    default:
        return AMI_COSEM_ATTRIBUTE_NOT_FOUND;
    }
    return AMI_COSEM_INVALID_ARGUMENT;

encoded:
    out->type = AMI_COSEM_TYPE_ENCODED;
    out->length = offset;
    return AMI_COSEM_OK;
}

size_t ami_cosem_register_count(void)
{
    return (sizeof(registers) / sizeof(registers[0])) +
           (sizeof(extended_registers) / sizeof(extended_registers[0]));
}

size_t ami_cosem_object_count(void)
{
    return ami_cosem_register_count() + 5u;
}

static const ExtendedRegisterDefinition *find_extended_register(
    const uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH])
{
    for (size_t i = 0; i < sizeof(extended_registers) / sizeof(extended_registers[0]); ++i) {
        if (memcmp(extended_registers[i].logical_name, logical_name,
                   AMI_COSEM_LOGICAL_NAME_LENGTH) == 0) return &extended_registers[i];
    }
    return NULL;
}

static const AmiCosemRegisterSample *find_register_sample(
    const AmiCosemRegisterSnapshot *snapshot,
    const uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH])
{
    if (snapshot->measurement_sample_count > AMI_COSEM_REGISTER_SAMPLE_MAX) return NULL;
    for (size_t i = 0; i < snapshot->measurement_sample_count; ++i) {
        if (memcmp(snapshot->measurement_samples[i].logical_name, logical_name,
                   AMI_COSEM_LOGICAL_NAME_LENGTH) == 0) return &snapshot->measurement_samples[i];
    }
    return NULL;
}

static AmiCosemStatus read_extended_register(
    const AmiCosemRegisterSnapshot *snapshot,
    const ExtendedRegisterDefinition *definition,
    uint8_t attribute_id, AmiCosemData *out)
{
    memset(out, 0, sizeof(*out));
    if (attribute_id == 1u) {
        out->type = AMI_COSEM_TYPE_OCTET_STRING;
        out->length = AMI_COSEM_LOGICAL_NAME_LENGTH;
        memcpy(out->value.logical_name, definition->logical_name, out->length);
        return AMI_COSEM_OK;
    }
    if (attribute_id == 3u) {
        out->type = AMI_COSEM_TYPE_STRUCTURE;
        out->length = 2u;
        out->value.scaler_unit.scaler = definition->scaler;
        out->value.scaler_unit.unit = definition->unit;
        return AMI_COSEM_OK;
    }
    if (attribute_id != 2u) return AMI_COSEM_ATTRIBUTE_NOT_FOUND;
    if (snapshot->measurement_sample_count > AMI_COSEM_REGISTER_SAMPLE_MAX)
        return AMI_COSEM_INVALID_ARGUMENT;
    const AmiCosemRegisterSample *sample = find_register_sample(snapshot, definition->logical_name);
    if (sample == NULL || !sample->valid) return AMI_COSEM_VALUE_UNAVAILABLE;
    out->type = definition->value_type;
    if (definition->value_type == AMI_COSEM_TYPE_LONG64_UNSIGNED) {
        out->length = sizeof(uint64_t);
        out->value.long64_unsigned = sample->raw_value;
    } else if (definition->value_type == AMI_COSEM_TYPE_DOUBLE_LONG) {
        out->length = sizeof(int32_t);
        out->value.double_long = sample->signed_value;
    } else {
        if (sample->raw_value > UINT32_MAX) return AMI_COSEM_INVALID_ARGUMENT;
        out->length = sizeof(uint32_t);
        out->value.double_long_unsigned = (uint32_t)sample->raw_value;
    }
    return AMI_COSEM_OK;
}

static const RegisterDefinition *find_register(
    const uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH])
{
    for (size_t i = 0; i < sizeof(registers) / sizeof(registers[0]); ++i) {
        if (memcmp(registers[i].logical_name, logical_name,
                   AMI_COSEM_LOGICAL_NAME_LENGTH) == 0) {
            return &registers[i];
        }
    }
    return NULL;
}

static uint64_t read_source(const AmiCosemRegisterSnapshot *snapshot,
                            const RegisterDefinition *definition)
{
    switch (definition->source) {
    case VALUE_VOLTAGE:
        return snapshot->phase_voltage_millivolts[definition->phase];
    case VALUE_CURRENT:
        return snapshot->phase_current_milliamps[definition->phase];
    case VALUE_ACTIVE_IMPORT:
        return snapshot->active_import_wh;
    case VALUE_ACTIVE_EXPORT:
        return snapshot->active_export_wh;
    case VALUE_REACTIVE_IMPORT:
        return snapshot->reactive_import_varh;
    case VALUE_REACTIVE_EXPORT:
        return snapshot->reactive_export_varh;
    case VALUE_ACTIVE_IMPORT_TARIFF:
        return definition->phase < 4u ? snapshot->active_import_tariff_wh[definition->phase] : 0u;
    case VALUE_ACTIVE_EXPORT_TARIFF:
        return definition->phase < 4u ? snapshot->active_export_tariff_wh[definition->phase] : 0u;
    case VALUE_REACTIVE_IMPORT_TARIFF:
        return definition->phase < 4u ? snapshot->reactive_import_tariff_varh[definition->phase] : 0u;
    case VALUE_FREQUENCY:
        return snapshot->frequency_mhz;
    case VALUE_POWER_FACTOR:
        return definition->phase < 4u ? (uint64_t)(int64_t)snapshot->power_factor[definition->phase] : 0u;
    case VALUE_THD_VOLTAGE:
        return definition->phase < 4u ? snapshot->thd_voltage_percent_x10[definition->phase] : 0u;
    case VALUE_THD_CURRENT:
        return definition->phase < 4u ? snapshot->thd_current_percent_x10[definition->phase] : 0u;
    case VALUE_ACTIVE_POWER_TOTAL:
        return snapshot->active_power_watts;
    case VALUE_REACTIVE_POWER_TOTAL:
        return snapshot->reactive_power_vars;
    }
    return 0;
}

AmiCosemStatus ami_cosem_register_read_selected(
    const AmiCosemRegisterSnapshot *snapshot,
    uint16_t class_id,
    const uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH],
    uint8_t attribute_id,
    const AmiCosemProfileSelection *selection,
    AmiCosemData *out)
{
    if (snapshot == NULL || logical_name == NULL || out == NULL) {
        return AMI_COSEM_INVALID_ARGUMENT;
    }
    if (class_id != AMI_COSEM_REGISTER_CLASS_ID &&
        class_id != AMI_COSEM_PROFILE_GENERIC_CLASS_ID &&
        class_id != AMI_COSEM_CLOCK_CLASS_ID &&
        class_id != AMI_COSEM_SECURITY_SETUP_CLASS_ID &&
        class_id != AMI_COSEM_LIMITER_CLASS_ID &&
        class_id != AMI_COSEM_DISCONNECT_CONTROL_CLASS_ID &&
        class_id != 1u /* Data */) {
        return AMI_COSEM_OBJECT_NOT_FOUND;
    }

    if (selection != NULL && class_id != AMI_COSEM_PROFILE_GENERIC_CLASS_ID) {
        return AMI_COSEM_SCOPE_VIOLATED;
    }

    if (class_id == 1u) {
        static const uint8_t ldn_ln[AMI_COSEM_LOGICAL_NAME_LENGTH] = {0, 0, 42, 0, 0, 255};
        static const uint8_t fw_ln[AMI_COSEM_LOGICAL_NAME_LENGTH] = {1, 0, 0, 2, 0, 255};
        static const uint8_t meter_id_ln[AMI_COSEM_LOGICAL_NAME_LENGTH] = {0, 0, 96, 1, 0, 255};
        static const uint8_t meter_type_ln[AMI_COSEM_LOGICAL_NAME_LENGTH] = {0, 0, 96, 1, 1, 255};
        memset(out, 0, sizeof(*out));
        if (memcmp(logical_name, ldn_ln, sizeof(ldn_ln)) == 0) {
            if (attribute_id == 1u) {
                out->type = AMI_COSEM_TYPE_OCTET_STRING;
                out->length = AMI_COSEM_LOGICAL_NAME_LENGTH;
                memcpy(out->value.logical_name, ldn_ln, AMI_COSEM_LOGICAL_NAME_LENGTH);
                return AMI_COSEM_OK;
            }
            if (attribute_id == 2u) {
                /* Return Logical Device Name (e.g., STM32U585-METER-01) */
                static const char device_name[] = "STM32U585-AMI-01";
                out->type = AMI_COSEM_TYPE_VISIBLE_STRING;
                out->length = sizeof(device_name) - 1u;
                memcpy(out->value.octets, device_name, out->length);
                return AMI_COSEM_OK;
            }
            return AMI_COSEM_ATTRIBUTE_NOT_FOUND;
        }
        if (memcmp(logical_name, fw_ln, sizeof(fw_ln)) == 0) {
            if (attribute_id == 1u) {
                out->type = AMI_COSEM_TYPE_OCTET_STRING;
                out->length = AMI_COSEM_LOGICAL_NAME_LENGTH;
                memcpy(out->value.logical_name, fw_ln, AMI_COSEM_LOGICAL_NAME_LENGTH);
                return AMI_COSEM_OK;
            }
            if (attribute_id == 2u) {
                static const char fw_version[] = "v1.0.0-sprint1";
                out->type = AMI_COSEM_TYPE_VISIBLE_STRING;
                out->length = sizeof(fw_version) - 1u;
                memcpy(out->value.octets, fw_version, out->length);
                return AMI_COSEM_OK;
            }
            return AMI_COSEM_ATTRIBUTE_NOT_FOUND;
        }
        if (memcmp(logical_name, meter_id_ln, sizeof(meter_id_ln)) == 0) {
            if (attribute_id == 1u) {
                out->type = AMI_COSEM_TYPE_OCTET_STRING;
                out->length = AMI_COSEM_LOGICAL_NAME_LENGTH;
                memcpy(out->value.logical_name, meter_id_ln, AMI_COSEM_LOGICAL_NAME_LENGTH);
                return AMI_COSEM_OK;
            }
            if (attribute_id == 2u) {
                const char *id_str = (snapshot->meter_id[0] != '\0') ? snapshot->meter_id : "PLN1234567890";
                size_t id_len = strlen(id_str);
                if (id_len > sizeof(out->value.octets)) id_len = sizeof(out->value.octets);
                out->type = AMI_COSEM_TYPE_VISIBLE_STRING;
                out->length = id_len;
                memcpy(out->value.octets, id_str, id_len);
                return AMI_COSEM_OK;
            }
            return AMI_COSEM_ATTRIBUTE_NOT_FOUND;
        }
        if (memcmp(logical_name, meter_type_ln, sizeof(meter_type_ln)) == 0) {
            if (attribute_id == 1u) {
                out->type = AMI_COSEM_TYPE_OCTET_STRING;
                out->length = AMI_COSEM_LOGICAL_NAME_LENGTH;
                memcpy(out->value.logical_name, meter_type_ln, AMI_COSEM_LOGICAL_NAME_LENGTH);
                return AMI_COSEM_OK;
            }
            if (attribute_id == 2u) {
                const char *type_str = (snapshot->meter_type[0] != '\0') ? snapshot->meter_type : "3-PHASE-AMI-POSTPAID";
                size_t type_len = strlen(type_str);
                if (type_len > sizeof(out->value.octets)) type_len = sizeof(out->value.octets);
                out->type = AMI_COSEM_TYPE_VISIBLE_STRING;
                out->length = type_len;
                memcpy(out->value.octets, type_str, type_len);
                return AMI_COSEM_OK;
            }
            return AMI_COSEM_ATTRIBUTE_NOT_FOUND;
        }
        return AMI_COSEM_OBJECT_NOT_FOUND;
    }

    if (class_id == AMI_COSEM_LIMITER_CLASS_ID) {
        static const uint8_t limiter_ln[AMI_COSEM_LOGICAL_NAME_LENGTH] = {0, 0, 17, 0, 3, 255};
        if (memcmp(logical_name, limiter_ln, AMI_COSEM_LOGICAL_NAME_LENGTH) != 0) {
            return AMI_COSEM_OBJECT_NOT_FOUND;
        }
        memset(out, 0, sizeof(*out));
        switch (attribute_id) {
        case 1:
            out->type = AMI_COSEM_TYPE_OCTET_STRING;
            out->length = AMI_COSEM_LOGICAL_NAME_LENGTH;
            memcpy(out->value.logical_name, limiter_ln, AMI_COSEM_LOGICAL_NAME_LENGTH);
            return AMI_COSEM_OK;
        case 2: {
            /* Monitored Value: Structure (class_id=3, LN=1.0.1.7.0.255 Active Power, attr=2) */
            static const uint8_t monitored_val[] = {
                0x02, 0x03,                         /* Structure, 3 elements */
                0x12, 0x00, 0x03,                   /* LongUnsigned: class 3 */
                0x09, 0x06, 1, 0, 1, 7, 0, 255,     /* OctetString 6B: 1.0.1.7.0.255 */
                0x0F, 0x02                          /* Integer: attribute 2 (value) */
            };
            out->type = AMI_COSEM_TYPE_ENCODED;
            out->length = sizeof(monitored_val);
            memcpy(out->value.encoded, monitored_val, sizeof(monitored_val));
            return AMI_COSEM_OK;
        }
        case 3:
            out->type = AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED;
            out->length = sizeof(uint32_t);
            out->value.double_long_unsigned = snapshot->limiter_threshold_normal_watts;
            return AMI_COSEM_OK;
        case 4:
            out->type = AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED;
            out->length = sizeof(uint32_t);
            out->value.double_long_unsigned = snapshot->limiter_threshold_active_watts;
            return AMI_COSEM_OK;
        case 5:
            out->type = AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED;
            out->length = sizeof(uint32_t);
            out->value.double_long_unsigned = snapshot->limiter_min_over_threshold_duration_sec;
            return AMI_COSEM_OK;
        case 6:
            out->type = AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED;
            out->length = sizeof(uint32_t);
            out->value.double_long_unsigned = snapshot->limiter_min_under_threshold_duration_sec;
            return AMI_COSEM_OK;
        case 7: {
            /* Actions: Structure of two script actions (Emergency Script, Normal Script) */
            static const uint8_t actions_data[] = {
                0x02, 0x02,                         /* Structure, 2 elements */
                0x02, 0x02, 0x09, 0x06, 0, 0, 10, 0, 105, 255, 0x12, 0x00, 0x01, /* Action 1: Script Table 0.0.10.0.105.255, ID 1 */
                0x02, 0x02, 0x09, 0x06, 0, 0, 10, 0, 105, 255, 0x12, 0x00, 0x02  /* Action 2: Script Table 0.0.10.0.105.255, ID 2 */
            };
            out->type = AMI_COSEM_TYPE_ENCODED;
            out->length = sizeof(actions_data);
            memcpy(out->value.encoded, actions_data, sizeof(actions_data));
            return AMI_COSEM_OK;
        }
        default:
            return AMI_COSEM_ATTRIBUTE_NOT_FOUND;
        }
    }

    if (class_id == AMI_COSEM_DISCONNECT_CONTROL_CLASS_ID) {
        static const uint8_t disconnect_ln[AMI_COSEM_LOGICAL_NAME_LENGTH] = {0, 0, 96, 3, 10, 255};
        if (memcmp(logical_name, disconnect_ln, AMI_COSEM_LOGICAL_NAME_LENGTH) != 0) {
            return AMI_COSEM_OBJECT_NOT_FOUND;
        }
        memset(out, 0, sizeof(*out));
        switch (attribute_id) {
        case 1:
            out->type = AMI_COSEM_TYPE_OCTET_STRING;
            out->length = AMI_COSEM_LOGICAL_NAME_LENGTH;
            memcpy(out->value.logical_name, disconnect_ln, AMI_COSEM_LOGICAL_NAME_LENGTH);
            return AMI_COSEM_OK;
        case 2:
            /* output_state: boolean */
            out->type = AMI_COSEM_TYPE_UNSIGNED;
            out->length = 1u;
            out->value.unsigned_integer = snapshot->disconnect_output_state ? 1u : 0u;
            return AMI_COSEM_OK;
        case 3:
            /* control_state: enum (0 = disconnected, 1 = connected, 2 = ready_for_reconnection) */
            out->type = AMI_COSEM_TYPE_ENUM;
            out->length = 1u;
            out->value.unsigned_integer = snapshot->disconnect_control_state;
            return AMI_COSEM_OK;
        case 4:
            /* control_mode: unsigned integer */
            out->type = AMI_COSEM_TYPE_UNSIGNED;
            out->length = 1u;
            out->value.unsigned_integer = snapshot->disconnect_control_mode;
            return AMI_COSEM_OK;
        default:
            return AMI_COSEM_ATTRIBUTE_NOT_FOUND;
        }
    }

    if (class_id == AMI_COSEM_PROFILE_GENERIC_CLASS_ID) {
        return read_profile(snapshot, logical_name, attribute_id, selection, out);
    }

    if (class_id == AMI_COSEM_SECURITY_SETUP_CLASS_ID) {
        if (memcmp(logical_name, security_setup_logical_name,
                   AMI_COSEM_LOGICAL_NAME_LENGTH) != 0) return AMI_COSEM_OBJECT_NOT_FOUND;
        memset(out, 0, sizeof(*out));
        switch (attribute_id) {
        case 1:
            out->type = AMI_COSEM_TYPE_OCTET_STRING;
            out->length = AMI_COSEM_LOGICAL_NAME_LENGTH;
            memcpy(out->value.logical_name, security_setup_logical_name,
                   AMI_COSEM_LOGICAL_NAME_LENGTH);
            return AMI_COSEM_OK;
        case 2:
            out->type = AMI_COSEM_TYPE_ENUM;
            out->length = 1u;
            out->value.unsigned_integer = snapshot->security_policy;
            return AMI_COSEM_OK;
        case 3:
            out->type = AMI_COSEM_TYPE_ENUM;
            out->length = 1u;
            out->value.unsigned_integer = snapshot->security_suite;
            return AMI_COSEM_OK;
        case 4:
            if ((snapshot->valid_mask & AMI_COSEM_VALID_SECURITY_CLIENT_TITLE) == 0u)
                return AMI_COSEM_VALUE_UNAVAILABLE;
            out->type = AMI_COSEM_TYPE_OCTET_STRING;
            out->length = sizeof(snapshot->client_system_title);
            memcpy(out->value.octets, snapshot->client_system_title, out->length);
            return AMI_COSEM_OK;
        case 5:
            out->type = AMI_COSEM_TYPE_OCTET_STRING;
            out->length = sizeof(snapshot->server_system_title);
            memcpy(out->value.octets, snapshot->server_system_title, out->length);
            return AMI_COSEM_OK;
        case 6:
            /* No certificate store is configured in this firmware slice. */
            out->type = AMI_COSEM_TYPE_ENCODED;
            out->length = 2u;
            out->value.encoded[0] = 1u;
            out->value.encoded[1] = 0u;
            return AMI_COSEM_OK;
        default:
            return AMI_COSEM_ATTRIBUTE_NOT_FOUND;
        }
    }

    if (class_id == AMI_COSEM_CLOCK_CLASS_ID) {
        if (memcmp(logical_name, clock_logical_name,
                   AMI_COSEM_LOGICAL_NAME_LENGTH) != 0) {
            return AMI_COSEM_OBJECT_NOT_FOUND;
        }

        memset(out, 0, sizeof(*out));
        if (attribute_id == 1u) {
            out->type = AMI_COSEM_TYPE_OCTET_STRING;
            out->length = AMI_COSEM_LOGICAL_NAME_LENGTH;
            memcpy(out->value.logical_name, clock_logical_name,
                   AMI_COSEM_LOGICAL_NAME_LENGTH);
            return AMI_COSEM_OK;
        }
        if ((snapshot->valid_mask & AMI_COSEM_VALID_CLOCK) == 0u) {
            return AMI_COSEM_VALUE_UNAVAILABLE;
        }
        switch (attribute_id) {
        case 2:
            out->type = AMI_COSEM_TYPE_OCTET_STRING;
            out->length = sizeof(AmiCosemDateTime);
            out->value.date_time = snapshot->clock_date_time;
            return AMI_COSEM_OK;
        case 3:
            out->type = AMI_COSEM_TYPE_DOUBLE_LONG;
            out->length = sizeof(int32_t);
            out->value.double_long = snapshot->clock_time_zone_minutes;
            return AMI_COSEM_OK;
        case 4:
            out->type = AMI_COSEM_TYPE_UNSIGNED;
            out->length = sizeof(snapshot->clock_status);
            out->value.unsigned_integer = snapshot->clock_status;
            return AMI_COSEM_OK;
        default:
            return AMI_COSEM_ATTRIBUTE_NOT_FOUND;
        }
    }

    const RegisterDefinition *definition = find_register(logical_name);
    if (definition == NULL) {
        const ExtendedRegisterDefinition *extended = find_extended_register(logical_name);
        return extended == NULL ? AMI_COSEM_OBJECT_NOT_FOUND
                                : read_extended_register(snapshot, extended, attribute_id, out);
    }

    memset(out, 0, sizeof(*out));
    switch (attribute_id) {
    case 1:
        out->type = AMI_COSEM_TYPE_OCTET_STRING;
        out->length = AMI_COSEM_LOGICAL_NAME_LENGTH;
        memcpy(out->value.logical_name, definition->logical_name,
               AMI_COSEM_LOGICAL_NAME_LENGTH);
        return AMI_COSEM_OK;
    case 2: {
        if ((snapshot->valid_mask & definition->valid_mask) == 0u) {
            return AMI_COSEM_VALUE_UNAVAILABLE;
        }
        uint64_t value = read_source(snapshot, definition);
        out->type = definition->value_type;
        out->length = definition->value_type == AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED
            ? sizeof(uint32_t) : sizeof(uint64_t);
        if (definition->value_type == AMI_COSEM_TYPE_DOUBLE_LONG_UNSIGNED) {
            if (value > UINT32_MAX) return AMI_COSEM_INVALID_ARGUMENT;
            out->value.double_long_unsigned = (uint32_t)value;
        } else {
            out->value.long64_unsigned = value;
        }
        return AMI_COSEM_OK;
    }
    case 3:
        out->type = AMI_COSEM_TYPE_STRUCTURE;
        out->length = 2u;
        out->value.scaler_unit.scaler = definition->scaler;
        out->value.scaler_unit.unit = definition->unit;
        return AMI_COSEM_OK;
    default:
        return AMI_COSEM_ATTRIBUTE_NOT_FOUND;
    }
}

AmiCosemStatus ami_cosem_register_read(
    const AmiCosemRegisterSnapshot *snapshot,
    uint16_t class_id,
    const uint8_t logical_name[AMI_COSEM_LOGICAL_NAME_LENGTH],
    uint8_t attribute_id,
    AmiCosemData *out)
{
    return ami_cosem_register_read_selected(snapshot, class_id, logical_name,
        attribute_id, NULL, out);
}
