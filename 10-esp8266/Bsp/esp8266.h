#ifndef __ESP8266_H
#define __ESP8266_H

#include "main.h"
#include "usart.h"

// 以下宏定义需要替换为自己的wifi账号和密码
#define WIFI_NAME        ""       // wifi账号
#define WIFI_PASSWARD    ""       // wifi密码

// 以下信息需要填入自己的云端信息
#define PROID		""            //产品ID
#define AUTH_INFO	""            //鉴权信息token
#define DEVID		""            //设备名称

void esp8266_init(void);

#endif
