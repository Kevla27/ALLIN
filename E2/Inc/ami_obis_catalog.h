/**
 * @file    ami_obis_catalog.h
 * @brief   SPLN D3.002:2021 & IEC 62056-6-2 (DLMS/COSEM) Comprehensive OBIS Catalog
 * @details Standardized OBIS Logical Name definitions for Smart Meter PLN.
 *          Each macro expands to the 6 octets: {A, B, C, D, E, F} as unsigned integers.
 *
 *          Total OBIS objects: 276 defined objects (SPLN D3.002:2021 Table Deteksi Objek).
 *          Compatible with three-phase & single-phase smart meter profiles.
 */

#ifndef AMI_OBIS_CATALOG_H
#define AMI_OBIS_CATALOG_H

#define AMI_OBIS_LENGTH                     6u

/* ========================================================================= */
/* 1. METER IDENTIFICATION & DEVICE INFORMATION */
/* ========================================================================= */
#define AMI_OBIS_METER_TYPE                          {0u, 0u, 96u, 1u, 1u, 255u}    /* No 1: Tipe meter | Data (class_id = 1, version: 0) | 0.0.96.1.1.255 */
#define AMI_OBIS_METER_ID                            {0u, 0u, 96u, 1u, 0u, 255u}    /* No 2: Meter ID | Data (class_id = 1, version: 0) | 0.0.96.1.0.255 */
#define AMI_OBIS_HARDWARE_VERSION                    {0u, 0u, 96u, 96u, 0u, 255u}   /* No 3: Versi hardware | Data (class_id = 1, version: 0) | 0.0.96.96.0.255 */
#define AMI_OBIS_SOFTWARE_VERSION                    {1u, 0u, 0u, 2u, 0u, 255u}     /* No 4: Versi software | Data (class_id = 1, version: 0) | 1.0.0.2.0.255 */
#define AMI_OBIS_CUSTOMER_ID                         {0u, 0u, 96u, 1u, 2u, 255u}    /* No 5: Pelanggan ID | Data (class_id = 1, version: 0) | 0.0.96.1.2.255 */
#define AMI_OBIS_PLN_UNIT_ID                         {0u, 0u, 96u, 1u, 3u, 255u}    /* No 6: Unit PLN ID | Data (class_id = 1, version: 0) | 0.0.96.1.3.255 */
#define AMI_OBIS_EXPORT_IMPORT_MODE                  {1u, 0u, 96u, 99u, 0u, 255u}   /* No 222: Mode ekspor-impor | Data (class_id = 1, version: 1) | 1.0.96.99.0.255 */
#define AMI_OBIS_LOGICAL_DEVICE_NAME                 {0u, 0u, 42u, 0u, 0u, 255u}    /* No 264: COSEM Logical Device Name | Data (class_id = 1, version: 0) | 0.0.42.0.0.255 */

/* ========================================================================= */
/* 2. METER CONSTANTS & AUXILIARY POWER (BATTERY / SUPERCAP) */
/* ========================================================================= */
#define AMI_OBIS_METER_CONSTANT_ACTIVE               {1u, 0u, 0u, 3u, 0u, 255u}     /* No 7: Meter konstanta (aktif) | Register (class_id = 3, version: 0) | 1.0.0.3.0.255 */
#define AMI_OBIS_METER_CONSTANT_REACTIVE             {1u, 0u, 0u, 3u, 1u, 255u}     /* No 8: Meter konstanta (reaktif) | Register (class_id = 3, version: 0) | 1.0.0.3.1.255 */
#define AMI_OBIS_BATTERY_CAPACITY                    {0u, 0u, 96u, 6u, 3u, 255u}    /* No 9: Kapasitas baterai | Register (class_id = 3, version: 0) | 0.0.96.6.3.255 */
#define AMI_OBIS_SUPERCAP_CAPACITY                   {0u, 0u, 96u, 6u, 11u, 255u}   /* No 10: Kapasitas superkapasitor | Register (class_id = 3, version: 0) | 0.0.96.6.11.255 */

/* ========================================================================= */
/* 3. MEASUREMENT ALGORITHMS & GENERAL CONFIGURATION */
/* ========================================================================= */
#define AMI_OBIS_ACTIVE_ENERGY_ALGORITHM             {1u, 0u, 0u, 11u, 2u, 255u}    /* No 15: Algoritma pengukuran untuk energi aktif | Data (class_id = 1, version: 0) | 1.0.0.11.2.255 */

/* ========================================================================= */
/* 4. SCRIPT TABLES & CONTROL I/O */
/* ========================================================================= */
#define AMI_OBIS_SCRIPT_METER_RESET                  {0u, 0u, 10u, 0u, 0u, 255u}    /* No 12: Reset meter | Script Table (class_id = 9, version: 0) | 0.0.10.0.0.255 */
#define AMI_OBIS_SCRIPT_CLEAR_ALL_TAMPER             {0u, 0u, 10u, 1u, 0u, 255u}    /* No 13: Clear all tamper | Script Table (class_id = 9, version: 0) | 0.0.10.1.0.255 */
#define AMI_OBIS_SCRIPT_TRIGGER_TEST                 {0u, 0u, 10u, 0u, 101u, 255u}  /* No 14: Trigger test | Script Table (class_id = 9, version: 0) | 0.0.10.0.101.255 */
#define AMI_OBIS_CONTROL_IO_STATE                    {0u, 0u, 96u, 3u, 0u, 255u}    /* No 16: Control I/O digital state | Data (class_id = 1, version: 0) | 0.0.96.3.0.255 */
#define AMI_OBIS_SCRIPT_CONTROL_IO                   {0u, 0u, 10u, 0u, 135u, 255u}  /* No 17: Control I/O digital script | Script Table (class_id = 9, version: 0) | 0.0.10.0.135.255 */
#define AMI_OBIS_SCRIPT_POWER_LIMITER                {0u, 0u, 10u, 2u, 105u, 255u}  /* No 177: Limiter Script Table | Script Table (Class id = 9, version: 0) | 0.0.10.2.105.255 */
#define AMI_OBIS_SCRIPT_TARIFFICATION                {0u, 0u, 10u, 0u, 100u, 255u}  /* No 227: Tariffication script table | Script Table (class_id = 9, version: 0) | 0.0.10.0.100.255 */
#define AMI_OBIS_SCRIPT_PUSH                         {0u, 0u, 10u, 0u, 108u, 255u}  /* No 233: Push script table | Script Table (class_id = 9, version: 0) | 0.0.10.0.108.255 */
#define AMI_OBIS_SCRIPT_ALARM_PUSH_1                 {0u, 0u, 10u, 0u, 130u, 255u}  /* No 241: Alarm push 1 on/off script | Script Table (class_id = 9, version: 0) | 0.0.10.0.130.255 */
#define AMI_OBIS_SCRIPT_ALARM_PUSH_2                 {0u, 0u, 10u, 0u, 131u, 255u}  /* No 242: Alarm push 2 on/off script | Script Table (class_id = 9, version: 0) | 0.0.10.0.131.255 */
#define AMI_OBIS_SCRIPT_IMAGE_ACTIVATION             {0u, 0u, 10u, 0u, 107u, 255u}  /* No 251: Image Activation Script | Script Table (class_id = 9, version: 0) | 0.0.10.0.107.255 */

/* ========================================================================= */
/* 5. ELECTRICAL INSTANTANEOUS MEASUREMENTS (CURRENT, VOLTAGE, FREQUENCY) */
/* ========================================================================= */
#define AMI_OBIS_FREQUENCY                           {1u, 0u, 14u, 7u, 0u, 255u}    /* No 11: Frekuensi | Register (class_id = 3, version: 0) | 1.0.14.7.0.255 */
#define AMI_OBIS_CURRENT_R                           {1u, 0u, 31u, 7u, 0u, 255u}    /* No 107: Arus fase R | Register (class_id = 3, version: 0) | 1.0.31.7.0.255 */
#define AMI_OBIS_CURRENT_S                           {1u, 0u, 51u, 7u, 0u, 255u}    /* No 108: Arus fase S | Register (class_id = 3, version: 0) | 1.0.51.7.0.255 */
#define AMI_OBIS_CURRENT_T                           {1u, 0u, 71u, 7u, 0u, 255u}    /* No 109: Arus fase T | Register (class_id = 3, version: 0) | 1.0.71.7.0.255 */
#define AMI_OBIS_CURRENT_NEUTRAL                     {1u, 0u, 91u, 7u, 0u, 255u}    /* No 110: Arus Netral | Register (class_id = 3, version: 0) | 1.0.91.7.0.255 */
#define AMI_OBIS_VOLTAGE_R                           {1u, 0u, 32u, 7u, 0u, 255u}    /* No 111: Tegangan fase R | Register (class_id = 3, version: 0) | 1.0.32.7.0.255 */
#define AMI_OBIS_VOLTAGE_S                           {1u, 0u, 52u, 7u, 0u, 255u}    /* No 112: Tegangan fase S | Register (class_id = 3, version: 0) | 1.0.52.7.0.255 */
#define AMI_OBIS_VOLTAGE_T                           {1u, 0u, 72u, 7u, 0u, 255u}    /* No 113: Tegangan fase T | Register (class_id = 3, version: 0) | 1.0.72.7.0.255 */
#define AMI_OBIS_VOLTAGE_NEUTRAL                     {1u, 0u, 92u, 7u, 0u, 255u}    /* No 114: Tegangan Netral | Register (class_id = 3, version: 0) | 1.0.92.7.0.255 */

/* ========================================================================= */
/* 6. POWER FACTOR & PHASE ANGLES */
/* ========================================================================= */
#define AMI_OBIS_POWER_FACTOR_R                      {1u, 0u, 33u, 7u, 0u, 255u}    /* No 115: Faktor daya fase R | Register (class_id = 3, version: 0) | 1.0.33.7.0.255 */
#define AMI_OBIS_POWER_FACTOR_S                      {1u, 0u, 53u, 7u, 0u, 255u}    /* No 116: Faktor daya fase S | Register (class_id = 3, version: 0) | 1.0.53.7.0.255 */
#define AMI_OBIS_POWER_FACTOR_T                      {1u, 0u, 73u, 7u, 0u, 255u}    /* No 117: Faktor daya fase T | Register (class_id = 3, version: 0) | 1.0.73.7.0.255 */
#define AMI_OBIS_POWER_FACTOR_TOTAL                  {1u, 0u, 13u, 7u, 0u, 255u}    /* No 118: Faktor daya | Register (class_id = 3, version: 0) | 1.0.13.7.0.255 */
#define AMI_OBIS_ANGLE_VOLTAGE_R                     {1u, 0u, 81u, 7u, 0u, 255u}    /* No 119: Sudut tegangan fase R (terhadap tegangan fase R) | Register (class_id = 3, version: 0) | 1.0.81.7.0.255 */
#define AMI_OBIS_ANGLE_VOLTAGE_S                     {1u, 0u, 81u, 7u, 1u, 255u}    /* No 120: Sudut tegangan fase S (terhadap tegangan fase R) | Register (class_id = 3, version: 0) | 1.0.81.7.1.255 */
#define AMI_OBIS_ANGLE_VOLTAGE_T                     {1u, 0u, 81u, 7u, 2u, 255u}    /* No 121: Sudut tegangan fase T (terhadap tegangan fase R) | Register (class_id = 3, version: 0) | 1.0.81.7.2.255 */
#define AMI_OBIS_ANGLE_CURRENT_R                     {1u, 0u, 81u, 7u, 4u, 255u}    /* No 122: Sudut arus fase R (terhadap tegangan fase R) | Register (class_id = 3, version: 0) | 1.0.81.7.4.255 */
#define AMI_OBIS_ANGLE_CURRENT_S                     {1u, 0u, 81u, 7u, 15u, 255u}   /* No 123: Sudut arus fase S (terhadap tegangan fase S) | Register (class_id = 3, version: 0) | 1.0.81.7.15.255 */
#define AMI_OBIS_ANGLE_CURRENT_T                     {1u, 0u, 81u, 7u, 26u, 255u}   /* No 124: Sudut arus fase T (terhadap tegangan fase T) | Register (class_id = 3, version: 0) | 1.0.81.7.26.255 */
#define AMI_OBIS_ANGLE_CURRENT_NEUTRAL               {1u, 0u, 81u, 7u, 7u, 255u}    /* No 125: Sudut arus fase N (terhadap tegangan fase R) | Register (class_id = 3, version: 0) | 1.0.81.7.7.255 */

/* ========================================================================= */
/* 7. INSTANTANEOUS POWER REGISTERS (ACTIVE, REACTIVE, APPARENT) */
/* ========================================================================= */
#define AMI_OBIS_ACTIVE_POWER_EXPORT_R               {1u, 0u, 21u, 7u, 0u, 255u}    /* No 83: Daya aktif kirim fase R | Register (class_id = 3, version: 0) | 1.0.21.7.0.255 */
#define AMI_OBIS_ACTIVE_POWER_EXPORT_S               {1u, 0u, 41u, 7u, 0u, 255u}    /* No 84: Daya aktif kirim fase S | Register (class_id = 3, version: 0) | 1.0.41.7.0.255 */
#define AMI_OBIS_ACTIVE_POWER_EXPORT_T               {1u, 0u, 61u, 7u, 0u, 255u}    /* No 85: Daya aktif kirim fase T | Register (class_id = 3, version: 0) | 1.0.61.7.0.255 */
#define AMI_OBIS_ACTIVE_POWER_IMPORT_R               {1u, 0u, 22u, 7u, 0u, 255u}    /* No 86: Daya aktif terima fase R | Register (class_id = 3, version: 0) | 1.0.22.7.0.255 */
#define AMI_OBIS_ACTIVE_POWER_IMPORT_S               {1u, 0u, 42u, 7u, 0u, 255u}    /* No 87: Daya aktif terima fase S | Register (class_id = 3, version: 0) | 1.0.42.7.0.255 */
#define AMI_OBIS_ACTIVE_POWER_IMPORT_T               {1u, 0u, 62u, 7u, 0u, 255u}    /* No 88: Daya aktif terima fase T | Register (class_id = 3, version: 0) | 1.0.62.7.0.255 */
#define AMI_OBIS_ACTIVE_POWER_EXPORT_TOTAL           {1u, 0u, 1u, 7u, 0u, 255u}     /* No 89: Daya aktif kirim | Register (class_id = 3, version: 0) | 1.0.1.7.0.255 */
#define AMI_OBIS_ACTIVE_POWER_IMPORT_TOTAL           {1u, 0u, 2u, 7u, 0u, 255u}     /* No 90: Daya aktif terima | Register (class_id = 3, version: 0) | 1.0.2.7.0.255 */
#define AMI_OBIS_REACTIVE_POWER_EXPORT_R             {1u, 0u, 23u, 7u, 0u, 255u}    /* No 91: Daya reaktif kirim fase R | Register (class_id = 3, version: 0) | 1.0.23.7.0.255 */
#define AMI_OBIS_REACTIVE_POWER_EXPORT_S             {1u, 0u, 43u, 7u, 0u, 255u}    /* No 92: Daya reaktif kirim fase S | Register (class_id = 3, version: 0) | 1.0.43.7.0.255 */
#define AMI_OBIS_REACTIVE_POWER_EXPORT_T             {1u, 0u, 63u, 7u, 0u, 255u}    /* No 93: Daya reaktif kirim fase T | Register (class_id = 3, version: 0) | 1.0.63.7.0.255 */
#define AMI_OBIS_REACTIVE_POWER_IMPORT_R             {1u, 0u, 24u, 7u, 0u, 255u}    /* No 94: Daya reaktif terima fase R | Register (class_id = 3, version: 0) | 1.0.24.7.0.255 */
#define AMI_OBIS_REACTIVE_POWER_IMPORT_S             {1u, 0u, 44u, 7u, 0u, 255u}    /* No 95: Daya reaktif terima fase S | Register (class_id = 3, version: 0) | 1.0.44.7.0.255 */
#define AMI_OBIS_REACTIVE_POWER_IMPORT_T             {1u, 0u, 64u, 7u, 0u, 255u}    /* No 96: Daya reaktif terima fase T | Register (class_id = 3, version: 0) | 1.0.64.7.0.255 */
#define AMI_OBIS_REACTIVE_POWER_EXPORT_TOTAL         {1u, 0u, 3u, 7u, 0u, 255u}     /* No 97: Daya reaktif kirim | Register (class_id = 3, version: 0) | 1.0.3.7.0.255 */
#define AMI_OBIS_REACTIVE_POWER_IMPORT_TOTAL         {1u, 0u, 4u, 7u, 0u, 255u}     /* No 98: Daya reaktif terima | Register (class_id = 3, version: 0) | 1.0.4.7.0.255 */
#define AMI_OBIS_APPARENT_POWER_EXPORT_R             {1u, 0u, 29u, 7u, 0u, 255u}    /* No 99: Daya semu kirim fase R | Register (class_id = 3, version: 0) | 1.0.29.7.0.255 */
#define AMI_OBIS_APPARENT_POWER_EXPORT_S             {1u, 0u, 49u, 7u, 0u, 255u}    /* No 100: Daya semu kirim fase S | Register (class_id = 3, version: 0) | 1.0.49.7.0.255 */
#define AMI_OBIS_APPARENT_POWER_EXPORT_T             {1u, 0u, 69u, 7u, 0u, 255u}    /* No 101: Daya semu kirim fase T | Register (class_id = 3, version: 0) | 1.0.69.7.0.255 */
#define AMI_OBIS_APPARENT_POWER_IMPORT_R             {1u, 0u, 30u, 7u, 0u, 255u}    /* No 102: Daya semu terima fase R | Register (class_id = 3, version: 0) | 1.0.30.7.0.255 */
#define AMI_OBIS_APPARENT_POWER_IMPORT_S             {1u, 0u, 50u, 7u, 0u, 255u}    /* No 103: Daya semu terima fase S | Register (class_id = 3, version: 0) | 1.0.50.7.0.255 */
#define AMI_OBIS_APPARENT_POWER_IMPORT_T             {1u, 0u, 70u, 7u, 0u, 255u}    /* No 104: Daya semu terima fase T | Register (class_id = 3, version: 0) | 1.0.70.7.0.255 */
#define AMI_OBIS_APPARENT_POWER_EXPORT_TOTAL         {1u, 0u, 9u, 7u, 0u, 255u}     /* No 105: Daya semu kirim | Register (class_id = 3, version: 0) | 1.0.9.7.0.255 */
#define AMI_OBIS_APPARENT_POWER_IMPORT_TOTAL         {1u, 0u, 10u, 7u, 0u, 255u}    /* No 106: Daya semu terima | Register (class_id = 3, version: 0) | 1.0.10.7.0.255 */

/* ========================================================================= */
/* 8. CUMULATIVE ENERGY REGISTERS (ACTIVE, REACTIVE, APPARENT, QUADRANTS) */
/* ========================================================================= */
#define AMI_OBIS_ACTIVE_ENERGY_EXPORT_R              {1u, 0u, 21u, 8u, 0u, 255u}    /* No 18: Energi aktif fase R kirim | Register (class_id = 3, version: 0) | 1.0.21.8.0.255 */
#define AMI_OBIS_ACTIVE_ENERGY_EXPORT_S              {1u, 0u, 41u, 8u, 0u, 255u}    /* No 19: Energi aktif fase S kirim | Register (class_id = 3, version: 0) | 1.0.41.8.0.255 */
#define AMI_OBIS_ACTIVE_ENERGY_EXPORT_T              {1u, 0u, 61u, 8u, 0u, 255u}    /* No 20: Energi aktif fase T kirim | Register (class_id = 3, version: 0) | 1.0.61.8.0.255 */
#define AMI_OBIS_ACTIVE_ENERGY_EXPORT_TOTAL          {1u, 0u, 1u, 8u, 0u, 255u}     /* No 21: Energi aktif kirim | Register (class_id = 3, version: 0) | 1.0.1.8.0.255 */
#define AMI_OBIS_ACTIVE_ENERGY_IMPORT_R              {1u, 0u, 22u, 8u, 0u, 255u}    /* No 22: Energi aktif fase R terima | Register (class_id = 3, version: 0) | 1.0.22.8.0.255 */
#define AMI_OBIS_ACTIVE_ENERGY_IMPORT_S              {1u, 0u, 42u, 8u, 0u, 255u}    /* No 23: Energi aktif fase S terima | Register (class_id = 3, version: 0) | 1.0.42.8.0.255 */
#define AMI_OBIS_ACTIVE_ENERGY_IMPORT_T              {1u, 0u, 62u, 8u, 0u, 255u}    /* No 24: Energi aktif fase T terima | Register (class_id = 3, version: 0) | 1.0.62.8.0.255 */
#define AMI_OBIS_ACTIVE_ENERGY_IMPORT_TOTAL          {1u, 0u, 2u, 8u, 0u, 255u}     /* No 25: Energi aktif terima | Register (class_id = 3, version: 0) | 1.0.2.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_EXPORT_R            {1u, 0u, 23u, 8u, 0u, 255u}    /* No 26: Energi reaktif fase R kirim | Register (class_id = 3, version: 0) | 1.0.23.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_EXPORT_S            {1u, 0u, 43u, 8u, 0u, 255u}    /* No 27: Energi reaktif fase S kirim | Register (class_id = 3, version: 0) | 1.0.43.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_EXPORT_T            {1u, 0u, 63u, 8u, 0u, 255u}    /* No 28: Energi reaktif fase T kirim | Register (class_id = 3, version: 0) | 1.0.63.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_EXPORT_TOTAL        {1u, 0u, 3u, 8u, 0u, 255u}     /* No 29: Energi reaktif kirim | Register (class_id = 3, version: 0) | 1.0.3.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_IMPORT_R            {1u, 0u, 24u, 8u, 0u, 255u}    /* No 30: Energi reaktif fase R terima | Register (class_id = 3, version: 0) | 1.0.24.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_IMPORT_S            {1u, 0u, 44u, 8u, 0u, 255u}    /* No 31: Energi reaktif fase S terima | Register (class_id = 3, version: 0) | 1.0.44.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_IMPORT_T            {1u, 0u, 64u, 8u, 0u, 255u}    /* No 32: Energi reaktif fase T terima | Register (class_id = 3, version: 0) | 1.0.64.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_IMPORT_TOTAL        {1u, 0u, 4u, 8u, 0u, 255u}     /* No 33: Energi reaktif terima | Register (class_id = 3, version: 0) | 1.0.4.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_BILLING             {1u, 0u, 128u, 8u, 0u, 255u}   /* No 34: Energi reaktif billing (kompensasi) | Register (class_id = 3, version: 0) | 1.0.128.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_Q1_R                {1u, 0u, 25u, 8u, 0u, 255u}    /* No 35: Energi reaktif Q1 fase R | Register (class_id = 3, version: 0) | 1.0.25.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_Q1_S                {1u, 0u, 45u, 8u, 0u, 255u}    /* No 36: Energi reaktif Q1 fase S | Register (class_id = 3, version: 0) | 1.0.45.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_Q1_T                {1u, 0u, 65u, 8u, 0u, 255u}    /* No 37: Energi reaktif Q1 fase T | Register (class_id = 3, version: 0) | 1.0.65.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_Q2_R                {1u, 0u, 26u, 8u, 0u, 255u}    /* No 38: Energi reaktif Q2 fase R | Register (class_id = 3, version: 0) | 1.0.26.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_Q2_S                {1u, 0u, 46u, 8u, 0u, 255u}    /* No 39: Energi reaktif Q2 fase S | Register (class_id = 3, version: 0) | 1.0.46.8.0.255 [Note: Table col C=45 corrected to 46 per DLMS Blue Book & code string] */
#define AMI_OBIS_REACTIVE_ENERGY_Q2_T                {1u, 0u, 66u, 8u, 0u, 255u}    /* No 40: Energi reaktif Q2 fase T | Register (class_id = 3, version: 0) | 1.0.66.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_Q3_R                {1u, 0u, 27u, 8u, 0u, 255u}    /* No 41: Energi reaktif Q3 fase R | Register (class_id = 3, version: 0) | 1.0.27.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_Q3_S                {1u, 0u, 47u, 8u, 0u, 255u}    /* No 42: Energi reaktif Q3 fase S | Register (class_id = 3, version: 0) | 1.0.47.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_Q3_T                {1u, 0u, 67u, 8u, 0u, 255u}    /* No 43: Energi reaktif Q3 fase T | Register (class_id = 3, version: 0) | 1.0.67.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_Q4_R                {1u, 0u, 28u, 8u, 0u, 255u}    /* No 44: Energi reaktif Q4 fase R | Register (class_id = 3, version: 0) | 1.0.28.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_Q4_S                {1u, 0u, 48u, 8u, 0u, 255u}    /* No 45: Energi reaktif Q4 fase S | Register (class_id = 3, version: 0) | 1.0.48.8.0.255 */
#define AMI_OBIS_REACTIVE_ENERGY_Q4_T                {1u, 0u, 68u, 8u, 0u, 255u}    /* No 46: Energi reaktif Q4 fase T | Register (class_id = 3, version: 0) | 1.0.68.8.0.255 */
#define AMI_OBIS_APPARENT_ENERGY_EXPORT_R            {1u, 0u, 29u, 8u, 0u, 255u}    /* No 47: Energi semu fase R kirim | Register (class_id = 3, version: 0) | 1.0.29.8.0.255 */
#define AMI_OBIS_APPARENT_ENERGY_EXPORT_S            {1u, 0u, 49u, 8u, 0u, 255u}    /* No 48: Energi semu fase S kirim | Register (class_id = 3, version: 0) | 1.0.49.8.0.255 */
#define AMI_OBIS_APPARENT_ENERGY_EXPORT_T            {1u, 0u, 69u, 8u, 0u, 255u}    /* No 49: Energi semu fase T kirim | Register (class_id = 3, version: 0) | 1.0.69.8.0.255 */
#define AMI_OBIS_APPARENT_ENERGY_IMPORT_R            {1u, 0u, 30u, 8u, 0u, 255u}    /* No 50: Energi semu fase R terima | Register (class_id = 3, version: 0) | 1.0.30.8.0.255 */
#define AMI_OBIS_APPARENT_ENERGY_IMPORT_S            {1u, 0u, 50u, 8u, 0u, 255u}    /* No 51: Energi semu fase S terima | Register (class_id = 3, version: 0) | 1.0.50.8.0.255 */
#define AMI_OBIS_APPARENT_ENERGY_IMPORT_T            {1u, 0u, 70u, 8u, 0u, 255u}    /* No 52: Energi semu fase T terima | Register (class_id = 3, version: 0) | 1.0.70.8.0.255 */
#define AMI_OBIS_ACTIVE_ENERGY_TOTAL                 {1u, 0u, 15u, 8u, 0u, 255u}    /* No 279: Energi aktif total | Register (class_id = 3, version: 0) | 1.0.15.8.0.255 */

/* ========================================================================= */
/* 9. TARIFF-BASED ENERGY REGISTERS (TARIFFS 1 TO 5) */
/* ========================================================================= */
#define AMI_OBIS_ACTIVE_ENERGY_EXPORT_TARIFF_1       {1u, 0u, 1u, 8u, 1u, 255u}     /* No 53: Energi aktif kirim Tarif 1 | Register (class_id = 3, version: 0) | 1.0.1.8.1.255 */
#define AMI_OBIS_ACTIVE_ENERGY_EXPORT_TARIFF_2       {1u, 0u, 1u, 8u, 2u, 255u}     /* No 54: Energi aktif kirim Tarif 2 | Register (class_id = 3, version: 0) | 1.0.1.8.2.255 */
#define AMI_OBIS_ACTIVE_ENERGY_EXPORT_TARIFF_3       {1u, 0u, 1u, 8u, 3u, 255u}     /* No 55: Energi aktif kirim Tarif 3 | Register (class_id = 3, version: 0) | 1.0.1.8.3.255 */
#define AMI_OBIS_ACTIVE_ENERGY_EXPORT_TARIFF_4       {1u, 0u, 1u, 8u, 4u, 255u}     /* No 56: Energi aktif kirim Tarif 4 | Register (class_id = 3, version: 0) | 1.0.1.8.4.255 */
#define AMI_OBIS_ACTIVE_ENERGY_EXPORT_TARIFF_5       {1u, 0u, 1u, 8u, 5u, 255u}     /* No 57: Energi aktif kirim Tarif 5 | Register (class_id = 3, version: 0) | 1.0.1.8.5.255 */
#define AMI_OBIS_ACTIVE_ENERGY_IMPORT_TARIFF_1       {1u, 0u, 2u, 8u, 1u, 255u}     /* No 58: Energi aktif terima Tarif 1 | Register (class_id = 3, version: 0) | 1.0.2.8.1.255 */
#define AMI_OBIS_ACTIVE_ENERGY_IMPORT_TARIFF_2       {1u, 0u, 2u, 8u, 2u, 255u}     /* No 59: Energi aktif terima Tarif 2 | Register (class_id = 3, version: 0) | 1.0.2.8.2.255 */
#define AMI_OBIS_ACTIVE_ENERGY_IMPORT_TARIFF_3       {1u, 0u, 2u, 8u, 3u, 255u}     /* No 60: Energi aktif terima Tarif 3 | Register (class_id = 3, version: 0) | 1.0.2.8.3.255 */
#define AMI_OBIS_ACTIVE_ENERGY_IMPORT_TARIFF_4       {1u, 0u, 2u, 8u, 4u, 255u}     /* No 61: Energi aktif terima Tarif 4 | Register (class_id = 3, version: 0) | 1.0.2.8.4.255 */
#define AMI_OBIS_ACTIVE_ENERGY_IMPORT_TARIFF_5       {1u, 0u, 2u, 8u, 5u, 255u}     /* No 62: Energi aktif terima Tarif 5 | Register (class_id = 3, version: 0) | 1.0.2.8.5.255 */
#define AMI_OBIS_REACTIVE_ENERGY_EXPORT_TARIFF_1     {1u, 0u, 3u, 8u, 1u, 255u}     /* No 63: Energi reaktif kirim Tarif 1 | Register (class_id = 3, version: 0) | 1.0.3.8.1.255 */
#define AMI_OBIS_REACTIVE_ENERGY_EXPORT_TARIFF_2     {1u, 0u, 3u, 8u, 2u, 255u}     /* No 64: Energi reaktif kirim Tarif 2 | Register (class_id = 3, version: 0) | 1.0.3.8.2.255 */
#define AMI_OBIS_REACTIVE_ENERGY_EXPORT_TARIFF_3     {1u, 0u, 3u, 8u, 3u, 255u}     /* No 65: Energi reaktif kirim Tarif 3 | Register (class_id = 3, version: 0) | 1.0.3.8.3.255 */
#define AMI_OBIS_REACTIVE_ENERGY_EXPORT_TARIFF_4     {1u, 0u, 3u, 8u, 4u, 255u}     /* No 66: Energi reaktif kirim Tarif 4 | Register (class_id = 3, version: 0) | 1.0.3.8.4.255 */
#define AMI_OBIS_REACTIVE_ENERGY_EXPORT_TARIFF_5     {1u, 0u, 3u, 8u, 5u, 255u}     /* No 67: Energi reaktif kirim Tarif 5 | Register (class_id = 3, version: 0) | 1.0.3.8.5.255 */
#define AMI_OBIS_REACTIVE_ENERGY_IMPORT_TARIFF_1     {1u, 0u, 4u, 8u, 1u, 255u}     /* No 68: Energi reaktif terima Tarif 1 | Register (class_id = 3, version: 0) | 1.0.4.8.1.255 */
#define AMI_OBIS_REACTIVE_ENERGY_IMPORT_TARIFF_2     {1u, 0u, 4u, 8u, 2u, 255u}     /* No 69: Energi reaktif terima Tarif 2 | Register (class_id = 3, version: 0) | 1.0.4.8.2.255 */
#define AMI_OBIS_REACTIVE_ENERGY_IMPORT_TARIFF_3     {1u, 0u, 4u, 8u, 3u, 255u}     /* No 70: Energi reaktif terima Tarif 3 | Register (class_id = 3, version: 0) | 1.0.4.8.3.255 */
#define AMI_OBIS_REACTIVE_ENERGY_IMPORT_TARIFF_4     {1u, 0u, 4u, 8u, 4u, 255u}     /* No 71: Energi reaktif terima Tarif 4 | Register (class_id = 3, version: 0) | 1.0.4.8.4.255 */
#define AMI_OBIS_REACTIVE_ENERGY_IMPORT_TARIFF_5     {1u, 0u, 4u, 8u, 5u, 255u}     /* No 72: Energi reaktif terima Tarif 5 | Register (class_id = 3, version: 0) | 1.0.4.8.5.255 */
#define AMI_OBIS_APPARENT_ENERGY_EXPORT_TARIFF_1     {1u, 0u, 9u, 8u, 1u, 255u}     /* No 73: Energi semu kirim Tarif 1 | Register (class_id = 3, version: 0) | 1.0.9.8.1.255 */
#define AMI_OBIS_APPARENT_ENERGY_EXPORT_TARIFF_2     {1u, 0u, 9u, 8u, 2u, 255u}     /* No 74: Energi semu kirim Tarif 2 | Register (class_id = 3, version: 0) | 1.0.9.8.2.255 */
#define AMI_OBIS_APPARENT_ENERGY_EXPORT_TARIFF_3     {1u, 0u, 9u, 8u, 3u, 255u}     /* No 75: Energi semu kirim Tarif 3 | Register (class_id = 3, version: 0) | 1.0.9.8.3.255 */
#define AMI_OBIS_APPARENT_ENERGY_EXPORT_TARIFF_4     {1u, 0u, 9u, 8u, 4u, 255u}     /* No 76: Energi semu kirim Tarif 4 | Register (class_id = 3, version: 0) | 1.0.9.8.4.255 */
#define AMI_OBIS_APPARENT_ENERGY_EXPORT_TARIFF_5     {1u, 0u, 9u, 8u, 5u, 255u}     /* No 77: Energi semu kirim Tarif 5 | Register (class_id = 3, version: 0) | 1.0.9.8.5.255 */
#define AMI_OBIS_APPARENT_ENERGY_IMPORT_TARIFF_1     {1u, 0u, 10u, 8u, 1u, 255u}    /* No 78: Energi semu terima Tarif 1 | Register (class_id = 3, version: 0) | 1.0.10.8.1.255 */
#define AMI_OBIS_APPARENT_ENERGY_IMPORT_TARIFF_2     {1u, 0u, 10u, 8u, 2u, 255u}    /* No 79: Energi semu terima Tarif 2 | Register (class_id = 3, version: 0) | 1.0.10.8.2.255 */
#define AMI_OBIS_APPARENT_ENERGY_IMPORT_TARIFF_3     {1u, 0u, 10u, 8u, 3u, 255u}    /* No 80: Energi semu terima Tarif 3 | Register (class_id = 3, version: 0) | 1.0.10.8.3.255 */
#define AMI_OBIS_APPARENT_ENERGY_IMPORT_TARIFF_4     {1u, 0u, 10u, 8u, 4u, 255u}    /* No 81: Energi semu terima Tarif 4 | Register (class_id = 3, version: 0) | 1.0.10.8.4.255 */
#define AMI_OBIS_APPARENT_ENERGY_IMPORT_TARIFF_5     {1u, 0u, 10u, 8u, 5u, 255u}    /* No 82: Energi semu terima Tarif 5 | Register (class_id = 3, version: 0) | 1.0.10.8.5.255 */

/* ========================================================================= */
/* 10. DEMAND & MAXIMUM DEMAND REGISTERS (TOTAL & PER TARIFF) */
/* ========================================================================= */
#define AMI_OBIS_DEMAND_ACTIVE_POWER_EXPORT          {1u, 0u, 1u, 4u, 0u, 255u}     /* No 138: Demand daya aktif kirim | Demand Register (class_id = 5, version: 0) | 1.0.1.4.0.255 */
#define AMI_OBIS_DEMAND_ACTIVE_POWER_IMPORT          {1u, 0u, 2u, 4u, 0u, 255u}     /* No 139: Demand daya aktif terima | Demand Register (class_id = 5, version: 0) | 1.0.2.4.0.255 */
#define AMI_OBIS_DEMAND_REACTIVE_POWER_EXPORT        {1u, 0u, 3u, 4u, 0u, 255u}     /* No 140: Demand daya reaktif kirim | Demand Register (class_id = 5, version: 0) | 1.0.3.4.0.255 */
#define AMI_OBIS_DEMAND_REACTIVE_POWER_IMPORT        {1u, 0u, 4u, 4u, 0u, 255u}     /* No 141: Demand daya reaktif terima | Demand Register (class_id = 5, version: 0) | 1.0.4.4.0.255 */
#define AMI_OBIS_DEMAND_APPARENT_POWER_EXPORT        {1u, 0u, 9u, 4u, 0u, 255u}     /* No 142: Demand daya semu kirim | Demand Register (class_id = 5, version: 0) | 1.0.9.4.0.255 */
#define AMI_OBIS_DEMAND_APPARENT_POWER_IMPORT        {1u, 0u, 10u, 4u, 0u, 255u}    /* No 143: Demand daya semu terima | Demand Register (class_id = 5, version: 0) | 1.0.10.4.0.255 */
#define AMI_OBIS_MAX_DEMAND_ACTIVE_POWER_EXPORT      {1u, 0u, 1u, 6u, 0u, 255u}     /* No 144: Maksimum demand daya aktif kirim | Extended Register (class_id = 4, version: 0) | 1.0.1.6.0.255 */
#define AMI_OBIS_MAX_DEMAND_ACTIVE_POWER_IMPORT      {1u, 0u, 2u, 6u, 0u, 255u}     /* No 145: Maksimum demand daya aktif terima | Extended Register (class_id = 4, version: 0) | 1.0.2.6.0.255 */
#define AMI_OBIS_MAX_DEMAND_REACTIVE_POWER_EXPORT    {1u, 0u, 3u, 6u, 0u, 255u}     /* No 146: Maksimum demand daya reaktif kirim | Extended Register (class_id = 4, version: 0) | 1.0.3.6.0.255 */
#define AMI_OBIS_MAX_DEMAND_REACTIVE_POWER_IMPORT    {1u, 0u, 4u, 6u, 0u, 255u}     /* No 147: Maksimum demand daya reaktif terima | Extended Register (class_id = 4, version: 0) | 1.0.4.6.0.255 */
#define AMI_OBIS_MAX_DEMAND_APPARENT_POWER_EXPORT    {1u, 0u, 9u, 6u, 0u, 255u}     /* No 148: Maksimum demand daya semu kirim | Extended Register (class_id = 4, version: 0) | 1.0.9.6.0.255 */
#define AMI_OBIS_MAX_DEMAND_APPARENT_POWER_IMPORT    {1u, 0u, 10u, 6u, 0u, 255u}    /* No 149: Maksimum demand daya semu terima | Extended Register (class_id = 4, version: 0) | 1.0.10.6.0.255 */
#define AMI_OBIS_MAX_DEMAND_APPARENT_EXPORT_TARIFF_1 {1u, 0u, 9u, 6u, 1u, 255u}     /* No 150: Maksimum demand daya semu kirim Tarif 1 | Extended Register (class_id = 4, version: 0) | 1.0.9.6.1.255 */
#define AMI_OBIS_MAX_DEMAND_APPARENT_IMPORT_TARIFF_1 {1u, 0u, 10u, 6u, 1u, 255u}    /* No 151: Maksimum demand daya semu terima Tarif 1 | Extended Register (class_id = 4, version: 0) | 1.0.10.6.1.255 */
#define AMI_OBIS_MAX_DEMAND_APPARENT_EXPORT_TARIFF_2 {1u, 0u, 9u, 6u, 2u, 255u}     /* No 152: Maksimum demand daya semu kirim Tarif 2 | Extended Register (class_id = 4, version: 0) | 1.0.9.6.2.255 */
#define AMI_OBIS_MAX_DEMAND_APPARENT_IMPORT_TARIFF_2 {1u, 0u, 10u, 6u, 2u, 255u}    /* No 153: Maksimum demand daya semu terima Tarif 2 | Extended Register (class_id = 4, version: 0) | 1.0.10.6.2.255 */
#define AMI_OBIS_MAX_DEMAND_APPARENT_EXPORT_TARIFF_3 {1u, 0u, 9u, 6u, 3u, 255u}     /* No 154: Maksimum demand daya semu kirim Tarif 3 | Extended Register (class_id = 4, version: 0) | 1.0.9.6.3.255 */
#define AMI_OBIS_MAX_DEMAND_APPARENT_IMPORT_TARIFF_3 {1u, 0u, 10u, 6u, 3u, 255u}    /* No 155: Maksimum demand daya semu terima Tarif 3 | Extended Register (class_id = 4, version: 0) | 1.0.10.6.3.255 */
#define AMI_OBIS_MAX_DEMAND_APPARENT_EXPORT_TARIFF_4 {1u, 0u, 9u, 6u, 4u, 255u}     /* No 156: Maksimum demand daya semu kirim Tarif 4 | Extended Register (class_id = 4, version: 0) | 1.0.9.6.4.255 */
#define AMI_OBIS_MAX_DEMAND_APPARENT_IMPORT_TARIFF_4 {1u, 0u, 10u, 6u, 4u, 255u}    /* No 157: Maksimum demand daya semu terima Tarif 4 | Extended Register (class_id = 4, version: 0) | 1.0.10.6.4.255 */
#define AMI_OBIS_MAX_DEMAND_APPARENT_EXPORT_TARIFF_5 {1u, 0u, 9u, 6u, 5u, 255u}     /* No 158: Maksimum demand daya semu kirim Tarif 5 | Extended Register (class_id = 4, version: 0) | 1.0.9.6.5.255 */
#define AMI_OBIS_MAX_DEMAND_APPARENT_IMPORT_TARIFF_5 {1u, 0u, 10u, 6u, 5u, 255u}    /* No 159: Maksimum demand daya semu terima Tarif 5 | Extended Register (class_id = 4, version: 0) | 1.0.10.6.5.255 */

/* ========================================================================= */
/* 11. POWER QUALITY (PQ), HARMONICS (THD/TDD) & VOLTAGE PROFILES */
/* ========================================================================= */
#define AMI_OBIS_THD_VOLTAGE_R                       {1u, 0u, 32u, 7u, 124u, 255u}  /* No 126: THD tegangan fase R | Register (class_id = 3, version: 0) | 1.0.32.7.124.255 */
#define AMI_OBIS_THD_VOLTAGE_S                       {1u, 0u, 52u, 7u, 124u, 255u}  /* No 127: THD tegangan fase S | Register (class_id = 3, version: 0) | 1.0.52.7.124.255 */
#define AMI_OBIS_THD_VOLTAGE_T                       {1u, 0u, 72u, 7u, 124u, 255u}  /* No 128: THD tegangan fase T | Register (class_id = 3, version: 0) | 1.0.72.7.124.255 */
#define AMI_OBIS_THD_VOLTAGE_TOTAL                   {1u, 0u, 12u, 7u, 124u, 255u}  /* No 129: THD tegangan | Register (class_id = 3, version: 0) | 1.0.12.7.124.255 */
#define AMI_OBIS_THD_CURRENT_R                       {1u, 0u, 31u, 7u, 124u, 255u}  /* No 130: THD arus fase R | Register (class_id = 3, version: 0) | 1.0.31.7.124.255 */
#define AMI_OBIS_THD_CURRENT_S                       {1u, 0u, 51u, 7u, 124u, 255u}  /* No 131: THD arus fase S | Register (class_id = 3, version: 0) | 1.0.51.7.124.255 */
#define AMI_OBIS_THD_CURRENT_T                       {1u, 0u, 71u, 7u, 124u, 255u}  /* No 132: THD arus fase T | Register (class_id = 3, version: 0) | 1.0.71.7.124.255 */
#define AMI_OBIS_THD_CURRENT_TOTAL                   {1u, 0u, 11u, 7u, 124u, 255u}  /* No 133: THD arus | Register (class_id = 3, version: 0) | 1.0.11.7.124.255 */
#define AMI_OBIS_TDD_CURRENT_R                       {1u, 0u, 31u, 7u, 125u, 255u}  /* No 134: TDD arus fase R | Register (class_id = 3, version: 0) | 1.0.31.7.125.255 */
#define AMI_OBIS_TDD_CURRENT_S                       {1u, 0u, 51u, 7u, 125u, 255u}  /* No 135: TDD arus fase S | Register (class_id = 3, version: 0) | 1.0.51.7.125.255 */
#define AMI_OBIS_TDD_CURRENT_T                       {1u, 0u, 71u, 7u, 125u, 255u}  /* No 136: TDD arus fase T | Register (class_id = 3, version: 0) | 1.0.71.7.125.255 */
#define AMI_OBIS_TDD_CURRENT_TOTAL                   {1u, 0u, 11u, 7u, 125u, 255u}  /* No 137: TDD arus | Register (class_id = 3, version: 0) | 1.0.11.7.125.255 */
#define AMI_OBIS_PQ_LOW_VOLTAGE_LIMIT                {1u, 0u, 12u, 31u, 0u, 255u}   /* No 166: Low voltage limit | Register (class_id = 3, version: 0) | 1.0.12.31.0.255 */
#define AMI_OBIS_PQ_LOW_VOLTAGE_DELAY                {1u, 0u, 12u, 33u, 0u, 255u}   /* No 167: Low voltage validation delay | Register (class_id = 3, version: 0) | 1.0.12.33.0.255 */
#define AMI_OBIS_PQ_LOW_VOLTAGE_COUNT                {1u, 0u, 12u, 32u, 0u, 255u}   /* No 168: Jumlah kejadian low voltage | Data (class_id = 1, version: 0) | 1.0.12.32.0.255 */
#define AMI_OBIS_PQ_VOLTAGE_REGISTER_R               {1u, 135u, 32u, 0u, 0u, 255u}  /* No 169: Power quality voltage register fase R | Data (class_id = 1, version: 0) | 1.135.32.0.0.255 */
#define AMI_OBIS_PQ_VOLTAGE_REGISTER_S               {1u, 135u, 52u, 0u, 0u, 255u}  /* No 170: Power quality voltage register fase S | Data (class_id = 1, version: 0) | 1.135.52.0.0.255 */
#define AMI_OBIS_PQ_VOLTAGE_REGISTER_T               {1u, 135u, 72u, 0u, 0u, 255u}  /* No 171: Power quality voltage register fase T | Data (class_id = 1, version: 0) | 1.135.72.0.0.255 */
#define AMI_OBIS_PQ_VOLTAGE_LOG_PROFILE_R            {1u, 0u, 99u, 14u, 2u, 255u}   /* No 172: Power quality voltage log profile R | Profile Generic (class_id = 7, version: 1) | 1.0.99.14.2.255 */
#define AMI_OBIS_PQ_VOLTAGE_LOG_PROFILE_S            {1u, 0u, 99u, 14u, 3u, 255u}   /* No 173: Power quality voltage log profile S | Profile Generic (class_id = 7, version: 1) | 1.0.99.14.3.255 */
#define AMI_OBIS_PQ_VOLTAGE_LOG_PROFILE_T            {1u, 0u, 99u, 14u, 4u, 255u}   /* No 174: Power quality voltage log profile T | Profile Generic (class_id = 7, version: 1) | 1.0.99.14.4.255 */
#define AMI_OBIS_PQ_POWER_FACTOR_COMPENSATION_LIMIT  {1u, 0u, 13u, 31u, 0u, 255u}   /* No 175: Faktor daya batas kompensasi | Register (class_id = 3, version: 0) | 1.0.13.31.0.255 */

/* ========================================================================= */
/* 12. PROFILES (BILLING / HISTORICAL, LOAD PROFILE, INSTANT PROFILE) */
/* ========================================================================= */
#define AMI_OBIS_PROFILE_BILLING_HISTORICAL          {0u, 0u, 98u, 1u, 0u, 255u}    /* No 160: Data historikal | Profile Generic (class_id = 7, version: 1) | 0.0.98.1.0.255 */
#define AMI_OBIS_SCHEDULE_BILLING_HISTORICAL         {0u, 1u, 15u, 0u, 4u, 255u}    /* No 161: Data historikal scheduler | Single action schedule (class_id = 22, version: 0) | 0.1.15.0.4.255 */
#define AMI_OBIS_PROFILE_LOAD                        {1u, 0u, 99u, 1u, 0u, 255u}    /* No 162: Load profile | Profile Generic (class_id = 7, version: 1) | 1.0.99.1.0.255 */
#define AMI_OBIS_PROFILE_INSTANT                     {1u, 96u, 98u, 128u, 0u, 255u} /* No 164: Instant profile | Profile Generic (class_id = 7, version: 1) | 1.96.98.128.0.255 */
#define AMI_OBIS_LOAD_PROFILE_DESCRIPTOR             {0u, 0u, 96u, 10u, 1u, 255u}   /* No 243: Load profile descriptor | Data (class_id = 1, version: 0) | 0.0.96.10.1.255 */
#define AMI_OBIS_LOAD_PROFILE_MONITOR                {0u, 0u, 16u, 0u, 0u, 255u}    /* No 244: Load profile monitor | Register monitor (class_id = 21, version: 0) | 0.0.16.0.0.255 */

/* ========================================================================= */
/* 13. POWER LIMITER & DISCONNECT CONTROL (BREAKER) */
/* ========================================================================= */
#define AMI_OBIS_POWER_LIMITER                       {0u, 0u, 17u, 0u, 3u, 255u}    /* No 176: Power Limiter | Limiter (class_id = 71, version: 0) | 0.0.17.0.3.255 */
#define AMI_OBIS_DISCONNECT_CONTROL                  {0u, 0u, 96u, 3u, 10u, 255u}   /* No 228: Disconnect control | Disconnect Control (class_id = 70, version: 0) | 0.0.96.3.10.255 */

/* ========================================================================= */
/* 14. ALARM REGISTERS, STATUS & DESCRIPTORS / FILTERS / MONITORS */
/* ========================================================================= */
#define AMI_OBIS_ALARM_REGISTER_1                    {0u, 0u, 97u, 98u, 0u, 255u}   /* No 178: Alarm register 1 (PLN specific) | Data (class_id = 1, version: 0) | 0.0.97.98.0.255 */
#define AMI_OBIS_ALARM_REGISTER_2                    {0u, 0u, 97u, 98u, 1u, 255u}   /* No 179: Alarm register 2 | Data (class_id = 1, version: 0) | 0.0.97.98.1.255 */
#define AMI_OBIS_STATUS_REGISTER                     {0u, 0u, 96u, 10u, 4u, 255u}   /* No 180: Status register | Data (class_id = 1, version: 0) | 0.0.96.10.4.255 */
#define AMI_OBIS_ALARM_DESCRIPTOR_1                  {0u, 0u, 97u, 98u, 20u, 255u}  /* No 235: Alarm descriptor 1 | Data (class_id = 1, version: 0) | 0.0.97.98.20.255 */
#define AMI_OBIS_ALARM_DESCRIPTOR_2                  {0u, 0u, 97u, 98u, 21u, 255u}  /* No 236: Alarm descriptor 2 | Data (class_id = 1, version: 0) | 0.0.97.98.21.255 */
#define AMI_OBIS_ALARM_FILTER_1                      {0u, 0u, 97u, 98u, 10u, 255u}  /* No 237: Alarm filter 1 | Data (class_id = 1, version: 0) | 0.0.97.98.10.255 */
#define AMI_OBIS_ALARM_FILTER_2                      {0u, 0u, 97u, 98u, 11u, 255u}  /* No 238: Alarm filter 2 | Data (class_id = 1, version: 0) | 0.0.97.98.11.255 */
#define AMI_OBIS_ALARM_MONITOR_1                     {0u, 0u, 16u, 1u, 0u, 255u}    /* No 239: Alarm monitor 1 | Register monitor (class_id = 21, version: 0) | 0.0.16.1.0.255 */
#define AMI_OBIS_ALARM_MONITOR_2                     {0u, 0u, 16u, 1u, 1u, 255u}    /* No 240: Alarm monitor 2 | Register monitor (class_id = 21, version: 0) | 0.0.16.1.1.255 */

/* ========================================================================= */
/* 15. ALARM LOG PROFILES (DEVICE / HARDWARE FAULTS) */
/* ========================================================================= */
#define AMI_OBIS_ALARM_LOG_FLASH_ERROR               {1u, 97u, 99u, 98u, 1u, 255u}  /* No 181: Alarm log Flash memory rusak/error | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.1.255 */
#define AMI_OBIS_ALARM_LOG_RAM_ERROR                 {1u, 97u, 99u, 98u, 1u, 255u}  /* No 182: Alarm log RAM rusak/error | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.1.255 */
#define AMI_OBIS_ALARM_LOG_CLOCK_LOSS                {1u, 97u, 99u, 98u, 2u, 255u}  /* No 183: Alarm log Clock loss (kembali ke waktu awal chipset) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.2.255 */
#define AMI_OBIS_ALARM_LOG_LOW_BATTERY               {1u, 97u, 99u, 98u, 3u, 255u}  /* No 184: Alarm log Low battery (threshold 2,7 V) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.3.255 */
#define AMI_OBIS_ALARM_LOG_MCU_FAULT                 {1u, 97u, 99u, 98u, 4u, 255u}  /* No 185: Alarm log Mikroprosesor tidak berfungsi normal | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.4.255 */
#define AMI_OBIS_ALARM_LOG_ADC_FAULT                 {1u, 97u, 99u, 98u, 5u, 255u}  /* No 186: Alarm log Kegagalan sampling data pengukuran pada ADC | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.5.255 */
#define AMI_OBIS_ALARM_LOG_SUPERCAP_FAULT            {1u, 97u, 99u, 98u, 6u, 255u}  /* No 187: Alarm log Superkapasitor rusak/tidak terhubung/terlepas | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.6.255 */
#define AMI_OBIS_ALARM_LOG_RELAY_FAULT               {1u, 97u, 99u, 98u, 7u, 255u}  /* No 188: Alarm log Relai/shunt trip gagal membuka/menutup | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.7.255 */

/* ========================================================================= */
/* 16. EVENT LOG PROFILES (SYSTEM / OPERATIONAL EVENTS) */
/* ========================================================================= */
#define AMI_OBIS_EVENT_LOG_POWER_SUPPLY_FAIL         {1u, 97u, 99u, 98u, 8u, 255u}  /* No 189: Event log Power supply failure | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.8.255 */
#define AMI_OBIS_EVENT_LOG_POWER_SUPPLY_UP           {1u, 97u, 99u, 98u, 9u, 255u}  /* No 190: Event log Power supply up | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.9.255 */
#define AMI_OBIS_EVENT_LOG_TOU_CONFIG_CHANGE         {1u, 97u, 99u, 98u, 10u, 255u} /* No 191: Event log Perubahan setting time-of-use (ToU) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.10.255 */
#define AMI_OBIS_EVENT_LOG_NON_TOU_CONFIG_CHANGE     {1u, 97u, 99u, 98u, 11u, 255u} /* No 192: Event log Perubahan setting (selain ToU) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.11.255 */
#define AMI_OBIS_EVENT_LOG_STOP_MEASUREMENT          {1u, 97u, 99u, 98u, 12u, 255u} /* No 193: Event log Stop-measurement | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.12.255 */
#define AMI_OBIS_EVENT_LOG_IO_STATUS_CHANGE          {1u, 97u, 99u, 98u, 14u, 255u} /* No 194: Event log Perubahan status I/O (termasuk trigger test) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.14.255 */
#define AMI_OBIS_EVENT_LOG_RELAY_STATE_CHANGE        {1u, 97u, 99u, 98u, 15u, 255u} /* No 195: Event log Perubahan status relai/kontaktor internal | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.15.255 */
#define AMI_OBIS_EVENT_LOG_METER_RESET               {1u, 97u, 99u, 98u, 16u, 255u} /* No 196: Event log Meter reset | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.16.255 */
#define AMI_OBIS_EVENT_LOG_TIME_DATE_CHANGE          {1u, 97u, 99u, 98u, 17u, 255u} /* No 197: Event log Perubahan tanggal dan waktu | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.17.255 */
#define AMI_OBIS_EVENT_LOG_ASYMMETRIC_POWER          {1u, 97u, 99u, 98u, 18u, 255u} /* No 198: Event log Asymmetric power (untuk meter fase tiga) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.18.255 */
#define AMI_OBIS_EVENT_LOG_READY_REMOTE_RECONNECT    {1u, 97u, 99u, 98u, 19u, 255u} /* No 199: Event log Meter ready remote reconnect | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.19.255 */
#define AMI_OBIS_EVENT_LOG_PINHOLE_BUTTON            {1u, 97u, 99u, 98u, 37u, 255u} /* No 216: Event log Penekanan tombol pinhole (jika ada) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.37.255 */
#define AMI_OBIS_EVENT_LOG_MFG_SPECIFIC_38           {1u, 97u, 99u, 98u, 38u, 255u} /* No 217: Event log – Man. specific / Reserved for future use | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.38.255 */
#define AMI_OBIS_EVENT_LOG_MFG_SPECIFIC_63           {1u, 97u, 99u, 98u, 63u, 255u} /* No 219: Event log – Man. specific / Reserved for future use | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.63.255 */
#define AMI_OBIS_TOTAL_METER_POWER_OFF_COUNT         {0u, 1u, 96u, 7u, 0u, 255u}    /* No 220: Total kejadian meter OFF | Data (class_id = 1, version: 0) | 0.1.96.7.0.255 */

/* ========================================================================= */
/* 17. TAMPER LOG PROFILES (FRAUD & TAMPER DETECTION) */
/* ========================================================================= */
#define AMI_OBIS_TAMPER_LOG_METER_COVER_OPEN         {1u, 97u, 99u, 98u, 20u, 255u} /* No 200: Tamper log Pembukaan tutup meter | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.20.255 */
#define AMI_OBIS_TAMPER_LOG_TERMINAL_COVER_OPEN      {1u, 97u, 99u, 98u, 21u, 255u} /* No 201: Tamper log Pembukaan tutup terminal | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.21.255 */
#define AMI_OBIS_TAMPER_LOG_PHASE_SEQUENCE_REVERSE   {1u, 97u, 99u, 98u, 22u, 255u} /* No 202: Tamper log Pengawatan tidak sesuai urutan fase untuk meter fase tiga | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.22.255 */
#define AMI_OBIS_TAMPER_LOG_VOLTAGE_CURRENT_OUT_OF_PHASE {1u, 97u, 99u, 98u, 23u, 255u} /* No 203: Tamper log Pengawatan arus dan tegangan tidak sefase (untuk meter fase tiga) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.23.255 */
#define AMI_OBIS_TAMPER_LOG_NEUTRAL_MISSING          {1u, 97u, 99u, 98u, 24u, 255u} /* No 204: Tamper log Arus netral hilang akibat kawat netral diputus baik pada kabel SMP maupun IML | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.24.255 */
#define AMI_OBIS_TAMPER_LOG_PHASE_VOLTAGE_MISSING    {1u, 97u, 99u, 98u, 25u, 255u} /* No 205: Tamper log Hilang tegangan 1 atau 2 fase (untuk meter fase tiga) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.25.255 */
#define AMI_OBIS_TAMPER_LOG_MAGNETIC_INDUCTION       {1u, 97u, 99u, 98u, 26u, 255u} /* No 206: Tamper log Induksi medan magnet dari luar (segala arah) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.26.255 */
#define AMI_OBIS_TAMPER_LOG_REVERSE_POWER_3P         {1u, 97u, 99u, 98u, 27u, 255u} /* No 207: Tamper log Reverse power (untuk meter fase tiga) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.27.255 */
#define AMI_OBIS_TAMPER_LOG_Q4_LOW_PF                {1u, 97u, 99u, 98u, 28u, 255u} /* No 208: Tamper log Pengukuran pada kuadran 4 dengan threshold PF < 0,85 (untuk meter fase tiga) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.28.255 */
#define AMI_OBIS_TAMPER_LOG_POWER_LIMIT_EXCEEDED     {1u, 97u, 99u, 98u, 29u, 255u} /* No 209: Power limiter threshold exceeded | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.29.255 */
#define AMI_OBIS_TAMPER_LOG_REVERSE_POWER_1P         {1u, 97u, 99u, 98u, 31u, 255u} /* No 210: Tamper log Pengawatan Terbalik (Reverse power sisi fase) (untuk meter fase tunggal) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.31.255 */
#define AMI_OBIS_TAMPER_LOG_INSCEK                   {1u, 97u, 99u, 98u, 32u, 255u} /* No 211: Tamper log INSCEK (untuk meter fase tunggal) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.32.255 */
#define AMI_OBIS_TAMPER_LOG_GNDCEK                   {1u, 97u, 99u, 98u, 33u, 255u} /* No 212: Tamper log GNDCEK (untuk meter fase tunggal) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.33.255 */
#define AMI_OBIS_TAMPER_LOG_CURRENT_UNBALANCE_6H     {1u, 97u, 99u, 98u, 34u, 255u} /* No 213: Tamper log Arus fase < arus netral. Terjadi selisih arus > 2 A selama 6 jam secara terus menerus (untuk meter fase tunggal) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.34.255 */
#define AMI_OBIS_TAMPER_LOG_TERMINAL_OPEN_POWERED    {1u, 97u, 99u, 98u, 35u, 255u} /* No 214: Tamper log Pembukaan tutup terminal dalam keadaan bertegangan (untuk meter fase tunggal) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.35.255 */
#define AMI_OBIS_TAMPER_LOG_TERMINAL_OPEN_UNPOWERED  {1u, 97u, 99u, 98u, 36u, 255u} /* No 215: Tamper log Pembukaan tutup terminal dalam keadaan tidak bertegangan (untuk meter fase tunggal) | Profile Generic (class_id = 7, version: 1) | 1.97.99.98.36.255 */
#define AMI_OBIS_TOTAL_TAMPER_COUNT                  {0u, 1u, 96u, 15u, 0u, 255u}   /* No 221: Total kejadian tamper | Data (class_id = 1, version: 0) | 0.1.96.15.0.255 */

/* ========================================================================= */
/* 18. TIME, CLOCK & CALENDAR MANAGEMENT */
/* ========================================================================= */
#define AMI_OBIS_CLOCK                               {0u, 0u, 1u, 0u, 0u, 255u}     /* No 223: Waktu/Clock | Clock (class_id = 8, version: 0) | 0.0.1.0.0.255 */
#define AMI_OBIS_ACTIVITY_CALENDAR                   {0u, 0u, 13u, 0u, 0u, 255u}    /* No 224: Activity calendar | Activity Calendar (class_id = 20, version: 0) | 0.0.13.0.0.255 */
#define AMI_OBIS_TARIFF_REGISTER_ACTIVATION          {0u, 0u, 14u, 0u, 0u, 255u}    /* No 225: Tariff register activation | Register Activation (class_id = 6, version: 0) | 0.0.14.0.0.255 */
#define AMI_OBIS_SPECIAL_DAYS_TABLE                  {0u, 0u, 11u, 0u, 0u, 255u}    /* No 226: Special days table | Special Days Table (class_id = 11, version: 0) | 0.0.11.0.0.255 */

/* ========================================================================= */
/* 19. PUSH SETUP & IP REGISTRATION */
/* ========================================================================= */
#define AMI_OBIS_PUSH_SETUP_ALARM_1                  {0u, 1u, 25u, 9u, 0u, 255u}    /* No 229: Push setup 1 – On alarm register 1 | Push Setup (class_id = 40, version: 0) | 0.1.25.9.0.255 */
#define AMI_OBIS_PUSH_SETUP_ALARM_2                  {0u, 2u, 25u, 9u, 0u, 255u}    /* No 230: Push setup 2 – On alarm register 2 | Push Setup (class_id = 40, version: 0) | 0.2.25.9.0.255 */
#define AMI_OBIS_PUSH_SETUP_LOAD_PROFILE             {0u, 3u, 25u, 9u, 0u, 255u}    /* No 231: Push setup 3 – Interval load profile | Push Setup (class_id = 40, version: 0) | 0.3.25.9.0.255 */
#define AMI_OBIS_PUSH_SETUP_HISTORICAL               {0u, 4u, 25u, 9u, 0u, 255u}    /* No 232: Push setup 4 – Interval Data Historikal | Push Setup (class_id = 40, version: 0) | 0.4.25.9.0.255 */
#define AMI_OBIS_PUSH_IP_REGISTRATION                {0u, 0u, 25u, 128u, 3u, 255u}  /* No 234: Push IP registration parameters | Data (class_id = 1, version: 0) | 0.0.25.128.3.255 */

/* ========================================================================= */
/* 20. IMAGE TRANSFER / FIRMWARE UPGRADE MANAGEMENT */
/* ========================================================================= */
#define AMI_OBIS_IMAGE_TRANSFER_0                    {0u, 0u, 44u, 0u, 0u, 255u}    /* No 245: Image Transfer 0 (default) | Image Transfer (class_id = 18, version: 0) | 0.0.44.0.0.255 */
#define AMI_OBIS_IMAGE_TRANSFER_1                    {0u, 0u, 44u, 0u, 1u, 255u}    /* No 246: Image Transfer 1 | Image Transfer (class_id = 18, version: 0) | 0.0.44.0.1.255 */
#define AMI_OBIS_IMAGE_TRANSFER_2                    {0u, 0u, 44u, 0u, 2u, 255u}    /* No 247: Image Transfer 2 | Image Transfer (class_id = 18, version: 0) | 0.0.44.0.2.255 */
#define AMI_OBIS_SCHEDULE_IMAGE_TRANSFER_0           {0u, 0u, 15u, 0u, 2u, 255u}    /* No 248: Image 0 Transfer Activation Scheduler (default) | Single Action Schedule (class_id = 22, version: 0) | 0.0.15.0.2.255 */
#define AMI_OBIS_SCHEDULE_IMAGE_TRANSFER_1           {0u, 1u, 15u, 0u, 2u, 255u}    /* No 249: Image 1 Transfer Activation Scheduler | Single Action Schedule (class_id = 22, version: 0) | 0.1.15.0.2.255 */
#define AMI_OBIS_SCHEDULE_IMAGE_TRANSFER_2           {0u, 2u, 15u, 0u, 2u, 255u}    /* No 250: Image 2 Transfer Activation Scheduler | Single Action Schedule (class_id = 22, version: 0) | 0.2.15.0.2.255 */
#define AMI_OBIS_ACTIVE_FIRMWARE_ID_0                {1u, 0u, 0u, 2u, 0u, 255u}     /* No 252: Active Firmware Identifier 0 (default) | Data (class_id = 1, version: 0) | 1.0.0.2.0.255 */
#define AMI_OBIS_ACTIVE_FIRMWARE_SIGNATURE_0         {1u, 0u, 0u, 2u, 8u, 255u}     /* No 253: Active Firmware Signature 0 (default) | Data (class_id = 1, version: 0) | 1.0.0.2.8.255 */
#define AMI_OBIS_ACTIVE_FIRMWARE_ID_1                {1u, 1u, 0u, 2u, 0u, 255u}     /* No 254: Active Firmware Identifier 1 | Data (class_id = 1, version: 0) | 1.1.0.2.0.255 */
#define AMI_OBIS_ACTIVE_FIRMWARE_SIGNATURE_1         {1u, 1u, 0u, 2u, 8u, 255u}     /* No 255: Active Firmware Signature 1 | Data (class_id = 1, version: 0) | 1.1.0.2.8.255 */
#define AMI_OBIS_ACTIVE_FIRMWARE_ID_2                {1u, 2u, 0u, 2u, 0u, 255u}     /* No 256: Active Firmware Identifier 2 | Data (class_id = 1, version: 0) | 1.2.0.2.0.255 */
#define AMI_OBIS_ACTIVE_FIRMWARE_SIGNATURE_2         {1u, 2u, 0u, 2u, 8u, 255u}     /* No 257: Active Firmware Signature 2 | Data (class_id = 1, version: 0) | 1.2.0.2.8.255 */

/* ========================================================================= */
/* 21. COSEM ASSOCIATIONS & SECURITY SETUP */
/* ========================================================================= */
#define AMI_OBIS_CURRENT_ASSOCIATION                 {0u, 0u, 40u, 0u, 0u, 255u}    /* No 258: Current Association | Association LN (class_id = 15, version: 3) | 0.0.40.0.0.255 */
#define AMI_OBIS_ASSOCIATION_LN_2                    {0u, 0u, 40u, 0u, 2u, 255u}    /* No 259: Association LN 2 | Association LN (class_id = 15, version: 3) | 0.0.40.0.2.255 */
#define AMI_OBIS_ASSOCIATION_LN_3                    {0u, 0u, 40u, 0u, 3u, 255u}    /* No 260: Association LN 3 | Association LN (class_id = 15, version: 3) | 0.0.40.0.3.255 */
#define AMI_OBIS_ASSOCIATION_LN_4                    {0u, 0u, 40u, 0u, 4u, 255u}    /* No 261: Association LN 4 | Association LN (class_id = 15, version: 3) | 0.0.40.0.4.255 */
#define AMI_OBIS_ASSOCIATION_LN_5                    {0u, 0u, 40u, 0u, 5u, 255u}    /* No 262: Association LN 5 | Association LN (class_id = 15, version: 3) | 0.0.40.0.5.255 */
#define AMI_OBIS_ASSOCIATION_LN_6                    {0u, 0u, 40u, 0u, 6u, 255u}    /* No 263: Association LN 6 | Association LN (class_id = 15, version: 3) | 0.0.40.0.6.255 */
#define AMI_OBIS_SECURITY_SETUP_LN_2                 {0u, 0u, 43u, 0u, 2u, 255u}    /* No 265: Security setup LN 2 - Operator 1 | Security Setup (class_id = 64, version: 1) | 0.0.43.0.2.255 */
#define AMI_OBIS_SECURITY_SETUP_LN_3                 {0u, 0u, 43u, 0u, 3u, 255u}    /* No 266: Security setup LN 3 - Operator 2 | Security Setup (class_id = 64, version: 1) | 0.0.43.0.3.255 */
#define AMI_OBIS_SECURITY_SETUP_LN_4                 {0u, 0u, 43u, 0u, 4u, 255u}    /* No 267: Security setup LN 4 - Supervisor | Security Setup (class_id = 64, version: 1) | 0.0.43.0.4.255 */
#define AMI_OBIS_SECURITY_SETUP_LN_5                 {0u, 0u, 43u, 0u, 5u, 255u}    /* No 268: Security setup LN 5 - Administrator | Security Setup (class_id = 64, version: 1) | 0.0.43.0.5.255 */
#define AMI_OBIS_SECURITY_SETUP_LN_6                 {0u, 0u, 43u, 0u, 6u, 255u}    /* No 269: Security setup LN 6 - Superadmin | Security Setup (class_id = 64, version: 1) | 0.0.43.0.6.255 */
#define AMI_OBIS_RX_FRAME_COUNTER_UNICAST_ASSOC_6    {0u, 0u, 43u, 1u, 0u, 255u}    /* No 270: Receive frame counter – Unicast Key – Association 6 | Data (class_id = 1, version: 0) | 0.0.43.1.0.255 */
#define AMI_OBIS_RX_FRAME_COUNTER_UNICAST_ASSOC_5    {0u, 0u, 43u, 1u, 1u, 255u}    /* No 271: Receive frame counter – Unicast Key – Association 5 | Data (class_id = 1, version: 0) | 0.0.43.1.1.255 */
#define AMI_OBIS_RX_FRAME_COUNTER_UNICAST_ASSOC_4    {0u, 0u, 43u, 1u, 2u, 255u}    /* No 272: Receive frame counter – Unicast Key – Association 4 | Data (class_id = 1, version: 0) | 0.0.43.1.2.255 */
#define AMI_OBIS_RX_FRAME_COUNTER_UNICAST_ASSOC_3    {0u, 0u, 43u, 1u, 3u, 255u}    /* No 273: Receive frame counter – Unicast Key – Association 3 | Data (class_id = 1, version: 0) | 0.0.43.1.3.255 */
#define AMI_OBIS_RX_FRAME_COUNTER_UNICAST_ASSOC_2    {0u, 0u, 43u, 1u, 4u, 255u}    /* No 274: Receive frame counter – Unicast Key – Association 2 | Data (class_id = 1, version: 0) | 0.0.43.1.4.255 */
#define AMI_OBIS_RX_FRAME_COUNTER_BROADCAST_ASSOC_7  {0u, 0u, 43u, 1u, 5u, 255u}    /* No 275: Receive frame counter – Broadcast Key – Association 7 | Data (class_id = 1, version: 0) | 0.0.43.1.5.255 */

/* ========================================================================= */
/* 22. COMMUNICATION PORTS & HDLC SETUP */
/* ========================================================================= */
#define AMI_OBIS_OPTICAL_PORT_SETUP                  {0u, 0u, 20u, 0u, 0u, 255u}    /* No 276: Optical port setup | IEC Local Port Setup (class_id = 19, version: 1) | 0.0.20.0.0.255 */
#define AMI_OBIS_OPTICAL_HDLC_SETUP                  {0u, 1u, 22u, 0u, 0u, 255u}    /* No 277: Optical HDLC setup | IEC HDLC Setup (class_id = 23, version: 1) | 0.1.22.0.0.255 */
#define AMI_OBIS_SERIAL_HDLC_SETUP                   {0u, 2u, 22u, 0u, 0u, 255u}    /* No 278: RS232/RS485 HDLC setup | IEC HDLC Setup (class_id = 23, version: 1) | 0.2.22.0.0.255 */

#endif /* AMI_OBIS_CATALOG_H */

