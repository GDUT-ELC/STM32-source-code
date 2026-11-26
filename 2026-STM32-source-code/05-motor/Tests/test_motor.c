#include "motor.h"
#include "motor_pwm.h"
#include "pid.h"
#include "tim.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <float.h>

GPIO_TypeDef test_gpiod;
uint32_t test_primask;
static test_timer_t tim3, tim4, tim6;
uint32_t test_tim2_clock = 1U, test_gpiod_clock = 1U;
test_timer_t test_tim2;
TIM_HandleTypeDef htim2 = {&test_tim2};
TIM_HandleTypeDef htim3 = {&tim3}, htim4 = {&tim4};
TIM_HandleTypeDef htim6 = {&tim6};
static uint32_t encoder_starts, timer_starts;


/**
  * @brief  推进一个正常的 10ms 控制周期
  * @param  None
  * @retval None
  */
static void control_period(void)
{
    motor_control_tick();
}

/**
  * @brief  模拟编码器启动，并检查定时器与通道参数
  * @param  h 模拟 TIM3 或 TIM4 句柄
  * @param  ch 编码器通道集合
  * @retval HAL_OK
  * @note   仅用于电脑端测试，不连接实际硬件。
  */
HAL_StatusTypeDef HAL_TIM_Encoder_Start(TIM_HandleTypeDef *h, uint32_t ch)
{
    assert((h == &htim3 || h == &htim4) && ch == TIM_CHANNEL_ALL);
    ++encoder_starts;
    return HAL_OK;
}
/**
  * @brief  模拟基础定时器启动并记录中断使能状态
  * @param  h 模拟 TIM2 或 TIM6 句柄
  * @retval HAL_OK
  * @note   仅用于电脑端测试。
  */
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *h)
{
    assert(h == &htim6 || h == &htim2);
    h->Instance->DIER |= TIM_DIER_UIE;
    ++timer_starts;
    return HAL_OK;
}

/**
  * @brief  模拟一个 PWM 周期开始，并注入指定中断延迟
  * @param  latency_ticks 进入中断时的计数器值
  * @retval None
  */
static void pwm_period(uint32_t latency_ticks)
{
    test_tim2.SR = TIM_SR_UIF;
    test_tim2.CNT = latency_ticks;
    motor_pwm_irq();
}

/**
  * @brief  模拟 PWM 比较事件，触发对应通道的关闭边沿
  * @param  flags 模拟的比较事件标志
  * @retval None
  */
static void pwm_edge(uint32_t flags)
{
    test_tim2.SR = flags;
    motor_pwm_irq();
}

/**
  * @brief  检查两路输入处于持续制动且不再产生关闭边沿
  * @param  None
  * @retval None
  */
static void assert_braked(void)
{
    pwm_period(0);
    assert(test_tim2.DIER == TIM_DIER_UIE);
    assert(test_gpiod.BSRR == (AIN1_Pin | AIN2_Pin | BIN1_Pin | BIN2_Pin));
}

/**
  * @brief  遍历两路 PWM 占空比并验证比较边沿、换向和延迟处理
  * @param  None
  * @retval None
  */
static void test_gpio_pwm(void)
{
    uint32_t i;
    /* Exercise every fractional duty in both directions on independent channels. */
    for (i = 1; i < 1000; ++i) {
        motor_set_pwm(MOTOR_A, (int16_t)i);
        motor_set_pwm(MOTOR_B, -(int16_t)(1000U - i));
        pwm_period(0);
        assert(test_tim2.CCR1 == i * 84000U / 1000U);
        assert(test_tim2.CCR2 == (1000U - i) * 84000U / 1000U);
        assert(test_tim2.DIER == (TIM_DIER_UIE | TIM_DIER_CC1IE | TIM_DIER_CC2IE));
        assert(test_gpiod.BSRR == (AIN2_Pin | (AIN1_Pin << 16)
                               | BIN1_Pin | (BIN2_Pin << 16)));
        pwm_edge(TIM_SR_CC1IF);
        assert(test_gpiod.BSRR == (AIN1_Pin | AIN2_Pin));
        pwm_edge(TIM_SR_CC2IF);
        assert(test_gpiod.BSRR == (BIN1_Pin | BIN2_Pin));
    }
    motor_set_pwm(MOTOR_A, 500);
    motor_set_pwm(MOTOR_B, 500);
    pwm_period(0);
    pwm_edge(TIM_SR_CC1IF | TIM_SR_CC2IF);
    assert(test_gpiod.BSRR == (AIN1_Pin | AIN2_Pin | BIN1_Pin | BIN2_Pin));

    motor_set_pwm(MOTOR_A, 1); /* A delayed handler must not turn 0.1% into 100%. */
    motor_stop(MOTOR_B);
    pwm_period(1000);
    assert(test_tim2.DIER == TIM_DIER_UIE);
    assert(test_gpiod.BSRR == (AIN1_Pin | AIN2_Pin));

    motor_set_pwm(MOTOR_A, 500);
    pwm_period(0);
    motor_set_pwm(MOTOR_A, -500); /* Reversal brakes now, drives at next rollover. */
    assert((test_tim2.DIER & TIM_DIER_CC1IE) == 0);
    assert(test_gpiod.BSRR == (AIN1_Pin | AIN2_Pin));
    pwm_period(0);
    assert(test_gpiod.BSRR == (AIN1_Pin | (AIN2_Pin << 16) | BIN1_Pin | BIN2_Pin));
    motor_stop_all();
    assert_braked();
}

/**
  * @brief  验证 PID 正反向收敛、输出限幅和积分抗饱和
  * @param  None
  * @retval None
  */
static void test_pid(void)
{
    speed_pid_t pid;
    float speed = 0.0f;
    int i;
    speed_pid_init(&pid, 2.0f, 4.0f, 0.0f, 1000.0f);
    for (i = 0; i < 10000; ++i) {
        assert(speed_pid_update(&pid, 100000.0f, 0.0f, 0.01f) == 1000.0f);
    }
    assert(pid.integral == 0.0f); /* Sustained saturation must not wind up. */
    assert(speed_pid_update(&pid, -100000.0f, 0.0f, 0.01f) == -1000.0f);
    speed_pid_reset(&pid, 0.0f);
    for (i = 0; i < 2000; ++i) {
        float output = speed_pid_update(&pid, 300.0f, speed, 0.01f);
        speed += (output - speed) * 0.02f; /* First-order motor model. */
    }
    assert(fabsf(speed - 300.0f) < 0.1f);
    for (i = 0; i < 2000; ++i) {
        float output = speed_pid_update(&pid, -300.0f, speed, 0.01f);
        speed += (output - speed) * 0.02f;
    }
    assert(fabsf(speed + 300.0f) < 0.1f);
    assert(speed_pid_update(&pid, 1.0f, 0.0f, 0.0f) == 0.0f);
    assert(speed_pid_update(&pid, 1.0f, 0.0f, NAN) == 0.0f);
    assert(speed_pid_update(&pid, NAN, 0.0f, 0.01f) == 0.0f);
    speed_pid_init(&pid, FLT_MAX, 0.0f, 0.0f, 1000.0f);
    assert(speed_pid_update(&pid, 1000.0f, 0.0f, 0.01f) == 0.0f);
}


/**
  * @brief  验证异常停车关闭 PWM，并兼容外设时钟尚未开启的情况
  * @param  None
  * @retval None
  */
static void test_emergency_stop(void)
{
    motor_set_pwm(MOTOR_A, 500);
    pwm_period(0);
    motor_pwm_emergency_stop();
    assert(test_tim2.DIER == 0U);
    assert(test_gpiod.BSRR == (AIN1_Pin | AIN2_Pin | BIN1_Pin | BIN2_Pin));
    motor_pwm_irq();
    assert(test_tim2.DIER == 0U);
    test_tim2_clock = test_gpiod_clock = 0U;
    test_tim2.DIER = 123U;
    test_gpiod.BSRR = 456U;
    motor_pwm_emergency_stop();
    assert(test_tim2.DIER == 123U && test_gpiod.BSRR == 456U);
    test_tim2_clock = test_gpiod_clock = 1U;
}

/**
  * @brief  用实际电机接口和默认参数验证 200 CPS 闭环、减速及负载变化
  * @param  None
  * @retval None
  * @note   模型含机械惯性、整数编码器计数和测速滤波，不代表实物参数。
  */
static void test_200_cps_loop(void)
{
    float speed = 0.0f, fraction = 0.0f;
    int i;
    motor_stop_all();
    motor_control_tick();
    motor_set_target_cps(MOTOR_A, 200.0f);
    for (i = 0; i < 3000; ++i) {
        int delta;
        float load = i >= 1000 && i < 2000 ? 100.0f : 0.0f;
        speed += (g_motor_feedback[0].pwm * 10.0f - load - speed) * 0.064493f;
        fraction += speed * 0.01f;
        delta = (int)fraction;
        fraction -= delta;
        tim4.CNT = (uint16_t)(tim4.CNT + delta);
        control_period();
        if (i > 500 && (i < 1000 || (i > 1500 && i < 2000) || i > 2500))
            assert(fabsf(speed - 200.0f) < 15.0f);
    }
    /* 主动注入正向超速，PID 输出和积分必须下降，而非继续加速。 */
    {
        int16_t old_pwm = g_motor_feedback[0].pwm;
        float old_integral = g_motor_feedback[0].pid_integral;
        tim4.CNT = (uint16_t)(tim4.CNT + 20U); /* 2000 CPS */
        control_period();
        assert(g_motor_feedback[0].speed_cps > 200.0f);
        assert(g_motor_feedback[0].pwm < old_pwm);
        assert(g_motor_feedback[0].pid_integral < old_integral);
    }
    motor_set_target_cps(MOTOR_A, -200.0f);
    for (i = 0; i < 1000; ++i) {
        int delta;
        speed += (g_motor_feedback[0].pwm * 10.0f - speed) * 0.064493f;
        fraction += speed * 0.01f;
        delta = (int)fraction;
        fraction -= delta;
        tim4.CNT = (uint16_t)(tim4.CNT + delta);
        control_period();
        if (i > 500) assert(fabsf(speed + 200.0f) < 15.0f);
    }
    motor_stop_all();
}


/* A stationary motor must keep receiving PI updates instead of a timed stop.
 * Once counts arrive, feedback must reduce the output. Target zero stays stopped. */
static void test_fixed_period(void)
{
    int i;
    uint32_t ticks;
    float integral;
    int16_t pwm;
    motor_stop_all();
    for (i = 0; i < 100; ++i) control_period();
    motor_set_target_cps(MOTOR_A, 200.0f);
    ticks = g_motor_feedback[0].control_ticks;
    for (i = 0; i < 100; ++i) control_period();
    assert(g_motor_feedback[0].control_ticks == ticks + 100U);
    assert(g_motor_feedback[0].pwm > 200);
    integral = g_motor_feedback[0].pid_integral;
    motor_set_target_cps(MOTOR_A, 200.0f);
    assert(g_motor_feedback[0].pid_integral == integral);
    for (i = 0; i < 500; ++i) control_period();
    assert(g_motor_feedback[0].pwm > 0);
    assert(g_motor_feedback[0].pwm <= MOTOR_PWM_LIMIT);
    pwm = g_motor_feedback[0].pwm;
    for (i = 0; i < 20; ++i) {
        tim4.CNT = (uint16_t)(tim4.CNT + 20U);
        control_period();
    }
    assert(g_motor_feedback[0].pwm < pwm);
    motor_set_target_cps(MOTOR_A, 0.0f);
    for (i = 0; i < 100; ++i) control_period();
    assert(g_motor_feedback[0].pwm == 0);
    assert(g_motor_feedback[0].pid_integral == 0.0f);
    motor_set_target_cps(MOTOR_A, 200.0f);
    control_period();
    assert(g_motor_feedback[0].pwm > 0);
    motor_stop_all();
}

/* Without encoder counts, diagnostic PWM must stay fixed and A must stay braked. */
static void test_single_channel_open_loop(void)
{
    uint32_t i;
    motor_set_pwm(MOTOR_A, 0);
    motor_set_pwm(MOTOR_B, 200);
    for (i = 0; i < 6000U; ++i) {
        control_period();
        pwm_period(0);
        assert((test_gpiod.BSRR & (AIN1_Pin | AIN2_Pin)) == (AIN1_Pin | AIN2_Pin));
        assert((test_gpiod.BSRR & ((AIN1_Pin | AIN2_Pin) << 16U)) == 0U);
        assert(test_tim2.CCR2 == 16800U);
        assert(test_tim2.DIER == (TIM_DIER_UIE | TIM_DIER_CC2IE));
        pwm_edge(TIM_SR_CC2IF);
        assert(test_gpiod.BSRR == (BIN1_Pin | BIN2_Pin));
    }
    assert(g_motor_feedback[0].mode == MOTOR_OPEN_LOOP);
    assert(g_motor_feedback[1].mode == MOTOR_OPEN_LOOP);
    assert(g_motor_feedback[0].pwm == 0);
    assert(g_motor_feedback[1].pwm == 200);
    assert(g_motor_feedback[1].pid_integral == 0.0f);
    motor_stop_all();
}

/**
  * @brief  在电脑端执行电机控制与 GPIO PWM 模拟测试
  * @param  None
  * @retval 0：全部断言通过；断言失败时终止程序
  */
int main(void)
{
    motor_feedback_t a, b;
    test_pid();
    test_tim2.ARR = 83999;
    tim4.CNT = 65530;
    tim3.CNT = 5;
    assert(motor_init() == HAL_OK);
    assert(encoder_starts == 2 && timer_starts == 2);
    assert(motor_init() == HAL_OK); /* 重复调用不应破坏 HAL 通道状态或清零反馈。 */
    assert(encoder_starts == 2 && timer_starts == 2);
    assert_braked();

    motor_set_pwm(MOTOR_A, 32767);
    assert(g_motor_feedback[0].pwm == 1000);
    motor_set_pwm(MOTOR_B, -32768);
    assert(g_motor_feedback[1].pwm == -1000);
    pwm_period(0);
    assert(test_tim2.DIER == TIM_DIER_UIE); /* Full duty has no off edge. */
    assert(test_gpiod.BSRR == (AIN2_Pin | (AIN1_Pin << 16)
                           | BIN1_Pin | (BIN2_Pin << 16)));

    tim4.CNT = 4;       /* Forward wrap: +10 counts. */
    tim3.CNT = 65530;   /* Reverse wrap: -11 counts. */
    control_period();
    motor_get_feedback(MOTOR_A, &a);
    motor_get_feedback(MOTOR_B, &b);
    assert(a.delta_counts == 10 && b.delta_counts == -11);
    assert(fabsf(a.speed_cps - 350.0f) < 0.01f);
    assert(fabsf(b.speed_cps + 385.0f) < 0.01f);
    assert(a.mode == MOTOR_OPEN_LOOP && b.mode == MOTOR_OPEN_LOOP);
    assert(a.pwm == 1000 && b.pwm == -1000); /* PID must not overwrite open loop. */
    assert(motor_set_target_rpm(MOTOR_A, 60.0f) == HAL_ERROR);

    motor_set_target_cps(MOTOR_A, 1000.0f);
    motor_set_target_cps(MOTOR_B, -1000.0f);
    control_period();
    assert(g_motor_feedback[0].pwm > 0 && g_motor_feedback[1].pwm < 0);
    motor_set_target_cps(MOTOR_A, -1000.0f);
    control_period();
    assert(g_motor_feedback[0].pwm < 0);
    motor_set_target_cps(MOTOR_A, 1.0e9f);
    assert(g_motor_feedback[0].target_cps == MOTOR_MAX_TARGET_CPS);
    motor_set_target_cps(MOTOR_A, NAN);
    assert(g_motor_feedback[0].pwm == 0);
    assert(test_gpiod.BSRR == (AIN1_Pin | AIN2_Pin));
    motor_setspeed1(-128);
    assert(g_motor_feedback[0].pwm == -1000);
    motor_setspeed2(50);
    pwm_period(0);
    assert(test_tim2.CCR2 == 42000);

    test_primask = 1;
    motor_stop_all();
    assert(test_primask == 1); /* Nested calls preserve interrupt mask. */
    test_primask = 0;
    control_period();
    assert_braked();
    assert(g_motor_feedback[0].target_cps == 0.0f);
    motor_set_pwm((motor_id_t)99, 1000);
    motor_set_target_cps((motor_id_t)-1, 1000.0f);
    assert_braked();
    test_gpio_pwm();
    test_200_cps_loop();
    test_fixed_period();
    test_single_channel_open_loop();
    test_emergency_stop();
    puts("PASS: PID convergence/anti-windup, encoder wrap/sign, independent channels,");
    puts("      PWM scaling/clamps, loop switching, invalid input and stop persistence.");
    puts("      DRV8848 GPIO truth table, 999 fractional duties, compare edges and late IRQ.");
    puts("      Fixed 10ms control, repeat initialization, numeric guards and emergency braking.");
    puts("      200 CPS closed loop under load, overspeed response, stop/restart and no timed latching.");
    puts("      60s diagnostic A=0/B=20% without encoder counts: independent, constant PWM.");
    return 0;
}
