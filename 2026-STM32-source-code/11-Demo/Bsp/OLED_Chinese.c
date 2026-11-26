/**********************************************************
 * 文件名: OLED_Chinese.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   封装OLED中文显示功能，支持单个GB2312汉字、GB2312字符串
 *   和UTF-8字符串显示，并在到达屏幕右边界时自动换行
 *
 * 接口备注:
 *   1. 使用前需先调用oled_init完成OLED初始化
 *   2. 行范围为1~4，中文起始列范围为1~15，一个汉字占两列
 *   3. oled_showgb2312string接收GB2312编码，oled_showutf8string接收UTF-8编码
 *   4. 无法显示的UTF-8字符使用两个问号代替，保持16像素字符宽度
 **********************************************************/

#include "OLED.h"
#include "oled_gb2312_font.h"

/**
 * @brief  检查显示位置，并在当前行空间不足时自动换到下一行
 * @param  Line 行号地址，范围1~4
 * @param  Column 列号地址，范围1~16
 * @param  Width 本次字符占用的像素宽度，ASCII为8，中文为16
 * @retval 1表示位置有效，0表示已经超出屏幕范围
 */
static uint8_t OLED_ChinesePosition(uint8_t *Line, uint8_t *Column, uint8_t Width)
{
    if (*Line < 1U || *Line > 4U || *Column < 1U || *Column > 16U)
    {
        return 0U;
    }

    if ((uint16_t)(*Column - 1U) * 8U + Width > 128U)
    {
        *Column = 1U;
        (*Line)++;
    }

    return *Line <= 4U;
}

/**
 * @brief  将Unicode编码查表转换为GB2312编码
 * @param  Unicode 需要转换的Unicode编码
 * @param  GB2312 用于保存转换结果的地址
 * @retval 1表示转换成功，0表示字库中没有该字符
 */
static uint8_t OLED_UnicodeToGB2312Code(uint16_t Unicode, uint16_t *GB2312)
{
    uint16_t Left = 0U;
    uint16_t Right = OLED_UnicodeToGB2312Count;

    while (Left < Right)
    {
        uint16_t Middle = (uint16_t)(Left + (Right - Left) / 2U);

        if (OLED_UnicodeToGB2312[Middle].unicode < Unicode)
        {
            Left = (uint16_t)(Middle + 1U);
        }
        else
        {
            Right = Middle;
        }
    }

    if (Left >= OLED_UnicodeToGB2312Count ||
        OLED_UnicodeToGB2312[Left].unicode != Unicode)
    {
        return 0U;
    }

    *GB2312 = OLED_UnicodeToGB2312[Left].gb2312;
    return 1U;
}

/**
 * @brief  从UTF-8字符串中解析一个Unicode字符
 * @param  String UTF-8字符串当前位置的地址，解析后自动指向下一个字符
 * @retval 解析得到的Unicode编码，格式错误时返回0xFFFD
 */
static uint32_t OLED_UTF8Decode(const uint8_t **String)
{
    const uint8_t *Text = *String;
    uint32_t Unicode;

    if (Text[0] < 0x80U)
    {
        *String = Text + 1;
        return Text[0];
    }

    if ((Text[0] & 0xE0U) == 0xC0U && Text[1] != 0U &&
        (Text[1] & 0xC0U) == 0x80U)
    {
        Unicode = ((uint32_t)(Text[0] & 0x1FU) << 6) |
                  (uint32_t)(Text[1] & 0x3FU);
        *String = Text + 2;
        return Unicode >= 0x80U ? Unicode : 0xFFFDU;
    }

    if ((Text[0] & 0xF0U) == 0xE0U && Text[1] != 0U && Text[2] != 0U &&
        (Text[1] & 0xC0U) == 0x80U && (Text[2] & 0xC0U) == 0x80U)
    {
        Unicode = ((uint32_t)(Text[0] & 0x0FU) << 12) |
                  ((uint32_t)(Text[1] & 0x3FU) << 6) |
                  (uint32_t)(Text[2] & 0x3FU);
        *String = Text + 3;
        if (Unicode < 0x800U || (Unicode >= 0xD800U && Unicode <= 0xDFFFU))
        {
            return 0xFFFDU;
        }
        return Unicode;
    }

    if ((Text[0] & 0xF8U) == 0xF0U && Text[1] != 0U &&
        Text[2] != 0U && Text[3] != 0U &&
        (Text[1] & 0xC0U) == 0x80U && (Text[2] & 0xC0U) == 0x80U &&
        (Text[3] & 0xC0U) == 0x80U)
    {
        Unicode = ((uint32_t)(Text[0] & 0x07U) << 18) |
                  ((uint32_t)(Text[1] & 0x3FU) << 12) |
                  ((uint32_t)(Text[2] & 0x3FU) << 6) |
                  (uint32_t)(Text[3] & 0x3FU);
        *String = Text + 4;
        return Unicode >= 0x10000U && Unicode <= 0x10FFFFU ? Unicode : 0xFFFDU;
    }

    *String = Text + 1;
    return 0xFFFDU;
}

/**
 * @brief  显示一个16像素宽的未知字符占位符
 * @param  Line 行位置，范围1~4
 * @param  Column 起始列位置，范围1~15
 * @retval None
 */
static void OLED_ShowUnknownChinese(uint8_t Line, uint8_t Column)
{
    oled_showchar(Line, Column, '?');
    oled_showchar(Line, (uint8_t)(Column + 1U), '?');
}

/**
 * @brief  显示一个16x16的GB2312汉字
 * @param  Line 行位置，范围1~4
 * @param  Column 起始列位置，范围1~15
 * @param  GB2312 汉字的GB2312双字节编码，例如“你”为0xC4E3
 * @retval None
 */
void oled_showchinese(uint8_t Line, uint8_t Column, uint16_t GB2312)
{
    uint8_t High = (uint8_t)(GB2312 >> 8);
    uint8_t Low = (uint8_t)GB2312;
    uint16_t Index;

    if (Line < 1U || Line > 4U || Column < 1U || Column > 15U ||
        High < OLED_GB2312_FIRST_HIGH || High > OLED_GB2312_LAST_HIGH ||
        Low < OLED_GB2312_FIRST_LOW || Low > OLED_GB2312_LAST_LOW)
    {
        return;
    }

    Index = (uint16_t)(((uint16_t)(High - OLED_GB2312_FIRST_HIGH) *
                        OLED_GB2312_CELL_COUNT) +
                       (uint16_t)(Low - OLED_GB2312_FIRST_LOW));

    oled_show16x16(Line, Column, OLED_GB2312_F16x16[Index]);
}

/**
 * @brief  显示GB2312编码的中英文混合字符串
 * @param  Line 起始行位置，范围1~4
 * @param  Column 起始列位置，范围1~16
 * @param  String 以0结尾的GB2312字符串地址
 * @retval None
 */
void oled_showgb2312string(uint8_t Line, uint8_t Column, const uint8_t *String)
{
    if (String == 0)
    {
        return;
    }

    while (*String != 0U && Line <= 4U)
    {
        if (*String < 0x80U)
        {
            if (!OLED_ChinesePosition(&Line, &Column, 8U))
            {
                break;
            }

            if (*String >= 0x20U && *String <= 0x7EU)
            {
                oled_showchar(Line, Column, (char)*String);
            }
            else
            {
                oled_showchar(Line, Column, '?');
            }

            Column = (uint8_t)(Column + 1U);
            String++;
        }
        else
        {
            uint8_t High;
            uint8_t Low;

            if (String[1] == 0U || !OLED_ChinesePosition(&Line, &Column, 16U))
            {
                break;
            }

            High = String[0];
            Low = String[1];
            if (High >= OLED_GB2312_FIRST_HIGH && High <= OLED_GB2312_LAST_HIGH &&
                Low >= OLED_GB2312_FIRST_LOW && Low <= OLED_GB2312_LAST_LOW)
            {
                uint16_t GB2312 = (uint16_t)(((uint16_t)High << 8) | Low);
                oled_showchinese(Line, Column, GB2312);
            }
            else
            {
                OLED_ShowUnknownChinese(Line, Column);
            }

            Column = (uint8_t)(Column + 2U);
            String += 2;
        }
    }
}

/**
 * @brief  显示UTF-8编码的中英文混合字符串
 * @param  Line 起始行位置，范围1~4
 * @param  Column 起始列位置，范围1~16
 * @param  String 以0结尾的UTF-8字符串地址
 * @retval None
 */
void oled_showutf8string(uint8_t Line, uint8_t Column, const char *String)
{
    const uint8_t *Text = (const uint8_t *)String;

    if (String == 0)
    {
        return;
    }

    while (*Text != 0U && Line <= 4U)
    {
        uint32_t Unicode = OLED_UTF8Decode(&Text);

        if (Unicode >= 0x20U && Unicode <= 0x7EU)
        {
            if (!OLED_ChinesePosition(&Line, &Column, 8U))
            {
                break;
            }

            oled_showchar(Line, Column, (char)Unicode);
            Column = (uint8_t)(Column + 1U);
        }
        else
        {
            uint16_t GB2312;

            if (!OLED_ChinesePosition(&Line, &Column, 16U))
            {
                break;
            }

            if (Unicode <= 0xFFFFU &&
                OLED_UnicodeToGB2312Code((uint16_t)Unicode, &GB2312))
            {
                oled_showchinese(Line, Column, GB2312);
            }
            else
            {
                OLED_ShowUnknownChinese(Line, Column);
            }

            Column = (uint8_t)(Column + 2U);
        }
    }
}
