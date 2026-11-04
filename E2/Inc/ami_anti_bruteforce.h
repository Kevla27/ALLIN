/**
 * @file ami_anti_bruteforce.h
 * @brief Anti-Brute-Force Progressive Exponential Backoff & Lockout per SPLN D5.002:2024.
 *
 * Implements authentication attempt tracking, progressive delay backoff,
 * and hard lockout after repeated authentication failures to protect DLMS HLS5.
 */

#ifndef AMI_ANTI_BRUTEFORCE_H
#define AMI_ANTI_BRUTEFORCE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AMI_ABF_DEFAULT_INITIAL_DELAY_SEC      5u
#define AMI_ABF_DEFAULT_BACKOFF_MULTIPLIER     2u
#define AMI_ABF_DEFAULT_MAX_FAILED_ATTEMPTS    5u
#define AMI_ABF_DEFAULT_MAX_PENALTY_DELAY_SEC  60u
#define AMI_ABF_PERMANENT_LOCKOUT              0xFFFFFFFFu

typedef enum {
    AMI_ABF_STATE_READY = 0,         /*!< Normal operational state, authentication allowed */
    AMI_ABF_STATE_PENALTY_DELAY,     /*!< Temporary penalty delay active after failed attempt */
    AMI_ABF_STATE_LOCKED_OUT         /*!< Hard lockout active after exceeding max allowed failures */
} AmiAntiBruteforceState;

typedef struct {
    uint32_t initial_delay_sec;      /*!< Initial penalty delay after 1st failure (e.g. 5s) */
    uint32_t backoff_multiplier;     /*!< Exponential multiplier per failure (e.g. 2x) */
    uint32_t max_failed_attempts;    /*!< Maximum failures before hard lockout (e.g. 5x per SPLN) */
    uint32_t max_penalty_delay_sec;  /*!< Upper ceiling for intermediate penalty delays (e.g. 60s) */
    uint32_t lockout_duration_sec;   /*!< Hard lockout window duration (0 or 0xFFFFFFFF for permanent) */
} AmiAntiBruteforceConfig;

typedef struct {
    AmiAntiBruteforceConfig config;
    uint32_t failed_attempts;
    uint32_t lockout_until_sec;      /*!< Absolute timestamp (seconds) until which access is denied */
    uint32_t total_failures_logged;  /*!< Lifetime total failure audit counter */
    int is_locked_permanent;         /*!< 1 if permanently locked until explicit administrative unlock */
    int is_initialized;
} AmiAntiBruteforceContext;

/**
 * @brief Initialize Anti-Brute-Force protection with given configuration.
 *
 * @param ctx Context to initialize.
 * @param config Configuration parameters, or NULL to use standard SPLN defaults.
 * @return 0 on success, negative error code on invalid parameters.
 */
int ami_anti_bruteforce_init(AmiAntiBruteforceContext *ctx,
                             const AmiAntiBruteforceConfig *config);

/**
 * @brief Check if an incoming association/authentication attempt is currently allowed.
 *
 * @param ctx Anti-bruteforce context.
 * @param current_time_sec Current timestamp in seconds.
 * @return 1 if allowed, 0 if currently locked out or in penalty delay.
 */
int ami_anti_bruteforce_is_allowed(const AmiAntiBruteforceContext *ctx,
                                   uint32_t current_time_sec);

/**
 * @brief Record an authentication failure and calculate the next penalty / lockout.
 *
 * @param ctx Anti-bruteforce context.
 * @param current_time_sec Current timestamp in seconds.
 * @return Penalty delay in seconds applied for this failure.
 */
uint32_t ami_anti_bruteforce_record_failure(AmiAntiBruteforceContext *ctx,
                                            uint32_t current_time_sec);

/**
 * @brief Record a successful authentication, resetting the failure counter and penalty.
 *
 * @param ctx Anti-bruteforce context.
 */
void ami_anti_bruteforce_record_success(AmiAntiBruteforceContext *ctx);

/**
 * @brief Get remaining penalty/lockout duration in seconds.
 *
 * @param ctx Anti-bruteforce context.
 * @param current_time_sec Current timestamp in seconds.
 * @return Remaining duration in seconds, or 0 if not locked out.
 */
uint32_t ami_anti_bruteforce_get_remaining_lockout(const AmiAntiBruteforceContext *ctx,
                                                   uint32_t current_time_sec);

/**
 * @brief Get current operational state.
 *
 * @param ctx Anti-bruteforce context.
 * @param current_time_sec Current timestamp in seconds.
 * @return Current AmiAntiBruteforceState.
 */
AmiAntiBruteforceState ami_anti_bruteforce_get_state(const AmiAntiBruteforceContext *ctx,
                                                    uint32_t current_time_sec);

/**
 * @brief Administratively clear lockout and restore normal operation.
 *
 * @param ctx Anti-bruteforce context.
 * @return 0 on success, negative error code on failure.
 */
int ami_anti_bruteforce_unlock(AmiAntiBruteforceContext *ctx);

#ifdef __cplusplus
}
#endif

#endif /* AMI_ANTI_BRUTEFORCE_H */

