/**********************************************************
 * 文件名: MPU6050.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2026-10-29
 *
 * 功能说明:
 *   基于软件I2C实现MPU6050初始化、设备ID检查和六轴原始数据读取，
 *   默认配置为±16g加速度量程、±2000°/s角速度量程和100Hz采样率
 *
 * 接口备注:
 *   1. 7位设备地址为0x68，发送到总线前会自动加入读写方向位
 *   2. 初始化和数据读取都会检查从机ACK，失败时返回HAL_ERROR
 *   3. 六轴数据使用一次14字节连续读取，避免跨采样拼接数据
 *   4. 返回的是传感器原始值，物理单位换算需由上层根据量程完成
 **********************************************************/

#include "MPU6050.h"
#include "MPU6050_Reg.h"
#include "iic_soft.h"

#define MPU6050_WRITE_ADDRESS  ((uint8_t)(MPU6050_ADDRESS_7BIT << 1U))
#define MPU6050_READ_ADDRESS   ((uint8_t)(MPU6050_WRITE_ADDRESS | 0x01U))
#define MPU6050_EXPECTED_ID    0x68U
#define MPU6050_DATA_LENGTH    14U

/**
  * @brief  向MPU6050的一个寄存器写入数据
  * @param  reg_address 寄存器地址
  * @param  data 待写入的数据
  * @retval HAL_OK表示写入成功，HAL_ERROR表示从机未应答
  */
static HAL_StatusTypeDef mpu6050_write_register(uint8_t reg_address, uint8_t data)
{
    iic_start();

    iic_send_byte(MPU6050_WRITE_ADDRESS);
    if (iic_receive_ack() != 0U)
    {
        iic_stop();
        return HAL_ERROR;
    }

    iic_send_byte(reg_address);
    if (iic_receive_ack() != 0U)
    {
        iic_stop();
        return HAL_ERROR;
    }

    iic_send_byte(data);
    if (iic_receive_ack() != 0U)
    {
        iic_stop();
        return HAL_ERROR;
    }

    iic_stop();
    return HAL_OK;
}

/**
  * @brief  从指定寄存器开始连续读取多个字节
  * @param  start_address 起始寄存器地址
  * @param  data 接收缓冲区
  * @param  length 需要读取的字节数
  * @retval HAL_OK表示读取成功，HAL_ERROR表示参数或通信错误
  */
static HAL_StatusTypeDef mpu6050_read_registers(uint8_t start_address,
                                                uint8_t *data,
                                                uint8_t length)
{
    uint8_t i;

    if ((data == NULL) || (length == 0U))
    {
        return HAL_ERROR;
    }

    iic_start();

    iic_send_byte(MPU6050_WRITE_ADDRESS);
    if (iic_receive_ack() != 0U)
    {
        iic_stop();
        return HAL_ERROR;
    }

    iic_send_byte(start_address);
    if (iic_receive_ack() != 0U)
    {
        iic_stop();
        return HAL_ERROR;
    }

    iic_start();
    iic_send_byte(MPU6050_READ_ADDRESS);
    if (iic_receive_ack() != 0U)
    {
        iic_stop();
        return HAL_ERROR;
    }

    for (i = 0U; i < length; i++)
    {
        data[i] = iic_receive_byte();
        iic_send_ack((i == (uint8_t)(length - 1U)) ? 1U : 0U);
    }

    iic_stop();
    return HAL_OK;
}

/**
  * @brief  初始化MPU6050并检查设备ID
  * @param  None
  * @retval HAL_OK表示初始化成功，HAL_ERROR表示通信或设备ID错误
  */
HAL_StatusTypeDef MPU6050_Init(void)
{
    uint8_t device_id;

    iic_init();
    HAL_Delay(100U);

    if ((MPU6050_GetID(&device_id) != HAL_OK) || (device_id != MPU6050_EXPECTED_ID))
    {
        return HAL_ERROR;
    }

    if (mpu6050_write_register(MPU6050_PWR_MGMT_1, 0x01U) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (mpu6050_write_register(MPU6050_PWR_MGMT_2, 0x00U) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (mpu6050_write_register(MPU6050_SMPLRT_DIV, 0x09U) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (mpu6050_write_register(MPU6050_CONFIG, 0x06U) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (mpu6050_write_register(MPU6050_GYRO_CONFIG, 0x18U) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (mpu6050_write_register(MPU6050_ACCEL_CONFIG, 0x18U) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}

/**
  * @brief  读取MPU6050的WHO_AM_I设备ID
  * @param  device_id 用于保存设备ID的指针
  * @retval HAL_OK表示读取成功，HAL_ERROR表示参数或通信错误
  */
HAL_StatusTypeDef MPU6050_GetID(uint8_t *device_id)
{
    return mpu6050_read_registers(MPU6050_WHO_AM_I, device_id, 1U);
}

/**
  * @brief  一次连续读取三轴加速度和三轴角速度原始值
  * @param  acc_x acc_y acc_z 三轴加速度原始值输出指针
  * @param  gyro_x gyro_y gyro_z 三轴角速度原始值输出指针
  * @retval HAL_OK表示读取成功，HAL_ERROR表示参数或通信错误
  */
HAL_StatusTypeDef MPU6050_GetData(int16_t *acc_x,
                                  int16_t *acc_y,
                                  int16_t *acc_z,
                                  int16_t *gyro_x,
                                  int16_t *gyro_y,
                                  int16_t *gyro_z)
{
    uint8_t data[MPU6050_DATA_LENGTH];

    if ((acc_x == NULL) || (acc_y == NULL) || (acc_z == NULL) ||
        (gyro_x == NULL) || (gyro_y == NULL) || (gyro_z == NULL))
    {
        return HAL_ERROR;
    }

    if (mpu6050_read_registers(MPU6050_ACCEL_XOUT_H,
                               data,
                               MPU6050_DATA_LENGTH) != HAL_OK)
    {
        return HAL_ERROR;
    }

    *acc_x = (int16_t)(((uint16_t)data[0] << 8U) | data[1]);
    *acc_y = (int16_t)(((uint16_t)data[2] << 8U) | data[3]);
    *acc_z = (int16_t)(((uint16_t)data[4] << 8U) | data[5]);
    *gyro_x = (int16_t)(((uint16_t)data[8] << 8U) | data[9]);
    *gyro_y = (int16_t)(((uint16_t)data[10] << 8U) | data[11]);
    *gyro_z = (int16_t)(((uint16_t)data[12] << 8U) | data[13]);

    return HAL_OK;
}
