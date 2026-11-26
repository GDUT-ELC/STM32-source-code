/**********************************************************
 * 文件名: led.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   定义开发板WS2812 RGB灯珠的控制接口，
 *   应用程序只需调用本文件中的函数，无需直接操作GPIO或定时器
 *
 * 接口备注:
 *   1. WS2812数据线使用TIM10通道1，使用前需先调用MX_TIM10_Init
 *   2. WS2812颜色和亮度参数的有效范围均为0~255
 **********************************************************/

#ifndef __LED_H
#define __LED_H

#include "main.h"

/* Watch: HAL_OK = frame sent, HAL_ERROR = clock off, HAL_TIMEOUT = no update. */
extern volatile HAL_StatusTypeDef g_ws_status;

void WS_SetGRBColor(uint8_t r, uint8_t g, uint8_t b);
void WS_SetBrightnessColor(uint8_t r, uint8_t g, uint8_t b,
                           uint8_t brightness);

#endif
