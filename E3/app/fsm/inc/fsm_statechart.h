/**
 * @file fsm_statechart.h
 * @brief Mesin Status Berhingga (FSM Statechart) Firmware Host MCU STM32U585 (E3/ENG-3)
 */

#ifndef FSM_STATECHART_H
#define FSM_STATECHART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Status Operasi Utama FSM Firmware (State 0 s/d State 5)
 */
typedef enum {
    STATE_0_BOOT_INIT = 0,    /**< State 0: Power-On Reset (POR), Init RCC 160MHz, Clocks, GPIO */
    STATE_1_SELF_TEST,        /**< State 1: Checksum ROM/RAM, Ping ADE9078 AFE, Flash & Modem Test */
    STATE_2_NORMAL_METERING,  /**< State 2: Superstate (Sampling 1s, Load Profile 15m, DLMS Push, HMI) */
    STATE_3_TAMPER_EVENT,     /**< State 3: Pin IRQ0 EXTI (<10ms), Manipulasi < 5s, LED & Buzzer */
    STATE_4_LAST_GASP_MODE,   /**< State 4: PWR_GOOD Low (<1ms), Freeze Metrology, Flash Dump, Deep Sleep */
    STATE_5_ACTUATION         /**< State 5: Buka/Tutup Relai, Kunci AES-128 GCM, Pulsa Koil 50ms */
} fsm_state_t;

typedef enum {
    FSM_EVT_NONE = 0,
    FSM_EVT_POR_COMPLETE,         /**< Power-On Reset selesai */
    FSM_EVT_SELF_TEST_PASS,       /**< Self-Test lulus (Checksum valid) */
    FSM_EVT_SELF_TEST_FAIL,       /**< Self-Test gagal */
    FSM_EVT_IRQ0_TAMPER_TRIGGER,  /**< Interupsi EXTI IRQ0 tamper terdeteksi */
    FSM_EVT_TAMPER_CLEAR,         /**< Tamper terkonfirmasi pulih / ACK */
    FSM_EVT_PWR_GOOD_LOW,         /**< Sinyal PWR_GOOD jatuh ke LOW (Last-Gasp / Blackout) */
    FSM_EVT_PWR_RESTORED,         /**< Listrik AC pulih kembali */
    FSM_EVT_CMD_RELAY_ACTUATE,    /**< Perintah remote DLMS/HES untuk buka/tutup relai */
    FSM_EVT_RELAY_COMPLETE        /**< Eksekusi pulsa koil relai selesai */
} fsm_event_t;

typedef struct {
    fsm_state_t current_state;      /**< State aktif saat ini */
    fsm_state_t previous_state;     /**< State sebelumnya */
    uint32_t    state_enter_time;   /**< Timestamp memasuki state */
    bool        is_self_test_ok;    /**< Status kesehatan sistem */
    bool        relay_closed;       /**< Status kontak relai (true = Connect, false = Disconnect) */
    uint32_t    tamper_active_mask; /**< Bit-string tamper aktif */
    uint32_t    last_gasp_dumps;    /**< Hitungan dump data pemadaman ke Flash */
} fsm_context_t;

void fsm_init(fsm_context_t *ctx);
bool fsm_process_event(fsm_context_t *ctx, fsm_event_t evt);
fsm_state_t fsm_get_current_state(const fsm_context_t *ctx);
const char* fsm_state_to_string(fsm_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* FSM_STATECHART_H */