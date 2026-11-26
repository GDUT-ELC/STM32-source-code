# STM32-LinkNode 外设学习工程

基于 STM32F407VET6、STM32CubeMX 和 HAL 库的外设学习代码，使用 Keil MDK-ARM 开发。当前目录包含 12 个编号工程、一个额外的 Flash 实验工程及硬件资料；每个编号工程的使用方法和注意事项见其 README。

## 目录与功能

| 目录 | 工程内容 |
| --- | --- |
| [01-LED](01-LED/README.md) | WS2812 RGB 灯珠颜色与亮度渐变 |
| [02-Buzzer](02-Buzzer/README.md) | TIM1 PWM 驱动无源蜂鸣器 |
| [03-OLED](03-OLED/README.md) | GPIO 模拟 SPI OLED，字符、数字及中文显示接口 |
| [04-KEY](04-KEY/README.md) | 四按键扫描、延时消抖、OLED 显示编号 |
| [05-motor](05-motor/README.md) | DRV8848 双电机、编码器测速、PI/PID 闭环与开环 PWM |
| [06-ADC](06-ADC/README.md) | PB0 光敏分压采样及电压换算 |
| [07-MPU6050](07-MPU6050/README.md) | 软件 I²C 读取六轴原始数据 |
| [08-HCSR04](08-HCSR04/README.md) | TIM3 输入捕获测量超声波回波和距离 |
| [09-uart](09-uart/README.md) | UART4 中断接收、静默分包与回显 |
| [10-esp8266](10-esp8266/README.md) | UART5 发送 WiFi/MQTT AT 指令 |
| [11-Demo](11-Demo/README.md) | 11 个测试页，综合外设验证及姿态角演示 |
| [12-gd25qdemo](12-gd25qdemo/README.md) | GD25Q80C 外部 Flash 读写与 OLED 校验 |
| [epprom](epprom/README_Flash.md) | 额外的 GD25Q80C Flash 实验工程，目录名保留；实际器件为 NOR Flash |
| [Hardware](Hardware/) | 扩展板与核心板原理图、STM32F407VET6 数据手册、STM32F4xx 中文参考手册 |
| `.vscode/` | 编辑器配置 |
| [all.md](all.md) | 本 README 与 12 个编号工程 README 的全文汇总 |

## 使用方法

1. 准备对应的 STM32F407VET6 开发板、ST-Link/J-Link 等 SWD 下载器，以及待测外设。按 `Hardware/` 中原理图和对应工程 README 核对接线、供电和电平。
2. 安装 Keil MDK、STM32F4 Device Family Pack 和 Arm Compiler 6。当前多数工程记录的编译器为 6.23，08、10 记录为 6.7；若本机未安装该版本，在工程选项中选择已安装的兼容 Arm Compiler 6。
3. 进入对应工程的 `MDK-ARM/`，打开 `.uvprojx`，设置实际使用的下载器和 SWD 接口，然后执行 Rebuild、下载并运行。
4. 按各工程 README 观察输出或修改演示。首次编译会重新生成输出目录；不改底层配置时，无需先运行 CubeMX。
5. 需要修改引脚、时钟或外设配置时，用 CubeMX 打开该工程根目录的 `.ioc`；重新生成后检查用户代码、自定义驱动、启动文件与中断回调是否完整。

编号工程通常按以下结构组织：`Core/` 保存主程序和外设初始化，`Bsp/` 保存驱动，`Drivers/` 保存 HAL/CMSIS，`MDK-ARM/` 保存 Keil 工程与启动文件。部分工程另有 `Tests/`、`Examples/`、`Tools/` 或 `Backup/`，分别用于主机测试、示例、辅助工具和历史备份。

## 使用注意

- 当前工程通常使用 8MHz 外部晶振和 168MHz 系统时钟；更换核心板时，必须核对时钟树、引脚和外设复用。各独立工程资源分配不同，不能直接将初始化代码叠加。
- 电机使用前先核对减速比、编码器每圈计数、方向和 PID。05 当前演示目标为 A=999、B=1001 CPS，11 当前两路闭环演示目标为 99000 CPS，必须按实际硬件调整；这些数值都不是 RPM。
- OLED 使用带 CS/DC/RES 的 SPI 接线。07 显示六轴原始数据，姿态角在 11；09 实际回显串口为 UART4，11 的第二路测试串口为 USART2。
- 10 的 WiFi/MQTT 参数默认空白，需自行填写；其返回成功只表示命令发送成功。11 提供基于回包的测试状态，联网仍需要匹配的模块固件、热点和平台参数。
- 11 和 12 上电都会执行 Flash 写入测试，默认改写 0x001000 起的数据；使用自有数据前先调整测试代码和存储区域。
- 外部设备应与 MCU 共地，核对 3.3V 逻辑与外设供电要求；特别注意超声波 ECHO、电机编码器输出及电机驱动电源。
- 本次清理范围为 01–12 号工程的编译产物、生成日志、反汇编输出及 Keil 用户界面缓存。源码、`.ioc`、`.uvprojx`、`.uvoptx`、启动文件、调试配置、HAL/CMSIS 库、测试及备份均保留，`epprom/` 和 `Hardware/` 保持原内容。
- 后续编译生成的目标文件、固件、映射文件等可再次清理；不要将库目录中的预编译 `.lib`、`.a` 当作编译垃圾删除。首次运行前以源码中的当前参数为准，并完成实物验证。

仓库来源：[GDUT-ELC/STM32-source-code](https://github.com/GDUT-ELC/STM32-source-code)。
