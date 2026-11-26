#include "esp8266.h"
#include <stdio.h>
#include <string.h>
volatile esp8266_state_t g_esp_state = ESP_IDLE;
volatile uint32_t g_esp_rx_bytes;
static uint8_t rx_byte;
static uint8_t rx_started;
static char line[256], command[384];
static uint16_t line_length;
static uint8_t line_overflow;
static volatile uint8_t reply_ok, reply_error, disconnected;
static uint32_t command_tick, command_timeout;
static uint8_t waiting(void)
{
    return (g_esp_state >= ESP_WAIT_AT && g_esp_state <= ESP_WAIT_MQTT_CONNECT)
        || g_esp_state == ESP_WAIT_PUB || g_esp_state == ESP_WAIT_SUB;
}
static HAL_StatusTypeDef send_command(const char *text, esp8266_state_t state, uint32_t timeout)
{
    uint32_t mask = __get_PRIMASK();
    HAL_StatusTypeDef result;
    __disable_irq();
    reply_ok = reply_error = disconnected = 0;
    line_length = 0; line_overflow = 0;
    g_esp_state = state;
    command_tick = HAL_GetTick(); command_timeout = timeout;
    __set_PRIMASK(mask);
    result = HAL_UART_Transmit(&huart5, (uint8_t *)text, (uint16_t)strlen(text), 100U);
    if (result != HAL_OK) g_esp_state = ESP_ERROR;
    return result;
}
HAL_StatusTypeDef esp8266_init(void)
{
    /* Abort only RX; UART5 has a single owner in this application. */
    (void)HAL_UART_AbortReceive(&huart5);
    line_length = 0; line_overflow = 0;
    rx_started = 0;
    if (HAL_UART_Receive_IT(&huart5, &rx_byte, 1U) != HAL_OK) {
        g_esp_state = ESP_ERROR;
        return HAL_ERROR;
    }
    rx_started = 1;
    return send_command("AT\r\n", ESP_WAIT_AT, 1500U);
}
void esp8266_rx_callback(UART_HandleTypeDef *port)
{
    if (port != &huart5 || !rx_started) return;
    ++g_esp_rx_bytes;
    if (rx_byte == '\n') {
        line[line_length] = '\0';
        if (!line_overflow) {
            if (strcmp(line, "OK") == 0) reply_ok = 1;
            if (strstr(line, "ERROR") || strstr(line, "FAIL") || strstr(line, "busy")) reply_error = 1;
            if (strstr(line, "WIFI DISCONNECT") || strstr(line, "+MQTTDISCONNECTED")) disconnected = 1;
        }
        line_length = 0; line_overflow = 0;
    } else if (rx_byte != '\r') {
        if (line_length < sizeof(line) - 1U) line[line_length++] = (char)rx_byte;
        else line_overflow = 1;
    }
    if (HAL_UART_Receive_IT(&huart5, &rx_byte, 1U) != HAL_OK) reply_error = 1;
}
void esp8266_error_callback(UART_HandleTypeDef *port)
{
    if (port != &huart5 || !rx_started) return;
    reply_error = 1;
    (void)HAL_UART_AbortReceive(port);
    (void)HAL_UART_Receive_IT(port, &rx_byte, 1U);
}
void esp8266_process(void)
{
    int length;
    if (!waiting()) {
        if (disconnected && (g_esp_state == ESP_WIFI_OK || g_esp_state == ESP_MQTT_OK)) g_esp_state = ESP_ERROR;
        return;
    }
    if (reply_error) { g_esp_state = ESP_ERROR; return; }
    if (!reply_ok) {
        if ((uint32_t)(HAL_GetTick() - command_tick) >= command_timeout) g_esp_state = ESP_TIMEOUT;
        return;
    }
    switch (g_esp_state) {
    case ESP_WAIT_AT:
        if (WIFI_NAME[0] == '\0') { g_esp_state = ESP_AT_OK; break; }
        (void)send_command("AT+CWMODE=1\r\n", ESP_WAIT_MODE, 2000U);
        break;
    case ESP_WAIT_MODE:
        length = snprintf(command, sizeof(command), "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_NAME, WIFI_PASSWORD);
        if (length < 0 || (size_t)length >= sizeof(command)) g_esp_state = ESP_ERROR;
        else (void)send_command(command, ESP_WAIT_WIFI, 20000U);
        break;
    case ESP_WAIT_WIFI:
        if (MQTT_CLIENT_ID[0] == '\0' || MQTT_USERNAME[0] == '\0' || MQTT_PASSWORD[0] == '\0') {
            g_esp_state = ESP_WIFI_OK; break;
        }
        length = snprintf(command, sizeof(command), "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
                          MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
        if (length < 0 || (size_t)length >= sizeof(command)) g_esp_state = ESP_ERROR;
        else (void)send_command(command, ESP_WAIT_MQTT_CONFIG, 3000U);
        break;
    case ESP_WAIT_MQTT_CONFIG:
        length = snprintf(command, sizeof(command), "AT+MQTTCONN=0,\"%s\",%d,1\r\n", MQTT_HOST, MQTT_PORT);
        if (length < 0 || (size_t)length >= sizeof(command)) g_esp_state = ESP_ERROR;
        else (void)send_command(command, ESP_WAIT_MQTT_CONNECT, 15000U);
        break;
    case ESP_WAIT_MQTT_CONNECT: case ESP_WAIT_PUB: case ESP_WAIT_SUB:
        g_esp_state = ESP_MQTT_OK;
        break;
    default: break;
    }
}
const char *esp8266_status_string(void)
{
    switch (g_esp_state) {
    case ESP_IDLE: return "BOOT WAIT";
    case ESP_WAIT_AT: return "WAIT AT";
    case ESP_WAIT_MODE: return "SET STA MODE";
    case ESP_WAIT_WIFI: return "JOIN WIFI";
    case ESP_WAIT_MQTT_CONFIG: return "MQTT CONFIG";
    case ESP_WAIT_MQTT_CONNECT: return "MQTT CONNECT";
    case ESP_AT_OK: return "AT PASS";
    case ESP_WIFI_OK: return "WIFI PASS";
    case ESP_MQTT_OK: return "MQTT PASS";
    case ESP_TIMEOUT: return "TIMEOUT";
    case ESP_WAIT_PUB: return "PUBLISH WAIT";
    case ESP_WAIT_SUB: return "SUBSCRIBE WAIT";
    default: return "ERROR";
    }
}
HAL_StatusTypeDef esp8266_pub(const char *product, const char *device, const char *property, int value)
{
    int length;
    if (!product || !device || !property) return HAL_ERROR;
    if (g_esp_state != ESP_MQTT_OK) return HAL_BUSY;
    length = snprintf(command, sizeof(command),
        "AT+MQTTPUB=0,\"$sys/%s/%s/thing/property/post\","
        "\"{\\\"id\\\":\\\"1\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%d}}}\",0,0\r\n",
        product, device, property, value);
    if (length < 0 || (size_t)length >= sizeof(command)) return HAL_ERROR;
    return send_command(command, ESP_WAIT_PUB, 5000U);
}
HAL_StatusTypeDef esp8266_sub(const char *product, const char *device)
{
    int length;
    if (!product || !device) return HAL_ERROR;
    if (g_esp_state != ESP_MQTT_OK) return HAL_BUSY;
    length = snprintf(command, sizeof(command),
        "AT+MQTTSUB=0,\"$sys/%s/%s/thing/property/post/reply\",0\r\n", product, device);
    if (length < 0 || (size_t)length >= sizeof(command)) return HAL_ERROR;
    return send_command(command, ESP_WAIT_SUB, 5000U);
}
