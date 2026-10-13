#include "x_snoreintervention.h"
#include "g.h"
#include "mfp_queue.h"
#include "x_uart.h"

#define SNORE_DBG_WINDOW_1MIN  (60U * 100U)
#define SNORE_DBG_WINDOW_5MIN  (5U * 60U * 100U)

#define SNORE_WINDOW_PROD_TICKS  (5U * 60U * 100U)  /* 5min */
#define SNORE_WINDOW_TEST_TICKS  (60U * 100U)       /* 60s */

#define SI_PWM               (0x55) 		/* 80/255     xiugai:0x50*/   
#define SI_TMR               (0x15) 		/* 16+2  =18s */
#define SI_SEG_MAX            3U         	/* 分 3 段逼近约 15° */
#define SNORE_NORMAL_SEG_SEC  3U         	/* 非缓启动主控：普通键值每段 xiugai :2s */

static uint32_t snore_dbg_1min_t0;
static uint16_t snore_dbg_1min_base;
static uint16_t snore_cnt_1min;
static uint32_t snore_dbg_5min_t0;
static uint16_t snore_dbg_5min_base;
static uint16_t snore_cnt_5min;

static void SnoringInterventDetectionReset(void);
static void snore_refresh_trig_timer(void);
uint8_t read_AntiSnore_intensity(void);

uint8_t snore_run_mode_get(void)
{
	return (g_sysparam_st.snoreIntervention.run_mode == SNORE_RUN_MODE_TEST)
		? SNORE_RUN_MODE_TEST : SNORE_RUN_MODE_PROD;
}

uint32_t snore_window_ticks(void)
{
	return (snore_run_mode_get() == SNORE_RUN_MODE_TEST)
		? SNORE_WINDOW_TEST_TICKS : SNORE_WINDOW_PROD_TICKS;
}

uint32_t snore_flat_delay_ticks(void)
{
	return (snore_run_mode_get() == SNORE_RUN_MODE_TEST)
		? SNORE_FLAT_DELAY_TICKS_TEST : SNORE_FLAT_DELAY_TICKS_PROD;
}

/* 窗内打鼾包阈值：量产 1/2/3 → 20/14/7；测试 1/2/3 → 7/5/3 (NEW 1302 4s一个打鼾包，阈值应修改为高25 中35 低45/ 总75)*/
static uint16_t snore_period_threshold(uint8_t intensity)
{
	if (snore_run_mode_get() == SNORE_RUN_MODE_TEST) {
		switch (intensity) {
		case 3: return 5U;
		case 2: return 6U;
		case 1: return 7U;
		default: return 0xFFFFU;
		}
	}
	switch (intensity) {
	case 3: return 7U;
	case 2: return 14U;
	case 1: return 20U;
	default: return 0xFFFFU;
	}
}

static void snore_refresh_trig_timer(void)
{
	uint8_t intensity = read_AntiSnore_intensity();

	if (intensity >= 1U && intensity <= 3U)
		g_sysparam_st.snoreIntervention.trigTimer = snore_window_ticks();
	else
		g_sysparam_st.snoreIntervention.trigTimer = 0U;
}

void snore_run_mode_set(uint8_t mode)
{
	uint8_t m = (mode == SNORE_RUN_MODE_TEST) ? SNORE_RUN_MODE_TEST : SNORE_RUN_MODE_PROD;

	if (g_sysparam_st.snoreIntervention.run_mode == m) {
		LOG_I("snore run_mode keep %u\r\n", (unsigned)m);
		return;
	}

	g_sysparam_st.snoreIntervention.run_mode = m;
	/* 切换：清检测窗、抬升段、周期计时，避免窗长/周期间隔串味 */
	SnoringInterventStateClear();
	SnoringInterventFlatTimerClear();
	snore_refresh_trig_timer();
	LOG_I("snore run_mode=%u win=%lu cycle=%lu\r\n",
	      (unsigned)m,
	      (unsigned long)snore_window_ticks(),
	      (unsigned long)snore_flat_delay_ticks());
}

static void snore_debug_window_tick(void)
{
	uint16_t nub = g_sysparam_st.sf.snoreNub;
	uint32_t t = g_sysparam_st.timer;

	if (snore_dbg_1min_t0 == 0U) {
		snore_dbg_1min_t0 = t;
		snore_dbg_1min_base = nub;
	} else if ((t - snore_dbg_1min_t0) >= SNORE_DBG_WINDOW_1MIN) {
		snore_dbg_1min_t0 = t;
		snore_dbg_1min_base = nub;
	}
	snore_cnt_1min = nub - snore_dbg_1min_base;

	if (snore_dbg_5min_t0 == 0U) {
		snore_dbg_5min_t0 = t;
		snore_dbg_5min_base = nub;
	} else if ((t - snore_dbg_5min_t0) >= SNORE_DBG_WINDOW_5MIN) {
		snore_dbg_5min_t0 = t;
		snore_dbg_5min_base = nub;
	}
	snore_cnt_5min = nub - snore_dbg_5min_base;
}

void snore_debug_counts_get(uint16_t *total, uint16_t *cnt_1min, uint16_t *cnt_5min)
{
	if (total != NULL)
		*total = g_sysparam_st.sf.snoreNub;
	if (cnt_1min != NULL)
		*cnt_1min = snore_cnt_1min;
	if (cnt_5min != NULL)
		*cnt_5min = snore_cnt_5min;
}

static void snore_lift_enqueue_pulse(uint8_t pwm)
{
	uint32_t key = (g_offline_voice.bed_type == 1U)? (uint32_t)(KEY_M1_OUT | KEY_M2_OUT): KEY_M1_OUT;	/* AB床：M1、M2 头部电机一起抬起 */

	if (mfp_mainbox_supports_soft_start())
		prepare_mfp_SOFT_START(key, pwm, 1, 2);
	else
		prepare_mfp_NORMAL_KET(key, 2U);
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

	if (mfp_mainbox_supports_soft_start()) {
		if (tmr_seg_sec == 0U) tmr_seg_sec = 1U;
		if (pwm == 0U) pwm = SI_PWM; 		/* 默认 80/255 */
	} else {
		tmr_seg_sec = SNORE_NORMAL_SEG_SEC;
	}

	if (g_sysparam_st.snoreIntervention.lift_seg_end_tick != 0U)  snore_lift_stop_and_release(); 	/* 上一段未结束：先停段并释放 */
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

/* 周期到期：不自动放平，清抬升段数与周期计时，允许再次触发打鼾干预 */
static void snore_try_cycle_reset_tick(void)
{
	uint32_t first = g_sysparam_st.snoreIntervention.first_lift_tick;
	uint32_t delay = snore_flat_delay_ticks();

	if (first == 0U)
		return;
	if ((g_sysparam_st.timer - first) < delay)
		return;

	LOG_I("snoreIntervention cycle reset (delay=%lu ticks mode=%u, no auto-flat)\r\n",
	      (unsigned long)delay, (unsigned)snore_run_mode_get());
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

	/* 上电强制量产模式（BSS 虽为 0，显式赋值更清晰） */
	g_sysparam_st.snoreIntervention.run_mode = SNORE_RUN_MODE_PROD;
	g_sysparam_st.last_AntiSnore_intensity = intensity;
	snore_refresh_trig_timer();

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
	uint16_t thr = snore_period_threshold(intensity);
	bool trig = (thr != 0xFFFFU) &&
		    (g_sysparam_st.sf.snoreNubperiod >= thr);

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

		LOG_I("AAS mode=%u intensity=%d snoreNubperiod=%d thr=%u\r\n",
		      (unsigned)snore_run_mode_get(),
		      g_sysparam_st.AntiSnore_intensity,
		      g_sysparam_st.sf.snoreNubperiod,
		      (unsigned)snore_period_threshold((uint8_t)g_sysparam_st.AntiSnore_intensity));

		g_sysparam_st.sf.snorevolume_acc = 0;
		g_sysparam_st.sf.snorevolume_cnt = 0;

		determine_intervention();
	}
}

void SnoringIntervention_run(void)
{
	uint8_t intensity = read_AntiSnore_intensity();

	snore_debug_window_tick(); /* 仅维护计数；MFP 不发 AA55，BLE 3.4.3 查询时再上报 */

	g_sysparam_st.snoreIntervention.enable = (intensity != 0);

	snore_try_cycle_reset_tick();
	snore_lift_process_tick();

	if (!g_sysparam_st.snoreIntervention.enable)
		return;

	if (intensity != g_sysparam_st.last_AntiSnore_intensity) {
		snore_refresh_trig_timer();
		SnoringInterventDetectionReset();
		g_sysparam_st.last_AntiSnore_intensity = intensity;
		LOG_I("Snore intensity changed to %d, trigTimer=%lu (mode=%u)\r\n",
		      intensity,
		      (unsigned long)g_sysparam_st.snoreIntervention.trigTimer,
		      (unsigned)snore_run_mode_get());
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
