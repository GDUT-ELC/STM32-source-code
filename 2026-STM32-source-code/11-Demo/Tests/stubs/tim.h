#ifndef TEST_TIM_H
#define TEST_TIM_H
#include "main.h"
typedef struct {
    volatile uint32_t CR1, CCMR1, CCER, CCR1, EGR, SR;
} TestTimerRegisters;
extern TestTimerRegisters test_tim10;
extern uint32_t test_tim10_clock;
#define TIM10 (&test_tim10)
#define TIM_CR1_CEN 1U
#define TIM_CCMR1_OC1PE 8U
#define TIM_CCER_CC1E 1U
#define TIM_EGR_UG 1U
#define TIM_SR_UIF 1U
#define __HAL_RCC_TIM10_IS_CLK_ENABLED() (test_tim10_clock != 0U)
extern TIM_HandleTypeDef htim5;
#define __HAL_TIM_GET_COUNTER(timer) ((timer)->counter++)
#endif
