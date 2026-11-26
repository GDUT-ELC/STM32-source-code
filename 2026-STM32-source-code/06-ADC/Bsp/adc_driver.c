/**********************************************************
 * 文件名: adc_driver.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   封装ADC启动、等待、读取和停止操作，并提供ADC原始值到电压值的
 *   换算功能，使应用程序可以直接调用函数完成模拟量采集
 *
 * 接口备注:
 *   1. 使用ADC1进行单次转换，ADC分辨率为12位
 *   2. ADC参考电压为3.3V，原始值范围为0~4095
 *   3. 采样失败或等待超时时，adc_read_value返回0
 **********************************************************/

#include "adc_driver.h"
#include "adc.h"

#define ADC_MAX_VALUE        4095.0f
#define ADC_REFERENCE_VOLTAGE   3.3f
#define ADC_TIMEOUT_MS             10

/**
  * @brief  启动ADC转换并读取一次原始采样值
  * @param  None
  * @retval uint16_t ADC原始值，范围为0~4095；采样失败时返回0
  */
uint16_t adc_read_value(void)
{
    uint16_t adc_value = 0;

    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        return 0;
    }

    if (HAL_ADC_PollForConversion(&hadc1, ADC_TIMEOUT_MS) == HAL_OK)
    {
        adc_value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }

    HAL_ADC_Stop(&hadc1);

    return adc_value;
}

/**
  * @brief  将12位ADC原始值换算为电压值
  * @param  adc_value ADC原始值，范围为0~4095
  * @retval float 换算后的电压值，单位为伏特(V)
  */
float adc_convert_to_voltage(uint16_t adc_value)
{
    return ((float)adc_value / ADC_MAX_VALUE) * ADC_REFERENCE_VOLTAGE;
}
