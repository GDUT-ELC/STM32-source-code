/**********************************************************
 * 文件名: buzzer.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   封装无源蜂鸣器的PWM控制功能，提供鸣叫和停止接口
 *
 * 接口备注:
 *   1. 蜂鸣器连接PE14，对应TIM1通道4的PWM输出
 *   2. TIM1当前产生约2.73 kHz、50%占空比方波
 *   3. 使用前先调用MX_TIM1_Init，停止时PE14不再输出PWM
 **********************************************************/

#include "buzzer.h"
#include "tim.h"

#define BUZZER_PWM_CHANNEL       TIM_CHANNEL_4
#define BUZZER_PWM_DUTY_COMPARE  366U

/**
 * @brief  开启蜂鸣器
 * @param  None
 * @retval None
 */
void buzzer_on(void)
{
    __HAL_TIM_SET_COMPARE(&htim1,
                          BUZZER_PWM_CHANNEL,
                          BUZZER_PWM_DUTY_COMPARE);
    (void)HAL_TIM_PWM_Start(&htim1, BUZZER_PWM_CHANNEL);
}

/**
 * @brief  关闭蜂鸣器
 * @param  None
 * @retval None
 */
void buzzer_off(void)
{
    (void)HAL_TIM_PWM_Stop(&htim1, BUZZER_PWM_CHANNEL);
    __HAL_TIM_SET_COMPARE(&htim1, BUZZER_PWM_CHANNEL, 0U);
}
