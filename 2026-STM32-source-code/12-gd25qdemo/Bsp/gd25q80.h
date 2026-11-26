/**********************************************************
 * 文件名: gd25q80.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-11-01
 *
 * 功能说明:
 *   GD25Q80C（1MB SPI Flash）驱动接口。新手只需掌握下面三个函数：
 *     GD25Q_Init()        上电初始化，只调一次
 *     GD25Q_Write()       保存数据，会自动擦除
 *     GD25Q_ReadData()    读取数据
 *   其余函数供进阶使用：擦除、低功耗、状态查询、单页编程等。
 *
 * 接口备注:
 *   1. 先用 GD25Q_Init()，返回 OK（值为 0）后才能读写
 *   2. 地址是芯片内部字节地址（0x000000 ~ 0x0FFFFF），不是 STM32 内存指针
 *   3. 阻塞式、不可重入，不要在中断或多任务里同时调用
 **********************************************************/

#ifndef __GD25Q80_H
#define __GD25Q80_H

#include <stdint.h>

/* ---- 容量参数 ---- */
#define GD25Q_FLASH_SIZE    0x00100000UL   /* 总容量 1MB，地址范围 0x000000 ~ 0x0FFFFF */
#define GD25Q_PAGE_SIZE     256UL          /* 一页 256 字节，单次写入上限，不能跨页 */
#define GD25Q_SECTOR_SIZE   4096UL         /* 一扇区 4KB，最小擦除单位 */
#define GD25Q_BLOCK32_SIZE  32768UL        /* 32KB 块 */
#define GD25Q_BLOCK64_SIZE  65536UL        /* 64KB 块 */
#define GD25Q_JEDEC_ID      0xC84014UL     /* 本板芯片 ID，初始化时用来核对型号 */

/* ---- 统一返回值：0 表示成功，非 0 含义见注释 ---- */
typedef enum
{
    GD25Q_OK = 0,                    /* 成功 */
    GD25Q_ERR_PARAM,                 /* 参数错：空指针、长度 0、单页写越界 */
    GD25Q_ERR_RANGE,                 /* 地址或长度超出 1MB */
    GD25Q_ERR_SPI,                   /* SPI 通信出错，查接线和 MISO */
    GD25Q_ERR_TIMEOUT,               /* 等待超时，芯片一直忙 */
    GD25Q_ERR_ID,                    /* 芯片 ID 不对，型号不符或没接好 */
    GD25Q_ERR_NOT_INIT,              /* 没初始化就调用了读写函数 */
    GD25Q_ERR_PROTECTED,             /* 状态寄存器有保护位，驱动不自动解锁 */
    GD25Q_ERR_WRITE_ENABLE,          /* 写使能失败，WEL 位没置起来 */
    GD25Q_ERR_NEED_ERASE,            /* 想把 0 改成 1，必须先擦除 */
    GD25Q_ERR_VERIFY,                /* 读回数据对不上，查供电和信号质量 */
    GD25Q_ERR_POWER_DOWN,            /* 芯片在深度掉电，先 WakeUp */
    GD25Q_ERR_SUSPENDED,             /* 芯片处于挂起状态 */
    GD25Q_ERR_MODE                   /* 芯片开了四线模式，与本板接法冲突 */
} GD25Q_Status;

/* ==================== 常用接口（新手掌握这三个即可） ==================== */

GD25Q_Status GD25Q_Init(void);
/* 初始化并确认芯片型号，上电调用一次 */

GD25Q_Status GD25Q_Write(uint32_t addr, const uint8_t *data, uint32_t length);
/* 保存数据：addr=写入地址  data=要存的数据  length=字节数；自动擦除，不破坏同扇区其他数据 */

GD25Q_Status GD25Q_ReadData(uint32_t addr, uint8_t *data, uint32_t length);
/* 读取数据：addr=起始地址  data=接收缓冲区  length=字节数；可跨页跨扇区 */

/* ==================== 进阶接口 ==================== */

/* --- 识别与状态 --- */
GD25Q_Status GD25Q_ReadID(uint8_t *mid, uint8_t *type, uint8_t *capacity);
/* 读三个 ID 字节，正常 0xC8 / 0x40 / 0x14 */

GD25Q_Status GD25Q_ReadJedecID(uint32_t *id);
/* 读合并后的 32 位 ID，GD25Q80C 应为 0xC84014 */

GD25Q_Status GD25Q_ReadStatus(uint8_t *sr1, uint8_t *sr2);
/* 读 SR1、SR2：SR1 的 bit0=WIP、bit1=WEL、bit6~2=BP；SR2 的 bit1=QE、bit6=CMP、bit7=SUS */

GD25Q_Status GD25Q_WaitReady(uint32_t timeout_ms);
/* 等芯片空闲，timeout_ms=最多等多少毫秒，0 表示只查一次 */

/* --- 写数据的底层接口 --- */
GD25Q_Status GD25Q_WriteNoErase(uint32_t addr, const uint8_t *data, uint32_t length);
/* 只写不擦，适合已擦除区域；遇 0 改 1 返回 NEED ERASE */

GD25Q_Status GD25Q_PageWrite(uint32_t addr, const uint8_t *data, uint16_t length);
/* 单页直接写：1~256 字节，且不能越过本页页尾 */

/* --- 擦除：参数是地址，不是编号 --- */
GD25Q_Status GD25Q_SectorErase(uint32_t addr);
/* 擦除地址所在的 4KB 扇区 */

GD25Q_Status GD25Q_Block32Erase(uint32_t addr);
/* 擦除地址所在的 32KB 块 */

GD25Q_Status GD25Q_Block64Erase(uint32_t addr);
/* 擦除地址所在的 64KB 块 */

GD25Q_Status GD25Q_ChipErase(void);
/* 擦除整片 1MB，全部数据丢失，耗时可达数秒 */

/* --- 低功耗与复位 --- */
GD25Q_Status GD25Q_PowerDown(void);
/* 进入深度掉电，之后访问需先 WakeUp */

GD25Q_Status GD25Q_WakeUp(void);
/* 退出深度掉电；唤醒不等同于初始化，首次使用仍需 Init */

GD25Q_Status GD25Q_SoftReset(void);
/* 软件复位，复位后需重新 Init */

/* --- 调试辅助 --- */
const char *GD25Q_StatusString(GD25Q_Status status);
/* 返回值转英文短词（"OK"/"VERIFY" 等），可直接显示 */

#endif
