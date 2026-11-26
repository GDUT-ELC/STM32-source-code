/**********************************************************
 * 文件名: esp8266.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 * 
 * 功能说明:
 * 定义ESP8266模块的配置参数和公开接口，用于WiFi连接、
 * MQTT服务器连接、属性数据发布及主题订阅。
 * 
 * 接口备注:
 * 1. 使用前需填写WiFi和云平台认证参数
 * 2. ESP8266通过UART5通信，调用接口前需先初始化UART5
 * 3. 接口返回值仅表示串口发送状态，不表示AT指令执行结果
 **********************************************************/

#ifndef __ESP8266_H
#define __ESP8266_H

#include "main.h"
#include "usart.h"

/* 以下宏定义需要替换为自己的WiFi账号和密码 */
#define WIFI_NAME        ""       /* WiFi账号 */
#define WIFI_PASSWORD    ""       /* WiFi密码 */

/* 以下宏定义需要替换为自己的云平台认证信息 */
#define MQTT_USERNAME    ""       /* 用户名 */
#define MQTT_PASSWORD    ""       /* 密码 */
#define MQTT_CLIENT_ID   ""       /* 客户端ID */

HAL_StatusTypeDef esp8266_init(void);
HAL_StatusTypeDef esp8266_pub(const char *product_id,
                             const char *device_name,
                             const char *property_name,
                             int value);
HAL_StatusTypeDef esp8266_sub(const char *product_id,
                             const char *device_name);

#endif
