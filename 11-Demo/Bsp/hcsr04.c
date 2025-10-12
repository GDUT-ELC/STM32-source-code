/**********************************************************
 * 文件名: hcsr04.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-29
 * 
 * 功能说明:
 * 实现HC-SR04超声波测距模块的驱动功能，包括触发信号发送、
 * 回波信号捕获及距离计算，通过定时器输入捕获功能实现高精度计时
 * 
 * 接口备注:
 * 1.需配合定时器中断使用，通过hcsr04_timeflow_handle处理定时器溢出
 * 2.通过hcsr04_incapture_handle处理回波边沿捕获
 **********************************************************/

#include "hcsr04.h"

HCSR04Info HCSR04;

/**
  * @brief： 微秒级延时函数
  * @param： None
  * @retval：None
  */
static void hcsr04_delay_us(uint32_t us)
{
	// 每次循环大约消耗 6 个时钟周期
    // 168MHz => 1 微秒 = 168 个时钟周期
    // 所以每个微秒大约需要 168 / 6 ≈ 28 次循环
    uint32_t n = us * 28;
    volatile uint32_t i;
	
    for (i = 0; i < n; i++) 
	{
        __NOP();
		__NOP();
    }
}

/**
  * @brief： 发送超声波起始信号
  * @param： None
  * @retval：None
  */
void hcsr04_start(void) 
{
    TRIG_H;
	hcsr04_delay_us(30);
	TRIG_L;
}

/**
  * @brief： 处理定时器溢出，用于计算回波持续时间
  * @param： hcsr04 - HC-SR04设备信息结构体指针
  * @retval：None
  */
void hcsr04_timeflow_handle(HCSR04Info *hcsr04) 
{
    hcsr04->flow_counter++;
}

/**
  * @brief： 处理输入捕获中断，计算回波高电平时间并转换为距离
  * @param： htim - 定时器句柄指针
  * @param： hcsr04 - HC-SR04设备信息结构体指针
  * @retval：None
  */
void hcsr04_incapture_handle(TIM_HandleTypeDef *htim, HCSR04Info *hcsr04)
{
    if (hcsr04->edge_state == 0)
	{
        hcsr04->ring_ccr = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        __HAL_TIM_SET_CAPTUREPOLARITY(&htim3, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_FALLING);

        hcsr04->flow_counter = 0;
        hcsr04->edge_state = 1;
    }
	else if (hcsr04->edge_state == 1)
	{
        hcsr04->fall_ccr = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
        hcsr04->fall_ccr += hcsr04->flow_counter * htim->Init.Period;

        hcsr04->high_time = hcsr04->fall_ccr - hcsr04->ring_ccr;
        hcsr04->distance = (hcsr04->high_time / 1000000.0f) * 348.0f / 2 * 100;

        hcsr04->edge_state = 0;
        __HAL_TIM_SET_CAPTUREPOLARITY(htim, TIM_CHANNEL_2, TIM_INPUTCHANNELPOLARITY_RISING);
    }
}

/**
  * @brief： 获取测量的距离值
  * @param： hcsr04 - HC-SR04设备信息结构体指针
  * @retval：float - 距离值，单位为厘米(cm)
  */
float hcsr04_getdistance(HCSR04Info *hcsr04) 
{
    return hcsr04->distance;
}
