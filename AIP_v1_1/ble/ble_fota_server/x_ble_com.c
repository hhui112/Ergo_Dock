#include "x_ble_com.h"
#include "g.h"
#include "mfp_queue.h"
#include "x_control.h"
#include <string.h>

uint8_t blerxbuff[BLE_LINK_FRAME_BUF];

uint8_t bletxbuff[BLE_LINK_FRAME_BUF];

uint8_t bletxlen;

/*
 * Smart2.0 / 床控 BLE：链路层为「帧长 + 端口 + 数据域」，无同步头、无链路层 CRC。
 * 主控盒原始数据段（0xB7）内的校验和属 MFP/主控协议，由串口侧保证，与 BLE 发送无关。
 */

#define BLE_APP_PORT_INNER          0x01U  /* 端口：0x01 = APP 键值帧 / 主控响应帧 */
#define BLE_DATA_PORT               0x06U  /* 端口：0x06 = 数据段 */
#define BLE_SERVICE_PORT            0x09U  /* 端口：0x09 = 服务帧 */

#define BLE_APP_KEY_LEN_NORMAL      0x05U
#define BLE_APP_KEY_LEN_SOFT        0x07U
#define BLE_RX_MAX_PAYLOAD          160U

#define BLE_VOICE_SEG_TYPE          0x10U
#define BLE_VOICE_SEG_PARAM         0xB6U  /* 离线语音段参数（协议 3.3.x） */
#define BLE_SNORE_SEG_TYPE          0x11U
#define BLE_SNORE_SEG_PARAM         0xB4U

/* 固定段结束下标（不含）：0~10 基础头，11~18 键值 A9，19~32 蓝牙盒 B8 */
#define BLE_RESP_SEG_END_FIXED      33U
#define BLE_SEG_MFP_RAW             0xB7U  /* 主控原始段类型 */

extern mfp_data_st uart1_data_pack;

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

static uint8_t ble_offline_voice_state_byte(void)
{
	if (!g_offline_voice.key_enable)
		return 0U;
	if (g_offline_voice.wake_word == Hello_Bed)
		return 1U;
	if (g_offline_voice.wake_word == Hello_Ergo)
		return 2U;
	return 0U;
}

/*
 * TODO：从 MFP 缓冲拷贝主控同步包；末尾 checkSum 属主控协议，在此段内一并上报，不由 BLE 另算 CRC。
 * 当前返回 0，仅占位。
 */
static uint8_t ble_mfp_raw_fetch(uint8_t *dst, uint8_t max_len)
{
	(void)dst;
	(void)max_len;
	return 0U;
}

static uint8_t ble_fill_app_key_data_domain(uint8_t *p)
{
	uint32_t mt;
	uint32_t keys_le;
	uint8_t raw_len;
	uint8_t domain_len;
	uint8_t max_raw;

	p[1] = BLE_APP_PORT_INNER;			// 端口：0x01 = APP键值帧/主控盒响应帧
	p[2] = g_sysparam_st.ubb ? 1U : 0U;
	p[3] = g_sysparam_st.m1;
	p[4] = g_sysparam_st.m2;
	mt = uart1_data_pack.rx.syncPacket.massageTimer;
	p[5] = (uint8_t)(mt & 0xFFU);
	p[6] = (uint8_t)((mt >> 8) & 0xFFU);
	p[7] = (uint8_t)((mt >> 16) & 0xFFU);
	p[8] = (uint8_t)((mt >> 24) & 0xFFU);
	p[9]  = uart1_data_pack.rx.syncPacket.massge_mode;
	p[10] = 0U;

	p[11] = 0xA9U;						// 0xA9 = 键值段标志
	p[12] = 0U;
	p[13] = 0U;
	p[14] = 0U;
	keys_le = uart1_data_pack.rx.syncPacket.keys;
	p[15] = (uint8_t)(keys_le & 0xFFU);
	p[16] = (uint8_t)((keys_le >> 8) & 0xFFU);
	p[17] = (uint8_t)((keys_le >> 16) & 0xFFU);
	p[18] = (uint8_t)((keys_le >> 24) & 0xFFU);

	p[19] = 0xB8U;						/* 3.2.2.4 段类型 B8 */
	p[20] = (uint8_t)BLE_APP_KEYRESP_SW_MAJOR;
	p[21] = (uint8_t)BLE_APP_KEYRESP_SW_MINOR;
	p[22] = (uint8_t)BLE_APP_KEYRESP_SW_PATCH;
	p[23] = (uint8_t)BLE_APP_KEYRESP_HW_MAJOR;
	p[24] = (uint8_t)BLE_APP_KEYRESP_HW_MINOR; 
	p[25] = 0U;
	p[26] = 0U;
	p[27] = 0U;
	p[28] = 0U;
	p[29] = 0U;
	p[30] = ble_offline_voice_state_byte();
	p[31] = g_sysparam_st.AntiSnore_intensity;
	p[32] = g_sysparam_st.charge_state;

	p[33] = 0xB7U;						// 0xB7 = 原始数据段标志
	max_raw = (uint8_t)(BLE_RX_MAX_PAYLOAD - BLE_RESP_SEG_END_FIXED - 2U);
	raw_len = ble_mfp_raw_fetch(&p[35], max_raw);
	if (raw_len > max_raw)
		raw_len = max_raw;
	p[34] = raw_len;
	domain_len = (uint8_t)(BLE_RESP_SEG_END_FIXED + 2U + raw_len);
	p[0] = domain_len;
	return domain_len;
}

/* 协议 3.3.3 应答：数据域 7 字节，首字节长度为 0x06 */
static void ble_offline_voice_send_response(void)
{
	uint8_t r[7];

	r[0] = 0x06U;
	r[1] = BLE_DATA_PORT;
	r[2] = BLE_VOICE_SEG_TYPE;
	r[3] = BLE_VOICE_SEG_PARAM;
	r[4] = ble_offline_voice_state_byte();
	r[5] = g_offline_voice.enabled ? 1U : 0U;
	r[6] = 0x00U;
	ble_link_send(BLE_DATA_PORT, r, 7U);
}

/* 协议 3.4.3 应答：数据域 16 字节，首字节长度为 0x10 */
static void ble_snore_send_response(void)
{
	uint8_t r[16];

	r[0] = 0x10U;
	r[1] = BLE_DATA_PORT;
	r[2] = BLE_SNORE_SEG_TYPE;
	r[3] = BLE_SNORE_SEG_PARAM;
	r[4] = g_sysparam_st.snoreIntervention.snoreIntervention_pwm;
	r[5] = g_sysparam_st.snoreIntervention.snoreIntervention_tmr;
	r[6] = g_sysparam_st.AntiSnore_intensity;
	r[7] = (uint8_t)(g_sysparam_st.snoreIntervention.triggered_flag ? 1U : 0U);
	memset(&r[8], 0, 8U);
	ble_link_send(BLE_DATA_PORT, r, 16U);
}

/*
 * 端口 0x06 数据域内：偏移 1 为数据域内端口（协议固定 0x06），偏移 2 段类型，偏移 3 参数。
 * 协议 3.3：段类型 0x10，参数 0xB6。
 */
static void ble_offline_voice_segment_handle(const uint8_t *d, uint8_t n)
{
	if (n < 4U) {
		LOG_I("BLE v: n=%u short", (unsigned)n);
		return;
	}
	if (d[1] != BLE_DATA_PORT) {
		LOG_I("BLE v: d1=0x%02X need 0x06", d[1]);
		return;
	}
	if (d[3] != BLE_VOICE_SEG_PARAM) {
		LOG_I("BLE v: d3=0x%02X need 0xB6", d[3]);
		return;
	}

	/* 3.3.1 查询：数据域总长 0x02，示例 02 06 10 B6 */
	if (d[0] == 0x02U) {
		ble_offline_voice_send_response();
		return;
	}
	/* 3.3.2 设置：数据域总长 0x03，示例 03 06 10 B6 + 使能 … */
	if (d[0] == 0x03U && n >= 5U) {
		uint8_t en = d[4];

		if (en == 0U)
			g_offline_voice.key_enable = false;
		else {
			g_offline_voice.key_enable = true;
			if (en == 1U)
				g_offline_voice.wake_word = Hello_Bed;
			else if (en == 2U)
				g_offline_voice.wake_word = Hello_Ergo;
		}
		ble_offline_voice_send_response();
		return;
	}
	LOG_I("BLE v: unk sub 0x%02X n=%u", d[0], (unsigned)n);
}

/*
 * 协议 3.4：段类型 0x11，参数 0xB4。
 */
static void ble_snore_segment_handle(const uint8_t *d, uint8_t n)
{
	if (n < 4U) {
		LOG_I("BLE s: n=%u short", (unsigned)n);
		return;
	}
	if (d[1] != BLE_DATA_PORT) {
		LOG_I("BLE s: d1=0x%02X need 0x06", d[1]);
		return;
	}
	if (d[3] != BLE_SNORE_SEG_PARAM) {
		LOG_I("BLE s: d3=0x%02X need 0xB4", d[3]);
		return;
	}

	/* 3.4.1 查询：02 06 11 B4 */
	if (d[0] == 0x02U) {
		ble_snore_send_response();
		return;
	}
	/* 3.4.2 设置：07 06 11 B4 + 缓启动参数 + 预留 3B */
	if (d[0] == 0x07U && n >= 8U) {
		g_sysparam_st.snoreIntervention.snoreIntervention_pwm = d[4];
		g_sysparam_st.snoreIntervention.snoreIntervention_tmr = d[5];
		ble_snore_send_response();
		return;
	}
	LOG_I("BLE s: unk sub 0x%02X n=%u", d[0], (unsigned)n);
}

/*
 * 端口 0x06（数据段）：外层已在 x_ble_com_handle 按端口选中；
 * 此处仅按「段类型」分流，具体「参数」与各子命令在各 segment_handle 内处理（协议 3.3 / 3.4）。
 */
static void ble_data_port_dispatch(const uint8_t *d, uint8_t n)
{
	if (n < 3U) {
		LOG_I("BLE 0x06: n=%u short", (unsigned)n);
		return;
	}
	switch (d[2]) {
	case BLE_VOICE_SEG_TYPE:
		ble_offline_voice_segment_handle(d, n);
		break;
	case BLE_SNORE_SEG_TYPE:
		ble_snore_segment_handle(d, n);
		break;
	default:
		LOG_I("BLE 0x06: seg 0x%02X ?", d[2]);
		break;
	}
}

static void ble_port09_dispatch(const uint8_t *d, uint8_t n)
{
	(void)d;
	LOG_I("BLE 0x09 n=%u TODO", (unsigned)n);
}

/*
 * 端口 0x01 数据域：普通/缓启动键值。
 * 键值为 0（含 APP 用作查询）仍原样经 MFP 下发，主控盒自行区分业务含义。
 */
static void ble_app_key_dispatch(const uint8_t *d, uint8_t n)
{
	uint32_t key;
	uint8_t inner_len;
	uint8_t inner_port;

	if (n < 7U)
		return;
	inner_len = d[0];
	inner_port = d[1];
	if (inner_port != BLE_APP_PORT_INNER) {
		LOG_I("BLE key: inner_port 0x%02X (!=0x01)", inner_port);
		return;
	}
	key = (uint32_t)d[2] | ((uint32_t)d[3] << 8) | ((uint32_t)d[4] << 16) | ((uint32_t)d[5] << 24);

	if (inner_len == BLE_APP_KEY_LEN_NORMAL) {
		(void)d[6];
		LOG_I("BLE key N 0x%08lX", (unsigned long)key);
		prepare_mfp_NORMAL_KET(key, 3);
		return;
	}
	if (inner_len == BLE_APP_KEY_LEN_SOFT) {
		if (n < 9U)
			return;
		(void)d[6];
		LOG_I("BLE key S 0x%08lX p=%u t=%u", (unsigned long)key, (unsigned)d[7], (unsigned)d[8]);
		prepare_mfp_SOFT_START(key, d[7], d[8], 3);
		return;
	}
	LOG_I("BLE key: inner_len 0x%02X ?", inner_len);
}

/*
 * 解析顺序：① 链路层 帧长+端口（pbuff[0]、pbuff[1]）→ 按端口分支；
 * ② 端口 0x06 时 ble_data_port_dispatch 按数据域偏移 2 段类型再分函数；
 * ③ 各段处理函数内校验参数（0xB6 / 0xB4）及 3.3.x / 3.4.x 子命令。
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
	if ((uint16_t)len < need)
		return;
	switch (port) {
	case BLE_APP_PORT_INNER:
		ble_app_key_dispatch(&pbuff[2], data_len);
		x_ble_com_txbuffFill();
		
		break;
	case BLE_DATA_PORT:
		ble_data_port_dispatch(&pbuff[2], data_len);
		break;
	case BLE_SERVICE_PORT:
		ble_port09_dispatch(&pbuff[2], data_len);
		/* 服务帧：暂未定义，暂不处理*/
		// ble_send_app_key_full_frame_notify();
		break;
	default:
		LOG_I("BLE: port 0x%02X ?", port);
		break;
	}
}

/*
 * 组「APP 键值响应」链路帧（端口 0x01 + 数据域），供周期上报与写后即时应答共用。
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
}
