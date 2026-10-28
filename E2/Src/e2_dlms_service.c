/**
 * @file e2_dlms_service.c
 * @brief Implementasi Layanan Terpadu Pilar E2: Protokol DLMS/COSEM, Port Optik (USART3) & Modem (USART2)
 */

#include "e2_dlms_service.h"
#include "e1_metrology_service.h"
#include "e3_meter_app.h"
#include "ami_obis_catalog.h"
#include <string.h>

#define CLI_RX_FIFO_SIZE 64U
static volatile uint8_t s_cli_rx_fifo[CLI_RX_FIFO_SIZE];
static volatile uint8_t s_cli_rx_head = 0U;
static volatile uint8_t s_cli_rx_tail = 0U;

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

typedef struct {
    UART_HandleTypeDef  *huart;
    uint8_t              rx_byte;
    HdlcContext          hdlc_ctx;
    HdlcFrame            rx_frame;
    AmiDlmsAssociation   assoc;
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
    snapshot->active_export_wh     = 0ULL;

    const ReactiveEnergyQuadrants_t *quads = e1_metrology_get_reactive_quadrants();
    if (quads != NULL)
    {
        snapshot->reactive_import_varh = (uint64_t)(quads->q1_varh + quads->q2_varh);
        snapshot->reactive_export_varh = (uint64_t)(quads->q3_varh + quads->q4_varh);
    }
    else
    {
        snapshot->reactive_import_varh = meas->reactive_energy_varh;
        snapshot->reactive_export_varh = 0ULL;
    }

    /* 2. Sinkronisasi Clock COSEM Class 8 langsung dari RTC Epoch Aktif */
    uint32_t now_epoch = e3_meter_app_get_epoch();
    uint16_t yr = 2026U;
    uint8_t mo = 10U, dy = 28U, dow = 3U;
    dlms_epoch_to_civil(now_epoch, &yr, &mo, &dy, &dow);
    snapshot->clock_date_time.year[0] = (uint8_t)(yr >> 8);
    snapshot->clock_date_time.year[1] = (uint8_t)(yr & 0xFFU);
    snapshot->clock_date_time.month   = mo;
    snapshot->clock_date_time.day     = dy;
    snapshot->clock_date_time.day_of_week = dow;
    uint32_t sec_of_day = now_epoch % 86400UL;
    snapshot->clock_date_time.hour    = (uint8_t)((sec_of_day / 3600UL) % 24UL);
    snapshot->clock_date_time.minute  = (uint8_t)((sec_of_day / 60UL) % 60UL);
    snapshot->clock_date_time.second  = (uint8_t)(sec_of_day % 60UL);
    snapshot->clock_date_time.deviation[0] = 0x01U; /* +420 menit (WIB) */
    snapshot->clock_date_time.deviation[1] = 0xA4U;

    snapshot->valid_mask = AMI_COSEM_VALID_VOLTAGE_A | AMI_COSEM_VALID_VOLTAGE_B |
                           AMI_COSEM_VALID_VOLTAGE_C | AMI_COSEM_VALID_CURRENT_A |
                           AMI_COSEM_VALID_CURRENT_B | AMI_COSEM_VALID_CURRENT_C |
                           AMI_COSEM_VALID_ACTIVE_IMPORT | AMI_COSEM_VALID_ACTIVE_EXPORT |
                           AMI_COSEM_VALID_REACTIVE_IMPORT | AMI_COSEM_VALID_REACTIVE_EXPORT |
                           AMI_COSEM_VALID_CLOCK;

    /* 3. Isi Katalog Sampel Pengukuran Tambahan SPLN D3.006 (Frekuensi, IN, PF, Daya) */
    snapshot->measurement_sample_count = 0U;
    static const uint8_t obis_freq[6]   = AMI_OBIS_FREQUENCY;
    static const uint8_t obis_in[6]     = AMI_OBIS_CURRENT_NEUTRAL;
    static const uint8_t obis_pf[6]     = AMI_OBIS_POWER_FACTOR_TOTAL;
    static const uint8_t obis_p_imp[6]  = AMI_OBIS_ACTIVE_POWER_IMPORT_TOTAL;
    static const uint8_t obis_q_imp[6]  = AMI_OBIS_REACTIVE_POWER_IMPORT_TOTAL;
    static const uint8_t obis_s_imp[6]  = AMI_OBIS_APPARENT_POWER_IMPORT_TOTAL;

    add_measurement_sample(snapshot, obis_freq,  (uint64_t)(meas->frequency_mhz / 10U), 0);
    add_measurement_sample(snapshot, obis_in,    (uint64_t)meas->current_n_mamps, 0);
    add_measurement_sample(snapshot, obis_pf,    (uint64_t)meas->power_factor_ppm, (int32_t)meas->power_factor_ppm);
    add_measurement_sample(snapshot, obis_p_imp, (uint64_t)((meas->active_power_w > 0) ? meas->active_power_w : 0), meas->active_power_w);
    add_measurement_sample(snapshot, obis_q_imp, (uint64_t)((meas->reactive_power_var > 0) ? meas->reactive_power_var : 0), meas->reactive_power_var);
    add_measurement_sample(snapshot, obis_s_imp, (uint64_t)meas->apparent_power_va, (int32_t)meas->apparent_power_va);
}

static void process_channel_rx(DlmsChannelState *ch)
{
    uint8_t tx_buf[AMI_HDLC_MAX_FRAME_BYTES];
    uint8_t apdu_resp[AMI_COSEM_AXDR_ENCODED_MAX];
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
                                          tx_buf, sizeof(tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
            {
                HAL_UART_Transmit(ch->huart, tx_buf, (uint16_t)tx_len, 100);
            }
        }
        else if (ch->rx_frame.type == AMI_HDLC_FRAME_RR)
        {
            if (ami_dlms_hdlc_build_frame(&ch->hdlc_ctx, &ch->rx_frame, NULL, 0,
                                          tx_buf, sizeof(tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
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
                                          apdu_resp, sizeof(apdu_resp), &apdu_resp_len) == AMI_DLMS_ASSOC_OK)
                {
                    if (ami_dlms_hdlc_build_frame(&ch->hdlc_ctx, &ch->rx_frame,
                                                  apdu_resp, apdu_resp_len,
                                                  tx_buf, sizeof(tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
                    {
                        HAL_UART_Transmit(ch->huart, tx_buf, (uint16_t)tx_len, 100);
                    }
                }
            }
            else if (tag == 0xC0U) /* GET-Request */
            {
                if (ami_dlms_process_get_request(&ch->assoc, &s_global_dlms_snapshot,
                                                 ch->rx_frame.payload, ch->rx_frame.payload_length,
                                                 apdu_resp, sizeof(apdu_resp), &apdu_resp_len) == AMI_DLMS_APDU_OK)
                {
                    if (ami_dlms_hdlc_build_frame(&ch->hdlc_ctx, &ch->rx_frame,
                                                  apdu_resp, apdu_resp_len,
                                                  tx_buf, sizeof(tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
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
                                              tx_buf, sizeof(tx_buf), &tx_len) == AMI_HDLC_FRAME_READY && tx_len > 0)
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
