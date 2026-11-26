#ifndef ESP8266_H
#define ESP8266_H
#include "main.h"
#include "usart.h"
#include "demo_config.h"
typedef enum {
    ESP_IDLE = 0, ESP_WAIT_AT, ESP_WAIT_MODE, ESP_WAIT_WIFI,
    ESP_WAIT_MQTT_CONFIG, ESP_WAIT_MQTT_CONNECT, ESP_AT_OK, ESP_WIFI_OK,
    ESP_MQTT_OK, ESP_ERROR, ESP_TIMEOUT, ESP_WAIT_PUB, ESP_WAIT_SUB
} esp8266_state_t;
extern volatile esp8266_state_t g_esp_state;
extern volatile uint32_t g_esp_rx_bytes;
/* Starts an asynchronous test. HAL_OK means started; inspect state for the result. */
HAL_StatusTypeDef esp8266_init(void);
void esp8266_process(void);
void esp8266_rx_callback(UART_HandleTypeDef *port);
void esp8266_error_callback(UART_HandleTypeDef *port);
const char *esp8266_status_string(void);
HAL_StatusTypeDef esp8266_pub(const char *product, const char *device,
                             const char *property, int value);
HAL_StatusTypeDef esp8266_sub(const char *product, const char *device);
#endif
