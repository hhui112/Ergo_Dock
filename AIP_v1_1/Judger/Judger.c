#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
//#include <unistd.h>
#include <math.h>
#include "timer_event.h"
#include "Judger.h"
#include "uart_base.h"
#include "sleep_posture.h"
#include "g.h"
#include "stdio.h"
#include "ls_hal_trng.h"
#define MAX_LENGTH_LEFT  9
#define MAX_LENGTH_POSITION 60


#define THRESHOLD_MOVE 150
#define THRESHOLD_POSITION 100
#define THRESHOLD_LEFT 1000
#define THRESHOLD_CHANGE 600

uint16_t threshold_move = 150;
uint16_t threshold_position = 100;
uint16_t threshold_left = 1000;
uint16_t humanPressure;// 有人气压

extern  airbag_parameter_def *airbagsetfile;
extern  sleepposture_def  sleepposture;
//用于调整的参数
//int MAX_LENGTH = 50;            // 数据窗口大小
//int GAP = 2;                    // 采样间隔
//int THRESHOLD_MOVE = 100;         // 体动阈值，数据方差超过该值认为发生体动
//int THRESHOLD_POSITION = 150;     // 判断姿势变化阈值,气压变化超过该值认为姿势发生了变化
//int THRESHOLD_LEFT = 1000;        // 离枕阈值 气压变化超过该值认为在离枕状态发生了变化
//int JUDGE_GAP = 5;              // 判断间隔
int pressure_position[MAX_LENGTH_POSITION];   //记录压力值 0.1S一条
int pressure_left[MAX_LENGTH_LEFT];
int current_size_position = 0;       //当前记录中的压力长度
int current_size_left = 0;
int pre_in_state = 0;       //默认离枕开始 0离床 1在床
int in_state = 0;           //在离枕状态 0离床 1在床
int pre_position_state = 0; //默认平躺开始 0离床 1在床
int position_state = 0;     //姿势状态  0 平躺 1侧躺
int step_judge_left = 0;
int step_judge_position = 0;
int gap_left = 3;
int gap_position = 20;
int in_state_position_judge = 0;
int pre_in_state_position_judge = 0;
int judge_move_flag = 0;
Judger judger;

char mybuff[255];
//设置参数函数
//void set_max_length(int length)
//{
//	MAX_LENGTH = length;
//}
//void set_gap(int gap)
//{
//	GAP = gap;
//}
//void set_threshold_move(int threshold)
//{
//	THRESHOLD_MOVE = threshold;
//}
//void set_threshold_position(int threshold)
//{
//	THRESHOLD_POSITION = threshold;
//}
//void set_threshold_left(int threshold)
//{
//	THRESHOLD_LEFT = threshold;
//}
//void set_judge_gap(int gap)
//{
//	JUDGE_GAP = gap;
//}


void judgerInit(void)
{
 threshold_move = 150;
 threshold_position = 100;
 threshold_left = 1000;
}

void uartforjudger(void)
{
	ble_data_init tx;
	tx.ble_data_receive[0]=(uint8_t)(in_state>>8);
	tx.ble_data_receive[1]=(uint8_t)(in_state);
	tx.ble_data_receive[2]=(uint8_t)(position_state>>8);
	tx.ble_data_receive[3]=(uint8_t)(position_state);
	tx.lengh=4;
	//ls_uart_tx_send(&tx);
}

//判断器初始化函数
void init_judger(Judger* judger)
{

	judger->threshold_move = threshold_move;
	judger->threshold_position = threshold_position;
	judger->threshold_left = threshold_left;
}


//辅助函数-计算平均压力值
double calculate_mean(const int* data, int start, int end) {
    double sum = 0;
    int count = end - start;
    
    for (int i = start; i < end; i++) {
        sum += data[i];
    }
    
    return sum / count;
}

//辅助函数-计算压力数据标准差
double calculate_std(const int* data, int start, int end) 
	{
    double mean = calculate_mean(data, start, end);
    double sum_squared_diff = 0;
    int count = end - start;
    
    for (int i = start; i < end; i++) {
        double diff = data[i] - mean;
        sum_squared_diff += diff * diff;
    }
    
    return sqrt(sum_squared_diff / count);
}

//辅助函数-记录压力值
void add_pressure(int* pressure, int* current_size, int pressure_value, int max_length) {
    if (*current_size >= max_length) {
        // 移动数组
        memmove(pressure, pressure + 1, 
                (max_length - 1) * sizeof(int));
        (*current_size)--;
    }
    pressure[*current_size] = pressure_value;
		(*current_size)++;
}

//辅助函数-重置压力值
void reset_pressure(int* pressure, int* current_size, int max_length) {
    *current_size = 0;
    memset(pressure, 0, max_length * sizeof(int));
}


//判断是否发生了体动
bool judge_move(Judger* judger, const int* pressure, int size)
{
    // 计算压力数据标准差
    int t = size / 3;
    double std = calculate_std(pressure, t, 2*t+1);

		DELAY_US(1000);
    if (std > judger->threshold_move) {
			g_sysparam_st.airpump.Stableflag = 1;//不稳定
        return true;
    }
		g_sysparam_st.airpump.Stableflag = 2;//稳定
    return false;
}

int judge_move_position(Judger* judger, const int* pressure, int size)
{
    // 计算压力数据标准差
    int t = size / 3;
    double std = calculate_std(pressure, t, 2*t+1);
	
//		sprintf(mybuff,"judge_move1 std = %f\r\n",std);
//		HAL_UART_Transmit_IT(&UART_Server_Config, (uint8_t *)mybuff,strlen(mybuff));
    // 判断是否发生了体动
    if (std > judger->threshold_move) {
        if(std < THRESHOLD_CHANGE)
				{
					//小体动
						return 1;
				}
				else
				{
					//大体动
						return 2;
				}
    }
    return 0;
}

//判断是否发生了离枕变化 返回0表示离枕，返回1表示在枕
int judge_left(Judger* judger, const int* pressure, int size, int pre_state)
{
    // 计算变化前后压力数据的差值
    int t = size / 3;
    double pre_mean = calculate_mean(pressure, 0, t+1);
    double post_mean = calculate_mean(pressure, 2*t+1, size);
    // 判断是否发生了离枕变化
	
//		sprintf(mybuff,"fabs_left = %f\r\n",fabs(pre_mean - post_mean));
//		HAL_UART_Transmit_IT(&UART_Server_Config, (uint8_t *)mybuff,strlen(mybuff));
		DELAY_US(1000);
    if (fabs(pre_mean - post_mean) > judger->threshold_left) {
        if (pre_mean < post_mean) 
				{
            return 1;
        }
        else 
				{
            return 0;
        }
    }
    return pre_state;
}

//判断是否发生了姿势变化 返回0表示平躺，返回1表示侧躺
int judge_position(Judger* judger, const int* pressure, int size, int pre_state)
{
    // 计算变化前后压力数据的差值
    int t = size / 3;
    double pre_mean = calculate_mean(pressure, 0, t+1); 
    double post_mean = calculate_mean(pressure, 2*t+1, size);
		double std = calculate_std(pressure, t, 2*t+1);
		//sprintf(mybuff," threshold_position = %d fabs_position = %f\r\n",judger->threshold_position,fabs(pre_mean - post_mean));
		//HAL_UART_Transmit_IT(&UART_Server_Config, (uint8_t *)mybuff,strlen(mybuff));
    // 判断是否发生了姿势变化
		int judge_threshold = judger->threshold_left;
		if (std > THRESHOLD_CHANGE)
		{
				judge_threshold = judger->threshold_left/3;
		}
    if ((fabs(pre_mean - post_mean) > judger->threshold_position) && (fabs(pre_mean - post_mean) < judge_threshold)) 
			{
        if (pre_mean > post_mean) 
					{
            return 1;
        }
        else 
					{
            return 0;
        }
    }
    return pre_state;
}


int read_pressure(void)
{
	 return (int)airbagsetfile->airpressure;
}

    

int judger_left(void)
{
	if(g_sysparam_st.humandetection == 0 &&g_sysparam_st.timer > 3000) // 3s后在进行判断
	{
		if( (g_sysparam_st.airpump.pa_cur >g_sysparam_st.airpump.oldpa_leave) &&  (g_sysparam_st.airpump.pa_cur- g_sysparam_st.airpump.oldpa_leave >1000))
		{
			g_sysparam_st.humandetection = 1;
			in_state = 1;
      pre_in_state = 1;
		}
	}
	
	if(g_sysparam_st.airpump.pa_cur <750)
	{
			g_sysparam_st.humandetection = 0;
			in_state = 0;
      pre_in_state = 0;
	}

	step_judge_left++;
	if(current_size_left < MAX_LENGTH_LEFT)
	{		
			add_pressure(pressure_left, &current_size_left, read_pressure(), MAX_LENGTH_LEFT);
			return 0;
	}
	else
	{
			add_pressure(pressure_left, &current_size_left, read_pressure(), MAX_LENGTH_LEFT);
			if ((step_judge_left%gap_left==0) && judge_move(&judger, pressure_left, current_size_left)) //判断是否发生了体动
			{          											
					pre_in_state = in_state;
					in_state = judge_left(&judger, pressure_left, current_size_left, in_state);					
					reset_pressure(pressure_left, &current_size_left, MAX_LENGTH_LEFT);
					g_sysparam_st.humandetection = in_state;
					
					if(g_sysparam_st.humandetection == 0)
					{
						g_sysparam_st.airpump.oldpa_leave = g_sysparam_st.airpump.pa_cur;//记录离开枕头的气压
					}
			}
			return 1;
	}
}


int judger_position(void)
{
		uint32_t temp_t = 0;
    uartforjudger();//串口打印

		if((g_sysparam_st.airpump.deflate_onoff == 1) || (g_sysparam_st.airpump.inflate_onff == 1))
		{
			 reset_pressure(pressure_position, &current_size_position, MAX_LENGTH_POSITION);
				return 0;
		}
		step_judge_position++;
    if(current_size_position < MAX_LENGTH_POSITION)
    {		
        add_pressure(pressure_position, &current_size_position, read_pressure(), MAX_LENGTH_POSITION);
        return 0;
    }
    else
    {
        add_pressure(pressure_position, &current_size_position, read_pressure(), MAX_LENGTH_POSITION);
				
        if (step_judge_position%gap_position==0 && in_state == 1) //判断是否发生了体动
        {   
						judge_move_flag = judge_move_position(&judger, pressure_position, current_size_position);
						if(judge_move_flag == 0)
						{
								return 1;
						}
						if(judge_move_flag == 1)
						{
								g_sysparam_st.cboxlog_st.small_movecount++;
						}
						if(judge_move_flag == 2)
						{
							pre_position_state = position_state;
							
							
							
							//position_state = judge_position(&judger, pressure_position, current_size_position, position_state);
							
								HAL_TRNG_GenerateRandomNumber(&temp_t);
								position_state = temp_t%2;
							
							if(pre_position_state == position_state)
							{
								position_state = !position_state;
								g_sysparam_st.cboxlog_st.big_movecount_unchange++;
							}
							else
							{
								g_sysparam_st.cboxlog_st.big_movecount_change++;
							}
						}
						
						//reset_pressure(pressure_position, &current_size_position, MAX_LENGTH_POSITION);
        }
     }
		 return 1;
}
void sf_refreshParameter(void)
{
	init_judger(&judger);
}



