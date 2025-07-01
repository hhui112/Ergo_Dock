#ifndef X_DRIVE_H
#define X_DRIVE_H

#include "stdint.h"

// ����
/*
#define IIC_SDA_PIN  				PA02
#define IIC_SCL_PIN  				PA01
#define UART_TX1_PIN 				PB00
#define UART_RX1_PIN 			 	PB01
#define UART_TX2_PIN 				PA15
#define UART_TX3_PIN 				PA06
#define UART_RX3_PIN  			PA05
#define SNORE_OUT_PIN  			PA08   //�������оƬ ���������
#define PUMPCRT_CTR_PIN  		PA12   //���ÿ�������
#define VALVE_CTR_PIN  			PA13   //������������
#define EXPUMPCRT_CTR_PIN  	PA14   //������
*/

// ��
#define UART_TX1_PIN 				PB01		// MFP tx
#define UART_RX1_PIN 			 	PB00
#define UART_CTR 			 			PB02		// DE

#define UART_TX2_PIN 				PC00		// �����������
#define UART_RX2_PIN 			 	PA05		

#define UART_TX3_PIN 				PA08		// ����оƬ���
#define UART_RX3_PIN  			PA09
#define SNORE_OUT_PIN  			PA06   //�������оƬ �������

#define IIC_SDA_PIN  				PA02
#define IIC_SCL_PIN  				PA01
#define PUMPCRT_CTR_PIN  		PA12   //���ÿ�������
#define VALVE_CTR_PIN  			PA13   //������������
#define EXPUMPCRT_CTR_PIN  	PA14   //������

void x_driver_init(void);
void x_rtc_set(uint16_t year,uint8_t Month,uint8_t Day,uint8_t Hour,uint8_t Minute,uint8_t Second,uint8_t Week);
void x_pump_pwmset(uint8_t set_value);
void x_valve_pwmset(uint8_t set_value);
void x_expump_pwmset(uint8_t set_value);
void x_rtc_get(void);
void x_rtc_init(void);
#endif