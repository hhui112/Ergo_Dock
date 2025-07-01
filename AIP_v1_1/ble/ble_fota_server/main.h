#ifndef __MAIN_H
#define __MAIN_H

#include "ls_ble.h"
#include "platform.h"
#include "prf_diss.h"
#include "log.h"
#include "ls_dbg.h"
#include "cpu.h"
#include "ls_hal_uart.h"
#include "builtin_timer.h"
#include <string.h>
#include "co_math.h"
#include "ls_soc_gpio.h"
//#include "ble_common_api.h"
#include "ls_hal_timer.h"
#include "ls_hal_adc.h"
#include "ls_hal_dmac.h"
#include "ble_base_set.h"
//#include "stdint.h"
//#include "ls_ble.h"
#include "platform.h"
#include "ls_hal_uart.h"
//#include "prf_diss.h"
//#include "log.h"
//#include "ls_dbg.h"
//#include "cpu.h"
//#include "lsuart.h"
//#include "builtin_timer.h"
//#include <string.h>
//#include "co_math.h"
//#include "io_config.h"
//#include "adv.h"

//#include "board_config.h"

//#include "srv_config.h"

//#include "bo_ota.h"

//#include "bsp.h"
uint8_t ls_bleup_server_send_notification(uint8_t  *data_notice,uint16_t length);//蓝牙上行数据通信
void create_adv_obj();//创建广播对象
void Error_Handler(void);
void changePWM_PULSE(uint8_t PULSE_SET);
#endif
