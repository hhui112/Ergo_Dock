#ifndef X_DRIVE_H
#define X_DRIVE_H

#include "stdint.h"

#define UART_TX1_PIN 				PB01		// MFP tx
#define UART_RX1_PIN 			 	PB00
#define UART_CTR 			 			PB02		// DE

#define UART_TX2_PIN 				PC00		// ÀëÏßÓïÒôÊä³ö
#define UART_RX2_PIN 			 	PA05		

#define UART_TX3_PIN 				PA08		// ÷ýÉùÐ¾Æ¬Êä³ö
#define UART_RX3_PIN  			PA09
#define SNORE_OUT_PIN  			PA06   //÷ýÉù¼ì²âÐ¾Æ¬ Âö³åÊä³ö

#define LED_Blue_0_PIN      PB15           
#define LED_Blue_1_PIN      PA00  
#define LED_Blue_2_PIN      PA02  
#define LED_Blue_3_PIN      PB09

#define LED_Green_1_PIN     PA01           
#define LED_Green_3_PIN     PB10  

#define LED_Red_all_PIN     PB13 

#define KEY1_PIN            PA14
#define KEY2_PIN            PA15
#define KEY3_PIN            PA12

#define PUMPCRT_CTR_PIN  		PA12   //Æø±Ã¿ØÖÆÒý½Å
#define VALVE_CTR_PIN  			PA13   //Æø·§¿ØÖÆÒý½Å
#define EXPUMPCRT_CTR_PIN  	PA14   //³éÆø·§

void x_driver_init(void);
void x_rtc_set(uint16_t year,uint8_t Month,uint8_t Day,uint8_t Hour,uint8_t Minute,uint8_t Second,uint8_t Week);
void x_pump_pwmset(uint8_t set_value);
void x_valve_pwmset(uint8_t set_value);
void x_expump_pwmset(uint8_t set_value);
void x_rtc_get(void);
void x_rtc_init(void);
#endif