/**********************************************************
 * 文件名: esp8266.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-29
 * 
 * 功能说明:
 * 定义ESP8266模块相关宏定义及函数声明，用于WiFi连接和MQTT通信初始化
 * 
 * 接口备注:
 * 包含WiFi账号密码、MQTT认证信息等宏定义，需根据实际环境修改
 **********************************************************/

#ifndef __ESP8266_H
#define __ESP8266_H

#include "main.h"
#include "usart.h"

// 以下宏定义需要替换为自己的wifi账号和密码
#define WIFI_NAME        ""       // wifi账号
#define WIFI_PASSWARD    ""       // wifi密码

// 以下信息需要填入自己的云端信息
#define USERNAME		 ""       // 用户名
#define PASSWARD	     ""       // 密码
#define CLIENTID		 ""       // clientId

void esp8266_init(void);

#endif
