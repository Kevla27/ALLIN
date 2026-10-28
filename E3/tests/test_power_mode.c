/**
 * @file test_power_mode.c
 * @brief Unit Test Manajemen Hemat Daya Ekstrem STM32U5 Stop 2 Mode
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "bsp_power_mode.h"
#include "board_profile_config.h"

static void test_power_mode_initialization(void) {
    printf("[TEST] Inisialisasi Subsistem Daya...\n");
    bsp_power_mode_init();

    assert(bsp_power_mode_get_current() == METER_POWER_MODE_RUN);
    assert(bsp_power_mode_get_last_wakeup_reason() == METER_WAKEUP_REASON_NONE);
    assert(bsp_power_mode_get_wakeup_count() == 0U);
    assert(bsp_power_mode_is_ac_failed() == false);
    assert(board_cfg_is_stop2_mode_enabled() == true);

    printf("  [PASS] Status awal RUN, AC OK, Wakeup Count 0.\n");
}

static void test_power_mode_name_strings(void) {
    printf("[TEST] Pemetaan Nama String Moda Daya...\n");
    assert(strcmp(bsp_power_mode_get_name(METER_POWER_MODE_RUN), "RUN_160MHZ_AC_MAINS") == 0);
    assert(strcmp(bsp_power_mode_get_name(METER_POWER_MODE_BATTERY_ACTIVE), "BATTERY_ACTIVE_MSIS_4MHZ") == 0);
    assert(strcmp(bsp_power_mode_get_name(METER_POWER_MODE_STOP2), "STOP2_STANDBY_ULTRA_LOW_POWER") == 0);
    assert(strcmp(bsp_power_mode_get_name((meter_power_mode_t)99), "UNKNOWN_POWER_MODE") == 0);
    printf("  [PASS] Seluruh nama mode daya valid.\n");
}

static void test_power_mode_ac_failure_toggle(void) {
    printf("[TEST] Transisi AC Fail Toggle...\n");
    bsp_power_mode_init();

    bsp_power_mode_set_ac_failed(true);
    assert(bsp_power_mode_is_ac_failed() == true);

    bsp_power_mode_set_ac_failed(false);
    assert(bsp_power_mode_is_ac_failed() == false);
    assert(bsp_power_mode_get_current() == METER_POWER_MODE_RUN);
    printf("  [PASS] Status AC Fail sinkron.\n");
}

static void test_stop2_wakeup_scenarios(void) {
    printf("[TEST] Skenario Transisi Stop 2 Mode & Wake-up Sources...\n");
    bsp_power_mode_init();
    bsp_power_mode_set_ac_failed(true);

    /* 1. Skenario A: Woken by Navigation Button Press (PA0/PB6) */
    bsp_power_mock_simulate_wakeup(METER_WAKEUP_REASON_BUTTON_PRESS);
    meter_wakeup_reason_t reason1 = bsp_power_enter_stop2();
    assert(reason1 == METER_WAKEUP_REASON_BUTTON_PRESS);
    assert(bsp_power_mode_get_last_wakeup_reason() == METER_WAKEUP_REASON_BUTTON_PRESS);
    assert(bsp_power_mode_get_current() == METER_POWER_MODE_BATTERY_ACTIVE);
    assert(bsp_power_mode_get_wakeup_count() == 1U);
    printf("  [PASS] Skenario A (Tombol): Masuk BATTERY_ACTIVE (MSIS 4MHz), Wake Count = 1.\n");

    /* 2. Skenario B: Woken by Tamper Protection (PC13, PE6, PE4) */
    bsp_power_mock_simulate_wakeup(METER_WAKEUP_REASON_TAMPER);
    meter_wakeup_reason_t reason2 = bsp_power_enter_stop2();
    assert(reason2 == METER_WAKEUP_REASON_TAMPER);
    assert(bsp_power_mode_get_last_wakeup_reason() == METER_WAKEUP_REASON_TAMPER);
    assert(bsp_power_mode_get_current() == METER_POWER_MODE_STOP2);
    assert(bsp_power_mode_get_wakeup_count() == 2U);
    printf("  [PASS] Skenario B (Tamper): Event tercatat, kembali hemat Stop 2, Wake Count = 2.\n");

    /* 3. Skenario C: Woken by RTC Periodic Wakeup Timer */
    bsp_power_mock_simulate_wakeup(METER_WAKEUP_REASON_RTC_PERIODIC);
    meter_wakeup_reason_t reason3 = bsp_power_enter_stop2();
    assert(reason3 == METER_WAKEUP_REASON_RTC_PERIODIC);
    assert(bsp_power_mode_get_last_wakeup_reason() == METER_WAKEUP_REASON_RTC_PERIODIC);
    assert(bsp_power_mode_get_current() == METER_POWER_MODE_STOP2);
    assert(bsp_power_mode_get_wakeup_count() == 3U);
    printf("  [PASS] Skenario C (RTC Timer): Evaluasi periodik, Wake Count = 3.\n");

    /* 4. Skenario D: Woken by AC Restored (Mains PLN Up - PC3) */
    bsp_power_mock_simulate_wakeup(METER_WAKEUP_REASON_AC_RESTORED);
    meter_wakeup_reason_t reason4 = bsp_power_enter_stop2();
    assert(reason4 == METER_WAKEUP_REASON_AC_RESTORED);
    assert(bsp_power_mode_get_last_wakeup_reason() == METER_WAKEUP_REASON_AC_RESTORED);
    assert(bsp_power_mode_get_current() == METER_POWER_MODE_RUN);
    assert(bsp_power_mode_is_ac_failed() == false);
    assert(bsp_power_mode_get_wakeup_count() == 4U);
    printf("  [PASS] Skenario D (AC Restored): Pulihkan PLL 160MHz & Mode RUN, Wake Count = 4.\n");
}

static void test_clock_mode_switches(void) {
    printf("[TEST] Pengujian Transisi Restorasi Jam Clock...\n");
    bsp_power_mode_init();

    /* Uji transisi ke baterai */
    bsp_power_set_battery_clock_msis();
    assert(bsp_power_mode_get_current() == METER_POWER_MODE_BATTERY_ACTIVE);

    /* Uji pemulihan ke normal */
    bsp_power_restore_run_clock();
    assert(bsp_power_mode_get_current() == METER_POWER_MODE_RUN);

    printf("  [PASS] Transisi clock berhasil.\n");
}

int main(void) {
    printf("============================================================\n");
    printf("   TEST SUITE: STM32U5 STOP 2 ULTRA-LOW POWER MANAGEMENT    \n");
    printf("============================================================\n");

    test_power_mode_initialization();
    test_power_mode_name_strings();
    test_power_mode_ac_failure_toggle();
    test_stop2_wakeup_scenarios();
    test_clock_mode_switches();

    printf("\n>>> SEMUA UJI MANAJEMEN DAYA STOP 2 LULUS (100%%)! <<<\n");
    return 0;
}
