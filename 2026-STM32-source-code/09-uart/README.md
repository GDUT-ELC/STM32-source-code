# 09-uart：串口中断接收与回显工程

## 工程说明

使用 STM32F407VET6 的 UART4 中断逐字节接收数据，连续 5ms 没有新数据时作为一包处理，原样回显后追加 `UART RX over`。单包最多 128 字节，溢出时提示 `UART RX overflow`。

## 使用方法

1. 使用 3.3V TTL USB 转串口模块：PA0（UART4 TX）接模块 RXD，PA1（UART4 RX）接模块 TXD，并连接 GND。
2. 用 Keil MDK 打开 `MDK-ARM/9-uart.uvprojx`，编译、下载。
3. 串口助手设置为 115200、8 数据位、1 停止位、无校验、无硬件流控。发送短文本，观察原样回显和完成提示。
4. 修改功能时，在 UART 初始化后调用 `uart_driver_init()`，主循环用 `uart_driver_read_packet()` 取包；发送文本用 `uart_driver_send_string()`，发送指定长度数据用 `uart_driver_send()`。

## 使用注意

- **当前实际收发使用 UART4（PA0/PA1）**。`uart_driver.h` 的 USART1 注释已过时；USART1 虽被初始化，PA9/PA10 不承担本示例回显。
- 5ms 是软件静默分包规则，不要求回车换行。逐字慢速输入可能被拆成多包；连续数据超过 128 字节会溢出，应按协议调整缓冲或分包方式。
- 二进制接收数据不保证以 `\0` 结尾，应按返回长度处理，不能直接当 C 字符串使用。完整输出还包含完成提示，自动化比对时需考虑这部分。
- USB 转串口必须与 MCU 共地并交叉连接 TX/RX；RS-232 电平不能直接接入。
- 移植时保留 UART4 中断入口和 `HAL_UART_RxCpltCallback()`，不要在同一工程重复定义 HAL 回调。
