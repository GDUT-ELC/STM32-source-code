# STM32-LinkNode

🚀 **基于 STM32CubeMX + HAL 库的学习代码仓库**

🔗 **项目链接**：
- **GitHub**: [https://github.com/GDUT-ELC/STM32-source-code ](https://github.com/GDUT-ELC/STM32-source-code)
- **百度网盘**: [https://pan.baidu.com/s/1cPx0L2Fa8IyqJIp1bxSxFQ?pwd=6wsi](https://pan.baidu.com/s/1cPx0L2Fa8IyqJIp1bxSxFQ?pwd=6wsi)

## 📖 项目简介

本仓库汇集了基于 STM32F4 系列（开发板型号：STM32F407VET6）的各种外设驱动与应用示例。所有工程均采用 **STM32CubeMX** 生成初始化代码，**Keil MDK-ARM** 进行开发，使用 **HAL 库** 进行编写。

旨在提供一套清晰、规范、模块化的 STM32 驱动参考，帮助开发者快速上手 HAL 库开发。

- **核心芯片**: `STM32F407VET6`
- **开发环境**: `Keil v5` / `STM32CubeMX`
- **库版本**: `STM32Cube_FW_F4_V1.26.0` (及以上)

---

## 📂 工程目录与功能说明

本仓库按功能模块划分为独立的子工程，便于单独测试与移植。

| 序号 | 工程路径 | 功能模块 | 关键外设/技术点 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| 01 | [`01-LED/`](./01-LED) | **GPIO 输出** | `GPIO` | 点亮板载 LED，HAL_GPIO_TogglePin 使用 |
| 02 | [`02-Buzzer/`](./02-Buzzer) | **蜂鸣器控制** | `GPIO` | 有源/无源蜂鸣器驱动逻辑 |
| 03 | [`03-OLED/`](./03-OLED) | **OLED 显示** | `I2C` / `SPI` | 0.96寸 OLED 屏幕驱动 (SSD1306)，含字体库 |
| 04 | [`04-KEY/`](./04-KEY) | **按键输入** | `GPIO` | 按键扫描、软件消抖处理 |
| 05 | [`05-motor/`](./05-motor) | **电机控制** | `TIM` (PWM) | 直流电机 PWM 调速驱动 |
| 06 | [`06-ADC/`](./06-ADC) | **模数转换** | `ADC` | 电压采集、电位器读取 |
| 07 | [`07-MPU6050/`](./07-MPU6050) | **姿态传感器** | `I2C` | MPU6050 六轴传感器数据读取与解算 |
| 08 | [`08-HCSR04/`](./08-HCSR04) | **超声波测距** | `TIM` (Input Capture) | 超声波模块距离测量，定时器输入捕获应用 |
| 09 | [`09-uart/`](./09-uart) | **串口通信** | `USART` | 串口收发数据、重定向 printf |
| 10 | [`10-esp8266/`](./10-esp8266) | **WiFi 通信** | `UART` / `AT指令` | ESP8266 模块联网与数据传输 |
| 11 | [`11-Demo/`](./11-Demo) | **综合应用** | `Multi-Module` | 综合功能演示工程 |
| -- | [`Hardware/`](./Hardware) | **硬件资料** | `PDF` | 包含核心板/拓展板原理图、STM32F407 数据手册及参考手册 |

---

## 🛠️ 使用说明

### 1. 硬件准备
*   **开发板**: GDUT_ELC 定制开发板 / STM32F4 核心板
*   **下载器**: ST-Link V2 / J-Link
*   **外设模块**: OLED 屏、MPU6050、ESP8266 等对应模块

### 2. 软件环境
*   请确保安装 **Keil uVision 5** (推荐 v5.25 以上)。
*   安装对应的 **STM32F4 Device Family Pack**。
*   (可选) 安装 **STM32CubeMX** 用于查看或修改底层配置。

### 3. 如何使用
1.  克隆本仓库到本地：
    ```bash
    git clone [本仓库地址]
    ```
2.  进入对应目录（如 `01-LED/MDK-ARM/`），双击打开 `.uvprojx` 工程文件。
3.  点击编译按钮，确保无报错。
4.  连接下载器，下载程序到开发板。

> **注意**：如果不进行代码修改，**不需要** 重新运行 CubeMX 生成代码，直接编译 Keil 工程即可。

---

## 📝 更新记录表

| 版本号 | 更新日期 | 更新类型 | 主要更新内容 | 备注 |
| :--- | :--- | :--- | :--- | :--- |
| `v1.0.2` | 2026-02-14 | **新增** | 增加 `Hardware` 文件夹，提供开发板硬件原理图及官方技术文档。 | 完善硬件参考 |
| `v1.0.1` | 2026-02-04 | **优化** | 优化仓库结构，配置 .gitignore 忽略编译垃圾文件。 | 仓库瘦身 |
| `v1.0.0` | 2025-10-30 | **发布** | 完成基础外设（GPIO, ADC, UART, PWM等）驱动编写。 | 初始版本 |

---

## 🤝 贡献与反馈

如果您发现代码中有 Bug，或者有更好的实现方式，欢迎提交 Issue 或 Pull Request。

- **维护组织**: GDUT-ELC
- **联系方式**: gdutelc@gmail.com

---

*最后更新：2026.02.14*

<span style="font-size: 0.7em; color: #cccccc;"><sup>zephyr97</sup></span>