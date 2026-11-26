/**********************************************************
 * 文件名: app_flash.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-11-01
 *
 * 功能说明:
 *   外部 Flash 的应用层封装。底层驱动在 gd25q80.c/h（负责 SPI、页、扇区等
 *   底层细节），本文件把常用功能封装成一看就懂的接口，供应用直接调用。
 *
 * 这一层做了两件底层驱动不做的事:
 *   1. 名字更直白：save 就是保存、load 就是读取，不用去记 Write / ReadData
 *   2. 自动记录运行状态：每次调用都会更新 g_flash_info，调试时在 Keil 的
 *      Watch 窗口里直接看芯片 ID、最近结果、卡在哪一步，不用额外加打印
 *
 * 接口备注:
 *   1. 使用前先调用 app_flash_init()，返回 OK（值为 0）后才能读写
 *   2. 一般只用 init / save / load / erase 四个函数；read_id 和 self_test
 *      用于确认芯片和硬件是否正常
 *   3. 需要更底层的控制（块擦除、掉电、状态寄存器等）时，直接使用
 *      gd25q80.h 里的完整接口
 **********************************************************/

#ifndef __APP_FLASH_H
#define __APP_FLASH_H

#include "gd25q80.h"

/* ---------- 执行步骤 ----------
 * 用来标记“最近执行到哪一步”。调试时 Watch 里看到的是数字，
 * 对照下面的注释就知道程序停在哪里。
 */
typedef enum
{
    APP_STEP_NONE      = 0,   /* 0：还没调用过任何函数 */
    APP_STEP_INIT      = 1,   /* 1：正在初始化 */
    APP_STEP_READ_ID   = 2,   /* 2：正在读芯片 ID */
    APP_STEP_SAVE      = 3,   /* 3：正在保存数据 */
    APP_STEP_LOAD      = 4,   /* 4：正在读取数据 */
    APP_STEP_ERASE     = 5,   /* 5：正在擦除扇区 */
    APP_STEP_SELF_TEST = 6,   /* 6：正在自检 */
    APP_STEP_DONE      = 7    /* 7：最后一步执行完，且成功 */
} app_flash_step_t;

/* ---------- 运行状态 ----------
 * 每个应用层函数执行完都会更新一次这个结构体。
 * 用法：Keil 进入调试后，在 Watch 窗口添加变量 g_flash_info，
 *       就能实时看到 ID、最近结果、操作地址和长度。
 */
typedef struct
{
    uint32_t         id;       /* 芯片 ID，正常为 0xC84014 */
    GD25Q_Status     result;   /* 最近一次的返回值，0（OK）表示成功 */
    app_flash_step_t step;     /* 最近执行到的步骤，数字含义见上面 */
    uint32_t         addr;     /* 最近一次操作的地址 */
    uint32_t         len;      /* 最近一次操作的字节数 */
} app_flash_info_t;

/* 全局运行状态变量，由本层自动维护，一般只需要在 Watch 里观察 */
extern volatile app_flash_info_t g_flash_info;

/**
  * @brief： 初始化外部 Flash 并确认芯片型号
  * @param： None
  * @retval：GD25Q_OK 成功；其他值表示通信、ID 或状态异常
  * @note：  上电后第一个调用，只调一次；成功后会自动把芯片 ID 填进
  *          g_flash_info.id，调试时直接看就有效果
  */
GD25Q_Status app_flash_init(void);

/**
  * @brief： 读取芯片 ID
  * @param： id - 输出，存放读到的 ID，正常为 0xC84014
  * @retval：GD25Q_OK 成功；其他值表示参数或通信错误
  * @note：  读到的 ID 会同时存进 g_flash_info.id 方便查看
  */
GD25Q_Status app_flash_read_id(uint32_t *id);

/**
  * @brief： 保存数据（自动擦除，不破坏同扇区其他数据）
  * @param： addr - 写入地址（0x000000 ~ 0x0FFFFF）
  * @param： data - 要保存的数据
  * @param： len  - 字节数
  * @retval：GD25Q_OK 成功；其他值表示错误
  */
GD25Q_Status app_flash_save(uint32_t addr, const uint8_t *data, uint32_t len);

/**
  * @brief： 读取数据
  * @param： addr - 起始地址
  * @param： data - 接收缓冲区，长度至少 len 字节
  * @param： len  - 字节数
  * @retval：GD25Q_OK 成功；其他值表示错误
  */
GD25Q_Status app_flash_load(uint32_t addr, uint8_t *data, uint32_t len);

/**
  * @brief： 擦除地址所在的 4KB 扇区
  * @param： addr - 扇区内任意地址（注意是地址，不是扇区编号）
  * @retval：GD25Q_OK 成功；其他值表示错误
  * @note：  擦除后该扇区所有字节变成 0xFF
  */
GD25Q_Status app_flash_erase(uint32_t addr);

/**
  * @brief： 上电自检：往测试地址写一段数据，再读回逐个比对
  * @param： None
  * @retval：GD25Q_OK 读写一致；其他值表示写入、读取或比对失败
  * @note：  自检会改写测试地址（默认 0x000000，属于示例/临时数据区）的
  *          8 个字节，不会动到 0x001000 以后的用户数据区
  */
GD25Q_Status app_flash_self_test(void);

/* ---------- 返回值中文对照表 ----------
 * 所有函数都返回 GD25Q_Status，0 就是成功，非 0 按下表查原因。
 * 想把它显示出来用底层自带的英文版 GD25Q_StatusString()。
 *
 *   返回值                  含义              常见原因
 *   GD25Q_OK                成功              ——
 *   GD25Q_ERR_PARAM         参数错            缓冲区指针为空、长度为 0
 *   GD25Q_ERR_RANGE         地址越界          地址或长度超出 1MB 范围
 *   GD25Q_ERR_SPI           通信失败          接线松了、MISO 没接、SPI 没初始化
 *   GD25Q_ERR_TIMEOUT       等待超时          芯片一直忙，可能供电不稳
 *   GD25Q_ERR_ID            型号不对          不是 GD25Q80C，或芯片没焊好
 *   GD25Q_ERR_NOT_INIT      没初始化          忘了先调 app_flash_init()
 *   GD25Q_ERR_PROTECTED     有写保护          状态寄存器保护位置起，驱动不自动解锁
 *   GD25Q_ERR_WRITE_ENABLE  写使能失败        WEL 位没置起来
 *   GD25Q_ERR_NEED_ERASE    需先擦除          想把 0 改成 1，必须先擦除
 *   GD25Q_ERR_VERIFY        校验失败          读回来和写进去的不一样，查供电和信号
 *   GD25Q_ERR_POWER_DOWN    已掉电            芯片在深度掉电，先 WakeUp
 *   GD25Q_ERR_SUSPENDED     已挂起            擦写被挂起，等它结束
 *   GD25Q_ERR_MODE          模式不对          芯片开了四线模式，与本板接法冲突
 */

#endif
