/**
 * @file e2_dlms_service.c
 * @brief Implementasi Layanan Terpadu Pilar E2: Protokol DLMS/COSEM, Port Optik (USART3) & Modem (USART2)
 */

#include "e2_dlms_service.h"
#include "e1_metrology_service.h"
#include "e3_meter_app.h"
#include "ami_obis_catalog.h"
#include "ami_dlms_cipher.h"
#include "ami_dlms_association.h"
#include "ami_disconnect_control.h"
#include <string.h>

#define CLI_RX_FIFO_SIZE 64U
static volatile uint8_t s_cli_rx_fifo[CLI_RX_FIFO_SIZE];
static volatile uint8_t s_cli_rx_head = 0U;
static volatile uint8_t s_cli_rx_tail = 0U;

static AmiDlmsSecuritySuite0Context s_cipher_ctx;
static AmiDlmsCipherBackend         s_cipher_backend;
static AmiDisconnectControlContext  s_disconnect_ctrl;
static bool                         s_cipher_initialized = false;

static int dlms_breaker_actuator(void *user, AmiBreakerCommand cmd)
{
    (void)user;
    if (cmd == AMI_BREAKER_CMD_OPEN)
    {
        return e3_meter_app_remote_disconnect() ? 0 : -1;
    }
    else if (cmd == AMI_BREAKER_CMD_CLOSE)
    {
        return e3_meter_app_remote_reconnect() ? 0 : -1;
    }
    return -1;
}

static void dlms_epoch_to_civil(uint32_t epoch, uint16_t *out_y, uint8_t *out_m, uint8_t *out_d, uint8_t *out_dow)
{
    uint32_t z = (epoch / 86400UL) + 719468UL;
    uint32_t era = z / 146097UL;
    uint32_t doe = z - era * 146097UL;
    uint32_t yoe = (doe - doe / 1460UL + doe / 36524UL - doe / 146096UL) / 365UL;
    uint32_t y = yoe + era * 400UL;
    uint32_t doy = doe - (365UL * yoe + yoe / 4UL - yoe / 100UL);
    uint32_t mp = (5UL * doy + 2UL) / 153UL;
    uint32_t d = doy - (153UL * mp + 2UL) / 5UL + 1UL;
    uint32_t m = (mp < 10UL) ? (mp + 3UL) : (mp - 9UL);
    if (m <= 2UL) y += 1UL;
    if (out_y) *out_y = (uint16_t)y;
    if (out_m) *out_m = (uint8_t)m;
    if (out_d) *out_d = (uint8_t)d;
    if (out_dow) *out_dow = (uint8_t)(((epoch / 86400UL) + 3UL) % 7UL + 1UL);
}

static void epoch_to_cosem_datetime(uint32_t epoch, AmiCosemDateTime *dt)
{
    if (dt == NULL) return;
    memset(dt, 0, sizeof(*dt));
    uint16_t yr = 2026U;
    uint8_t mo = 10U, dy = 28U, dow = 3U;
    dlms_epoch_to_civil(epoch, &yr, &mo, &dy, &dow);
    dt->year[0] = (uint8_t)(yr >> 8);
    dt->year[1] = (uint8_t)(yr & 0xFFU);
    dt->month   = mo;
    dt->day     = dy;
    dt->day_of_week = dow;
    uint32_t sec_of_day = epoch % 86400UL;
    dt->hour    = (uint8_t)((sec_of_day / 3600UL) % 24UL);
    dt->minute  = (uint8_t)((sec_of_day / 60UL) % 60UL);
    dt->second  = (uint8_t)(sec_of_day % 60UL);
    dt->deviation[0] = 0x01U; /* +420 menit (WIB) */
    dt->deviation[1] = 0xA4U;
}

typedef struct {
    UART_HandleTypeDef  *huart;
    uint8_t              rx_byte;
    HdlcContext          hdlc_ctx;
    HdlcFrame            rx_frame;
    AmiDlmsAssociation   assoc;
    uint8_t              tx_buf[AMI_HDLC_MAX_FRAME_BYTES];
    uint8_t              apdu_resp[AMI_COSEM_AXDR_ENCODED_MAX];
} DlmsChannelState;

static DlmsChannelState          s_optical_channel;
static DlmsChannelState          s_modem_channel;
static AmiCosemRegisterSnapshot  s_global_dlms_snapshot;

static void init_dlms_channel(DlmsChannelState *ch, UART_HandleTypeDef *huart)
{
    if (ch == NULL) return;
    memset(ch, 0, sizeof(*ch));
    ch->huart = huart;
    ami_dlms_hdlc_init(&ch->hdlc_ctx);
    ami_dlms_association_reset(&ch->assoc);
    if (ch->huart != NULL)
    {
        HAL_UART_Receive_IT(ch->huart, &ch->rx_byte, 1);
    }
}

static void add_measurement_sample(AmiCosemRegisterSnapshot *snapshot,
                                   const uint8_t obis[6],
                                   uint64_t raw_val,
                                   int32_t signed_val)
{
    if (snapshot->measurement_sample_count >= AMI_COSEM_REGISTER_SAMPLE_MAX) return;
    AmiCosemRegisterSample *s = &snapshot->measurement_samples[snapshot->measurement_sample_count++];
    memcpy(s->logical_name, obis, AMI_COSEM_LOGICAL_NAME_LENGTH);
    s->raw_value    = raw_val;
    s->signed_value = signed_val;
    s->valid        = 1u;
}

void e2_dlms_service_init(UART_HandleTypeDef *optical_huart)
{
    memset(&s_global_dlms_snapshot, 0, sizeof(s_global_dlms_snapshot));

    /* Set identitas Server System Title default ("ALLINMTR") */
    memcpy(s_global_dlms_snapshot.server_system_title, "ALLINMTR", 8);
    s_global_dlms_snapshot.clock_time_zone_minutes = 420; /* UTC+7 (WIB) */

    /* Inisialisasi Disconnect Control (Class 70) State Machine */
    ami_disconnect_control_init(&s_disconnect_ctrl, AMI_DISCONNECT_MODE_2, dlms_breaker_actuator, NULL);

    /* Inisialisasi Security Suite 0 Cipher Backend untuk sesi terenkripsi (HLS/glo-*) */
    static const uint8_t default_ek[16] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F};
    static const uint8_t default_ak[16] = {0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF};
    static const uint8_t client_title[8] = {'P','L','N','C','L','I','0','1'};
    static const uint8_t server_title[8] = {'A','L','L','I','N','M','T','R'};

    if (ami_dlms_cipher_suite0_init(&s_cipher_ctx, default_ek, default_ak, client_title, server_title, 1u) == 0)
    {
        if (ami_dlms_cipher_suite0_get_backend(&s_cipher_ctx, &s_cipher_backend) == 0)
        {
            s_cipher_backend.disconnect_control = &s_disconnect_ctrl;
            s_cipher_initialized = true;
        }
    }

    init_dlms_channel(&s_optical_channel, optical_huart);
}

void e2_dlms_service_set_modem_uart(UART_HandleTypeDef *modem_huart)
{
    init_dlms_channel(&s_modem_channel, modem_huart);
}

void e2_dlms_service_sync_snapshot(AmiCosemRegisterSnapshot *snapshot, const meter_measurements_t *meas)
{
    if (snapshot == NULL || meas == NULL) return;

    /* 1. Penskalaan dVolts (x0.1V) ke milliVolts (x0.001V) untuk objek OBIS DLMS */
    snapshot->phase_voltage_millivolts[0] = (uint32_t)meas->voltage_r_dvolts * 100U;
    snapshot->phase_voltage_millivolts[1] = (uint32_t)meas->voltage_s_dvolts * 100U;
    snapshot->phase_voltage_millivolts[2] = (uint32_t)meas->voltage_t_dvolts * 100U;

    snapshot->phase_current_milliamps[0] = meas->current_r_mamps;
    snapshot->phase_current_milliamps[1] = meas->current_s_mamps;
    snapshot->phase_current_milliamps[2] = meas->current_t_mamps;

    snapshot->active_import_wh     = meas->active_energy_wh;
    snapshot->active_export_wh     = meas->active_energy_import_wh;

    const ReactiveEnergyQuadrants_t *quads = e1_metrology_get_reactive_quadrants();
    if (quads != NULL)
    {
        snapshot->reactive_import_varh = (uint64_t)(quads->q1_varh + quads->q2_varh);
        snapshot->reactive_export_varh = (uint64_t)(quads->q3_varh + quads->q4_varh);
    }
    else
    {
        snapshot->reactive_import_varh = (meas->reactive_energy_exp_varh > 0ULL) ? meas->reactive_energy_exp_varh : meas->reactive_energy_varh;
        snapshot->reactive_export_varh = meas->reactive_energy_imp_varh;
    }

    /* 2. Sinkronisasi Clock COSEM Class 8 langsung dari RTC Epoch Aktif */
    uint32_t now_epoch = e3_meter_app_get_epoch();
    epoch_to_cosem_datetime(now_epoch, &snapshot->clock_date_time);

    snapshot->valid_mask = AMI_COSEM_VALID_VOLTAGE_A | AMI_COSEM_VALID_VOLTAGE_B |
                           AMI_COSEM_VALID_VOLTAGE_C | AMI_COSEM_VALID_CURRENT_A |
                           AMI_COSEM_VALID_CURRENT_B | AMI_COSEM_VALID_CURRENT_C |
                           AMI_COSEM_VALID_ACTIVE_IMPORT | AMI_COSEM_VALID_ACTIVE_EXPORT |
                           AMI_COSEM_VALID_REACTIVE_IMPORT | AMI_COSEM_VALID_REACTIVE_EXPORT |
                           AMI_COSEM_VALID_CLOCK;

    /* 3. Isi Katalog Sampel Pengukuran Tambahan SPLN D3.006 (Frekuensi, IN, PF, Daya, Kuadran & Tarif) */
    snapshot->measurement_sample_count = 0U;
    static const uint8_t obis_freq[6]   = AMI_OBIS_FREQUENCY;
    static const uint8_t obis_in[6]     = AMI_OBIS_CURRENT_NEUTRAL;
    static const uint8_t obis_pf[6]     = AMI_OBIS_POWER_FACTOR_TOTAL;
    static const uint8_t obis_p_imp[6]  = AMI_OBIS_ACTIVE_POWER_IMPORT_TOTAL;
    static const uint8_t obis_q_imp[6]  = AMI_OBIS_REACTIVE_POWER_IMPORT_TOTAL;
    static const uint8_t obis_s_imp[6]  = AMI_OBIS_APPARENT_POWER_IMPORT_TOTAL;

    /* Koreksi Frekuensi: scaler adalah -3 (10^-3 Hz), sehingga 50000 mHz terbaca 50.000 Hz */
    add_measurement_sample(snapshot, obis_freq,  (uint64_t)meas->frequency_mhz, 0);
    add_measurement_sample(snapshot, obis_in,    (uint64_t)meas->current_n_mamps, 0);
    add_measurement_sample(snapshot, obis_pf,    (uint64_t)meas->power_factor_ppm, (int32_t)meas->power_factor_ppm);
    add_measurement_sample(snapshot, obis_p_imp, (uint64_t)((meas->active_power_w > 0) ? meas->active_power_w : 0), meas->active_power_w);
    add_measurement_sample(snapshot, obis_q_imp, (uint64_t)((meas->reactive_power_var > 0) ? meas->reactive_power_var : 0), meas->reactive_power_var);
    add_measurement_sample(snapshot, obis_s_imp, (uint64_t)meas->apparent_power_va, (int32_t)meas->apparent_power_va);

    /* Energi Reaktif 4-Kuadran Total (1.0.5.8.0.255 s/d 1.0.8.8.0.255) */
    static const uint8_t obis_q1[6] = {1u, 0u, 5u, 8u, 0u, 255u};
    static const uint8_t obis_q2[6] = {1u, 0u, 6u, 8u, 0u, 255u};
    static const uint8_t obis_q3[6] = {1u, 0u, 7u, 8u, 0u, 255u};
    static const uint8_t obis_q4[6] = {1u, 0u, 8u, 8u, 0u, 255u};
    if (quads != NULL)
    {
        add_measurement_sample(snapshot, obis_q1, (uint64_t)quads->q1_varh, 0);
        add_measurement_sample(snapshot, obis_q2, (uint64_t)quads->q2_varh, 0);
        add_measurement_sample(snapshot, obis_q3, (uint64_t)quads->q3_varh, 0);
        add_measurement_sample(snapshot, obis_q4, (uint64_t)quads->q4_varh, 0);
    }

    /* Energi Aktif per Tarif Ekspor & Impor (1.0.1.8.1..3.255 & 1.0.2.8.1..3.255) */
    static const uint8_t obis_ea_exp_t1[6] = {1u, 0u, 1u, 8u, 1u, 255u};
    static const uint8_t obis_ea_exp_t2[6] = {1u, 0u, 1u, 8u, 2u, 255u};
    static const uint8_t obis_ea_exp_t3[6] = {1u, 0u, 1u, 8u, 3u, 255u};
    static const uint8_t obis_ea_imp_t1[6] = {1u, 0u, 2u, 8u, 1u, 255u};
    static const uint8_t obis_ea_imp_t2[6] = {1u, 0u, 2u, 8u, 2u, 255u};
    static const uint8_t obis_ea_imp_t3[6] = {1u, 0u, 2u, 8u, 3u, 255u};
    add_measurement_sample(snapshot, obis_ea_exp_t1, meas->active_energy_wbp_wh, 0);
    add_measurement_sample(snapshot, obis_ea_exp_t2, meas->active_energy_lwbp_wh, 0);
    add_measurement_sample(snapshot, obis_ea_exp_t3, meas->active_energy_lwbp2_wh, 0);
    add_measurement_sample(snapshot, obis_ea_imp_t1, meas->active_energy_imp_wbp_wh, 0);
    add_measurement_sample(snapshot, obis_ea_imp_t2, meas->active_energy_imp_lwbp1_wh, 0);
    add_measurement_sample(snapshot, obis_ea_imp_t3, meas->active_energy_imp_lwbp2_wh, 0);

    /* Faktor Daya per Fasa (1.0.33.7.0.255, 1.0.53.7.0.255, 1.0.73.7.0.255) */
    static const uint8_t obis_pf_r[6] = {1u, 0u, 33u, 7u, 0u, 255u};
    static const uint8_t obis_pf_s[6] = {1u, 0u, 53u, 7u, 0u, 255u};
    static const uint8_t obis_pf_t[6] = {1u, 0u, 73u, 7u, 0u, 255u};
    add_measurement_sample(snapshot, obis_pf_r, (uint64_t)meas->power_factor_ppm, (int32_t)meas->power_factor_ppm);
    add_measurement_sample(snapshot, obis_pf_s, (uint64_t)meas->power_factor_ppm, (int32_t)meas->power_factor_ppm);
    add_measurement_sample(snapshot, obis_pf_t, (uint64_t)meas->power_factor_ppm, (int32_t)meas->power_factor_ppm);

    /* 4. Sinkronisasi Event Log Tamper E3 ke Snapshot DLMS (OBIS 0.0.99.98.0.255) */
    const tamper_context_t *t_ctx = e3_meter_app_get_tamper_context();
    if (t_ctx != NULL && t_ctx->tamper_log_fifo.count > 0)
    {
        size_t n = t_ctx->tamper_log_fifo.count;
        if (n > AMI_COSEM_PROFILE_MAX_ENTRIES) n = AMI_COSEM_PROFILE_MAX_ENTRIES;
        snapshot->event_log_count = n;
        for (size_t i = 0; i < n; i++)
        {
            size_t idx = (t_ctx->tamper_log_fifo.head + SPLN_LOG_FIFO_MAX_ENTRIES - t_ctx->tamper_log_fifo.count + i) % SPLN_LOG_FIFO_MAX_ENTRIES;
            const spln_standard_log_entry_t *rec = &t_ctx->tamper_log_fifo.records[idx];
            epoch_to_cosem_datetime(rec->timestamp_epoch, &snapshot->event_log[i].timestamp);
            snapshot->event_log[i].event_code = (uint8_t)(rec->event_code & 0xFFU);
        }
        snapshot->valid_mask |= AMI_COSEM_VALID_EVENT_LOG;
    }

    /* 5. Sinkronisasi Histori Billing E3 ke Snapshot DLMS (OBIS 1.0.98.1.0.255) */
    const billing_snapshot_ctx_t *b_ctx = e3_meter_app_get_billing_snapshots();
    if (b_ctx != NULL && b_ctx->count > 0)
    {
        size_t n = b_ctx->count;
        if (n > AMI_COSEM_PROFILE_MAX_ENTRIES) n = AMI_COSEM_PROFILE_MAX_ENTRIES;
        snapshot->billing_history_count = n;
        for (size_t i = 0; i < n; i++)
        {
            size_t idx = (b_ctx->head + BILLING_MAX_SNAPSHOTS - b_ctx->count + i) % BILLING_MAX_SNAPSHOTS;
            const billing_snapshot_entry_t *b_rec = &b_ctx->entries[idx];
            epoch_to_cosem_datetime(b_rec->billing_epoch_sec, &snapshot->billing_history[i].billing_timestamp);
            snapshot->billing_history[i].active_import_wh = b_rec->active_energy_import_wh;
            snapshot->billing_history[i].active_export_wh = 0ULL;
        }
        snapshot->valid_mask |= AMI_COSEM_VALID_BILLING_PROFILE;
    }

    /* 6. Sinkronisasi Register Multi-Tarif SPLN (Tarif 1..4: WBP, LWBP1, LWBP2, T4) */
    snapshot->active_import_tariff_wh[0] = meas->active_energy_wbp_wh;
    snapshot->active_import_tariff_wh[1] = meas->active_energy_lwbp_wh;
    snapshot->active_import_tariff_wh[2] = meas->active_energy_lwbp2_wh;
    snapshot->active_import_tariff_wh[3] = 0ULL;

    snapshot->active_export_tariff_wh[0] = meas->active_energy_imp_wbp_wh;
    snapshot->active_export_tariff_wh[1] = meas->active_energy_imp_lwbp1_wh;
    snapshot->active_export_tariff_wh[2] = meas->active_energy_imp_lwbp2_wh;
    snapshot->active_export_tariff_wh[3] = 0ULL;

    snapshot->reactive_import_tariff_varh[0] = 0ULL;
    snapshot->reactive_import_tariff_varh[1] = 0ULL;
    snapshot->reactive_import_tariff_varh[2] = 0ULL;
    snapshot->reactive_import_tariff_varh[3] = 0ULL;

    /* 7. Sinkronisasi Kualitas Daya (Power Quality) & Parameter Sesaat */
    snapshot->frequency_mhz = (uint32_t)meas->frequency_mhz;
    snapshot->power_factor[0] = (int32_t)meas->power_factor_ppm;
    snapshot->power_factor[1] = (int32_t)meas->power_factor_ppm;
    snapshot->power_factor[2] = (int32_t)meas->power_factor_ppm;
    snapshot->power_factor[3] = (int32_t)meas->power_factor_ppm;
    snapshot->thd_voltage_percent_x10[0] = 15U;
    snapshot->thd_voltage_percent_x10[1] = 15U;
    snapshot->thd_voltage_percent_x10[2] = 15U;
    snapshot->thd_voltage_percent_x10[3] = 15U;
    snapshot->thd_current_percent_x10[0] = 25U;
    snapshot->thd_current_percent_x10[1] = 25U;
    snapshot->thd_current_percent_x10[2] = 25U;
    snapshot->thd_current_percent_x10[3] = 25U;
    snapshot->active_power_watts = (uint32_t)((meas->active_power_w > 0) ? meas->active_power_w : 0);
    snapshot->reactive_power_vars = (uint32_t)((meas->reactive_power_var > 0) ? meas->reactive_power_var : 0);

    /* 8. Informasi Perangkat Meter */
    const char *mid = e3_meter_app_get_meter_id();
    if (mid != NULL && mid[0] != '\0')
    {
        strncpy(snapshot->meter_id, mid, sizeof(snapshot->meter_id) - 1);
        snapshot->meter_id[sizeof(snapshot->meter_id) - 1] = '\0';
    }
    else
    {
        strncpy(snapshot->meter_id, "PLN1234567890", sizeof(snapshot->meter_id) - 1);
    }
    strncpy(snapshot->meter_type, "3-PHASE-AMI-POSTPAID", sizeof(snapshot->meter_type) - 1);

    /* 9. Sinkronisasi Status Disconnect Control (Class 70) */
    ami_disconnect_control_sync_snapshot(&s_disconnect_ctrl, snapshot);

    snapshot->valid_mask |= (AMI_COSEM_VALID_TARIFF_REGISTERS |
                             AMI_COSEM_VALID_POWER_QUALITY |
                             AMI_COSEM_VALID_DISCONNECT_CTRL |
                             AMI_COSEM_VALID_DEVICE_INFO);
}

static void process_channel_rx(DlmsChannelState *ch)
{
    uint8_t *tx_buf = ch->tx_buf;
    uint8_t *apdu_resp = ch->apdu_resp;
    size_t  tx_len = 0;
    size_t  apdu_resp_len = 0;

    /* Jika kanal USART2 sedang tidak mengoleksi frame HDLC (0x7E) dan menerima karakter ASCII CLI */
    if (ch == &s_modem_channel && ch->hdlc_ctx.collecting == 0U && ch->rx_byte != AMI_HDLC_FLAG)
    {
        uint8_t next_head = (uint8_t)((s_cli_rx_head + 1U) % CLI_RX_FIFO_SIZE);
        if (next_head != s_cli_rx_tail)
        {
            s_cli_rx_fifo[s_cli_rx_head] = ch->rx_byte;
            s_cli_rx_head = next_head;
        }
        HAL_UART_Receive_IT(ch->huart, &ch->rx_byte, 1);
        return;
    }

    AmiHdlcStatus status = ami_dlms_hdlc_parse_byte(&ch->hdlc_ctx, ch->rx_byte, &ch->rx_frame);
    if (status == AMI_HDLC_FRAME_READY)
    {
        if (ch->rx_frame.type == AMI_HDLC_FRAME_SNRM || ch->rx_frame.type == AMI_HDLC_FRAME_DISC)
        {
            ami_dlms_association_reset(&ch->assoc);
            if (ami_dlms_hdlc_build_frame(&ch->hdlc_ctx, &ch->rx_frame, NULL, 0,
                                          tx_buf, sizeof(ch->tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
            {
                HAL_UART_Transmit(ch->huart, tx_buf, (uint16_t)tx_len, 100);
            }
        }
        else if (ch->rx_frame.type == AMI_HDLC_FRAME_RR)
        {
            if (ami_dlms_hdlc_build_frame(&ch->hdlc_ctx, &ch->rx_frame, NULL, 0,
                                          tx_buf, sizeof(ch->tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
            {
                HAL_UART_Transmit(ch->huart, tx_buf, (uint16_t)tx_len, 100);
            }
        }
        else if (ch->rx_frame.type == AMI_HDLC_FRAME_I && ch->rx_frame.payload_length > 0)
        {
            uint8_t tag = ch->rx_frame.payload[0];
            if (tag == 0x60U) /* AARQ (Application Association Request) */
            {
                if (ami_dlms_process_aarq(&ch->assoc,
                                          ch->rx_frame.payload, ch->rx_frame.payload_length,
                                          apdu_resp, sizeof(ch->apdu_resp), &apdu_resp_len) == AMI_DLMS_ASSOC_OK)
                {
                    if (ami_dlms_hdlc_build_frame(&ch->hdlc_ctx, &ch->rx_frame,
                                                  apdu_resp, apdu_resp_len,
                                                  tx_buf, sizeof(ch->tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
                    {
                        HAL_UART_Transmit(ch->huart, tx_buf, (uint16_t)tx_len, 100);
                    }
                }
            }
            else if (tag == 0xC0U) /* GET-Request */
            {
                if (ami_dlms_process_get_request(&ch->assoc, &s_global_dlms_snapshot,
                                                 ch->rx_frame.payload, ch->rx_frame.payload_length,
                                                 apdu_resp, sizeof(ch->apdu_resp), &apdu_resp_len) == AMI_DLMS_APDU_OK)
                {
                    if (ami_dlms_hdlc_build_frame(&ch->hdlc_ctx, &ch->rx_frame,
                                                  apdu_resp, apdu_resp_len,
                                                  tx_buf, sizeof(ch->tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
                    {
                        HAL_UART_Transmit(ch->huart, tx_buf, (uint16_t)tx_len, 100);
                    }
                }
            }
            else if (tag == 0xC1U) /* SET-Request (plain unauthenticated SET is rejected per SPLN) */
            {
                /* SET-Request -> SetResponseNormal with read-write-denied (3) */
                apdu_resp[0] = 0xC5U;
                apdu_resp[1] = 0x01U;
                apdu_resp[2] = (ch->rx_frame.payload_length > 2U) ? ch->rx_frame.payload[2] : 0U;
                apdu_resp[3] = 0x03U;
                apdu_resp_len = 4U;

                if (ami_dlms_hdlc_build_frame(&ch->hdlc_ctx, &ch->rx_frame,
                                              apdu_resp, apdu_resp_len,
                                              tx_buf, sizeof(ch->tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
                {
                    HAL_UART_Transmit(ch->huart, tx_buf, (uint16_t)tx_len, 100);
                }
            }
            else if (tag == 0xC8U || tag == 0xC9U || tag == 0xCBU) /* glo-get (C8), glo-set (C9), glo-action (CB) */
            {
                static uint8_t cipher_ws[AMI_HDLC_MAX_INFO_LENGTH];
                (void)ami_dlms_process_ciphered_request(&ch->assoc, &s_global_dlms_snapshot,
                                                       s_cipher_initialized ? &s_cipher_backend : NULL,
                                                       ch->rx_frame.payload, ch->rx_frame.payload_length,
                                                       cipher_ws, sizeof(cipher_ws),
                                                       apdu_resp, sizeof(ch->apdu_resp), &apdu_resp_len);
                if (apdu_resp_len > 0)
                {
                    if (ami_dlms_hdlc_build_frame(&ch->hdlc_ctx, &ch->rx_frame,
                                                  apdu_resp, apdu_resp_len,
                                                  tx_buf, sizeof(ch->tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
                    {
                        HAL_UART_Transmit(ch->huart, tx_buf, (uint16_t)tx_len, 100);
                    }
                }
            }
            else if (tag == 0x62U) /* RLRQ (Release Request) */
            {
                ami_dlms_association_reset(&ch->assoc);
                static const uint8_t rlre_pdu[] = { 0x63U, 0x03U, 0x80U, 0x01U, 0x00U };
                if (ami_dlms_hdlc_build_frame(&ch->hdlc_ctx, &ch->rx_frame,
                                              rlre_pdu, sizeof(rlre_pdu),
                                              tx_buf, sizeof(ch->tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
                {
                    HAL_UART_Transmit(ch->huart, tx_buf, (uint16_t)tx_len, 100);
                }
            }
        }
    }

    HAL_UART_Receive_IT(ch->huart, &ch->rx_byte, 1);
}

void e2_dlms_service_uart_rx_callback(UART_HandleTypeDef *huart)
{
    if (huart == NULL) return;

    if (s_optical_channel.huart != NULL && huart->Instance == s_optical_channel.huart->Instance)
    {
        process_channel_rx(&s_optical_channel);
    }
    else if (s_modem_channel.huart != NULL && huart->Instance == s_modem_channel.huart->Instance)
    {
        process_channel_rx(&s_modem_channel);
    }
}

bool e2_dlms_send_last_gasp_push(uint32_t timestamp_epoch, uint64_t active_import_wh)
{
    /* Bangun APDU DLMS Event-Notification-Request (Tag 0xC2) untuk Last-Gasp Power Down (Kode Event 0x50) */
    uint8_t event_apdu[32];
    size_t  pos = 0;

    event_apdu[pos++] = 0xE6U; /* LLC Destination LSAP */
    event_apdu[pos++] = 0xE7U; /* LLC Source LSAP */
    event_apdu[pos++] = 0x00U; /* LLC Quality */
    event_apdu[pos++] = 0xC2U; /* DLMS Event-Notification-Request APDU Tag */
    event_apdu[pos++] = 0x00U; /* Optional time absent (encoded in attribute value below) */
    /* Class ID: 0x0001 (Data), OBIS: 0.0.97.98.0.255 (Alarm/Event Register), Attr: 0x02 */
    event_apdu[pos++] = 0x00U; event_apdu[pos++] = 0x01U;
    event_apdu[pos++] = 0U;    event_apdu[pos++] = 0U;    event_apdu[pos++] = 97U;
    event_apdu[pos++] = 98U;   event_apdu[pos++] = 0U;    event_apdu[pos++] = 255U;
    event_apdu[pos++] = 0x02U;
    /* Data Structure (2 elements: epoch timestamp + active energy Wh) */
    event_apdu[pos++] = 0x02U; event_apdu[pos++] = 0x02U;
    event_apdu[pos++] = 0x06U; /* double-long-unsigned (epoch) */
    event_apdu[pos++] = (uint8_t)(timestamp_epoch >> 24);
    event_apdu[pos++] = (uint8_t)(timestamp_epoch >> 16);
    event_apdu[pos++] = (uint8_t)(timestamp_epoch >> 8);
    event_apdu[pos++] = (uint8_t)(timestamp_epoch);
    event_apdu[pos++] = 0x06U; /* double-long-unsigned (active_import_wh low 32-bit) */
    uint32_t wh32 = (uint32_t)active_import_wh;
    event_apdu[pos++] = (uint8_t)(wh32 >> 24);
    event_apdu[pos++] = (uint8_t)(wh32 >> 16);
    event_apdu[pos++] = (uint8_t)(wh32 >> 8);
    event_apdu[pos++] = (uint8_t)(wh32);

    uint8_t hdlc_buf[64];
    size_t  hdlc_len = 0;
    if (ami_dlms_hdlc_encode_frame(0x10U, 0x01U, 0x13U /* UI Frame */,
                                   event_apdu, pos,
                                   hdlc_buf, sizeof(hdlc_buf), &hdlc_len) != AMI_HDLC_FRAME_READY)
    {
        return false;
    }

    UART_HandleTypeDef *target_uart = (s_modem_channel.huart != NULL) ? s_modem_channel.huart : s_optical_channel.huart;
    if (target_uart != NULL && hdlc_len > 0)
    {
        HAL_UART_Transmit(target_uart, hdlc_buf, (uint16_t)hdlc_len, 50);
        return true;
    }
    return false;
}

AmiCosemRegisterSnapshot *e2_dlms_service_get_snapshot(void)
{
    return &s_global_dlms_snapshot;
}

bool e2_dlms_service_pop_cli_rx_byte(uint8_t *out_byte)
{
    if (out_byte == NULL || s_cli_rx_head == s_cli_rx_tail)
    {
        return false;
    }
    *out_byte = s_cli_rx_fifo[s_cli_rx_tail];
    s_cli_rx_tail = (uint8_t)((s_cli_rx_tail + 1U) % CLI_RX_FIFO_SIZE);
    return true;
}
