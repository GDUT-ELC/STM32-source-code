/**********************************************************
 * 文件名: adc_driver.h
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   声明ADC采样与电压换算函数，为应用程序提供简单、直接的调用接口
 *
 * 接口备注:
 *   1. 使用前需先调用CubeMX生成的MX_ADC1_Init函数
 *   2. adc_read_value用于读取12位ADC原始值
 *   3. adc_convert_to_voltage用于将原始值换算为电压值
 **********************************************************/

#ifndef __ADC_DRIVER_H
#define __ADC_DRIVER_H

#include "main.h"

uint16_t adc_read_value(void);
float adc_convert_to_voltage(uint16_t adc_value);

#endif
