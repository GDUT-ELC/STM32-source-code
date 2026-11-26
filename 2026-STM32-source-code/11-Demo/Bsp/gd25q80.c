/**********************************************************
 * 文件名: gd25q80.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-11-01
 *
 * 功能说明:
 *   GD25Q80C（8Mbit / 1MB SPI Flash）驱动实现：命令收发、连续读、页编程、
 *   扇区保留式写入、擦除、掉电唤醒与软件复位。接口说明见 gd25q80.h。
 *
 * 接口备注:
 *   1. 三条硬件约束：编程只能将 1 改为 0；擦除最小单位 4KB；单页不超过 256 字节
 *   2. 阻塞式、不可重入，禁止在中断中调用
 *   3. 内部函数以 gd25q_ 开头，仅供本文件使用
 **********************************************************/

#include "gd25q80.h"
#include "flash_config.h"
#include <string.h>

/* ---------- 命令码 ---------- */
#define CMD_WRITE_ENABLE    0x06U
#define CMD_READ_SR1        0x05U
#define CMD_READ_SR2        0x35U
#define CMD_READ_DATA       0x03U
#define CMD_PAGE_PROGRAM    0x02U
#define CMD_SECTOR_ERASE    0x20U
#define CMD_BLOCK32_ERASE   0x52U
#define CMD_BLOCK64_ERASE   0xD8U
#define CMD_CHIP_ERASE      0xC7U
#define CMD_READ_ID         0x9FU
#define CMD_POWER_DOWN      0xB9U
#define CMD_RELEASE_POWER   0xABU
#define CMD_RESET_ENABLE    0x66U
#define CMD_RESET           0x99U

/* ---------- 状态寄存器位 ---------- */
#define SR1_BUSY            0x01U   /* bit0 WIP：1 表示擦写进行中 */
#define SR1_WEL             0x02U   /* bit1 WEL：1 表示写使能生效 */
#define SR1_PROTECT_MASK    0x7CU   /* bit6~2 BP：块保护 */
#define SR2_CMP             0x40U   /* bit6 CMP：互补保护 */
#define SR2_SUSPEND         0x80U   /* bit7 SUS：擦写挂起 */

/* ---------- 内部状态与缓冲 ---------- */
static uint8_t gd25q_is_init = 0U;                    /* 0 未初始化，1 已初始化 */
static uint8_t gd25q_is_sleep = 0U;                   /* 0 正常，1 深度掉电 */
static uint8_t gd25q_sector_buf[GD25Q_SECTOR_SIZE];   /* 扇区缓冲，覆盖写入时保留其他数据 */
static uint8_t gd25q_page_buf[GD25Q_PAGE_SIZE];       /* 页缓冲，单页编程与校验用 */

/* ==================== 内部辅助函数 ==================== */

/* HAL 状态转换为驱动返回值 */
static GD25Q_Status gd25q_translate(HAL_StatusTypeDef hal_result)
{
    if (hal_result == HAL_OK) return GD25Q_OK;
    return (hal_result == HAL_TIMEOUT) ? GD25Q_ERR_TIMEOUT : GD25Q_ERR_SPI;
}

/* SPI 发送，片选由调用者控制 */
static GD25Q_Status gd25q_send(const uint8_t *data, uint16_t length)
{
    return gd25q_translate(HAL_SPI_Transmit(&FLASH_SPI_HANDLE, (uint8_t *)data,length, FLASH_SPI_TIMEOUT_MS));
}

/* SPI 接收 */
static GD25Q_Status gd25q_recv(uint8_t *data, uint16_t length)
{
    return gd25q_translate(HAL_SPI_Receive(&FLASH_SPI_HANDLE, data,
                                           length, FLASH_SPI_TIMEOUT_MS));
}

/* 发送单字节命令，内部完成片选 */
static GD25Q_Status gd25q_send_cmd(uint8_t cmd)
{
    GD25Q_Status result;

    FLASH_CS_L();
    result = gd25q_send(&cmd, 1U);      /* 需传入变量地址，不能对常量取地址 */
    FLASH_CS_H();
    return result;
}

/* 读状态寄存器：0x05 读 SR1，0x35 读 SR2 */
static GD25Q_Status gd25q_read_reg(uint8_t cmd, uint8_t *value)
{
    GD25Q_Status result;

    FLASH_CS_L();
    result = gd25q_send(&cmd, 1U);
    if (result == GD25Q_OK) result = gd25q_recv(value, 1U);
    FLASH_CS_H();
    return result;
}

/* 组装“命令 + 24 位地址”，header 至少 4 字节 */
static void gd25q_make_header(uint8_t *header, uint8_t cmd, uint32_t addr)
{
    header[0] = cmd;
    header[1] = (uint8_t)(addr >> 16);
    header[2] = (uint8_t)(addr >> 8);
    header[3] = (uint8_t)addr;
}

/* 校验指针、长度与地址范围 */
static GD25Q_Status gd25q_check_range(uint32_t addr, const void *data, uint32_t length)
{
    if ((data == NULL) || (length == 0U)) return GD25Q_ERR_PARAM;
    /* 用减法判断越界，避免 addr + length 溢出 */
    if ((addr >= GD25Q_FLASH_SIZE) || (length > (GD25Q_FLASH_SIZE - addr)))
    {
        return GD25Q_ERR_RANGE;
    }
    return GD25Q_OK;
}

/* 执行读命令，超长数据分段接收；调用者需已检查初始化和范围 */
static GD25Q_Status gd25q_read_raw(uint32_t addr, uint8_t *data, uint32_t length)
{
    uint8_t header[4];
    uint16_t count;
    GD25Q_Status result;

    gd25q_make_header(header, CMD_READ_DATA, addr);

    FLASH_CS_L();
    result = gd25q_send(header, 4U);
    /* HAL 单次传输长度为 16 位，超过 256 字节分批接收 */
    while ((result == GD25Q_OK) && (length > 0U))
    {
        count = (length > GD25Q_PAGE_SIZE) ? (uint16_t)GD25Q_PAGE_SIZE : (uint16_t)length;
        result = gd25q_recv(data, count);
        data += count;
        length -= count;
    }
    FLASH_CS_H();
    return result;
}

/* 检查初始化状态并等待芯片空闲 */
static GD25Q_Status gd25q_ready(void)
{
    if (gd25q_is_init == 0U) return GD25Q_ERR_NOT_INIT;
    return GD25Q_WaitReady(FLASH_CHIP_TIMEOUT_MS);
}

/* 写使能：检查保护位，发送 WREN，确认 WEL 置位；存在保护时直接拒绝 */
static GD25Q_Status gd25q_write_enable(void)
{
    uint8_t sr1, sr2;
    GD25Q_Status result;

    result = GD25Q_ReadStatus(&sr1, &sr2);
    if (result != GD25Q_OK) return result;

    if (((sr1 & SR1_PROTECT_MASK) != 0U) || ((sr2 & SR2_CMP) != 0U))
    {
        return GD25Q_ERR_PROTECTED;
    }
    /* QE enables quad data instructions; it does not disable the standard
     * 1-1-1 commands used here. Do not change QE or other status bits. */

    result = gd25q_send_cmd(CMD_WRITE_ENABLE);
    if (result != GD25Q_OK) return result;

    result = gd25q_read_reg(CMD_READ_SR1, &sr1);
    if (result != GD25Q_OK) return result;
    return ((sr1 & SR1_WEL) != 0U) ? GD25Q_OK : GD25Q_ERR_WRITE_ENABLE;
}

/* 擦除指定区域并读回校验全 0xFF；addr 按 size 向下对齐 */
static GD25Q_Status gd25q_erase_region(uint32_t addr, uint32_t size,
                                       uint8_t cmd, uint32_t timeout_ms)
{
    uint8_t header[4];
    uint32_t offset, i;
    GD25Q_Status result;

    if (addr >= GD25Q_FLASH_SIZE) return GD25Q_ERR_RANGE;
    addr = addr - (addr % size);

    result = gd25q_ready();
    if (result != GD25Q_OK) return result;
    result = gd25q_write_enable();
    if (result != GD25Q_OK) return result;

    /* 整片擦除只有命令字节，无地址 */
    gd25q_make_header(header, cmd, addr);
    FLASH_CS_L();
    result = gd25q_send(header, (cmd == CMD_CHIP_ERASE) ? 1U : 4U);
    FLASH_CS_H();
    if (result != GD25Q_OK) return result;

    result = GD25Q_WaitReady(timeout_ms);
    if (result != GD25Q_OK) return result;

    /* WIP 清零不足以证明擦除生效，需读回确认 */
    for (offset = 0U; offset < size; offset += GD25Q_PAGE_SIZE)
    {
        result = gd25q_read_raw(addr + offset, gd25q_page_buf, GD25Q_PAGE_SIZE);
        if (result != GD25Q_OK) return result;
        for (i = 0U; i < GD25Q_PAGE_SIZE; i++)
        {
            if (gd25q_page_buf[i] != 0xFFU) return GD25Q_ERR_VERIFY;
        }
    }
    return GD25Q_OK;
}

/* ==================== 常用接口 ==================== */

/* 初始化并确认芯片型号；先完成 HAL/GPIO/SPI 初始化，上电调用一次 */
GD25Q_Status GD25Q_Init(void)
{
    uint32_t id;
    uint8_t sr1, sr2;
    GD25Q_Status result;

    gd25q_is_init = 0U;
    FLASH_CS_H();
    HAL_Delay(10U);                     /* 等待电源稳定 */

    result = GD25Q_WakeUp();            /* 芯片可能处于掉电状态 */
    if (result != GD25Q_OK) return result;

    result = GD25Q_ReadJedecID(&id);
    if (result != GD25Q_OK) return result;
    if (id != GD25Q_JEDEC_ID) return GD25Q_ERR_ID;

    result = GD25Q_ReadStatus(&sr1, &sr2);
    if (result != GD25Q_OK) return result;
    /* QE=1 still accepts 0x03/0x02/0x20 over ordinary SPI. QE is not a
     * QPI-mode indicator, so it must not prevent initialization. */

    gd25q_is_init = 1U;
    return GD25Q_OK;
}

/* 保存数据：必要时擦除并保留同扇区其他数据，最常用 */
GD25Q_Status GD25Q_Write(uint32_t addr, const uint8_t *data, uint32_t length)
{
    uint32_t sector_base, offset_in_sector, count, i;
    uint8_t need_erase, changed;
    GD25Q_Status result;

    result = gd25q_check_range(addr, data, length);
    if (result != GD25Q_OK) return result;
    result = gd25q_ready();
    if (result != GD25Q_OK) return result;

    /* 按扇区逐段处理 */
    while (length > 0U)
    {
        sector_base = addr - (addr % GD25Q_SECTOR_SIZE);
        offset_in_sector = addr - sector_base;
        count = GD25Q_SECTOR_SIZE - offset_in_sector;
        if (count > length) count = length;

        /* 读入整个扇区，擦除后据此恢复其他数据 */
        result = gd25q_read_raw(sector_base, gd25q_sector_buf, GD25Q_SECTOR_SIZE);
        if (result != GD25Q_OK) return result;

        need_erase = 0U;
        changed = 0U;
        for (i = 0U; i < count; i++)
        {
            if (gd25q_sector_buf[offset_in_sector + i] != data[i]) changed = 1U;
            if ((gd25q_sector_buf[offset_in_sector + i] & data[i]) != data[i]) need_erase = 1U;
        }

        if (changed != 0U)
        {
            if (need_erase != 0U)
            {
                /* 新数据合并进缓冲，擦除后整扇区回写 */
                memcpy(&gd25q_sector_buf[offset_in_sector], data, count);
                result = GD25Q_SectorErase(sector_base);
                if (result != GD25Q_OK) return result;
                result = GD25Q_WriteNoErase(sector_base, gd25q_sector_buf,
                                            GD25Q_SECTOR_SIZE);
            }
            else
            {
                result = GD25Q_WriteNoErase(addr, data, count);
            }
            if (result != GD25Q_OK) return result;
        }

        addr += count;
        data += count;
        length -= count;
    }
    return GD25Q_OK;
}

/* 连续读：addr 起始地址，data 接收缓冲，length 字节数 */
GD25Q_Status GD25Q_ReadData(uint32_t addr, uint8_t *data, uint32_t length)
{
    GD25Q_Status result;

    result = gd25q_check_range(addr, data, length);
    if (result != GD25Q_OK) return result;
    result = gd25q_ready();
    if (result != GD25Q_OK) return result;

    return gd25q_read_raw(addr, data, length);
}

/* ==================== ID 与状态 ==================== */

/* 读三个 ID 字节：正常为 0xC8 / 0x40 / 0x14 */
GD25Q_Status GD25Q_ReadID(uint8_t *mid, uint8_t *type, uint8_t *capacity)
{
    uint8_t cmd = CMD_READ_ID;
    uint8_t id[3];
    GD25Q_Status result;

    if ((mid == NULL) || (type == NULL) || (capacity == NULL)) return GD25Q_ERR_PARAM;

    result = GD25Q_WaitReady(FLASH_CHIP_TIMEOUT_MS);
    if (result != GD25Q_OK) return result;

    FLASH_CS_L();
    result = gd25q_send(&cmd, 1U);
    if (result == GD25Q_OK) result = gd25q_recv(id, 3U);
    FLASH_CS_H();

    if (result == GD25Q_OK)
    {
        *mid = id[0];
        *type = id[1];
        *capacity = id[2];
    }
    return result;
}

/* 读合并后的 32 位 ID，GD25Q80C 应为 0xC84014 */
GD25Q_Status GD25Q_ReadJedecID(uint32_t *id)
{
    uint8_t mid, type, capacity;
    GD25Q_Status result;

    if (id == NULL) return GD25Q_ERR_PARAM;

    result = GD25Q_ReadID(&mid, &type, &capacity);
    if (result == GD25Q_OK)
    {
        *id = ((uint32_t)mid << 16) | ((uint32_t)type << 8) | (uint32_t)capacity;
    }
    return result;
}

/* 读 SR1、SR2：SR1 的 bit0=WIP、bit1=WEL、bit6~2=BP；SR2 的 bit1=QE、bit6=CMP、bit7=SUS */
GD25Q_Status GD25Q_ReadStatus(uint8_t *sr1, uint8_t *sr2)
{
    uint8_t low, high;
    GD25Q_Status result;

    if ((sr1 == NULL) || (sr2 == NULL)) return GD25Q_ERR_PARAM;
    if (gd25q_is_sleep != 0U) return GD25Q_ERR_POWER_DOWN;

    result = gd25q_read_reg(CMD_READ_SR1, &low);
    if (result == GD25Q_OK) result = gd25q_read_reg(CMD_READ_SR2, &high);
    if (result == GD25Q_OK)             /* 两次都成功才更新输出 */
    {
        *sr1 = low;
        *sr2 = high;
    }
    return result;
}

/* 等待 WIP 清零；timeout_ms 为等待上限（毫秒），0 表示只查一次 */
GD25Q_Status GD25Q_WaitReady(uint32_t timeout_ms)
{
    uint8_t sr1, sr2;
    uint32_t start_tick;
    GD25Q_Status result;

    if (timeout_ms == 0xFFFFFFFFUL) return GD25Q_ERR_PARAM;
    if (gd25q_is_sleep != 0U) return GD25Q_ERR_POWER_DOWN;

    start_tick = HAL_GetTick();
    for (;;)
    {
        result = gd25q_read_reg(CMD_READ_SR1, &sr1);
        if (result != GD25Q_OK) return result;

        if ((sr1 & SR1_BUSY) == 0U)
        {
            result = gd25q_read_reg(CMD_READ_SR2, &sr2);
            if (result != GD25Q_OK) return result;
            if ((sr2 & SR2_SUSPEND) != 0U) return GD25Q_ERR_SUSPENDED;
            return GD25Q_OK;
        }

        /* 用时间差比较，HAL_GetTick 回绕时仍然正确 */
        if ((uint32_t)(HAL_GetTick() - start_tick) >= timeout_ms)
        {
            return GD25Q_ERR_TIMEOUT;
        }
        HAL_Delay(1U);
    }
}

/* ==================== 底层写入 ==================== */

/* 单页编程，不擦除，写后读回校验；长度 1~256 且不越页尾 */
GD25Q_Status GD25Q_PageWrite(uint32_t addr, const uint8_t *data, uint16_t length)
{
    uint8_t header[4];
    uint32_t i;
    GD25Q_Status result;

    result = gd25q_check_range(addr, data, length);
    if (result != GD25Q_OK) return result;

    /* 单页上限与页尾剩余空间双重限制 */
    if ((length > GD25Q_PAGE_SIZE) ||
        (length > (GD25Q_PAGE_SIZE - (addr % GD25Q_PAGE_SIZE))))
    {
        return GD25Q_ERR_PARAM;
    }

    result = gd25q_ready();
    if (result != GD25Q_OK) return result;
    result = gd25q_read_raw(addr, gd25q_page_buf, length);
    if (result != GD25Q_OK) return result;

    /* 原数据某位为 0 而新数据为 1 时无法直接编程：原数据 & 新数据 必须等于新数据 */
    for (i = 0U; i < length; i++)
    {
        if ((gd25q_page_buf[i] & data[i]) != data[i]) return GD25Q_ERR_NEED_ERASE;
    }
    if (memcmp(gd25q_page_buf, data, length) == 0) return GD25Q_OK;  /* 内容一致不必重写 */

    result = gd25q_write_enable();
    if (result != GD25Q_OK) return result;

    gd25q_make_header(header, CMD_PAGE_PROGRAM, addr);
    FLASH_CS_L();
    result = gd25q_send(header, 4U);
    if (result == GD25Q_OK) result = gd25q_send(data, length);
    FLASH_CS_H();
    if (result != GD25Q_OK) return result;

    result = GD25Q_WaitReady(FLASH_PAGE_TIMEOUT_MS);
    if (result != GD25Q_OK) return result;

    result = gd25q_read_raw(addr, gd25q_page_buf, length);
    if (result != GD25Q_OK) return result;
    return (memcmp(gd25q_page_buf, data, length) == 0) ? GD25Q_OK : GD25Q_ERR_VERIFY;
}

/* 连续写不擦除，自动分页；遇 0 改 1 返回 NEED ERASE */
GD25Q_Status GD25Q_WriteNoErase(uint32_t addr, const uint8_t *data, uint32_t length)
{
    uint32_t offset, count, i;
    GD25Q_Status result;

    result = gd25q_check_range(addr, data, length);
    if (result != GD25Q_OK) return result;
    result = gd25q_ready();
    if (result != GD25Q_OK) return result;

    /* 先整体预检，避免写到一半才发现后续页需要擦除 */
    for (offset = 0U; offset < length; offset += count)
    {
        count = length - offset;
        if (count > GD25Q_PAGE_SIZE) count = GD25Q_PAGE_SIZE;

        result = gd25q_read_raw(addr + offset, gd25q_page_buf, count);
        if (result != GD25Q_OK) return result;
        for (i = 0U; i < count; i++)
        {
            if ((gd25q_page_buf[i] & data[offset + i]) != data[offset + i])
            {
                return GD25Q_ERR_NEED_ERASE;
            }
        }
    }

    /* 按页写入，每次写到本页页尾为止 */
    while (length > 0U)
    {
        count = GD25Q_PAGE_SIZE - (addr % GD25Q_PAGE_SIZE);
        if (count > length) count = length;

        result = GD25Q_PageWrite(addr, data, (uint16_t)count);
        if (result != GD25Q_OK) return result;

        addr += count;
        data += count;
        length -= count;
    }
    return GD25Q_OK;
}

/* ==================== 擦除 ==================== */

/* 擦除地址所在的 4KB 扇区 */
GD25Q_Status GD25Q_SectorErase(uint32_t addr)
{
    return gd25q_erase_region(addr, GD25Q_SECTOR_SIZE, CMD_SECTOR_ERASE,
                              FLASH_SECTOR_TIMEOUT_MS);
}

/* 擦除地址所在的 32KB 块 */
GD25Q_Status GD25Q_Block32Erase(uint32_t addr)
{
    return gd25q_erase_region(addr, GD25Q_BLOCK32_SIZE, CMD_BLOCK32_ERASE,
                              FLASH_BLOCK_TIMEOUT_MS);
}

/* 擦除地址所在的 64KB 块 */
GD25Q_Status GD25Q_Block64Erase(uint32_t addr)
{
    return gd25q_erase_region(addr, GD25Q_BLOCK64_SIZE, CMD_BLOCK64_ERASE,
                              FLASH_BLOCK_TIMEOUT_MS);
}

/* 擦除整片 1MB */
GD25Q_Status GD25Q_ChipErase(void)
{
    return gd25q_erase_region(0U, GD25Q_FLASH_SIZE, CMD_CHIP_ERASE,
                              FLASH_CHIP_TIMEOUT_MS);
}

/* ==================== 低功耗与复位 ==================== */

/* 进入深度掉电：先确认芯片空闲，再发命令 */
GD25Q_Status GD25Q_PowerDown(void)
{
    GD25Q_Status result;

    result = gd25q_ready();
    if (result != GD25Q_OK) return result;

    result = gd25q_send_cmd(CMD_POWER_DOWN);
    if (result == GD25Q_OK)
    {
        HAL_Delay(1U);                  /* tDP 最大 20us */
        gd25q_is_sleep = 1U;
    }
    return result;
}

/* 退出深度掉电；唤醒不等同于初始化 */
GD25Q_Status GD25Q_WakeUp(void)
{
    GD25Q_Status result;

    result = gd25q_send_cmd(CMD_RELEASE_POWER);
    if (result == GD25Q_OK)
    {
        HAL_Delay(1U);                  /* tRES1 最大 20us */
        gd25q_is_sleep = 0U;
    }
    return result;
}

/* 软件复位；复位后需重新调用 GD25Q_Init() */
GD25Q_Status GD25Q_SoftReset(void)
{
    GD25Q_Status result;

    result = gd25q_ready();
    if (result != GD25Q_OK) return result;

    result = gd25q_send_cmd(CMD_RESET_ENABLE);
    if (result != GD25Q_OK) return result;

    gd25q_is_init = 0U;                 /* 复位命令即使传输失败也可能已生效 */
    result = gd25q_send_cmd(CMD_RESET);
    HAL_Delay(20U);
    return result;
}

/* ==================== 调试 ==================== */

/* 返回值转换为英文短字符串 */
const char *GD25Q_StatusString(GD25Q_Status status)
{
    switch (status)
    {
    case GD25Q_OK:                return "OK";
    case GD25Q_ERR_PARAM:         return "PARAM";
    case GD25Q_ERR_RANGE:         return "RANGE";
    case GD25Q_ERR_SPI:           return "SPI";
    case GD25Q_ERR_TIMEOUT:       return "TIMEOUT";
    case GD25Q_ERR_ID:            return "ID";
    case GD25Q_ERR_NOT_INIT:      return "NOT INIT";
    case GD25Q_ERR_PROTECTED:     return "PROTECTED";
    case GD25Q_ERR_WRITE_ENABLE:  return "WEL";
    case GD25Q_ERR_NEED_ERASE:    return "NEED ERASE";
    case GD25Q_ERR_VERIFY:        return "VERIFY";
    case GD25Q_ERR_POWER_DOWN:    return "POWER DOWN";
    case GD25Q_ERR_SUSPENDED:     return "SUSPENDED";
    case GD25Q_ERR_MODE:          return "QE MODE";
    default:                      return "UNKNOWN";
    }
}
