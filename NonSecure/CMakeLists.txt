cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Compiler options
set(STM32_MCU_FLAGS  "-mcpu=cortex-m33 -mfpu=fpv5-sp-d16 -mfloat-abi=hard " )

#Linker options
set(STM32_LINKER_SCRIPT STM32U585xx_FLASH_ns.ld)
set(STM32_LINKER_OPTION  )

# Set the project name
set(CMAKE_PROJECT_NAME ALLIN_NS)
project(${CMAKE_PROJECT_NAME})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Include mx-generated
include("mx-generated.cmake")


# In order to use CMake for cross-compiling
set(CMAKE_CXX_COMPILER_FORCED "true")

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    EMBEDDED_HARDWARE_TARGET
)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/../E1
    ${CMAKE_SOURCE_DIR}/../E1/Metrology
    ${CMAKE_SOURCE_DIR}/../E1/Mock
    ${CMAKE_SOURCE_DIR}/../E1/RegisterMap
    ${CMAKE_SOURCE_DIR}/../E1/SPI
    ${CMAKE_SOURCE_DIR}/../E2/Inc
    ${CMAKE_SOURCE_DIR}/../E3/app/display/inc
    ${CMAKE_SOURCE_DIR}/../E3/app/tamper/inc
    ${CMAKE_SOURCE_DIR}/../E3/app/fsm/inc
    ${CMAKE_SOURCE_DIR}/../E3/app/profiling/inc
    ${CMAKE_SOURCE_DIR}/../E3/app/console/inc
    ${CMAKE_SOURCE_DIR}/../E3/drivers/buzzer/inc
    ${CMAKE_SOURCE_DIR}/../E3/drivers/oled/inc
    ${CMAKE_SOURCE_DIR}/../E3/drivers/power/inc
    ${CMAKE_SOURCE_DIR}/../E3/app
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/../E1/Metrology/Metrology.c
    ${CMAKE_SOURCE_DIR}/../E1/Mock/Mock.c
    ${CMAKE_SOURCE_DIR}/../E1/SPI/SPI.c
    ${CMAKE_SOURCE_DIR}/../E2/Src/ami_cosem_model.c
    ${CMAKE_SOURCE_DIR}/../E2/Src/ami_dlms_apdu.c
    ${CMAKE_SOURCE_DIR}/../E2/Src/ami_dlms_association.c
    ${CMAKE_SOURCE_DIR}/../E2/Src/ami_dlms_hdlc.c
    ${CMAKE_SOURCE_DIR}/../E2/Src/ami_security.c
    ${CMAKE_SOURCE_DIR}/../E3/app/display/src/display.c
    ${CMAKE_SOURCE_DIR}/../E3/app/tamper/src/tamper_manager.c
    ${CMAKE_SOURCE_DIR}/../E3/app/fsm/src/power_relay_manager.c
    ${CMAKE_SOURCE_DIR}/../E3/app/fsm/src/fsm_statechart.c
    ${CMAKE_SOURCE_DIR}/../E3/app/profiling/src/load_profile.c
    ${CMAKE_SOURCE_DIR}/../E3/app/profiling/src/load_profile_dlms_adapter.c
    ${CMAKE_SOURCE_DIR}/../E3/app/profiling/src/nvram_storage.c
    ${CMAKE_SOURCE_DIR}/../E3/app/console/src/console_io.c
    ${CMAKE_SOURCE_DIR}/../E3/drivers/buzzer/src/bsp_alarm_buzzer.c
    ${CMAKE_SOURCE_DIR}/../E3/drivers/power/src/bsp_power_mode.c
    ${CMAKE_SOURCE_DIR}/../E3/drivers/oled/src/bsp_display_oled.c
    ${CMAKE_SOURCE_DIR}/../E3/drivers/oled/src/ssd1306.c
    ${CMAKE_SOURCE_DIR}/../E3/drivers/oled/src/ssd1306_fonts.c
    ${CMAKE_SOURCE_DIR}/../E1/Metrology/e1_metrology_service.c
    ${CMAKE_SOURCE_DIR}/../E2/Src/e2_dlms_service.c
    ${CMAKE_SOURCE_DIR}/../E3/app/e3_meter_app.c
    ${CMAKE_SOURCE_DIR}/Core/Src/meter_system_bridge.c
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined libraries
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()









