/**
 * @file test_fsm_statechart.c
 * @brief Unit Test FSM Statechart Firmware Host MCU STM32U585
 */

#include <stdio.h>
#include <assert.h>
#include "fsm_statechart.h"

void test_fsm_lifecycle(void) {
    printf("[TEST] Testing FSM Lifecycle (State 0 -> 1 -> 2 -> 3 -> 4 -> 0 -> 5)...\n");

    fsm_context_t fsm;
    fsm_init(&fsm);
    assert(fsm.current_state == STATE_0_BOOT_INIT);

    // Transisi State 0 -> State 1 (POR Complete)
    bool ok = fsm_process_event(&fsm, FSM_EVT_POR_COMPLETE);
    assert(ok && fsm.current_state == STATE_1_SELF_TEST);

    // Transisi State 1 -> State 2 (Self Test Pass)
    ok = fsm_process_event(&fsm, FSM_EVT_SELF_TEST_PASS);
    assert(ok && fsm.current_state == STATE_2_NORMAL_METERING);
    assert(fsm.is_self_test_ok == true);

    // Transisi State 2 -> State 3 (Tamper IRQ0)
    ok = fsm_process_event(&fsm, FSM_EVT_IRQ0_TAMPER_TRIGGER);
    assert(ok && fsm.current_state == STATE_3_TAMPER_EVENT);

    // Transisi State 3 -> State 4 (Last Gasp Outage)
    ok = fsm_process_event(&fsm, FSM_EVT_PWR_GOOD_LOW);
    assert(ok && fsm.current_state == STATE_4_LAST_GASP_MODE);
    assert(fsm.last_gasp_dumps == 1);

    // Transisi State 4 -> State 0 (Power Restored)
    ok = fsm_process_event(&fsm, FSM_EVT_PWR_RESTORED);
    assert(ok && fsm.current_state == STATE_0_BOOT_INIT);

    // Boot ulang sampai Normal Metering
    fsm_process_event(&fsm, FSM_EVT_POR_COMPLETE);
    fsm_process_event(&fsm, FSM_EVT_SELF_TEST_PASS);
    assert(fsm.current_state == STATE_2_NORMAL_METERING);

    // Transisi State 2 -> State 5 (Relay Actuation)
    ok = fsm_process_event(&fsm, FSM_EVT_CMD_RELAY_ACTUATE);
    assert(ok && fsm.current_state == STATE_5_ACTUATION);

    // Actuation Selesai -> Kembali ke Normal Metering
    ok = fsm_process_event(&fsm, FSM_EVT_RELAY_COMPLETE);
    assert(ok && fsm.current_state == STATE_2_NORMAL_METERING);

    printf("   Current State: %s\n", fsm_state_to_string(fsm.current_state));
    printf("   [PASS] FSM Statechart All Transitions Validated 100%%!\n\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING FSM STATECHART UNIT TEST        \n");
    printf("=========================================\n\n");

    test_fsm_lifecycle();

    printf("=========================================\n");
    printf(" ALL FSM TESTS PASSED SUCCESSFULLY! (100%%)\n");
    printf("=========================================\n");
    return 0;
}