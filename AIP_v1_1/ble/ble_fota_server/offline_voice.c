#include "x_faultDetect.h"
#include "offline_voice.h"
#include "mfp_queue.h"
#include "g.h"
#include "x_snoreintervention.h"
#include "app_led_ctrl.h"

/* 3.5 精确计数：cmd 与 voice_detail[] 下标一一对应 0x21→[0] … 0x38→[23] */
static void ble_testinfo_voice_detail_inc(uint8_t cmd)
{
	if (cmd < 0x21U || cmd > 0x38U)
		return;
	uint8_t *p = &g_sysparam_st.ble_testinfo.voice_detail[cmd - 0x21U];
	if (*p < 0xFFU)
		(*p)++;
}

/* 首次从 DISABLED 识别唤醒词时调用；与 dataHandle 内 0x21/0x22 互斥，同一帧不会两处都计 */
void offline_voice_wake_up(void)
{
	LOG_I("V wake_up");
	if (g_sysparam_st.ble_testinfo.offline_voice_wake < 0xFFFFU)
		g_sysparam_st.ble_testinfo.offline_voice_wake++;
	if (g_offline_voice.wake_word == Hello_Ergo)
		ble_testinfo_voice_detail_inc(0x21U);
	else if (g_offline_voice.wake_word == Hello_Bed)
		ble_testinfo_voice_detail_inc(0x22U);
	led_voice_wakeup();
}

void offline_voice_wake_off(void)
{
	LOG_I("V wake_off");
	g_offline_voice.enabled = false;
	g_offline_voice.last_control_cmd = 0U;
	led_voice_close_flash();
}

/* 床体位置指令：停干预检测/抬升段，8h 放平计时保留；语音放平(0x26/0x33)另取消计时 */
static void snore_intervention_cancel_on_voice_position(uint8_t cmd)
{
    switch (cmd) {
    case 0x25: case 0x26: case 0x27: case 0x28: case 0x29:
    case 0x2A: case 0x2B: case 0x2C: case 0x33: case 0x34:
    case 0x35: case 0x36: case 0x37: case 0x38:
        SnoringInterventStateClear();
        if (cmd == 0x26U || cmd == 0x33U)
            SnoringInterventFlatTimerClear();
        break;
    default:
        break;
    }
}

/* Map CI1302 voice command byte (data) to MFP keys after wake word matched. */
void offline_voice_dataHandle(uint8_t cmd)
{
	/* 仅 ACTIVE 后再次说唤醒词走这里；首次唤醒只走 offline_voice_wake_up() */
	if (cmd == 0x21 || cmd == 0x22) {
		ble_testinfo_voice_detail_inc(cmd);
		LOG_I("V wake_word 0x%02x", cmd);
		if (g_sysparam_st.ble_testinfo.offline_voice_wake < 0xFFFFU)
			g_sysparam_st.ble_testinfo.offline_voice_wake++;
		led_voice_wakeup();
		return;
	}

	ble_testinfo_voice_detail_inc(cmd);

	g_offline_voice.last_control_cmd = cmd;

	LOG_I("V cmd 0x%02x", cmd);
	led_voice_command_confirm();

	if (g_sysparam_st.ble_testinfo.offline_voice_resp < 0xFFFFU)
		g_sysparam_st.ble_testinfo.offline_voice_resp++;

	// 使用离线语音的时候，清除打鼾干预状态（可删除）
	snore_intervention_cancel_on_voice_position(cmd);
	
	switch (cmd) {
	case 0x23:
		mfp_tx_queue_clear();
		prepare_mfp_NORMAL_KET(KEY_MASSAGE_STOP_ALL, 1);
		break;
	case 0x24:
		if (g_offline_voice.bed_type == 1)  /* AB床  M1 M2 M3头部/脚部电机一起抬起*/
			prepare_mfp_NORMAL_KET((KEY_M1_OUT | KEY_M2_OUT | KEY_M3_OUT), 30);
		else
			prepare_mfp_NORMAL_KET((KEY_M1_OUT | KEY_M2_OUT), 30);
			/* 标定：与打鼾相同 timer+M1_OUT 抬升一段（pwm/tmr 取自 Flash/BLE 配置） */
			//snore_lift_start_from_cfg();
		break;
	case 0x25:
		prepare_mfp_NORMAL_KET(KEY_ZEROG, 3);
		break;
	case 0x26:
		mfp_tx_queue_clear();
		prepare_mfp_NORMAL_KET(KEY_ALLFATE, 3);
		break;
	case 0x27:
		prepare_mfp_NORMAL_KET(KEY_MEMORY5, 3);
		break;
	case 0x28:
		prepare_mfp_NORMAL_KET(KEY_MEMORY3, 3);
		break;
	case 0x29:
	    if(g_offline_voice.bed_type == 1)  /* AB床  M1 M2都为头部电机*/
			prepare_mfp_NORMAL_KET(KEY_M1_OUT | KEY_M2_OUT, 15);
		else
			prepare_mfp_NORMAL_KET(KEY_M1_OUT, 15);
		break;
	case 0x2A:
		if(g_offline_voice.bed_type == 1)  /* AB床  M1 M2都为头部电机*/
			prepare_mfp_NORMAL_KET(KEY_M1_IN | KEY_M2_IN, 15);
		else
			prepare_mfp_NORMAL_KET(KEY_M1_IN, 15);
		break;
	case 0x2B:
		if(g_offline_voice.bed_type == 1)  /* AB床  M3为脚部电机*/
			prepare_mfp_NORMAL_KET(KEY_M3_OUT, 15);
		else
			prepare_mfp_NORMAL_KET(KEY_M2_OUT, 15);
		break;
	case 0x2C:
		if(g_offline_voice.bed_type == 1)  /* AB床  M3为脚部电机*/
			prepare_mfp_NORMAL_KET(KEY_M3_IN, 15);
		else
			prepare_mfp_NORMAL_KET(KEY_M2_IN, 15);
		break;
	case 0x2D:
		if (g_sysparam_st.m1 == 0)
			prepare_mfp_NORMAL_KET(KEY_MASSAGE_All, 3);
		break;
	case 0x2E:
		prepare_mfp_NORMAL_KET(KEY_MASSAGE_FEET | KEY_MASSAGE_HEAD, 3);
		break;
	case 0x2F:
		prepare_mfp_NORMAL_KET(KEY_MASSAGE_HEAD_MINUS | KEY_MASSAGE_FEET_MIUNS, 3);
		break;
	case 0x30:
		if (g_sysparam_st.m1 != 0)
			prepare_mfp_NORMAL_KET(KEY_MASSAGE_STOP_ALL, 3);
		break;
	case 0x31:
		if (g_offline_voice.ubb_enable == true)
			prepare_mfp_NORMAL_KET(KEY_UBB, 3);
		break;
	case 0x32:
		if (g_offline_voice.ubb_enable == false)
			prepare_mfp_NORMAL_KET(KEY_UBB, 3);
		break;
	case 0x33:
		prepare_mfp_NORMAL_KET(KEY_ALLFATE, 3);
		break;
	case 0x34:
		prepare_mfp_NORMAL_KET(KEY_MEMORY4, 3);
		break;
	case 0x35:
		prepare_mfp_NORMAL_KET(KEY_M4_OUT, 15);
		break;
	case 0x36:
		prepare_mfp_NORMAL_KET(KEY_M4_IN, 15);
		break;
	case 0x37:
		prepare_mfp_NORMAL_KET(KEY_M3_OUT, 15);
		break;
	case 0x38:
		prepare_mfp_NORMAL_KET(KEY_M3_IN, 15);
		break;
	default:
		LOG_I("unknown cmd 0x%02x", cmd);
		break;
	}
}

typedef enum {
	VOICE_STATE_DISABLED,
	VOICE_STATE_WAKE_WORD_DETECTED,
	VOICE_STATE_ACTIVE
} VoiceState;

/*
 * cmd 0x81: voice payload; data = sub-command.
 * cmd 0x82: session end from CI1302.
 */
void offline_voice_Handle(uint8_t cmd, uint8_t data)
{
	static VoiceState state = VOICE_STATE_DISABLED;

	if (g_offline_voice.key_enable == false) {
		if (g_offline_voice.enabled || state != VOICE_STATE_DISABLED) {
			LOG_I("V off LED3");
			g_offline_voice.enabled = false;
			led_ctrl_force_off(LED_ID_3);
		}
		state = VOICE_STATE_DISABLED;
		return;
	}

	LOG_I("V key_enable=%d wake_word=%d data=0x%x ubb_enable=%d", g_offline_voice.key_enable, g_offline_voice.wake_word,data, g_offline_voice.ubb_enable);

	switch (state) {
	case VOICE_STATE_DISABLED:
		if ((cmd == 0x81 && data == 0x21 && g_offline_voice.wake_word == Hello_Ergo) ||
		    (cmd == 0x81 && data == 0x22 && g_offline_voice.wake_word == Hello_Bed)) {
			state = VOICE_STATE_WAKE_WORD_DETECTED;
			g_offline_voice.enabled = true;
			offline_voice_wake_up();
		}
		break;

	case VOICE_STATE_WAKE_WORD_DETECTED:
	case VOICE_STATE_ACTIVE:
		if (cmd == 0x81) {
			offline_voice_dataHandle(data);
			state = VOICE_STATE_ACTIVE;
		} else if (cmd == 0x82) {
			offline_voice_wake_off();
			state = VOICE_STATE_DISABLED;
		}
		break;
	}
}
