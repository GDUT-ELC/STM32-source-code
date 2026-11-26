#include "hcsr04.h"
HCSR04Info HCSR04;
void hcsr04_start(void)
{
    uint32_t start;
    uint32_t mask = __get_PRIMASK();
    if (HCSR04.edge_state != 0U) return;
    __disable_irq();
    __HAL_GPIO_EXTI_CLEAR_IT(ECHO_Pin);
    HCSR04.edge_state = 1U;
    HCSR04.started_ms = HAL_GetTick();
    __set_PRIMASK(mask);
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_SET);
    start = __HAL_TIM_GET_COUNTER(&htim5);
    while ((uint32_t)(__HAL_TIM_GET_COUNTER(&htim5) - start) < 15U) {}
    HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET);
}
void hcsr04_echo_irq(void)
{
    uint32_t now = __HAL_TIM_GET_COUNTER(&htim5);
    if (HAL_GPIO_ReadPin(ECHO_GPIO_Port, ECHO_Pin) == GPIO_PIN_SET) {
        if (HCSR04.edge_state == 1U) {
            HCSR04.ring_ccr = now;
            HCSR04.edge_state = 2U;
        }
    } else if (HCSR04.edge_state == 2U) {
        uint32_t width = now - HCSR04.ring_ccr;
        HCSR04.high_time = width;
        HCSR04.valid = (width >= 100U && width <= 25000U);
        HCSR04.distance = HCSR04.valid ? (float)width * 0.0174f : 0.0f;
        if (HCSR04.valid) ++HCSR04.samples;
        HCSR04.edge_state = 0U;
    }
}
void hcsr04_process(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (HCSR04.edge_state != 0U && (uint32_t)(HAL_GetTick() - HCSR04.started_ms) >= 35U) {
        HCSR04.edge_state = 0U;
        HCSR04.valid = 0U;
        HCSR04.distance = 0.0f;
        ++HCSR04.timeouts;
    }
    __set_PRIMASK(mask);
}
float hcsr04_getdistance(HCSR04Info *sensor)
{
    return sensor->valid ? sensor->distance : 0.0f;
}
