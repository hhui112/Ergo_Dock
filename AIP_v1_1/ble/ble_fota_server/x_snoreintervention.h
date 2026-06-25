#ifndef X_SNOREINTERVENTION
#define X_SNOREINTERVENTION

#include "stdint.h"

/* 首次抬升后放平间隔（g_sysparam_st.timer 单位 10ms）；量产 8*3600*100 8小时放平*/
#define SNORE_FLAT_DELAY_TICKS  (8U * 60U * 60U * 100U)

void SnoringInterventionInit(void);
/* 清干预检测/抬升段；不取消 first_lift_tick（8h 放平计时保留） */
void SnoringInterventStateClear(void);
/* 仅取消 8h 放平计时（Init、语音/手动放平、自动放平成功后） */
void SnoringInterventFlatTimerClear(void);
void SnoringIntervention_run(void);

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

#endif
