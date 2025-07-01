#ifndef _uart_base_H
#define _uart_base_H
#include "main.h"
#include "timer_event.h"
#define CON_IDX_INVALID_VAL 0xff

#define uart1_datasize  100
#define uart_rxtime_over 5//串口软定时接收溢出时间设置
typedef struct{
	uint8_t index;
	uint8_t buf[uart1_datasize];
}__attribute__((packed))uart_buf_def;

//////////////////////////////////
typedef struct {
	uart_buf_def uart_rx;
	uart_buf_def uart_tx;
}__attribute__((packed))uart_def;//

extern uint8_t uart_server_rx_byte;
extern struct gatt_svc_env ls_uart_server_svc_env;
extern UART_HandleTypeDef UART_Server_Config; 
extern bool update_adv_intv_flag ;
void ls_uart_init(void);
void ls_uarttime_server_init(void);
void ls_uart_tx_send(ble_data_init *p);
void ls_uart_realtimeprint(void);
void x_uart_realtimeprint(void);
#endif
