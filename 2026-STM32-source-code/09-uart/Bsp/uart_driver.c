/**********************************************************
 * 文件名: uart_driver.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   封装USART1中断接收、空闲分包和阻塞发送功能。中断每次接收
 *   一个字节，主循环通过公开接口读取完整数据包并进行后续处理
 *
 * 接口备注:
 *   1. 调用uart_driver_init前需先完成MX_USART1_UART_Init
 *   2. 主循环应持续调用uart_driver_read_packet检查是否收到完整数据包
 *   3. 接收缓冲区满后会丢弃后续字节，并返回UART_DRIVER_RX_OVERFLOW
 **********************************************************/

#include "uart_driver.h"
#include "usart.h"
#include <string.h>

#define UART_DRIVER_IDLE_TIME_MS     5U
#define UART_DRIVER_TX_TIMEOUT_MS 1000U

static uint8_t uart_rx_byte = 0;
static uint8_t uart_rx_buffer[UART_DRIVER_BUFFER_SIZE] = {0};
static volatile uint16_t uart_rx_length = 0;
static volatile uint32_t uart_last_rx_tick = 0;
static volatile uint8_t uart_rx_overflow = 0;

/**
  * @brief  初始化串口驱动并启动USART1中断接收
  * @param  None
  * @retval HAL_OK: 启动成功；其他值: 启动失败
  */
HAL_StatusTypeDef uart_driver_init(void)
{
    uart_rx_length = 0;
    uart_last_rx_tick = 0;
    uart_rx_overflow = 0;

    return HAL_UART_Receive_IT(&huart4, &uart_rx_byte, 1U);
}

/**
  * @brief  读取一个已经接收完成的数据包
  * @param  data: 用于保存接收数据的数组
  * @param  data_size: data数组能够保存的最大字节数
  * @param  data_length: 用于返回本次实际读取的字节数
  * @retval UART_DRIVER_RX_NONE: 暂无完整数据包
  * @retval UART_DRIVER_RX_READY: 已读取一个完整数据包
  * @retval UART_DRIVER_RX_OVERFLOW: 接收数据超过缓冲区或输出数组容量
  * @retval UART_DRIVER_RX_ERROR: 参数无效
  */
UART_DriverRxStatus uart_driver_read_packet(uint8_t *data,
                                            uint16_t data_size,
                                            uint16_t *data_length)
{
    UART_DriverRxStatus status;
    uint16_t length;
    uint32_t primask;

    if ((data == NULL) || (data_length == NULL) || (data_size == 0U))
    {
        return UART_DRIVER_RX_ERROR;
    }

    *data_length = 0;

    if ((uart_rx_length == 0U) && (uart_rx_overflow == 0U))
    {
        return UART_DRIVER_RX_NONE;
    }

    if ((HAL_GetTick() - uart_last_rx_tick) < UART_DRIVER_IDLE_TIME_MS)
    {
        return UART_DRIVER_RX_NONE;
    }

    primask = __get_PRIMASK();
    __disable_irq();

    /* 关中断后再次检查，避免新字节刚好在第一次检查后到达 */
    if ((HAL_GetTick() - uart_last_rx_tick) < UART_DRIVER_IDLE_TIME_MS)
    {
        if (primask == 0U)
        {
            __enable_irq();
        }
        return UART_DRIVER_RX_NONE;
    }

    length = uart_rx_length;
    if ((uart_rx_overflow != 0U) || (length > data_size))
    {
        status = UART_DRIVER_RX_OVERFLOW;
    }
    else
    {
        memcpy(data, uart_rx_buffer, length);
        *data_length = length;
        status = UART_DRIVER_RX_READY;
    }

    uart_rx_length = 0;
    uart_rx_overflow = 0;

    if (primask == 0U)
    {
        __enable_irq();
    }

    return status;
}

/**
  * @brief  通过USART1发送指定长度的数据
  * @param  data: 待发送数据的首地址
  * @param  data_length: 待发送数据的字节数
  * @retval HAL_OK: 发送成功；其他值: 发送失败
  */
HAL_StatusTypeDef uart_driver_send(const uint8_t *data, uint16_t data_length)
{
    if ((data == NULL) && (data_length > 0U))
    {
        return HAL_ERROR;
    }

    if (data_length == 0U)
    {
        return HAL_OK;
    }

    return HAL_UART_Transmit(&huart4,
                             (uint8_t *)data,
                             data_length,
                             UART_DRIVER_TX_TIMEOUT_MS);
}

/**
  * @brief  通过USART1发送以\0结尾的字符串
  * @param  text: 待发送的字符串
  * @retval HAL_OK: 发送成功；其他值: 发送失败
  */
HAL_StatusTypeDef uart_driver_send_string(const char *text)
{
    size_t length;

    if (text == NULL)
    {
        return HAL_ERROR;
    }

    length = strlen(text);
    if (length > 65535U)
    {
        return HAL_ERROR;
    }

    return uart_driver_send((const uint8_t *)text, (uint16_t)length);
}

/**
  * @brief  USART1接收完成回调，将新字节存入内部缓冲区并继续接收
  * @param  huart: 产生接收完成中断的串口句柄
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart4)
    {
        if (uart_rx_length < UART_DRIVER_BUFFER_SIZE)
        {
            uart_rx_buffer[uart_rx_length] = uart_rx_byte;
            uart_rx_length++;
        }
        else
        {
            uart_rx_overflow = 1U;
        }

        uart_last_rx_tick = HAL_GetTick();
        (void)HAL_UART_Receive_IT(&huart4, &uart_rx_byte, 1U);
    }
}
