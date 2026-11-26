#include "key.h"
/* One event per press after 20ms stability; holding a key never blocks tasks. */
uint8_t key_getnum(void)
{
    static uint8_t candidate, stable;
    static uint32_t changed_at;
    uint8_t raw = 0;
    uint32_t now = HAL_GetTick();
    if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET) raw = 1;
    else if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) raw = 2;
    else if (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_RESET) raw = 3;
    else if (HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin) == GPIO_PIN_RESET) raw = 4;
    if (raw != candidate) { candidate = raw; changed_at = now; }
    if (stable != candidate && (uint32_t)(now - changed_at) >= 20U) {
        stable = candidate;
        return stable;
    }
    return 0;
}
