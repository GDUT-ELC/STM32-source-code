#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
typedef enum { HAL_OK = 0, HAL_ERROR = 1 } HAL_StatusTypeDef;
typedef struct { uint32_t BSRR; } GPIO_TypeDef;
extern GPIO_TypeDef test_gpiod;
#define GPIOD (&test_gpiod)
#define AIN1_Pin (1U << 11)
#define AIN2_Pin (1U << 10)
#define BIN1_Pin (1U << 8)
#define BIN2_Pin (1U << 9)
#define AIN1_GPIO_Port (&test_gpiod)
#define BIN1_GPIO_Port (&test_gpiod)
extern uint32_t test_primask;
/**
  * @brief  读取模拟的中断屏蔽状态
  * @param  None
  * @retval 模拟 PRIMASK 状态
  */
static inline uint32_t __get_PRIMASK(void) { return test_primask; }
/**
  * @brief  将模拟中断状态设为屏蔽
  * @param  None
  * @retval None
  */
static inline void __disable_irq(void) { test_primask = 1; }
/**
  * @brief  恢复模拟中断屏蔽状态
  * @param  key 需要恢复的模拟状态
  * @retval None
  */
static inline void __set_PRIMASK(uint32_t key) { test_primask = key; }
#define CLEAR_BIT(reg, bits) ((reg) &= ~(bits))
extern uint32_t test_tim2_clock, test_gpiod_clock;
#define __HAL_RCC_TIM2_IS_CLK_ENABLED() (test_tim2_clock != 0U)
#define __HAL_RCC_GPIOD_IS_CLK_ENABLED() (test_gpiod_clock != 0U)
#endif
