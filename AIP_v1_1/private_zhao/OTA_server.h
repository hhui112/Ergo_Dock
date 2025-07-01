#ifndef __OTA_SERVER_H
#define __OTA_SERVER_H

#define UART_SERVER_WITH_OTA 1 //启用OTA升级服务
#include "main.h"
#include "platform.h"
#include "prf_fotas.h"
#include "ls_dbg.h"
#include "ota_settings.h"
#include "ls_ble.h"

#if UART_SERVER_WITH_OTA == 1
void prf_fota_server_callback(enum fotas_evt_type type,union fotas_evt_u *evt);
void prf_added_handler(struct profile_added_evt *evt);
#endif


#endif