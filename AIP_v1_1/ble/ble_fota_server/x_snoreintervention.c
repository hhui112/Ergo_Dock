#include "x_faultDetect.h"
#include "x_snoreintervention.h"
#include "g.h"

#define SNORETIMER     2*60   		// (5*60)			//(15*60) 5���Ӵ���һ�δ�����Ԥ���
#define SI_TH					(1)					// ��Ԥ��ֵ
#define SI_PWM				(0x32)
#define SI_TMR				(0x40)			// 0x40
#define UP_HOLD_TIMES	 30*60				// (30*60)


// ������Ԥǿ�Ⱥ�ʹ��
uint8_t read_AntiSnore_intensity(void) 
{	
		g_sysparam_st.snoreIntervention.enable = (g_sysparam_st.AntiSnore_intensity != 0);
    return g_sysparam_st.AntiSnore_intensity;
}

void SnoringInterventionInit(void)
{
    int intensity = read_AntiSnore_intensity();
		g_sysparam_st.last_AntiSnore_intensity = intensity;
    if (intensity == 3) {
        g_sysparam_st.snoreIntervention.trigTimer = 2*60*100;  
    } else if (intensity == 2) {
        g_sysparam_st.snoreIntervention.trigTimer = 5*60*100;  
    } else if (intensity == 1) {
        g_sysparam_st.snoreIntervention.trigTimer = 8*60*100; 
    } else {
        g_sysparam_st.snoreIntervention.trigTimer = 0;
    }

    g_sysparam_st.snoreIntervention.snoreIntervention_pwm = SI_PWM;
    g_sysparam_st.snoreIntervention.snoreIntervention_tmr = SI_TMR;
    g_sysparam_st.snoreIntervention.snoreIntervention_threshold = SI_TH;
    g_sysparam_st.snoreIntervention.enable = false;

    SnoringInterventStateClear();
}


// �б��Ԥ�ֶ�
void determine_intervention(void) 
{
    uint8_t intensity = read_AntiSnore_intensity();
    bool trig = false;

    switch (intensity) {
        case 3: 
            trig = (g_sysparam_st.sf.snoreNubperiod >= 10);
            break;
        case 2: 
            trig = (g_sysparam_st.sf.snoreNubperiod >= 25);
            break;
        case 1: 
            trig = (g_sysparam_st.sf.snoreNubperiod >= 40);
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
	if (g_sysparam_st.snoreIntervention.timer_s == 0 && g_sysparam_st.snoreIntervention.trigTimer > 0) {
			g_sysparam_st.snoreIntervention.timer_s = g_sysparam_st.timer;
			g_sysparam_st.snoreIntervention.oldsnoreNub = g_sysparam_st.sf.snoreNub;
	}
		
	if( (g_sysparam_st.snoreIntervention.timer_s!=0) && (g_sysparam_st.timer - g_sysparam_st.snoreIntervention.timer_s > g_sysparam_st.snoreIntervention.trigTimer))
	{
		g_sysparam_st.snoreIntervention.timer_s = 0;
		g_sysparam_st.snoreIntervention.trig = 1;
		g_sysparam_st.sf.snoreNubperiod = g_sysparam_st.sf.snoreNub - g_sysparam_st.snoreIntervention.oldsnoreNub;

		
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


void SnoringIntervention_run(void) /* 50-100ms����һ�� */
{
	
    uint8_t intensity = read_AntiSnore_intensity();

    g_sysparam_st.snoreIntervention.enable = (intensity != 0);
    if (!g_sysparam_st.snoreIntervention.enable) return;
	
    if (intensity != g_sysparam_st.last_AntiSnore_intensity) {
        if (intensity == 3) {
            g_sysparam_st.snoreIntervention.trigTimer = 2*60*100;
        } else if (intensity == 2) {
            g_sysparam_st.snoreIntervention.trigTimer = 5*60*100;
        } else if (intensity == 1) {
            g_sysparam_st.snoreIntervention.trigTimer = 8*60*100;
        } else {
            g_sysparam_st.snoreIntervention.trigTimer = 0;
        }

        SnoringInterventStateClear();
				g_sysparam_st.last_AntiSnore_intensity = intensity; 
        LOG_I("Snore intensity changed to %d, new trigTimer=%d\r\n", intensity, g_sysparam_st.snoreIntervention.trigTimer);
    }
		
		
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
						prepare_mfp_SOFT_START(KEY_ALLFATE,g_sysparam_st.snoreIntervention.snoreIntervention_pwm+5,g_sysparam_st.snoreIntervention.snoreIntervention_tmr+10,3);
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
	g_sysparam_st.snoreIntervention.timer_s = 0;
	g_sysparam_st.snoreIntervention.oldsnoreNub = 0;
}



