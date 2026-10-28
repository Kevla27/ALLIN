cmake_minimum_required(VERSION 3.20)
project(E3_Application_Suite C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler Warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()

# =========================================================
# E3 Application Core Library
# =========================================================
add_library(e3_core STATIC
    app/display/src/display.c
    app/tamper/src/tamper_manager.c
    app/fsm/src/fsm_statechart.c
    app/fsm/src/power_relay_manager.c
    app/profiling/src/load_profile.c
    app/profiling/src/load_profile_dlms_adapter.c
    app/profiling/src/nvram_storage.c
    app/console/src/console_io.c
    drivers/buzzer/src/bsp_alarm_buzzer.c
    drivers/power/src/bsp_power_mode.c
    tests/e3_mock_stubs.c
)

target_include_directories(e3_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/app/display/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/app/tamper/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/app/fsm/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/app/profiling/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/app/console/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/oled/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/buzzer/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers/power/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../E2/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Metrology
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/Mock
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/SPI
    ${CMAKE_CURRENT_SOURCE_DIR}/../E1/RegisterMap
)

# =========================================================
# Standalone Main Demonstration
# =========================================================
add_executable(e3_demo main.c)
target_link_libraries(e3_demo PRIVATE e3_core)

# =========================================================
# Testing Configuration (CTest)
# =========================================================
enable_testing()

# 1. Main Integration Demo Run
add_test(NAME Integration_E3_Demo COMMAND e3_demo)

# 2. Unit Test: E3 Application (Display & Tamper)
add_executable(test_e3_app tests/test_e3_app.c)
target_link_libraries(test_e3_app PRIVATE e3_core)
add_test(NAME Unit_E3_Display_Tamper COMMAND test_e3_app)

# 3. Unit Test: Power Relay & Asymmetry FSM
add_executable(test_power_relay tests/test_power_relay.c)
target_link_libraries(test_power_relay PRIVATE e3_core)
add_test(NAME Unit_E3_Power_Relay COMMAND test_power_relay)

# 4. Unit Test: FSM Statechart
add_executable(test_fsm_statechart tests/test_fsm_statechart.c)
target_link_libraries(test_fsm_statechart PRIVATE e3_core)
add_test(NAME Unit_E3_FSM_Statechart COMMAND test_fsm_statechart)

# 5. Unit Test: Load Profiling
add_executable(test_load_profile tests/test_load_profile.c)
target_link_libraries(test_load_profile PRIVATE e3_core)
add_test(NAME Unit_E3_Load_Profile COMMAND test_load_profile)

# 6. Unit Test: SPLN Display & Tamper Formats
add_executable(test_spln_display_tamper tests/test_spln_display_tamper.c)
target_link_libraries(test_spln_display_tamper PRIVATE e3_core)
add_test(NAME Unit_E3_SPLN_Display_Tamper COMMAND test_spln_display_tamper)

# 7. Stress & Soak Audit: 10,000 Cycles & Full Billing Test
add_executable(test_final_system_stress tests/test_final_system_stress.c)
target_link_libraries(test_final_system_stress PRIVATE e3_core)
add_test(NAME Stress_E3_Final_System_Audit COMMAND test_final_system_stress)

# 8. Unit Test: Interactive Serial Console CLI Parser
add_executable(test_console_cli tests/test_console_cli.c)
target_link_libraries(test_console_cli PRIVATE e3_core)
add_test(NAME Unit_E3_Console_CLI COMMAND test_console_cli)

# 9. Unit Test: Hardware Board Profile Configuration
add_executable(test_board_profile tests/test_board_profile.c)
target_link_libraries(test_board_profile PRIVATE e3_core)
add_test(NAME Unit_E3_Board_Profile COMMAND test_board_profile)

# 10. Unit Test: Manajemen Power Saving Ekstrem STM32U5 Stop 2 Mode
add_executable(test_power_mode tests/test_power_mode.c)
target_link_libraries(test_power_mode PRIVATE e3_core)
add_test(NAME Unit_E3_Power_Mode_Stop2 COMMAND test_power_mode)
