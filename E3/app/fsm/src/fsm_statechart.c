/**
 * @file fsm_statechart.c
 * @brief Implementasi FSM Statechart Firmware Host MCU STM32U585
 */

#include "fsm_statechart.h"
#include <stddef.h>

void fsm_init(fsm_context_t *ctx) {
    if (ctx == NULL) return;
    ctx->current_state = STATE_0_BOOT_INIT;
    ctx->previous_state = STATE_0_BOOT_INIT;
    ctx->state_enter_time = 0;
    ctx->is_self_test_ok = false;
    ctx->relay_closed = true;
    ctx->tamper_active_mask = 0;
    ctx->last_gasp_dumps = 0;
}

bool fsm_process_event(fsm_context_t *ctx, fsm_event_t evt) {
    if (ctx == NULL) return false;

    fsm_state_t next_state = ctx->current_state;
    bool transition_occurred = false;

    switch (ctx->current_state) {
        case STATE_0_BOOT_INIT:
            if (evt == FSM_EVT_POR_COMPLETE) {
                next_state = STATE_1_SELF_TEST;
                transition_occurred = true;
            }
            break;

        case STATE_1_SELF_TEST:
            if (evt == FSM_EVT_SELF_TEST_PASS) {
                ctx->is_self_test_ok = true;
                next_state = STATE_2_NORMAL_METERING;
                transition_occurred = true;
            } else if (evt == FSM_EVT_SELF_TEST_FAIL) {
                ctx->is_self_test_ok = false;
            }
            break;

        case STATE_2_NORMAL_METERING:
            if (evt == FSM_EVT_PWR_GOOD_LOW) {
                next_state = STATE_4_LAST_GASP_MODE;
                ctx->last_gasp_dumps++;
                transition_occurred = true;
            } else if (evt == FSM_EVT_IRQ0_TAMPER_TRIGGER) {
                next_state = STATE_3_TAMPER_EVENT;
                transition_occurred = true;
            } else if (evt == FSM_EVT_CMD_RELAY_ACTUATE) {
                next_state = STATE_5_ACTUATION;
                transition_occurred = true;
            }
            break;

        case STATE_3_TAMPER_EVENT:
            if (evt == FSM_EVT_PWR_GOOD_LOW) {
                next_state = STATE_4_LAST_GASP_MODE;
                ctx->last_gasp_dumps++;
                transition_occurred = true;
            } else if (evt == FSM_EVT_TAMPER_CLEAR) {
                next_state = STATE_2_NORMAL_METERING;
                transition_occurred = true;
            }
            break;

        case STATE_4_LAST_GASP_MODE:
            if (evt == FSM_EVT_PWR_RESTORED) {
                next_state = STATE_0_BOOT_INIT;
                transition_occurred = true;
            }
            break;

        case STATE_5_ACTUATION:
            if (evt == FSM_EVT_PWR_GOOD_LOW) {
                next_state = STATE_4_LAST_GASP_MODE;
                ctx->last_gasp_dumps++;
                transition_occurred = true;
            } else if (evt == FSM_EVT_RELAY_COMPLETE) {
                ctx->relay_closed = !ctx->relay_closed;
                next_state = STATE_2_NORMAL_METERING;
                transition_occurred = true;
            }
            break;

        default:
            ctx->current_state = STATE_0_BOOT_INIT;
            break;
    }

    if (transition_occurred) {
        ctx->previous_state = ctx->current_state;
        ctx->current_state = next_state;
    }

    return transition_occurred;
}

fsm_state_t fsm_get_current_state(const fsm_context_t *ctx) {
    return (ctx != NULL) ? ctx->current_state : STATE_0_BOOT_INIT;
}

const char* fsm_state_to_string(fsm_state_t state) {
    switch (state) {
        case STATE_0_BOOT_INIT:       return "STATE_0_BOOT_INIT";
        case STATE_1_SELF_TEST:       return "STATE_1_SELF_TEST";
        case STATE_2_NORMAL_METERING: return "STATE_2_NORMAL_METERING";
        case STATE_3_TAMPER_EVENT:    return "STATE_3_TAMPER_EVENT";
        case STATE_4_LAST_GASP_MODE:  return "STATE_4_LAST_GASP_MODE";
        case STATE_5_ACTUATION:       return "STATE_5_ACTUATION";
        default:                      return "STATE_UNKNOWN";

    }
}
        