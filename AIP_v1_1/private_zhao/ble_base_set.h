#ifndef __BLE_BASE_SET_H
#define __BLE_BASE_SET_H
#include "stdint.h"
#include "ls_ble.h"
#include "ls_dbg.h"
#include "ls_hal_uart.h"
#include "ls_ble.h"
#include "fota_svc_server.h"
#define ADV_NAME   "iPillow:"
#define ADV_NAME_LEN   (sizeof(ADV_NAME)-1)
#define UART_SVC_RX_MAX_LEN (USER_MAX_MTU - 3)
#define UART_SVC_TX_MAX_LEN (USER_MAX_MTU - 3)
#define UART_SVC_BUFFER_SIZE (1024)
	typedef struct
{
	uint8_t  pdata[512];
	uint16_t plength;
}s0020_data_info_t;

typedef struct
{
	s0020_data_info_t tx;
	s0020_data_info_t rx;
}srv0020_data_info_t;
enum uart_svc_att_db_handles
{
    SRV0020_SVC_IDX_RX_CHAR,
    SRV0020_SVC_IDX_RX_VAL,
    SRV0020_SVC_IDX_TX_CHAR,
    SRV0020_SVC_IDX_TX_VAL,
    SRV0020_SVC_IDX_TX_NTF_CFG,
    SRV0020_SVC_ATT_NUM
};
extern uint8_t name_adv[ADV_NAME_LEN +8];
//extern uint8_t     srv0020_start_handle;
extern srv0020_data_info_t  srv0020;
	
extern uint8_t 		  adv_obj_hdl;


extern const struct att_decl ls_srv0020_server_att_decl[SRV0020_SVC_ATT_NUM];
extern const struct svc_decl ls_srv0020_server_svc;

void start_adv(void);
void ls_srv0020_server_read_req_ind(uint8_t att_idx, uint8_t con_idx);
#endif
