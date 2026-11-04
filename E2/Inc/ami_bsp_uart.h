#ifndef AMI_BSP_UART_H
#define AMI_BSP_UART_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AMI_UART_DMA_RX_BUFFER_SIZE  512u
#define AMI_UART_TX_TIMEOUT_MS       200u

/**
 * @brief Initialize UART transport with circular DMA reception and IDLE line interrupt.
 * @param uart_handle Pointer to STM32 HAL UART handle (UART_HandleTypeDef *).
 * @return 0 on success, negative error code on failure.
 */
int ami_bsp_uart_init(void *uart_handle);

/**
 * @brief Transmit DLMS HDLC frame over physical UART.
 * @param data Pointer to encoded HDLC frame buffer.
 * @param length Byte length to transmit.
 * @param user_data Pointer to UART_HandleTypeDef or NULL for default.
 * @return 1 on success, 0 on failure.
 */
int ami_bsp_uart_transmit(const uint8_t *data, size_t length, void *user_data);

/**
 * @brief Hardware UART IDLE Line Interrupt handler.
 * Called from USARTx_IRQHandler / HAL_UARTEx_RxEventCallback when a frame burst finishes.
 * Pushes received bytes directly to the DLMS task stream buffer.
 * @param uart_handle Pointer to STM32 HAL UART handle.
 * @param size_received Number of bytes received in current DMA cycle.
 */
void ami_bsp_uart_rx_event_isr(void *uart_handle, uint16_t size_received);

#ifdef __cplusplus
}
#endif

#endif /* AMI_BSP_UART_H */

