#ifndef	_WF_I2C_H_
#define	_WF_I2C_H_
#include "main.h"
#include "reg_gpio.h"
 
void SDA_OUTPUT(void);
void SDA_INPUT(void);
void SCL_OUTPUT(void);
void SCL_INPUT(void);

void iic_start(void);
void iic_stop(void);
void iic_wait_ack(void);
void iic_ack(void);
void iic_noAck(void);

void iic_write_byte(uint8_t byte);
uint8_t iic_read_byte(void);

void IIC_ReadContiune(uint8_t Device,uint8_t addr,uint8_t *p,uint8_t Length);

void IIC_WriteByte(uint8_t Device,uint8_t addr,uint8_t Data);

#endif //_I2C_H_
