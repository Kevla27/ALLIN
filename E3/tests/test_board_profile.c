/**
 * @file test_board_profile.c
 * @brief Unit Test Verifikasi Konfigurasi Profil Papan Perangkat Keras (board_profile_config.h)
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "board_profile_config.h"

static void test_default_board_profile_oled(void) {
    printf("[TEST] Verifikasi Profil Default (Prototype OLED SSD1306)...\n");

    /* Verifikasi identitas papan */
    assert(strcmp(board_cfg_get_board_name(), "ALLIN_STM32U585_REV_A") == 0);
    assert(BOARD_HARDWARE_REVISION_MAJOR == 1);
    assert(BOARD_HARDWARE_REVISION_MINOR == 0);

    /* Verifikasi backend display */
    assert(board_cfg_is_oled_active() == true);
    assert(board_cfg_is_segment_lcd_active() == false);
    assert(BOARD_CFG_DISPLAY_I2C_ADDR == 0x78U);
    assert(BOARD_CFG_DISPLAY_WIDTH == 128U);
    assert(BOARD_CFG_DISPLAY_HEIGHT == 32U);
    assert(BOARD_CFG_DISPLAY_WAKE_MS == 10000U);
    assert(BOARD_CFG_DISPLAY_ERR_ROTATE_MS == 3000U);

    /* Verifikasi konfigurasi relai & proteksi tegangan SPLN */
    assert(BOARD_CFG_RELAY_DUAL_COIL == 1);
    assert(BOARD_CFG_RELAY_PULSE_MS == 100U);
    assert(BOARD_CFG_RELAY_UNDERVOLT_DV == 1100U); /* 110.0V */
    assert(BOARD_CFG_RELAY_OVERVOLT_DV == 2800U);  /* 280.0V */
    assert(BOARD_CFG_RELAY_UNDERVOLT_MS == 3000U); /* 3 detik */
    assert(BOARD_CFG_RELAY_RECONNECT_MS == 5000U); /* 5 detik */
    assert(BOARD_CFG_RELAY_DUAL_BTN_MS == 1000U);  /* 1 detik */

    /* Verifikasi sensor tamper */
    assert(BOARD_CFG_TAMPER_CASE_OPEN_LVL == 1);
    assert(BOARD_CFG_TAMPER_TERM_OPEN_LVL == 1);
    assert(BOARD_CFG_TAMPER_MAGNET_LVL == 1);
    assert(BOARD_CFG_TAMPER_MAG_PERSIST_MS == 3000U);
    assert(BOARD_CFG_TAMPER_MAG_CLEAR_MS == 10000U);

    /* Verifikasi penyimpanan NVRAM */
    assert(BOARD_CFG_NVRAM_STORAGE_SPI_FLASH == 1);
    assert(BOARD_CFG_LOAD_PROFILE_CAPACITY == 100U);
    assert(BOARD_CFG_FIFO_LOG_CAPACITY == 30U);
    assert(BOARD_CFG_BILLING_SNAPSHOT_MAX == 12U);

    /* Verifikasi audio buzzer cadence */
    assert(BOARD_CFG_BUZZER_ALARM_ON_MS == 200U);
    assert(BOARD_CFG_BUZZER_ALARM_OFF_MS == 200U);
    assert(BOARD_CFG_BUZZER_KEY_BEEP_MS == 50U);

    /* Verifikasi port komunikasi */
    assert(BOARD_CFG_OPTICAL_BAUDRATE == 9600U);
    assert(BOARD_CFG_MODEM_BAUDRATE == 19200U);
    assert(BOARD_CFG_CLI_BAUDRATE == 115200U);

    /* Verifikasi manajemen hemat daya Stop 2 & telemetri ADC4 */
    assert(board_cfg_is_stop2_mode_enabled() == true);
    assert(BOARD_CFG_ENABLE_STOP2_MODE == 1);
    assert(BOARD_CFG_STOP2_WAKEUP_CLOCK_MSI == 1);
    assert(BOARD_CFG_RTC_WAKEUP_PERIOD_SEC == 60U);
    assert(BOARD_CFG_ADC4_BATT_LOW_MV == 2700U);     /* 2.7V sesuai PIN.docx & SPLN ERR03 */
    assert(BOARD_CFG_ADC4_SUPERCAP_LOW_MV == 2000U); /* 2.0V sesuai ERR06 */
    assert(BOARD_CFG_DISPLAY_CAROUSEL_MS == 5000U);  /* 5.0 detik sesuai SPLN */

    printf("[PASS] Seluruh parameter Profil Papan Default terverifikasi 100%%!\n");
}

int main(void) {
    printf("============================================================\n");
    printf("   TEST SUITE: HARDWARE BOARD PROFILE CONFIGURATION        \n");
    printf("============================================================\n");

    test_default_board_profile_oled();

    printf("\n>>> SEMUA UJI PROFIL PAPAN PERANGKAT KERAS LULUS (100%%)! <<<\n");
    return 0;
}

