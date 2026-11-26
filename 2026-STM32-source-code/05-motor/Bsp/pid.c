/**********************************************************
 * 文件名: pid.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 * 
 * 功能说明:
 * 实现速度 PID 计算，根据目标速度与实际速度的差值调整输出。
 * 包含输出限幅和积分限幅，输出饱和时停止同方向积分。
 * 
 * 接口备注:
 * 1.先调用 speed_pid_init，再按固定周期调用 speed_pid_update。
 * 2.每路电机使用独立的 speed_pid_t 结构体。
 **********************************************************/
 
#include "pid.h"
#include <float.h>

/**
  * @brief  检查 PID 输入和中间运算是否为有限值
  * @param  value 待检查的浮点数
  * @retval 1：有效；0：NaN 或无穷大
  */
static int pid_finite(float value)
{
    return value == value && value <= FLT_MAX && value >= -FLT_MAX;
}

/**
  * @brief  把浮点数限制在正负上限之间
  * @param  value 输入数值
  * @param  limit 正的绝对值上限
  * @retval 范围为 -limit～limit 的数值
  */
static float clamp(float value, float limit)
{
    if (value > limit) return limit;
    if (value < -limit) return -limit;
    return value;
}

/**
  * @brief  清空 PID 积分并记录当前速度，避免旧状态影响下一次控制
  * @param  pid 指向有效的 PID 状态结构体
  * @param  measurement 当前实测速度，单位 CPS
  * @retval None
  */
void speed_pid_reset(speed_pid_t *pid, float measurement)
{
    pid->integral = 0.0f;
    pid->previous_measurement = measurement;
}

/**
  * @brief  设置 PID 系数和输出上限，初始化内部状态
  * @param  pid 指向有效的 PID 状态结构体
  * @param  kp 比例系数
  * @param  ki 积分系数，时间单位秒
  * @param  kd 测量速度的微分系数
  * @param  limit 输出绝对值上限，单位千分比
  * @retval None
  */
void speed_pid_init(speed_pid_t *pid, float kp, float ki, float kd, float limit)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->output_limit = limit;
    speed_pid_reset(pid, 0.0f);
}

/**
  * @brief  计算一次速度 PID，包含输出限幅和抗积分饱和
  * @param  pid 指向有效的 PID 状态结构体
  * @param  target 目标速度，单位 CPS
  * @param  measurement 实测速度，单位 CPS
  * @param  dt 实际采样间隔，单位秒，正常约 0.01f
  * @retval 有符号 PWM 千分比；输入或运算结果无效时返回 0
  * @note   微分针对测量值，避免目标突变引起微分冲击；参数由电机层检查。
  */
float speed_pid_update(speed_pid_t *pid, float target, float measurement, float dt)
{
    float error;
    float next_integral;
    float proportional_derivative;
    float candidate;

    if (!pid_finite(dt) || dt <= 0.0f || !pid_finite(target)
     || !pid_finite(measurement)) return 0.0f;

    error = target - measurement;

    next_integral = clamp(pid->integral + pid->ki * error * dt, pid->output_limit);

    proportional_derivative = pid->kp * error
        - pid->kd * (measurement - pid->previous_measurement) / dt;
    candidate = proportional_derivative + next_integral;

    if (!pid_finite(proportional_derivative) || !pid_finite(candidate)) {
        speed_pid_reset(pid, measurement);
        return 0.0f;
    }


    if (!((candidate > pid->output_limit && error > 0.0f)
       || (candidate < -pid->output_limit && error < 0.0f))) {
        pid->integral = next_integral;
    }
    pid->previous_measurement = measurement;
    return clamp(proportional_derivative + pid->integral, pid->output_limit);
}
