/**********************************************************
 * 文件名: motor_pwm.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 * 
 * 功能说明:
 * 实现 DRV8848 双电机的 GPIO PWM 输出。
 * TIM2 周期开始时驱动电机，比较事件到来时进入制动。
 * 
 * 接口备注:
 * 1.由 motor.c 调用，主程序不需要直接操作这些函数。
 * 2.motor_pwm_irq 由 TIM2_IRQHandler 调用。
 * 3.沿用 PD8～PD11 接线，TIM2 只提供内部计时。
 **********************************************************/

#include "motor_pwm.h"
#include "tim.h"

/* 待生效的指令：主程序/PID 写入，TIM2 在下个 PWM 周期统一读取。 */
static volatile int16_t s_pending[2];
static const uint32_t s_pin1[2] = {AIN1_Pin, BIN1_Pin};
static const uint32_t s_pin2[2] = {AIN2_Pin, BIN2_Pin};
static const uint32_t s_compare_interrupt[2] = {TIM_DIER_CC1IE, TIM_DIER_CC2IE};

/**
  * @brief  异常时关闭 PWM 中断并直接制动，不依赖 HAL 或浮点运算
  * @param  None
  * @retval None
  * @note   供故障处理调用；恢复正常控制必须复位系统。
  */
void motor_pwm_emergency_stop(void)
{
    uint32_t key = __get_PRIMASK();
    __disable_irq();
    s_pending[0] = 0;
    s_pending[1] = 0;
    if (__HAL_RCC_TIM2_IS_CLK_ENABLED()) TIM2->DIER = 0U;
    if (__HAL_RCC_GPIOD_IS_CLK_ENABLED())
        GPIOD->BSRR = AIN1_Pin | AIN2_Pin | BIN1_Pin | BIN2_Pin;
    __set_PRIMASK(key);
}

/**
  * @brief  保存 DRV8848 PWM 指令，在下一 PWM 周期应用
  * @param  motor 电机下标：0 为 A 路，1 为 B 路
  * @param  duty -1000～1000 的有符号千分比，已包含方向修正
  * @retval None
  * @note   停车和换向会先立即制动；普通更新最多等待约 100us。
  */
void motor_pwm_set(uint32_t motor, int16_t duty)
{
    uint32_t key;
    int16_t previous;
    if (motor >= 2U) return;
    if (duty > 1000) duty = 1000;
    if (duty < -1000) duty = -1000;
    key = __get_PRIMASK();
    __disable_irq();
    previous = s_pending[motor];
    s_pending[motor] = duty;
    if (duty == 0 || (previous > 0 && duty < 0) || (previous < 0 && duty > 0)) {
        /* 停车/换向先立即制动，新的驱动方向等下一个 PWM 周期再输出。 */
        TIM2->DIER &= ~s_compare_interrupt[motor];
        GPIOD->BSRR = s_pin1[motor] | s_pin2[motor];
    }
    __set_PRIMASK(key);
}

/**
  * @brief  初始化 DRV8848 GPIO PWM，启动 TIM2 并保持制动
  * @param  None
  * @retval HAL_OK：TIM2 启动成功；其他 HAL 状态：启动失败
  * @note   由 motor_init() 调用；TIM2 必须已经配置为 10kHz。
  */
HAL_StatusTypeDef motor_pwm_init(void)
{
    uint32_t key = __get_PRIMASK();
    HAL_StatusTypeDef result;
    __disable_irq();
    s_pending[0] = 0;
    s_pending[1] = 0;
    GPIOD->BSRR = AIN1_Pin | AIN2_Pin | BIN1_Pin | BIN2_Pin;
    TIM2->DIER = 0;
    TIM2->CCMR1 = 0;
    TIM2->CCER = 0;
    TIM2->CNT = 0;
    TIM2->SR = 0;
    result = HAL_TIM_Base_Start_IT(&htim2);
    __set_PRIMASK(key);
    return result;
}

/**
  * @brief  处理 TIM2 周期和比较事件，生成两路 GPIO PWM 波形
  * @param  None
  * @retval None
  * @note   由 TIM2_IRQHandler() 直接调用；不使用 HAL 重复处理同一批标志。
  */
void motor_pwm_irq(void)
{
    uint32_t flags = TIM2->SR & TIM2->DIER;
    if ((flags & TIM_SR_UIF) != 0U) {
        uint32_t i;
        uint32_t start_pins = 0;
        uint32_t interrupts = TIM_DIER_UIE;
        uint32_t period = TIM2->ARR + 1U;
        uint32_t compare[2] = {0, 0};

        TIM2->DIER = TIM_DIER_UIE;
        TIM2->SR = ~(TIM_SR_UIF | TIM_SR_CC1IF | TIM_SR_CC2IF);
        for (i = 0; i < 2U; ++i) {
            int32_t duty = s_pending[i];
            uint32_t magnitude = (uint32_t)(duty < 0 ? -duty : duty);
            if (duty == 0) {
                start_pins |= s_pin1[i] | s_pin2[i];  
            } else {
                start_pins |= duty > 0 ? s_pin2[i] | (s_pin1[i] << 16U)
                                      : s_pin1[i] | (s_pin2[i] << 16U);
                if (magnitude < 1000U) {
                    compare[i] = magnitude * period / 1000U;
                    interrupts |= s_compare_interrupt[i];
                }
            }
        }
        TIM2->CCR1 = compare[0];
        TIM2->CCR2 = compare[1];

        TIM2->SR = ~(TIM_SR_CC1IF | TIM_SR_CC2IF);
        GPIOD->BSRR = start_pins;

        for (i = 0; i < 2U; ++i) {
            if ((interrupts & s_compare_interrupt[i]) != 0U && TIM2->CNT >= compare[i]) {
                GPIOD->BSRR = s_pin1[i] | s_pin2[i];
                interrupts &= ~s_compare_interrupt[i];
            }
        }
        TIM2->DIER = interrupts;
    } else {
        uint32_t off_pins = 0;
        TIM2->SR = ~flags;
        if ((flags & TIM_SR_CC1IF) != 0U) off_pins |= AIN1_Pin | AIN2_Pin;
        if ((flags & TIM_SR_CC2IF) != 0U) off_pins |= BIN1_Pin | BIN2_Pin;
        if (off_pins != 0U) GPIOD->BSRR = off_pins;
    }
}
