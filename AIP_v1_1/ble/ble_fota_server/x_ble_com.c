#include "x_ble_com.h"
#include "g.h"
#include "mfp_queue.h"
#include "x_control.h"
#include "x_snoreintervention.h"
#include "x_uart.h"
#include "app_led_ctrl.h"
#include <stdio.h>
#include <string.h>

uint8_t blerxbuff[BLE_LINK_FRAME_BUF];

uint8_t bletxbuff[BLE_LINK_FRAME_BUF];

uint8_t bletxlen;

/*
 * Ergo_Dock2.0 BLE：链路层「帧长(=数据域字节数) + 端口 + 数据域」，无同步头、无链路 CRC。
 * 例 3.3.1：02 06 10 B6 → 数据域仅 10 B6（2B），帧长 02 不包含端口 06。
 * 3.5 测试信息：查询 02 06 12 B6 / 12 B7；应答 21 06 12 B4 + 三个 uint16（小端）+ B3 + 24×uint8（cmd 0x21..0x38）。
 * 3.1 键值仅两种整帧：05 01 01 00 00 00 00（普通）；07 01 01 00 00 00 50 10（缓启动）。
 */

#define BLE_APP_PORT_INNER          0x01U  /* 端口：0x01 = APP 键值帧 / 主控响应帧 */
#define BLE_DATA_PORT               0x06U  /* 端口：0x06 = 数据段 */
#define BLE_SERVICE_PORT            0x09U  /* 端口：0x09 = 服务帧 */

#define BLE_RX_MAX_PAYLOAD          160U

/* 数据域内：BA/A9/B8 固定段共 32 字节，下一字节为 B7；B7 后紧跟主控盒 UART 原始帧（首字节即为协议 length，不再单独带 raw 长度字节） */
#define BLE_RESP_SEG_END_FIXED      32U
#define BLE_SEG_MFP_RAW             0xB7U  /* 主控原始段类型 */
#define BLE_SEG_TESTINFO            0x12U  /* 3.5 测试信息段 */
#define BLE_TESTINFO_PARAM_DETAIL   0xB3U  /* 测试信息精确数据（紧随 B4 汇总之后） */
#define BLE_TESTINFO_TX_DATA_LEN    (2U + 6U + 1U + BLE_TESTINFO_VOICE_DETAIL_N) /* 12 B4 + 3×LE16 + B3 + detail */

extern mfp_data_st uart1_data_pack;
extern uint8_t ls_bleup_server_send_notification(uint8_t *data_notice, uint16_t length);
extern uint8_t uart1_get_last_mfp_raw(uint8_t *dst, uint8_t max_len);

/* 一帧有效字节数：帧长 + 端口 + 数据域 = 2 + data_len（无尾 CRC） */
static uint16_t ble_frame_len(uint8_t data_len)
{
	return (uint16_t)(2U + (uint16_t)data_len);
}

static void ble_link_send(uint8_t port, const uint8_t *data, uint8_t n)
{
	int i = 0;

	if (n > BLE_RX_MAX_PAYLOAD)
		return;
	bletxbuff[i++] = n;
	bletxbuff[i++] = port;
	if (n != 0U && data != NULL)
		memcpy(&bletxbuff[i], data, n);
	i += n;
	bletxlen = (uint8_t)i;
	(void)ls_bleup_server_send_notification(bletxbuff, bletxlen);
}

/* 3.3.3 使能字节：与协议一致 0x01=Hello Ergo, 0x02=Hello Bed（与 g_offline_voice.wake_word 1/2 一致） */
static uint8_t ble_offline_voice_state_byte(void)
{
	if (!g_offline_voice.key_enable)
		return 0U;
	if (g_offline_voice.wake_word == Hello_Ergo)
		return 1U;
	if (g_offline_voice.wake_word == Hello_Bed)
		return 2U;
	return 0U;
}

static uint8_t ble_fill_app_key_data_domain(uint8_t *p) // APP键值帧/主控盒响应帧（仅填数据域；帧长、端口由 x_ble_com_txbuffFill 写在 p 之前）
{
	uint32_t mt;
	uint32_t keys_le;
	uint8_t raw_len;
	uint8_t domain_len;
	uint8_t max_raw;

	p[0] = 0xBAU;						/* 基础头段类型 */
	p[1] = g_sysparam_st.ubb;
	p[2] = g_sysparam_st.m1;
	p[3] = g_sysparam_st.m2;
	mt = g_sysparam_st.massage_timer;
	p[4] = (uint8_t)(mt & 0xFFU);
	p[5] = (uint8_t)((mt >> 8) & 0xFFU);
	p[6] = (uint8_t)((mt >> 16) & 0xFFU);
	p[7] = (uint8_t)((mt >> 24) & 0xFFU);
	{
		uint8_t mfp_len = uart1_get_last_mfp_sync_len();
		p[8] = (mfp_len == 0x30U || mfp_len == 0x34U) ? 1U : 0U;
	}
	p[9] = 0U;

	p[10] = 0xB9U;						/* 键值段类型 */
	p[11] = 0U;
	p[12] = 0U;
	p[13] = 0U;
	keys_le = g_sysparam_st.mfp_keys;
	p[14] = (uint8_t)(keys_le & 0xFFU);
	p[15] = (uint8_t)((keys_le >> 8) & 0xFFU);
	p[16] = (uint8_t)((keys_le >> 16) & 0xFFU);
	p[17] = (uint8_t)((keys_le >> 24) & 0xFFU);

	p[18] = 0xB8U;						/* 系统信息段类型 */
	p[19] = (uint8_t)BLE_APP_KEYRESP_SW_MAJOR;
	p[20] = (uint8_t)BLE_APP_KEYRESP_SW_MINOR;
	p[21] = (uint8_t)BLE_APP_KEYRESP_SW_PATCH;
	/* 软件版本三字节：主版本 2，次版本 0，修订 0 */
	p[22] = (uint8_t)BLE_APP_KEYRESP_HW_MAJOR;
	p[23] = (uint8_t)BLE_APP_KEYRESP_HW_MINOR;
	/* 硬件版本两字节：主版本 2，次版本 1 */
	p[24] = 0U;
	p[25] = 0U;
	p[26] = 0U;
	p[27] = 0U;
	p[28] = 0U;
	p[29] = ble_offline_voice_state_byte();
	p[30] = g_sysparam_st.AntiSnore_intensity;
	p[31] = g_sysparam_st.charge_state;

	p[32] = 0xB7U;						/* 主控原始数据段类型；其后直接为主控 UART 原始字节流 */
	max_raw = (uint8_t)(BLE_RX_MAX_PAYLOAD - BLE_RESP_SEG_END_FIXED - 1U);
	raw_len = uart1_get_last_mfp_raw(&p[33], max_raw);
	domain_len = (uint8_t)(BLE_RESP_SEG_END_FIXED + 1U + raw_len);
	return domain_len;
}

/*
 * 3.3.3 应答：数据域固定 5B = 10 + B4 + 使能 + 唤醒状态 + 控制词；整帧 05 06 10 B4 xx xx xx。
 * 使能：0=关；0x01=Hello Ergo；0x02=Hello Bed。唤醒状态：0=休眠；1=已唤醒。控制词：最近一次 CI1302 指令，默认 0。
 */
static void ble_offline_voice_send_response(void)
{
	uint8_t r[5];

	r[0] = 0x10U;
	r[1] = 0xB4U;
	r[2] = ble_offline_voice_state_byte();
	r[3] = g_offline_voice.enabled ? 1U : 0U;
	r[4] = g_offline_voice.last_control_cmd;
	LOG_I("BLE 3.3.3 tx: 05 06 %02X %02X %02X %02X %02X", r[0], r[1], r[2], r[3], r[4]);
	ble_link_send(BLE_DATA_PORT, r, 5U);
}

/*
 * 3.4.3 应答：帧长 0x0A、端口 0x06 由 ble_link_send 写入；数据域组帧：长度1 + 端口1 + 11 B4 + 缓启 + 强度 + 触发 + 剩余 4B。
 * 整帧示例：0A 06 11 B4 50 40 01 01 00 00 00 00
 */
static void ble_snore_send_response(void)
{
	uint8_t r[10];
	uint32_t remain_10ms = 0U;

	if (g_sysparam_st.snoreIntervention.first_lift_tick != 0U) {
		uint32_t first = g_sysparam_st.snoreIntervention.first_lift_tick;
		uint32_t elapsed = g_sysparam_st.timer - first;

		if (elapsed < SNORE_FLAT_DELAY_TICKS)
			remain_10ms = SNORE_FLAT_DELAY_TICKS - elapsed;
	}

	r[0] = 0x11U;
	r[1] = 0xB4U;
	r[2] = g_sysparam_st.snoreIntervention.snoreIntervention_pwm;
	r[3] = g_sysparam_st.snoreIntervention.snoreIntervention_tmr;
	r[4] = (uint8_t)(g_sysparam_st.AntiSnore_intensity > 3U ? 3U : g_sysparam_st.AntiSnore_intensity);
	r[5] = (uint8_t)((g_sysparam_st.snoreIntervention.is_intervening ||
	                  g_sysparam_st.snoreIntervention.triggered_flag) ? 1U : 0U);
	r[6]  = (uint8_t)(remain_10ms & 0xFFU);
	r[7]  = (uint8_t)((remain_10ms >> 8) & 0xFFU);
	r[8] = (uint8_t)((remain_10ms >> 16) & 0xFFU);
	r[9] = (uint8_t)((remain_10ms >> 24) & 0xFFU);
	LOG_I("BLE 3.4.3 tx: 0A 06 %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7], r[8], r[9]);
	ble_link_send(BLE_DATA_PORT, r, 10U);
}

/*
 * 数据段 0x10：B6 查询；B7 设置（3.3.2：10 B7 床型，使能/唤醒词设置暂不开放，床型写入 g_offline_voice.bed_type）。
 */
static void ble_offline_voice_segment_handle(const uint8_t *d, uint8_t n)
{
	if (n < 2U) {
		LOG_I("BLE v: n=%u short", (unsigned)n);
		return;
	}
	if (d[0] != 0x10U) {
		LOG_I("BLE v: seg 0x%02X need 0x10", d[0]);
		return;
	}

	if (d[1] == 0xB6U && n >= 2U)  /* 查询离线语音（数据域至少 10 B6，允许尾部填充） */
	{
		LOG_I("BLE 3.3.3: query B6 ok n=%u", (unsigned)n);
		ble_offline_voice_send_response();
		return;
	}
	if (d[1] == 0xB7U && n >= 3U)  /* 3.3.2 设置离线语音：10 B7 床型 */
	{
		uint8_t bed_type = d[2];

		if (bed_type > 1U) {
			LOG_I("BLE v: B7 bed_type %u invalid (0=默认主控盒 1=AB床)", (unsigned)bed_type);
		} else {
			g_offline_voice.bed_type = bed_type;
			LOG_I("BLE v: B7 bed_type=%u", (unsigned)bed_type);
		}
		ble_offline_voice_send_response();
		return;
	}
	LOG_I("BLE v: param 0x%02X n=%u", d[1], (unsigned)n);
}

/* 数据域内先 0x11，再 B6 查询 / B7 设置 */
static void ble_snore_segment_handle(const uint8_t *d, uint8_t n)
{
	if (n < 2U) {
		LOG_I("BLE s: n=%u short", (unsigned)n);
		return;
	}
	if (d[0] != 0x11U) {
		LOG_I("BLE s: seg 0x%02X need 0x11", d[0]);
		return;
	}

	if (d[1] == 0xB6U && n >= 2U)  /* 查询打鼾（02 06 11 B6，数据域至少 11 B6） */
	{
		LOG_I("BLE 3.4.3: query 11 B6 ok n=%u", (unsigned)n);
		ble_snore_send_response();
		return;
	}
	if (d[1] == 0xB7U && n == 7U)  /* 设置打鼾 */
	 {
		uint8_t intensity = d[4];

		if (intensity > 3U) {
			LOG_I("BLE s: intensity %u > 3", (unsigned)intensity);
			return;
		}
		if (d[5] != 0U || d[6] != 0U)
			LOG_I("BLE s: reserve %02X %02X", d[5], d[6]);
		g_sysparam_st.snoreIntervention.snoreIntervention_pwm = d[2];
		g_sysparam_st.snoreIntervention.snoreIntervention_tmr = d[3];
		g_sysparam_st.AntiSnore_intensity = intensity;
		g_sysparam_st.snoreIntervention.enable = (bool)(intensity != 0U);
		if (intensity == 0U) {
			g_sysparam_st.snoreIntervention.triggered_flag = false;
			SnoringInterventStateClear();
		}
		flash_save_snore_cfg_if_changed();
		/* 与按键切档一致：档位蓝灯亮 10s 后熄灭（app_led_ctrl 内 timeout_s=10） */
		led_snore_level_set(g_sysparam_st.AntiSnore_intensity);
		ble_snore_send_response();
		return;
	}
	LOG_I("BLE s: param 0x%02X n=%u", d[1], (unsigned)n);
}


static void ble_testinfo_send_response(void)
{
	uint8_t r[BLE_TESTINFO_TX_DATA_LEN];
	uint16_t w = g_sysparam_st.ble_testinfo.offline_voice_wake;
	uint16_t resp = g_sysparam_st.ble_testinfo.offline_voice_resp;
	uint16_t sn = g_sysparam_st.ble_testinfo.snore_intervention_trig;

	r[0] = BLE_SEG_TESTINFO;
	r[1] = 0xB4U;
	r[2] = (uint8_t)(w & 0xFFU);
	r[3] = (uint8_t)((w >> 8) & 0xFFU);
	r[4] = (uint8_t)(resp & 0xFFU);
	r[5] = (uint8_t)((resp >> 8) & 0xFFU);
	r[6] = (uint8_t)(sn & 0xFFU);
	r[7] = (uint8_t)((sn >> 8) & 0xFFU);
	r[8] = BLE_TESTINFO_PARAM_DETAIL;
	memcpy(&r[9], g_sysparam_st.ble_testinfo.voice_detail, BLE_TESTINFO_VOICE_DETAIL_N);

	LOG_I("BLE 3.5 tx: 21 06 12 B4 +6B B3 +24B");
	ble_link_send(BLE_DATA_PORT, r, (uint8_t)sizeof(r));
}

static void ble_testinfo_segment_handle(const uint8_t *d, uint8_t n)
{
	if (n < 2U) {
		LOG_I("BLE testinfo: n=%u short", (unsigned)n);
		return;
	}
	if (d[0] != BLE_SEG_TESTINFO) {
		LOG_I("BLE testinfo: seg 0x%02X need 0x12", d[0]);
		return;
	}
	if (d[1] == 0xB6U && n >= 2U) {
		LOG_I("BLE 3.5: query 12 B6 ok n=%u", (unsigned)n);
		ble_testinfo_send_response();
		return;
	}
	if (d[1] == 0xB7U && n >= 2U) {
		LOG_I("BLE 3.5: set 12 B7 reserved, echo counters n=%u", (unsigned)n);
		ble_testinfo_send_response();
		return;
	}
	LOG_I("BLE testinfo: param 0x%02X n=%u", d[1], (unsigned)n);
}

/* 端口 0x06：按数据域首字节段类型再分派（0x10 离线语音 / 0x11 打鼾 / 0x12 测试信息） */
static void ble_data_port_dispatch(const uint8_t *payload, uint8_t length)
{
	if (length < 2U) {
		LOG_I("BLE 0x06: data too short n=%u", (unsigned)length);
		return;
	}
	LOG_I("BLE 0x06: n=%u %02X %02X ...", (unsigned)length, payload[0], payload[1]);

	if (payload[0] == 0x10U) {
		ble_offline_voice_segment_handle(payload, length);
		return;
	}
	if (payload[0] == 0x11U) {
		ble_snore_segment_handle(payload, length);
		return;
	}
	if (payload[0] == 0x12U) {
		ble_testinfo_segment_handle(payload, length);
		return;
	}
	LOG_I("BLE 0x06: seg 0x%02X ?", payload[0]);
}

static void ble_port09_dispatch(const uint8_t *payload, uint8_t length)
{
	/* 服务帧暂不开放 */
	/*(void)payload;*/
	LOG_I("BLE 0x09 n=%u TODO", (unsigned)length);
}

/*
 * 端口 0x01：数据域 = 键值 4B（小端）+ 扩展 1B（5B）；缓启动再 +Pwm+Tmr（7B）。
 */
static void ble_app_key_dispatch(const uint8_t *d, uint8_t n)
{
	uint32_t key;

	if (n == 5U) {
		key = (uint32_t)d[0] | ((uint32_t)d[1] << 8) | ((uint32_t)d[2] << 16) | ((uint32_t)d[3] << 24);
		LOG_I("BLE key N 0x%08lX", (unsigned long)key);
		prepare_mfp_NORMAL_KET(key, 3);
		return;
	}
	if (n == 7U) {
		key = (uint32_t)d[0] | ((uint32_t)d[1] << 8) | ((uint32_t)d[2] << 16) | ((uint32_t)d[3] << 24);
		LOG_I("BLE key S 0x%08lX ext=%u pwm=%u tmr=%u", (unsigned long)key, (unsigned)d[4], (unsigned)d[5],
		       (unsigned)d[6]);
		prepare_mfp_SOFT_START(key, d[5], d[6], 3);
		return;
	}
	LOG_I("BLE key: n=%u (need data 5B / 7B per 3.1.1)", (unsigned)n);
}

/* 端口 0x01：3.1 键值下发主控 + 3.2 组帧并下发（与 0x06「收令-应答」结构一致） */
static void ble_app_port_inner_dispatch(const uint8_t *payload, uint8_t length)
{
	ble_app_key_dispatch(payload, length);
	x_ble_com_txbuffFill();
}

/*
 * 解析顺序：① 帧长 + 端口；② 0x01 → 键值；0x06 → 数据段再判段类型与参数（如 06→10→B7）。
 */
void x_ble_com_handle(uint8_t *pbuff, uint8_t len)
{
	uint8_t data_len;
	uint8_t port;
	uint16_t need;
	uint16_t body_len;

	data_len = pbuff[0];
	port = pbuff[1];
	if (data_len > BLE_RX_MAX_PAYLOAD)
		return;
	body_len = ble_frame_len(data_len);
	need = body_len;
	LOG_I("BLE rx: port=0x%02X data_len=%u need=%u len=%u", port, data_len, need, len);
	if ((uint16_t)len < need) {
		LOG_I("BLE rx: truncated need %u got %u [%02X %02X %02X %02X]", need, len,
		      len > 0 ? pbuff[0] : 0U, len > 1 ? pbuff[1] : 0U, len > 2 ? pbuff[2] : 0U, len > 3 ? pbuff[3] : 0U);
		return;
	}
	switch (port) {
	case BLE_APP_PORT_INNER:
		ble_app_port_inner_dispatch(&pbuff[2], data_len);
		break;
	case BLE_DATA_PORT:
		ble_data_port_dispatch(&pbuff[2], data_len);
		break;
	case BLE_SERVICE_PORT:
		ble_port09_dispatch(&pbuff[2], data_len);
		break;
	default:
		LOG_I("BLE: port 0x%02X ?", port);
		break;
	}
}

/*
 * 组「APP 键值响应」链路帧（端口 0x01 + 数据域）并 BLE Notification 发出。
 * 由 ble_app_port_inner_dispatch 在收到键值后调用；若将来需只组帧不发，可再拆函数。
 */
void x_ble_com_txbuffFill(void)
{
	uint8_t domain_len;
	int i = 0;

	memset(bletxbuff, 0, sizeof(bletxbuff));
	g_sysparam_st.snoreIntervention.triging = (uint8_t)(g_sysparam_st.snoreIntervention.is_intervening ? 1U : 0U);

	bletxbuff[i++] = 0U;
	bletxbuff[i++] = BLE_APP_PORT_INNER;   // 端口：0x01 = APP键值帧/主控盒响应帧
	domain_len = ble_fill_app_key_data_domain(&bletxbuff[2]);
	bletxbuff[0] = domain_len;
	i = 2 + (int)domain_len;
	bletxlen = (uint8_t)i;

#if 1 /* 调试：打印 APP 键值响应整帧，调完可改为 0 或整段注释 */
	{
		unsigned j, k;
		char line[3U * 16U + 1U];

		LOG_I("BLE TX notify len=%u", (unsigned)bletxlen);
		for (j = 0U; j < (unsigned)bletxlen; j += 16U) {
			unsigned n = (unsigned)bletxlen - j;

			if (n > 16U)
				n = 16U;
			for (k = 0U; k < n; k++)
				(void)snprintf(line + k * 3U, sizeof(line) - k * 3U, "%02X ", bletxbuff[j + k]);
			line[n * 3U] = '\0';
			LOG_I("BLE TX: %s", line);
		}
	}
#endif

	(void)ls_bleup_server_send_notification(bletxbuff, bletxlen);
}
