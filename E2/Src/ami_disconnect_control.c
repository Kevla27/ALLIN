/**
 * @file ami_disconnect_control.c
 * @brief DLMS/COSEM Class 70 Disconnect Control (Breaker) State Machine Implementation.
 */

#include "ami_disconnect_control.h"
#include <string.h>

#if defined(STM32U585xx) && defined(USE_HAL_DRIVER)
#include "main.h"
#endif

void ami_disconnect_control_init(AmiDisconnectControlContext *ctx,
                                AmiDisconnectControlMode mode,
                                AmiBreakerActuatorFn actuator,
                                void *user_data)
{
    if (ctx == NULL) {
        return;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->output_state = 1u; /* 1 = Connected / Closed */
    ctx->control_state = AMI_DISCONNECT_STATE_CONNECTED;
    ctx->control_mode = (mode >= AMI_DISCONNECT_MODE_1 && mode <= AMI_DISCONNECT_MODE_6)
                            ? mode : AMI_DISCONNECT_MODE_2;
    ctx->actuator = actuator;
    ctx->actuator_user_data = user_data;
    ctx->transition_count = 0u;
    ctx->open_command_count = 0u;
    ctx->close_command_count = 0u;
}

AmiDisconnectStatus ami_disconnect_control_remote_disconnect(AmiDisconnectControlContext *ctx,
                                                            int8_t param)
{
    (void)param;
    if (ctx == NULL) {
        return AMI_DISCONNECT_ERR_INVALID_PARAM;
    }

    /* Check mode permissions per IEC 62056-6-2:
     * Mode 3 and Mode 4 disallow remote disconnect (manual/local only). */
    if (ctx->control_mode == AMI_DISCONNECT_MODE_3 ||
        ctx->control_mode == AMI_DISCONNECT_MODE_4) {
        return AMI_DISCONNECT_ERR_NOT_PERMITTED;
    }

    /* If already disconnected, remain disconnected */
    if (ctx->control_state == AMI_DISCONNECT_STATE_DISCONNECTED) {
        return AMI_DISCONNECT_OK;
    }

    /* Actuate physical relay to OPEN */
    if (ctx->actuator != NULL) {
        if (ctx->actuator(ctx->actuator_user_data, AMI_BREAKER_CMD_OPEN) != 0) {
            return AMI_DISCONNECT_ERR_ACTUATOR_FAILED;
        }
    }

    ctx->open_command_count++;
    ctx->output_state = 0u; /* Disconnected / Open */
    ctx->control_state = AMI_DISCONNECT_STATE_DISCONNECTED;
    ctx->transition_count++;

    return AMI_DISCONNECT_OK;
}

AmiDisconnectStatus ami_disconnect_control_remote_reconnect(AmiDisconnectControlContext *ctx,
                                                           int8_t param)
{
    (void)param;
    if (ctx == NULL) {
        return AMI_DISCONNECT_ERR_INVALID_PARAM;
    }

    /* Check mode permissions per IEC 62056-6-2:
     * Mode 3 disallows remote reconnect (manual reconnection only). */
    if (ctx->control_mode == AMI_DISCONNECT_MODE_3) {
        return AMI_DISCONNECT_ERR_NOT_PERMITTED;
    }

    /* If already connected, nothing to do */
    if (ctx->control_state == AMI_DISCONNECT_STATE_CONNECTED) {
        return AMI_DISCONNECT_OK;
    }

    /* Mode 1 & Mode 5: Remote reconnection transitions to Ready_for_Reconnection.
     * Contacts remain open (output_state = 0) until local manual confirmation. */
    if (ctx->control_mode == AMI_DISCONNECT_MODE_1 ||
        ctx->control_mode == AMI_DISCONNECT_MODE_5) {
        if (ctx->control_state != AMI_DISCONNECT_STATE_READY_FOR_RECONNECTION) {
            ctx->control_state = AMI_DISCONNECT_STATE_READY_FOR_RECONNECTION;
            ctx->output_state = 0u;
            ctx->transition_count++;
        }
        return AMI_DISCONNECT_OK;
    }

    /* Mode 2, 4, 6: Direct remote reconnection allowed -> transition to CONNECTED */
    if (ctx->actuator != NULL) {
        if (ctx->actuator(ctx->actuator_user_data, AMI_BREAKER_CMD_CLOSE) != 0) {
            return AMI_DISCONNECT_ERR_ACTUATOR_FAILED;
        }
    }

    ctx->close_command_count++;
    ctx->output_state = 1u; /* Connected / Closed */
    ctx->control_state = AMI_DISCONNECT_STATE_CONNECTED;
    ctx->transition_count++;

    return AMI_DISCONNECT_OK;
}

AmiDisconnectStatus ami_disconnect_control_manual_reconnect(AmiDisconnectControlContext *ctx)
{
    if (ctx == NULL) {
        return AMI_DISCONNECT_ERR_INVALID_PARAM;
    }

    /* If already connected, nothing to do */
    if (ctx->control_state == AMI_DISCONNECT_STATE_CONNECTED) {
        return AMI_DISCONNECT_OK;
    }

    /* Mode 6 does not allow manual reconnect (auto only) */
    if (ctx->control_mode == AMI_DISCONNECT_MODE_6) {
        return AMI_DISCONNECT_ERR_NOT_PERMITTED;
    }

    /* Mode 1 & 5: Requires prior transition to READY_FOR_RECONNECTION */
    if (ctx->control_mode == AMI_DISCONNECT_MODE_1 ||
        ctx->control_mode == AMI_DISCONNECT_MODE_5) {
        if (ctx->control_state != AMI_DISCONNECT_STATE_READY_FOR_RECONNECTION) {
            return AMI_DISCONNECT_ERR_NOT_PERMITTED;
        }
    }

    /* Execute physical actuation to CLOSE */
    if (ctx->actuator != NULL) {
        if (ctx->actuator(ctx->actuator_user_data, AMI_BREAKER_CMD_CLOSE) != 0) {
            return AMI_DISCONNECT_ERR_ACTUATOR_FAILED;
        }
    }

    ctx->close_command_count++;
    ctx->output_state = 1u; /* Connected / Closed */
    ctx->control_state = AMI_DISCONNECT_STATE_CONNECTED;
    ctx->transition_count++;

    return AMI_DISCONNECT_OK;
}

AmiDisconnectStatus ami_disconnect_control_set_mode(AmiDisconnectControlContext *ctx,
                                                   AmiDisconnectControlMode mode)
{
    if (ctx == NULL) {
        return AMI_DISCONNECT_ERR_INVALID_PARAM;
    }
    if (mode < AMI_DISCONNECT_MODE_1 || mode > AMI_DISCONNECT_MODE_6) {
        return AMI_DISCONNECT_ERR_INVALID_PARAM;
    }

    ctx->control_mode = mode;
    return AMI_DISCONNECT_OK;
}

void ami_disconnect_control_sync_snapshot(const AmiDisconnectControlContext *ctx,
                                         AmiCosemRegisterSnapshot *snapshot)
{
    if (ctx == NULL || snapshot == NULL) {
        return;
    }

    snapshot->disconnect_output_state = ctx->output_state;
    snapshot->disconnect_control_state = (uint8_t)ctx->control_state;
    snapshot->disconnect_control_mode = (uint8_t)ctx->control_mode;
    snapshot->valid_mask |= AMI_COSEM_VALID_DISCONNECT_CTRL;
}

int ami_disconnect_hal_stm32_actuate(void *user, AmiBreakerCommand cmd)
{
    (void)user;

#if defined(STM32U585xx) && defined(USE_HAL_DRIVER)
    /* Hardware driver for bistable latching relay coil on STM32U585:
     * Driving a bistable relay requires a 100ms pulse on SET or RESET pin.
     * Pins can be defined in main.h or board config (e.g. RELAY_SET_Pin, RELAY_RESET_Pin).
     */
#if defined(RELAY_SET_Pin) && defined(RELAY_RESET_Pin)
    if (cmd == AMI_BREAKER_CMD_CLOSE) {
        HAL_GPIO_WritePin(RELAY_SET_GPIO_Port, RELAY_SET_Pin, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(RELAY_SET_GPIO_Port, RELAY_SET_Pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(RELAY_RESET_GPIO_Port, RELAY_RESET_Pin, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(RELAY_RESET_GPIO_Port, RELAY_RESET_Pin, GPIO_PIN_RESET);
    }
    return 0;
#else
    /* Target STM32 defined, but pins not yet mapped in CubeMX: safe stub */
    (void)cmd;
    return 0;
#endif
#else
    /* Host environment or simulation stub */
    (void)cmd;
    return 0;
#endif
}

