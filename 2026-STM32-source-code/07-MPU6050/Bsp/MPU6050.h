/**********************************************************
 * 文件名: MPU6050.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   声明MPU6050六轴传感器的初始化、设备识别和数据读取接口，
 *   隐藏底层寄存器及软件I2C通信细节
 *
 * 接口备注:
 *   1. 使用前先完成GPIO初始化，再调用MPU6050_Init
 *   2. MPU6050_GetData一次连续读取同一组加速度和角速度原始值
 *   3. 所有接口返回HAL_OK表示成功，其他返回值表示通信或参数错误
 *   4. 本工程使用7位地址0x68，对应总线写地址字节0xD0
 **********************************************************/

#ifndef __MPU6050_H
#define __MPU6050_H

#include "main.h"

#define MPU6050_ADDRESS_7BIT  0x68U

/**
  * @brief  初始化MPU6050并检查设备ID
  * @param  None
  * @retval HAL_OK表示初始化成功，HAL_ERROR表示通信或设备ID错误
  */
HAL_StatusTypeDef MPU6050_Init(void);

/**
  * @brief  读取MPU6050的WHO_AM_I设备ID
  * @param  device_id 用于保存设备ID的指针
  * @retval HAL_OK表示读取成功，HAL_ERROR表示参数或通信错误
  */
HAL_StatusTypeDef MPU6050_GetID(uint8_t *device_id);

/**
  * @brief  一次连续读取三轴加速度和三轴角速度原始值
  * @param  acc_x acc_y acc_z 三轴加速度原始值输出指针
  * @param  gyro_x gyro_y gyro_z 三轴角速度原始值输出指针
  * @retval HAL_OK表示读取成功，HAL_ERROR表示参数或通信错误
  */
HAL_StatusTypeDef MPU6050_GetData(int16_t *acc_x,
                                  int16_t *acc_y,
                                  int16_t *acc_z,
                                  int16_t *gyro_x,
                                  int16_t *gyro_y,
                                  int16_t *gyro_z);

#endif
