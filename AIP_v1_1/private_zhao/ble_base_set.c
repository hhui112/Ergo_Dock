#include "ble_base_set.h"
#include "log.h"
#include <string.h>
#include "hal_flash_int.h"

uint8_t name_adv[ADV_NAME_LEN +8];
uint8_t adv_obj_hdl;
uint8_t advertising_data[28];
uint8_t scan_response_data[31];

struct gatt_svc_env ls_uart_server_svc_env;
static uint16_t cccd_config = 0;
//uint8_t               srv0020_start_handle;
srv0020_data_info_t  srv0020;
static const uint8_t ls_srv0020_svc_uuid_128[] =     {0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x20,0x00,0x40,0x6e};
static const uint8_t ls_srv0020_rx_char_uuid_128[] = {0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x21,0x00,0x40,0x6e};
static const uint8_t ls_srv0020_tx_char_uuid_128[] = {0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x22,0x00,0x40,0x6e};
static const uint8_t att_decl_char_array[] = {0x03,0x28};
static const uint8_t att_desc_client_char_cfg_array[] = {0x02,0x29};

const struct att_decl ls_srv0020_server_att_decl[SRV0020_SVC_ATT_NUM] =  //添加服务 主要包括服务定义、服务内部的特征值、描述符定义
{
    [SRV0020_SVC_IDX_RX_CHAR] = {
        .uuid = att_decl_char_array,
        .s.max_len = 0,
        .s.uuid_len = UUID_LEN_16BIT,
        .s.read_indication = 1,   
        .char_prop.rd_en = 1,
    },
    [SRV0020_SVC_IDX_RX_VAL] = {
        .uuid = ls_srv0020_rx_char_uuid_128,
        .s.max_len = UART_SVC_RX_MAX_LEN,
        .s.uuid_len = UUID_LEN_128BIT,
        .s.read_indication = 1,
        .char_prop.wr_cmd = 1,
        .char_prop.wr_req = 1,
    },
    [SRV0020_SVC_IDX_TX_CHAR] = {
        .uuid = att_decl_char_array,
        .s.max_len = 0,
        .s.uuid_len = UUID_LEN_16BIT,
        .s.read_indication = 1,
        .char_prop.rd_en = 1, 
    },
    [SRV0020_SVC_IDX_TX_VAL] = {
        .uuid = ls_srv0020_tx_char_uuid_128,
        .s.max_len = UART_SVC_TX_MAX_LEN,
        .s.uuid_len = UUID_LEN_128BIT,
        .s.read_indication = 1,
        .char_prop.ntf_en = 1,
    },
    [SRV0020_SVC_IDX_TX_NTF_CFG] = {
        .uuid = att_desc_client_char_cfg_array,
        .s.max_len = 0,
        .s.uuid_len = UUID_LEN_16BIT,
        .s.read_indication = 1,
        .char_prop.rd_en = 1,
        .char_prop.wr_req = 1,
    },
};
const struct svc_decl ls_srv0020_server_svc =
{
    .uuid = ls_srv0020_svc_uuid_128,
    .att = (struct att_decl*)ls_srv0020_server_att_decl,
    .nb_att = SRV0020_SVC_ATT_NUM,
    .uuid_len = UUID_LEN_128BIT,
};

void ls_srv0020_server_read_req_ind(uint8_t att_idx, uint8_t con_idx)
{
    uint16_t handle = 0;
    if(att_idx == SRV0020_SVC_IDX_TX_NTF_CFG)
    {
        handle = gatt_manager_get_svc_att_handle(&ls_uart_server_svc_env, att_idx);
        gatt_manager_server_read_req_reply(con_idx, handle, 0, (void*)&cccd_config, 2);
    }
}

void start_adv(void)//开启广播
{
		uint32_t serial_num=*(uint32_t *)0x1807BF00;
		LOG_I("SERIAL_NUM[%d]",serial_num);
	  memcpy(&name_adv[0],ADV_NAME,ADV_NAME_LEN);
	  name_adv[ADV_NAME_LEN ]=serial_num/ 10000000%10+'0';
		name_adv[ADV_NAME_LEN + 1]=serial_num/1000000%10+'0';
		name_adv[ADV_NAME_LEN + 2]=serial_num/100000%10+'0';
		name_adv[ADV_NAME_LEN + 3]=serial_num/10000%10+'0';
		name_adv[ADV_NAME_LEN + 4]=serial_num/1000%10+'0';
		name_adv[ADV_NAME_LEN + 5]=serial_num/100%10+'0';
		name_adv[ADV_NAME_LEN + 6]=serial_num/10%10+'0';
		name_adv[ADV_NAME_LEN + 7]=serial_num%10+'0';
	
	  LS_ASSERT(adv_obj_hdl != 0xff);
    uint8_t adv_data_length = ADV_DATA_PACK(advertising_data, 1, GAP_ADV_TYPE_SHORTENED_NAME, name_adv, sizeof(name_adv));
    dev_manager_start_adv(adv_obj_hdl, advertising_data, adv_data_length, scan_response_data, 0);
		LOG_I("BLE: advertising start");
}

void stop_adv(void)
{
    LS_ASSERT(adv_obj_hdl != 0xff);
    dev_manager_stop_adv(adv_obj_hdl);
		LOG_I("BLE: advertising stopped");
}

