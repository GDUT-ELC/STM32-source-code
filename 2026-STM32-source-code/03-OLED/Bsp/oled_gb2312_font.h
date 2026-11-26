#ifndef OLED_GB2312_FONT_H
#define OLED_GB2312_FONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OLED_GB2312_FIRST_HIGH 0xA1u
#define OLED_GB2312_LAST_HIGH  0xF7u
#define OLED_GB2312_FIRST_LOW  0xA1u
#define OLED_GB2312_LAST_LOW   0xFEu
#define OLED_GB2312_ZONE_COUNT 87u
#define OLED_GB2312_CELL_COUNT 94u
#define OLED_GB2312_GLYPH_COUNT (OLED_GB2312_ZONE_COUNT * OLED_GB2312_CELL_COUNT)
#define OLED_GB2312_GLYPH_BYTES 32u

typedef struct
{
    uint16_t unicode;
    uint16_t gb2312;
} OLED_UnicodeGB2312Map;

extern const uint8_t OLED_GB2312_F16x16[OLED_GB2312_GLYPH_COUNT][OLED_GB2312_GLYPH_BYTES];
extern const OLED_UnicodeGB2312Map OLED_UnicodeToGB2312[];
extern const uint16_t OLED_UnicodeToGB2312Count;

#ifdef __cplusplus
}
#endif

#endif
