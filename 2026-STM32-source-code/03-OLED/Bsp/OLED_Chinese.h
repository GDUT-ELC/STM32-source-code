/**********************************************************
 * 文件名: OLED_Chinese.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   声明OLED中文显示接口，应用程序可以直接显示GB2312汉字、
 *   GB2312字符串或UTF-8字符串
 *
 * 接口备注:
 *   1. 使用前需先调用oled_init完成OLED初始化
 *   2. 行范围为1~4，中文起始列范围为1~15，一个汉字占两列
 *   3. 源文件为UTF-8编码时，优先使用oled_showutf8string接口
 **********************************************************/

#ifndef OLED_CHINESE_H
#define OLED_CHINESE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  显示一个16x16的GB2312汉字
 * @param  Line 行位置，范围1~4
 * @param  Column 起始列位置，范围1~15
 * @param  GB2312 汉字的GB2312双字节编码
 * @retval None
 */
void oled_showchinese(uint8_t Line, uint8_t Column, uint16_t GB2312);

/**
 * @brief  显示GB2312编码的中英文混合字符串
 * @param  Line 起始行位置，范围1~4
 * @param  Column 起始列位置，范围1~16
 * @param  String 以0结尾的GB2312字符串地址
 * @retval None
 */
void oled_showgb2312string(uint8_t Line, uint8_t Column, const uint8_t *String);

/**
 * @brief  显示UTF-8编码的中英文混合字符串
 * @param  Line 起始行位置，范围1~4
 * @param  Column 起始列位置，范围1~16
 * @param  String 以0结尾的UTF-8字符串地址
 * @retval None
 */
void oled_showutf8string(uint8_t Line, uint8_t Column, const char *String);

#ifdef __cplusplus
}
#endif

#endif
