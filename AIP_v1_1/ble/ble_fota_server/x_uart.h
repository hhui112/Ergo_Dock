#ifndef X_UART_H
#define X_UART_H
#include "stdint.h"
#include "stdbool.h"
#include "g.h"

#define TXBUFF_LEN 100
#define RXBUFF_LEN 100

#define COM_TIMEOUT		2	//	20 ms


typedef struct{
	uint8_t rxindex;
	uint32_t timeout;
	uint8_t rxbuf[RXBUFF_LEN];
	uint8_t txbuf[RXBUFF_LEN];
}uart_data_st;


void x_uart_init(void);
void x_uart_10ms(void);
void x_uart1_dateReceiveHandle(void);

/** CI1302 离线语音：按 g_offline_voice 下发 8 字节配置帧（UART2）。force=true 时强制发送（如上电）；false 时仅在关/开或 Ergo/Bed 切换时发送。 */
void ci1302_uart2_voice_config_sync(bool force);

extern uint8_t s_tx_busy;

extern UART_HandleTypeDef UART_Server3_Config; 
extern UART_HandleTypeDef UART_Server2_Config; 
extern UART_HandleTypeDef UART_Server1_Config;
#endif