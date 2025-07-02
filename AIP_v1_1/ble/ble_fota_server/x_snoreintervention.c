#include "x_faultDetect.h"
#include "x_snoreintervention.h"
#include "g.h"

#define SNORETIMER  (5*60)			//(15*60) 5���Ӵ���һ�δ�����Ԥ���
#define SNORELIMITPA  (2500)
#define SI_TH					(5)				// ��Ԥ��ֵ
#define SI_PWM				(0x80)
#define SI_TMR				(0x50)


// ����ѹ����ȡ����������ʹ�õ�ǰѹ��ֵ��Ϊ����ֵ��ģ��ʵ�ʶ�ȡ
int read_air_cushion_pressure(void) 
{
    return g_sysparam_st.airpump.pa_cur;
}

//5����һ��  : ��¼���ڼ�Ĵ������� g_sysparam_st.sf.snoreNub
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

// �б��Ԥ�ֶ�
void determine_intervention(void) 
{
	if(g_sysparam_st.sf.snoreNubperiod > SI_TH){
			g_sysparam_st.snoreIntervention.triggered_flag = true;
	}else{	
			g_sysparam_st.snoreIntervention.triggered_flag = false;
	}
}

void SnoringInterventionInit(void)
{
	g_sysparam_st.snoreIntervention.trigTimer = SNORETIMER*100;	// 5����
	g_sysparam_st.snoreIntervention.snoreIntervention_pwm = SI_PWM;
	g_sysparam_st.snoreIntervention.snoreIntervention_tmr = SI_TMR;
	g_sysparam_st.snoreIntervention.snoreIntervention_threshold = SI_TH;

	SnoringInterventStateClear();
}


void SnoringIntervention_run(void) /* 50-100ms����һ�� */
{
    if (!g_sysparam_st.snoreIntervention.enable) return;

    simulate_detection_period();  // ��¼���ڴ�������

    if (g_sysparam_st.snoreIntervention.trig == 1) // ���ڽ������жϽ��
    {
        g_sysparam_st.snoreIntervention.trig = 0;
        determine_intervention();  // �ж��Ƿ���Ҫ��Ԥ

        if (!g_sysparam_st.snoreIntervention.is_intervening && g_sysparam_st.snoreIntervention.triggered_flag)
        {
            LOG_I("������Ԥ��ʼ��������̧��\r\n");
            // prepare_mfp_SOFT_START(KEY_MEMORY4, g_sysparam_st.snoreIntervention.snoreIntervention_pwm, g_sysparam_st.snoreIntervention.snoreIntervention_tmr);
            g_sysparam_st.snoreIntervention.triggered_time_s = g_sysparam_st.timer;
            g_sysparam_st.snoreIntervention.is_intervening = true;
        }
    }

    if (g_sysparam_st.snoreIntervention.is_intervening)
    {
        if (g_sysparam_st.timer - g_sysparam_st.snoreIntervention.triggered_time_s >= 30 * 60 * 100) // 30����
        {
            LOG_I("������Ԥ�������������½�\r\n");
			// prepare_mfp_SOFT_START(KEY_MEMORY4,g_sysparam_st.snoreIntervention.snoreIntervention_pwm,g_sysparam_st.snoreIntervention.snoreIntervention_tmr);
			g_sysparam_st.snoreIntervention.is_intervening = false;
		}
    }
}



void SnoringInterventStateClear(void)
{
	g_sysparam_st.sf.snoreIntervenNub = 0;
	g_sysparam_st.snoreIntervention.trig = 0;
	g_sysparam_st.snoreIntervention.is_intervening = false;
	// Ҫ��Ҫ�ϵ��ƽ��

}



