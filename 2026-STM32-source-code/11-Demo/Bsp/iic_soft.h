/**********************************************************
 * 文件名: iic_soft.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   声明GPIO模拟I2C的基础时序接口，提供总线初始化、起始、停止、
 *   字节收发和应答位收发功能
 *
 * 接口备注:
 *   1. SCL和SDA必须配置为开漏输出，并由上拉电阻保持空闲高电平
 *   2. 地址字节由上层驱动组合，最低位为读写方向位
 *   3. iic_receive_ack返回0表示从机应答，返回1表示从机未应答
 **********************************************************/

#ifndef __IIC_SOFT_H
#define __IIC_SOFT_H

#include "main.h"

void iic_init(void);
void iic_start(void);
void iic_stop(void);
void iic_send_byte(uint8_t byte);
uint8_t iic_receive_byte(void);
void iic_send_ack(uint8_t ack_bit);
uint8_t iic_receive_ack(void);

#endif
