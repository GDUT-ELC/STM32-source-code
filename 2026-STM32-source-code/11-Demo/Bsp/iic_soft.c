/**********************************************************
 * 文件名: iic_soft.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   使用GPIO开漏输出模拟I2C时序，实现起始、停止、字节收发和
 *   应答位收发等基础功能
 *
 * 接口备注:
 *   1. 当前延时参数按168MHz系统时钟设置，属于阻塞式软件I2C
 *   2. 上层驱动必须检查iic_receive_ack的返回值
 *   3. 本驱动适合单主机教学实验，不处理多主机仲裁和时钟延展
 **********************************************************/

#include "iic_soft.h"

/**
  * @brief  产生近似微秒级阻塞延时
  * @param  us 延时时间，单位为微秒
  * @retval None
  * @note   当前循环参数按168MHz系统时钟估算，仅用于本工程软件I2C时序。
  */
static void iic_delay_us(uint32_t us)
{
    uint32_t count = us * 28U;
    volatile uint32_t i;

    for (i = 0U; i < count; i++)
    {
        __NOP();
        __NOP();
    }
}

/**
  * @brief  设置I2C时钟线SCL的电平
  * @param  bit_value 0表示拉低，非0表示释放为高电平
  * @retval None
  */
static void iic_write_scl(uint8_t bit_value)
{
    HAL_GPIO_WritePin(IIC_SCL_GPIO_Port,
                      IIC_SCL_Pin,
                      (bit_value != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    iic_delay_us(10U);
}

/**
  * @brief  设置I2C数据线SDA的电平
  * @param  bit_value 0表示拉低，非0表示释放为高电平
  * @retval None
  */
static void iic_write_sda(uint8_t bit_value)
{
    HAL_GPIO_WritePin(IIC_SDA_GPIO_Port,
                      IIC_SDA_Pin,
                      (bit_value != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    iic_delay_us(10U);
}

/**
  * @brief  读取I2C数据线SDA的当前电平
  * @param  None
  * @retval 0表示低电平，1表示高电平
  */
static uint8_t iic_read_sda(void)
{
    uint8_t bit_value;

    bit_value = (HAL_GPIO_ReadPin(IIC_SDA_GPIO_Port, IIC_SDA_Pin) == GPIO_PIN_SET) ? 1U : 0U;
    iic_delay_us(10U);
    return bit_value;
}

/**
  * @brief  初始化软件I2C总线为空闲状态
  * @param  None
  * @retval None
  */
void iic_init(void)
{
    iic_write_scl(1U);
    iic_write_sda(1U);
}

/**
  * @brief  产生I2C起始信号
  * @param  None
  * @retval None
  */
void iic_start(void)
{
    iic_write_sda(1U);
    iic_write_scl(1U);
    iic_write_sda(0U);
    iic_write_scl(0U);
}

/**
  * @brief  产生I2C停止信号
  * @param  None
  * @retval None
  */
void iic_stop(void)
{
    iic_write_sda(0U);
    iic_write_scl(1U);
    iic_write_sda(1U);
}

/**
  * @brief  通过I2C发送一个字节
  * @param  byte 待发送的数据
  * @retval None
  */
void iic_send_byte(uint8_t byte)
{
    uint8_t i;

    for (i = 0U; i < 8U; i++)
    {
        iic_write_sda((uint8_t)((byte & (0x80U >> i)) != 0U));
        iic_write_scl(1U);
        iic_write_scl(0U);
    }
}

/**
  * @brief  通过I2C接收一个字节
  * @param  None
  * @retval 接收到的8位数据
  */
uint8_t iic_receive_byte(void)
{
    uint8_t i;
    uint8_t byte = 0U;

    iic_write_sda(1U);
    for (i = 0U; i < 8U; i++)
    {
        iic_write_scl(1U);
        if (iic_read_sda() != 0U)
        {
            byte |= (uint8_t)(0x80U >> i);
        }
        iic_write_scl(0U);
    }
    return byte;
}

/**
  * @brief  主机向从机发送应答位
  * @param  ack_bit 0表示ACK，1表示NACK
  * @retval None
  */
void iic_send_ack(uint8_t ack_bit)
{
    iic_write_sda(ack_bit);
    iic_write_scl(1U);
    iic_write_scl(0U);
}

/**
  * @brief  主机读取从机返回的应答位
  * @param  None
  * @retval 0表示从机ACK，1表示从机NACK
  */
uint8_t iic_receive_ack(void)
{
    uint8_t ack_bit;

    iic_write_sda(1U);
    iic_write_scl(1U);
    ack_bit = iic_read_sda();
    iic_write_scl(0U);
    return ack_bit;
}
