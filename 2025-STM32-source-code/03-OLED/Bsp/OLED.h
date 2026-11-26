/**********************************************************
 * 文件名: OLED.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-28
 * 
 * 功能说明:
 *   声明OLED显示屏的相关函数接口，包括初始化、清屏、字符/字符串/数字显示等功能，
 *   支持多种格式的数据显示，基于128x64分辨率屏幕
 * 
 * 接口备注:
 *   1. 需配合和OLED_Font.h字体库使用
 *   2. 支持显示字符、字符串、无符号数、有符号数、十六进制数和二进制数
 *   3. 坐标范围：行(1-4)，列(1-16)，对应屏幕物理像素位置映射
 *   4. 所有显示函数调用后立即生效，无需额外刷新操作
 **********************************************************/

#ifndef __OLED_H
#define __OLED_H

#include "OLED_Font.h"
#include "main.h"

void oled_init(void);
void oled_clear(void);
void oled_showchar(uint8_t Line, uint8_t Column, char Char);
void oled_showstring(uint8_t Line, uint8_t Column, char *String);
void oled_shownum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void oled_showsignednum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void oled_showhexnum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void oled_showbinnum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

#endif
