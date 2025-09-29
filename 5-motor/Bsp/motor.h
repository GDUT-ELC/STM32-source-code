/**********************************************************
 * 文件名: motor.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-28
 * 
 * 功能说明:
 *   声明电机控制相关函数接口，提供电机速度设置功能，
 *   支持两个电机的正反转及速度调节控制
 * 
 * 接口备注:
 *   1. 函数参数为int8_t类型，正值表示正转，负值表示反转
 *   2. motor_setspeed1用于控制电机1，motor_setspeed2用于控制电机2
 **********************************************************/

#ifndef __MOTOR_H
#define __MOTOR_H

#include "main.h"

void motor_setspeed1(int8_t Speed);
void motor_setspeed2(int8_t Speed);

#endif
