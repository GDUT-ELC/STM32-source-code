/**********************************************************
 * 文件名: OLED.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-28
 * 
 * 功能说明:
 *   实现OLED显示屏的底层驱动及上层控制功能，包括初始化、数据/命令传输、
 *   字符/字符串/数字显示等操作，基于软件SPI通信协议
 * 
 * 接口备注:
 *   1. 需配合OLED_Font.c字体库使用，支持128x64分辨率
 *   2. 提供8x16字体显示支持，包含多种数字格式显示函数
 *   3. 所有显示操作直接写入显存并立即显示，无需额外刷新
 *   4. 坐标映射：行(1-4)对应页地址(0-7)，列(1-16)对应水平像素(0-127)
 *   5. 包含OLED初始化序列和清屏功能，支持屏幕方向配置
 **********************************************************/

#include "OLED.h"

/* 引脚配置 */
#define OLED_W_SCK(x)    HAL_GPIO_WritePin(OLED_SCK_GPIO_Port, OLED_SCK_Pin, (GPIO_PinState)(x))
#define OLED_W_MOSI(x)	 HAL_GPIO_WritePin(OLED_MOSI_GPIO_Port, OLED_MOSI_Pin, (GPIO_PinState)(x))
#define OLED_W_RES(x)	 HAL_GPIO_WritePin(OLED_RES_GPIO_Port, OLED_RES_Pin, (GPIO_PinState)(x))
#define OLED_W_DC(x)	 HAL_GPIO_WritePin(OLED_DC_GPIO_Port, OLED_DC_Pin, (GPIO_PinState)(x))
#define OLED_W_CS(x)	 HAL_GPIO_WritePin(OLED_CS_GPIO_Port, OLED_CS_Pin, (GPIO_PinState)(x))

/**
  * @brief  引脚电平初始化
  * @param  None
  * @retval None
  */
void OLED_SPI_Init(void)
{
	OLED_W_SCK(0);
	OLED_W_MOSI(1);
	OLED_W_RES(1);
	OLED_W_DC(1);
	OLED_W_CS(1);
}

/**
  * @brief  SPI发送一个字节
  * @param  Byte 要发送的一个字节
  * @retval None
  */
void OLED_SPI_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_MOSI(Byte & (0x80 >> i));
		OLED_W_SCK(1);
		OLED_W_SCK(0);
	}
}

/**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval None
  */
static void OLED_WriteCommand(uint8_t Command)
{
	OLED_W_CS(0);
	OLED_W_DC(0);
	OLED_SPI_SendByte(Command);
	OLED_W_CS(1);
}

/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval None
  */
static void OLED_WriteData(uint8_t Data)
{
	OLED_W_CS(0);
	OLED_W_DC(1);
	OLED_SPI_SendByte(Data);
	OLED_W_CS(1);
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval None
  */
static void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED清屏
  * @param  None
  * @retval None
  */
void oled_clear(void)
{  
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		for(i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);
		}
	}
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval None
  */
void oled_showchar(uint8_t Line, uint8_t Column, char Char)
{      	
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
	}
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval None
  */
void oled_showstring(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		oled_showchar(Line, Column + i, String[i]);
	}
}

/**
  * @brief  OLED次方函数
  * @retval 返回值等于X的Y次方
  */
static uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval None
  */
void oled_shownum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		oled_showchar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval None
  */
void oled_showsignednum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		oled_showchar(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		oled_showchar(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		oled_showchar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval None
  */
void oled_showhexnum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			oled_showchar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			oled_showchar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval None
  */
void oled_showbinnum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		oled_showchar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
  * @brief  OLED初始化
  * @param  None
  * @retval None
  */
void oled_init(void)
{
	uint32_t i, j;
	
	for (i = 0; i < 100; i++)	//上电延时
	{
		for (j = 0; j < 100; j++);
	}
	
	OLED_SPI_Init();			//端口初始化
	
	OLED_WriteCommand(0xAE);	//关闭显示
	
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);
	
	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);
	
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);
	
	OLED_WriteCommand(0x40);	//设置显示开始行
	
	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭

	OLED_WriteCommand(0xA6);	//设置正常/倒转显示

	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//开启显示
		
	oled_clear();				//OLED清屏
}
