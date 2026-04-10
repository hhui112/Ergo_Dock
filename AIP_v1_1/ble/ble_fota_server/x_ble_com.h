#ifndef X_BLE_COM_H
#define X_BLE_COM_H
#include "stdint.h"

/*
 * Smart2.0 链路层（无同步头、无 BLE 尾校验）：帧长(1) + 端口(1) + 数据域(0~160)
 * 整帧最大 2 + 160 = 162 字节；主控原始段内校验和见主控/MFP 协议，非 BLE 链路字段。
 */
#define BLE_LINK_FRAME_BUF      180U

extern uint8_t blerxbuff[BLE_LINK_FRAME_BUF];

extern uint8_t bletxbuff[BLE_LINK_FRAME_BUF];

extern uint8_t bletxlen;

void x_ble_com_handle(uint8_t *pbuff, uint8_t len);
void x_ble_com_txbuffFill(void);

#endif
