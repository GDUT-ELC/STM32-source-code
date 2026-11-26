/**********************************************************
 * 文件名: motor.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 * 
 * 功能说明:
 * 实现双电机的初始化、速度设置、停车和反馈读取。
 * 通过编码器计算实际速度，再使用 PID 调整电机输出。
 * 
 * 接口备注:
 * 1.主程序包含 motor.h，先调用 motor_init，再设置目标。
 * 2.motor_control_tick 由 TIM6 每 10ms 调用一次。
 **********************************************************/

#include "motor.h"
#include "motor_pwm.h"
#include "pid.h"
#include "tim.h"
#include <float.h>
#include <stddef.h>


volatile motor_feedback_t g_motor_feedback[MOTOR_COUNT];
static speed_pid_t s_pid[MOTOR_COUNT];
static uint16_t s_previous_count[MOTOR_COUNT];
static uint8_t s_ready;
static uint32_t s_previous_time_us;
static const int8_t s_drive_sign[MOTOR_COUNT] = {MOTOR_A_DRIVE_SIGN, MOTOR_B_DRIVE_SIGN};
static const int8_t s_encoder_sign[MOTOR_COUNT] = {MOTOR_A_ENCODER_SIGN, MOTOR_B_ENCODER_SIGN};
static const float s_counts_per_rev[MOTOR_COUNT] = {MOTOR_A_COUNTS_PER_REV, MOTOR_B_COUNTS_PER_REV};

/**
  * @brief  检查电机编号是否有效
  * @param  id 电机编号：MOTOR_A 或 MOTOR_B
  * @retval 1：有效；0：无效
  */
static uint8_t valid_id(motor_id_t id)
{
    return (uint32_t)id < MOTOR_COUNT;
}

/**
  * @brief  检查浮点数是否为有限值，排除 NaN 和无穷大
  * @param  value 待检查的数值
  * @retval 1：有限值；0：无效值
  */
static uint8_t finite_float(float value)
{
    return value == value && value <= FLT_MAX && value >= -FLT_MAX;
}

/**
  * @brief  把目标速度限制在允许的 CPS 范围内
  * @param  value 原始目标速度，单位编码计数/秒
  * @retval 限幅后的速度；无效数值按 0 处理
  */
static float limit_target(float value)
{
    if (!finite_float(value)) return 0.0f;
    if (value > MOTOR_MAX_TARGET_CPS) return MOTOR_MAX_TARGET_CPS;
    if (value < -MOTOR_MAX_TARGET_CPS) return -MOTOR_MAX_TARGET_CPS;
    return value;
}

/**
  * @brief  暂时关闭中断，保护主循环与控制中断共享的数据
  * @param  None
  * @retval 进入前的 PRIMASK 中断屏蔽状态
  */
static uint32_t lock_control(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

/**
  * @brief  恢复进入临界区之前的中断状态
  * @param  primask lock_control() 返回的原始状态
  * @retval None
  */
static void unlock_control(uint32_t primask)
{
    __set_PRIMASK(primask);
}

/**
  * @brief  对单路 PWM 指令限幅、修正方向并交给底层输出
  * @param  id MOTOR_A 或 MOTOR_B
  * @param  duty 有符号占空比，单位千分比
  * @retval None
  * @note   内部函数；调用方应避免与 TIM6 同时修改反馈数据。
  */
static void write_pwm(motor_id_t id, int16_t duty)
{
    if (duty > MOTOR_PWM_LIMIT) duty = MOTOR_PWM_LIMIT;
    if (duty < -MOTOR_PWM_LIMIT) duty = -MOTOR_PWM_LIMIT;
    motor_pwm_set((uint32_t)id, (int16_t)(duty * s_drive_sign[id]));
    g_motor_feedback[id].pwm = duty;
}

/**
  * @brief  初始化两路电机控制，启动编码器、GPIO PWM 和 PID 节拍
  * @param  None
  * @retval HAL_OK：成功；HAL_ERROR：启动失败并停车
  * @note   先完成 MX_GPIO_Init 和 MX_TIM2/3/4/5/6_Init；成功后重复调用不重新初始化。
  */
HAL_StatusTypeDef motor_init(void)
{
    uint32_t i;
    if (s_ready) return HAL_OK;
    s_ready = 0U;
    for (i = 0; i < MOTOR_COUNT; ++i) {
        g_motor_feedback[i].target_cps = 0.0f;
        g_motor_feedback[i].speed_cps = 0.0f;
        g_motor_feedback[i].speed_rpm = 0.0f;
        g_motor_feedback[i].delta_counts = 0;
        g_motor_feedback[i].mode = MOTOR_SPEED_LOOP;
        g_motor_feedback[i].encoder_count = 0U;
        g_motor_feedback[i].control_ticks = 0U;
        g_motor_feedback[i].pid_integral = 0.0f;
        write_pwm((motor_id_t)i, 0);
    }
    speed_pid_init(&s_pid[MOTOR_A], MOTOR_A_KP, MOTOR_A_KI, MOTOR_A_KD, MOTOR_PWM_LIMIT);
    speed_pid_init(&s_pid[MOTOR_B], MOTOR_B_KP, MOTOR_B_KI, MOTOR_B_KD, MOTOR_PWM_LIMIT);
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_RESET);
    if (HAL_TIM_Encoder_Start(&htim4, TIM_CHANNEL_ALL) != HAL_OK
     || HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL) != HAL_OK
     || motor_pwm_init() != HAL_OK) {     //初始化失败就停车并告知 main。
        motor_stop_all();
        return HAL_ERROR;
    }
    s_previous_count[MOTOR_A] = (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);
    s_previous_count[MOTOR_B] = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
    __HAL_TIM_SET_COUNTER(&htim6, 0);
    __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE);
    s_ready = 1U;
		HAL_GPIO_WritePin(GPIOC,GPIO_PIN_9,GPIO_PIN_SET);
    if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK) {
        s_ready = 0U;
        motor_stop_all();
        return HAL_ERROR;
    }
    return HAL_OK;
}

/**
  * @brief  设置单路目标速度，启用编码器 PID 闭环
  * @param  id MOTOR_A 或 MOTOR_B
  * @param  counts_per_second 四倍频编码计数/秒；正负表示方向，0 表示制动停车
  * @retval None
  * @note   初始化成功后调用，目标会一直保持；不是 RPM，也不是 PWM。
  */
void motor_set_target_cps(motor_id_t id, float counts_per_second)
{
    uint32_t key;
    float target;
    float previous;
    if (!valid_id(id) || !s_ready) return;
    target = limit_target(counts_per_second);
    key = lock_control();
    previous = g_motor_feedback[id].target_cps;
	
    if (g_motor_feedback[id].mode != MOTOR_SPEED_LOOP || target == 0.0f
     || (previous > 0.0f && target < 0.0f) || (previous < 0.0f && target > 0.0f)) {
        speed_pid_reset(&s_pid[id], g_motor_feedback[id].speed_cps);
        g_motor_feedback[id].pid_integral = 0.0f;
        write_pwm(id, 0);
    }
    g_motor_feedback[id].target_cps = target;
    g_motor_feedback[id].mode = MOTOR_SPEED_LOOP;
    unlock_control(key);
}

/**
  * @brief  将目标转速换算成 CPS 后启用速度闭环
  * @param  id MOTOR_A 或 MOTOR_B
  * @param  rpm 输出轴目标转速，单位转/分，正负表示方向
  * @retval HAL_OK：指令已受理；HAL_ERROR：编号、状态、转速或每圈计数无效
  * @note   先在 motor_config.h 填入实际 COUNTS_PER_REV；换算结果仍受 CPS 上限限制。
  */
HAL_StatusTypeDef motor_set_target_rpm(motor_id_t id, float rpm)
{
    float cps;
    if (!valid_id(id) || !s_ready || !finite_float(s_counts_per_rev[id])
     || s_counts_per_rev[id] <= 0.0f || !finite_float(rpm)) {
        return HAL_ERROR;
    }
    cps = rpm / 60.0f * s_counts_per_rev[id];
    if (!finite_float(cps)) return HAL_ERROR;
    motor_set_target_cps(id, cps);
    return HAL_OK;
}

/**
  * @brief  设置单路固定占空比，切换为 PWM 开环控制
  * @param  id MOTOR_A 或 MOTOR_B
  * @param  duty_per_mille -1000～1000；300 为 30%，负数反向，0 为制动
  * @retval None
  * @note   编码器仍然测速，但 PID 不再自动调整该路输出。
  */
void motor_set_pwm(motor_id_t id, int16_t duty_per_mille)
{
    uint32_t key;
    if (!valid_id(id) || !s_ready) return;
    key = lock_control();
    g_motor_feedback[id].mode = MOTOR_OPEN_LOOP;
    g_motor_feedback[id].target_cps = 0.0f;
    speed_pid_reset(&s_pid[id], g_motor_feedback[id].speed_cps);
    g_motor_feedback[id].pid_integral = 0.0f;
    write_pwm(id, duty_per_mille);
    unlock_control(key);
}

/**
  * @brief  立即制动单路电机，清除目标和 PID 积分
  * @param  id MOTOR_A 或 MOTOR_B
  * @retval None
  * @note   后续中断保持停车；重新发送非零速度或 PWM 指令才会再次驱动。
  */
void motor_stop(motor_id_t id)
{
    uint32_t key;
    if (!valid_id(id)) return;
    key = lock_control();
    g_motor_feedback[id].target_cps = 0.0f;
    g_motor_feedback[id].mode = MOTOR_SPEED_LOOP;
    speed_pid_reset(&s_pid[id], g_motor_feedback[id].speed_cps);
    g_motor_feedback[id].pid_integral = 0.0f;
    write_pwm(id, 0);
    unlock_control(key);
}

/**
  * @brief  立即制动两路电机，并清除两路目标和积分
  * @param  None
  * @retval None
  */
void motor_stop_all(void)
{
    uint32_t key = lock_control();
    motor_stop(MOTOR_A);
    motor_stop(MOTOR_B);
    unlock_control(key);
}

/**
  * @brief  读取单路完整反馈快照，避免读取过程中被中断更新
  * @param  id MOTOR_A 或 MOTOR_B
  * @param  feedback 输出参数：指向接收数据的 motor_feedback_t 变量
  * @retval None
  * @note   无效编号或空指针时不写入；主循环读取反馈优先用本函数。
  */
void motor_get_feedback(motor_id_t id, motor_feedback_t *feedback)
{
    uint32_t key;
    if (!valid_id(id) || feedback == NULL) return;
    key = lock_control();
    *feedback = g_motor_feedback[id];
    unlock_control(key);
}

/**
  * @brief  更新单路 PID 系数并清空旧积分
  * @param  id MOTOR_A 或 MOTOR_B
  * @param  kp 非负比例系数
  * @param  ki 非负积分系数，时间单位秒
  * @param  kd 非负微分系数，可先设为 0
  * @retval None
  * @note   初始化后按需调用，不能在主循环每次迭代都调用，否则积分无法累计。
  */
void motor_set_pid(motor_id_t id, float kp, float ki, float kd)
{
    uint32_t key;
    if (!valid_id(id) || !s_ready || !finite_float(kp) || !finite_float(ki) || !finite_float(kd)
     || kp < 0.0f || ki < 0.0f || kd < 0.0f) return;
    key = lock_control();
    speed_pid_init(&s_pid[id], kp, ki, kd, MOTOR_PWM_LIMIT);
    speed_pid_reset(&s_pid[id], g_motor_feedback[id].speed_cps);
    g_motor_feedback[id].pid_integral = 0.0f;
    unlock_control(key);
}

/**
  * @brief  由 TIM6 周期触发，以 TIM5 实测间隔计算速度和两路 PID
  * @param  None
  * @retval None
  * @note   正常约 10ms；超 30ms 丢弃采样，闭环制动一次后按原目标恢复；不要在 main 重复调用。
  */
void motor_control_tick(void)
{
    uint32_t i;
    uint32_t now_us;
    uint32_t elapsed_us;
    float dt;
    uint16_t current[MOTOR_COUNT];
    if (!s_ready) return;
    now_us = __HAL_TIM_GET_COUNTER(&htim5);
    elapsed_us = now_us - s_previous_time_us; 
    if (elapsed_us < MOTOR_MIN_SAMPLE_US) return;
    s_previous_time_us = now_us;
    dt = (float)elapsed_us * 0.000001f;
    current[MOTOR_A] = (uint16_t)__HAL_TIM_GET_COUNTER(&htim4);
    current[MOTOR_B] = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
    if (elapsed_us > MOTOR_MAX_SAMPLE_US) {
       
        for (i = 0; i < MOTOR_COUNT; ++i) {
            s_previous_count[i] = current[i];
            g_motor_feedback[i].encoder_count = current[i];
            g_motor_feedback[i].sample_us = elapsed_us;
            ++g_motor_feedback[i].timing_resets;
            g_motor_feedback[i].delta_counts = 0;
            g_motor_feedback[i].speed_cps = 0.0f;
            g_motor_feedback[i].speed_rpm = 0.0f;
            speed_pid_reset(&s_pid[i], 0.0f);
            g_motor_feedback[i].pid_integral = 0.0f;
            if (g_motor_feedback[i].mode == MOTOR_SPEED_LOOP) write_pwm((motor_id_t)i, 0);
        }
        return;
    }
    for (i = 0; i < MOTOR_COUNT; ++i) {
        int32_t delta = (uint16_t)(current[i] - s_previous_count[i]);
        float speed;
        float output;
        
        if (delta >= 32768) delta -= 65536;
        s_previous_count[i] = current[i];
        delta *= s_encoder_sign[i];
        g_motor_feedback[i].delta_counts = delta;
        g_motor_feedback[i].encoder_count = current[i];
        g_motor_feedback[i].sample_us = elapsed_us;
        ++g_motor_feedback[i].control_ticks;
        
        speed = (float)delta / dt;
        g_motor_feedback[i].speed_cps += MOTOR_SPEED_FILTER_ALPHA
            * (speed - g_motor_feedback[i].speed_cps);
        g_motor_feedback[i].speed_rpm = s_counts_per_rev[i] > 0.0f
            ? g_motor_feedback[i].speed_cps * 60.0f / s_counts_per_rev[i] : 0.0f;
        if (g_motor_feedback[i].mode != MOTOR_SPEED_LOOP) continue;
        if (g_motor_feedback[i].target_cps == 0.0f) {
            speed_pid_reset(&s_pid[i], g_motor_feedback[i].speed_cps);
            write_pwm((motor_id_t)i, 0);
        } else {
            output = speed_pid_update(&s_pid[i], g_motor_feedback[i].target_cps,
                g_motor_feedback[i].speed_cps, dt);
            write_pwm((motor_id_t)i, (int16_t)output);
        }
        g_motor_feedback[i].pid_integral = s_pid[i].integral;
    }
}

/**
  * @brief  旧版兼容接口：设置 A 路百分比 PWM，切换为开环
  * @param  speed -100～100 的有符号百分比，50 表示 50%
  * @retval None
  * @note   新代码优先使用 motor_set_pwm(MOTOR_A, 千分比)。
  */
void motor_setspeed1(int8_t speed)
{
    motor_set_pwm(MOTOR_A, (int16_t)speed * 10);
}

/**
  * @brief  旧版兼容接口：设置 B 路百分比 PWM，切换为开环
  * @param  speed -100～100 的有符号百分比，50 表示 50%
  * @retval None
  * @note   新代码优先使用 motor_set_pwm(MOTOR_B, 千分比)。
  */
void motor_setspeed2(int8_t speed)
{
    motor_set_pwm(MOTOR_B, (int16_t)speed * 10);
}
