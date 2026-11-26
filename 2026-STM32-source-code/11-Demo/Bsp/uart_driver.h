#ifndef UART_DRIVER_H
#define UART_DRIVER_H
#include "main.h"
#define UART_DRIVER_BUFFER_SIZE 128U
typedef enum {
    UART_DRIVER_RX_NONE = 0, UART_DRIVER_RX_READY,
    UART_DRIVER_RX_OVERFLOW, UART_DRIVER_RX_ERROR
} UART_DriverRxStatus;
/* UART4 PA0/PA1 and USART2 PA2/PA3; 115200 8N1, 5ms silence ends a packet. */
HAL_StatusTypeDef uart_driver_init(void);
UART_DriverRxStatus uart_driver_read_packet(uint8_t *data, uint16_t size, uint16_t *length);
UART_DriverRxStatus uart_driver_read_port(UART_HandleTypeDef *port, uint8_t *data,
                                         uint16_t size, uint16_t *length);
HAL_StatusTypeDef uart_driver_send(const uint8_t *data, uint16_t length);
HAL_StatusTypeDef uart_driver_send_string(const char *text);
void uart_driver_rx_callback(UART_HandleTypeDef *port);
void uart_driver_error_callback(UART_HandleTypeDef *port);
#endif
