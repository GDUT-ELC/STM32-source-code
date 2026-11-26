/**********************************************************
 * 文件名: motor.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 * 
 * 功能说明:
 * 定义电机编号、反馈数据结构和电机控制函数。
 * 
 * 接口备注:
 * 1.MOTOR_A、MOTOR_B 分别表示 A 路和 B 路电机。
 * 2.闭环速度单位为 CPS，即编码器每秒的四倍频计数。
 * 3.开环 PWM 单位为千分比，300 表示 30%。
 **********************************************************/

#ifndef MOTOR_H
#define MOTOR_H

#include "main.h"
#include "motor_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 编号与 Watch 下标一致：A 路为 [0]，B 路为 [1]。 */
typedef enum { MOTOR_A = 0, MOTOR_B = 1, MOTOR_COUNT = 2 } motor_id_t;
typedef enum { MOTOR_OPEN_LOOP = 0, MOTOR_SPEED_LOOP = 1 } motor_mode_t;

typedef struct {
    float target_cps;       /* 目标速度：编码计数/秒，带方向。 */
    float speed_cps;        /* 实际速度：滤波后的编码计数/秒。 */
    float speed_rpm;        /* 实际转/分；每圈计数未配置时始终为 0。 */
    int32_t delta_counts;   /* 最近采样的有符号计数差，间隔见 sample_us，不是累计值。 */
    int16_t pwm;            /* 输出指令：-1000～1000，300 表示 30%。 */
    motor_mode_t mode;      /* 0：开环；1：编码器速度闭环。 */
    uint16_t encoder_count; /* 本次读取的原始 16 位编码器计数。 */
    uint32_t control_ticks; /* 有效测速/PID 执行次数，运行时应持续增加。 */
    uint32_t sample_us;     /* 最近采样间隔，正常约 10000us。 */
    uint32_t timing_resets; /* 采样超过 30ms 的恢复次数。 */
    float pid_integral;     /* PID 积分输出；开环或复位后为 0。 */
} motor_feedback_t;

/* Keil Watch 中直接观察，勿直接写入；程序读取整组数据用 motor_get_feedback()。 */
extern volatile motor_feedback_t g_motor_feedback[MOTOR_COUNT];

/**
  * @brief  初始化两路电机控制，启动编码器、GPIO PWM 和 PID 节拍
  * @param  None
  * @retval HAL_OK：成功；HAL_ERROR：启动失败并停车
  * @note   先完成 MX_GPIO_Init 和 MX_TIM2/3/4/5/6_Init；成功后重复调用不重新初始化。
  */
HAL_StatusTypeDef motor_init(void);
/**
  * @brief  设置单路目标速度，启用编码器 PID 闭环
  * @param  id MOTOR_A 或 MOTOR_B
  * @param  counts_per_second 四倍频编码计数/秒；正负表示方向，0 表示制动停车
  * @retval None
  * @note   初始化成功后调用，目标会一直保持；不是 RPM，也不是 PWM。
  */
void motor_set_target_cps(motor_id_t id, float counts_per_second);
/**
  * @brief  将目标转速换算成 CPS 后启用速度闭环
  * @param  id MOTOR_A 或 MOTOR_B
  * @param  rpm 输出轴目标转速，单位转/分，正负表示方向
  * @retval HAL_OK：指令已受理；HAL_ERROR：编号、状态、转速或每圈计数无效
  * @note   先在 motor_config.h 填入实际 COUNTS_PER_REV；换算结果仍受 CPS 上限限制。
  */
HAL_StatusTypeDef motor_set_target_rpm(motor_id_t id, float rpm);
/**
  * @brief  设置单路固定占空比，切换为 PWM 开环控制
  * @param  id MOTOR_A 或 MOTOR_B
  * @param  duty_per_mille -1000～1000；300 为 30%，负数反向，0 为制动
  * @retval None
  * @note   编码器仍然测速，但 PID 不再自动调整该路输出。
  */
void motor_set_pwm(motor_id_t id, int16_t duty_per_mille);
/**
  * @brief  立即制动单路电机，清除目标和 PID 积分
  * @param  id MOTOR_A 或 MOTOR_B
  * @retval None
  * @note   后续中断保持停车；重新发送非零速度或 PWM 指令才会再次驱动。
  */
void motor_stop(motor_id_t id);
/**
  * @brief  立即制动两路电机，并清除两路目标和积分
  * @param  None
  * @retval None
  */
void motor_stop_all(void);
/**
  * @brief  读取单路完整反馈快照，避免读取过程中被中断更新
  * @param  id MOTOR_A 或 MOTOR_B
  * @param  feedback 输出参数：指向接收数据的 motor_feedback_t 变量
  * @retval None
  * @note   无效编号或空指针时不写入；主循环读取反馈优先用本函数。
  */
void motor_get_feedback(motor_id_t id, motor_feedback_t *feedback);
/**
  * @brief  更新单路 PID 系数并清空旧积分
  * @param  id MOTOR_A 或 MOTOR_B
  * @param  kp 非负比例系数
  * @param  ki 非负积分系数，时间单位秒
  * @param  kd 非负微分系数，可先设为 0
  * @retval None
  * @note   初始化后按需调用，不能在主循环每次迭代都调用，否则积分无法累计。
  */
void motor_set_pid(motor_id_t id, float kp, float ki, float kd);
/**
  * @brief  由 TIM6 周期触发，以 TIM5 实测间隔计算速度和两路 PID
  * @param  None
  * @retval None
  * @note   正常约 10ms；超 30ms 丢弃采样，闭环制动一次后按原目标恢复；不要在 main 重复调用。
  */
void motor_control_tick(void);

/* 以下为旧版兼容接口；新代码优先使用 motor_set_pwm()，注意单位不同。 */
/**
  * @brief  旧版兼容接口：设置 A 路百分比 PWM，切换为开环
  * @param  speed -100～100 的有符号百分比，50 表示 50%
  * @retval None
  * @note   新代码优先使用 motor_set_pwm(MOTOR_A, 千分比)。
  */
void motor_setspeed1(int8_t speed);
/**
  * @brief  旧版兼容接口：设置 B 路百分比 PWM，切换为开环
  * @param  speed -100～100 的有符号百分比，50 表示 50%
  * @retval None
  * @note   新代码优先使用 motor_set_pwm(MOTOR_B, 千分比)。
  */
void motor_setspeed2(int8_t speed);

#ifdef __cplusplus
}
#endif
#endif
