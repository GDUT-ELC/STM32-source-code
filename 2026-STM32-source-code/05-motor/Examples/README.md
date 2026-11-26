# 电机控制接口调用示例

以下片段分别展示一种用法。将选定片段放在 `main.c` 的 `USER CODE BEGIN 2` 中，
替换默认的两条 `motor_set_target_cps()` 调用；保留已有的 `motor_init()` 和外设初始化。
每个片段独立使用，后发的控制指令会覆盖同一路先前的设置。

## 1. PWM 开环控制

```c
motor_set_pwm(MOTOR_A, 300);   /* A 路正向 30%。 */
motor_set_pwm(MOTOR_B, -300);  /* B 路反向 30%。 */
```

参数单位千分比，范围 -1000～1000。编码器仍输出测速反馈，但不根据反馈调节 PWM。

## 2. 编码器速度闭环

```c
motor_set_target_cps(MOTOR_A, 2000.0f);
motor_set_target_cps(MOTOR_B, 2000.0f);
```

两路目标均为每秒 2000 个四倍频计数。目标保持有效，TIM6 每 10ms 自动测速与计算 PID。
正 PWM 应读出正速度；方向不一致时先修正 `motor_config.h` 中对应的 `ENCODER_SIGN`。

## 3. 正转、停车、反转

```c
motor_set_target_cps(MOTOR_A, 2000.0f);
HAL_Delay(3000);   /* 正转 3 秒；延时期间控制中断继续执行。 */

motor_stop(MOTOR_A);
HAL_Delay(1000);   /* 制动停留 1 秒。 */

motor_set_target_cps(MOTOR_A, -2000.0f);
HAL_Delay(3000);   /* 反转 3 秒。 */

motor_stop_all();  /* 示例结束，两路保持制动。 */
```

延时仅用于展示时序；有按键、通信等并行任务时，可改为基于 `HAL_GetTick()` 的非阻塞流程。

## 4. 只观察编码器

```c
motor_stop_all();
```

在 Watch 中查看 A 路 `htim4.Instance->CNT`、B 路 `htim3.Instance->CNT`。
手动转动时数值会随方向增减并回绕，制动状态下转动可能有阻力。
`g_motor_feedback[0/1].delta_counts` 表示对应电机最近 10ms 的增量。

## 5. 在应用程序中读取反馈

在 `main.c` 的 `USER CODE BEGIN PV` 中定义：

```c
motor_feedback_t motor_a_feedback;
motor_feedback_t motor_b_feedback;
```

在主循环 `USER CODE BEGIN 3` 中读取：

```c
motor_get_feedback(MOTOR_A, &motor_a_feedback);
motor_get_feedback(MOTOR_B, &motor_b_feedback);
HAL_Delay(100);
```

随后可将 `motor_a_feedback.speed_cps` 等值交给显示或通信模块。
接口读取的是单路一致快照；A/B 两次读取并非严格同时采样。
若保留原主循环的 `HAL_Delay(10)`，可替换为此处的延时，避免重复延时。

## 6. 修改 PID 系数

```c
motor_set_pid(MOTOR_A, 0.20f, 1.32f, 0.0f);
motor_set_pid(MOTOR_B, 0.20f, 0.80f, 0.0f);
motor_set_target_cps(MOTOR_A, 2000.0f);
motor_set_target_cps(MOTOR_B, 2000.0f);
```

系数顺序为 Kp、Ki、Kd；此例沿用工程当前参数。
`motor_set_pid()` 会清空积分，初始化后或需要调参时调用，不应在主循环持续调用。

## 7. RPM 控制

先在 `motor_config.h` 中将 `MOTOR_A_COUNTS_PER_REV`、`MOTOR_B_COUNTS_PER_REV` 改为实际输出轴每圈计数。
然后调用：

```c
if (motor_set_target_rpm(MOTOR_A, 60.0f) != HAL_OK)
{
    motor_stop(MOTOR_A);
    /* 检查初始化状态、目标值与 COUNTS_PER_REV 配置。 */
}
```

当前每圈计数默认为 0，因此 RPM 接口会返回 `HAL_ERROR`，不能直接用示例数字代替实际电机参数。
