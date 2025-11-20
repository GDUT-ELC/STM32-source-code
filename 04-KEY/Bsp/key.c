/**********************************************************
 * 文件名: key.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-28
 * 
 * 功能说明:
 *   实现按键检测的底层功能，包括按键状态读取、消抖处理和键码返回，
 *   支持4个独立按键的识别
 * 
 * 接口备注:
 *   1. 依赖HAL库的GPIO操作函数
 *   2. key_getnum()函数采用阻塞方式实现，按键按下时会等待松手
 *   3. 包含20ms延时消抖处理，避免按键机械抖动导致误判
 *   4. 按键引脚定义需在工程中通过宏定义配置(KEY1_GPIO_Port等)
 **********************************************************/

#include "key.h"

 /**
  * @brief  按键获取键码
  * @param  None
  * @retval 按下按键的键码值(0~4)，返回0代表没有按键按下
  */
uint8_t key_getnum(void)
{
	//定义变量，默认键码值为0
	uint8_t keynum = 0;
	
	if (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == 0)			
	{
		HAL_Delay(20);											    //延时消抖
		while (HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == 0);	//等待按键松手
		HAL_Delay(20);											    //延时消抖
		keynum = 1;												    //置键码为1
	}
	else if (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == 0)			
	{
		HAL_Delay(20);											    //延时消抖
		while (HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == 0);	//等待按键松手
		HAL_Delay(20);											    //延时消抖
		keynum = 2;												    //置键码为2
	}
	else if (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == 0)			
	{
		HAL_Delay(20);											    //延时消抖
		while (HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == 0);	//等待按键松手
		HAL_Delay(20);											    //延时消抖
		keynum = 3;												    //置键码为3
	}
	else if (HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin) == 0)			
	{
		HAL_Delay(20);											    //延时消抖
		while (HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin) == 0);	//等待按键松手
		HAL_Delay(20);											    //延时消抖
		keynum = 4;												    //置键码为4
	}
	
	//返回键码值，如果没有按键按下，所有if都不成立，则键码为默认值0
	return keynum;
}

