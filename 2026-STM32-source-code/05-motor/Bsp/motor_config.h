/**
  * @file   motor_config.h
  * @brief  集中配置控制周期、方向修正、每圈计数及两路 PID 参数。
  */
#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H

/* 【控制周期】必须与 TIM6 一致：84MHz / 84 / 10000 = 100Hz，即 10ms。 */
#define MOTOR_CONTROL_PERIOD_S       0.01f
/* 【输出与测速】DRV8848：TIM2 为 1kHz，更新/比较中断直接控制 PD8～PD11。
 * 这些引脚没有定时器 PWM 复用功能，因此使用 GPIO 软件 PWM。
 */
#define MOTOR_PWM_LIMIT              1000       /* 最大千分比：1000 = 100%。 */
#define MOTOR_MAX_TARGET_CPS         100000.0f  /* 目标指令上限，不代表电机能达到。 */
#define MOTOR_SPEED_FILTER_ALPHA     0.35f      /* 0～1；越小越平滑，响应也越慢。 */

/* 【方向】只能填 +1 或 -1。正向 PWM 必须测出正速度，才能正常闭环。
 * DRIVE_SIGN 决定机械正方向；ENCODER_SIGN 修正编码器计数的方向。
 */
#define MOTOR_A_DRIVE_SIGN           1
#define MOTOR_B_DRIVE_SIGN           1
#define MOTOR_A_ENCODER_SIGN         1
#define MOTOR_B_ENCODER_SIGN         1

/* 【RPM 换算】输出轴每圈计数 = 电机轴单相脉冲数 × 4 × 减速比。
 * 仅举例：单相 11 脉冲、减速比 30:1 => 1320，不能当作本电机实际参数。
 * 0 表示未知：CPS 控制正常，RPM 指令返回 HAL_ERROR，speed_rpm 显示 0。
 */
#define MOTOR_A_COUNTS_PER_REV       0.0f
#define MOTOR_B_COUNTS_PER_REV       0.0f

/* 【PID 参数】保留目前的调试值；两路需要按实际电机和负载分别整定。
 * Kp：误差越大，调整越大；Ki：消除持续误差；Kd：抑制变化，当前不用。
 * 误差单位 CPS，输出单位千分比，积分时间单位为秒。
 */
#define MOTOR_A_KP                   0.20f
#define MOTOR_A_KI                   1.32f
#define MOTOR_A_KD                   0.00f
#define MOTOR_B_KP                   0.20f
#define MOTOR_B_KI                   0.80f
#define MOTOR_B_KD                   0.00f

#if (MOTOR_PWM_LIMIT < 1) || (MOTOR_PWM_LIMIT > 1000)
#error "MOTOR_PWM_LIMIT must be in 1..1000"
#endif
#if ((MOTOR_A_DRIVE_SIGN != 1) && (MOTOR_A_DRIVE_SIGN != -1)) || \
    ((MOTOR_B_DRIVE_SIGN != 1) && (MOTOR_B_DRIVE_SIGN != -1)) || \
    ((MOTOR_A_ENCODER_SIGN != 1) && (MOTOR_A_ENCODER_SIGN != -1)) || \
    ((MOTOR_B_ENCODER_SIGN != 1) && (MOTOR_B_ENCODER_SIGN != -1))
#error "Motor and encoder signs must be +1 or -1"
#endif

#endif
