#ifndef AMI_OBIS_CATALOG_H
#define AMI_OBIS_CATALOG_H

/* SPLN D3.002:2021 three-phase read-only measurement logical names.
 * Each macro expands to the six OBIS logical-name octets (A through F). */
#define AMI_OBIS_FREQUENCY                 {1u, 0u, 14u, 7u, 0u, 255u}
#define AMI_OBIS_CURRENT_NEUTRAL           {1u, 0u, 91u, 7u, 0u, 255u}
#define AMI_OBIS_VOLTAGE_NEUTRAL           {1u, 0u, 92u, 7u, 0u, 255u}
#define AMI_OBIS_POWER_FACTOR_TOTAL        {1u, 0u, 13u, 7u, 0u, 255u}
#define AMI_OBIS_POWER_FACTOR_R            {1u, 0u, 33u, 7u, 0u, 255u}
#define AMI_OBIS_POWER_FACTOR_S            {1u, 0u, 53u, 7u, 0u, 255u}
#define AMI_OBIS_POWER_FACTOR_T            {1u, 0u, 73u, 7u, 0u, 255u}

#define AMI_OBIS_ACTIVE_POWER_EXPORT_TOTAL {1u, 0u, 1u, 7u, 0u, 255u}
#define AMI_OBIS_ACTIVE_POWER_IMPORT_TOTAL {1u, 0u, 2u, 7u, 0u, 255u}
#define AMI_OBIS_REACTIVE_POWER_EXPORT_TOTAL {1u, 0u, 3u, 7u, 0u, 255u}
#define AMI_OBIS_REACTIVE_POWER_IMPORT_TOTAL {1u, 0u, 4u, 7u, 0u, 255u}
#define AMI_OBIS_APPARENT_POWER_EXPORT_TOTAL {1u, 0u, 9u, 7u, 0u, 255u}
#define AMI_OBIS_APPARENT_POWER_IMPORT_TOTAL {1u, 0u, 10u, 7u, 0u, 255u}

#define AMI_OBIS_ACTIVE_ENERGY_EXPORT_R    {1u, 0u, 21u, 8u, 0u, 255u}
#define AMI_OBIS_ACTIVE_ENERGY_EXPORT_S    {1u, 0u, 41u, 8u, 0u, 255u}
#define AMI_OBIS_ACTIVE_ENERGY_EXPORT_T    {1u, 0u, 61u, 8u, 0u, 255u}
#define AMI_OBIS_ACTIVE_ENERGY_IMPORT_R    {1u, 0u, 22u, 8u, 0u, 255u}
#define AMI_OBIS_ACTIVE_ENERGY_IMPORT_S    {1u, 0u, 42u, 8u, 0u, 255u}
#define AMI_OBIS_ACTIVE_ENERGY_IMPORT_T    {1u, 0u, 62u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_EXPORT_R  {1u, 0u, 23u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_EXPORT_S  {1u, 0u, 43u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_EXPORT_T  {1u, 0u, 63u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_IMPORT_R  {1u, 0u, 24u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_IMPORT_S  {1u, 0u, 44u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_IMPORT_T  {1u, 0u, 64u, 8u, 0u, 255u}
#define AMI_OBIS_APPARENT_ENERGY_EXPORT_R  {1u, 0u, 29u, 8u, 0u, 255u}
#define AMI_OBIS_APPARENT_ENERGY_EXPORT_S  {1u, 0u, 49u, 8u, 0u, 255u}
#define AMI_OBIS_APPARENT_ENERGY_EXPORT_T  {1u, 0u, 69u, 8u, 0u, 255u}
#define AMI_OBIS_APPARENT_ENERGY_IMPORT_R  {1u, 0u, 30u, 8u, 0u, 255u}
#define AMI_OBIS_APPARENT_ENERGY_IMPORT_S  {1u, 0u, 50u, 8u, 0u, 255u}
#define AMI_OBIS_APPARENT_ENERGY_IMPORT_T  {1u, 0u, 70u, 8u, 0u, 255u}
#define AMI_OBIS_VOLTAGE_R                 {1u, 0u, 32u, 7u, 0u, 255u}
#define AMI_OBIS_VOLTAGE_S                 {1u, 0u, 52u, 7u, 0u, 255u}
#define AMI_OBIS_VOLTAGE_T                 {1u, 0u, 72u, 7u, 0u, 255u}
#define AMI_OBIS_CURRENT_R                 {1u, 0u, 31u, 7u, 0u, 255u}
#define AMI_OBIS_CURRENT_S                 {1u, 0u, 51u, 7u, 0u, 255u}
#define AMI_OBIS_CURRENT_T                 {1u, 0u, 71u, 7u, 0u, 255u}

#define AMI_OBIS_ACTIVE_POWER_EXPORT_R     {1u, 0u, 21u, 7u, 0u, 255u}
#define AMI_OBIS_ACTIVE_POWER_EXPORT_S     {1u, 0u, 41u, 7u, 0u, 255u}
#define AMI_OBIS_ACTIVE_POWER_EXPORT_T     {1u, 0u, 61u, 7u, 0u, 255u}
#define AMI_OBIS_ACTIVE_POWER_IMPORT_R     {1u, 0u, 22u, 7u, 0u, 255u}
#define AMI_OBIS_ACTIVE_POWER_IMPORT_S     {1u, 0u, 42u, 7u, 0u, 255u}
#define AMI_OBIS_ACTIVE_POWER_IMPORT_T     {1u, 0u, 62u, 7u, 0u, 255u}
#define AMI_OBIS_REACTIVE_POWER_EXPORT_R   {1u, 0u, 23u, 7u, 0u, 255u}
#define AMI_OBIS_REACTIVE_POWER_EXPORT_S   {1u, 0u, 43u, 7u, 0u, 255u}
#define AMI_OBIS_REACTIVE_POWER_EXPORT_T   {1u, 0u, 63u, 7u, 0u, 255u}
#define AMI_OBIS_REACTIVE_POWER_IMPORT_R   {1u, 0u, 24u, 7u, 0u, 255u}
#define AMI_OBIS_REACTIVE_POWER_IMPORT_S   {1u, 0u, 44u, 7u, 0u, 255u}
#define AMI_OBIS_REACTIVE_POWER_IMPORT_T   {1u, 0u, 64u, 7u, 0u, 255u}
#define AMI_OBIS_APPARENT_POWER_EXPORT_R   {1u, 0u, 29u, 7u, 0u, 255u}
#define AMI_OBIS_APPARENT_POWER_EXPORT_S   {1u, 0u, 49u, 7u, 0u, 255u}
#define AMI_OBIS_APPARENT_POWER_EXPORT_T   {1u, 0u, 69u, 7u, 0u, 255u}
#define AMI_OBIS_APPARENT_POWER_IMPORT_R   {1u, 0u, 30u, 7u, 0u, 255u}
#define AMI_OBIS_APPARENT_POWER_IMPORT_S   {1u, 0u, 50u, 7u, 0u, 255u}
#define AMI_OBIS_APPARENT_POWER_IMPORT_T   {1u, 0u, 70u, 7u, 0u, 255u}

#define AMI_OBIS_REACTIVE_ENERGY_Q1_R      {1u, 0u, 25u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_Q1_S      {1u, 0u, 45u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_Q1_T      {1u, 0u, 65u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_Q2_R      {1u, 0u, 26u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_Q2_T      {1u, 0u, 66u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_Q3_R      {1u, 0u, 27u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_Q3_S      {1u, 0u, 47u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_Q3_T      {1u, 0u, 67u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_Q4_R      {1u, 0u, 28u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_Q4_S      {1u, 0u, 48u, 8u, 0u, 255u}
#define AMI_OBIS_REACTIVE_ENERGY_Q4_T      {1u, 0u, 68u, 8u, 0u, 255u}

#endif