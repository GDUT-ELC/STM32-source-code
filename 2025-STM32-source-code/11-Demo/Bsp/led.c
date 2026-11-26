/**********************************************************
 * 文件名: led.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-28
 * 
 * 功能说明:
 *   提供红、绿、蓝三色LED的控制函数，包括点亮、关闭
 *   和状态切换功能，基于HAL库GPIO接口实现
 * 
 * 实现备注:
 *   LED采用低电平有效方式控制(GPIO_PIN_RESET点亮)
 **********************************************************/

#include "led.h"

/**
 * @brief  点亮红色LED
 * @param  None
 * @retval None
 */
void led_red_on(void)
{
	HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  关闭红色LED
 * @param  None
 * @retval None
 */
void led_red_off(void)
{
	HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
}

/**
 * @brief  切换红色LED状态（亮→灭/灭→亮）
 * @param  None
 * @retval None
 */
void led_red_toggle(void)
{
	HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
}

/**
 * @brief  点亮绿色LED
 * @param  None
 * @retval None
 */
void led_green_on(void)
{
	HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  关闭绿色LED
 * @param  None
 * @retval None
 */
void led_green_off(void)
{
	HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
}

/**
 * @brief  切换绿色LED状态（亮→灭/灭→亮）
 * @param  None
 * @retval None
 */
void led_green_toggle(void)
{
	HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
}

/**
 * @brief  点亮蓝色LED
 * @param  None
 * @retval None
 */
void led_blue_on(void)
{
	HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
}

/**
 * @brief  关闭蓝色LED
 * @param  None
 * @retval None
 */
void led_blue_off(void)
{
	HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
}

/**
 * @brief  切换蓝色LED状态（亮→灭/灭→亮）
 * @param  None
 * @retval None
 */
void led_blue_toggle(void)
{
	HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
}


