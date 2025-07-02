#include "x_faultDetect.h"
#include "x_snoreintervention.h"
#include "g.h"

#define SNORETIMER  (5*60)			//(15*60) 5分钟触发一次打鼾干预检查
#define SNORELIMITPA  (2500)
#define SI_TH					(5)				// 干预阈值
#define SI_PWM				(0x80)
#define SI_TMR				(0x50)


// 气囊压力读取函数，这里使用当前压力值作为返回值，模拟实际读取
int read_air_cushion_pressure(void) 
{
    return g_sysparam_st.airpump.pa_cur;
}

//5分钟一次  : 记录这期间的打鼾次数 g_sysparam_st.sf.snoreNub
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

// 判别干预手段
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
	g_sysparam_st.snoreIntervention.trigTimer = SNORETIMER*100;	// 5分钟
	g_sysparam_st.snoreIntervention.snoreIntervention_pwm = SI_PWM;
	g_sysparam_st.snoreIntervention.snoreIntervention_tmr = SI_TMR;
	g_sysparam_st.snoreIntervention.snoreIntervention_threshold = SI_TH;

	SnoringInterventStateClear();
}


void SnoringIntervention_run(void) /* 50-100ms进入一次 */
{
    if (!g_sysparam_st.snoreIntervention.enable) return;

    simulate_detection_period();  // 记录窗口打鼾次数

    if (g_sysparam_st.snoreIntervention.trig == 1) // 窗口结束，判断结果
    {
        g_sysparam_st.snoreIntervention.trig = 0;
        determine_intervention();  // 判断是否需要干预

        if (!g_sysparam_st.snoreIntervention.is_intervening && g_sysparam_st.snoreIntervention.triggered_flag)
        {
            LOG_I("打鼾干预开始：缓启动抬升\r\n");
            // prepare_mfp_SOFT_START(KEY_MEMORY4, g_sysparam_st.snoreIntervention.snoreIntervention_pwm, g_sysparam_st.snoreIntervention.snoreIntervention_tmr);
            g_sysparam_st.snoreIntervention.triggered_time_s = g_sysparam_st.timer;
            g_sysparam_st.snoreIntervention.is_intervening = true;
        }
    }

    if (g_sysparam_st.snoreIntervention.is_intervening)
    {
        if (g_sysparam_st.timer - g_sysparam_st.snoreIntervention.triggered_time_s >= 30 * 60 * 100) // 30分钟
        {
            LOG_I("打鼾干预结束：缓启动下降\r\n");
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
	// 要不要上电放平？

}



