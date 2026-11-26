#include "led.h"
#include "tim.h"

volatile HAL_StatusTypeDef g_ws_status = HAL_OK;

/* TIM10: 168MHz / 210 = 800kHz. CCR preload queues the following bit.
 * The 30us frame is atomic; latch time runs with interrupts enabled. */
void WS_SetGRBColor(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t color = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
    uint32_t mask = __get_PRIMASK();
    int32_t bit;
    if (!__HAL_RCC_TIM10_IS_CLK_ENABLED()) {
        g_ws_status = HAL_ERROR;
        return;
    }
    __disable_irq();
    TIM10->CR1 &= ~TIM_CR1_CEN;
    TIM10->CCMR1 |= TIM_CCMR1_OC1PE;
    TIM10->CCER &= ~TIM_CCER_CC1E;
    TIM10->CCR1 = (color & (1UL << 23)) ? 115U : 55U;
    TIM10->EGR = TIM_EGR_UG;
    TIM10->SR = 0U;
    TIM10->CCR1 = (color & (1UL << 22)) ? 115U : 55U;
    TIM10->CCER |= TIM_CCER_CC1E;
    TIM10->CR1 |= TIM_CR1_CEN;
    for (bit = 21; bit >= -2; --bit) {
        /* SysTick cannot advance while interrupts are masked. Bound polling
         * by iterations so a stopped timer cannot block boot/page changes. */
        uint32_t remaining = 4096U;
        while ((TIM10->SR & TIM_SR_UIF) == 0U) {
            if (--remaining == 0U) {
                TIM10->CCR1 = 0U;
                TIM10->EGR = TIM_EGR_UG;
                TIM10->CR1 &= ~TIM_CR1_CEN;
                __set_PRIMASK(mask);
                g_ws_status = HAL_TIMEOUT;
                return;
            }
        }
        TIM10->SR = ~TIM_SR_UIF;
        TIM10->CCR1 = (bit >= 0) ? ((color & (1UL << bit)) ? 115U : 55U) : 0U;
    }
    /* The last update transferred zero duty; the output is now held low. */
    TIM10->CR1 &= ~TIM_CR1_CEN;
    __set_PRIMASK(mask);
    g_ws_status = HAL_OK;
    if (mask == 0U) HAL_Delay(1U);
}
void WS_SetBrightnessColor(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    WS_SetGRBColor((uint8_t)((uint16_t)r * brightness / 255U),
                   (uint8_t)((uint16_t)g * brightness / 255U),
                   (uint8_t)((uint16_t)b * brightness / 255U));
}
