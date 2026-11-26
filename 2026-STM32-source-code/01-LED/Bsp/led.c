/**********************************************************
 * 文件名: led.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   封装开发板WS2812 RGB灯珠的控制功能，
 *   包括板载LED的亮灭/翻转以及WS2812颜色和亮度设置
 *
 * 接口备注:
 *   1. WS2812使用PB8对应的TIM10通道1输出800 kHz PWM时序
 *   2. WS2812内部按照G、R、B顺序发送24位颜色数据
 **********************************************************/

#include "led.h"
#include "tim.h"

#define WS_T0H       55U
#define WS_T1H       115U
#define WS_PERIOD    210U


/**
 * @brief  通过PWM时序向WS2812发送一个字节
 * @param  data 要发送的8位数据
 * @retval None
 */
static void WS_SendByte(uint8_t data)
{
    int8_t bit;

    for (bit = 7; bit >= 0; bit--)
    {
        __HAL_TIM_SET_COMPARE(&htim10, TIM_CHANNEL_1,
                              (data & (1U << bit)) ? WS_T1H : WS_T0H);

        /* 等待当前PWM周期完成，再发送下一位数据 */
        while (__HAL_TIM_GET_COUNTER(&htim10) < (WS_PERIOD / 2U))
        {
        }
        while (__HAL_TIM_GET_COUNTER(&htim10) >= (WS_PERIOD / 2U))
        {
        }
    }
}

/**
 * @brief  设置WS2812显示颜色
 * @param  r 红色分量，范围0~255
 * @param  g 绿色分量，范围0~255
 * @param  b 蓝色分量，范围0~255
 * @retval None
 */
void WS_SetGRBColor(uint8_t r, uint8_t g, uint8_t b)
{
    HAL_TIM_PWM_Start(&htim10, TIM_CHANNEL_1);

    WS_SendByte(g);
    WS_SendByte(r);
    WS_SendByte(b);

    HAL_TIM_PWM_Stop(&htim10, TIM_CHANNEL_1);

    /* 保持低电平超过50 us，使WS2812锁存本次颜色 */
    HAL_Delay(1U);
}

/**
 * @brief  按统一亮度系数设置WS2812显示颜色
 * @param  r 红色分量，范围0~255
 * @param  g 绿色分量，范围0~255
 * @param  b 蓝色分量，范围0~255
 * @param  brightness 亮度系数，0表示熄灭，255表示原始亮度
 * @retval None
 */
void WS_SetBrightnessColor(uint8_t r, uint8_t g, uint8_t b,
                           uint8_t brightness)
{
    r = (uint8_t)(((uint16_t)r * brightness) / 255U);
    g = (uint8_t)(((uint16_t)g * brightness) / 255U);
    b = (uint8_t)(((uint16_t)b * brightness) / 255U);

    WS_SetGRBColor(r, g, b);
}
