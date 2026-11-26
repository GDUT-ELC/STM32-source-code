/**********************************************************
 * 文件名: MPU6050.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-29
 * 
 * 功能说明:
 *   定义MPU6050传感器驱动模块的对外接口函数与全局常量，封装底层寄存器操作。
 *   提供简洁易用的初始化、数据获取和设备识别接口，便于上层应用快速集成。
 *   包含对加速度计和陀螺仪三轴数据的批量读取支持。
 * 
 * 接口备注:
 *   1. 外部模块无需了解寄存器细节，只需调用MPU6050_Init()和GetData()
 *   2. 支持与其他I2C设备共用总线，由底层I2C驱动处理地址仲裁
 **********************************************************/

#ifndef __MPU6050_H
#define __MPU6050_H

#include "main.h"
#include "iic_soft.h"
#include "MPU6050_Reg.h"

#define MPU6050_ADDRESS		0xD0		//MPU6050的I2C从机地址

void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data);
uint8_t MPU6050_ReadReg(uint8_t RegAddress);

void MPU6050_Init(void);
uint8_t MPU6050_GetID(void);
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ);

#endif
