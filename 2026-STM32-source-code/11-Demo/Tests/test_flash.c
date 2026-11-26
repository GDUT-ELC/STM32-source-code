/* Exercise the actual integrated Flash driver. The model accepts standard
 * SPI commands with QE either set or clear, and rejects status-register writes. */
#include "spi.h"
#include "app_flash.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

SPI_HandleTypeDef hspi1;
static uint8_t memory[GD25Q_FLASH_SIZE], pending[GD25Q_PAGE_SIZE];
static uint8_t command, sr1_protection, sr2;
static uint16_t pending_length;
static uint32_t address, cursor, tick, busy_until, programs, erases;
static int selected, header_received, wel, bad_id, rx_failure;
static int busy(void) { return (int32_t)(busy_until - tick) > 0; }

uint32_t HAL_GetTick(void) { return tick; }
void HAL_Delay(uint32_t delay) { tick += delay; }
void HAL_GPIO_WritePin(unsigned port, unsigned pin, GPIO_PinState state)
{
    assert(port == GPIOA && pin == GPIO_PIN_15);
    if (state == GPIO_PIN_RESET) {
        assert(!selected);
        selected = 1; header_received = 0; pending_length = 0; command = 0;
        return;
    }
    if (!selected) return;
    selected = 0;
    if (!header_received) return;
    if (command == 0xAB) return;
    if (busy()) return;
    if (command == 0x06) wel = 1;
    else if (command == 0x02 && pending_length) {
        uint32_t i;
        assert(wel && !sr1_protection && !(sr2 & 0xC0U));
        for (i = 0; i < pending_length; ++i)
            memory[(address & ~255UL) | ((address + i) & 255UL)] &= pending[i];
        ++programs; wel = 0; busy_until = tick + 2U;
    } else if (command == 0x20) {
        assert(wel && !sr1_protection && !(sr2 & 0xC0U));
        memset(memory + address - address % GD25Q_SECTOR_SIZE, 255, GD25Q_SECTOR_SIZE);
        ++erases; wel = 0; busy_until = tick + 3U;
    }
}
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *port, uint8_t *data,
                                  uint16_t length, uint32_t timeout)
{
    assert(port == &hspi1 && selected && length && timeout);
    if (!header_received) {
        command = data[0];
        /* No writes to QE, protection, OTP, or other status bits are allowed. */
        assert(command == 0xAB || command == 0x9F || command == 0x05 ||
               command == 0x35 || command == 0x06 || command == 0x03 ||
               command == 0x02 || command == 0x20);
        address = 0;
        if (length == 4U)
            address = ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | data[3];
        else assert(length == 1U);
        assert(address < GD25Q_FLASH_SIZE);
        cursor = address; header_received = 1;
    } else {
        assert(command == 0x02 && length <= sizeof(pending));
        memcpy(pending, data, length); pending_length = length;
    }
    return HAL_OK;
}
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *port, uint8_t *data,
                                 uint16_t length, uint32_t timeout)
{
    assert(port == &hspi1 && selected && header_received && length && timeout);
    if (rx_failure) return HAL_ERROR;
    if (command == 0x9F) {
        assert(length == 3U);
        data[0] = bad_id ? 0xEF : 0xC8; data[1] = 0x40; data[2] = 0x14;
    } else if (command == 0x05) {
        assert(length == 1U);
        *data = sr1_protection | (wel ? 2U : 0U) | (busy() ? 1U : 0U);
    } else if (command == 0x35) {
        assert(length == 1U); *data = sr2;
    } else {
        assert(command == 0x03 && !busy() && cursor + length <= GD25Q_FLASH_SIZE);
        memcpy(data, memory + cursor, length); cursor += length;
    }
    return HAL_OK;
}

static void test_qe(uint8_t status2)
{
    uint8_t data[] = {0x11, 0x22, 0x33, 0x44};
    uint8_t inverse[] = {0xEE, 0xDD, 0xCC, 0xBB}, result[4], low, high;
    uint32_t i, erased_before;
    memset(memory, 255, sizeof(memory));
    sr1_protection = 0; sr2 = status2; wel = 0;
    assert(app_flash_init() == GD25Q_OK);
    assert(g_flash_info.id == GD25Q_JEDEC_ID);
    /* The existing 12-page test address, plus data elsewhere in its sector. */
    memory[0x1100] = 0x5A;
    assert(app_flash_save(0x1000, data, sizeof(data)) == GD25Q_OK);
    assert(app_flash_load(0x1000, result, sizeof(result)) == GD25Q_OK);
    assert(memcmp(result, data, sizeof(data)) == 0);
    erased_before = erases;
    assert(app_flash_save(0x1000, inverse, sizeof(inverse)) == GD25Q_OK);
    assert(app_flash_load(0x1000, result, sizeof(result)) == GD25Q_OK);
    assert(memcmp(result, inverse, sizeof(result)) == 0);
    assert(erases == erased_before + 1U && memory[0x1100] == 0x5A);
    assert(GD25Q_ReadStatus(&low, &high) == GD25Q_OK && high == status2);
    assert(sr2 == status2);
    for (i = 0; i < sizeof(data); ++i) data[i] = 0;
    sr1_protection = 0x04;
    assert(app_flash_save(0x1000, data, sizeof(data)) == GD25Q_ERR_PROTECTED);
    sr1_protection = 0; sr2 |= 0x40;
    assert(app_flash_save(0x1000, data, sizeof(data)) == GD25Q_ERR_PROTECTED);
    sr2 = status2 | 0x80;
    assert(app_flash_load(0x1000, result, sizeof(result)) == GD25Q_ERR_SUSPENDED);
    sr2 = status2;
    bad_id = 1; assert(app_flash_init() == GD25Q_ERR_ID); bad_id = 0;
    rx_failure = 1; assert(app_flash_init() == GD25Q_ERR_SPI); rx_failure = 0;
    printf("PASS: SR2=0x%02X init, write, erase/rewrite, data preservation; protection/ID/SPI checks retained\n", status2);
}
int main(void)
{
    test_qe(0x00); test_qe(0x02); test_qe(0x0A);
    assert(programs && erases);
    puts("All Flash QE regression tests passed; no status-register writes issued.");
    return 0;
}
