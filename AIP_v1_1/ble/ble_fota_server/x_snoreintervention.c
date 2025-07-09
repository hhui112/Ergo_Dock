#include "x_faultDetect.h"
#include "x_snoreintervention.h"
#include "g.h"

#define SNORETIMER     2*60   		// (5*60)			//(15*60) 5���Ӵ���һ�δ�����Ԥ���
#define SI_TH					(1)				// ��Ԥ��ֵ
#define SI_PWM				(0x32)
#define SI_TMR				(0x40)
#define UP_HOLD_TIMES	 30*60				// (30*60)

// ������Ԥǿ�Ⱥ�ʹ��
int read_AntiSnore_intensity(void) 
{	
		g_sysparam_st.snoreIntervention.enable = (g_sysparam_st.AntiSnore_intensity != 0);
    return g_sysparam_st.AntiSnore_intensity;
}

// �б��Ԥ�ֶ�
void determine_intervention(void) 
{
		int intensity = read_AntiSnore_intensity();
		bool trig = false;
	
		 switch (intensity) {
				case 3:		// �ߣ� 2�����ڴ������� > 4  �� �������������� 500  �ж�Ϊ����
						trig = (g_sysparam_st.sf.snoreNubperiod > 4 && g_sysparam_st.sf.snorevolume_avg > 520);
						break;
				case 2:		// �У�2�����ڴ������� > 7  �� �������������� 600  �ж�Ϊ����
						trig = (g_sysparam_st.sf.snoreNubperiod > 7 && g_sysparam_st.sf.snorevolume_avg > 550);
						break;
				case 1:		// �ͣ�2�����ڴ������� > 10  �� �������������� 700  �ж�Ϊ����
						trig = (g_sysparam_st.sf.snoreNubperiod > 10 && g_sysparam_st.sf.snorevolume_avg > 600);
						break;
				default:
						trig = false;
						break;
		}
		g_sysparam_st.snoreIntervention.triggered_flag = trig;
}

//5����һ��  : ��¼���ڼ�Ĵ������� g_sysparam_st.sf.snoreNub
void simulate_detection_period(void) 
{
	static uint32_t timer_s;
	static uint32_t oldsnoreNub;
	
	if(timer_s == 0 && g_sysparam_st.snoreIntervention.trigTimer > 0)
	{
		timer_s =  g_sysparam_st.timer;
		oldsnoreNub = g_sysparam_st.sf.snoreNub;
	}
	if( (timer_s!=0) && (g_sysparam_st.timer - timer_s > g_sysparam_st.snoreIntervention.trigTimer))
	{
		timer_s = 0;
		g_sysparam_st.snoreIntervention.trig = 1;
		g_sysparam_st.sf.snoreNubperiod = g_sysparam_st.sf.snoreNub - oldsnoreNub;

		
				// ����ƽ������
		if (g_sysparam_st.sf.snorevolume_cnt > 0) {
				g_sysparam_st.sf.snorevolume_avg = g_sysparam_st.sf.snorevolume_acc / g_sysparam_st.sf.snorevolume_cnt;
		} else {
				g_sysparam_st.sf.snorevolume_avg = 0;
		}
		
		LOG_I("AntiSnore_intensity =%d, snoreNubperiod = %d, snorevolume_avg = %d\r\n",g_sysparam_st.AntiSnore_intensity,g_sysparam_st.sf.snoreNubperiod,g_sysparam_st.sf.snorevolume_avg);
		
		 g_sysparam_st.sf.snorevolume_acc = 0;
     g_sysparam_st.sf.snorevolume_cnt = 0;
		
		 determine_intervention();  // �ж��Ƿ���Ҫ��Ԥ
	}		
}

void SnoringInterventionInit(void)
{
	g_sysparam_st.snoreIntervention.trigTimer = SNORETIMER*100;	// 5����
	g_sysparam_st.snoreIntervention.snoreIntervention_pwm = SI_PWM;
	g_sysparam_st.snoreIntervention.snoreIntervention_tmr = SI_TMR;
	g_sysparam_st.snoreIntervention.snoreIntervention_threshold = SI_TH;
	g_sysparam_st.snoreIntervention.enable = false;

	SnoringInterventStateClear();
}


void SnoringIntervention_run(void) /* 50-100ms����һ�� */
{
		g_sysparam_st.snoreIntervention.enable = (g_sysparam_st.AntiSnore_intensity != 0);
    if (!g_sysparam_st.snoreIntervention.enable) return; 

    simulate_detection_period();  // ��¼���ڴ����������ж��Ƿ���Ҫ��Ԥ

    if (g_sysparam_st.snoreIntervention.trig == 1) // ���ڽ������жϽ��
    {
        g_sysparam_st.snoreIntervention.trig = 0;

        if (!g_sysparam_st.snoreIntervention.is_intervening && g_sysparam_st.snoreIntervention.triggered_flag)
        {
            LOG_I("snoreIntervention Up\r\n");
            prepare_mfp_SOFT_START(KEY_MEMORY4, g_sysparam_st.snoreIntervention.snoreIntervention_pwm, g_sysparam_st.snoreIntervention.snoreIntervention_tmr,3);  //KEY_MEMORY4 
            g_sysparam_st.snoreIntervention.triggered_time_s = g_sysparam_st.timer;
            g_sysparam_st.snoreIntervention.is_intervening = true;
        }
    }

    if (g_sysparam_st.snoreIntervention.is_intervening)
    {
        if (g_sysparam_st.timer - g_sysparam_st.snoreIntervention.triggered_time_s >= UP_HOLD_TIMES * 100) // 30����
        {
            LOG_I("snoreIntervention Down \r\n");
						prepare_mfp_SOFT_START(KEY_ALLFATE,g_sysparam_st.snoreIntervention.snoreIntervention_pwm,g_sysparam_st.snoreIntervention.snoreIntervention_tmr+5,3);
						g_sysparam_st.snoreIntervention.is_intervening = false;
			}
    }
}


void SnoringInterventStateClear(void)
{
	g_sysparam_st.sf.snoreIntervenNub = 0;
	g_sysparam_st.snoreIntervention.trig = 0;
	g_sysparam_st.snoreIntervention.is_intervening = false;
	g_sysparam_st.sf.snorevolume_acc = 0;
  g_sysparam_st.sf.snorevolume_cnt = 0;

}



