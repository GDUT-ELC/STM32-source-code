/**********************************************************
 * 文件名: app_flash.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-11-01
 * 
 * 功能说明:
 *   外部 Flash 的应用层封装。把底层驱动 gd25q80.c/h 的常用功能封装成
 *   语义更直白的接口（init / save / load / erase / read_id / self_test），
 *   供应用直接调用，无需了解 SPI、页、扇区等底层细节。
 *
 *   本层除了改名，还多做一件事：每个函数执行完都把“做了什么、结果如何”
 *   记进 g_flash_info，这样在 Keil 里不用加打印也能观察运行状态。
 *
 * 接口备注:
 *   1. 本层只调用 gd25q80.h 的公开接口，不修改底层驱动
 *   2. 本层不依赖显示屏，显示结果由调用方（main）自行处理
 *   3. 需要块擦除、掉电、状态寄存器等底层能力时，直接使用 gd25q80.h
 **********************************************************/

#include "app_flash.h"

/* 自检使用的地址、数据和长度。
 * 地址选 0x000000，对应 README 地址规划里的“程序示例 / 临时数据”区；
 * 用户数据区从 0x001000 开始，这样自检就不会动到用户数据。
 * 注意：自检会改写这 8 个字节，不要把重要数据放在这里。 */
#define APP_TEST_ADDR   0x000000UL
#define APP_TEST_LEN    8U

static const uint8_t app_test_data[APP_TEST_LEN] =
{
    0xA5, 0x5A, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC
};

/* 全局运行状态变量。加 volatile 是为了调试时 Watch 窗口能实时看到变化，
 * 防止编译器把它优化掉 */
volatile app_flash_info_t g_flash_info = {0, GD25Q_OK, APP_STEP_NONE, 0, 0};

/**
  * @brief： 内部小工具：把本次操作的状态记进 g_flash_info
  * @param： step   - 执行到的步骤
  * @param： addr   - 本次操作的地址，没有就填 0
  * @param： len    - 本次操作的字节数，没有就填 0
  * @param： result - 本次操作的结果
  * @retval：None
  */
static void app_flash_record(app_flash_step_t step,uint32_t addr,uint32_t len,GD25Q_Status result)
{
    g_flash_info.step   = step;
    g_flash_info.addr   = addr;
    g_flash_info.len    = len;
    g_flash_info.result = result;
}
/**
  * @brief： 初始化外部 Flash 并确认芯片型号
  * @param： None
  * @retval：GD25Q_OK 成功；其他值表示通信、ID 或状态异常
  */
GD25Q_Status app_flash_init(void)
{
    GD25Q_Status result;
    uint32_t id = 0;
    result = GD25Q_Init();
    app_flash_record(APP_STEP_INIT, 0, 0, result);

    /* 初始化成功后顺手读一次 ID，存进 g_flash_info.id，
       这样调试时不用再单独调用 read_id 就能看到芯片型号。
       这一步只是为了观察，读失败也不算初始化失败，所以不改变返回值 */
    if (result == GD25Q_OK)
    {
        if (GD25Q_ReadJedecID(&id) == GD25Q_OK)
        {
            g_flash_info.id = id;
        }
    }
    return result;
}

/**
  * @brief： 读取芯片 ID
  * @param： id - 输出，存放读到的 ID，正常为 0xC84014
  * @retval：GD25Q_OK 成功；其他值表示参数或通信错误
  */
GD25Q_Status app_flash_read_id(uint32_t *id)
{
    GD25Q_Status result;
    result = GD25Q_ReadJedecID(id);
    /* 读成功了就顺手记下 ID，调试时直接从 g_flash_info.id 看 */
    if (result == GD25Q_OK)
    {
        g_flash_info.id = *id;
    }
    app_flash_record(APP_STEP_READ_ID, 0, 0, result);

    return result;
}

/**
  * @brief： 保存数据（自动擦除，不破坏同扇区其他数据）
  * @param： addr - 写入地址
  * @param： data - 要保存的数据
  * @param： len  - 字节数
  * @retval：GD25Q_OK 成功；其他值表示错误
  */
GD25Q_Status app_flash_save(uint32_t addr, const uint8_t *data, uint32_t len)
{
    GD25Q_Status result;

    result = GD25Q_Write(addr, data, len);
    app_flash_record(APP_STEP_SAVE, addr, len, result);

    return result;
}

/**
  * @brief： 读取数据
  * @param： addr - 起始地址
  * @param： data - 接收缓冲区
  * @param： len  - 字节数
  * @retval：GD25Q_OK 成功；其他值表示错误
  */
GD25Q_Status app_flash_load(uint32_t addr, uint8_t *data, uint32_t len)
{
    GD25Q_Status result;

    result = GD25Q_ReadData(addr, data, len);
    app_flash_record(APP_STEP_LOAD, addr, len, result);

    return result;
}

/**
  * @brief： 擦除地址所在的 4KB 扇区
  * @param： addr - 扇区内任意地址
  * @retval：GD25Q_OK 成功；其他值表示错误
  */
GD25Q_Status app_flash_erase(uint32_t addr)
{
    GD25Q_Status result;

    result = GD25Q_SectorErase(addr);
    app_flash_record(APP_STEP_ERASE, addr, 0, result);

    return result;
}

/**
  * @brief： 上电自检：往测试地址写一段数据，再读回逐个比对
  * @param： None
  * @retval：GD25Q_OK 读写一致；其他值表示写入、读取或比对失败
  */
GD25Q_Status app_flash_self_test(void)
{
    uint8_t      buf[APP_TEST_LEN];
    uint32_t     i;
    GD25Q_Status result;

    /* 第一步：把测试数据写进去 */
    result = GD25Q_Write(APP_TEST_ADDR, app_test_data, APP_TEST_LEN);
    app_flash_record(APP_STEP_SELF_TEST, APP_TEST_ADDR, APP_TEST_LEN, result);
    if (result != GD25Q_OK)
    {
        return result;
    }

    /* 第二步：原样读回来 */
    result = GD25Q_ReadData(APP_TEST_ADDR, buf, APP_TEST_LEN);
    app_flash_record(APP_STEP_SELF_TEST, APP_TEST_ADDR, APP_TEST_LEN, result);
    if (result != GD25Q_OK)
    {
        return result;
    }

    /* 第三步：一个字节一个字节比对，有任何一个不一样就认为失败 */
    for (i = 0; i < APP_TEST_LEN; i++)
    {
        if (buf[i] != app_test_data[i])
        {
            app_flash_record(APP_STEP_SELF_TEST, APP_TEST_ADDR,
                             APP_TEST_LEN, GD25Q_ERR_VERIFY);
            return GD25Q_ERR_VERIFY;
        }
    }

    app_flash_record(APP_STEP_DONE, APP_TEST_ADDR, APP_TEST_LEN, GD25Q_OK);
    return GD25Q_OK;
}

/* 说明：返回值想显示成文字，用底层自带的 GD25Q_StatusString()，
 * 它会返回 OK、RANGE、NEED ERASE 这类英文短词，可直接送 OLED 显示。
 * 每个返回值的中文含义见 app_flash.h 里的对照表。
 *
 * 这里没有做中文版字符串，原因是 Keil 的编译器会把 UTF-8 中文字符串
 * 解析出错（报 missing closing quote）。中文注释不受影响，可以放心写。
 */
