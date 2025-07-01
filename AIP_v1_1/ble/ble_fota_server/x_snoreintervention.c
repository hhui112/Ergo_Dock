#include "x_faultDetect.h"
#include "x_snoreintervention.h"
#include "g.h"

#define SNORETIMER  (15*60)			//(15*60) 15���Ӵ���һ�δ�����Ԥ���
#define SNORELIMITPA  (2500)

void x_snoreInterven_10ms(void)
{
	
}

// ����ѹ����ȡ����������ʹ�õ�ǰѹ��ֵ��Ϊ����ֵ��ģ��ʵ�ʶ�ȡ
int read_air_cushion_pressure(void) 
{
    return g_sysparam_st.airpump.pa_cur;
}

//15����һ��  : ��¼���ڼ�Ĵ������� g_sysparam_st.sf.snoreNub
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

// �����������
int calculate_snoring_score(int snoring_times) {
    return snoring_times * 4;
}


// �б��Ԥ�ֶ�
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



// ��������ѹ��������Ŀ��ѹ��
int adjust_air_cushion_pressure(int air_cushion_upper_limit, int increment) 
{
    // ��ȡ��ǰ����ѹ��
    int air_cushion_pressure = read_air_cushion_pressure();
    //printf("��ǰ����ѹ��: %d pa\n", air_cushion_pressure);
    
    // ����Ŀ��ѹ��
    int target_pressure = air_cushion_pressure + increment;
    
    // ȷ��Ŀ��ѹ������������
    if (target_pressure > air_cushion_upper_limit) {
        target_pressure = air_cushion_upper_limit;
    }
    
    return target_pressure;
}

//// ������Ԥ���������𵥴ε��������͸�Ԥ�߼�
//void snoring_intervention(int* current_pressure, int air_cushion_upper_limit, int detection_flag, int* intervention_count) {
//    // ģ��������
//    int snoring_times = simulate_detection_period(detection_flag);
//    
//    // �����������
//    int snoring_score = calculate_snoring_score(snoring_times);
//    printf("\n15���Ӽ�����ڽ�������������: %d ��\n", snoring_times);
//    printf("��������: %d\n", snoring_score);
//    
//    char intervention_strength[10];
//    int increment;
//    
//    // ���ݷ������и�Ԥ
//    determine_intervention(snoring_score, intervention_strength, &increment);
//    
//    if (strcmp(intervention_strength, "��") != 0) {
//        printf("\n����%s��Ԥ\n", intervention_strength);
//        int target_pressure = adjust_air_cushion_pressure(*current_pressure, air_cushion_upper_limit, increment);
//        printf("%s��Ԥ: ������ѹ���� %d pa ������ %d pa\n", intervention_strength, *current_pressure, target_pressure);
//        *current_pressure = target_pressure;
//        *intervention_count = 1;
//    } else {
//        printf("\n������������5�������Ԥ\n");
//        *intervention_count = 0;
//    }
//}
void x_SnoringInterventionInit(void)
{
	g_sysparam_st.snoreIntervention.trigTimer = SNORETIMER*100;	// 15����
	g_sysparam_st.snoreIntervention.limit_pa = SNORELIMITPA;
	x_SnoringInterventStateClear();
}

void x_SnoringIntervention_run(void)
{
	int increment = 0;
	
	if(g_sysparam_st.snoreIntervention.enable == true)
	{
		
		simulate_detection_period();		// ����������
		if(g_sysparam_st.snoreIntervention.trig == 1)	// �����
		{
			g_sysparam_st.snoreIntervention.trig = 0;
			if(g_sysparam_st.sf.snoreNubperiod > 5)	// �����������5��
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
				adjust_pressure(	adjust_air_cushion_pressure(g_sysparam_st.snoreIntervention.limit_pa,increment),50);//��������
				g_sysparam_st.sf.snoreIntervenNub++;//��Ԥ����++
				g_sysparam_st.snoreIntervention.triging = 1;//��Ԥ��
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



