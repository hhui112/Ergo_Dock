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
#include "app_key.h"





#define BLE_SEND_SYN_LEN  41//蓝牙发送同步数据长度
#define BLE_AIRBAG_SEND_SYN_LEN 11//气囊控制蓝牙发送同步数据长度
#define temp_default  42//默认设置温度  单位度
#define time_default  20*60//默认定时  单位秒
#define protect_I  2000  //过流保护电流设置



#define snore_timevoer_def 5*60 //鼾声判断5分钟内无鼾声判断为不打鼾



sleepposture_def  sleepposture;
airbag_ctrbase_def airbag_ctrbase;

uint16_t airpreee_LD=200;//自动充气目标气压下误差
uint16_t airpreee_HD=200;//自动放气目标气压上误差
uint16_t airpreee_set=2000;//控制变量设置目标气压
uint8_t pump_PWMset=255;

//uint8_t vidoehead[32]={0};
//uint16_t length_video1,length_video_i,add_video1;
uint16_t snore_timevoer=snore_timevoer_def;//停止打鼾超时计时器

ble_data_init ble_data_receive;//定义蓝牙接收数据缓存
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

airbag_parameter_def *airbagsetfile=(airbag_parameter_def *)(&set_parameter[50]);//气囊控制功能设置文件，也是APP上行文件  50个字节






void ls_even_timer_init(void)
{
	
	xTimer_5ms_inst = builtin_timer_create(ls_5ms_timer_cb);
	builtin_timer_start(xTimer_5ms_inst, 5, NULL);

	xTimer_10ms_inst = builtin_timer_create(ls_10ms_timer_cb);
	builtin_timer_start(xTimer_10ms_inst, 10, NULL);	

	xTimer_100ms_inst = builtin_timer_create(ls_100ms_timer_cb);
	builtin_timer_start(xTimer_100ms_inst, 100, NULL);

	xTimer_1000ms_inst = builtin_timer_create(ls_1000ms_timer_cb);
	builtin_timer_start(xTimer_1000ms_inst, 1000, NULL);

	
}



void parameter_init(void)//参数初始化
{
	
}
//气囊控制函数,10ms进入一次，对控制状态更新
void airbag_ctr(void)
{

	static uint8_t valve_pwmnow=0;
	
	switch(airbag_ctrbase)
	{
		case air_outstop://停止放气
			          
								airbagsetfile->flag &=~0X02;//停止放气标志
		            airbagsetfile->flag &=~0X01;//关闭充气标志
								if(LSGPTIMA->CCR1>0)
								{
									valve_pwmnow=LSGPTIMA->CCR1;
									if(valve_pwmnow>160)//PWM值大于160时，PWM值直接设置为160，然后从160逐渐变小
										valve_pwmnow=160;
									airbag_ctrbase=base_state1;
								}
								else
									airbag_ctrbase=base_state2;
								break;
		case base_state1://气阀PWM值逐渐变小，直到0
								if(valve_pwmnow>0)
									valve_pwmnow --;
								x_valve_pwmset(valve_pwmnow);
								if(valve_pwmnow==0)
								{
									airbag_ctrbase=base_state2;

								  airbagsetfile->flag &=~0X02;//停止放气标志
									//airbagsetfile->flag &=~0X01;//关闭充气标志
								}
								break;
		case base_state2://气阀关闭结束
			         break;
		case air_outstart://开始放气

								airbagsetfile->flag |=0X02;//开始放气标志
		            airbagsetfile->flag &=~0X01;//关闭充气标志
								if(LSGPTIMA->CCR1<255)
								{
									valve_pwmnow=LSGPTIMA->CCR1;
									if(valve_pwmnow<128)
										valve_pwmnow=128;
									airbag_ctrbase=base_state3;
								}
								else//气阀已经完全打开
								{
									airbag_ctrbase=air_outing;
									airbagsetfile->flag |=0X02;//开始放气标志
								}
								break;
		case base_state3:
								if(valve_pwmnow<255)
									valve_pwmnow++;
								x_valve_pwmset(valve_pwmnow);
								if(valve_pwmnow==255)
								{
									airbag_ctrbase=air_outing;

									airbagsetfile->flag |=0X02;//开始放气标志
								}
								break;
		case air_outing://气阀打开结束		
		           break;
		case air_inputstart://开始充气


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
		case base_state5://气阀PWM值逐渐变小，直到0
								if(valve_pwmnow>0)
									valve_pwmnow--;
								x_valve_pwmset(valve_pwmnow);
								if(valve_pwmnow==0)
								{
									airbag_ctrbase=base_state6;
								  //airbagsetfile->flag |=0X02;//打开放气标志
									airbagsetfile->flag &=~0X02;//停止放气标志
								}
								break;
		case base_state6://气阀关闭结束

		            airbagsetfile->flag |=0X01;//打开充气标志
								airbag_ctrbase=air_inputing;

								break;
		case air_inputing://开始充气进度完成
		            break;
		case air_inputstop://停止充气


		            airbagsetfile->flag &=~0X01;//关闭充气标志
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

									airbagsetfile->flag &=~0X02;//停止放气标志
								}
								break;		
		case base_state9://停止充气进度完成	
		            break;														
		default:

		            x_valve_pwmset(0);
		            airbagsetfile->flag &=~0X01;//关闭充气标志
		            airbagsetfile->flag &=~0X02;//停止放气标志
		            airbag_ctrbase=base_state9;
		            break;
	}
}



static void ls_5ms_timer_cb(void *param)
{
//	switch(GetKey())//开关按键扫描
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
	
	//OKIN_ai_run();
	// x_flash_run();
	flash_run();
	// SnoringIntervention_run();
 if(xTimer_5ms_inst)
 {
    builtin_timer_start(xTimer_5ms_inst, 5, NULL); 
  }
}
void pump_control(void)
{
	static uint8_t opentime=0; // 开30s 停10s

	if((airbagsetfile->flag&0X01)==0x01)//气泵打开标志
	{
//		if(opentime==0)
//		{
//		  x_pump_pwmset(pump_PWMset);//打开充气泵
//		}
//		else if(opentime==30)
//		{
//			x_pump_pwmset(0);//关闭气泵
//		}
//		
//		if(opentime<40)
//			opentime++;
//		else
//			opentime=0;
		
		x_pump_pwmset(pump_PWMset);//打开充气泵
	}
	else//
	{
		opentime=0;
		x_pump_pwmset(0);//关闭气泵
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
	//airbag_ctr();//气囊控制进度函数轮询

 if(xTimer_10ms_inst)
 {
    builtin_timer_start(xTimer_10ms_inst, 10, NULL); 
  }
  on_key_10ms_handle();
	on_led_10ms_handle();
	control_timer10ms();
	x_uart_10ms();
	// x_report_time_10ms();
}

//根据睡姿修改气压控制参数
void shuizi_canshu(uint8_t posture_per)
{
	if(posture_per==0)//平躺
	{
		airpreee_set=pingtang_pressset;

	}
	else if(posture_per==1)//侧躺
	{
		airpreee_set=cetang_pressset;
	}
}


static void ls_100ms_timer_cb(void *param)
{
	float variate;
	uint8_t i;
	
	///////////看门狗喂狗///////////
	 HAL_IWDG_Refresh();


	//////////////////////////////////通过气压变化计算睡姿//////////////////////////////////////

	///////////串口打印气压实时数据	////////////////////
	// x_uart_realtimeprint();

	SnoringIntervention_run();		//打鼾干预 100ms调用一次

	if(xTimer_100ms_inst)
	{
		builtin_timer_start(xTimer_100ms_inst, 100, NULL); 
	}
}






static void ls_1000ms_timer_cb(void *param)
{
	control_timer1000ms();
	
	x_rtc_get();
///////////////////////////////////////	
		//ls_uart_realtimeprint();
 if(xTimer_1000ms_inst)
 {
    builtin_timer_start(xTimer_1000ms_inst, 1000, NULL); 
  }
}




