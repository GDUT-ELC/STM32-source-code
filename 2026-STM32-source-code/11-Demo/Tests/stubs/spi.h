#ifndef TEST_SPI_H
#define TEST_SPI_H
#include "main.h"
typedef struct { unsigned unused; } SPI_HandleTypeDef;
extern SPI_HandleTypeDef hspi1;
#define GPIOA 3U
#define GPIO_PIN_15 0x8000U
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *, uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *, uint8_t *, uint16_t, uint32_t);
#endif
