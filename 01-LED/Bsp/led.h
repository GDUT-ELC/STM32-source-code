/**********************************************************
 * 文件名: led.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-28
 * 
 * 功能说明:
 *   声明红、绿、蓝三色LED的控制函数接口，包括点亮、关闭
 *   和状态切换等操作，供外部模块调用
 **********************************************************/
 
#ifndef __LED_H
#define __LED_H

#include "main.h"

void led_red_on(void);
void led_red_off(void);
void led_red_toggle(void);
void led_green_on(void);
void led_green_off(void);
void led_green_toggle(void);
void led_blue_on(void);
void led_blue_off(void);
void led_blue_toggle(void);

#endif

