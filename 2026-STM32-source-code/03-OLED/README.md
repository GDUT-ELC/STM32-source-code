# 03-OLED：OLED 显示工程

## 工程说明

使用 STM32F407VET6 通过 GPIO 模拟 SPI 驱动 128×64 OLED，显示字符串、有符号数、十六进制数和二进制数，并提供中文显示与字库接口。

## 使用方法

1. 连接支持对应初始化命令的 SPI OLED：SCK→PE9、MOSI/DIN→PE10、RES→PE11、DC→PE12、CS→PE13，按模块要求供电并共地。
2. 用 Keil MDK 打开 `MDK-ARM/3-OLED.uvprojx`，编译、下载后，屏幕应显示 `STM32F407VET6`、45、-88、ABCD 和二进制数。
3. GPIO 初始化后先调用 `oled_init()`，再调用 `oled_showstring()`、`oled_shownum()` 等接口；清屏使用 `oled_clear()`。
4. 显示 UTF-8 中文时包含 `OLED_Chinese.h`，调用 `oled_showutf8string()`；其他接口见对应头文件。

## 使用注意

- 当前驱动为带 CS、DC、RES 信号的软件 SPI，不能直接用于只有 SCL/SDA 的 I²C 屏；更换屏幕控制器或分辨率时要核对初始化命令与显存布局。
- 常用 8×16 字符接口的行范围为 1–4、列范围为 1–16，字符串及数字总宽度不能越界；16×16 汉字占两列。
- 中文内容必须与输入编码和字库匹配，源文件建议保存为 UTF-8；UTF-8 字符串不要直接交给普通 ASCII 显示接口。
- 改接线时同时修改 `Core/Inc/main.h` 的引脚宏及 GPIO 初始化配置。
