/**********************************************************
 * 文件名: motor.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-28
 * 
 * 功能说明:
 *   实现电机控制的具体逻辑，通过GPIO引脚控制电机转向，
 *   利用定时器TIM12输出PWM信号调节电机速度
 * 
 * 实现备注:
 *   1. 需确保定时器相关定时器已正确初始化
 *   2. 电机1通过AIN1、AIN2引脚控制方向，使用TIM12_CH1输出PWM
 *   3. 电机2通过BIN1、BIN2引脚控制方向，使用TIM12_CH2输出PWM
 *   4. 速度参数取绝对值后作为PWM比较值，正负值决定转向切换
 **********************************************************/

#include "motor.h"
#include "tim.h"

void motor_setspeed1(int8_t Speed)
{
	if (Speed >= 0)		//如果设置正转的速度值
	{
		HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);	
		HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_SET);
		__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_1, Speed);
	}
	else	 //否则，即设置反转的速度值
	{
		HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_SET);	
		HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
		__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_1, -Speed);
	}
}

void motor_setspeed2(int8_t Speed)
{
	if (Speed >= 0)	    //如果设置正转的速度值
	{
		HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_RESET);	
		HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_SET);
		__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, Speed);
	}
	else    //否则，即设置反转的速度值
	{
		HAL_GPIO_WritePin(BIN1_GPIO_Port, BIN1_Pin, GPIO_PIN_SET);	
		HAL_GPIO_WritePin(BIN2_GPIO_Port, BIN2_Pin, GPIO_PIN_RESET);
		__HAL_TIM_SET_COMPARE(&htim12, TIM_CHANNEL_2, -Speed);
	}
}
