/**
 * @file test_e1_to_e2_integration.c
 * @brief Host Integration Test Suite: E1 Metrology -> E3 Measurement -> E2 DLMS/COSEM OBIS
 *
 * Menguji keterhubungan penuh dari:
 * 1. ADE9000 Mock & Metrology Calculation (E1)
 * 2. Pemetaan Struktur Data Meter (E3)
 * 3. Sinkronisasi Kamus OBIS & Respon Frame HDLC/APDU DLMS (E2)
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

/* E1 Metrology Headers */
#include "RegisterMap.h"
#include "Mock.h"
#include "Metrology.h"
#include "SPI.h"

/* E3 Display & Data Model Headers */
#include "display.h"

/* E2 DLMS/COSEM Headers */
#include "dlms_types.h"
#include "dlms_hdlc.h"
#include "dlms_association.h"
#include "dlms_apdu.h"
#include "dlms_obis.h"
#include "dlms_server.h"

int main(void) {
    printf("========================================================================\n");
    printf("   HOST INTEGRATION TEST: E1 (METROLOGY) -> E3 (MEAS) -> E2 (DLMS/OBIS) \n");
    printf("========================================================================\n\n");

    /* ========================================================================= */
    /* TAHAP 1: INISIALISASI MOCK & PERHITUNGAN METROLOGI E1                    */
    /* ========================================================================= */
    printf("[TAHAP 1] Inisialisasi Virtual ADE9000 & Konversi Metrologi E1...\n");
    ADE9000_Mock_Init();
    ADE9000_SPI_Init();

    /* Set register ADE9000 Mock */
    ADE9000_Mock_SetAIRMS(5962491);       /* Arus Fasa A = 11.946 A */
    ADE9000_Mock_SetAVRMS(17136895);      /* Tegangan Fasa A = 230.020 V */
    ADE9000_Mock_SetAWATT(636984);        /* Daya Fasa A = 2299.3 W */
    ADE9000_Mock_SetAPERIOD(20000);       /* Frekuensi Grid = 50.00 Hz */
    ADE9000_Mock_SetNIRMS(1192498);       /* Arus Netral = 2.389 A */

    /* Baca via SPI Driver E1 */
    uint32_t airms = ADE9000_SPI_ReadRegister(ADE9000_AIRMS);
    uint32_t avrms = ADE9000_SPI_ReadRegister(ADE9000_AVRMS);
    int32_t  awatt = (int32_t)ADE9000_SPI_ReadRegister(ADE9000_AWATT);
    uint32_t aperiod = ADE9000_SPI_ReadRegister(ADE9000_APERIOD);
    uint32_t nirms = ADE9000_SPI_ReadRegister(ADE9000_NIRMS);

    /* Hitung engineering value */
    float va = Metrology_VoltageFromAVRMS(avrms);
    float ia = Metrology_CurrentFromAIRMS(airms);
    float in = Metrology_CurrentFromNIRMS(nirms);
    float pa = Metrology_PowerFromAWATT(awatt);
    float p_total = pa * 3.0f; /* 3-fasa seimbang = 6898 W */
    float freq = Metrology_FrequencyFromPeriod(aperiod);

    printf("  > Raw AVRMS: %u -> Voltage V_A: %.2f V\n", avrms, va);
    printf("  > Raw AIRMS: %u -> Current I_A: %.3f A\n", airms, ia);
    printf("  > Raw NIRMS: %u -> Current I_N: %.3f A\n", nirms, in);
    printf("  > Raw AWATT: %d -> Active Power Total: %.1f W\n", awatt, p_total);
    printf("  > Raw APERIOD: %u -> Frequency: %.2f Hz\n", aperiod, freq);

    /* ========================================================================= */
    /* TAHAP 2: PEMETAAN KE STRUKTUR PENGUKURAN E3 (METER MEASUREMENTS)          */
    /* ========================================================================= */
    printf("\n[TAHAP 2] Memetakan Data ke Struktur meter_measurements_t E3...\n");
    meter_measurements_t meas;
    memset(&meas, 0, sizeof(meas));

    meas.voltage_r_dvolts = (uint32_t)(va * 10.0f);   /* 2300 deci-Volt (230.0 V) */
    meas.voltage_s_dvolts = (uint32_t)(va * 10.0f);
    meas.voltage_t_dvolts = (uint32_t)(va * 10.0f);

    meas.current_r_mamps  = (uint32_t)(ia * 1000.0f); /* 11946 mA (11.946 A) */
    meas.current_s_mamps  = (uint32_t)(ia * 1000.0f);
    meas.current_t_mamps  = (uint32_t)(ia * 1000.0f);
    meas.current_n_mamps  = (uint32_t)(in * 1000.0f); /* 2389 mA (2.389 A) */

    meas.active_power_w   = (int32_t)p_total;         /* 6898 W */
    meas.apparent_power_va = (uint32_t)(va * ia * 3.0f);
    meas.power_factor_ppm  = 1000;                    /* 1.000 PF */
    meas.frequency_mhz    = (uint16_t)(freq * 1000.0f); /* 50000 mHz (50.00 Hz) */
    meas.active_energy_wh = 125430ULL;                /* 125430 Wh = 125.43 kWh */

    /* ========================================================================= */
    /* TAHAP 3: SINKRONISASI DATA KE TABEL OBIS DLMS E2                         */
    /* ========================================================================= */
    printf("\n[TAHAP 3] Menyinkronkan Data Pengukuran ke Kamus OBIS E2...\n");
    dlms_obis_update_from_meter(&meas);
    printf("  > Sukses memperbarui kamus OBIS dengan data dinamis E1!\n");

    /* ========================================================================= */
    /* TAHAP 4: SIMULASI TRANSAKSI DLMS/COSEM CLIENT -> SERVER                   */
    /* ========================================================================= */
    printf("\n[TAHAP 4] Menjalankan Transaksi Protokol DLMS/COSEM (HDLC & APDU)...\n");
    dlms_server_t server;
    assert(dlms_server_init(&server) == DLMS_OK);
    assert(server.ctx.current_state == DLMS_STATE_UNASSOCIATED);

    uint8_t tx_buf[512];
    size_t  tx_len = 0;

    /* 4A. Transaksi SNRM (HDLC Connection Setup) */
    printf("  [4A] Mengirim Frame HDLC SNRM (Client SAP 0x10 -> Server SAP 0x01)...\n");
    uint8_t snrm_frame[128];
    size_t snrm_len = dlms_hdlc_encode_frame(HDLC_CTRL_SNRM, DLMS_SERVER_SAP_MANAGEMENT, DLMS_CLIENT_SAP_PUBLIC, NULL, 0, snrm_frame, sizeof(snrm_frame));
    assert(snrm_len > 0);

    assert(dlms_server_process_bytes(&server, snrm_frame, snrm_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(tx_len > 0);
    assert(tx_buf[0] == HDLC_FLAG);
    printf("       -> Server merespons: Frame HDLC UA [PASS]\n");

    /* 4B. Transaksi AARQ (Association Request) */
    printf("  [4B] Mengirim Frame DLMS AARQ (Sesi Public Read-Only)...\n");
    uint8_t aarq_apdu[] = {0x60, 0x04, 0x00, 0x10};
    uint8_t aarq_hdlc[256];
    size_t aarq_hdlc_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, DLMS_SERVER_SAP_MANAGEMENT, DLMS_CLIENT_SAP_PUBLIC, aarq_apdu, sizeof(aarq_apdu), aarq_hdlc, sizeof(aarq_hdlc));

    assert(dlms_server_process_bytes(&server, aarq_hdlc, aarq_hdlc_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(tx_len > 0);
    assert(server.ctx.current_state == DLMS_STATE_ASSOCIATED_READONLY);
    printf("       -> Server merespons: Frame DLMS AARE (State: ASSOCIATED_READONLY) [PASS]\n");

    /* 4C. GET-Request: OBIS Tegangan Fasa A (1.0.32.7.0.255) */
    printf("  [4C] GET-Request: OBIS 1.0.32.7.0.255 (Tegangan Fasa A)...\n");
    uint8_t get_v_apdu[] = {
        0xC0, 0x01, 0x81, 0x00, 0x03, 1, 0, 32, 7, 0, 255, 0x02
    };
    uint8_t get_v_hdlc[256];
    size_t get_v_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, DLMS_SERVER_SAP_MANAGEMENT, DLMS_CLIENT_SAP_PUBLIC, get_v_apdu, sizeof(get_v_apdu), get_v_hdlc, sizeof(get_v_hdlc));

    assert(dlms_server_process_bytes(&server, get_v_hdlc, get_v_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(tx_len > 0);

    uint8_t res_ctrl = 0;
    uint16_t res_dest = 0;
    uint8_t res_src = 0;
    uint8_t res_apdu[256];
    size_t res_apdu_len = 0;
    assert(dlms_hdlc_decode_frame(tx_buf, tx_len, &res_ctrl, &res_dest, &res_src, res_apdu, &res_apdu_len) == DLMS_OK);
    assert(res_apdu[0] == DLMS_TAG_GET_RESPONSE);
    assert(res_apdu[4] == DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED);

    uint32_t val_v = ((uint32_t)res_apdu[5] << 24) | ((uint32_t)res_apdu[6] << 16) | ((uint32_t)res_apdu[7] << 8) | (uint32_t)res_apdu[8];
    printf("       -> Respon DLMS: %u (Skala 0.1V -> %.1f V) [PASS]\n", val_v, val_v / 10.0f);
    assert(val_v == 2300); /* Harus sama persis dengan E1 (230.0 V) */

    /* 4D. GET-Request: OBIS Arus Fasa A (1.0.31.7.0.255) */
    printf("  [4D] GET-Request: OBIS 1.0.31.7.0.255 (Arus Fasa A)...\n");
    uint8_t get_i_apdu[] = {
        0xC0, 0x01, 0x82, 0x00, 0x03, 1, 0, 31, 7, 0, 255, 0x02
    };
    size_t get_i_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, DLMS_SERVER_SAP_MANAGEMENT, DLMS_CLIENT_SAP_PUBLIC, get_i_apdu, sizeof(get_i_apdu), get_v_hdlc, sizeof(get_v_hdlc));

    assert(dlms_server_process_bytes(&server, get_v_hdlc, get_i_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(dlms_hdlc_decode_frame(tx_buf, tx_len, &res_ctrl, &res_dest, &res_src, res_apdu, &res_apdu_len) == DLMS_OK);
    assert(res_apdu[0] == DLMS_TAG_GET_RESPONSE);
    assert(res_apdu[4] == DLMS_DATA_TYPE_LONG_UNSIGNED);

    uint32_t val_i = ((uint32_t)res_apdu[5] << 8) | (uint32_t)res_apdu[6];
    printf("       -> Respon DLMS: %u (Skala 0.01A -> %.2f A) [PASS]\n", val_i, val_i / 100.0f);
    assert(val_i == 1194); /* 11.94 A dari E1 (11.946 A) */

    /* 4E. GET-Request: OBIS Total Energi Aktif Impor +A (1.0.1.8.0.255) */
    printf("  [4E] GET-Request: OBIS 1.0.1.8.0.255 (Total Energi Aktif)...\n");
    uint8_t get_e_apdu[] = {
        0xC0, 0x01, 0x83, 0x00, 0x03, 1, 0, 1, 8, 0, 255, 0x02
    };
    size_t get_e_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, DLMS_SERVER_SAP_MANAGEMENT, DLMS_CLIENT_SAP_PUBLIC, get_e_apdu, sizeof(get_e_apdu), get_v_hdlc, sizeof(get_v_hdlc));

    assert(dlms_server_process_bytes(&server, get_v_hdlc, get_e_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(dlms_hdlc_decode_frame(tx_buf, tx_len, &res_ctrl, &res_dest, &res_src, res_apdu, &res_apdu_len) == DLMS_OK);
    assert(res_apdu[0] == DLMS_TAG_GET_RESPONSE);
    assert(res_apdu[4] == DLMS_DATA_TYPE_DOUBLE_LONG_UNSIGNED);

    uint32_t val_e = ((uint32_t)res_apdu[5] << 24) | ((uint32_t)res_apdu[6] << 16) | ((uint32_t)res_apdu[7] << 8) | (uint32_t)res_apdu[8];
    printf("       -> Respon DLMS: %u (Skala 0.01 kWh -> %.2f kWh) [PASS]\n", val_e, val_e / 100.0f);
    assert(val_e == 12543); /* 125.43 kWh sinkron dengan E1 & Layar OLED! */

    /* 4F. Injeksi Event Sabotase (Tamper) & GET-Request Tamper Event Log (0.0.99.98.0.255) */
    printf("  [4F] Injeksi Event Tamper & GET-Request OBIS 0.0.99.98.0.255 (Tamper Log Profile)...\n");
    assert(dlms_tamper_log_add_event(&server.tamper_log, 1700008888, DLMS_TAMPER_METER_COVER_OPEN, 1) == DLMS_OK);
    dlms_obis_increment_tamper_counter(DLMS_TAMPER_METER_COVER_OPEN);

    uint8_t get_tamper_apdu[] = {
        0xC0, 0x01, 0x84, 0x00, 0x07, 0, 0, 99, 98, 0, 255, 0x02
    };
    size_t get_t_len = dlms_hdlc_encode_frame(HDLC_CTRL_I, DLMS_SERVER_SAP_MANAGEMENT, DLMS_CLIENT_SAP_PUBLIC, get_tamper_apdu, sizeof(get_tamper_apdu), get_v_hdlc, sizeof(get_v_hdlc));
    assert(dlms_server_process_bytes(&server, get_v_hdlc, get_t_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(dlms_hdlc_decode_frame(tx_buf, tx_len, &res_ctrl, &res_dest, &res_src, res_apdu, &res_apdu_len) == DLMS_OK);
    assert(res_apdu[0] == DLMS_TAG_GET_RESPONSE);
    assert(res_apdu[4] == DLMS_DATA_TYPE_OCTET_STRING);
    assert(res_apdu[5] == 1); /* 1 Record */
    uint32_t t_ts = ((uint32_t)res_apdu[6] << 24) | ((uint32_t)res_apdu[7] << 16) | ((uint32_t)res_apdu[8] << 8) | (uint32_t)res_apdu[9];
    assert(t_ts == 1700008888);
    assert(res_apdu[10] == DLMS_TAMPER_METER_COVER_OPEN);
    assert(res_apdu[11] == 1);
    printf("       -> Respon DLMS: Log Tamper Berhasil Dibaca (Time: %u, Code: 0x%02X, Status: %d) [PASS]\n", t_ts, res_apdu[10], res_apdu[11]);

    /* 4G. Transaksi DISC (Pelepasan Sesi) */
    printf("  [4G] Mengirim Frame HDLC DISC (Disconnect)...\n");
    uint8_t disc_frame[128];
    size_t disc_len = dlms_hdlc_encode_frame(HDLC_CTRL_DISC, DLMS_SERVER_SAP_MANAGEMENT, DLMS_CLIENT_SAP_PUBLIC, NULL, 0, disc_frame, sizeof(disc_frame));
    assert(dlms_server_process_bytes(&server, disc_frame, disc_len, tx_buf, sizeof(tx_buf), &tx_len) == DLMS_OK);
    assert(server.ctx.current_state == DLMS_STATE_UNASSOCIATED);
    printf("       -> Server kembali ke State: UNASSOCIATED [PASS]\n");

    printf("\n========================================================================\n");
    printf("   SELURUH RANGKAIAN TEST INTEGRASI E1 -> E3 -> E2: 100%% BERHASIL!      \n");
    printf("========================================================================\n");
    return 0;
}
