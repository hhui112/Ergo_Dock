#include "x_snoreintervention.h"
#include "g.h"

/* AAS: rolling 5min detection window (timer unit 10ms) */
#define AAS_WINDOW_5MIN     (5*60*100)
#define SI_PWM               (0x32)
#define SI_TMR               (0x40)
#define UP_HOLD_TIMES        (30*60)     /* hold 30 min */
#define AAS_WINDOW_8H_SEC    (8*3600)     /* 8h window for future 15deg cap, use UTC */


/* read intensity and sync enable */
uint8_t read_AntiSnore_intensity(void) 
{	
		g_sysparam_st.snoreIntervention.enable = (g_sysparam_st.AntiSnore_intensity != 0);
    return g_sysparam_st.AntiSnore_intensity;
}

void SnoringInterventionInit(void)
{
    int intensity = read_AntiSnore_intensity();
    g_sysparam_st.last_AntiSnore_intensity = intensity;
    /* fixed 5 min window */
    if (intensity >= 1 && intensity <= 3) {
        g_sysparam_st.snoreIntervention.trigTimer = AAS_WINDOW_5MIN;
    } else {
        g_sysparam_st.snoreIntervention.trigTimer = 0;
    }

    g_sysparam_st.snoreIntervention.snoreIntervention_pwm = SI_PWM;
    g_sysparam_st.snoreIntervention.snoreIntervention_tmr = SI_TMR;
    g_sysparam_st.snoreIntervention.enable = false;

    SnoringInterventStateClear();
}


/* decide if trigger intervention by threshold */
void determine_intervention(void) 
{
    uint8_t intensity = read_AntiSnore_intensity();
    bool trig = false;

    /* intensity1: 5min 20, intensity2: 5min 15, intensity3: 5min 10 */
    switch (intensity) {
        case 3:
            trig = (g_sysparam_st.sf.snoreNubperiod >= 7);
            break;
        case 2:
            trig = (g_sysparam_st.sf.snoreNubperiod >= 14);
            break;
        case 1:
            trig = (g_sysparam_st.sf.snoreNubperiod >= 20);
            break;
        default:
            trig = false;
            break;
    }

    g_sysparam_st.snoreIntervention.triggered_flag = trig;
}

/* rolling 5min: count snoreNub in window, judge at window end */
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

		LOG_I("AAS intensity=%d snoreNubperiod=%d\r\n", g_sysparam_st.AntiSnore_intensity, g_sysparam_st.sf.snoreNubperiod);

		g_sysparam_st.sf.snorevolume_acc = 0;
		g_sysparam_st.sf.snorevolume_cnt = 0;
		
		 determine_intervention();
	}		
}


void SnoringIntervention_run(void)
{
	uint8_t intensity = read_AntiSnore_intensity();
	g_sysparam_st.snoreIntervention.enable = (intensity != 0);
	if (!g_sysparam_st.snoreIntervention.enable) return;

	/* 8h window reset by UTC: for future 15deg cap within 8h */
	{
		uint32_t utc = x_time_RTC_ToUTC();
		uint32_t block8h = utc / AAS_WINDOW_8H_SEC;
		static uint32_t s_aas_8h_block_id = 0;
		if (block8h != s_aas_8h_block_id) {
			s_aas_8h_block_id = block8h;
			/* 8h passed, window reset; future: reset cumulative 15deg here */
		}
	}

	if (intensity != g_sysparam_st.last_AntiSnore_intensity) {
		/* fixed 5 min */
		if (intensity >= 1 && intensity <= 3) {
			g_sysparam_st.snoreIntervention.trigTimer = AAS_WINDOW_5MIN;
		} else {
			g_sysparam_st.snoreIntervention.trigTimer = 0;
		}
		SnoringInterventStateClear();
		g_sysparam_st.last_AntiSnore_intensity = intensity;
		LOG_I("Snore intensity changed to %d, trigTimer=5min\r\n", intensity);
	}

	simulate_detection_period();

	if (g_sysparam_st.snoreIntervention.trig == 1) {
		g_sysparam_st.snoreIntervention.trig = 0;
		if (!g_sysparam_st.snoreIntervention.is_intervening && g_sysparam_st.snoreIntervention.triggered_flag) {
			LOG_I("snoreIntervention Up\r\n");
			prepare_mfp_SOFT_START(KEY_MEMORY4, g_sysparam_st.snoreIntervention.snoreIntervention_pwm,
				g_sysparam_st.snoreIntervention.snoreIntervention_tmr, 3);
			g_sysparam_st.snoreIntervention.triggered_time_s = g_sysparam_st.timer;
			g_sysparam_st.snoreIntervention.is_intervening = true;
		}
	}

	if (g_sysparam_st.snoreIntervention.is_intervening) {
		if (g_sysparam_st.timer - g_sysparam_st.snoreIntervention.triggered_time_s >= UP_HOLD_TIMES * 100) {
			LOG_I("snoreIntervention Down\r\n");
			prepare_mfp_SOFT_START(KEY_ALLFATE, g_sysparam_st.snoreIntervention.snoreIntervention_pwm + 5,
				g_sysparam_st.snoreIntervention.snoreIntervention_tmr + 10, 3);
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



