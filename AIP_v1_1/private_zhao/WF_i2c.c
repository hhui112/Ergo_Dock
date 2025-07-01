/*
 * SoftIIC.c
 *
 *  Created on: Aug 3, 2020
 *      Author: ITry
 */
#include "WF_i2c.h"
#include "main.h"
#include "x_sf.h"
#include "x_drive.h"
#define	io_SCL_SET     io_write_pin(IIC_SCL_PIN,1)//  LSGPIOB->DOUT |=0X0008;//PB3 输出1
#define	io_SCL_RESET	io_write_pin(IIC_SCL_PIN,0)//LSGPIOB->DOUT ^=~0X0008;//PB3 输出0
#define	io_SDA_SET	  io_write_pin(IIC_SDA_PIN,1)//LSGPIOB->DOUT |=0X0010;//PB4 输出1
#define	io_SDA_RESET	io_write_pin(IIC_SDA_PIN,0)//LSGPIOB->DOUT ^=~0X0010;//PB4 输出0
//#define	io_SDA   	(LSGPIOB->DIN>>4)&0X01
//io_get_input_val(PB4);
void SDA_OUTPUT(void)//设置为输出口  由于该芯片为真I2C接口，不需要做任何操作
{
}

void SDA_INPUT(void)//设置为输入口 由于该芯片为真I2C接口，不需要做任何设置操作，只需要把OC输出设置为高即可
{
	io_SDA_SET;
}
void SCL_OUTPUT(void)//设置为输出口  由于该芯片为真I2C接口，不需要做任何操作
{
}

void SCL_INPUT(void)//设置为输入口  由于该芯片为真I2C接口，不需要做任何设置操作，只需要把OC输出设置为高即可
{
  io_SCL_SET;
}


/********************************************************************
* Function Name  : iic_start.
* Description    : iic启动:当SCL处于高电平状态时，SDA出现一个下降沿,即产生IIC启动信号
* Input          : None.
* Output         : None.
* Return         : None.
********************************************************************/
void iic_start(void) 
{
	SDA_OUTPUT();
	io_SDA_SET;
	io_SCL_SET;
	DELAY_US(1);//DELAY_US(1)
	//delay_us(1);
	io_SDA_RESET;
	DELAY_US(1);
	//delay_us(1);
	io_SCL_RESET;
	DELAY_US(2);
	//delay_us(2);
}
/********************************************************************
* Function Name  : iic_stop.
* Description    : iic停止:当SCL处于高电平状态时，SDA出现一个上升沿,即产生IIC停止信号
* Input          : None.
* Output         : None.
* Return         : None.
********************************************************************/
void iic_stop(void) 
{
	SDA_OUTPUT();
	io_SCL_RESET;
	io_SDA_RESET;
	DELAY_US(1);
	//delay_us(1);
	io_SCL_SET;
	DELAY_US(1);
	//delay_us(1);
	io_SDA_SET;
	DELAY_US(2);
	//delay_us(2);	
}
/********************************************************************
* Function Name  : iic_wait_ack.
* Description    : 应答状态，0表示应答，1表示设备无响应
* Input          : None.
* Output         : None.
* Return         : None.
********************************************************************/
void iic_wait_ack(void)
{
	uint8_t err_time = 0;
	SDA_INPUT();  /** 在等待应答信号之前，要释放SDA */
	io_SDA_SET;
	DELAY_US(2);
	//delay_us(2);
	io_SCL_SET;
	DELAY_US(1);
	//delay_us(1);
//	GIE = 0;
	
	while (io_get_input_val(IIC_SDA_PIN)) 
	{
		err_time++ ;
		if (err_time > 250)
		{
		//	iic_stop();
			break;
		}
	}
	
//	GIE = 1;
	io_SCL_RESET;
 DELAY_US(3);
	//delay_us(3);

}
/********************************************************************
* Function Name  : iic_ack.
* Description    : 主机（主控制器）产生应答信号
* Input          : None.
* Output         : None.
* Return         : None.
********************************************************************/
void iic_ack(void)
{
	SDA_OUTPUT();
	io_SDA_RESET;
	DELAY_US(1);
	//delay_us(1);
	io_SCL_SET;
	DELAY_US(1);
	//delay_us(1);
	io_SCL_RESET;
	DELAY_US(2);
	//delay_us(2);
	io_SDA_SET;
}
/********************************************************************
* Function Name  : iic_read_byte.
* Description    : 主机（主控制器）产生不应答信号
* Input          : None.
* Output         : None.
* Return         : None.
********************************************************************/
void iic_noAck(void)
{
	SDA_OUTPUT();
	io_SDA_SET;
	DELAY_US(1);
	//delay_us(1);
	io_SCL_SET;
	DELAY_US(1);
	//delay_us(1);
	io_SCL_RESET;
	DELAY_US(2);
	//delay_us(2);
	io_SDA_RESET;
}

/********************************************************************
* Function Name  : iic_write_byte.
* Description    : 获取一次数据
* Input          : None.
* Output         : None.
* Return         : None.
********************************************************************/
void iic_write_byte(uint8_t byte) 
{
	uint8_t i;
	SDA_OUTPUT();
	io_SCL_RESET;
	/** 发送一个字节的高7位 */
	
	for (i = 0; i < 8; i++) 
	{
		if (byte & 0x80) 
		{
			io_SDA_SET;
		} 
		else 
		{
			io_SDA_RESET;
		}
		
		DELAY_US(2);
		io_SCL_SET;
		DELAY_US(2);
		io_SCL_RESET;
		byte <<= 1;
	}
}

/********************************************************************
* Function Name  : iic_read_byte.
* Description    : 获取一次数据
* Input          : None.
* Output         : None.
* Return         : None.
********************************************************************/
uint8_t iic_read_byte(void)
{
	uint8_t i;
	uint8_t recv_value = 0;
	DELAY_US(2);
	//delay_us(2);
	SDA_INPUT();
	for (i = 0; i < 8; i++)
	{
		io_SCL_SET;
		DELAY_US(2);
		//delay_us(2);
		recv_value <<= 1;
		if (io_get_input_val(IIC_SDA_PIN))
		{
			recv_value |= 0x01;
		} else {
			recv_value &= ~0x01;
		}
		io_SCL_RESET;
		DELAY_US(2);
		//delay_us(2);
	}
	return recv_value;
}

/********************************************************************
* Function Name  : IIC_WriteByte.
* Description    : 
* Input          : None.
* Output         : None.
* Return         : None.
********************************************************************/
void IIC_WriteByte(uint8_t Device,uint8_t addr,uint8_t Data)
{
	SCL_OUTPUT();
	iic_start();
	iic_write_byte(Device); //写设备号
	iic_wait_ack();
	iic_write_byte(addr); //写要写入的寄存器地址
	iic_wait_ack();
	iic_write_byte(Data); //读取数据
	iic_wait_ack();
	iic_stop();
	SDA_INPUT();
//	SCL_INPUT();
}
/********************************************************************
* Function Name  : IIC_ReadContiune.
* Description    : 
* Input          : None.
* Output         : None.
* Return         : None.
********************************************************************/
void IIC_ReadContiune(uint8_t Device,uint8_t addr,uint8_t *p,uint8_t Length)
{
	uint8_t i = 0;
//	SDA_OUTPUT();
	SCL_OUTPUT();
	iic_start();
	iic_write_byte(Device); //写设备号
	iic_wait_ack();
	iic_write_byte(addr); //写要写入的寄存器地址
	iic_wait_ack();
	iic_start();
	iic_write_byte(Device|0X01); //读取数据
	iic_wait_ack();
	for(i=0;i<Length;i++)
	{
		p[i]= iic_read_byte();
		if(i < Length-1)
		{
			iic_ack();
		}
	}
	
	iic_noAck();
	iic_stop();
	SDA_INPUT();
	SCL_INPUT();
}