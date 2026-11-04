/**
 * @file ami_anti_bruteforce.c
 * @brief Anti-Brute-Force Progressive Exponential Backoff & Lockout per SPLN D5.002:2024.
 */

#include "ami_anti_bruteforce.h"
#include <string.h>

int ami_anti_bruteforce_init(AmiAntiBruteforceContext *ctx,
                             const AmiAntiBruteforceConfig *config)
{
    if (ctx == NULL) {
        return -1;
    }

    memset(ctx, 0, sizeof(*ctx));

    if (config != NULL) {
        ctx->config = *config;
        if (ctx->config.initial_delay_sec == 0u) {
            ctx->config.initial_delay_sec = AMI_ABF_DEFAULT_INITIAL_DELAY_SEC;
        }
        if (ctx->config.backoff_multiplier == 0u) {
            ctx->config.backoff_multiplier = AMI_ABF_DEFAULT_BACKOFF_MULTIPLIER;
        }
        if (ctx->config.max_failed_attempts == 0u) {
            ctx->config.max_failed_attempts = AMI_ABF_DEFAULT_MAX_FAILED_ATTEMPTS;
        }
        if (ctx->config.max_penalty_delay_sec == 0u) {
            ctx->config.max_penalty_delay_sec = AMI_ABF_DEFAULT_MAX_PENALTY_DELAY_SEC;
        }
    } else {
        /* Standard SPLN D5.002:2024 defaults */
        ctx->config.initial_delay_sec = AMI_ABF_DEFAULT_INITIAL_DELAY_SEC;
        ctx->config.backoff_multiplier = AMI_ABF_DEFAULT_BACKOFF_MULTIPLIER;
        ctx->config.max_failed_attempts = AMI_ABF_DEFAULT_MAX_FAILED_ATTEMPTS;
        ctx->config.max_penalty_delay_sec = AMI_ABF_DEFAULT_MAX_PENALTY_DELAY_SEC;
        ctx->config.lockout_duration_sec = 0u; /* 0 = Permanent lockout until administrative unlock */
    }

    ctx->failed_attempts = 0u;
    ctx->lockout_until_sec = 0u;
    ctx->total_failures_logged = 0u;
    ctx->is_locked_permanent = 0;
    ctx->is_initialized = 1;

    return 0;
}

int ami_anti_bruteforce_is_allowed(const AmiAntiBruteforceContext *ctx,
                                   uint32_t current_time_sec)
{
    if (ctx == NULL || !ctx->is_initialized) {
        return 0; /* Fail-closed */
    }

    if (ctx->is_locked_permanent) {
        return 0;
    }

    if (current_time_sec < ctx->lockout_until_sec) {
        return 0;
    }

    return 1;
}

uint32_t ami_anti_bruteforce_record_failure(AmiAntiBruteforceContext *ctx,
                                            uint32_t current_time_sec)
{
    if (ctx == NULL || !ctx->is_initialized) {
        return 0u;
    }

    ctx->failed_attempts++;
    ctx->total_failures_logged++;

    /* Check for hard lockout threshold */
    if (ctx->failed_attempts >= ctx->config.max_failed_attempts) {
        if (ctx->config.lockout_duration_sec == 0u ||
            ctx->config.lockout_duration_sec == AMI_ABF_PERMANENT_LOCKOUT) {
            ctx->is_locked_permanent = 1;
            ctx->lockout_until_sec = 0xFFFFFFFFu;
            return 0xFFFFFFFFu;
        } else {
            ctx->lockout_until_sec = current_time_sec + ctx->config.lockout_duration_sec;
            return ctx->config.lockout_duration_sec;
        }
    }

    /* Calculate progressive exponential backoff: delay = initial * (multiplier ^ (attempts - 1)) */
    uint32_t delay = ctx->config.initial_delay_sec;
    for (uint32_t i = 1u; i < ctx->failed_attempts; i++) {
        if (delay > ctx->config.max_penalty_delay_sec / ctx->config.backoff_multiplier) {
            delay = ctx->config.max_penalty_delay_sec;
            break;
        }
        delay *= ctx->config.backoff_multiplier;
    }

    if (delay > ctx->config.max_penalty_delay_sec) {
        delay = ctx->config.max_penalty_delay_sec;
    }

    /* Guard against timestamp addition overflow */
    if (current_time_sec > 0xFFFFFFFFu - delay) {
        ctx->lockout_until_sec = 0xFFFFFFFFu;
    } else {
        ctx->lockout_until_sec = current_time_sec + delay;
    }

    return delay;
}

void ami_anti_bruteforce_record_success(AmiAntiBruteforceContext *ctx)
{
    if (ctx == NULL || !ctx->is_initialized) {
        return;
    }

    /* A successful authentication resets the consecutive failure counter */
    if (!ctx->is_locked_permanent) {
        ctx->failed_attempts = 0u;
        ctx->lockout_until_sec = 0u;
    }
}

uint32_t ami_anti_bruteforce_get_remaining_lockout(const AmiAntiBruteforceContext *ctx,
                                                   uint32_t current_time_sec)
{
    if (ctx == NULL || !ctx->is_initialized) {
        return 0u;
    }

    if (ctx->is_locked_permanent) {
        return 0xFFFFFFFFu;
    }

    if (current_time_sec >= ctx->lockout_until_sec) {
        return 0u;
    }

    return ctx->lockout_until_sec - current_time_sec;
}

AmiAntiBruteforceState ami_anti_bruteforce_get_state(const AmiAntiBruteforceContext *ctx,
                                                    uint32_t current_time_sec)
{
    if (ctx == NULL || !ctx->is_initialized) {
        return AMI_ABF_STATE_LOCKED_OUT;
    }

    if (ctx->is_locked_permanent ||
        (ctx->failed_attempts >= ctx->config.max_failed_attempts && current_time_sec < ctx->lockout_until_sec)) {
        return AMI_ABF_STATE_LOCKED_OUT;
    }

    if (current_time_sec < ctx->lockout_until_sec) {
        return AMI_ABF_STATE_PENALTY_DELAY;
    }

    return AMI_ABF_STATE_READY;
}

int ami_anti_bruteforce_unlock(AmiAntiBruteforceContext *ctx)
{
    if (ctx == NULL || !ctx->is_initialized) {
        return -1;
    }

    ctx->failed_attempts = 0u;
    ctx->lockout_until_sec = 0u;
    ctx->is_locked_permanent = 0;
    return 0;
}

