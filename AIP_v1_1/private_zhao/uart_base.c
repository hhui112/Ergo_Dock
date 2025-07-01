#include "uart_base.h"
#include "ble_base_set.h"
#include "timer_event.h"
#include "qs_protobuf.h"
#include "g.h"

#define  bochuang_s  1  //0:����1���ڲ�������������ͨ��  1������1��ʱ����˯�˼�⴮�ڴ�ӡ��ѹ���ݺ����ҿ���

extern  airbag_ctrbase_def airbag_ctrbase;
extern  airbag_parameter_def *airbagsetfile;
extern  uint16_t airpreee_set;
extern  uint8_t pump_PWMset;


static bool uart_server_tx_busy;

uint8_t uart_server_rx_byte;

UART_HandleTypeDef UART_Server_Config; 
bool update_adv_intv_flag = false;

uart_def uart1_data;
static uint8_t uartindexsave=0;
static struct builtin_timer *uart_rx_timerover_inst = NULL;//��ʱ

void x_uart_Internalparameterprint(void);


static void ls_uart_5ms_timer_cb(void *param)//��ʱ�ص�����
{
	uint16_t CRC16,data16,pa_t,pa_t2,mode_t;
	 if((uartindexsave==uart1_data.uart_rx.index)&&(uart1_data.uart_rx.index>4))
	 {
		
		 if(uart1_data.uart_rx.buf[0]==0X5A)//�ж�ͷ
		 {
			 CRC16=Modbus_Crc_Compute(&uart1_data.uart_rx.buf[0], uart1_data.uart_rx.buf[1]+2);//CRCУ��
			 //LOG_I("CRC16: %d",CRC16); 
				data16=uart1_data.uart_rx.buf[uart1_data.uart_rx.buf[1]+2+1];
				data16=(data16<<8)|uart1_data.uart_rx.buf[uart1_data.uart_rx.buf[1]+2];
			  if(CRC16==data16)//CRCУ���ж�
				{
					//LOG_I("snore_n: %d",airbagsetfile->snore_n); 
					
				}
		 }

		 ///////////////////////////////////////
		 uart1_data.uart_rx.index=0;
	  //uartindexsave=uart1_data.uart_rx.index;
	 }
	 
	 
	 uartindexsave=uart1_data.uart_rx.index;//�����������ȱ���  �����´��жϴ��ڽ���ʱ�����
    if(uart_rx_timerover_inst)//��ʱ��λ
    {
        builtin_timer_start(uart_rx_timerover_inst, uart_rxtime_over, NULL); 
    }
}

void ls_uarttime_server_init(void)//����һ����ʱ��������ʱ��ѯ�Ƿ�������ͨ�������ϱ�  
{
    uart_rx_timerover_inst = builtin_timer_create(ls_uart_5ms_timer_cb);
    builtin_timer_start(uart_rx_timerover_inst, uart_rxtime_over, NULL);//��ʱʱ������  ��λms
}

void ls_uart_init(void)//���ڳ�ʼ��
{
    pinmux_uart1_init(PB00, PB01);//PB00=TX  PB01=RX
    io_pull_write(PB01, IO_PULL_UP);
    UART_Server_Config.UARTX = UART1;
    UART_Server_Config.Init.BaudRate = UART_BAUDRATE_115200;//UART_BAUDRATE_38400  UART_BAUDRATE_115200
    UART_Server_Config.Init.MSBEN = 0;
    UART_Server_Config.Init.Parity = UART_NOPARITY;
    UART_Server_Config.Init.StopBits = UART_STOPBITS1;
    UART_Server_Config.Init.WordLength = UART_BYTESIZE8;
    HAL_UART_Init(&UART_Server_Config);
		//���ڻ����ʼ��	
		uart1_data.uart_rx.index=0;
		uart1_data.uart_tx.index=0;
}

//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)//���ڽ��ջص�����
//{
//	if(uart1_data.uart_rx.index<uart1_datasize)
//	{
//		uart1_data.uart_rx.buf[uart1_data.uart_rx.index++]=uart_server_rx_byte;//�������ݴ�ŵ�buf��
//		//LOG_I("uart rx !");
//	}
//	else
//	{
//		LOG_I("uart server rx buffer overflow!");
//		uart1_data.uart_rx.index=0;
//	}
//	HAL_UART_Receive_IT(&UART_Server_Config, &uart_server_rx_byte, 1);//���ô˺��������ڽ���ʹ�ܣ�ÿ�ν���1byte����ŵ�uart_server_rx_byte

//}



void ls_uart_tx_send(ble_data_init *p)//������������͸��������
{
    
        if(uart_server_tx_busy)
        {
            LOG_I("Uart tx busy, data discard!");
        }
        else
        {
            uart_server_tx_busy = true;
					  uart1_data.uart_tx.index=p->lengh;
            memcpy(&uart1_data.uart_tx.buf[0], &p->ble_data_receive[0], p->lengh);					
            HAL_UART_Transmit_IT(&UART_Server_Config, &uart1_data.uart_tx.buf[0], uart1_data.uart_tx.index);
        } 

}



