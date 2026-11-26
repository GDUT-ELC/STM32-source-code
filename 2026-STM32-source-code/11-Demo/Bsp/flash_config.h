/**********************************************************
 * 文件名: flash_config.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-11-01
 *
 * 功能说明:
 *   集中保存 GD25Q80C 的接线与等待时间。更换板子或改动接线时，
 *   只需修改本文件，驱动源码无需改动。
 *
 * 接口备注:
 *   1. 引脚须与 CubeMX 工程（12-gd25qdemo.ioc）保持一致（可修改）：
 *      PB3=SPI1_SCK，PB4=SPI1_MISO，PB5=SPI1_MOSI，PA15=软件片选
 *   2. 片选由普通 GPIO 控制，不使用 SPI 外设的硬件 NSS
 *   3. 等待时间单位为毫秒，按数据手册最大值留有裕量
 **********************************************************/

#ifndef __FLASH_CONFIG_H
#define __FLASH_CONFIG_H

#include "spi.h"
#include "main.h"

/* ---------- 接线 ---------- */
#define FLASH_SPI_HANDLE    hspi1           /* 使用的 SPI 外设 */
#define FLASH_CS_PORT       GPIOA           /* 片选端口 */
#define FLASH_CS_PIN        GPIO_PIN_15     /* 片选引脚 PA15 */


/* 片选电平：拉低选中芯片，拉高结束本次通信 */
#define FLASH_CS_L()        HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_RESET)
#define FLASH_CS_H()        HAL_GPIO_WritePin(FLASH_CS_PORT, FLASH_CS_PIN, GPIO_PIN_SET)


/* ---------- 等待时间 ---------- */
#define FLASH_SPI_TIMEOUT_MS     100UL      /* 单次 SPI 收发超时 */


/* 擦写等待上限，注释为数据手册最大值，其余为裕量：
 * 页编程 2.4ms，扇区擦除 300ms，块擦除 800ms，整片擦除 10s */
#define FLASH_PAGE_TIMEOUT_MS    10UL
#define FLASH_SECTOR_TIMEOUT_MS  1000UL
#define FLASH_BLOCK_TIMEOUT_MS   2000UL
#define FLASH_CHIP_TIMEOUT_MS    20000UL

#endif
