#ifndef X_SNOREINTERVENTION
#define X_SNOREINTERVENTION

#include "stdint.h"

/* 运行模式（仅 RAM；上电/复位为量产） */
#define SNORE_RUN_MODE_PROD  0U  /* 量产：5min 窗、阈值 20/14/7、8h 周期后可再抬升（不自动放平） */
#define SNORE_RUN_MODE_TEST  1U  /* 测试：60s 窗、阈值 7/5/3、1min 周期后可再抬升（不自动放平） */

/* 干预周期（g_sysparam_st.timer 单位 10ms）；到期清 lift_stage 允许再抬升，不发放平；运行时请用 snore_flat_delay_ticks() */
#define SNORE_FLAT_DELAY_TICKS_PROD  (8U * 60U * 60U * 100U)
#define SNORE_FLAT_DELAY_TICKS_TEST  (60U * 100U)
#define SNORE_FLAT_DELAY_TICKS       SNORE_FLAT_DELAY_TICKS_PROD

void SnoringInterventionInit(void);
/* 清干预检测/抬升段；不取消 first_lift_tick（周期计时保留） */
void SnoringInterventStateClear(void);
/* 仅取消周期计时（Init、语音/手动放平、周期到期后） */
void SnoringInterventFlatTimerClear(void);
void SnoringIntervention_run(void);

/* 当前运行模式（0=量产 1=测试） */
uint8_t snore_run_mode_get(void);
/* 设置运行模式；切换时清检测窗/抬升段/周期计时，并刷新 trigTimer；不写 Flash */
void snore_run_mode_set(uint8_t mode);
/* 当前检测窗长度（10ms tick）：量产 5min / 测试 60s */
uint32_t snore_window_ticks(void);
/* 当前干预周期（10ms tick）：量产 8h / 测试 1min；到期不放平 */
uint32_t snore_flat_delay_ticks(void);

/* 每段抬升时长（秒）= APP/Flash TMR / 3 */
uint8_t snore_lift_tmr_seg_seconds(void);

/* 触发一段 M1_OUT 抬升：设 timer 窗口并入队首包（段内补发/停止见 snore_lift_process_tick） */
void snore_lift_start(uint8_t pwm, uint8_t tmr_seg_sec);

/* 用 Flash/BLE 配置的 pwm/tmr 触发一段（语音 0x24 标定） */
void snore_lift_start_from_cfg(void);

/* 100ms 周期：窗口内队列为空则补发 M1_OUT；到点清队列并 stop */
void snore_lift_process_tick(void);

/* 仅清 lift_seg_end_tick / lift_seg_pwm，不动 MFP 队列、不发 stop */
void snore_lift_reset_state(void);

/* 读取打鼾包统计（供 BLE 3.4.3 应答；内部 uint16，应答时饱和至 0xFF） */
void snore_debug_counts_get(uint16_t *total, uint16_t *cnt_1min, uint16_t *cnt_5min);

#endif
