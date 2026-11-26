#include "uart_driver.h"
#include "usart.h"
#include <string.h>
typedef struct {
    uint8_t byte, data[UART_DRIVER_BUFFER_SIZE];
    volatile uint16_t length;
    volatile uint32_t last_tick;
    volatile uint8_t overflow, error;
} RxPort;
static RxPort rx[2];
static RxPort *find_port(UART_HandleTypeDef *port)
{
    if (port == &huart4) return &rx[0];
    if (port == &huart2) return &rx[1];
    return NULL;
}
HAL_StatusTypeDef uart_driver_init(void)
{
    memset(rx, 0, sizeof(rx));
    if (HAL_UART_Receive_IT(&huart4, &rx[0].byte, 1U) != HAL_OK) return HAL_ERROR;
    return HAL_UART_Receive_IT(&huart2, &rx[1].byte, 1U);
}
UART_DriverRxStatus uart_driver_read_port(UART_HandleTypeDef *port, uint8_t *data,
                                         uint16_t size, uint16_t *length)
{
    RxPort *p = find_port(port);
    UART_DriverRxStatus status;
    uint32_t mask;
    if (p == NULL || data == NULL || length == NULL || size == 0U) return UART_DRIVER_RX_ERROR;
    *length = 0;
    mask = __get_PRIMASK();
    __disable_irq();
    if (p->error) { p->error = 0; status = UART_DRIVER_RX_ERROR; }
    else if ((p->length == 0U && p->overflow == 0U) || (uint32_t)(HAL_GetTick() - p->last_tick) < 5U) {
        __set_PRIMASK(mask);
        return UART_DRIVER_RX_NONE;
    } else if (p->overflow || p->length > size) status = UART_DRIVER_RX_OVERFLOW;
    else {
        *length = p->length;
        memcpy(data, p->data, p->length);
        status = UART_DRIVER_RX_READY;
    }
    p->length = 0; p->overflow = 0;
    __set_PRIMASK(mask);
    return status;
}
UART_DriverRxStatus uart_driver_read_packet(uint8_t *data, uint16_t size, uint16_t *length)
{
    return uart_driver_read_port(&huart4, data, size, length);
}
HAL_StatusTypeDef uart_driver_send(const uint8_t *data, uint16_t length)
{
    if (data == NULL && length != 0U) return HAL_ERROR;
    if (length == 0U) return HAL_OK;
    return HAL_UART_Transmit(&huart4, (uint8_t *)data, length, 100U);
}
HAL_StatusTypeDef uart_driver_send_string(const char *text)
{
    size_t length;
    if (text == NULL) return HAL_ERROR;
    length = strlen(text);
    if (length > 65535U) return HAL_ERROR;
    return uart_driver_send((const uint8_t *)text, (uint16_t)length);
}
void uart_driver_rx_callback(UART_HandleTypeDef *port)
{
    RxPort *p = find_port(port);
    if (p == NULL) return;
    if (p->length < UART_DRIVER_BUFFER_SIZE) p->data[p->length++] = p->byte;
    else p->overflow = 1;
    p->last_tick = HAL_GetTick();
    if (HAL_UART_Receive_IT(port, &p->byte, 1U) != HAL_OK) p->error = 1;
}
void uart_driver_error_callback(UART_HandleTypeDef *port)
{
    RxPort *p = find_port(port);
    if (p == NULL) return;
    p->error = 1;
    (void)HAL_UART_AbortReceive(port);
    (void)HAL_UART_Receive_IT(port, &p->byte, 1U);
}
