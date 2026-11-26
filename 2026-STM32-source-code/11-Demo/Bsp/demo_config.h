#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

/* WiFi/MQTT fields are optional. Empty WiFi name runs the AT hardware test. */
#ifndef WIFI_NAME
#define WIFI_NAME        ""
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD    ""
#endif
#ifndef MQTT_USERNAME
#define MQTT_USERNAME    ""
#endif
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD    ""
#endif
#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID   ""
#endif
#ifndef MQTT_HOST
#define MQTT_HOST        "mqtts.heclouds.com"
#endif
#ifndef MQTT_PORT
#define MQTT_PORT        1883
#endif

/* Same Flash test address and data as project 12. Reserve this sector. */
#define DEMO_FLASH_TEST_ADDR  0x001000UL
#define DEMO_MOTOR_A_CPS      99000.0f
#define DEMO_MOTOR_B_CPS      99000.0f
#endif
