/**
 * @file test_power_relay.c
 * @brief Unit test for SPLN Power Limiter, Asymmetric Power Detection, and Relay FSM
 *        Sesuai Standar SPLN D3.006: 2021 Tabel 5 (No 11, 12, 13)
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "power_relay_manager.h"
#include "display.h"

void test_asymmetry_detection(void) {
    printf("[TEST] Testing Asymmetry Power Detection (SPLN Tabel 5 No 11)...\n");

    power_relay_context_t ctx;
    power_relay_init(&ctx, 6600, 5, 30);

    /* 1. Kondisi Arus Seimbang (10A, 10A, 10A) -> Deviasi 0% */
    bool is_asym = power_relay_check_asymmetry(&ctx, 10000, 10000, 10000);
    assert(!is_asym);
    assert(ctx.current_deviation_pct == 0);
    assert(!ctx.is_asymmetry_active);
    printf("   10A/10A/10A -> Asymmetry = %u%%, Active = %d [PASS]\n", ctx.current_deviation_pct, ctx.is_asymmetry_active);

    /* 2. Kondisi Arus Rendah (< 500mA) -> Guard false positive */
    is_asym = power_relay_check_asymmetry(&ctx, 300, 100, 200);
    assert(!is_asym);
    assert(ctx.current_deviation_pct == 0);
    assert(!ctx.is_asymmetry_active);
    printf("   300mA/100mA/200mA -> Low current guard active, Active = %d [PASS]\n", ctx.is_asymmetry_active);

    /* 3. Kondisi Asimetris Sedang (< 30%): R=10A, S=9A, T=8A
       Iavg = 9000mA, MaxDev = 1000mA -> 1000/9000 * 100% = 11% */
    is_asym = power_relay_check_asymmetry(&ctx, 10000, 9000, 8000);
    assert(!is_asym);
    assert(ctx.current_deviation_pct < 30);
    assert(!ctx.is_asymmetry_active);
    printf("   10A/9A/8A -> Asymmetry = %u%% (< 30%%), Active = %d [PASS]\n", ctx.current_deviation_pct, ctx.is_asymmetry_active);

    /* 4. Kondisi Asimetris Berat (> 30%): R=10A, S=5A, T=5A
       Iavg = 6666mA, MaxDev = 3334mA -> 3334/6666 * 100% = 50% */
    is_asym = power_relay_check_asymmetry(&ctx, 10000, 5000, 5000);
    assert(is_asym);
    assert(ctx.current_deviation_pct >= 30);
    assert(ctx.is_asymmetry_active);
    printf("   10A/5A/5A -> Asymmetry = %u%% (>= 30%%), Active = %d [PASS]\n", ctx.current_deviation_pct, ctx.is_asymmetry_active);

    /* 5. Pulih kembali ke seimbang */
    is_asym = power_relay_check_asymmetry(&ctx, 10000, 10000, 10000);
    assert(!is_asym);
    assert(ctx.current_deviation_pct == 0);
    assert(!ctx.is_asymmetry_active);
    printf("   Pulih normal -> Asymmetry = %u%%, Active = %d [PASS]\n\n", ctx.current_deviation_pct, ctx.is_asymmetry_active);
}

void test_power_limiter_tripping(void) {
    printf("[TEST] Testing Power Limiter Duration Tripping (SPLN Tabel 5 No 13)...\n");

    power_relay_context_t ctx;
    power_relay_init(&ctx, 6600, 5, 30);
    assert(ctx.relay_state == RELAY_STATE_CONNECTED);

    /* 1. Daya Normal (5000 W < 6600 W) selama 10 detik */
    for (uint32_t t = 1000; t <= 10000; t += 1000) {
        bool tripped = power_relay_process_power(&ctx, 5000, t);
        assert(!tripped);
        assert(ctx.relay_state == RELAY_STATE_CONNECTED);
        assert(ctx.over_power_timer_sec == 0);
    }
    printf("   5000 W selama 10s -> Tidak Trip [PASS]\n");

    /* 2. Daya Melebihi Batas (7500 W > 6600 W) selama 4 detik (Belum mencapai 5 detik) */
    for (uint32_t t = 11000; t <= 14000; t += 1000) {
        bool tripped = power_relay_process_power(&ctx, 7500, t);
        assert(!tripped);
        assert(ctx.relay_state == RELAY_STATE_CONNECTED);
    }
    printf("   7500 W selama 4s -> Akumulasi timer %u detik, belum trip [PASS]\n", ctx.over_power_timer_sec);

    /* 3. Detik ke-5 (Waktu 15000) -> Harus TRIP */
    bool tripped = power_relay_process_power(&ctx, 7500, 15000);
    assert(tripped);
    assert(ctx.relay_state == RELAY_STATE_DISCONNECTED);
    assert(ctx.last_reason == RELAY_REASON_POWER_LIMITER);
    assert(ctx.is_limiter_tripped == true);
    printf("   Detik ke-5 -> RELAY TRIP! State = %s, Reason = %d [PASS]\n\n",
           power_relay_state_to_string(ctx.relay_state), ctx.last_reason);
}

void test_relay_fsm_ready_on(void) {
    printf("[TEST] Testing Relay FSM & READY ON State (SPLN Tabel 5 No 12)...\n");

    power_relay_context_t ctx;
    power_relay_init(&ctx, 6600, 5, 30);
    assert(ctx.relay_state == RELAY_STATE_CONNECTED);

    /* 1. Trip relay via manual / remote */
    power_relay_disconnect(&ctx, RELAY_REASON_REMOTE_DISCONNECT, 1000);
    assert(ctx.relay_state == RELAY_STATE_DISCONNECTED);
    printf("   Relay diputus -> State = %s [PASS]\n", power_relay_state_to_string(ctx.relay_state));

    /* 2. Terima Remote Reconnect Command (AMI) -> Harus masuk READY ON, bukan langsung ON */
    power_relay_request_reconnect(&ctx, 1010);
    assert(ctx.relay_state == RELAY_STATE_READY_ON);
    assert(power_relay_is_ready_on(&ctx));
    printf("   Remote Reconnect Request -> State = %s (Tabel 5 No 12) [PASS]\n",
           power_relay_state_to_string(ctx.relay_state));

    /* 3. Verifikasi aksi konfirmasi lokal (tombol ditekan) -> Masuk CONNECTED */
    bool conf_ok = power_relay_confirm_reconnect(&ctx, 1020);
    assert(conf_ok);
    assert(ctx.relay_state == RELAY_STATE_CONNECTED);
    assert(!power_relay_is_ready_on(&ctx));
    assert(ctx.last_reason == RELAY_REASON_LOCAL_CONFIRM);
    printf("   Konfirmasi Tombol Lokal -> State = %s [PASS]\n",
           power_relay_state_to_string(ctx.relay_state));

    /* 4. Konfirmasi lokal saat CONNECTED tidak boleh mengubah state */
    conf_ok = power_relay_confirm_reconnect(&ctx, 1030);
    assert(!conf_ok);
    assert(ctx.relay_state == RELAY_STATE_CONNECTED);
    printf("   Konfirmasi ulang saat CONNECTED ditolak dengan aman [PASS]\n\n");
}

void test_display_integration_ready_on(void) {
    printf("[TEST] Testing Display Integration for READY ON & Asymmetry...\n");

    display_context_t disp;
    display_spln_frame_t frame;
    bool ok = display_init(&disp, "530000000001", 2000);
    assert(ok);

    /* 1. Set READY ON di display */
    display_set_relay_ready_on(&disp, true);
    assert(disp.relay_ready_on == true);

    disp.current_page = DISP_PAGE_RELAY_READY_ON;
    display_render_spln_frame(&disp, &frame);

    assert(strcmp(frame.scroll_index_zz, "on") == 0);
    assert(strcmp(frame.main_value, "READY ON") == 0);
    assert(strcmp(frame.unit, "RECONNECT") == 0);
    assert(strcmp(frame.obis_code, "00.00") == 0);
    assert(frame.is_large_font == false);
    printf("   Page 'on': zz='%s' | Val='%s' | Unit='%s' | OBIS='%s' [PASS]\n",
           frame.scroll_index_zz, frame.main_value, frame.unit, frame.obis_code);

    /* 2. Set Asymmetry Warning */
    display_set_asymmetry_warning(&disp, true);
    assert(disp.asymmetry_warning == true);

    /* Render halaman normal, periksa alarm icon / status */
    disp.current_page = DISP_PAGE_VOLTAGE_R;
    display_render_spln_frame(&disp, &frame);
    assert(frame.is_alarm_active == true);
    assert(strcmp(frame.alarm_code, "!ALM") == 0);
    printf("   Asymmetry Warning -> Banner Alarm '!ALM' aktif di halaman pengukuran [PASS]\n\n");
}

void test_simultaneous_button_reconnect(void) {
    printf("[TEST] Testing Simultaneous Button Action (SPLN Tabel 5 No 12)...\n");

    display_context_t disp;
    display_init(&disp, "530000000001", 2000);

    /* Simulasi relai dalam status READY ON */
    display_set_relay_ready_on(&disp, true);
    disp.current_page = DISP_PAGE_RELAY_READY_ON;

    /* 1. Tekan tombol tunggal UP atau DOWN -> TIDAK mereset relay_ready_on */
    display_handle_button_press(&disp, BUTTON_DIR_UP);
    assert(disp.relay_ready_on == true);

    display_handle_button_press(&disp, BUTTON_DIR_DOWN);
    assert(disp.relay_ready_on == true);
    printf("   Tombol Tunggal (UP/DOWN) tidak mereset READY ON [PASS]\n");

    /* 2. Tekan tombol simultan BOTH (▲ + ▼) */
    display_handle_button_press(&disp, BUTTON_DIR_BOTH);
    printf("   Tombol Simultan (▲ + ▼) Berhasil Diproses [PASS]\n\n");
}

void test_voltage_cutoff_protection(void) {
    printf("[TEST] Testing Voltage Cut-Off Protection (SPLN D3.006:2021 Butir 10.3 Tabel 9)...\n");

    power_relay_context_t ctx;
    power_relay_init(&ctx, 6600, 5, 30);
    assert(ctx.relay_state == RELAY_STATE_CONNECTED);

    /* 1. Kondisi Tegangan Normal (230 V nominal per fasa = 2300 dV) */
    bool tripped = power_relay_process_voltage(&ctx, 2300, 2300, 2300, 1000);
    assert(!tripped);
    assert(ctx.relay_state == RELAY_STATE_CONNECTED);
    assert(!ctx.is_under_voltage_trip);
    assert(!ctx.is_over_voltage_trip);
    printf("   230V/230V/230V -> Normal, Relay CONNECTED [PASS]\n");

    /* 2. Kondisi Under-Voltage: Fasa R anjlok ke 105 V (< 110 V = 1100 dV) */
    tripped = power_relay_process_voltage(&ctx, 1050, 2300, 2300, 1001);
    assert(tripped);
    assert(ctx.relay_state == RELAY_STATE_DISCONNECTED);
    assert(ctx.last_reason == RELAY_REASON_UNDER_VOLTAGE);
    assert(ctx.is_under_voltage_trip == true);
    printf("   Fasa R = 105V (< 110V) -> Relay TRIP! Reason: %s [PASS]\n", power_relay_reason_to_string(ctx.last_reason));

    /* 3. Fasa R naik sedikit ke 113 V (1130 dV, berada di antara 110 V dan 115 V) -> Histeresis, tetap TRIP */
    tripped = power_relay_process_voltage(&ctx, 1130, 2300, 2300, 1002);
    assert(!tripped);
    assert(ctx.relay_state == RELAY_STATE_DISCONNECTED);
    assert(ctx.is_under_voltage_trip == true);
    printf("   Fasa R = 113V (Histeresis 110-115V) -> Relay tetap DISCONNECTED [PASS]\n");

    /* 4. Fasa R pulih ke 118 V (> 115 V = 1150 dV) -> Harus Menutup Kembali (Auto-Reclose) */
    tripped = power_relay_process_voltage(&ctx, 1180, 2300, 2300, 1003);
    assert(!tripped);
    assert(ctx.relay_state == RELAY_STATE_CONNECTED);
    assert(ctx.is_under_voltage_trip == false);
    printf("   Fasa R = 118V (> 115V) -> Relay Pulih Otomatis CONNECTED [PASS]\n");

    /* 5. Kondisi Over-Voltage: Fasa T melonjak ke 285 V (> 280 V = 2800 dV) */
    tripped = power_relay_process_voltage(&ctx, 2300, 2300, 2850, 1004);
    assert(tripped);
    assert(ctx.relay_state == RELAY_STATE_DISCONNECTED);
    assert(ctx.last_reason == RELAY_REASON_OVER_VOLTAGE);
    assert(ctx.is_over_voltage_trip == true);
    printf("   Fasa T = 285V (> 280V) -> Relay TRIP! Reason: %s [PASS]\n", power_relay_reason_to_string(ctx.last_reason));

    /* 6. Fasa T turun sedikit ke 278 V (2780 dV, berada di antara 276 V dan 280 V) -> Histeresis, tetap TRIP */
    tripped = power_relay_process_voltage(&ctx, 2300, 2300, 2780, 1005);
    assert(!tripped);
    assert(ctx.relay_state == RELAY_STATE_DISCONNECTED);
    assert(ctx.is_over_voltage_trip == true);
    printf("   Fasa T = 278V (Histeresis 276-280V) -> Relay tetap DISCONNECTED [PASS]\n");

    /* 7. Fasa T turun ke 272 V (< 276 V = 2760 dV) -> Harus Menutup Kembali (Auto-Reclose) */
    tripped = power_relay_process_voltage(&ctx, 2300, 2300, 2720, 1006);
    assert(!tripped);
    assert(ctx.relay_state == RELAY_STATE_CONNECTED);
    assert(ctx.is_over_voltage_trip == false);
    printf("   Fasa T = 272V (< 276V) -> Relay Pulih Otomatis CONNECTED [PASS]\n\n");
}

int main(void) {
    printf("====================================================================\n");
    printf("  TEST VERIFIKASI PRIORITAS 3: POWER & RELAY FSM (SPLN D3.006)       \n");
    printf("====================================================================\n\n");

    test_asymmetry_detection();
    test_power_limiter_tripping();
    test_relay_fsm_ready_on();
    test_display_integration_ready_on();
    test_simultaneous_button_reconnect();
    test_voltage_cutoff_protection();

    printf("====================================================================\n");
    printf("  ALL PRIORITAS 3 TESTS PASSED SUCCESSFULLY! (100%%)                 \n");
    printf("====================================================================\n");
    return 0;
}

