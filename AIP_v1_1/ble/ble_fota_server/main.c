#define LOG_TAG "MAIN"
#include "main.h"
#include "ls_ble.h"
#include "platform.h"
#include "prf_diss.h"
#include "log.h"
#include "ls_dbg.h"
#include "cpu.h"

#include <string.h>
#include "co_math.h"
#include "ls_soc_gpio.h"
#include "fota_svc_server.h"
#include "SEGGER_RTT.h"
#include "ble_base_set.h"
#include "timer_event.h"
#include "APP_io_init.h"
#include "uart_base.h"
#include "g.h"
#include "app_led_ctrl.h"
#include "stdio.h"
#include "ls_hal_trng.h"
#include "x_drive.h"
uint8_t recv_flag;

#define UART_SVC_ADV_NAME "LS Uart Server OTA svc"
#define UART_SERVER_MTU_DFT  200 /* 协商前本地假定值；ATT 单包有效载荷≈MTU-3，≥~160B 业务域时建议 APP 协商到 200 */
#define UART_SERVER_MAX_DATA_LEN (uart_server_mtu - 3)
#define UART_SVC_RX_MAX_LEN (USER_MAX_MTU - 3)
#define UART_SVC_TX_MAX_LEN (USER_MAX_MTU - 3)





//#define UART_SERVER_TIMEOUT 50 // timer units: ms

static uint8_t connect_id = 0xff; 
static bool uart_server_ntf_done = true;
static uint16_t uart_server_mtu = UART_SERVER_MTU_DFT;

//static uint8_t advertising_data[28] = {0x05, 0x09, 'u', 'a', 'r', 't'};
//static uint8_t scan_response_data[31];
//static void ls_uart_server_read_req_ind(uint8_t att_idx, uint8_t con_idx);

uint8_t ls_bleup_server_send_notification(uint8_t  *data_notice,uint16_t length);
static void ls_srv0020_server_data_length_update(uint8_t con_idx);

uint8_t ls_bleup_server_send_notification(uint8_t  *data_notice,uint16_t length)
{
	if(connect_id != CON_IDX_INVALID_VAL && length != 0 && uart_server_ntf_done)
	{
			uint32_t cpu_stat = enter_critical();
			uart_server_ntf_done = false;
			uint16_t handle = gatt_manager_get_svc_att_handle(&ls_uart_server_svc_env, SRV0020_SVC_IDX_TX_VAL);
			gatt_manager_server_send_notification(connect_id, handle, &data_notice[0], length, NULL);         
			srv0020.tx.plength = 0;	
			exit_critical(cpu_stat);
			return 1;
	}
	else
	{
			return 0;
	}   
}

static void ls_srv0020_server_data_length_update(uint8_t con_idx)
{
    struct gap_set_pkt_size dlu_param = 
    {
        .pkt_size = 251,
    };
    gap_manager_set_pkt_size(con_idx, &dlu_param);
}

static void get_dev_name(struct gap_dev_info_dev_name *dev_name_ptr, uint8_t con_idx)
{
    LS_ASSERT(dev_name_ptr);
    dev_name_ptr->value = (uint8_t*)UART_SVC_ADV_NAME;
    dev_name_ptr->length = sizeof(UART_SVC_ADV_NAME) - 1;
}
static void get_appearance(struct gap_dev_info_appearance *dev_appearance_ptr, uint8_t con_idx)
{
    LS_ASSERT(dev_appearance_ptr);
    dev_appearance_ptr->appearance = 0;
}
static void get_slv_pref_param(struct gap_dev_info_slave_pref_param *dev_slv_pref_param_ptr, uint8_t con_idx)
{
    LS_ASSERT(dev_slv_pref_param_ptr);
    dev_slv_pref_param_ptr->con_intv_min  = 8;
    dev_slv_pref_param_ptr->con_intv_max  = 20;
    dev_slv_pref_param_ptr->slave_latency =  0;
    dev_slv_pref_param_ptr->conn_timeout  = 600;  /* 监督超时 600×10ms=6s（原 200=2s 过短易断连） */
}

static void gap_manager_callback(enum gap_evt_type type,union gap_evt_u *evt,uint8_t con_idx)
{
    switch(type)
    {
    case CONNECTED:
        connect_id = con_idx;
			g_sysparam_st.ble.ble_connect = 1;
        LOG_I("BLE connected");
				if(g_sysparam_st.ble.ble_pair_flag && g_sysparam_st.ble.ble_pair_time > 0) {
					led_bt_pairing(true);
				}
    break;
    case DISCONNECTED:
        connect_id = 0xff;
        uart_server_mtu = UART_SERVER_MTU_DFT;
        LOG_I("BLE disconnected");
        fota_clean_state();
			g_sysparam_st.ble.ble_connect = 0;
			/* ble_pair_time==3000: 30s pair window ended, otherwise restart adv */
			if(g_sysparam_st.ble.ble_pair_time != 3000 )
			{
					start_adv();
					if(g_sysparam_st.ble.ble_pair_flag && g_sysparam_st.ble.ble_pair_time > 0) {
						led_bt_pairing(false);
					}
			}
    break;
    case CONN_PARAM_REQ:
    break;
    case CONN_PARAM_UPDATED:

    break;
    case GET_DEV_INFO_DEV_NAME:
        get_dev_name((struct gap_dev_info_dev_name*)evt, con_idx);
    break;
    case GET_DEV_INFO_APPEARANCE:
        get_appearance((struct gap_dev_info_appearance*)evt, con_idx);
    break;
    case GET_DEV_INFO_SLV_PRE_PARAM:
        get_slv_pref_param((struct gap_dev_info_slave_pref_param*)evt, con_idx);
    break;
    default:

    break;
    }
}

static void gatt_manager_callback(enum gatt_evt_type type,union gatt_evt_u *evt,uint8_t con_idx)
{
    switch (type)
    {
    case SERVER_READ_REQ:
        LOG_I("GATT read req");
        if (evt->server_read_req.svc == &ls_uart_server_svc_env)
        {
            ls_srv0020_server_read_req_ind(evt->server_read_req.att_idx, con_idx);
        }
        else
        {
            ls_ota_server_read_req_ind(evt->server_read_req.att_idx, con_idx);
        }
    break;
    case SERVER_WRITE_REQ:
        if (evt->server_write_req.svc == &ls_uart_server_svc_env)
        {
            /* 最短链路帧如 3.3.3 查询 02 06 10 B6 仅 4 字节，故下限为 2（帧长+端口），非 5 */
            if (evt->server_write_req.length <= 50U && evt->server_write_req.length >= 2U) {
                memcpy(&ble_data_receive.ble_data_receive[0], &evt->server_write_req.value[0], evt->server_write_req.length);
                ble_data_receive.lengh = evt->server_write_req.length;
                x_ble_com_handle((uint8_t *)&evt->server_write_req.value[0], evt->server_write_req.length);
            } else {
                LOG_I("GATT write skip len=%u (need 2..50)", (unsigned)evt->server_write_req.length);
            }
        }
        else
        {
            ls_ota_server_write_req_ind(&evt->server_write_req, con_idx);
        }
    break;
    case SERVER_NOTIFICATION_DONE:
        uart_server_ntf_done = true;
    break;
    case SERVER_INDICATION_DONE:
        LOG_I("GATT indication done");
    break;
    case MTU_CHANGED_INDICATION:
        uart_server_mtu = evt->mtu_changed_ind.mtu;
        LOG_I("MTU=%d", uart_server_mtu);
        ls_srv0020_server_data_length_update(con_idx);
    break;
    default:
        LOG_I("GATT unhandled type=%d", type);
        break;
    }
}

/* Connectable + scannable adv, interval 0x20*0.625ms on ch37/38/39 */
static void create_adv_obj()
{
    struct legacy_adv_obj_param adv_param = {
        .adv_intv_min = 0x20,
        .adv_intv_max = 0x20,
        .own_addr_type = PUBLIC_OR_RANDOM_STATIC_ADDR,
        .filter_policy = 0,
        .ch_map = 0x7,
        .disc_mode = ADV_MODE_GEN_DISC,
        .prop = {
            .connectable = 1,
            .scannable = 1,
            .directed = 0,
            .high_duty_cycle = 0,
        },
    };
    dev_manager_create_legacy_adv_object(&adv_param);
}
static void dev_manager_callback(enum dev_evt_type type,union dev_evt_u *evt)
{
    switch(type)
    {
    case STACK_INIT:
    {
        struct ble_stack_cfg cfg = {
            .private_addr = false,
            .controller_privacy = false,
        };
        dev_manager_stack_init(&cfg);
    }
    break;
    case STACK_READY:
    {
        uint8_t addr[6];
				bool type;
				x_uart_init();
        dev_manager_get_identity_bdaddr(addr,&type);
			  LOG_I("BLE addr type=%d",type);
        LOG_HEX(addr,sizeof(addr));
        dev_manager_add_service((struct svc_decl *)&ls_srv0020_server_svc);
				ls_even_timer_init();
    }
    break;
    case SERVICE_ADDED:
    {
        static uint8_t svc_added_flag = 0;
        LS_ASSERT(svc_added_flag < 2);
        if (0 == svc_added_flag)
        {
            gatt_manager_svc_register(evt->service_added.start_hdl, SRV0020_SVC_ATT_NUM, &ls_uart_server_svc_env);
            fotas_add_service();
            svc_added_flag++;
        }
        else if (1 == svc_added_flag)
        {
            fotas_register_svc(evt->service_added.start_hdl);
            create_adv_obj();
        }
    }
    break;
    case ADV_OBJ_CREATED:
        LS_ASSERT(evt->obj_created.status == 0);
        adv_obj_hdl = evt->obj_created.handle;
    break;
    case ADV_STOPPED:
    break;
    case SCAN_STOPPED:
    break;
    default:

    break;
    }
    
}

void delay_ms(uint32_t ms)
{
    volatile uint32_t count;
    while(ms--)
    {
        count = 8000;  
        while(count--);
    }
}

int main()
{
    sys_init_app();
	x_driver_init();
	led_ctrl_init();
    ble_init();
    dev_manager_init(dev_manager_callback);
    gap_manager_init(gap_manager_callback);
    gatt_manager_init(gatt_manager_callback);
	flash_load();
	SnoringInterventionInit();
	ble_loop();
}

void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */

    /* USER CODE END Error_Handler_Debug */
}
/**
  * @brief  Conversion complete callback in non blocking mode 
  * @param  hadc: ADC handle
  * @retval None
  */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	(void)hadc;
	recv_flag = 1;
}
