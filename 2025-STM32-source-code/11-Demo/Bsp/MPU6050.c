/**********************************************************
 * 文件名: MPU6050.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-29
 * 
 * 功能说明:
 *   实现MPU6050六轴传感器的驱动功能，基于软件I2C通信
 *   提供寄存器读写、设备初始化、ID获取及加速度/角速度数据采集等核心操作，
 *   支持±16g加速度量程与±2000°/s陀螺仪量程配置，适用于姿态解算与运动检测。
 * 
 * 接口备注:
 *   1. 依赖iic_soft.c实现底层I2C通信，使用7位从机地址
 **********************************************************/

#include "MPU6050.h"

/**
  * @brief： MPU6050写寄存器
  * @param： RegAddress 寄存器地址，范围：参考MPU6050手册的寄存器描述
  * @param： Data 要写入寄存器的数据，范围：0x00~0xFF
  * @retval：无
  */
void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
	iic_start();						//I2C起始
	iic_sendbyte(MPU6050_ADDRESS);	//发送从机地址，读写位为0，表示即将写入
	iic_recvask();					//接收应答
	iic_sendbyte(RegAddress);			//发送寄存器地址
	iic_recvask();					//接收应答
	iic_sendbyte(Data);				//发送要写入寄存器的数据
	iic_recvask();					//接收应答
	iic_stop();						//I2C终止
}

/**
  * @brief： MPU6050读寄存器
  * @param： RegAddress 寄存器地址，范围：参考MPU6050手册的寄存器描述
  * @retval：读取寄存器的数据，范围：0x00~0xFF
  */
uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
	uint8_t Data;
	
	iic_start();						//I2C起始
	iic_sendbyte(MPU6050_ADDRESS);	//发送从机地址，读写位为0，表示即将写入
	iic_recvask();					//接收应答
	iic_sendbyte(RegAddress);			//发送寄存器地址
	iic_recvask();					//接收应答
	
	iic_start();						//I2C重复起始
	iic_sendbyte(MPU6050_ADDRESS | 0x01);	//发送从机地址，读写位为1，表示即将读取
	iic_recvask();					//接收应答
	Data = iic_recvbyte();			//接收指定寄存器的数据
	iic_sendask(1);					//发送应答，给从机非应答，终止从机的数据输出
	iic_stop();						//I2C终止
	
	return Data;
}

/**
  * @brief： MPU6050初始化
  * @param： 无
  * @retval：无
  */
void MPU6050_Init(void)
{
	iic_init();									//先初始化底层的I2C
	
	/*MPU6050寄存器初始化，需要对照MPU6050手册的寄存器描述配置，此处仅配置了部分重要的寄存器*/
	MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);		//电源管理寄存器1，取消睡眠模式，选择时钟源为X轴陀螺仪
	MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);		//电源管理寄存器2，保持默认值0，所有轴均不待机
	MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x09);		//采样率分频寄存器，配置采样率
	MPU6050_WriteReg(MPU6050_CONFIG, 0x06);			//配置寄存器，配置DLPF
	MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);	//陀螺仪配置寄存器，选择满量程为±2000°/s
	MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);	//加速度计配置寄存器，选择满量程为±16g
}

/**
  * @brief： MPU6050获取ID号
  * @param： 无
  * @retval：MPU6050的ID号
  */
uint8_t MPU6050_GetID(void)
{
	return MPU6050_ReadReg(MPU6050_WHO_AM_I);		//返回WHO_AM_I寄存器的值
}

/**
  * @brief： MPU6050获取数据
  * @param： AccX AccY AccZ 加速度计X、Y、Z轴的数据，使用输出参数的形式返回，范围：-32768~32767
  * @param： GyroX GyroY GyroZ 陀螺仪X、Y、Z轴的数据，使用输出参数的形式返回，范围：-32768~32767
  * @retval：无
  */
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ, 
						int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
	uint8_t DataH, DataL;								//定义数据高8位和低8位的变量
	
	DataH = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_H);		//读取加速度计X轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_XOUT_L);		//读取加速度计X轴的低8位数据
	*AccX = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_H);		//读取加速度计Y轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_YOUT_L);		//读取加速度计Y轴的低8位数据
	*AccY = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_H);		//读取加速度计Z轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_ACCEL_ZOUT_L);		//读取加速度计Z轴的低8位数据
	*AccZ = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_GYRO_XOUT_H);		//读取陀螺仪X轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_GYRO_XOUT_L);		//读取陀螺仪X轴的低8位数据
	*GyroX = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_GYRO_YOUT_H);		//读取陀螺仪Y轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_GYRO_YOUT_L);		//读取陀螺仪Y轴的低8位数据
	*GyroY = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
	
	DataH = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_H);		//读取陀螺仪Z轴的高8位数据
	DataL = MPU6050_ReadReg(MPU6050_GYRO_ZOUT_L);		//读取陀螺仪Z轴的低8位数据
	*GyroZ = (DataH << 8) | DataL;						//数据拼接，通过输出参数返回
}
