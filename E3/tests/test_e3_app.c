/**
 * @file test_e3_app.c
 * @brief Unit Test untuk Modul Display & Tamper Manager E3
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "display.h"
#include "tamper_manager.h"

void test_display_carousel(void) {
    printf("[TEST] Testing Display Auto Carousel & Button Nav...\n");

    display_context_t disp;
    bool ok = display_init(&disp, "54321678901", 3000);
    assert(ok);
    assert(disp.current_page == DISP_PAGE_IDPEL);

    meter_measurements_t meas = { .voltage_r_dvolts = 2205 }; // 220.5 V
    display_update_measurements(&disp, &meas);

    // Simulasi 3 detik -> Carousel bergulir ke VOLTAGE_R
    display_process_tick(&disp, 3000);
    assert(disp.current_page == DISP_PAGE_VOLTAGE_R);

    char l1[64], l2[64], l3[64];
    display_render_frame(&disp, l1, l2, l3, sizeof(l1));
    
    printf("   Render Line 1: %s\n", l1);
    printf("   Render Line 2: %s\n", l2);
    printf("   Render Line 3: %s\n", l3);

    assert(strstr(l3, "220.5 V") != NULL);

    /* Pastikan display_update_measurements tidak menimpa konfigurasi statis Tabel E.2 dengan 0 */
    assert(disp.meas_buffer.power_limit_w == 11000U);
    assert(disp.meas_buffer.firmware_checksum == 0xA5B6U);
    assert(disp.meas_buffer.eob_day == 1U);
    assert(disp.meas_buffer.eob_hour == 10U);
    assert(strcmp(disp.meas_buffer.pln_unit_code, "PLN UID JAYA") == 0);

    printf("   [PASS] Display Carousel & Rendering OK!\n\n");
}

void test_tamper_manager_fifo(void) {
    printf("[TEST] Testing Tamper Event Manager & FIFO Log...\n");

    tamper_context_t tamper;
    tamper_init(&tamper);

    // Event 1: Case Open
    tamper_process_signal(&tamper, TAMPER_VECTOR_CASE_OPEN, true, 1000);
    assert(tamper_get_active_mask(&tamper) & TAMPER_VECTOR_CASE_OPEN);

    // Event 2: Medan Magnet
    tamper_process_signal(&tamper, TAMPER_VECTOR_MAGNETIC_FIELD, true, 1005);

    tamper_event_entry_t logs[10];
    size_t count = 0;
    tamper_get_log_entries(&tamper, logs, 10, &count);
    assert(count == 2);

    printf("   Active Mask: 0x%02X, Log Count: %zu\n", tamper_get_active_mask(&tamper), count);
    printf("   [PASS] Tamper Event Manager OK!\n\n");
}

void test_spln_tabel6_relay_and_led_behavior(void) {
    printf("[TEST] Testing SPLN D3.006:2021 Tabel 6 Relay & LED Behavior...\n");

    tamper_context_t ctx;

    // 1. Case Open (Tutup Meter Dibuka): Relai Buka, LED Kuning MATI (-)
    tamper_init(&ctx);
    tamper_process_signal(&ctx, TAMPER_VECTOR_CASE_OPEN, true, 1000);
    assert(tamper_should_trip_relay(&ctx) == true);
    assert(tamper_should_turn_on_led(&ctx) == false);

    // 2. Terminal Open (Tutup Terminal Dibuka): Relai Buka, LED Kuning AKTIF
    tamper_init(&ctx);
    tamper_process_signal(&ctx, TAMPER_VECTOR_TERMINAL_OPEN, true, 1000);
    assert(tamper_should_trip_relay(&ctx) == true);
    assert(tamper_should_turn_on_led(&ctx) == true);

    // 3. Wrong Phase Sequence (Urutan Fase Terbalik): Relai Buka, LED Kuning AKTIF
    tamper_init(&ctx);
    tamper_process_signal(&ctx, TAMPER_VECTOR_WRONG_SEQUENCE, true, 1000);
    assert(tamper_should_trip_relay(&ctx) == true);
    assert(tamper_should_turn_on_led(&ctx) == true);

    // 4. Cross Phase (Pengawatan Silang): Relai Buka, LED Kuning AKTIF
    tamper_init(&ctx);
    tamper_process_signal(&ctx, TAMPER_VECTOR_CROSS_PHASE, true, 1000);
    assert(tamper_should_trip_relay(&ctx) == true);
    assert(tamper_should_turn_on_led(&ctx) == true);

    // 5. Neutral Missing / Bypass: Relai TIDAK Buka, LED Kuning AKTIF
    tamper_init(&ctx);
    tamper_process_signal(&ctx, TAMPER_VECTOR_NEUTRAL_BYPASS, true, 1000);
    assert(tamper_should_trip_relay(&ctx) == false);
    assert(tamper_should_turn_on_led(&ctx) == true);

    // 6. Phase Loss (Hilang Tegangan 1/2 Fase): Relai Buka, LED Kuning AKTIF
    tamper_init(&ctx);
    tamper_process_signal(&ctx, TAMPER_VECTOR_PHASE_LOSS, true, 1000);
    assert(tamper_should_trip_relay(&ctx) == true);
    assert(tamper_should_turn_on_led(&ctx) == true);

    // 7. Magnetic Field <= 500 mT (ERR25): Relai TIDAK Buka (Catatan 4), LED Kuning MATI (-)
    tamper_init(&ctx);
    tamper_process_signal(&ctx, TAMPER_VECTOR_MAGNETIC_FIELD, true, 1000);
    assert(tamper_should_trip_relay(&ctx) == false);
    assert(tamper_should_turn_on_led(&ctx) == false);

    // 8. Reverse Power (ERR26): Relai TIDAK Buka, LED Kuning AKTIF
    tamper_init(&ctx);
    tamper_process_signal(&ctx, TAMPER_VECTOR_REVERSE_POWER, true, 1000);
    assert(tamper_should_trip_relay(&ctx) == false);
    assert(tamper_should_turn_on_led(&ctx) == true);

    // 9. Low PF Quad 4 (ERR27): Relai TIDAK Buka, LED Kuning AKTIF
    tamper_init(&ctx);
    tamper_process_signal(&ctx, TAMPER_VECTOR_LOW_PF_QUAD4, true, 1000);
    assert(tamper_should_trip_relay(&ctx) == false);
    assert(tamper_should_turn_on_led(&ctx) == true);

    // 10. Kombinasi: Case Open (Relay ON, LED OFF) + Magnetic Field (Relay OFF, LED OFF)
    // -> Relay ON, LED OFF
    tamper_init(&ctx);
    tamper_process_signal(&ctx, TAMPER_VECTOR_CASE_OPEN, true, 1000);
    tamper_process_signal(&ctx, TAMPER_VECTOR_MAGNETIC_FIELD, true, 1001);
    assert(tamper_should_trip_relay(&ctx) == true);
    assert(tamper_should_turn_on_led(&ctx) == false);

    // 11. Kombinasi: Case Open + Reverse Power -> Relay ON, LED ON
    tamper_process_signal(&ctx, TAMPER_VECTOR_REVERSE_POWER, true, 1002);
    assert(tamper_should_trip_relay(&ctx) == true);
    assert(tamper_should_turn_on_led(&ctx) == true);

    // 12. Pemulihan Sabotase: Case Open ditutup kembali & Reverse Power dinetralkan
    tamper_process_signal(&ctx, TAMPER_VECTOR_CASE_OPEN, false, 1003);
    assert(tamper_should_trip_relay(&ctx) == false); // Relai bebas dari blokir sabotase
    assert(tamper_should_turn_on_led(&ctx) == true);  // Masih ada Reverse Power

    tamper_process_signal(&ctx, TAMPER_VECTOR_REVERSE_POWER, false, 1004);
    assert(tamper_should_trip_relay(&ctx) == false);
    assert(tamper_should_turn_on_led(&ctx) == false); // Semua pulih normal
    assert(ctx.alarm_relay_trigger == false);

    printf("   [PASS] SPLN D3.006:2021 Tabel 6 Relay & LED Behavior Verified!\n\n");
}

void test_spln_tabel4_internal_alarm_behavior(void) {
    printf("[TEST] Testing SPLN D3.006:2021 Tabel 4 Internal Alarm Hardware Behavior...\n");

    // 1. Flash memory rusak (ERR00): Relai Buka (TRIP = true), LED Kuning Aktif (LED = true)
    uint32_t alm_flash = SPLN_ALARM_FLASH_ERROR;
    assert(spln_alarm_should_trip_relay(alm_flash) == true);
    assert(spln_alarm_should_turn_on_led(alm_flash) == true);

    // 2. RAM rusak/error (ERR01): Relai Buka (TRIP = true), LED Kuning Aktif (LED = true)
    uint32_t alm_ram = SPLN_ALARM_RAM_ERROR;
    assert(spln_alarm_should_trip_relay(alm_ram) == true);
    assert(spln_alarm_should_turn_on_led(alm_ram) == true);

    // 3. Clock loss (ERR02): Relai Buka (TRIP = true), LED Kuning Aktif (LED = true)
    uint32_t alm_rtc = SPLN_ALARM_RTC_ERROR;
    assert(spln_alarm_should_trip_relay(alm_rtc) == true);
    assert(spln_alarm_should_turn_on_led(alm_rtc) == true);

    // 4. Low battery (ERR03): Relai TIDAK Buka (TRIP = false), LED Kuning Aktif (LED = true)
    uint32_t alm_bat = SPLN_ALARM_LOW_BATTERY;
    assert(spln_alarm_should_trip_relay(alm_bat) == false);
    assert(spln_alarm_should_turn_on_led(alm_bat) == true);

    // 5. Mikroprosesor tidak berfungsi normal (ERR04): Relai Buka (TRIP = true), LED Kuning Aktif (LED = true)
    uint32_t alm_mcu = SPLN_ALARM_MCU_ERROR;
    assert(spln_alarm_should_trip_relay(alm_mcu) == true);
    assert(spln_alarm_should_turn_on_led(alm_mcu) == true);

    // 6. Kegagalan sampling ADC (ERR05): Relai Buka (TRIP = true), LED Kuning Aktif (LED = true)
    uint32_t alm_adc = SPLN_ALARM_ADC_ERROR;
    assert(spln_alarm_should_trip_relay(alm_adc) == true);
    assert(spln_alarm_should_turn_on_led(alm_adc) == true);

    // 7. Superkapasitor rusak/lepas (ERR06): Relai TIDAK Buka (TRIP = false), LED Kuning Aktif (LED = true)
    uint32_t alm_supercap = SPLN_ALARM_SUPERCAP_FAIL;
    assert(spln_alarm_should_trip_relay(alm_supercap) == false);
    assert(spln_alarm_should_turn_on_led(alm_supercap) == true);

    // 8. Relai/shunt trip gagal (ERR07): Relai TIDAK Buka (TRIP = false), LED Kuning Aktif (LED = true)
    uint32_t alm_relay = SPLN_ALARM_RELAY_FAIL;
    assert(spln_alarm_should_trip_relay(alm_relay) == false);
    assert(spln_alarm_should_turn_on_led(alm_relay) == true);

    // 9. Kombinasi: Low Battery (TRIP = false) + Supercap (TRIP = false) -> TRIP = false, LED = true
    uint32_t non_critical_alarms = SPLN_ALARM_LOW_BATTERY | SPLN_ALARM_SUPERCAP_FAIL;
    assert(spln_alarm_should_trip_relay(non_critical_alarms) == false);
    assert(spln_alarm_should_turn_on_led(non_critical_alarms) == true);

    // 10. Kombinasi: Low Battery + ADC Sampling Error (Critical) -> TRIP = true, LED = true
    uint32_t mixed_alarms = SPLN_ALARM_LOW_BATTERY | SPLN_ALARM_ADC_ERROR;
    assert(spln_alarm_should_trip_relay(mixed_alarms) == true);
    assert(spln_alarm_should_turn_on_led(mixed_alarms) == true);

    // 11. Tidak ada alarm (SPLN_ALARM_NONE) -> TRIP = false, LED = false
    assert(spln_alarm_should_trip_relay(SPLN_ALARM_NONE) == false);
    assert(spln_alarm_should_turn_on_led(SPLN_ALARM_NONE) == false);

    printf("   [PASS] SPLN D3.006:2021 Tabel 4 Internal Alarm Behavior Verified!\n\n");
}

void test_spln_butir12_3_three_fifo_registers(void) {
    printf("[TEST] Testing SPLN D3.006:2021 Butir 12.3 Three FIFO Registers & Error Register...\n");

    tamper_context_t ctx;
    tamper_init(&ctx);

    /* 1. Pengujian Register 1: Alarm Log FIFO (8 jenis, min. 30 entri) */
    assert(ctx.alarm_log_fifo.count == 0);
    for (uint32_t i = 1; i <= 35; i++) {
        spln_alarm_log_code_t code = (spln_alarm_log_code_t)((i % 8) + 1);
        spln_log_record_alarm(&ctx, code, true, 1000 + i, i * 10);
    }
    // Kapasitas FIFO maksimal 30, entri ke-31 s.d 35 menimpa entri paling awal
    assert(ctx.alarm_log_fifo.count == SPLN_LOG_FIFO_MAX_ENTRIES);
    assert(ctx.alarm_log_fifo.total_events == 35);

    // Selective Access: Baca entri pertama (indeks 0, yang tertua yang masih tersimpan)
    spln_standard_log_entry_t entry;
    bool ok = spln_log_get_entry_by_index(&ctx.alarm_log_fifo, 0, &entry);
    assert(ok);
    assert(entry.timestamp_epoch == 1000 + 6); // Entri ke-6 (timestamp 1006) adalah yang tertua tersisa

    // Selective Access: Baca entri terakhir (indeks 29)
    ok = spln_log_get_entry_by_index(&ctx.alarm_log_fifo, 29, &entry);
    assert(ok);
    assert(entry.timestamp_epoch == 1035);

    printf("   Alarm Log FIFO (30 entries overflow, FIFO rolling, selective access) [PASS]\n");

    /* 2. Pengujian Register 2: Event Log FIFO (12 jenis, min. 30 entri) */
    assert(ctx.event_log_fifo.count == 0);
    spln_log_record_event(&ctx, SPLN_LOG_EVENT_POWER_FAILURE, true, 2001, 0);
    spln_log_record_event(&ctx, SPLN_LOG_EVENT_POWER_UP, true, 2002, 0);
    spln_log_record_event(&ctx, SPLN_LOG_EVENT_ASYMMETRIC_POWER, true, 2003, 45); // 45% deviasi
    spln_log_record_event(&ctx, SPLN_LOG_EVENT_POWER_LIMIT_EXC, true, 2004, 7500); // 7500 W
    assert(ctx.event_log_fifo.count == 4);
    assert(ctx.event_log_fifo.total_events == 4);

    spln_standard_log_entry_t event_entries[10];
    size_t event_count = 0;
    spln_log_get_entries(&ctx.event_log_fifo, event_entries, 10, &event_count);
    assert(event_count == 4);
    assert(event_entries[0].event_code == SPLN_LOG_EVENT_POWER_FAILURE);
    assert(event_entries[2].parameter_val == 45); // Asymmetric deviation 45%
    printf("   Event Log FIFO (Power failure, Power up, Asymmetry, Limiter) [PASS]\n");

    /* 3. Pengujian Register 3: Tamper Log FIFO (9 jenis, min. 30 entri) */
    assert(ctx.tamper_log_fifo.count == 0);
    tamper_process_signal(&ctx, TAMPER_VECTOR_CASE_OPEN, true, 3001);
    tamper_process_signal(&ctx, TAMPER_VECTOR_MAGNETIC_FIELD, true, 3002);
    tamper_process_signal(&ctx, TAMPER_VECTOR_REVERSE_POWER, true, 3003);
    assert(ctx.tamper_log_fifo.count == 3);

    spln_standard_log_entry_t tamper_entries[10];
    size_t tamper_count = 0;
    spln_log_get_entries(&ctx.tamper_log_fifo, tamper_entries, 10, &tamper_count);
    assert(tamper_count == 3);
    assert(tamper_entries[0].event_code == SPLN_LOG_TAMPER_CASE_OPEN);
    assert(tamper_entries[1].event_code == SPLN_LOG_TAMPER_MAGNETIC_FIELD);
    assert(tamper_entries[2].event_code == SPLN_LOG_TAMPER_REVERSE_POWER);
    printf("   Tamper Log FIFO (Case open, Magnetic field, Reverse power) [PASS]\n");

    /* 4. Pengujian Error Register (Bit-String Rangkuman Real-Time SPLN Butir 12.3) */
    uint32_t err_reg = spln_log_get_error_register(&ctx);
    // Verifikasi bit alarm aktif (bit 0..7)
    assert((err_reg & 0xFF) != 0);
    // Verifikasi bit event aktif (bit 8..19): POWER_FAILURE (bit 8), POWER_UP (bit 9), ASYMMETRIC (bit 17), POWER_LIMIT (bit 19)
    assert((err_reg & (1U << 8)) != 0);
    assert((err_reg & (1U << 9)) != 0);
    // Verifikasi bit tamper aktif (bit 20..28): CASE_OPEN (bit 20), MAGNETIC (bit 26), REVERSE (bit 27)
    assert((err_reg & (1U << 20)) != 0);
    assert((err_reg & (1U << 26)) != 0);
    assert((err_reg & (1U << 27)) != 0);
    printf("   Error Register Bit-String: 0x%08X (Alarm, Event, Tamper summary verified) [PASS]\n", err_reg);

    /* 5. Pengujian Pemulihan Alarm Baterai Tanpa Reset (SPLN D3.006: 2021 Butir 6.11 Hal 39) */
    uint32_t batt_bit = (1U << ((uint32_t)SPLN_LOG_ALARM_LOW_BATTERY - 1U));
    assert((ctx.error_register_bitstring & batt_bit) != 0);
    // Baterai pulih: catat deassertion (is_asserted = false)
    spln_log_record_alarm(&ctx, SPLN_LOG_ALARM_LOW_BATTERY, false, 4001, 3600);
    assert((ctx.error_register_bitstring & batt_bit) == 0); // Bit alarm baterai terhapus otomatis!
    printf("   Battery Recovery Deassertion: Bitstring cleared without reset [PASS]\n\n");
}

int main(void) {
    printf("=========================================\n");
    printf(" RUNNING E3 FIRMWARE APPLICATION TESTS   \n");
    printf("=========================================\n\n");

    test_display_carousel();
    test_tamper_manager_fifo();
    test_spln_tabel6_relay_and_led_behavior();
    test_spln_tabel4_internal_alarm_behavior();
    test_spln_butir12_3_three_fifo_registers();

    printf("=========================================\n");
    printf(" ALL E3 TESTS PASSED SUCCESSFULLY! (100%%)\n");
    printf("=========================================\n");
    return 0;
}
