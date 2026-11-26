/**********************************************************
 * 文件名: pid.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 * 
 * 功能说明:
 * 定义 PID 参数、运行状态以及初始化和计算函数。
 * 
 * 接口备注:
 * 1.速度单位为 CPS，时间单位为秒，输出为 PWM 千分比。
 **********************************************************/

#ifndef MOTOR_PID_H
#define MOTOR_PID_H

typedef struct {
    float kp;                    /* 比例系数。 */
    float ki;                    /* 积分系数，时间单位秒。 */
    float kd;                    /* 微分系数，对实际速度求微分。 */
    float integral;              /* 已累计的积分输出。 */
    float previous_measurement;  /* 上一次的实际速度。 */
    float output_limit;          /* 输出绝对值上限，单位千分比。 */
} speed_pid_t;

/**
  * @brief  设置 PID 系数和输出上限，初始化内部状态
  * @param  pid 指向有效的 PID 状态结构体
  * @param  kp 比例系数
  * @param  ki 积分系数，时间单位秒
  * @param  kd 测量速度的微分系数
  * @param  limit 输出绝对值上限，单位千分比
  * @retval None
  */
void speed_pid_init(speed_pid_t *pid, float kp, float ki, float kd, float limit);
/**
  * @brief  清空 PID 积分并记录当前速度，避免旧状态影响下一次控制
  * @param  pid 指向有效的 PID 状态结构体
  * @param  measurement 当前实测速度，单位 CPS
  * @retval None
  */
void speed_pid_reset(speed_pid_t *pid, float measurement);
/**
  * @brief  计算一次速度 PID，包含输出限幅和抗积分饱和
  * @param  pid 指向有效的 PID 状态结构体
  * @param  target 目标速度，单位 CPS
  * @param  measurement 实测速度，单位 CPS
  * @param  dt 实际采样间隔，单位秒，正常约 0.01f
  * @retval 有符号 PWM 千分比；输入或运算结果无效时返回 0
  * @note   微分针对测量值，避免目标突变引起微分冲击；参数由电机层检查。
  */
float speed_pid_update(speed_pid_t *pid, float target, float measurement, float dt);

#endif
