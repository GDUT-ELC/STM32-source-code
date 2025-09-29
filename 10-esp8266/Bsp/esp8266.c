/**********************************************************
 * 文件名: esp8266.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-29
 * 
 * 功能说明:
 * 实现ESP8266模块的初始化流程和数据发布功能，通过AT指令控制模块
 * 完成WiFi连接、MQTT服务器连接及属性数据上报
 * 
 * 接口备注:
 * 依赖UART1通信接口，需确保串口配置正确
 **********************************************************/

#include "esp8266.h"

static char esp_buf[200] = {0};

/**
  * @brief： 向ESP8266模块发送数据
  * @param： txdata - 待发送的数据指针（字符串形式）
  * @retval：None
  */
static void esp8266_write(uint8_t *txdata)
{
	HAL_UART_Transmit(&huart1, txdata, strlen((char*)txdata), HAL_MAX_DELAY);
}

/**
  * @brief： ESP8266模块初始化
  * @param： None
  * @retval：None
  */
void esp8266_init(void)
{
    esp8266_write((uint8_t*)"AT\r\n");
    HAL_Delay(500);

    esp8266_write((uint8_t*)"AT+CWMODE=3\r\n");
    HAL_Delay(500);
	
	sprintf(esp_buf, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_NAME, WIFI_PASSWARD);
    esp8266_write((uint8_t*)esp_buf);
    HAL_Delay(500);
	
	sprintf(esp_buf, "AT+MQTTUSERCFG=0,1,\"NULL\",\"%s\",\"%s\",0,0,\"\"\r\n", USERNAME, PASSWARD);
    esp8266_write((uint8_t*)esp_buf);
    HAL_Delay(1000);
	
	sprintf(esp_buf, "AT+MQTTCLIENTID=0,\"%s\"\r\n", CLIENTID);
    esp8266_write((uint8_t*)esp_buf);
    HAL_Delay(1000);
	
    esp8266_write((uint8_t*)"AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,1\r\n");
    HAL_Delay(1000);
}

/**
  * @brief： 向MQTT服务器发布属性数据
  * @param： 产品ID
  * @param： 设备名称/设备ID
  * @param： 属性标识符
  * @param： 属性值
  * @retval：None
  */
void esp8266_pub(char* id, char* devide, char* flag, int value)
{
	sprintf(esp_buf, "AT+MQTTPUB=0,\"sys/%s/%s/thing/property/post\",\"{\\\"id\\\":\\\"123\\\"\\,\\\"params\\\":{\\\"%s\\\":{\\\"value\\\":%d\\}}}\",0,0", id, devide, flag, value);
}
