/**********************************************************
 * 文件名: buzzer.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-28
 * 
 * 功能说明:
 *   提供蜂鸣器的控制函数，包括开启和关闭
 *   功能，基于HAL库GPIO接口实现
 * 
 * 实现备注:
 *   蜂鸣器采用高电平有效方式控制(GPIO_PIN_SET开启)
 **********************************************************/

#include "buzzer.h"

/**
 * @brief  开启蜂鸣器
 * @param  None
 * @retval None
 */
void buzzer_on(void)
{
	HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
}

/**
 * @brief  关闭蜂鸣器
 * @param  None
 * @retval None
 */
void buzzer_off(void)
{
	HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
}
