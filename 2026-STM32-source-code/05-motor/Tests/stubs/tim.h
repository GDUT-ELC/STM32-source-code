#ifndef TEST_TIM_H
#define TEST_TIM_H
#include "main.h"
typedef struct {
    uint32_t CCMR1;
    uint32_t CNT;
    uint32_t ARR;
    uint32_t CCR[2];
    uint32_t DIER;
    uint32_t SR;
    uint32_t CCER;
    uint32_t CCR1;
    uint32_t CCR2;
} test_timer_t;
typedef struct { test_timer_t *Instance; } TIM_HandleTypeDef;
extern TIM_HandleTypeDef htim2, htim3, htim4, htim6;
extern test_timer_t test_tim2;
#define TIM2 (&test_tim2)
#define TIM_DIER_UIE 1U
#define TIM_DIER_CC1IE 2U
#define TIM_DIER_CC2IE 4U
#define TIM_SR_UIF 1U
#define TIM_SR_CC1IF 2U
#define TIM_SR_CC2IF 4U
#define TIM_CHANNEL_1 0U
#define TIM_CHANNEL_2 1U
#define TIM_CHANNEL_ALL 3U
#define TIM_CCMR1_OC1PE (1U << 3)
#define TIM_CCMR1_OC2PE (1U << 11)
#define TIM_FLAG_UPDATE 1U
#define __HAL_TIM_SET_COMPARE(h, ch, val) ((h)->Instance->CCR[ch] = (val))
#define __HAL_TIM_GET_AUTORELOAD(h) ((h)->Instance->ARR)
#define __HAL_TIM_GET_COUNTER(h) ((h)->Instance->CNT)
#define __HAL_TIM_SET_COUNTER(h, val) ((h)->Instance->CNT = (val))
#define __HAL_TIM_CLEAR_FLAG(h, flag) ((void)(h), (void)(flag))
/**
  * @brief  模拟编码器启动，并检查定时器与通道参数
  * @param  h 模拟 TIM3 或 TIM4 句柄
  * @param  ch 编码器通道集合
  * @retval HAL_OK
  * @note   仅用于电脑端测试，不连接实际硬件。
  */
HAL_StatusTypeDef HAL_TIM_Encoder_Start(TIM_HandleTypeDef *h, uint32_t ch);
/**
  * @brief  模拟基础定时器启动并记录中断使能状态
  * @param  h 模拟 TIM2 或 TIM6 句柄
  * @retval HAL_OK
  * @note   仅用于电脑端测试。
  */
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *h);
#endif
