/**********************************************************
 * 文件名: uart_driver.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   声明USART1中断接收、空闲分包和数据发送接口，使应用程序可以
 *   直接调用函数完成串口收发，不需要操作接收中断和内部缓冲区
 *
 * 接口备注:
 *   1. 使用USART1，默认引脚为PA9(TX)和PA10(RX)，波特率115200
 *   2. 连续5ms没有收到新字节时，将当前数据判断为一个完整数据包
 *   3. 单个数据包最多接收128字节，超过后返回缓冲区溢出状态
 **********************************************************/

#ifndef __UART_DRIVER_H
#define __UART_DRIVER_H

#include "main.h"

#define UART_DRIVER_BUFFER_SIZE  128U

typedef enum
{
    UART_DRIVER_RX_NONE = 0,
    UART_DRIVER_RX_READY,
    UART_DRIVER_RX_OVERFLOW,
    UART_DRIVER_RX_ERROR
} UART_DriverRxStatus;

HAL_StatusTypeDef uart_driver_init(void);
UART_DriverRxStatus uart_driver_read_packet(uint8_t *data,
                                            uint16_t data_size,
                                            uint16_t *data_length);
HAL_StatusTypeDef uart_driver_send(const uint8_t *data, uint16_t data_length);
HAL_StatusTypeDef uart_driver_send_string(const char *text);

#endif
