#include "x_snoreintervention.h"
#include "g.h"
#include "mfp_queue.h"
#include "x_uart.h"

/* 主控 MFP 同步帧首字节 length：协议约定仅 0x30 / 0x34 为支持缓启动(0x07)的主控盒 */
#define MFP_SYNC_LEN_SOFTSTART_0  0x30U
#define MFP_SYNC_LEN_SOFTSTART_1  0x34U

static bool mfp_mainbox_supports_snore_soft_start(void)
{
	uint8_t len = uart1_get_last_mfp_sync_len();
	return (len == MFP_SYNC_LEN_SOFTSTART_0 || len == MFP_SYNC_LEN_SOFTSTART_1);
}

#define AAS_WINDOW_5MIN     (5*60*100)
#define SI_PWM               (0x50) 		/* 80/255 */
#define SI_TMR               (0x15) 		/* 16+2  =18s */
#define SI_SEG_MAX            3U         	/* 分 3 段逼近约 15° */

static void SnoringInterventDetectionReset(void);

static void snore_lift_enqueue_pulse(uint8_t pwm)
{
	uint32_t key = (g_offline_voice.bed_type == 1U)? (uint32_t)(KEY_M1_OUT | KEY_M2_OUT): KEY_M1_OUT;	/* AB床  M1 M2 M3头部/脚部电机一起抬起*/

	prepare_mfp_SOFT_START(key, pwm, 1, 2);
}

/* 仅清段 tick 状态，不动 MFP 队列 */
void snore_lift_reset_state(void)
{
	g_sysparam_st.snoreIntervention.lift_seg_end_tick = 0U;
	g_sysparam_st.snoreIntervention.lift_seg_pwm = 0U;
}

/* 段结束：清 tick + 清队列 + 键值 0 释放（唯一清队列路径） */
static void snore_lift_stop_and_release(void)
{
	snore_lift_reset_state();
	mfp_tx_queue_clear();
	prepare_mfp_NORMAL_KET(0U, 3U);
}

uint8_t snore_lift_tmr_seg_seconds(void)
{
	uint8_t t = g_sysparam_st.snoreIntervention.snoreIntervention_tmr;
	uint8_t seg = (uint8_t)(t / 3U);
	return (seg == 0U) ? 1U : seg;
}

void snore_lift_start(uint8_t pwm, uint8_t tmr_seg_sec)
{
	uint32_t end;

	if (tmr_seg_sec == 0U) tmr_seg_sec = 1U;
	if (pwm == 0U) pwm = SI_PWM; 		/* 默认 80/255 */

	if (g_sysparam_st.snoreIntervention.lift_seg_end_tick != 0U)  snore_lift_stop_and_release(); 	/* 清段，不动队列 */
	end = g_sysparam_st.timer + (uint32_t)tmr_seg_sec * 100U;	/* 段结束 timer(10ms) */
	g_sysparam_st.snoreIntervention.lift_seg_pwm = pwm;			/* 本段 sustain 使用的 PWM */
	g_sysparam_st.snoreIntervention.lift_seg_end_tick = end;		/* 抬升段结束 timer(10ms) */
	snore_lift_enqueue_pulse(pwm);								/* 入队首包（段内补发/停止见 snore_lift_process_tick） */
	LOG_I("snore lift start pwm=%u tmr_seg=%us end_tick=%lu\r\n", (unsigned)pwm, (unsigned)tmr_seg_sec, (unsigned long)end);
}

void snore_lift_start_from_cfg(void)
{
	snore_lift_start(g_sysparam_st.snoreIntervention.snoreIntervention_pwm,
			 snore_lift_tmr_seg_seconds());
}

void snore_lift_process_tick(void)
{
	uint32_t end = g_sysparam_st.snoreIntervention.lift_seg_end_tick;

	if (end == 0U)
		return;

	if (g_sysparam_st.timer >= end) {
		LOG_I("snore lift end (timer)\r\n");
		snore_lift_stop_and_release();
		return;
	}

	if (mfp_tx_queue_is_empty())
		snore_lift_enqueue_pulse(g_sysparam_st.snoreIntervention.lift_seg_pwm);
}

static void snore_try_flat_tick(void)
{
	uint32_t first = g_sysparam_st.snoreIntervention.first_lift_tick;

	if (first == 0U)
		return;
	if ((g_sysparam_st.timer - first) < SNORE_FLAT_DELAY_TICKS)
		return;

	if (g_sysparam_st.snoreIntervention.lift_seg_end_tick != 0U)
		snore_lift_stop_and_release();

	LOG_I("snoreIntervention Down flat tick (delay=%u ticks)\r\n", (unsigned)SNORE_FLAT_DELAY_TICKS);
	prepare_mfp_SOFT_START(KEY_ALLFATE,
		(uint8_t)(g_sysparam_st.snoreIntervention.snoreIntervention_pwm),
		(uint8_t)(g_sysparam_st.snoreIntervention.snoreIntervention_tmr), 3U);
	g_sysparam_st.snoreIntervention.lift_stage = 0U;
	SnoringInterventFlatTimerClear();
	g_sysparam_st.snoreIntervention.is_intervening = false;
}

uint8_t read_AntiSnore_intensity(void)
{
	g_sysparam_st.snoreIntervention.enable = (g_sysparam_st.AntiSnore_intensity != 0);
	return g_sysparam_st.AntiSnore_intensity;
}

void SnoringInterventionInit(void)
{
	int intensity = read_AntiSnore_intensity();

	g_sysparam_st.last_AntiSnore_intensity = intensity;
	if (intensity >= 1 && intensity <= 3) {
		g_sysparam_st.snoreIntervention.trigTimer = AAS_WINDOW_5MIN;
	} else {
		g_sysparam_st.snoreIntervention.trigTimer = 0;
	}

	if (g_sysparam_st.snoreIntervention.snoreIntervention_pwm == 0U)
		g_sysparam_st.snoreIntervention.snoreIntervention_pwm = SI_PWM;
	if (g_sysparam_st.snoreIntervention.snoreIntervention_tmr == 0U)
		g_sysparam_st.snoreIntervention.snoreIntervention_tmr = SI_TMR;
	g_sysparam_st.snoreIntervention.enable = (intensity != 0);

	SnoringInterventStateClear();
	SnoringInterventFlatTimerClear();
}

void determine_intervention(void)
{
	uint8_t intensity = read_AntiSnore_intensity();
	bool trig = false;

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

void simulate_detection_period(void)
{
	if (g_sysparam_st.snoreIntervention.timer_s == 0 && g_sysparam_st.snoreIntervention.trigTimer > 0) {
		g_sysparam_st.snoreIntervention.timer_s = g_sysparam_st.timer;
		g_sysparam_st.snoreIntervention.oldsnoreNub = g_sysparam_st.sf.snoreNub;
	}

	if ((g_sysparam_st.snoreIntervention.timer_s != 0) &&
	    (g_sysparam_st.timer - g_sysparam_st.snoreIntervention.timer_s > g_sysparam_st.snoreIntervention.trigTimer)) {
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

	snore_try_flat_tick();
	snore_lift_process_tick();

	if (!g_sysparam_st.snoreIntervention.enable)
		return;

	if (!mfp_mainbox_supports_snore_soft_start()) {
		g_sysparam_st.snoreIntervention.is_intervening = false;
		g_sysparam_st.snoreIntervention.lift_stage = 0U;
		return;
	}

	if (intensity != g_sysparam_st.last_AntiSnore_intensity) {
		if (intensity >= 1 && intensity <= 3) {
			g_sysparam_st.snoreIntervention.trigTimer = AAS_WINDOW_5MIN;
		} else {
			g_sysparam_st.snoreIntervention.trigTimer = 0;
		}
		SnoringInterventDetectionReset();
		g_sysparam_st.last_AntiSnore_intensity = intensity;
		LOG_I("Snore intensity changed to %d, trigTimer=5min (lift cycle unchanged)\r\n", intensity);
	}

	simulate_detection_period();

	if (g_sysparam_st.snoreIntervention.trig == 1U) {
		uint8_t pwm = g_sysparam_st.snoreIntervention.snoreIntervention_pwm;
		uint8_t tmr_seg = snore_lift_tmr_seg_seconds();

		g_sysparam_st.snoreIntervention.trig = 0U;

		if (g_sysparam_st.snoreIntervention.triggered_flag &&
		    g_sysparam_st.snoreIntervention.lift_stage < SI_SEG_MAX) {
			LOG_I("snoreIntervention Up seg%u pwm=%u tmr_seg=%us\r\n",
			      (unsigned)g_sysparam_st.snoreIntervention.lift_stage + 1U,
			      (unsigned)pwm, (unsigned)tmr_seg);
			snore_lift_start(pwm, tmr_seg);
			/* 旧方案（部分主控非放平不可用）：prepare_mfp_SOFT_START(KEY_MEMORY4, pwm, tmr_seg, 3U); */
			if (g_sysparam_st.ble_testinfo.snore_intervention_trig < 0xFFFFU)
				g_sysparam_st.ble_testinfo.snore_intervention_trig++;
			g_sysparam_st.snoreIntervention.lift_stage++;
			if (g_sysparam_st.snoreIntervention.lift_stage == 1U) {
				g_sysparam_st.snoreIntervention.first_lift_tick = g_sysparam_st.timer;
				g_sysparam_st.snoreIntervention.triggered_time_s = g_sysparam_st.timer;
			}
			g_sysparam_st.snoreIntervention.is_intervening = true;
		}
	}
}

static void SnoringInterventDetectionReset(void)
{
	g_sysparam_st.sf.snoreIntervenNub = 0;
	g_sysparam_st.snoreIntervention.trig = 0;
	g_sysparam_st.snoreIntervention.triggered_flag = false;
	g_sysparam_st.sf.snorevolume_acc = 0;
	g_sysparam_st.sf.snorevolume_cnt = 0;
	g_sysparam_st.snoreIntervention.timer_s = 0;
	g_sysparam_st.snoreIntervention.oldsnoreNub = 0;
	g_sysparam_st.snoreIntervention.is_intervening = (g_sysparam_st.snoreIntervention.first_lift_tick != 0U);
}

void SnoringInterventStateClear(void)
{
	if (g_sysparam_st.snoreIntervention.lift_seg_end_tick != 0U)
		snore_lift_stop_and_release();
	else
		snore_lift_reset_state();
	SnoringInterventDetectionReset();
	g_sysparam_st.snoreIntervention.is_intervening = false;
	g_sysparam_st.snoreIntervention.lift_stage = 0U;
}

void SnoringInterventFlatTimerClear(void)
{
	g_sysparam_st.snoreIntervention.first_lift_tick = 0U;
}
