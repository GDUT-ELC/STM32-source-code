#include "main.h"
#include "usart.h"
#include "tim.h"
#include "key.h"
#include "hcsr04.h"
#include "esp8266.h"
#include "uart_driver.h"
#include "led.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

UART_HandleTypeDef huart4 = {0}, huart2 = {1}, huart5 = {2};
TIM_HandleTypeDef htim5;
TestTimerRegisters test_tim10;
uint32_t test_tim10_clock;
uint32_t test_mask;
static uint32_t tick;
static unsigned pressed;
static GPIO_PinState echo;
static uint8_t *rx_targets[3];
static char last_command[512];
static HAL_StatusTypeDef transmit_result = HAL_OK;

uint32_t HAL_GetTick(void) { return tick; }
void HAL_Delay(uint32_t delay) { assert(test_mask == 0U); tick += delay; }
GPIO_PinState HAL_GPIO_ReadPin(unsigned port, unsigned pin)
{
    if (port == ECHO_GPIO_Port) return echo;
    return (pressed & pin) ? GPIO_PIN_RESET : GPIO_PIN_SET;
}
void HAL_GPIO_WritePin(unsigned port, unsigned pin, GPIO_PinState state)
{ (void)port; (void)pin; (void)state; }
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *port, uint8_t *byte, uint16_t count)
{
    assert(count == 1);
    rx_targets[port->id] = byte;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *port, uint8_t *data, uint16_t count, uint32_t timeout)
{
    (void)port; (void)timeout;
    assert(count < sizeof(last_command));
    memcpy(last_command, data, count); last_command[count] = '\0';
    return transmit_result;
}
HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *port)
{ rx_targets[port->id] = NULL; return HAL_OK; }
static void receive(UART_HandleTypeDef *port, const char *text)
{
    while (*text) {
        assert(rx_targets[port->id] != NULL);
        *rx_targets[port->id] = (uint8_t)*text++;
        uart_driver_rx_callback(port);
        esp8266_rx_callback(port);
    }
}

static void test_keys(void)
{
    unsigned i;
    assert(key_getnum() == 0);
    pressed = KEY1_Pin; tick = 1; assert(key_getnum() == 0);
    pressed = 0; tick = 10; assert(key_getnum() == 0);
    pressed = KEY1_Pin; tick = 11; assert(key_getnum() == 0);
    tick = 30; assert(key_getnum() == 0);
    tick = 31; assert(key_getnum() == 1);
    for (i = 0; i < 100; ++i) { tick++; assert(key_getnum() == 0); }
    pressed = 0; key_getnum(); tick += 20; assert(key_getnum() == 0);
    /* Debounce time arithmetic also crosses the 32-bit tick wrap. */
    pressed = KEY4_Pin; tick = UINT32_MAX - 10U; assert(key_getnum() == 0);
    tick = 10; assert(key_getnum() == 4);
    pressed = 0; key_getnum(); tick += 20; key_getnum();
    puts("PASS: key bounce, one event while held, tick wrap");
}

static void test_ultrasonic(void)
{
    float distance;
    memset(&HCSR04, 0, sizeof(HCSR04));
    tick = 100;
    hcsr04_start();
    assert(HCSR04.edge_state == 1);
    htim5.counter = UINT32_MAX - 99U; echo = GPIO_PIN_SET; hcsr04_echo_irq();
    htim5.counter = 900; echo = GPIO_PIN_RESET; hcsr04_echo_irq();
    assert(HCSR04.high_time == 1000 && HCSR04.valid && HCSR04.samples == 1);
    distance = hcsr04_getdistance(&HCSR04);
    assert(distance > 17.39f && distance < 17.41f);
    hcsr04_start(); tick += 35; hcsr04_process();
    assert(!HCSR04.valid && HCSR04.edge_state == 0 && HCSR04.timeouts == 1);
    assert(hcsr04_getdistance(&HCSR04) == 0);
    /* A late falling edge after timeout must not revive stale data. */
    hcsr04_echo_irq(); assert(!HCSR04.valid);
    hcsr04_start(); echo = GPIO_PIN_SET; htim5.counter = 1000; hcsr04_echo_irq();
    echo = GPIO_PIN_RESET; htim5.counter = 1100; hcsr04_echo_irq();
    assert(HCSR04.valid && HCSR04.samples == 2);
    hcsr04_start(); echo = GPIO_PIN_SET; hcsr04_echo_irq();
    echo = GPIO_PIN_RESET; htim5.counter += 30000; hcsr04_echo_irq();
    assert(!HCSR04.valid);
    assert(test_mask == 0);
    puts("PASS: echo measurement, timer wrap, missing/late/out-of-range echo, recovery");
}

static void test_uart(void)
{
    uint8_t data[128]; uint16_t length; unsigned i;
    assert(uart_driver_init() == HAL_OK);
    tick = 100;
    receive(&huart4, "UART4"); receive(&huart2, "USART2");
    tick += 4;
    assert(uart_driver_read_packet(data, sizeof(data), &length) == UART_DRIVER_RX_NONE);
    tick++;
    assert(uart_driver_read_packet(data, sizeof(data), &length) == UART_DRIVER_RX_READY);
    assert(length == 5 && memcmp(data, "UART4", 5) == 0);
    assert(uart_driver_read_port(&huart2, data, sizeof(data), &length) == UART_DRIVER_RX_READY);
    assert(length == 6 && memcmp(data, "USART2", 6) == 0);
    for (i = 0; i < 129; ++i) receive(&huart4, "x");
    tick += 5;
    assert(uart_driver_read_packet(data, sizeof(data), &length) == UART_DRIVER_RX_OVERFLOW);
    assert(length == 0);
    receive(&huart4, "recover"); tick += 5;
    assert(uart_driver_read_packet(data, sizeof(data), &length) == UART_DRIVER_RX_READY);
    uart_driver_error_callback(&huart4);
    assert(uart_driver_read_packet(data, sizeof(data), &length) == UART_DRIVER_RX_ERROR);
    receive(&huart4, "ok"); tick += 5;
    assert(uart_driver_read_packet(data, sizeof(data), &length) == UART_DRIVER_RX_READY);
    assert(length == 2 && data[0] == 'o');
    assert(test_mask == 0);
    puts("PASS: independent UARTs, idle boundary, overflow and UART error recovery");
}

static void test_esp(void)
{
    tick = UINT32_MAX - 100U;
    assert(esp8266_init() == HAL_OK);
    receive(&huart5, "AT\r\n"); esp8266_process();
    assert(g_esp_state == ESP_WAIT_AT); /* An echoed command is not an ACK. */
    tick += 1500; esp8266_process(); assert(g_esp_state == ESP_TIMEOUT);
    tick = 100;
    assert(esp8266_init() == HAL_OK);
    receive(&huart5, "\r\nO"); esp8266_process(); assert(g_esp_state == ESP_WAIT_AT);
    receive(&huart5, "K\r\n"); esp8266_process();
    if (WIFI_NAME[0] == '\0') {
        assert(g_esp_state == ESP_AT_OK);
    } else {
        assert(g_esp_state == ESP_WAIT_MODE);
        receive(&huart5, "OK\r\n"); esp8266_process();
        assert(g_esp_state == ESP_WAIT_WIFI && strstr(last_command, "AT+CWJAP="));
        receive(&huart5, "WIFI CONNECTED\r\n"); esp8266_process();
        assert(g_esp_state == ESP_WAIT_WIFI);
        receive(&huart5, "OK\r\n"); esp8266_process();
        assert(g_esp_state == ESP_WAIT_MQTT_CONFIG);
        receive(&huart5, "OK\r\n"); esp8266_process();
        assert(g_esp_state == ESP_WAIT_MQTT_CONNECT);
        receive(&huart5, "OK\r\n"); esp8266_process();
        assert(g_esp_state == ESP_MQTT_OK);
        assert(esp8266_pub("p", "d", "value", 1) == HAL_OK);
        receive(&huart5, "ERROR\r\n"); esp8266_process(); assert(g_esp_state == ESP_ERROR);
    }
    assert(esp8266_init() == HAL_OK);
    receive(&huart5, "ERROR\r\nOK\r\n"); esp8266_process(); assert(g_esp_state == ESP_ERROR);
    transmit_result = HAL_ERROR;
    assert(esp8266_init() == HAL_ERROR && g_esp_state == ESP_ERROR);
    transmit_result = HAL_OK;
    assert(test_mask == 0);
    puts("PASS: ESP fragmented replies, actual ACKs, stage progression, error and timeout");
}
static void test_ws2812_stopped_timer(void)
{
    /* Reproduce the boot/page-switch failure: no clock, then a timer that
     * never raises UIF. Both calls must return without a ticking SysTick. */
    memset(&test_tim10, 0, sizeof(test_tim10));
    test_tim10_clock = 0;
    test_mask = 0;
    WS_SetGRBColor(0, 0, 0);
    assert(g_ws_status == HAL_ERROR && test_mask == 0U);
    assert(test_tim10.CR1 == 0U && test_tim10.CCER == 0U);
    test_tim10_clock = 1;
    WS_SetGRBColor(0, 0, 0);
    assert(g_ws_status == HAL_TIMEOUT && test_mask == 0U);
    assert((test_tim10.CR1 & TIM_CR1_CEN) == 0U && test_tim10.CCR1 == 0U);
    test_mask = 1;
    WS_SetGRBColor(255, 128, 64);
    assert(g_ws_status == HAL_TIMEOUT && test_mask == 1U);
    test_mask = 0;
    puts("PASS: WS2812 missing clock/stalled timer returns; interrupt mask restored");
}
int main(void)
{
    test_keys(); test_ultrasonic(); test_uart(); test_esp(); test_ws2812_stopped_timer();
    puts("All integration driver tests passed.");
    return 0;
}
