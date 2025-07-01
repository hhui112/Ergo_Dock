#include "timer_event.h"
//#include "lsiwdg.h"
#include "ls_hal_iwdg.h"
#include "main.h"
#include "key_check.h"
#include "ble_base_set.h"
#include "hal_flash_int.h"
#include "APP_io_init.h"
#include "uart_base.h"
#include<math.h>
#include "WF_i2c.h"
#include "wfsensor.h"
#include "sleep_posture.h"
#include "heat_control.h"
#include "g.h"






#define BLE_SEND_SYN_LEN  41//��������ͬ�����ݳ���
#define BLE_AIRBAG_SEND_SYN_LEN 11//���ҿ�����������ͬ�����ݳ���
#define temp_default  42//Ĭ�������¶�  ��λ��
#define time_default  20*60//Ĭ�϶�ʱ  ��λ��
#define protect_I  2000  //����������������



#define snore_timevoer_def 5*60 //�����ж�5�������������ж�Ϊ������



sleepposture_def  sleepposture;
airbag_ctrbase_def airbag_ctrbase;

uint16_t airpreee_LD=200;//�Զ�����Ŀ����ѹ�����
uint16_t airpreee_HD=200;//�Զ�����Ŀ����ѹ�����
uint16_t airpreee_set=2000;//���Ʊ�������Ŀ����ѹ
uint8_t pump_PWMset=255;

//uint8_t vidoehead[32]={0};
//uint16_t length_video1,length_video_i,add_video1;
uint16_t snore_timevoer=snore_timevoer_def;//ֹͣ������ʱ��ʱ��

ble_data_init ble_data_receive;//���������������ݻ���
extern  uint8_t recv_flag;
extern  ADC_HandleTypeDef hadc;


static void   				ls_5ms_timer_cb(void *param);
struct builtin_timer 	*xTimer_5ms_inst = NULL;


static void   				ls_10ms_timer_cb(void *param);
struct builtin_timer 	*xTimer_10ms_inst = NULL;


static void   				ls_100ms_timer_cb(void *param);
struct builtin_timer 	*xTimer_100ms_inst = NULL;

static void   				ls_1000ms_timer_cb(void *param);
struct builtin_timer 	*xTimer_1000ms_inst = NULL;


unsigned  char set_parameter[100]=
{
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,
};


//state_flag motor_state;

airbag_parameter_def *airbagsetfile=(airbag_parameter_def *)(&set_parameter[50]);//���ҿ��ƹ��������ļ���Ҳ��APP�����ļ�  50���ֽ�






void ls_even_timer_init(void)
{
	/*
	xTimer_5ms_inst = builtin_timer_create(ls_5ms_timer_cb);
	builtin_timer_start(xTimer_5ms_inst, 5, NULL);
*/
	xTimer_10ms_inst = builtin_timer_create(ls_10ms_timer_cb);
	builtin_timer_start(xTimer_10ms_inst, 10, NULL);	
/*
	xTimer_100ms_inst = builtin_timer_create(ls_100ms_timer_cb);
	builtin_timer_start(xTimer_100ms_inst, 100, NULL);
	
	xTimer_1000ms_inst = builtin_timer_create(ls_1000ms_timer_cb);
	builtin_timer_start(xTimer_1000ms_inst, 1000, NULL);
	*/
	
}



void parameter_init(void)//������ʼ��
{
	
}
//���ҿ��ƺ���,10ms����һ�Σ��Կ���״̬����
void airbag_ctr(void)
{

	static uint8_t valve_pwmnow=0;
	
	switch(airbag_ctrbase)
	{
		case air_outstop://ֹͣ����
			          
								airbagsetfile->flag &=~0X02;//ֹͣ������־
		            airbagsetfile->flag &=~0X01;//�رճ�����־
								if(LSGPTIMA->CCR1>0)
								{
									valve_pwmnow=LSGPTIMA->CCR1;
									if(valve_pwmnow>160)//PWMֵ����160ʱ��PWMֱֵ������Ϊ160��Ȼ���160�𽥱�С
										valve_pwmnow=160;
									airbag_ctrbase=base_state1;
								}
								else
									airbag_ctrbase=base_state2;
								break;
		case base_state1://����PWMֵ�𽥱�С��ֱ��0
								if(valve_pwmnow>0)
									valve_pwmnow --;
								x_valve_pwmset(valve_pwmnow);
								if(valve_pwmnow==0)
								{
									airbag_ctrbase=base_state2;

								  airbagsetfile->flag &=~0X02;//ֹͣ������־
									//airbagsetfile->flag &=~0X01;//�رճ�����־
								}
								break;
		case base_state2://�����رս���
			         break;
		case air_outstart://��ʼ����

								airbagsetfile->flag |=0X02;//��ʼ������־
		            airbagsetfile->flag &=~0X01;//�رճ�����־
								if(LSGPTIMA->CCR1<255)
								{
									valve_pwmnow=LSGPTIMA->CCR1;
									if(valve_pwmnow<128)
										valve_pwmnow=128;
									airbag_ctrbase=base_state3;
								}
								else//�����Ѿ���ȫ��
								{
									airbag_ctrbase=air_outing;
									airbagsetfile->flag |=0X02;//��ʼ������־
								}
								break;
		case base_state3:
								if(valve_pwmnow<255)
									valve_pwmnow++;
								x_valve_pwmset(valve_pwmnow);
								if(valve_pwmnow==255)
								{
									airbag_ctrbase=air_outing;

									airbagsetfile->flag |=0X02;//��ʼ������־
								}
								break;
		case air_outing://�����򿪽���		
		           break;
		case air_inputstart://��ʼ����


								if(LSGPTIMA->CCR1>0)
								{
									valve_pwmnow=LSGPTIMA->CCR1;
									if(valve_pwmnow>160)
										valve_pwmnow=160;
									airbag_ctrbase=base_state5;
								}
								else
									airbag_ctrbase=base_state6;
								break;
		case base_state5://����PWMֵ�𽥱�С��ֱ��0
								if(valve_pwmnow>0)
									valve_pwmnow--;
								x_valve_pwmset(valve_pwmnow);
								if(valve_pwmnow==0)
								{
									airbag_ctrbase=base_state6;
								  //airbagsetfile->flag |=0X02;//�򿪷�����־
									airbagsetfile->flag &=~0X02;//ֹͣ������־
								}
								break;
		case base_state6://�����رս���

		            airbagsetfile->flag |=0X01;//�򿪳�����־
								airbag_ctrbase=air_inputing;

								break;
		case air_inputing://��ʼ�����������
		            break;
		case air_inputstop://ֹͣ����


		            airbagsetfile->flag &=~0X01;//�رճ�����־
		            if(LSGPTIMA->CCR1>0)
								{
									valve_pwmnow=LSGPTIMA->CCR1;
									if(valve_pwmnow>160)
										valve_pwmnow=160;
									airbag_ctrbase=base_state8;
								}
								else
									airbag_ctrbase=base_state9;
			          break;
		case base_state8:
			         if(valve_pwmnow>0)
									valve_pwmnow--;
								x_valve_pwmset(valve_pwmnow);
								if(valve_pwmnow==0)
								{
									airbag_ctrbase=base_state9;

									airbagsetfile->flag &=~0X02;//ֹͣ������־
								}
								break;		
		case base_state9://ֹͣ�����������	
		            break;														
		default:

		            x_valve_pwmset(0);
		            airbagsetfile->flag &=~0X01;//�رճ�����־
		            airbagsetfile->flag &=~0X02;//ֹͣ������־
		            airbag_ctrbase=base_state9;
		            break;
	}
}



static void ls_5ms_timer_cb(void *param)
{
//	switch(GetKey())//���ذ���ɨ��
//	{
//		case 0X81:
//			if(g_sysparam_st.timer>500)
//			{
//				airbagsetfile->snore_n++;
//				snore_timevoer=0;
//			}

//			break;
//		default:break;
//	}
	
	OKIN_ai_run();
	x_flash_run();
	x_SnoringIntervention_run();
 if(xTimer_5ms_inst)
 {
    builtin_timer_start(xTimer_5ms_inst, 5, NULL); 
  }
}
void pump_control(void)
{
	static uint8_t opentime=0; // ��30s ͣ10s

	if((airbagsetfile->flag&0X01)==0x01)//���ô򿪱�־
	{
//		if(opentime==0)
//		{
//		  x_pump_pwmset(pump_PWMset);//�򿪳�����
//		}
//		else if(opentime==30)
//		{
//			x_pump_pwmset(0);//�ر�����
//		}
//		
//		if(opentime<40)
//			opentime++;
//		else
//			opentime=0;
		
		x_pump_pwmset(pump_PWMset);//�򿪳�����
	}
	else//
	{
		opentime=0;
		x_pump_pwmset(0);//�ر�����
	}
	
	if(g_sysparam_st.airpump.aspirator_onff == 1)
	{
		x_expump_pwmset(120);
	}
	else
	{
		x_expump_pwmset(0);
	}
	
}
static void ls_10ms_timer_cb(void *param)
{
	float value;
	g_sysparam_st.timer++;
	//x_ai_timer10ms();
	//airbag_ctr();//���ҿ��ƽ��Ⱥ�����ѯ

 if(xTimer_10ms_inst)
 {
    builtin_timer_start(xTimer_10ms_inst, 10, NULL); 
  }
	x_snoreInterven_10ms();
	//control_timer10ms();
	x_uart_10ms();
	// x_report_time_10ms();
}

//����˯���޸���ѹ���Ʋ���
void shuizi_canshu(uint8_t posture_per)
{
	if(posture_per==0)//ƽ��
	{
		airpreee_set=pingtang_pressset;

	}
	else if(posture_per==1)//����
	{
		airpreee_set=cetang_pressset;
	}
}


static void ls_100ms_timer_cb(void *param)
{
	float variate;
	uint8_t i;
	
	///////////���Ź�ι��///////////
	 HAL_IWDG_Refresh();


	//////////////////////////////////ͨ����ѹ�仯����˯��//////////////////////////////////////
	getwfData();

	///////////���ڴ�ӡ��ѹʵʱ����	////////////////////
	x_uart_realtimeprint();


	if(xTimer_100ms_inst)
	{
		builtin_timer_start(xTimer_100ms_inst, 100, NULL); 
	}
}






static void ls_1000ms_timer_cb(void *param)
{
	if(snore_timevoer<=snore_timevoer_def)
     snore_timevoer++;//����ʶ���ʱ
	
	if(snore_timevoer>=snore_timevoer_def)
	{
		g_sysparam_st.sf.snoreState=0;
	}
	else
		g_sysparam_st.sf.snoreState=1;


	
	if(airbagsetfile->flag&0X80)//�ֶ�ģʽ��
	{
		//��ѹ���󱣻�
		if(airbagsetfile->airpressure>17000)
			{
				airbag_ctrbase=air_outstart;//��ʼ����
			}
		else if(airbagsetfile->airpressure>8000)
			{
				x_pump_pwmset(0);
				x_valve_pwmset(0);
				airbagsetfile->flag &=~0X01;//�رճ���
				airbag_ctrbase=air_inputstop;
			}
	}
	else//�Զ�ģʽ��
	{
	
	}

////////////////�¶ȿ����߼�/////////////////////////	

	//////////////////ѹ�羲�����ÿ���/////////////////////	
	pump_control();

	x_rtc_get();
///////////////////////////////////////	
		//ls_uart_realtimeprint();
 if(xTimer_1000ms_inst)
 {
    builtin_timer_start(xTimer_1000ms_inst, 1000, NULL); 
  }
}




