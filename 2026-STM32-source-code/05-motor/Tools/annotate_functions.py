"""为应用层函数补齐中文 Doxygen 注释；CubeMX 重新生成后可再次运行。

从任意目录执行：python 路径/Tools/annotate_functions.py
处理 Bsp、Core 应用文件与 Tests 模拟代码，不改 ST 的 Drivers、CMSIS 系统文件或函数实现。
"""

from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

# 每项依次是：功能、参数说明、返回值、可选的调用注意事项。
DOCS = {
    "pid_finite": ("检查 PID 输入和中间运算是否为有限值", ["value 待检查的浮点数"], "1：有效；0：NaN 或无穷大"),
    "motor_pwm_emergency_stop": ("异常时关闭 PWM 中断并直接制动，不依赖 HAL 或浮点运算", [], "None", "供故障处理调用；恢复正常控制必须复位系统。"),
    "MX_TIM5_Init": ("保留的 TIM5 生成配置，简化版电机控制不使用", [], "None", "当前 main 不调用；测速和 PI 使用 TIM6 固定 10ms 周期。"),
    "valid_id": ("检查电机编号是否有效", ["id 电机编号：MOTOR_A 或 MOTOR_B"], "1：有效；0：无效"),
    "finite_float": ("检查浮点数是否为有限值，排除 NaN 和无穷大", ["value 待检查的数值"], "1：有限值；0：无效值"),
    "limit_target": ("把目标速度限制在允许的 CPS 范围内", ["value 原始目标速度，单位编码计数/秒"], "限幅后的速度；无效数值按 0 处理"),
    "lock_control": ("暂时关闭中断，保护主循环与控制中断共享的数据", [], "进入前的 PRIMASK 中断屏蔽状态"),
    "unlock_control": ("恢复进入临界区之前的中断状态", ["primask lock_control() 返回的原始状态"], "None"),
    "write_pwm": ("对单路 PWM 指令限幅、修正方向并交给底层输出", ["id MOTOR_A 或 MOTOR_B", "duty 有符号占空比，单位千分比"], "None", "内部函数；调用方应避免与 TIM6 同时修改反馈数据。"),
    "motor_init": ("初始化两路电机控制，启动编码器、GPIO PWM 和 PID 节拍", [], "HAL_OK：成功；HAL_ERROR：启动失败并停车", "先完成 MX_GPIO_Init 和 MX_TIM2/3/4/6_Init；成功后重复调用不重新初始化。"),
    "motor_set_target_cps": ("设置单路目标速度，启用编码器 PID 闭环", ["id MOTOR_A 或 MOTOR_B", "counts_per_second 四倍频编码计数/秒；正负表示方向，0 表示制动停车"], "None", "初始化成功后调用，目标会一直保持；不是 RPM，也不是 PWM。"),
    "motor_set_target_rpm": ("将目标转速换算成 CPS 后启用速度闭环", ["id MOTOR_A 或 MOTOR_B", "rpm 输出轴目标转速，单位转/分，正负表示方向"], "HAL_OK：指令已受理；HAL_ERROR：编号、状态、转速或每圈计数无效", "先在 motor_config.h 填入实际 COUNTS_PER_REV；换算结果仍受 CPS 上限限制。"),
    "motor_set_pwm": ("设置单路固定占空比，切换为 PWM 开环控制", ["id MOTOR_A 或 MOTOR_B", "duty_per_mille -1000～1000；300 为 30%，负数反向，0 为制动"], "None", "编码器仍然测速，但 PID 不再自动调整该路输出。"),
    "motor_stop": ("立即制动单路电机，清除目标和 PID 积分", ["id MOTOR_A 或 MOTOR_B"], "None", "后续中断保持停车；重新发送非零速度或 PWM 指令才会再次驱动。"),
    "motor_stop_all": ("立即制动两路电机，并清除两路目标和积分", [], "None"),
    "motor_get_feedback": ("读取单路完整反馈快照，避免读取过程中被中断更新", ["id MOTOR_A 或 MOTOR_B", "feedback 输出参数：指向接收数据的 motor_feedback_t 变量"], "None", "无效编号或空指针时不写入；主循环读取反馈优先用本函数。"),
    "motor_set_pid": ("更新单路 PID 系数并清空旧积分", ["id MOTOR_A 或 MOTOR_B", "kp 非负比例系数", "ki 非负积分系数，时间单位秒", "kd 非负微分系数，可先设为 0"], "None", "初始化后按需调用，不能在主循环每次迭代都调用，否则积分无法累计。"),
    "motor_control_tick": ("TIM6 每 10ms 读取编码器，计算速度并更新两路 PI", [], "None", "固定 dt=0.01 秒；只由 TIM6 调用，主循环不要重复调用。"),
    "motor_setspeed1": ("旧版兼容接口：设置 A 路百分比 PWM，切换为开环", ["speed -100～100 的有符号百分比，50 表示 50%"], "None", "新代码优先使用 motor_set_pwm(MOTOR_A, 千分比)。"),
    "motor_setspeed2": ("旧版兼容接口：设置 B 路百分比 PWM，切换为开环", ["speed -100～100 的有符号百分比，50 表示 50%"], "None", "新代码优先使用 motor_set_pwm(MOTOR_B, 千分比)。"),
    "motor_pwm_set": ("保存 DRV8848 PWM 指令，在下一 PWM 周期应用", ["motor 电机下标：0 为 A 路，1 为 B 路", "duty -1000～1000 的有符号千分比，已包含方向修正"], "None", "停车和换向会先立即制动；普通更新最多等待约 1ms。"),
    "motor_pwm_init": ("初始化 DRV8848 GPIO PWM，启动 TIM2 并保持制动", [], "HAL_OK：TIM2 启动成功；其他 HAL 状态：启动失败", "由 motor_init() 调用；TIM2 必须已经配置为 1kHz。"),
    "motor_pwm_irq": ("处理 TIM2 周期和比较事件，生成两路 GPIO PWM 波形", [], "None", "由 TIM2_IRQHandler() 直接调用；不使用 HAL 重复处理同一批标志。"),
    "clamp": ("把浮点数限制在正负上限之间", ["value 输入数值", "limit 正的绝对值上限"], "范围为 -limit～limit 的数值"),
    "speed_pid_reset": ("清空 PID 积分并记录当前速度，避免旧状态影响下一次控制", ["pid 指向有效的 PID 状态结构体", "measurement 当前实测速度，单位 CPS"], "None"),
    "speed_pid_init": ("设置 PID 系数和输出上限，初始化内部状态", ["pid 指向有效的 PID 状态结构体", "kp 比例系数", "ki 积分系数，时间单位秒", "kd 测量速度的微分系数", "limit 输出绝对值上限，单位千分比"], "None"),
    "speed_pid_update": ("计算一次速度 PID，包含输出限幅和抗积分饱和", ["pid 指向有效的 PID 状态结构体", "target 目标速度，单位 CPS", "measurement 实测速度，单位 CPS", "dt 实际采样间隔，单位秒，正常约 0.01f"], "有符号 PWM 千分比；输入或运算结果无效时返回 0", "微分针对测量值，避免目标突变引起微分冲击；参数由电机层检查。"),
    "main": ("电机闭环示例入口：初始化外设、设置目标并读取反馈", [], "正常运行不返回"),
    "SystemClock_Config": ("配置 8MHz 外部晶振和 PLL，得到 168MHz 系统时钟", [], "None", "APB1 为 42MHz，APB1 定时器时钟为 84MHz。"),
    "HAL_TIM_PeriodElapsedCallback": ("HAL 定时器周期回调，将 TIM6 的 10ms 节拍交给电机控制", ["htim 触发周期事件的定时器句柄"], "None", "HAL_TIM_IRQHandler() 自动调用；TIM2 使用独立的 GPIO PWM 中断入口。"),
    "Error_Handler": ("初始化或 HAL 操作失败时制动电机并停留在错误现场", [], "None（进入死循环）"),
    "assert_failed": ("接收 HAL 参数断言失败的位置，供调试定位", ["file 出错文件名", "line 出错代码行号"], "None", "仅在启用 USE_FULL_ASSERT 时编译。"),
    "MX_GPIO_Init": ("初始化驱动输入引脚电平及推挽输出模式", [], "None", "PD8/9/10/11 初始为低电平（滑行），motor_init() 随后切换为制动。"),
    "MX_TIM2_Init": ("配置 1kHz TIM2，为两路 GPIO PWM 提供周期和比较计时", [], "None", "84MHz / (0+1) / (83999+1) = 1kHz；PD8～PD11 没有硬件 PWM 复用。"),
    "MX_TIM3_Init": ("配置 B 路编码器：PC6/PC7，TIM3 四倍频计数", [], "None", "16 位计数器，ARR=65535，输入滤波为 6，上拉输入。"),
    "MX_TIM4_Init": ("配置 A 路编码器：PD12/PD13，TIM4 四倍频计数", [], "None", "16 位计数器，ARR=65535，输入滤波为 6，上拉输入。"),
    "MX_TIM6_Init": ("配置 10ms TIM6 节拍，用于编码器测速和速度 PID", [], "None", "84MHz / (83+1) / (9999+1) = 100Hz。"),
    "HAL_TIM_Base_MspInit": ("使能 TIM2/5/6 时钟并设置 TIM2/TIM6 中断优先级", ["tim_baseHandle 待初始化的定时器句柄"], "None", "HAL 初始化自动调用；TIM2 优先级 4，高于 TIM6 的 5；TIM5 不开中断。"),
    "HAL_TIM_Encoder_MspInit": ("使能编码器定时器时钟并配置对应 GPIO 复用", ["tim_encoderHandle TIM3 或 TIM4 的定时器句柄"], "None", "HAL_TIM_Encoder_Init() 自动调用；AF2 为对应编码器通道复用。"),
    "HAL_TIM_Base_MspDeInit": ("关闭基础定时器时钟和对应中断", ["tim_baseHandle 待停用的定时器句柄"], "None", "HAL 反初始化自动调用，正常演示不需要手动调用。"),
    "HAL_TIM_Encoder_MspDeInit": ("关闭编码器时钟并释放对应的 GPIO 引脚", ["tim_encoderHandle 待停用的编码器定时器句柄"], "None", "HAL 反初始化自动调用，正常演示不需要手动调用。"),
    "HAL_MspInit": ("使能系统配置与电源时钟，完成 HAL 的底层公共初始化", [], "None", "HAL_Init() 自动调用。"),
    "NMI_Handler": ("不可屏蔽中断入口，关闭 PWM 并制动后停留现场", [], "None（进入死循环）"),
    "HardFault_Handler": ("硬故障异常入口，关闭 PWM 并制动后停留现场", [], "None（进入死循环）"),
    "MemManage_Handler": ("内存管理异常入口，关闭 PWM 并制动后停留现场", [], "None（进入死循环）"),
    "BusFault_Handler": ("总线访问异常入口，关闭 PWM 并制动后停留现场", [], "None（进入死循环）"),
    "UsageFault_Handler": ("非法指令或使用错误异常入口，关闭 PWM 并制动后停留现场", [], "None（进入死循环）"),
    "SVC_Handler": ("系统服务调用异常入口，本裸机演示无需处理", [], "None"),
    "DebugMon_Handler": ("调试监视异常入口，保留给调试器使用", [], "None"),
    "PendSV_Handler": ("可挂起系统服务异常入口，本裸机演示无需处理", [], "None"),
    "SysTick_Handler": ("每 1ms 更新 HAL 时间计数，为 HAL_Delay 等函数提供时基", [], "None"),
    "TIM2_IRQHandler": ("TIM2 中断入口，交给 GPIO PWM 驱动处理波形边沿", [], "None", "在 USER CODE 区提前返回，跳过 CubeMX 默认的 HAL 分发以免重复处理。"),
    "TIM6_DAC_IRQHandler": ("TIM6 中断入口，通过 HAL 调用 10ms 电机控制回调", [], "None", "调用顺序：HAL_TIM_IRQHandler -> HAL_TIM_PeriodElapsedCallback -> motor_control_tick。"),
}

TEST_DOCS = {
    "control_period": ("推进一个正常的 10ms 控制周期", [], "None"),
    "test_emergency_stop": ("验证异常停车关闭 PWM，并兼容外设时钟尚未开启的情况", [], "None"),
    "main": ("在电脑端执行电机控制与 GPIO PWM 模拟测试", [], "0：全部断言通过；断言失败时终止程序"),
    "HAL_TIM_Encoder_Start": ("模拟编码器启动，并检查定时器与通道参数", ["h 模拟 TIM3 或 TIM4 句柄", "ch 编码器通道集合"], "HAL_OK", "仅用于电脑端测试，不连接实际硬件。"),
    "HAL_TIM_Base_Start_IT": ("模拟基础定时器启动并记录中断使能状态", ["h 模拟 TIM2 或 TIM6 句柄"], "HAL_OK", "仅用于电脑端测试。"),
    "pwm_period": ("模拟一个 PWM 周期开始，并注入指定中断延迟", ["latency_ticks 进入中断时的计数器值"], "None"),
    "pwm_edge": ("模拟 PWM 比较事件，触发对应通道的关闭边沿", ["flags 模拟的比较事件标志"], "None"),
    "assert_braked": ("检查两路输入处于持续制动且不再产生关闭边沿", [], "None"),
    "test_gpio_pwm": ("遍历两路 PWM 占空比并验证比较边沿、换向和延迟处理", [], "None"),
    "test_pid": ("验证 PID 正反向收敛、输出限幅和积分抗饱和", [], "None"),
    "__get_PRIMASK": ("读取模拟的中断屏蔽状态", [], "模拟 PRIMASK 状态"),
    "__disable_irq": ("将模拟中断状态设为屏蔽", [], "None"),
    "__set_PRIMASK": ("恢复模拟中断屏蔽状态", ["key 需要恢复的模拟状态"], "None"),
}

FUNCTION = re.compile(
    r"^(?:static\s+)?(?:inline\s+)?(?:HAL_StatusTypeDef|uint8_t|uint32_t|float|void|int)\s+"
    r"(?P<name>\w+)\([^;{}\n]*\)(?=\s*\{|;)", re.MULTILINE
)
OLD_DOC = re.compile(r"/\*\*(?:(?!/\*).)*?\*/\s*$", re.DOTALL)

FILE_GUIDES = {
    "motor.c": "电机应用层：编码器测速、速度 PID、PWM 输出与开闭环控制接口。",
    "motor.h": "主程序使用的电机接口与反馈数据类型，参数单位在每个函数前说明。",
    "motor_config.h": "集中配置控制周期、方向修正、每圈计数及两路 PID 参数。",
    "motor_pwm.c": "DRV8848 底层 GPIO PWM：TIM2 周期和比较中断控制电机输入电平。",
    "motor_pwm.h": "GPIO PWM 内部接口；应用程序优先调用 motor.h 中的控制函数。",
    "pid.c": "速度 PID 算法：输入目标和实际速度，输出有符号 PWM 千分比。",
    "pid.h": "PID 状态与算法接口；两路电机各自保存独立的积分和历史速度。",
}

# 替换 CubeMX 中影响阅读的常见英文说明，保留 USER CODE 标记及版权信息。
COMMENT_TRANSLATIONS = {
    "Main program body": "DRV8848 双电机编码器速度闭环示例",
    "Interrupt Service Routines.": "系统异常与 TIM2/TIM6 中断入口",
    "Header for main.c file.": "主程序公共定义与电机 GPIO 引脚映射。",
    "This file contains the common defines of the application.": "驱动输入为 PD8～PD11，编码器配置见 tim.c。",
    "Includes ------------------------------------------------------------------": "头文件 --------------------------------------------------------------------",
    "Private includes ----------------------------------------------------------": "应用层头文件 ---------------------------------------------------------------",
    "Private typedef -----------------------------------------------------------": "本文件的数据类型 -----------------------------------------------------------",
    "Private define ------------------------------------------------------------": "本文件的常量配置 -----------------------------------------------------------",
    "Private macro -------------------------------------------------------------": "本文件的辅助宏 -------------------------------------------------------------",
    "Private variables ---------------------------------------------------------": "本文件变量 -----------------------------------------------------------------",
    "Private function prototypes -----------------------------------------------": "函数声明 -------------------------------------------------------------------",
    "Private user code ---------------------------------------------------------": "自定义函数 -----------------------------------------------------------------",
    "External variables --------------------------------------------------------": "其他文件定义的变量 ---------------------------------------------------------",
    "MCU Configuration--------------------------------------------------------": "系统初始化 ----------------------------------------------------------------",
    "Reset of all peripherals, Initializes the Flash interface and the Systick.": "初始化 HAL、Flash 接口和 1ms 系统节拍。",
    "Configure the system clock": "配置系统时钟为 168MHz",
    "Infinite loop": "主循环：控制中断独立运行，可在这里添加其他演示任务",
    "Configure the main internal regulator output voltage": "配置内部稳压器，支持 168MHz 运行",
    "Initializes the RCC Oscillators according to the specified parameters": "使用 8MHz 外部晶振和 PLL 生成系统时钟",
    "Initializes the CPU, AHB and APB buses clocks": "分配 CPU、AHB 与 APB 总线时钟",
    "GPIO Ports Clock Enable": "使能使用到的 GPIO 端口时钟",
    "Configure GPIO pin Output Level": "先设定输出电平，再切换到输出模式，避免初始化毛刺",
    "Configure GPIO pins :": "配置以下 GPIO 引脚：",
    "System interrupt init": "系统中断公共配置",
    "Peripheral clock disable": "关闭外设时钟",
    "TIM2 clock enable": "使能 TIM2 时钟（GPIO PWM 计时）",
    "TIM3 clock enable": "使能 TIM3 时钟（B 路编码器）",
    "TIM4 clock enable": "使能 TIM4 时钟（A 路编码器）",
    "TIM6 clock enable": "使能 TIM6 时钟（10ms PID 节拍）",
    "TIM2 interrupt Init": "配置 TIM2 中断，优先级 4，可抢占 TIM6",
    "TIM6 interrupt Init": "配置 TIM6 中断，优先级 5",
    "TIM2 interrupt Deinit": "关闭 TIM2 中断",
    "TIM6 interrupt Deinit": "关闭 TIM6 中断",
    "TIM3 GPIO Configuration": "B 路编码器引脚复用（AF2）",
    "TIM4 GPIO Configuration": "A 路编码器引脚复用（AF2）",
}


def make_doc(name, docs):
    """按统一格式生成一个函数的中文说明。"""
    brief, parameters, retval, *notes = docs[name]
    lines = ["/**", f"  * @brief  {brief}"]
    lines += [f"  * @param  {param}" for param in parameters] if parameters else ["  * @param  None"]
    lines += [f"  * @retval {retval}"]
    lines += [f"  * @note   {note}" for note in notes]
    return "\n".join(lines + ["  */", ""])


def annotate_file(path):
    """替换函数前的旧说明，或为缺少说明的函数插入注释；保留函数实现。"""
    source = path.read_text(encoding="utf-8-sig")
    for old_text, new_text in COMMENT_TRANSLATIONS.items():
        source = source.replace(old_text, new_text)
    guide = FILE_GUIDES.get(path.name)
    if guide:
        header = f"/**\n  * @file   {path.name}\n  * @brief  {guide}\n  */\n"
        old_header = re.match(r"/\*\*.*?\*/\n?", source, re.DOTALL)
        if old_header and "@file" in old_header[0]:
            source = header + source[old_header.end():]
        else:
            source = header + source
    matches = list(FUNCTION.finditer(source))
    docs = TEST_DOCS if "Tests" in path.relative_to(ROOT).parts else DOCS
    for match in reversed(matches):
        name = match["name"]
        if name not in docs:
            raise ValueError(f"未配置函数说明：{path.name}:{name}")
        prefix = source[:match.start()]
        old = OLD_DOC.search(prefix)
        start = old.start() if old else match.start()
        source = source[:start] + make_doc(name, docs) + source[match.start():]
    # BOM 便于 Windows/Keil 识别中文 UTF-8；编译器可正常处理。
    path.write_text(source, encoding="utf-8-sig", newline="\n")
    return len(matches)


def main():
    """统一更新应用源码和头文件注释，输出覆盖数量。"""
    paths = sorted((ROOT / "Bsp").glob("*.[ch]"))
    paths += sorted(p for p in (ROOT / "Core" / "Src").glob("*.c") if p.name != "system_stm32f4xx.c")
    paths += sorted(p for p in (ROOT / "Core" / "Inc").glob("*.h") if p.name != "stm32f4xx_hal_conf.h")
    paths += sorted((ROOT / "Tests").rglob("*.[ch]"))
    total = sum(annotate_file(path) for path in paths)
    print(f"Updated {len(paths)} application/test files, {total} function definitions/declarations.")


if __name__ == "__main__":
    main()
