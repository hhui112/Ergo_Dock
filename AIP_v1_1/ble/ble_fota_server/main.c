#define LOG_TAG "MAIN"
#include "main.h"
#include "ls_ble.h"
#include "platform.h"
#include "prf_diss.h"
#include "log.h"
#include "ls_dbg.h"
#include "cpu.h"

#include "builtin_timer.h"
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
#include "app_led_ctrl.h"  // 新的LED控制模块
#include "stdio.h"
#include "ls_hal_trng.h"
#include "x_drive.h"
uint8_t recv_flag;//static volatile

#define UART_SVC_ADV_NAME "LS Uart Server OTA svc"
#define UART_SERVER_MTU_DFT  100
#define UART_SERVER_MAX_DATA_LEN (uart_server_mtu - 3)
#define UART_SVC_RX_MAX_LEN (USER_MAX_MTU - 3)
#define UART_SVC_TX_MAX_LEN (USER_MAX_MTU - 3)





//#define UART_SERVER_TIMEOUT 50 // timer units: ms

static uint8_t connect_id = 0xff; 
static bool uart_server_ntf_done = true;
static uint16_t uart_server_mtu = UART_SERVER_MTU_DFT;
static struct builtin_timer *uart_server_timer_inst = NULL;//����ʱ

//static uint8_t advertising_data[28] = {0x05, 0x09, 'u', 'a', 'r', 't'};
//static uint8_t scan_response_data[31];
//static void ls_uart_server_read_req_ind(uint8_t att_idx, uint8_t con_idx);

uint8_t ls_bleup_server_send_notification(uint8_t  *data_notice,uint16_t length);
static void ls_srv0020_server_data_length_update(uint8_t con_idx);

static void ls_ble_dataup_50ms_timer_cb(void *param)//����ʱ�ص�����
{
	static uint16_t count;
	
	count++;
	
	if(count >= 10)
	{
		count = 0;
		x_ble_com_txbuffFill();
		ls_bleup_server_send_notification(bletxbuff,bletxlen);//��ѯ�Ƿ����������з���
	}
		
	if(uart_server_timer_inst)//����ʱ��λ
	{
			builtin_timer_start(uart_server_timer_inst, 50, NULL); 
	}
}

static void ls_time_server_init(void)//����һ������ʱ��������ʱ��ѯ�Ƿ�������ͨ�������ϱ�  
{
    uart_server_timer_inst = builtin_timer_create(ls_ble_dataup_50ms_timer_cb);
    builtin_timer_start(uart_server_timer_inst, 50, NULL);//����ʱʱ������50ms  ��λms
}



uint8_t ls_bleup_server_send_notification(uint8_t  *data_notice,uint16_t length)//������������ͨ��
{
	
	
	if(connect_id != CON_IDX_INVALID_VAL && length != 0 && uart_server_ntf_done)
	{
			uint32_t cpu_stat = enter_critical();//ϵͳ����������
			uart_server_ntf_done = false;
			uint16_t handle = gatt_manager_get_svc_att_handle(&ls_uart_server_svc_env, SRV0020_SVC_IDX_TX_VAL);
			gatt_manager_server_send_notification(connect_id, handle, &data_notice[0], length, NULL);         
			srv0020.tx.plength = 0;	
			exit_critical(cpu_stat);//ϵͳ����������
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
    dev_slv_pref_param_ptr->conn_timeout  = 200;
}

static void gap_manager_callback(enum gap_evt_type type,union gap_evt_u *evt,uint8_t con_idx)
{
    switch(type)
    {
    case CONNECTED:
        connect_id = con_idx;
			g_sysparam_st.ble.ble_connect = 1;
        LOG_I("connected!");
				// 如果在配对期间连接成功，切换LED为常亮
				if(g_sysparam_st.ble.ble_pair_flag && g_sysparam_st.ble.ble_pair_time > 0) {
					led_bt_pairing(true);  // 新的LED控制：配对成功（常亮）
				}
    break;
    case DISCONNECTED:
        connect_id = 0xff;
        uart_server_mtu = UART_SERVER_MTU_DFT;
        LOG_I("disconnected!");
        fota_clean_state();
			g_sysparam_st.ble.ble_connect = 0;
			if(g_sysparam_st.ble.ble_pair_time != 3000 )// �����30s��Թ����жϿ�������㲥
			{
					start_adv();
					// 如果还在配对期间，恢复闪烁状态
					if(g_sysparam_st.ble.ble_pair_flag && g_sysparam_st.ble.ble_pair_time > 0) {
						led_bt_pairing(false);  // 新的LED控制：配对中（闪烁）
					}
			}
        // start_adv();  // �����Զ��㲥
    break;
    case CONN_PARAM_REQ:
        //LOG_I
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
        LOG_I("read req");
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
        //LOG_I("write req");
        if (evt->server_write_req.svc == &ls_uart_server_svc_env)	// ����͸������
        {
           // ls_uart_server_write_req_ind(evt->server_write_req.att_idx, con_idx, evt->server_write_req.length, evt->server_write_req.value);//������������͸��������
					LOG_I("srv0020");
							if(evt->server_write_req.length<=50&&evt->server_write_req.length>=5)
							{
								LOG_I("APP_DATA");
								memcpy(&ble_data_receive.ble_data_receive[0],&evt->server_write_req.value[0],evt->server_write_req.length);
								ble_data_receive.lengh=evt->server_write_req.length;
								x_ble_com_handle((uint8_t*)&evt->server_write_req.value[0],evt->server_write_req.length);
							}
        }
        else
        {
            ls_ota_server_write_req_ind(&evt->server_write_req, con_idx);	// ota����
        }
    break;
    case SERVER_NOTIFICATION_DONE:
        uart_server_ntf_done = true;
        // LOG_I("ntf done");
    break;
    case SERVER_INDICATION_DONE:
        LOG_I("ind done");
    break;
    case MTU_CHANGED_INDICATION:
        uart_server_mtu = evt->mtu_changed_ind.mtu;
        LOG_I("mtu: %d", uart_server_mtu);
        ls_srv0020_server_data_length_update(con_idx);
    break;
    default:
        LOG_I("Event not handled! %d", type);
        break;
    }
}

static void create_adv_obj()
{
    struct legacy_adv_obj_param adv_param = {
        .adv_intv_min = 0x20,//�㲥������С����  ��λ625us  ������Сֵһ������ͬһ��ֵ
        .adv_intv_max = 0x20,//�㲥�����������  ��λ625us  
        .own_addr_type = PUBLIC_OR_RANDOM_STATIC_ADDR,
        .filter_policy = 0,
        .ch_map = 0x7,//����ÿ��㲥���ĸ�����Ĭ��Ϊ7����ʾ37/38/39��3��channel�϶��ᷢ��
        .disc_mode = ADV_MODE_GEN_DISC,//ͨ�ù㲥
        .prop = {
            .connectable = 1,//����Ϊ1������Ϊ���������㲥���������޷���������
            .scannable = 1,//��ɨ��
            .directed = 0,//�Ƕ���
            .high_duty_cycle = 0,
        },
    };
    dev_manager_create_legacy_adv_object(&adv_param);
}
//static void start_adv(void)
//{
//    LS_ASSERT(adv_obj_hdl != 0xff);
//    uint8_t adv_data_length = ADV_DATA_PACK(advertising_data, 1, GAP_ADV_TYPE_SHORTENED_NAME, UART_SVC_ADV_NAME, sizeof(UART_SVC_ADV_NAME));
//    dev_manager_start_adv(adv_obj_hdl, advertising_data, adv_data_length, scan_response_data, 0);
//    LOG_I("adv start");
//}
/*
static void create_highduty_adv_obj(void)
{
    struct legacy_adv_obj_param adv_param = {
        .peer_addr = (struct dev_addr*)peer_addr_1,
        .peer_addr_type = RANDOM_ADDR,
        .adv_intv_min = 0x20,
        .adv_intv_max = 0x20,
        .own_addr_type = PUBLIC_OR_RANDOM_STATIC_ADDR,
        .filter_policy = 0,
        .ch_map = 0x7,
        .disc_mode = ADV_MODE_NON_DISC,
        .prop = {
            .connectable = 1,
            .scannable = 0,
            .directed = 1,
            .high_duty_cycle = 1,
        },
    };
    dev_manager_create_legacy_adv_object(&adv_param);
}*/

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
				x_uart_init();  	// ��ʼ������
        dev_manager_get_identity_bdaddr(addr,&type);
			  LOG_I("type:%d,addr:",type);
        LOG_HEX(addr,sizeof(addr));
        dev_manager_add_service((struct svc_decl *)&ls_srv0020_server_svc);	//���ӷ����ȵ��øú�����Ȼ����SERVICE_ADDED���ȵ���gatt_manager_svc_register(evt->service_added.start_hdl, UART_SVC_ATT_NUM, &ls_uart_server_svc_env);
       // HAL_UART_Receive_IT(&UART_Server_Config, &uart_server_rx_byte, 1); //���ڽ���ʹ�ܣ�ÿ�ν���1byte����ŵ�uart_server_rx_byte
        ls_time_server_init();   //main��50ms����ʱ��ʼ��       	
				ls_even_timer_init();//user ����ʱ��ʼ��				
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
            create_adv_obj();//�����㲥����
        }
    }
    break;
    case ADV_OBJ_CREATED:
        LS_ASSERT(evt->obj_created.status == 0);
        adv_obj_hdl = evt->obj_created.handle;
        // start_adv();  // ��ʼ�����ڹ㲥
    break;
    case ADV_STOPPED:
        // start_adv();  // ֹͣ�㲥�������¹㲥
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
    sys_init_app();//ϵͳ��ʼ��
		x_driver_init();
		led_ctrl_init();  // 初始化LED控制模块
    ble_init();//ble��ʼ��
    dev_manager_init(dev_manager_callback);//�豸������ʼ����ע��ص�
    gap_manager_init(gap_manager_callback);//gap��ʼ����ص�ע��
    gatt_manager_init(gatt_manager_callback);//gatt��ʼ����ص�ע��
		SnoringInterventionInit();
		ble_loop();//ѭ������ble�¼�

	
	/*
		x_uart_init(); 
		while(1)
		{
		
			x_uart_10ms();
			delay_ms(100);
		}
*/
	
	/*
		x_control_init();
		judgerInit();
		HAL_TRNG_Init();
		x_SnoringInterventionInit();
		adaptivecontrol_default();
		x_flash_load();
		x_ai_Init();
    ble_loop();//ѭ������ble�¼�
	*/
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
  //conver_flag = CONVER_COMPLETED;
	recv_flag = 1;
}
