/**
 * @file ami_disconnect_control.h
 * @brief DLMS/COSEM Class 70 Disconnect Control (Breaker) State Machine.
 *
 * Implements IEC 62056-6-2 / DLMS Blue Book Disconnect Control object
 * (OBIS 0.0.96.3.10.255) for Three-Phase AMI Postpaid Smart Meter.
 * Provides a Hardware Abstraction Layer (HAL) for bistable latching relay coil actuation.
 */

#ifndef AMI_DISCONNECT_CONTROL_H
#define AMI_DISCONNECT_CONTROL_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ami_cosem_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Control state of the disconnect unit (Attribute 3).
 * Sesuai IEC 62056-6-2 Table 41 / DLMS Blue Book.
 */
typedef enum {
    AMI_DISCONNECT_STATE_DISCONNECTED = 0,           /**< 0: Disconnected (contacts open) */
    AMI_DISCONNECT_STATE_CONNECTED = 1,              /**< 1: Connected (contacts closed) */
    AMI_DISCONNECT_STATE_READY_FOR_RECONNECTION = 2  /**< 2: Ready for reconnection (waiting manual confirmation) */
} AmiDisconnectControlState;

/**
 * @brief Control mode configuration of the disconnect unit (Attribute 4).
 * Menentukan izin dan perilaku pemutusan (disconnect) serta penyambungan kembali (reconnect).
 */
typedef enum {
    AMI_DISCONNECT_MODE_1 = 1, /**< Disconnect: Remote, Manual, Local | Reconnect: Manual */
    AMI_DISCONNECT_MODE_2 = 2, /**< Disconnect: Remote, Manual, Local | Reconnect: Remote, Manual, Local (Default PLN) */
    AMI_DISCONNECT_MODE_3 = 3, /**< Disconnect: Manual, Local | Reconnect: Manual */
    AMI_DISCONNECT_MODE_4 = 4, /**< Disconnect: Manual, Local | Reconnect: Remote, Manual */
    AMI_DISCONNECT_MODE_5 = 5, /**< Disconnect: Remote, Manual, Local | Reconnect: Auto, Manual */
    AMI_DISCONNECT_MODE_6 = 6  /**< Disconnect: Remote, Manual, Local | Reconnect: Auto */
} AmiDisconnectControlMode;

/**
 * @brief Status return code for Disconnect Control operations.
 */
typedef enum {
    AMI_DISCONNECT_OK = 0,
    AMI_DISCONNECT_ERR_INVALID_PARAM,
    AMI_DISCONNECT_ERR_INVALID_STATE,
    AMI_DISCONNECT_ERR_NOT_PERMITTED,
    AMI_DISCONNECT_ERR_ACTUATOR_FAILED
} AmiDisconnectStatus;

/**
 * @brief Physical Breaker Command issued to the hardware actuation layer.
 */
typedef enum {
    AMI_BREAKER_CMD_OPEN = 0,  /**< Pulse reset coil -> Open contacts (disconnect load) */
    AMI_BREAKER_CMD_CLOSE = 1  /**< Pulse set coil -> Close contacts (connect load) */
} AmiBreakerCommand;

/**
 * @brief Hardware Actuator function signature (bistable latching relay coil driver).
 *
 * @param user Pointer to user/board context.
 * @param cmd Command to execute (CMD_OPEN or CMD_CLOSE).
 * @return 0 on success, non-zero on hardware/driver error.
 */
typedef int (*AmiBreakerActuatorFn)(void *user, AmiBreakerCommand cmd);

/**
 * @brief Context structure for Disconnect Control (Class 70) State Machine.
 */
typedef struct {
    uint8_t output_state;                     /**< Attribute 2: 0 = disconnected/open, 1 = connected/closed */
    AmiDisconnectControlState control_state;  /**< Attribute 3: Current control state enum */
    AmiDisconnectControlMode control_mode;    /**< Attribute 4: Operating control mode (1..6) */
    AmiBreakerActuatorFn actuator;            /**< HAL callback function for physical relay */
    void *actuator_user_data;                 /**< User context passed to actuator */
    uint32_t transition_count;                /**< Total number of state transitions */
    uint32_t open_command_count;              /**< Total OPEN commands issued */
    uint32_t close_command_count;             /**< Total CLOSE commands issued */
} AmiDisconnectControlContext;

/**
 * @brief Initialize Disconnect Control context to default power-on state.
 *
 * @param ctx Pointer to Disconnect Control context.
 * @param mode Initial control mode (typically AMI_DISCONNECT_MODE_2).
 * @param actuator Optional hardware actuator callback (can be NULL for mock/stub).
 * @param user_data User pointer passed to actuator callback.
 */
void ami_disconnect_control_init(AmiDisconnectControlContext *ctx,
                                AmiDisconnectControlMode mode,
                                AmiBreakerActuatorFn actuator,
                                void *user_data);

/**
 * @brief Execute Method 1: remote_disconnect(integer).
 *
 * If allowed by control_mode and current state:
 * - Transitions state to DISCONNECTED.
 * - Sets output_state to 0 (false).
 * - Fires actuator with AMI_BREAKER_CMD_OPEN.
 *
 * @param ctx Pointer to context.
 * @param param Parameter value from DLMS Action Request (usually 0).
 * @return AMI_DISCONNECT_OK on success, or appropriate error code.
 */
AmiDisconnectStatus ami_disconnect_control_remote_disconnect(AmiDisconnectControlContext *ctx,
                                                            int8_t param);

/**
 * @brief Execute Method 2: remote_reconnect(integer).
 *
 * Behavior depends on control_mode:
 * - In Mode 1 & 5: Transitions from DISCONNECTED -> READY_FOR_RECONNECTION (output remains 0).
 * - In Mode 2, 4, 6: Transitions from DISCONNECTED/READY -> CONNECTED (output becomes 1, fires actuator CLOSE).
 * - In Mode 3: Rejects with AMI_DISCONNECT_ERR_NOT_PERMITTED.
 *
 * @param ctx Pointer to context.
 * @param param Parameter value from DLMS Action Request (usually 0).
 * @return AMI_DISCONNECT_OK on success, or appropriate error code.
 */
AmiDisconnectStatus ami_disconnect_control_remote_reconnect(AmiDisconnectControlContext *ctx,
                                                           int8_t param);

/**
 * @brief Local manual reconnection trigger (e.g. consumer push button on meter front cover).
 *
 * In Mode 1, 3, 5: If control_state is READY_FOR_RECONNECTION, transitions to CONNECTED,
 * sets output_state to 1, and fires actuator CLOSE.
 *
 * @param ctx Pointer to context.
 * @return AMI_DISCONNECT_OK on success, or appropriate error code.
 */
AmiDisconnectStatus ami_disconnect_control_manual_reconnect(AmiDisconnectControlContext *ctx);

/**
 * @brief Change operating control mode (Attribute 4).
 *
 * @param ctx Pointer to context.
 * @param mode New mode (AMI_DISCONNECT_MODE_1 .. AMI_DISCONNECT_MODE_6).
 * @return AMI_DISCONNECT_OK on success, or AMI_DISCONNECT_ERR_INVALID_PARAM.
 */
AmiDisconnectStatus ami_disconnect_control_set_mode(AmiDisconnectControlContext *ctx,
                                                   AmiDisconnectControlMode mode);

/**
 * @brief Synchronize state machine values into the COSEM register snapshot.
 *
 * Updates disconnect_output_state, disconnect_control_state, and disconnect_control_mode
 * in the provided snapshot and sets the AMI_COSEM_VALID_DISCONNECT_CTRL flag.
 *
 * @param ctx Pointer to context.
 * @param snapshot Pointer to snapshot to update.
 */
void ami_disconnect_control_sync_snapshot(const AmiDisconnectControlContext *ctx,
                                         AmiCosemRegisterSnapshot *snapshot);

/**
 * @brief Reference HAL GPIO driver for STM32U585 bistable latching relay.
 *
 * Provides a pulse on the designated GPIO pin (SET for close, RESET for open).
 * On host/unit test targets, functions as a safe stub returning 0.
 *
 * @param user Optional pointer to board GPIO configuration.
 * @param cmd Command to execute.
 * @return 0 on success, non-zero on failure.
 */
int ami_disconnect_hal_stm32_actuate(void *user, AmiBreakerCommand cmd);

#ifdef __cplusplus
}
#endif

#endif /* AMI_DISCONNECT_CONTROL_H */

