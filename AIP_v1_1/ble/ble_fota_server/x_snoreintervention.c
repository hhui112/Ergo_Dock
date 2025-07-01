#include "x_faultDetect.h"
#include "x_snoreintervention.h"
#include "g.h"

#define SNORETIMER  (15*60)			//(15*60) 15分钟触发一次打鼾干预检查
#define SNORELIMITPA  (2500)

void x_snoreInterven_10ms(void)
{
	
}

// 气囊压力读取函数，这里使用当前压力值作为返回值，模拟实际读取
int read_air_cushion_pressure(void) 
{
    return g_sysparam_st.airpump.pa_cur;
}

//15分钟一次  : 记录这期间的打鼾次数 g_sysparam_st.sf.snoreNub
void simulate_detection_period(void) 
{
	static uint32_t timer_s;
	static uint32_t oldsnoreNub;
	
	if(timer_s == 0)
	{
		timer_s =  g_sysparam_st.timer;
		oldsnoreNub = g_sysparam_st.sf.snoreNub;
	}
	
	if( (timer_s!=0) && (g_sysparam_st.timer - timer_s > g_sysparam_st.snoreIntervention.trigTimer))
	{
		timer_s = 0;
		g_sysparam_st.snoreIntervention.trig = 1;
		g_sysparam_st.sf.snoreNubperiod = g_sysparam_st.sf.snoreNub - oldsnoreNub;
	}
}

// 计算打鼾分数
int calculate_snoring_score(int snoring_times) {
    return snoring_times * 4;
}


// 判别干预手段
void determine_intervention(int snoring_score, int* increment) 
{
    if (snoring_score >= 5) 
		{
        if (snoring_score <= 15) 			
            *increment = 200;      
				else 				
            *increment = 300;       
    } 
		else
		{
        *increment = 0;
    }
}



// 调整气囊压力，返回目标压力
int adjust_air_cushion_pressure(int air_cushion_upper_limit, int increment) 
{
    // 读取当前气囊压力
    int air_cushion_pressure = read_air_cushion_pressure();
    //printf("当前气囊压力: %d pa\n", air_cushion_pressure);
    
    // 计算目标压力
    int target_pressure = air_cushion_pressure + increment;
    
    // 确保目标压力不超过上限
    if (target_pressure > air_cushion_upper_limit) {
        target_pressure = air_cushion_upper_limit;
    }
    
    return target_pressure;
}

//// 鼾声干预函数，负责单次的鼾声检测和干预逻辑
//void snoring_intervention(int* current_pressure, int air_cushion_upper_limit, int detection_flag, int* intervention_count) {
//    // 模拟检测周期
//    int snoring_times = simulate_detection_period(detection_flag);
//    
//    // 计算打鼾分数
//    int snoring_score = calculate_snoring_score(snoring_times);
//    printf("\n15分钟检测周期结束，打鼾次数: %d 次\n", snoring_times);
//    printf("打鼾分数: %d\n", snoring_score);
//    
//    char intervention_strength[10];
//    int increment;
//    
//    // 根据分数进行干预
//    determine_intervention(snoring_score, intervention_strength, &increment);
//    
//    if (strcmp(intervention_strength, "无") != 0) {
//        printf("\n进行%s干预\n", intervention_strength);
//        int target_pressure = adjust_air_cushion_pressure(*current_pressure, air_cushion_upper_limit, increment);
//        printf("%s干预: 将气囊压力从 %d pa 充气至 %d pa\n", intervention_strength, *current_pressure, target_pressure);
//        *current_pressure = target_pressure;
//        *intervention_count = 1;
//    } else {
//        printf("\n打鼾分数低于5，无需干预\n");
//        *intervention_count = 0;
//    }
//}
void x_SnoringInterventionInit(void)
{
	g_sysparam_st.snoreIntervention.trigTimer = SNORETIMER*100;	// 15分钟
	g_sysparam_st.snoreIntervention.limit_pa = SNORELIMITPA;
	x_SnoringInterventStateClear();
}

void x_SnoringIntervention_run(void)
{
	int increment = 0;
	
	if(g_sysparam_st.snoreIntervention.enable == true)
	{
		
		simulate_detection_period();		// 检测打鼾次数
		if(g_sysparam_st.snoreIntervention.trig == 1)	// 检测中
		{
			g_sysparam_st.snoreIntervention.trig = 0;
			if(g_sysparam_st.sf.snoreNubperiod > 5)	// 如果次数大于5次
			{
			
			}
		}
		
		/*
		if(g_sysparam_st.snoreIntervention.trig == 1)
		{
			g_sysparam_st.snoreIntervention.trig = 0;
			g_sysparam_st.sf.snoring_score =  calculate_snoring_score(g_sysparam_st.sf.snoreNubperiod);
			determine_intervention(g_sysparam_st.sf.snoring_score,&increment);
			
			if(increment != 0 )
			{
				adjust_pressure(	adjust_air_cushion_pressure(g_sysparam_st.snoreIntervention.limit_pa,increment),50);//触发充气
				g_sysparam_st.sf.snoreIntervenNub++;//干预次数++
				g_sysparam_st.snoreIntervention.triging = 1;//干预中
			}
			
		}
		
		*/
		
		
		
		
		
		
	}
}

void x_SnoringInterventStateClear(void)
{
	//g_sysparam_st.sf.snoreIntervenNub = 0;
	g_sysparam_st.snoreIntervention.triging = 0;
}



