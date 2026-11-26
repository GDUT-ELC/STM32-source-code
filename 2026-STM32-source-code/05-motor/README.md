# 05-motor：双电机编码器速度闭环工程

## 工程说明

使用 STM32F407VET6 和 DRV8848 控制两路直流电机，支持编码器测速、PID 速度闭环及 PWM 开环调速。TIM2 中断产生约 1kHz GPIO PWM，TIM6 每 10ms 执行测速和控制；当前 Kd=0，实际采用 PI 控制。

## 使用方法

1. 按实际硬件连接电机和驱动电源，并让 MCU、驱动、编码器共地。A 路 AIN1/AIN2 接 PD11/PD10，B 路 BIN1/BIN2 接 PD8/PD9；A 编码器两相接 PD12/PD13（TIM4），B 编码器两相接 PC6/PC7（TIM3）。编码器电源与输出电平按实际型号确认。
2. 用 Keil MDK 打开 `MDK-ARM/5-motor.uvprojx`。先检查 `Core/Src/main.c` 的 `DEMO_A_TARGET_CPS` 和 `DEMO_B_TARGET_CPS`：当前分别为 **999 和 1001 CPS**，建议首次调试改为 0，再逐路设置低速目标。
3. 编译、下载并连续运行。先让 MCU 运行，再开启电机驱动 VM 电源；调试时将轮子架空。
4. 初始化成功后，调用 `motor_set_target_cps(MOTOR_A, 目标值)` 进入闭环；正负值控制方向，0 制动。调用 `motor_set_pwm(MOTOR_A, 300)` 切换为 30% 开环输出，`motor_stop_all()` 停止两路。
5. 在 Keil Watch 查看 `motor_a_feedback`、`motor_b_feedback` 或 `g_motor_feedback`，关注目标、`speed_cps`、`pwm`、`delta_counts` 和持续增加的 `control_ticks`。反馈变量仅用于观察，不能用它下发目标。

## 使用注意

- **先确认减速比和编码器参数。** CPS 是编码器四倍频计数/秒，不是 RPM。若厂家给出电机轴单相每圈脉冲数，则输出轴每圈计数为 `单相脉冲数 × 4 × 减速比`；若参数已经是四倍频计数或输出轴每圈计数，不要重复相乘。可手动转动输出轴一圈核对实际计数。
- 在 `Bsp/motor_config.h` 分别填写 `MOTOR_A_COUNTS_PER_REV`、`MOTOR_B_COUNTS_PER_REV` 后才能正确使用 `motor_set_target_rpm()`。换算为 `RPM = CPS × 60 / 每圈计数`。当前两项均为 0，RPM 指令会返回错误，RPM 反馈显示 0，CPS 控制仍可使用。
- 两路 PID 应按实际电机、减速箱、电源和负载分别整定。先检查小幅正向 PWM 是否得到正向编码器速度，再按需调整 `DRIVE_SIGN`、`ENCODER_SIGN`；方向错误会导致闭环持续加大输出。
- 先调 Kp，再逐步加入 Ki；不要在主循环反复调用 `motor_set_pid()`，否则积分会不断清零。改变 TIM6 周期时，必须同步修改 `MOTOR_CONTROL_PERIOD_S`。
- 编码器未接、堵转或目标超过实际能力时，PI 可能持续增大输出至限幅；当前版本不提供反馈故障自动锁存停车。首次测试应降低目标和 PWM 限幅，并留意驱动及电机温升。
- 本板 DRV8848 的 nSLEEP 接 VCCT。启动文件含提前配置 PD8–PD11 的代码，用于缩短输入悬空时间；它不能保证解除已经锁存的并联模式。优先采用“MCU 先运行、再开 VM”的顺序，仅复位 MCU 不一定能恢复驱动模式。
- 闭环运行中不要靠断点或单步判断速度；替换启动文件、重新生成 CubeMX 代码时，应检查早期 GPIO 初始化、TIM2 PWM 中断及 TIM6 控制回调是否保留。
