/**********************************************************
 * 文件名: hcsr04.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-29
 * 
 * 功能说明:
 * 定义HC-SR04超声波测距模块的驱动接口及数据结构，
 * 包含宏定义、结构体类型和函数声明
 * 
 * 接口备注:
 * 1.TRIG_H/TRIG_L宏需根据硬件接线在宏定义中配置对应的GPIO端口和引脚
 * 2.依赖定时器驱动(tim.h)和主头文件(main.h)
 **********************************************************/

#ifndef __HCSR04_H
#define __HCSR04_H

#include "main.h"
#include "tim.h"

#define TRIG_H  HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_SET)    // 触发信号置高
#define TRIG_L  HAL_GPIO_WritePin(TRIG_GPIO_Port, TRIG_Pin, GPIO_PIN_RESET)  // 触发信号置低

typedef struct
{
    uint8_t edge_state;         // 边沿检测状态(0:等待上升沿,1:等待下降沿)
    uint16_t flow_counter;      // 定时器溢出计数器
    uint32_t ring_ccr;          // 上升沿捕获值
    uint32_t fall_ccr;          // 下降沿捕获值
    uint32_t high_time;         // 回波高电平持续时间(微秒)
    float distance;             // 计算得到的距离(厘米)
} HCSR04Info;

extern HCSR04Info HCSR04;

void hcsr04_start(void);
void hcsr04_timeflow_handle(HCSR04Info *hcsr04);
void hcsr04_incapture_handle(TIM_HandleTypeDef *htim, HCSR04Info *hcsr04);
float hcsr04_getdistance(HCSR04Info *hcsr04);

#endif

