#ifndef X_BLE_COM_H
#define X_BLE_COM_H
#include "stdint.h"

extern uint8_t blerxbuff[150];

extern uint8_t bletxbuff[150];

extern uint8_t bletxlen;

void x_ble_com_handle(uint8_t *pbuff,uint8_t len);
void x_ble_com_txbuffFill(void);
#endif