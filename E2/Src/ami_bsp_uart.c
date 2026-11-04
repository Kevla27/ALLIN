#include "ami_bsp_uart.h"
#include "ami_dlms_port.h"
#include <string.h>

#if defined(STM32U585xx) || defined(USE_HAL_DRIVER)
#include "stm32u5xx_hal.h"

static UART_HandleTypeDef *s_active_uart = NULL;
static uint8_t s_dma_rx_buf[AMI_UART_DMA_RX_BUFFER_SIZE];
static uint16_t s_last_dma_index = 0u;

int ami_bsp_uart_init(void *uart_handle)
{
    if (uart_handle == NULL) {
        return -1;
    }
    s_active_uart = (UART_HandleTypeDef *)uart_handle;
    s_last_dma_index = 0u;

    /* Start DMA reception with IDLE line detection */
    if (HAL_UARTEx_ReceiveToIdle_DMA(s_active_uart, s_dma_rx_buf, sizeof(s_dma_rx_buf)) != HAL_OK) {
        return -2;
    }

    return 0;
}

int ami_bsp_uart_transmit(const uint8_t *data, size_t length, void *user_data)
{
    UART_HandleTypeDef *huart = (user_data != NULL) ? (UART_HandleTypeDef *)user_data : s_active_uart;
    if (huart == NULL || data == NULL || length == 0u) {
        return 0;
    }

    HAL_StatusTypeDef st = HAL_UART_Transmit(huart, (const uint8_t *)data, (uint16_t)length, AMI_UART_TX_TIMEOUT_MS);
    return (st == HAL_OK) ? 1 : 0;
}

void ami_bsp_uart_rx_event_isr(void *uart_handle, uint16_t size_received)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)uart_handle;
    if (huart != s_active_uart || size_received == 0u) {
        return;
    }

    /* Calculate newly received bytes from circular DMA buffer */
    if (size_received > s_last_dma_index) {
        uint16_t len = size_received - s_last_dma_index;
        ami_dlms_uart_rx_buffer_isr(&s_dma_rx_buf[s_last_dma_index], len);
    } else if (size_received < s_last_dma_index) {
        /* Wrapped around buffer end */
        uint16_t len1 = sizeof(s_dma_rx_buf) - s_last_dma_index;
        if (len1 > 0u) {
            ami_dlms_uart_rx_buffer_isr(&s_dma_rx_buf[s_last_dma_index], len1);
        }
        if (size_received > 0u) {
            ami_dlms_uart_rx_buffer_isr(&s_dma_rx_buf[0], size_received);
        }
    }
    s_last_dma_index = (size_received < sizeof(s_dma_rx_buf)) ? size_received : 0u;
}

/* Override weak default UART TX in ami_dlms_port.c */
int ami_dlms_hardware_uart_tx(const uint8_t *data, size_t length, void *user_data)
{
    return ami_bsp_uart_transmit(data, length, user_data);
}

#else

/* Host simulation stub */
static int (*s_host_tx_mock)(const uint8_t *, size_t) = NULL;

int ami_bsp_uart_init(void *uart_handle)
{
    (void)uart_handle;
    return 0;
}

int ami_bsp_uart_transmit(const uint8_t *data, size_t length, void *user_data)
{
    (void)user_data;
    if (s_host_tx_mock != NULL) {
        return s_host_tx_mock(data, length);
    }
    return (data != NULL && length > 0) ? 1 : 0;
}

void ami_bsp_uart_rx_event_isr(void *uart_handle, uint16_t size_received)
{
    (void)uart_handle;
    (void)size_received;
}

#endif

