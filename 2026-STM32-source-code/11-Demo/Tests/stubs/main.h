#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
#include <stddef.h>
typedef enum { HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef enum { GPIO_PIN_RESET, GPIO_PIN_SET } GPIO_PinState;
typedef struct { unsigned id; } UART_HandleTypeDef;
typedef struct { uint32_t counter; } TIM_HandleTypeDef;
extern uint32_t test_mask;
static inline uint32_t __get_PRIMASK(void) { return test_mask; }
static inline void __disable_irq(void) { test_mask = 1; }
static inline void __set_PRIMASK(uint32_t value) { test_mask = value; }
#define KEY1_GPIO_Port 1U
#define KEY2_GPIO_Port 1U
#define KEY3_GPIO_Port 1U
#define KEY4_GPIO_Port 1U
#define KEY1_Pin 1U
#define KEY2_Pin 2U
#define KEY3_Pin 4U
#define KEY4_Pin 8U
#define ECHO_GPIO_Port 2U
#define ECHO_Pin 128U
#define TRIG_GPIO_Port 2U
#define TRIG_Pin 64U
#define __HAL_GPIO_EXTI_CLEAR_IT(pin) ((void)(pin))
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t delay);
GPIO_PinState HAL_GPIO_ReadPin(unsigned port, unsigned pin);
void HAL_GPIO_WritePin(unsigned port, unsigned pin, GPIO_PinState state);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *port, uint8_t *byte, uint16_t count);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *port, uint8_t *data, uint16_t count, uint32_t timeout);
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *port);
#endif
