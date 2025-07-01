#include "x_sf.h"
#include "g.h"

#define MAX_LENGTH 						15
#define AI_DEFFER   					300
#define AI_SLEEP_POS_DEFFER   3   		//�粿˯�˲�ֵ
#define SLEEP_P     			   	400				//˯�˼��
#define HUMANCOM_GO          	1000				//����ȥ
#define LEAVE_BED_TIMEROUT   	500			//�봲  5s
#define AI_WENDIN 						1
#define AI_BU_WENDIN 					0

#define NEW_LEAVE_VLAUE  			55


int lastqiya_s;
int leaveBedTimerout;
int leaveBedLvue;
int s_buff[MAX_LENGTH];
uint16_t sw_index = 0;
static uint16_t bigData,smallData = UINT16_MAX;



void x_ai_getdata(void);
void x_ai_human_detection(int lastkpa_s ,int curkpa_s);
void x_ai_SleepingPositionJudgment(int lastkpa ,int curkpa);
void x_ai_tidong(void);
void x_new_humandetection(void);

// �Ƚϲ������������еĽϴ�ֵ
int max_t(int a, int b);

// �Ƚϲ������������еĽ�Сֵ
int min_t(int a, int b);

void x_ai_timer10ms(void)
{
	static int i = 0;
	
	i++;	
	
	if(i%10 == 0)//�������
	{
		x_ai_getdata();
	}
	
	if( leaveBedTimerout >1)
	{
		leaveBedTimerout--;
		
		if(leaveBedTimerout == 1)
		{
			leaveBedTimerout = 0;
		}
	}
}

void x_ai_getdata(void)
{
	
	if(g_sysparam_st.timer>200)
	{
		if(sw_index<MAX_LENGTH)
		{	
			s_buff[sw_index] = (g_sysparam_st.airpump.pa_cur);
			sw_index++;
		}
		else
		{
			memcpy(s_buff,&s_buff[1],(MAX_LENGTH-1)*4);
			s_buff[MAX_LENGTH-1] = g_sysparam_st.airpump.pa_cur;
		}
	}
	
	x_ai_tidong();
}


void x_ai_tidong(void)
{
	
	static int oldstabilize;
	
	
	if(g_sysparam_st.sf.stabilize == AI_BU_WENDIN)
	{
		bigData = max_t(bigData,g_sysparam_st.airpump.pa_cur);
		smallData = min_t(smallData,g_sysparam_st.airpump.pa_cur);		
	}
	
	
	if((g_sysparam_st.sf.stabilize !=oldstabilize)  &&   (g_sysparam_st.sf.stabilize == AI_WENDIN)&&(g_sysparam_st.stretch.stretchTrig == 0))
	{
		LOG_I("bigData = %d  smallData = %d %d",bigData,smallData,bigData-smallData);
		if(bigData - smallData > g_sysparam_st.sf.ThresholdMove)
		{
			g_sysparam_st.sf.bigMove++;
		}
		else
		{
			g_sysparam_st.sf.smallMove++;
		}
		
		
		bigData = g_sysparam_st.airpump.pa_cur;
		smallData =g_sysparam_st.airpump.pa_cur;
	}
	
	
	
	oldstabilize = g_sysparam_st.sf.stabilize;
}



int x_ai_getsum(int *buff,int len)
{
	int sum = 0,i = 0;
	
	for(i = 0; i<len;i++)
	{
		sum += buff[i];
	}
	return sum/len;
}

/*
		�ж��Ƿ��ȶ�
*/
int x_ai_isWaveformStable(int arr[], int size, int threshold) 
{
	int threshold_t;
	
	
	if(g_sysparam_st.airpump.pa_cur<1600)
		threshold_t = 150;
	else
		threshold_t = 300;
	
	//threshold_t = threshold;
	

	if (size < 2) {
			return 1;  // ֻ��һ��Ԫ�ػ��߿�������Ϊ���ȶ��ģ��ɸ���ʵ�ʶ����޸ģ�
	}
	for (int i = 0; i < size - 1; i++) 
	{
			int diff = abs(arr[i] - arr[i + 1]);
		
			LOG_I("diff = %d",diff);
			if (diff > threshold_t) 
			{
				LOG_I("diff = %d",diff);
					return 0;  // ��������Ԫ�ز�ֵ����ֵ������ֵ�����ȶ�
			}
	}
	return 1;  // ��������������Ԫ�أ���ֵ����ֵ��û������ֵ���ȶ�
}


/*
		��ȡ����ֵ
*/
int x_ai_get_fluctuatingValue(int arr[], int size) 
{
	int diff = 0;
	uint16_t bigData_t = 0,smallData_t = UINT16_MAX;

	

	if (size < 2) {
			return 1;  // ֻ��һ��Ԫ�ػ��߿�������Ϊ���ȶ��ģ��ɸ���ʵ�ʶ����޸ģ�
	}
	for (int i = 0; i < size - 1; i++) 
	{
		diff = abs(arr[i] - arr[i + 1]);
		
		bigData_t = max_t(bigData_t,diff);
		smallData_t = min_t(smallData_t,diff);
	}
	return bigData_t-smallData_t;  // ��������������Ԫ�أ���ֵ����ֵ��û������ֵ���ȶ�
}

void x_ai_Init(void)
{
	g_sysparam_st.sf.ThresholdMove = 1000;
}

//�ȶ�
void OKIN_ai_run(void)
{
	static int  stabilize,oldstabilize;
	static uint32_t t_timer = 0;
	static uint8_t oldadj;

	if(t_timer == 0)
	{
		t_timer = g_sysparam_st.timer;
	}
	
	if( (t_timer != 0) && ((g_sysparam_st.timer - t_timer) > 50))//500ms�ж�һ���Ƿ��ȶ�
	{
		t_timer = 0;
	
		if(sw_index == MAX_LENGTH)  //���������ٽ��м���
		{
			if(x_ai_isWaveformStable(s_buff,MAX_LENGTH,AI_DEFFER) == 1)
			{				
				stabilize = 1;			
			}
			else
			{
				stabilize = 0;
			}
			g_sysparam_st.sf.stabilize = stabilize;
			
			if(oldstabilize!= stabilize)
			{
				if(stabilize == AI_WENDIN)
				{
					LOG_I("wen din");
					leaveBedTimerout = 0;								
					x_ai_human_detection(lastqiya_s,x_ai_getsum(s_buff,MAX_LENGTH));				
				}
				else
				{
						LOG_I("bu wen din");
				}			
			}
				
			if(oldstabilize == stabilize)
			{
				if(stabilize == AI_WENDIN)
				{
					lastqiya_s = x_ai_getsum(s_buff,MAX_LENGTH);  // ��¼�ȶ�ǰ����ѹ			
				}
			}
		}
		oldstabilize = stabilize;	
	}	
	
	
	x_new_humandetection();
}

// �Ƚϲ������������еĽϴ�ֵ
int max_t(int a, int b) {
    return (a > b)? a : b;
}

// �Ƚϲ������������еĽ�Сֵ
int min_t(int a, int b) {
    return (a < b)? a : b;
}


// �������������ֵ����Сֵ�Ĳ�ֵ�ĺ���
int OKIN_ai_isWaveformStable1(int arr[], int size,int threshold) 
{
	int diff = 0;
	if (size <= 0) {
			return 0;  // ��������С���Ϸ�������0
	}
	int max_value = arr[0];
	int min_value = arr[0];
	for (int i = 1; i < size; i++) {
			max_value = max_t(max_value, arr[i]);
			min_value = min_t(min_value, arr[i]);
	}
	LOG_I("max_value = %d,min_value = %d",max_value,min_value);
	diff = max_value - min_value;
		
	if(diff < threshold)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


void x_ai_human_detection(int lastkpa_s ,int curkpa_s)
{
	int defs = lastkpa_s - curkpa_s;
	
	int t_max = max_t(lastkpa_s,curkpa_s);
	
	static int myinit = 0;
	
	if(myinit == 0)
	{
		myinit = 1;
		return;
	}
	LOG_I("curkpa = %d lastkpa = %d   def = %d",lastkpa_s,curkpa_s,defs);
	if( ((abs(defs) > t_max/4))  || (abs(defs) > 2000))
	{
		if(defs > 0)
		{
			LOG_I("defs = %d",defs);
			g_sysparam_st.humandetection = 0; //�봲
			LOG_I("Out bed");			
		}
		else
		{
			LOG_I("defs = %d ",defs);
			LOG_I("In bed");
			g_sysparam_st.humandetection = 1; //�ڴ�
			x_ai_SleepingPositionJudgment(lastkpa_s,curkpa_s);
		}
	}
	else
	{
			if(abs(defs)>300)
			{
				LOG_I("In bed");
				g_sysparam_st.humandetection = 1; //�ڴ�
			}		
		x_ai_SleepingPositionJudgment(lastkpa_s,curkpa_s);
	}	
}


void x_ai_SleepingPositionJudgment(int lastkpa ,int curkpa)
{
	static int myinit = 0;
	int def = lastkpa -curkpa ;
	uint32_t temp_t = 0;
	//LOG_I("curkpa = %d lastkpa = %d   def = %d",lastkpa,curkpa,def);
	
	if(myinit == 0)
	{
		myinit = 1;
		return;
	}
	
	if(g_sysparam_st.humandetection == 0)
		return;
		
	if(g_sysparam_st.ai_adj == 0)
		return;
	
	if(g_sysparam_st.airpump.inflate_onff==1 || 
		 g_sysparam_st.airpump.deflate_onoff == 1)
	{
		return;
	}
	g_sysparam_st.cboxlog_st.ai_adjNub++;
	
	HAL_TRNG_GenerateRandomNumber(&temp_t);	
	
	if((temp_t%2) != g_sysparam_st.sleepingPosture)
	{	
		g_sysparam_st.sleepingPosture  = temp_t%2;
	}
	else
	{
		g_sysparam_st.cboxlog_st.big_movecount_unchange++;
	} 
	//LOG_I("g_sysparam_st.sleepingPosture %d  temp_t = %d",g_sysparam_st.sleepingPosture,temp_t);	
}

void x_new_humandetection(void)
{
	static uint32_t t_timer = 0,t_timer1 = 0;
	static uint32_t count_s,count_s1,sum_s;
	int temp = 0;
	 
	
	if(g_sysparam_st.airpump.pa_cur<2000)
		return;
	
	
	if((g_sysparam_st.airpump.deflate_onoff != 0 ) ||(g_sysparam_st.airpump.inflate_onff != 0 )  || (g_sysparam_st.sf.stabilize == AI_BU_WENDIN))
	{
		t_timer1 = g_sysparam_st.timer;
		return ;
	}
		
	if((t_timer1 !=0) && (g_sysparam_st.timer - t_timer1 > 300))//3s���ڽ����ж�
	{
		t_timer1 = 0;
	}
	else if((t_timer1 !=0) && (g_sysparam_st.timer - t_timer1 <= 300))
	{
		return ;
	}
	
	if(t_timer == 0)
	{
		t_timer = g_sysparam_st.timer;
	}
	
	if( (t_timer != 0) && ((g_sysparam_st.timer - t_timer) > 20))//500ms�ж�һ���Ƿ��ȶ�
	{
		t_timer = 0;
		
		if(sw_index == MAX_LENGTH)  //���������ٽ��м���
		{
			temp = x_ai_get_fluctuatingValue(s_buff,MAX_LENGTH);
			
			
			sum_s +=temp;
			count_s++;
			
			
			if(count_s >= 10)
			{
							
				if((sum_s < NEW_LEAVE_VLAUE)  && (g_sysparam_st.humandetection == 1))
				{
					count_s1++;
					
					if(count_s1>5)
					{
						g_sysparam_st.humandetection = 0;
					}
						
				}	
				if(sum_s >= NEW_LEAVE_VLAUE)
				{
					count_s1 = 0;
				}								
				
				LOG_I("x_ai_get_fluctuatingValue = %d",sum_s);
				
				sum_s = 0;
				count_s = 0;
			}	
		}
	}
}











