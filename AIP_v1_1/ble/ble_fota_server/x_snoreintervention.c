#include "x_faultDetect.h"
#include "x_snoreintervention.h"
#include "g.h"

#define SNORETIMER     2*60   		// (5*60)			//(15*60) 5分钟触发一次打鼾干预检查
#define SI_TH					(1)				// 干预阈值
#define SI_PWM				(0x32)
#define SI_TMR				(0x40)
#define UP_HOLD_TIMES	 30*60				// (30*60)

// 读：干预强度和使能
int read_AntiSnore_intensity(void) 
{	
		g_sysparam_st.snoreIntervention.enable = (g_sysparam_st.AntiSnore_intensity != 0);
    return g_sysparam_st.AntiSnore_intensity;
}

// 判别干预手段
void determine_intervention(void) 
{
		int intensity = read_AntiSnore_intensity();
		bool trig = false;
	
		 switch (intensity) {
				case 3:		// 高： 2分钟内打鼾次数 > 4  且 打鼾包声音大于 500  判断为打鼾
						trig = (g_sysparam_st.sf.snoreNubperiod > 4 && g_sysparam_st.sf.snorevolume_avg > 520);
						break;
				case 2:		// 中：2分钟内打鼾次数 > 7  且 打鼾包声音大于 600  判断为打鼾
						trig = (g_sysparam_st.sf.snoreNubperiod > 7 && g_sysparam_st.sf.snorevolume_avg > 550);
						break;
				case 1:		// 低：2分钟内打鼾次数 > 10  且 打鼾包声音大于 700  判断为打鼾
						trig = (g_sysparam_st.sf.snoreNubperiod > 10 && g_sysparam_st.sf.snorevolume_avg > 600);
						break;
				default:
						trig = false;
						break;
		}
		g_sysparam_st.snoreIntervention.triggered_flag = trig;
}

//5分钟一次  : 记录这期间的打鼾次数 g_sysparam_st.sf.snoreNub
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

		
				// 计算平均音量
		if (g_sysparam_st.sf.snorevolume_cnt > 0) {
				g_sysparam_st.sf.snorevolume_avg = g_sysparam_st.sf.snorevolume_acc / g_sysparam_st.sf.snorevolume_cnt;
		} else {
				g_sysparam_st.sf.snorevolume_avg = 0;
		}
		
		LOG_I("AntiSnore_intensity =%d, snoreNubperiod = %d, snorevolume_avg = %d\r\n",g_sysparam_st.AntiSnore_intensity,g_sysparam_st.sf.snoreNubperiod,g_sysparam_st.sf.snorevolume_avg);
		
		 g_sysparam_st.sf.snorevolume_acc = 0;
     g_sysparam_st.sf.snorevolume_cnt = 0;
		
		 determine_intervention();  // 判断是否需要干预
	}		
}

void SnoringInterventionInit(void)
{
	g_sysparam_st.snoreIntervention.trigTimer = SNORETIMER*100;	// 5分钟
	g_sysparam_st.snoreIntervention.snoreIntervention_pwm = SI_PWM;
	g_sysparam_st.snoreIntervention.snoreIntervention_tmr = SI_TMR;
	g_sysparam_st.snoreIntervention.snoreIntervention_threshold = SI_TH;
	g_sysparam_st.snoreIntervention.enable = false;

	SnoringInterventStateClear();
}


void SnoringIntervention_run(void) /* 50-100ms进入一次 */
{
		g_sysparam_st.snoreIntervention.enable = (g_sysparam_st.AntiSnore_intensity != 0);
    if (!g_sysparam_st.snoreIntervention.enable) return; 

    simulate_detection_period();  // 记录窗口打鼾次数并判断是否需要干预

    if (g_sysparam_st.snoreIntervention.trig == 1) // 窗口结束，判断结果
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
        if (g_sysparam_st.timer - g_sysparam_st.snoreIntervention.triggered_time_s >= UP_HOLD_TIMES * 100) // 30分钟
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



