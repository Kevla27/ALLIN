#ifndef AMI_NVM_COUNTER_H
#define AMI_NVM_COUNTER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AMI_NVM_RECORD_MAGIC              0x414D4943u /* 'AMIC' */
#define AMI_NVM_RECORD_VERSION            1u
#define AMI_NVM_DEFAULT_TX_WINDOW_STEP    100u
#define AMI_NVM_DEFAULT_RX_WINDOW_STEP    10u

/**
 * @brief Non-volatile persistent counter record structure.
 * Stored in non-volatile memory (Flash page or TAMP Backup registers).
 */
typedef struct {
    uint32_t magic;                  /*!< Magic identifier: 0x414D4943 ('AMIC') */
    uint32_t version;                /*!< Record structure format version (1) */
    uint32_t tx_invocation_counter;  /*!< High-water lease bound for Server TX counter */
    uint32_t rx_invocation_counter;  /*!< High-water mark for Client RX anti-replay */
    uint32_t reboot_count;           /*!< Number of device reboots / power cycles */
    uint32_t crc32;                  /*!< CRC-32 integrity checksum of preceding fields */
} AmiNvmCounterRecord;

/**
 * @brief Storage driver abstraction for Non-Volatile Memory.
 * Allows decoupling between Flash sector, battery-backed registers, and test RAM.
 */
typedef struct {
    int (*read_record)(void *user_data, AmiNvmCounterRecord *record);
    int (*write_record)(void *user_data, const AmiNvmCounterRecord *record);
    void *user_data;
} AmiNvmStorageDriver;

/**
 * @brief NVM Invocation Counter & Anti-Replay manager context.
 */
typedef struct {
    AmiNvmStorageDriver storage;
    uint32_t current_tx_counter;    /*!< Current runtime TX invocation counter */
    uint32_t persisted_tx_limit;    /*!< Persisted upper limit lease in NVM */
    uint32_t last_rx_counter;       /*!< Current runtime RX counter high-water mark */
    uint32_t persisted_rx_counter;  /*!< Persisted RX counter high-water mark in NVM */
    uint32_t tx_window_step;        /*!< TX lease step size (avoids writing Flash on every TX) */
    uint32_t rx_window_step;        /*!< RX commit threshold step size */
    uint32_t reboot_count;          /*!< Total recorded reboot cycles */
    uint8_t  is_initialized;
} AmiNvmCounterContext;

/**
 * @brief Initialize the NVM counter subsystem.
 *
 * Reads persisted record from NVM. If uninitialized or corrupted (invalid CRC/magic),
 * safely formats a new record. To strictly prevent nonce-reuse across reboots,
 * advances the persisted TX limit by @p tx_window_step and commits to NVM immediately.
 *
 * @param ctx Pointer to counter context.
 * @param storage Pointer to storage driver implementation.
 * @param tx_window_step TX lease step (pass 0 for AMI_NVM_DEFAULT_TX_WINDOW_STEP).
 * @param rx_window_step RX commit step (pass 0 for AMI_NVM_DEFAULT_RX_WINDOW_STEP).
 * @return 0 on success, negative error code on failure.
 */
int ami_nvm_counter_init(AmiNvmCounterContext *ctx,
                         const AmiNvmStorageDriver *storage,
                         uint32_t tx_window_step,
                         uint32_t rx_window_step);

/**
 * @brief Allocate the next strictly monotonic TX invocation counter.
 *
 * Increments current TX counter. If current counter reaches the persisted
 * lease boundary, commits the next window step to NVM before returning.
 *
 * @param ctx Pointer to counter context.
 * @param out_tx_ic Pointer to receive allocated TX counter.
 * @return 1 on success, 0 on failure.
 */
int ami_nvm_counter_next_tx(AmiNvmCounterContext *ctx, uint32_t *out_tx_ic);

/**
 * @brief Validate incoming peer/client invocation counter against anti-replay rules.
 *
 * Enforces strict monotonic increase (@p in_rx_ic > last_rx_counter). If accepted,
 * updates high-water mark. If threshold reached, commits to NVM.
 *
 * @param ctx Pointer to counter context.
 * @param in_rx_ic Incoming invocation counter from received ciphered APDU.
 * @return 1 if accepted (valid and fresh), 0 if rejected (replay attack or invalid).
 */
int ami_nvm_counter_accept_rx(AmiNvmCounterContext *ctx, uint32_t in_rx_ic);

/**
 * @brief Force flush current runtime TX & RX counters to Non-Volatile Memory.
 * Typically invoked on clean association termination or brown-out/power-down interrupt.
 *
 * @param ctx Pointer to counter context.
 * @return 0 on success, negative error code on failure.
 */
int ami_nvm_counter_flush(AmiNvmCounterContext *ctx);

/**
 * @brief Compute standard IEEE 802.3 CRC-32 checksum.
 */
uint32_t ami_nvm_calculate_crc32(const uint8_t *data, size_t length);

/**
 * @brief Create a RAM-backed storage driver for testing and host simulation.
 */
void ami_nvm_create_ram_driver(AmiNvmStorageDriver *out_driver,
                               AmiNvmCounterRecord *ram_storage);

/**
 * @brief Create an STM32 hardware battery-backed TAMP Backup Register driver.
 * Uses TAMP_BKP0R..BKP3R (zero flash endurance wear, preserved across resets).
 */
void ami_nvm_create_tamp_backup_driver(AmiNvmStorageDriver *out_driver);

#ifdef __cplusplus
}
#endif

#endif /* AMI_NVM_COUNTER_H */

