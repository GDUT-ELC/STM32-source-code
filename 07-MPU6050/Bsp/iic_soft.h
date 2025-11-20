/**********************************************************
 * 文件名: iic_soft.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-29
 * 
 * 功能说明:
 *   定义软件模拟I2C驱动的对外接口函数，封装底层GPIO操作，
 *   提供简洁易用的I2C通信接口，便于上层模块（如OLED、传感器）调用
 *   包含初始化、起始/停止、字节收发及应答控制等核心功能声明
 * 
 * 接口备注:
 *   1. 外部模块只需调用公开函数，无需关心底层时序实现
 *   2. 可与iic_soft.c配合实现多设备I2C通信，支持7位地址模式
 **********************************************************/

#ifndef __IIC_SOFT_H
#define __IIC_SOFT_H

#include "main.h"

void iic_init(void);
void iic_start(void);
void iic_stop(void);
void iic_sendbyte(uint8_t Byte);
uint8_t iic_recvbyte(void);
void iic_sendask(uint8_t AckBit);
uint8_t iic_recvask(void);

#endif

