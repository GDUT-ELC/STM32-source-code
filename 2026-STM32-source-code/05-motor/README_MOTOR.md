# 05-motor 简化速度闭环

当前默认运行 **A=0（停车）、B=200 CPS** 的编码器速度闭环。
主程序直接调用 `motor_set_target_cps()` 设置目标，TIM6 每 10ms 测速并计算 PID，自动调整 PWM。
当前 Kd=0，实际为 PI 控制；需要微分时可在 `motor_config.h` 中配置 Kd。
闭环时 B 的反馈 `mode=1`、`target_cps=200`，实测 `speed_cps` 稳定后应接近 200。
已移除临时的固定 PWM 排查分支和引脚电平观察变量，保留启动入口的 GPIO 初始化。

DRV8848 上电或退出休眠时若 AIN1、AIN2 均为高阻，会进入并联模式，由 BIN1/BIN2 同时控制两路。
本板 nSLEEP 接 VCCT，用户要求不改硬件：固件在 `startup_stm32f407xx.s` 的 Reset_Handler
入口直接配置 PD8～PD11 为推挽低电平，并启用内部下拉，早于 SystemInit、C 运行库、HAL_Init 和时钟配置。
`MX_GPIO_Init()` 延续下拉设置。该处理缩短 CPU 开始执行之后的引脚悬空时间，
不能保证赶在驱动芯片判断模式之前，也不能解除此前已锁存的并联模式。

使用已验证有效的先 MCU、后驱动 VM 上电顺序，无需增加元件。
让 MCU 连续运行后再开启 VM，避免调试器在复位入口暂停。
若 MCU 和驱动必须同时上电，软件的效果取决于电源和复位时序，需要实测。
仅复位 MCU 不能保证退出已锁存的并联模式。

MCU 与电机电源可分别控制，按此顺序运行：

1. 关闭电机驱动 VM 电源，保持 MCU 供电。
2. 下载固件并连续运行，Watch 中两路反馈的 `mode` 应为 1。
3. 保持 MCU 运行，再开启驱动 VM 电源，不再复位 MCU。
4. 当前目标下 A 停车，B 由 PID 自动调速到 200 CPS；实际响应需结合电机负载验证。

若 A 目标为 0 但 A、B 仍同时被驱动，应核对上电模式、两路接线及实际输入/输出波形。
更新或替换 Keil 启动文件时，需要保留这段早期 GPIO 初始化；更改 `gpio.c` 本身不能替代它。

闭环时：
TIM6 每 10ms 执行一次：读取编码器计数差 → 计算速度 → PI → 输出 PWM。
默认 Kd=0，因此实际使用 PI；保留 PID 接口供需要时调参。

## 改目标

在 `Core/Src/main.c` 修改目标后重新编译、下载：

```c
#define DEMO_A_TARGET_CPS     0.0f
#define DEMO_B_TARGET_CPS     200.0f
```

正数正转，负数反转，0 制动停车。CPS 是编码器四倍频计数/秒；
200 CPS 对应平均每 10ms 增加 2 个计数，并不是 200 RPM 或 20% PWM。

程序只在初始化后设置一次目标，主循环读取反馈即可。
需要改变速度时调用 `motor_set_target_cps(MOTOR_A, 目标值)`；
需要停车时调用 `motor_stop_all()`。
Watch 中的 `motor_a_feedback` / `motor_b_feedback` 只用于观察，不能通过修改它们下发目标。

## 改 PI 参数

在 `Bsp/motor_config.h` 修改，当前参数：

| 电机 | Kp | Ki | Kd |
| --- | --- | --- | --- |
| A | 0.20 | 1.32 | 0 |
| B | 0.20 | 0.80 | 0 |

误差单位 CPS，输出单位 PWM 千分比，积分时间单位秒。
参数需要结合电机和负载调整；先调 Kp，再加 Ki，Kd 先保持 0。
不要在主循环反复调用 `motor_set_pid()`，它会清空积分。

## 观察结果

Keil 连续运行时查看 `motor_a_feedback`（A）或 `motor_b_feedback`（B）：

| 字段 | 含义 |
| --- | --- |
| `target_cps` | 实际生效目标 |
| `speed_cps` | 编码器测速，经过简单滤波 |
| `delta_counts` | 最近 10ms 的有符号计数差 |
| `pwm` | 输出千分比，300 表示 30% |
| `control_ticks` | 控制执行次数，应约每秒增加 100 |
| `pid_integral` | 积分输出 |

正 PWM 必须对应正的编码器速度。若持续反向，核对接线并修改对应的
`MOTOR_A_ENCODER_SIGN` 或 `MOTOR_B_ENCODER_SIGN`（+1 / -1）。

## 接线与定时器

| 功能 | 引脚 / 配置 |
| --- | --- |
| AIN1 / AIN2 | PD11 / PD10 |
| BIN1 / BIN2 | PD8 / PD9 |
| A 编码器 | PD12 / PD13，TIM4 |
| B 编码器 | PC6 / PC7，TIM3 |
| PWM | TIM2，84MHz / 84000 = 1kHz，GPIO 软件输出 |
| 控制周期 | TIM6，84MHz / 84 / 10000 = 100Hz |

以用户提供的 DRV8848PWPR 芯片原理图为准：AIN1/2 和 BIN1/2 同时承担方向与 PWM，
没有独立 PWMA/PWMB 输入；PB14/PB15 不用于此驱动。NSLEEP 接 VCCT，由硬件拉高。
A 电机为 P3：1=AO1，6=AO2，3=PD13（TIM4_CH2），4=PD12（TIM4_CH1）。
B 电机为 P4：1=BO1，6=BO2，3=PC7（TIM3_CH2），4=PC6（TIM3_CH1）。
P3/P4 的 2、5 脚供电由图中的 0Ω 电阻选择，按实际焊接核对电源和地。

PD8～PD11 没有定时器 PWM 复用，沿用 GPIO PWM。
DRV8848 的输入 01/10 控制方向，11 制动；PWM 关闭阶段和停车使用 11。
1kHz 下普通 PWM 更新最多等一个周期（1ms），停车/换向先立即制动。
软件 PWM 的小占空比仍受中断延迟影响，实际波形需上板测量；1kHz 可能有可听见的声音。
电机驱动电源、编码器电源和共地按实际硬件连接。

## 本次简化

- 删除 TIM5 测间隔及超过 30ms 清积分停车的分支。TIM5 生成代码保留，当前主程序不初始化、不使用。
- 删除新增的反馈故障锁存及 `fault` 等字段。
- 删除 Watch 目标命令数组和同步逻辑，目标统一在 `main.c` 设置。
- 软件 PWM 从 10kHz 改为 1kHz，降低中断负担及相同占空比下的脉冲缩短比例。
- 保留输出限幅、积分抗饱和、目标 0 停车及系统异常制动。

固定周期控制适用于 TIM6 连续运行；不要在控制过程中单步或设置断点来判断速度。
无编码器反馈时 PI 会增大输出直至限幅，这个简化版不再自动锁存停车。
修改前文件保存在 `Backup/before_simple_control_20261113.zip`。

## 编译和验证

打开 `MDK-ARM/5-motor.uvprojx` 编译下载。
固件：`MDK-ARM/5-motor/5-motor.hex`。
编译日志：`MDK-ARM/motor_simple_build.log`。
包含启动入口和当前闭环固件的最新编译日志：`MDK-ARM/motor_early_gpio_build.log`。

运行电脑端模拟测试：

```powershell
& ./05-motor/Tests/run_tests.ps1
```

测试包含 200 CPS 正反转、负载变化、超速降输出、停车再启动、编码器回绕、
积分抗饱和和 PWM 比较边沿。编译与模拟测试不能代替实物验证。
其他接口用法见 `Examples/README.md`。
