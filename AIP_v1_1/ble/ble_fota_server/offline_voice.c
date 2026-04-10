#include "x_faultDetect.h"
#include "offline_voice.h"
#include "mfp_queue.h"
#include "g.h"
#include "app_led_ctrl.h"

void offline_voice_wake_up(void)
{
	LOG_I("V wake_up");
	led_voice_wakeup();
}

void offline_voice_wake_off(void)
{
	LOG_I("V wake_off");
	g_offline_voice.enabled = false;
	led_voice_close_flash();
}

/* Map CI1302 voice command byte (data) to MFP keys after wake word matched. */
void offline_voice_dataHandle(uint8_t cmd)
{
	if (cmd == 0x21 || cmd == 0x22) {
		LOG_I("V wake_word 0x%02x", cmd);
		led_voice_wakeup();
		return;
	}

	LOG_I("V cmd 0x%02x", cmd);
	led_voice_command_confirm();

	g_sysparam_st.snoreIntervention.is_intervening = false;
	g_sysparam_st.snoreIntervention.triggered_flag = false;
	switch (cmd) {
	case 0x23:
		mfp_tx_queue_clear();
		prepare_mfp_NORMAL_KET(KEY_MASSAGE_STOP_ALL, 1);
		break;
	case 0x24:
		prepare_mfp_NORMAL_KET((KEY_M1_OUT | KEY_M2_OUT), 30);
		break;
	case 0x25:
		prepare_mfp_NORMAL_KET(KEY_ZEROG, 3);
		break;
	case 0x26:
		prepare_mfp_NORMAL_KET(KEY_ALLFATE, 3);
		break;
	case 0x27:
		prepare_mfp_NORMAL_KET(KEY_MEMORY5, 3);
		break;
	case 0x28:
		prepare_mfp_NORMAL_KET(KEY_MEMORY3, 3);
		break;
	case 0x29:
		prepare_mfp_NORMAL_KET(KEY_M1_OUT, 15);
		break;
	case 0x2A:
		prepare_mfp_NORMAL_KET(KEY_M1_IN, 15);
		break;
	case 0x2B:
		prepare_mfp_NORMAL_KET(KEY_M2_OUT, 15);
		break;
	case 0x2C:
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
		LOG_I("V unk cmd 0x%02x", cmd);
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
