/**
 * @file e3_mock_stubs.c
 * @brief Mock Stubs untuk pengujian unit standalone E3 (Display & Tamper Callback)
 */

#include <stdbool.h>

static bool s_mock_alarm_active = false;

bool display_task_is_alarm_active(void) {
    return s_mock_alarm_active;
}

void display_task_trigger_instant_refresh(bool active) {
    s_mock_alarm_active = active;
}

