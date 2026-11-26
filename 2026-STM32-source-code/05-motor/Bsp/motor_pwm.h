/**********************************************************
 * 文件名: motor_pwm.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 * 
 * 功能说明:
 * 声明双电机 GPIO PWM 的初始化、输出和中断处理函数。
 * 
 * 接口备注:
 * 1.供 motor.c 和 TIM2 中断入口使用。
 * 2.主程序使用 motor.h 中的电机控制接口。
 **********************************************************/
 
#ifndef MOTOR_PWM_H
#define MOTOR_PWM_H

#include "main.h"

/* 底层驱动：PD8～PD11 输出 DRV8848 输入电平，TIM2 仅提供周期/比较事件。
 * 主程序应调用 motor.h 中的电机接口，不需要直接调用本文件的底层函数。
 */
/**
  * @brief  初始化 DRV8848 GPIO PWM，启动 TIM2 并保持制动
  * @param  None
  * @retval HAL_OK：TIM2 启动成功；其他 HAL 状态：启动失败
  * @note   由 motor_init() 调用；TIM2 必须已经配置为 1kHz。
  */
HAL_StatusTypeDef motor_pwm_init(void);
/**
  * @brief  异常时关闭 PWM 中断并直接制动，不依赖 HAL 或浮点运算
  * @param  None
  * @retval None
  * @note   供故障处理调用；恢复正常控制必须复位系统。
  */
void motor_pwm_emergency_stop(void);
/**
  * @brief  保存 DRV8848 PWM 指令，在下一 PWM 周期应用
  * @param  motor 电机下标：0 为 A 路，1 为 B 路
  * @param  duty -1000～1000 的有符号千分比，已包含方向修正
  * @retval None
  * @note   停车和换向会先立即制动；普通更新最多等待约 1ms。
  */
void motor_pwm_set(uint32_t motor, int16_t duty);
/**
  * @brief  处理 TIM2 周期和比较事件，生成两路 GPIO PWM 波形
  * @param  None
  * @retval None
  * @note   由 TIM2_IRQHandler() 直接调用；不使用 HAL 重复处理同一批标志。
  */
void motor_pwm_irq(void);

#endif
