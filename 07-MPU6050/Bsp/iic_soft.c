/**********************************************************
 * 文件名: iic_soft.c
 * 作  者: 电子科技协会嵌入式组
 * 日  期: 2025-09-29
 * 
 * 功能说明:
 *   实现基于GPIO模拟的I2C软件协议驱动
 *   提供SCL和SDA引脚的电平控制、起始/终止信号生成、字节收发及应答位处理，
 *   支持标准I2C通信速率，通过微秒级延时确保时序准确性
 **********************************************************/

#include "iic_soft.h"

/**
  * @brief： 微秒级延时函数
  * @param： None
  * @retval：None
  */
static void iic_delay_us(uint32_t us) 
{
    // 每次循环大约消耗 6 个时钟周期
    // 168MHz => 1 微秒 = 168 个时钟周期
    // 所以每个微秒大约需要 168 / 6 ≈ 28 次循环
    uint32_t n = us * 28;
    volatile uint32_t i;
	
    for (i = 0; i < n; i++) 
	{
        __NOP();
		__NOP();
    }
}

/**
  * @brief： I2C写SCL引脚电平
  * @param： BitValue 协议层传入的当前需要写入SCL的电平
  * @retval：None
  */
static void iic_w_scl(uint8_t BitValue)
{
	HAL_GPIO_WritePin(IIC_SCL_GPIO_Port, IIC_SCL_Pin, (GPIO_PinState)BitValue);
	iic_delay_us(10);
}

/**
  * @brief： I2C写SDA引脚电平
  * @param： BitValue 协议层传入的当前需要写入SDA的电平
  * @retval：None
  */
static void iic_w_sda(uint8_t BitValue)
{
	HAL_GPIO_WritePin(IIC_SDA_GPIO_Port, IIC_SDA_Pin, (GPIO_PinState)BitValue);	
	iic_delay_us(10);
}

/**
  * @brief： I2C读SDA引脚电平
  * @param： None
  * @retval：协议层需要得到的当前SDA的电平
  */
static uint8_t iic_r_sda(void)
{
	uint8_t BitValue;
	
	BitValue = HAL_GPIO_ReadPin(IIC_SDA_GPIO_Port, IIC_SDA_Pin);
	iic_delay_us(10);
	
	return BitValue;
}

/**
  * @brief： I2C初始化
  * @param： None
  * @retval：None
  */
void iic_init(void)
{
	//设置PB10和PB11引脚初始化后默认为高电平（释放总线状态）
	iic_w_scl(1);
	iic_w_sda(1);
}

/**
  * @brief： I2C起始
  * @param： None
  * @retval：None
  */
void iic_start(void)
{
	iic_w_sda(1);							//释放SDA，确保SDA为高电平
	iic_w_scl(1);							//释放SCL，确保SCL为高电平
	iic_w_sda(0);							//在SCL高电平期间，拉低SDA，产生起始信号
	iic_w_scl(0);							//起始后把SCL也拉低，即为了占用总线，也为了方便总线时序的拼接
}

/**
  * @brief： I2C终止
  * @param： None
  * @retval：None
  */
void iic_stop(void)
{
	iic_w_sda(0);							//拉低SDA，确保SDA为低电平
	iic_w_scl(1);							//释放SCL，使SCL呈现高电平
	iic_w_sda(1);							//在SCL高电平期间，释放SDA，产生终止信号
}

/**
  * @brief： I2C发送一个字节
  * @param： Byte 要发送的一个字节数据，范围：0x00~0xFF
  * @retval：None
  */
void iic_sendbyte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i ++)				//循环8次，主机依次发送数据的每一位
	{
		iic_w_sda(Byte & (0x80 >> i));	    //使用掩码的方式取出Byte的指定一位数据并写入到SDA线
		iic_w_scl(1);						//释放SCL，从机在SCL高电平期间读取SDA
		iic_w_scl(0);						//拉低SCL，主机开始发送下一位数据
	}
}

/**
  * @brief： I2C接收一个字节
  * @param： None
  * @retval：接收到的一个字节数据，范围：0x00~0xFF
  */
uint8_t iic_recvbyte(void)
{
	uint8_t i, Byte = 0x00;					//定义接收的数据，并赋初值0x00，此处必须赋初值0x00，后面会用到
	iic_w_sda(1);							//接收前，主机先确保释放SDA，避免干扰从机的数据发送
	for (i = 0; i < 8; i ++)				//循环8次，主机依次接收数据的每一位
	{
		iic_w_scl(1);						//释放SCL，主机机在SCL高电平期间读取SDA
		if (iic_r_sda() == 1){Byte |= (0x80 >> i);}	//读取SDA数据，并存储到Byte变量
														//当SDA为1时，置变量指定位为1，当SDA为0时，不做处理，指定位为默认的初值0
		iic_w_scl(0);						//拉低SCL，从机在SCL低电平期间写入SDA
	}
	return Byte;							//返回接收到的一个字节数据
}

/**
  * @brief： I2C发送应答位
  * @param： Byte 要发送的应答位，范围：0~1，0表示应答，1表示非应答
  * @retval：None
  */
void iic_sendask(uint8_t AckBit)
{
	iic_w_sda(AckBit);					    //主机把应答位数据放到SDA线
	iic_w_scl(1);							//释放SCL，从机在SCL高电平期间，读取应答位
	iic_w_scl(0);							//拉低SCL，开始下一个时序模块
}

/**
  * @brief： I2C接收应答位
  * @param： None
  * @retval：接收到的应答位，范围：0~1，0表示应答，1表示非应答
  */
uint8_t iic_recvask(void)
{
	uint8_t AckBit;							//定义应答位变量
	iic_w_sda(1);							//接收前，主机先确保释放SDA，避免干扰从机的数据发送
	iic_w_scl(1);							//释放SCL，主机机在SCL高电平期间读取SDA
	AckBit = iic_r_sda();					//将应答位存储到变量里
	iic_w_scl(0);							//拉低SCL，开始下一个时序模块
	return AckBit;							//返回定义应答位变量
}
