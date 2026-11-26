/**********************************************************
 * 文件名: esp8266.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 * 封装ESP8266的WiFi连接和MQTT通信功能，包括模块初始化、
 * 属性数据发布及属性上报结果主题订阅。
 *
 * 接口备注:
 * 1. 使用UART5与ESP8266通信，调用前需先完成MX_UART5_Init()
 * 2. WiFi和云平台参数在esp8266.h中配置
 * 3. 返回值只表示串口发送是否成功，不代表ESP8266已正确回复AT指令
 **********************************************************/

#include "esp8266.h"
#include <stdio.h>
#include <string.h>

#define ESP8266_BUFFER_SIZE       200U
#define ESP8266_UART_TIMEOUT_MS  1000U

static char esp_buf[ESP8266_BUFFER_SIZE] = {0};

/**
  * @brief  通过UART5向ESP8266发送字符串
  * @param  txdata: 待发送的字符串
  * @retval HAL_OK: 发送成功；HAL_ERROR/HAL_TIMEOUT: 发送失败
  */
static HAL_StatusTypeDef esp8266_write(const char *txdata)
{
    if (txdata == NULL)
    {
        return HAL_ERROR;
    }

    return HAL_UART_Transmit(&huart5,
                             (uint8_t *)txdata,
                             (uint16_t)strlen(txdata),
                             ESP8266_UART_TIMEOUT_MS);
}

/**
  * @brief  发送一条AT指令，并在发送成功后等待模块处理
  * @param  command: AT指令字符串
  * @param  wait_ms: 指令发送后的等待时间，单位ms
  * @retval HAL_OK: 发送成功；其他值: 发送失败
  */
static HAL_StatusTypeDef esp8266_send_command(const char *command, uint32_t wait_ms)
{
    HAL_StatusTypeDef status;

    status = esp8266_write(command);
    if (status == HAL_OK)
    {
        HAL_Delay(wait_ms);
    }

    return status;
}

/**
  * @brief  初始化ESP8266，并连接WiFi和MQTT服务器
  * @param  None
  * @retval HAL_OK: 所有AT指令已成功发送；HAL_ERROR/HAL_TIMEOUT: 发送失败
  * @note   本函数未读取ESP8266返回内容，因此HAL_OK不等同于联网成功
  */
HAL_StatusTypeDef esp8266_init(void)
{
    int length;

    if (esp8266_send_command("AT\r\n", 500U) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if (esp8266_send_command("AT+CWMODE=1\r\n", 500U) != HAL_OK)
    {
        return HAL_ERROR;
    }

    length = snprintf(esp_buf,
                      sizeof(esp_buf),
                      "AT+CWJAP=\"%s\",\"%s\"\r\n",
                      WIFI_NAME,
                      WIFI_PASSWORD);
    if ((length < 0) || ((size_t)length >= sizeof(esp_buf)))
    {
        return HAL_ERROR;
    }
    if (esp8266_send_command(esp_buf, 1500U) != HAL_OK)
    {
        return HAL_ERROR;
    }

    length = snprintf(esp_buf,
                      sizeof(esp_buf),
                      "AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",
                      MQTT_CLIENT_ID,
                      MQTT_USERNAME,
                      MQTT_PASSWORD);
    if ((length < 0) || ((size_t)length >= sizeof(esp_buf)))
    {
        return HAL_ERROR;
    }
    if (esp8266_send_command(esp_buf, 1500U) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return esp8266_send_command(
        "AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,1\r\n",
        1000U);
}

/**
  * @brief  向MQTT服务器发布一项设备属性数据
  * @param  product_id: 产品ID字符串
  * @param  device_name: 设备名称或设备ID字符串
  * @param  property_name: 属性标识符字符串
  * @param  value: 要上报的整数属性值
  * @retval HAL_OK: AT指令已成功发送；HAL_ERROR/HAL_TIMEOUT: 发送失败
  * @note   返回HAL_OK不代表云平台已经接收到数据
  */
HAL_StatusTypeDef esp8266_pub(const char *product_id,
                             const char *device_name,
                             const char *property_name,
                             int value)
{
    int length;

    if ((product_id == NULL) ||
        (device_name == NULL) ||
        (property_name == NULL))
    {
        return HAL_ERROR;
    }

    length = snprintf(esp_buf,
                      sizeof(esp_buf),
                      "AT+MQTTPUB=0,\"$sys/%s/%s/thing/property/post\","
                      "\"{\\\"id\\\":\\\"1\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%d}}}\",0,0\r\n",
                      product_id,
                      device_name,
                      property_name,
                      value);
    if ((length < 0) || ((size_t)length >= sizeof(esp_buf)))
    {
        return HAL_ERROR;
    }

    return esp8266_write(esp_buf);
}

/**
  * @brief  订阅设备属性上报结果主题
  * @param  product_id: 产品ID字符串
  * @param  device_name: 设备名称或设备ID字符串
  * @retval HAL_OK: AT指令已成功发送；HAL_ERROR/HAL_TIMEOUT: 发送失败
  * @note   返回HAL_OK不代表云平台已经确认订阅
  */
HAL_StatusTypeDef esp8266_sub(const char *product_id, const char *device_name)
{
    int length;

    if ((product_id == NULL) || (device_name == NULL))
    {
        return HAL_ERROR;
    }

    length = snprintf(esp_buf,
                      sizeof(esp_buf),
                      "AT+MQTTSUB=0,\"$sys/%s/%s/thing/property/post/reply\",0\r\n",
                      product_id,
                      device_name);
    if ((length < 0) || ((size_t)length >= sizeof(esp_buf)))
    {
        return HAL_ERROR;
    }

    return esp8266_write(esp_buf);
}
