/**********************************************************
 * 文件名: buzzer.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   定义无源蜂鸣器的PWM鸣叫和停止接口，应用程序可直接调用
 *
 * 接口备注:
 *   1. 蜂鸣器连接PE14，由TIM1_CH4输出方波驱动
 *   2. 使用前需先调用MX_TIM1_Init完成PWM初始化
 *   3. buzzer_on开始输出音调，buzzer_off停止PWM输出
 **********************************************************/

#ifndef __BUZZER_H
#define __BUZZER_H

#include "main.h"

void buzzer_on(void);
void buzzer_off(void);

#endif
