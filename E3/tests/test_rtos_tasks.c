/**
* @file test_rtos_tasks.c
* @brief Pengujian Inisialisasi System Task, FreeRTOS Queue & DLMS Consumer E3
*/
#include <stdio.h>
#include <assert.h>
#include "rtos_tasks.h"
#include "dlms_obis.h"
#include "bsp_alarm_buzzer.h"

volatile bool g_tamper_alarm_active = false;

int main(void) {
 printf("=========================================\n");
 printf(" RUNNING RTOS TASKS & QUEUE TEST \n");
 printf("=========================================\n\n");
 /* 1. Inisialisasi Sistem & Queue */
 rtos_system_init();

 /* 2. Uji Kirim Pesan Event Sabotase oleh Task Tamper (Producer) */
 tamper_event_msg_t tx_msg = {
 .timestamp = 1700000000,
 .tamper_code = 0x01, /* Terminal Cover Open */
 .is_active = true
 };
 bool send_ok = rtos_queue_send_tamper_event(&tx_msg);
 assert(send_ok == true);
 printf("[TEST] Task Tamper (Producer) Send Event to Queue: SUCCESS\n");

 /* 3. Uji Terima Pesan Event Sabotase oleh Task DLMS (Consumer) */
 tamper_event_msg_t rx_msg = {0};
 bool recv_ok = rtos_queue_receive_tamper_event(&rx_msg, 100);
 assert(recv_ok == true);
 assert(rx_msg.timestamp == 1700000000);
 assert(rx_msg.tamper_code == 0x01);
 assert(rx_msg.is_active == true);
 printf("[TEST] Task DLMS (Consumer) Receive Event from Queue: SUCCESS (Code: 0x%02X)\n",
 rx_msg.tamper_code);

 /* 4. Pastikan Queue Kosong Setelah Dibaca */
 bool empty_ok = rtos_queue_receive_tamper_event(&rx_msg, 100);
 assert(empty_ok == false);
 printf("[TEST] Verify Queue Empty State: SUCCESS\n");

 /* 5. Uji Pemrosesan Event ke DLMS Tamper Log & Kamus OBIS */
 uint32_t init_term_count = dlms_obis_get_tamper_counter(DLMS_TAMPER_TERMINAL_COVER_OPEN);
 rtos_dlms_process_tamper_event(&rx_msg);

 dlms_task_ctx_t *task_ctx = rtos_dlms_get_task_ctx();
 assert(task_ctx != NULL);
 assert(task_ctx->server.tamper_log.entries_in_use == 1);

 dlms_tamper_record_t rec;
 assert(dlms_tamper_log_get_entry(&task_ctx->server.tamper_log, 0, &rec) == DLMS_OK);
 assert(rec.timestamp_epoch == 1700000000);
 assert(rec.event_code == DLMS_TAMPER_TERMINAL_COVER_OPEN);
 assert(rec.status == 1);
 printf("[TEST] Verify DLMS Tamper Log FIFO Record: SUCCESS\n");

 assert(dlms_obis_get_tamper_counter(DLMS_TAMPER_TERMINAL_COVER_OPEN) == init_term_count + 1);
 printf("[TEST] Verify OBIS Counter (0.0.96.20.5.255) Incremented: SUCCESS\n");

 tamper_context_t *t_ctx = rtos_tamper_get_ctx();
 assert(t_ctx != NULL);
 assert(tamper_get_active_mask(t_ctx) & TAMPER_VECTOR_TERMINAL_OPEN);
 printf("[TEST] Verify E3 Tamper Active Mask: SUCCESS\n");

 /* 6. Uji Aktuasi Buzzer & Indikator Alarm SPLN (PB7) */
 assert(bsp_alarm_buzzer_get() == false);
 bsp_alarm_buzzer_set(true);
 assert(bsp_alarm_buzzer_get() == true);
 bsp_alarm_buzzer_toggle();
 assert(bsp_alarm_buzzer_get() == false);
 bsp_alarm_buzzer_toggle();
 assert(bsp_alarm_buzzer_get() == true);
 bsp_alarm_buzzer_set(false);
 assert(bsp_alarm_buzzer_get() == false);
 printf("[TEST] Verify SPLN Buzzer & Alarm Indicator (PB7) Actuation: SUCCESS\n");

 printf("\n>>> ALL RTOS TASKS, QUEUE, BUZZER & DLMS CONSUMER TESTS PASSED 100%! <<<\n");
 printf("=========================================\n");
 return 0;
}
