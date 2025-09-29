#include "esp8266.h"

static char esp_buf[100] = {0};

static void esp8266_write(uint8_t *txdata)
{
	HAL_UART_Transmit(&huart1, txdata, strlen((char*)txdata), HAL_MAX_DELAY);
}

void esp8266_init(void)
{
    esp8266_write((uint8_t*)"AT\r\n");
    HAL_Delay(500);

    esp8266_write((uint8_t*)"AT+CWMODE=3\r\n");
    HAL_Delay(500);
	
	sprintf(esp_buf, "AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_NAME, WIFI_PASSWARD);
    esp8266_write((uint8_t*)esp_buf);
    HAL_Delay(500);

    esp8266_write((uint8_t*)"AT+CIPSNTPCFG=1,8,\"ntp1.aliyun.com\"\r\n");
    HAL_Delay(1000);
	
    esp8266_write((uint8_t*)"AT+MQTTUSERCFG=0,1,\"NULL\",\"8266Driver1&k1n4cMTpnH3\",\"56AB235955E5B97A29EF82AB85EAF7A38527BFBB\",0,0,\"\"\r\n");
    HAL_Delay(1000);
	
    esp8266_write((uint8_t*)"AT+MQTTCLIENTID=0,\"test1|securemode=3\\,signmethod=hmacsha1|\"\r\n");
    HAL_Delay(1000);
	
    esp8266_write((uint8_t*)"AT+MQTTCONN=0,\"k1n4cMTpnH3.iot-as-mqtt.cn-shanghai.aliyuncs.com\",1883,1\r\n");
    HAL_Delay(1000);
}
