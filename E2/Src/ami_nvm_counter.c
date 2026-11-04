#include "ami_nvm_counter.h"
#include <string.h>

#if defined(STM32U585xx)
#include "stm32u5xx_hal.h"
#endif

uint32_t ami_nvm_calculate_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFu;
    if (data == NULL) return 0u;

    for (size_t i = 0u; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1u) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

static uint32_t compute_record_crc(const AmiNvmCounterRecord *rec)
{
    const size_t crc_payload_len = offsetof(AmiNvmCounterRecord, crc32);
    return ami_nvm_calculate_crc32((const uint8_t *)rec, crc_payload_len);
}

static int commit_record(AmiNvmCounterContext *ctx, uint32_t tx_ic, uint32_t rx_ic)
{
    if (ctx == NULL || ctx->storage.write_record == NULL) return -1;

    AmiNvmCounterRecord rec;
    rec.magic = AMI_NVM_RECORD_MAGIC;
    rec.version = AMI_NVM_RECORD_VERSION;
    rec.tx_invocation_counter = tx_ic;
    rec.rx_invocation_counter = rx_ic;
    rec.reboot_count = ctx->reboot_count;
    rec.crc32 = compute_record_crc(&rec);

    return ctx->storage.write_record(ctx->storage.user_data, &rec);
}

int ami_nvm_counter_init(AmiNvmCounterContext *ctx,
                         const AmiNvmStorageDriver *storage,
                         uint32_t tx_window_step,
                         uint32_t rx_window_step)
{
    if (ctx == NULL || storage == NULL ||
        storage->read_record == NULL || storage->write_record == NULL) {
        return -1;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->storage = *storage;
    ctx->tx_window_step = (tx_window_step > 0u) ? tx_window_step : AMI_NVM_DEFAULT_TX_WINDOW_STEP;
    ctx->rx_window_step = (rx_window_step > 0u) ? rx_window_step : AMI_NVM_DEFAULT_RX_WINDOW_STEP;

    AmiNvmCounterRecord rec;
    memset(&rec, 0, sizeof(rec));
    int read_res = ctx->storage.read_record(ctx->storage.user_data, &rec);
    int is_valid = (read_res == 0 &&
                    rec.magic == AMI_NVM_RECORD_MAGIC &&
                    rec.version == AMI_NVM_RECORD_VERSION &&
                    rec.crc32 == compute_record_crc(&rec));

    if (!is_valid) {
        /* Format fresh record on uninitialized or corrupt NVM */
        ctx->reboot_count = 1u;
        ctx->current_tx_counter = 1u;
        ctx->persisted_tx_limit = 1u + ctx->tx_window_step;
        ctx->last_rx_counter = 0u;
        ctx->persisted_rx_counter = 0u;

        if (commit_record(ctx, ctx->persisted_tx_limit, ctx->persisted_rx_counter) != 0) {
            return -2;
        }
    } else {
        /* Valid persistent record found */
        ctx->reboot_count = rec.reboot_count + 1u;
        ctx->current_tx_counter = rec.tx_invocation_counter;
        /* Commit next lease window immediately to prevent nonce reuse across reboots */
        ctx->persisted_tx_limit = rec.tx_invocation_counter + ctx->tx_window_step;
        ctx->last_rx_counter = rec.rx_invocation_counter;
        ctx->persisted_rx_counter = rec.rx_invocation_counter;

        if (commit_record(ctx, ctx->persisted_tx_limit, ctx->persisted_rx_counter) != 0) {
            return -3;
        }
    }

    ctx->is_initialized = 1u;
    return 0;
}

int ami_nvm_counter_next_tx(AmiNvmCounterContext *ctx, uint32_t *out_tx_ic)
{
    if (ctx == NULL || !ctx->is_initialized || out_tx_ic == NULL) {
        return 0;
    }

    /* Guard against 32-bit counter overflow (NIST SP 800-38D nonce exhaustion) */
    if (ctx->current_tx_counter >= 0xFFFFFFFEu) {
        return 0; /* Fail-closed: counter space exhausted */
    }

    /* Advance TX counter */
    ctx->current_tx_counter++;

    /* If we have reached or exceeded the persisted lease window, advance window */
    if (ctx->current_tx_counter >= ctx->persisted_tx_limit) {
        if (ctx->persisted_tx_limit > 0xFFFFFFFFu - ctx->tx_window_step) {
            ctx->persisted_tx_limit = 0xFFFFFFFFu;
        } else {
            ctx->persisted_tx_limit += ctx->tx_window_step;
        }
        if (commit_record(ctx, ctx->persisted_tx_limit, ctx->persisted_rx_counter) != 0) {
            return 0; /* Fail-closed on storage failure */
        }
    }

    *out_tx_ic = ctx->current_tx_counter;
    return 1;
}

int ami_nvm_counter_accept_rx(AmiNvmCounterContext *ctx, uint32_t in_rx_ic)
{
    if (ctx == NULL || !ctx->is_initialized) {
        return 0;
    }

    /* Anti-replay: Incoming counter must be strictly greater than last accepted */
    if (in_rx_ic <= ctx->last_rx_counter) {
        return 0; /* Replay detected -> Reject */
    }

    /* Check if periodic high-water mark commit threshold reached */
    if (in_rx_ic >= ctx->persisted_rx_counter + ctx->rx_window_step) {
        if (commit_record(ctx, ctx->persisted_tx_limit, in_rx_ic) != 0) {
            return 0; /* Fail-closed: do not accept APDU if persistent record write fails */
        }
        ctx->persisted_rx_counter = in_rx_ic;
    }

    ctx->last_rx_counter = in_rx_ic;
    return 1;
}

int ami_nvm_counter_flush(AmiNvmCounterContext *ctx)
{
    if (ctx == NULL || !ctx->is_initialized) {
        return -1;
    }

    /* Write exact latest runtime counters to storage */
    ctx->persisted_rx_counter = ctx->last_rx_counter;
    return commit_record(ctx, ctx->persisted_tx_limit, ctx->persisted_rx_counter);
}

/* ========================================================================= */
/* RAM-backed Mock Driver (for Testing / Simulation)                        */
/* ========================================================================= */

static int ram_read_record(void *user_data, AmiNvmCounterRecord *record)
{
    if (user_data == NULL || record == NULL) return -1;
    memcpy(record, user_data, sizeof(AmiNvmCounterRecord));
    return 0;
}

static int ram_write_record(void *user_data, const AmiNvmCounterRecord *record)
{
    if (user_data == NULL || record == NULL) return -1;
    memcpy(user_data, record, sizeof(AmiNvmCounterRecord));
    return 0;
}

void ami_nvm_create_ram_driver(AmiNvmStorageDriver *out_driver,
                               AmiNvmCounterRecord *ram_storage)
{
    if (out_driver == NULL) return;
    out_driver->read_record = ram_read_record;
    out_driver->write_record = ram_write_record;
    out_driver->user_data = ram_storage;
}

/* ========================================================================= */
/* Hardware TAMP Backup Register Driver (STM32 Target)                       */
/* ========================================================================= */

#if defined(TAMP) && defined(TAMP_BKP0R)
static int tamp_read_record(void *user_data, AmiNvmCounterRecord *record)
{
    (void)user_data;
    if (record == NULL) return -1;

    record->magic                  = TAMP->BKP0R;
    record->version                = (TAMP->BKP1R >> 16) & 0xFFFFu;
    record->reboot_count           = TAMP->BKP1R & 0xFFFFu;
    record->tx_invocation_counter  = TAMP->BKP2R;
    record->rx_invocation_counter  = TAMP->BKP3R;
    record->crc32                  = TAMP->BKP4R;
    return 0;
}

static int tamp_write_record(void *user_data, const AmiNvmCounterRecord *record)
{
    (void)user_data;
    if (record == NULL) return -1;

    /* Write backup registers */
    TAMP->BKP0R = record->magic;
    TAMP->BKP1R = ((record->version & 0xFFFFu) << 16) | (record->reboot_count & 0xFFFFu);
    TAMP->BKP2R = record->tx_invocation_counter;
    TAMP->BKP3R = record->rx_invocation_counter;
    TAMP->BKP4R = record->crc32;
    return 0;
}

void ami_nvm_create_tamp_backup_driver(AmiNvmStorageDriver *out_driver)
{
    if (out_driver == NULL) return;
    out_driver->read_record = tamp_read_record;
    out_driver->write_record = tamp_write_record;
    out_driver->user_data = NULL;
}
#else
void ami_nvm_create_tamp_backup_driver(AmiNvmStorageDriver *out_driver)
{
    /* Fallback stub when TAMP registers are not physically available on host */
    if (out_driver == NULL) return;
    out_driver->read_record = NULL;
    out_driver->write_record = NULL;
    out_driver->user_data = NULL;
}
#endif

