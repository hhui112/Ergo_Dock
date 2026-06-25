#include "x_uart.h"
#include "mfp_queue.h"
#include "g.h"
#include "offline_voice.h"
#include "x_control.h"
#include <string.h>

uint8_t s_tx_busy;
uint8_t rx1_t_data, rx2_t_data, rx3_t_data;

mfp_data_st uart1_data_pack;
uart_data_st uart2_data_pack;
uart_data_st uart3_data_pack;

UART_HandleTypeDef UART_Server3_Config;
UART_HandleTypeDef UART_Server2_Config;
UART_HandleTypeDef UART_Server1_Config;

/** 最近一次 MFP 校验通过帧的原始快照（与 rx.rawData 一致，避免后续 memset 后 BLE 读不到）。 */
static uint8_t s_uart1_last_mfp_raw[UART_RX_BUF_SIZE];
static uint16_t s_uart1_last_mfp_raw_len;

uint8_t uart1_get_last_mfp_raw(uint8_t *dst, uint8_t max_len)
{
	uint16_t n;

	if (dst == NULL || max_len == 0U || s_uart1_last_mfp_raw_len == 0U)
		return 0U;
	n = s_uart1_last_mfp_raw_len;
	if (n > (uint16_t)max_len)
		n = (uint16_t)max_len;
	memcpy(dst, s_uart1_last_mfp_raw, (size_t)n);
	return (uint8_t)n;
}

uint8_t uart1_get_last_mfp_sync_len(void)
{
	if (s_uart1_last_mfp_raw_len == 0U)
		return 0U;
	return s_uart1_last_mfp_raw[0];
}

void x_uart1_dateReceiveHandle(void);
void x_uart2_dateReceiveHandle(void);
void x_uart3_dateReceiveHandle(void);
void x_uart_Internalparameterprint(void);

/* Decrement idle timeouts; run RX assembly for UART1/2/3 (10 ms tick). */
void x_uart_10ms(void)
{
	if (uart1_data_pack.timeout > 0)
		uart1_data_pack.timeout--;
	if (uart2_data_pack.timeout > 0)
		uart2_data_pack.timeout--;
	if (uart3_data_pack.timeout > 0)
		uart3_data_pack.timeout--;

	x_uart1_dateReceiveHandle();
	x_uart2_dateReceiveHandle();
	x_uart3_dateReceiveHandle();
}

/* One-byte RX complete: re-arm IT and append to per-UART buffer. No LOG here (rate too high). */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &UART_Server1_Config) {
		HAL_UART_Receive_IT(&UART_Server1_Config, &rx1_t_data, 1);
		if (uart1_data_pack.rxindex < RXBUFF_LEN) {
			uart1_data_pack.rx.rawData[uart1_data_pack.rxindex++] = rx1_t_data;
			uart1_data_pack.timeout = COM_TIMEOUT;
		}
	}

	if (huart == &UART_Server2_Config) {
		HAL_UART_Receive_IT(&UART_Server2_Config, &rx2_t_data, 1);
		if (uart2_data_pack.rxindex < RXBUFF_LEN) {
			uart2_data_pack.rxbuf[uart2_data_pack.rxindex++] = rx2_t_data;
			uart2_data_pack.timeout = COM_TIMEOUT;
		}
	}

	if (huart == &UART_Server3_Config) {
		HAL_UART_Receive_IT(&UART_Server3_Config, &rx3_t_data, 1);
		if (uart3_data_pack.rxindex < RXBUFF_LEN) {
			uart3_data_pack.rxbuf[uart3_data_pack.rxindex++] = rx3_t_data;
			uart3_data_pack.timeout = COM_TIMEOUT;
		}
	}
}

/* MFP 同步包：先快照整帧供 BLE 0xB7，再解析 UBB / 按摩等固定偏移。 */
void uart1_cmdHandle(void)
{
	uint16_t flen = (uint16_t)uart1_data_pack.rx.Syncdata.length + 3U;

	if (flen > UART_RX_BUF_SIZE)
		flen = UART_RX_BUF_SIZE;
	memcpy(s_uart1_last_mfp_raw, uart1_data_pack.rx.rawData, (size_t)flen);
	s_uart1_last_mfp_raw_len = flen;

	g_sysparam_st.ubb = uart1_data_pack.rx.rawData[11] & 0x01;
	/* 与主控同步灯态：离线语音 0x31/0x32 分支依赖 ubb_enable，必须在每帧更新（不能只靠拨键回调） */
	g_offline_voice.ubb_enable = (g_sysparam_st.ubb != 0U);
	g_sysparam_st.m1 = uart1_data_pack.rx.rawData[12];
	g_sysparam_st.m2 = uart1_data_pack.rx.rawData[13];
	if (flen >= 18U) {
		g_sysparam_st.mfp_keys = uart1_data_pack.rx.syncPacket.keys;
		g_sysparam_st.massage_timer = uart1_data_pack.rx.syncPacket.massageTimer;
	}
	// LOG_I("ubb// LOG_I("ubb:%d,m1:%d,m2:%d",g_sysparam_st.ubb,g_sysparam_st.m1,g_sysparam_st.m2);:%d,m1:%d,m2:%d",g_sysparam_st.ubb,g_sysparam_st.m1,g_sysparam_st.m2);
}

/* CI1302 voice: 8-byte frame, cmd at [3], data at [6]. */
void uart2_cmdHandle(void)
{
	offline_voice_Handle(uart2_data_pack.rxbuf[3], uart2_data_pack.rxbuf[6]);
}

unsigned char rxCalcCheckSum(void)
{
	uint16_t i;
	uint8_t sum;

	sum = 0xff;
	for (i = 0; i < ((uint16_t)2 + (uint16_t)uart1_data_pack.rx.Syncdata.length); i++)
		sum -= uart1_data_pack.rx.rawData[i];
	return sum;
}

/* MFP UART1: idle timeout clears buffer; on full frame verify checksum then cmd + MFP TX task. */
void x_uart1_dateReceiveHandle(void)
{
	// for (int i=0; i<uart1_data_pack.rxindex; i++) {LOG_I("UART1 RxData[%d]=0x%02X", i, uart1_data_pack.rx.rawData[i]);}
	if (uart1_data_pack.rxindex != 0 && uart1_data_pack.timeout == 0) {
		uart1_data_pack.rxindex = 0;
		memset(uart1_data_pack.rx.rawData, 0, sizeof(uart1_data_pack.rx.rawData));
	}
	if ((uart1_data_pack.rxindex > 0) &&
	    (uart1_data_pack.rxindex >= (uart1_data_pack.rx.Syncdata.length + 3))) {
		if (uart1_data_pack.rx.Syncdata.data[uart1_data_pack.rx.Syncdata.length] == rxCalcCheckSum()) {
			uart1_cmdHandle();
			mfp_tx_task();
		}
		uart1_data_pack.rxindex = 0;
		memset(uart1_data_pack.rx.rawData, 0, sizeof(uart1_data_pack.rx.rawData));
	}
}

/* CI1302 UART2: 8-byte frames 0xA5 .. 0xFB. */
void x_uart2_dateReceiveHandle(void)
{
	if (uart2_data_pack.rxindex != 0 && uart2_data_pack.timeout == 0) {
		uart2_data_pack.rxindex = 0;
		memset(uart2_data_pack.rxbuf, 0, sizeof(uart2_data_pack.rxbuf));
	}
	if (uart2_data_pack.rxindex >= 8) {
		if (uart2_data_pack.rxbuf[0] == 0xA5 && uart2_data_pack.rxbuf[7] == 0xFB)
			uart2_cmdHandle();
		else
			LOG_W("U2 bad 0x%02X..0x%02X", uart2_data_pack.rxbuf[0], uart2_data_pack.rxbuf[7]);

		uart2_data_pack.rxindex = 0;
		memset(uart2_data_pack.rxbuf, 0, sizeof(uart2_data_pack.rxbuf));
	}
}

/* CI1302 UART3: variable length + Modbus CRC at tail. */
void x_uart3_dateReceiveHandle(void)
{
	uint16_t crc1_t = 0, crc2_t = 0;

	if (uart3_data_pack.rxindex != 0 && uart3_data_pack.timeout == 0) {
		uart3_data_pack.rxindex = 0;
		memset(uart3_data_pack.rxbuf, 0, sizeof(uart3_data_pack.rxbuf));
	}

	if ((uart3_data_pack.rxindex > 0) && (uart3_data_pack.rxindex >= (uart3_data_pack.rxbuf[1] + 4))) {
		crc1_t = (uint16_t)((uart3_data_pack.rxbuf[uart3_data_pack.rxbuf[1] + 2] << 8) |
				    (uart3_data_pack.rxbuf[uart3_data_pack.rxbuf[1] + 3]));
		crc2_t = Modbus_Crc_Compute(uart3_data_pack.rxbuf, uart3_data_pack.rxbuf[1] + 2);

		if (crc2_t == crc1_t) {
			g_sysparam_st.ci1302.version =
				(uint16_t)(uart3_data_pack.rxbuf[4] << 8 | uart3_data_pack.rxbuf[5]);
			g_sysparam_st.ci1302.snorevolume =
				(uint16_t)(uart3_data_pack.rxbuf[6] << 8 | uart3_data_pack.rxbuf[7]);
			g_sysparam_st.ci1302.snoreState = uart3_data_pack.rxbuf[8];
			g_sysparam_st.sf.snorevolume = g_sysparam_st.ci1302.snorevolume;
			if (g_sysparam_st.ci1302.snoreState == 1 && g_sysparam_st.snoreIntervention.enable) {
				g_sysparam_st.sf.snorevolume_acc += g_sysparam_st.ci1302.snorevolume;
				g_sysparam_st.sf.snorevolume_cnt++;
			}
		} else {
			LOG_W("U3 crc len=%u", (unsigned)uart3_data_pack.rxbuf[1]);
		}
		uart3_data_pack.rxindex = 0;
		memset(uart3_data_pack.rxbuf, 0, sizeof(uart3_data_pack.rxbuf));
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	(void)huart;
	s_tx_busy = false;
}

/* UART1: MFP, 38400 8E1. */
void x_uart1_init(void)
{
	pinmux_uart1_init(UART_TX1_PIN, UART_RX1_PIN);

	UART_Server1_Config.UARTX = UART1;
	UART_Server1_Config.Init.BaudRate = UART_BAUDRATE_38400;
	UART_Server1_Config.Init.MSBEN = 0;
	UART_Server1_Config.Init.Parity = UART_EVENPARITY;
	UART_Server1_Config.Init.StopBits = UART_STOPBITS1;
	UART_Server1_Config.Init.WordLength = UART_BYTESIZE8;
	HAL_UART_Init(&UART_Server1_Config);
	HAL_UART_Receive_IT(&UART_Server1_Config, &rx1_t_data, 1);
}

/* UART2: offline voice CI1302, 9600 8N1, 8-byte protocol. */
void x_uart2_init(void)
{
	pinmux_uart2_init(UART_TX2_PIN, UART_RX2_PIN);

	io_pull_write(UART_RX2_PIN, IO_PULL_UP);
	UART_Server2_Config.UARTX = UART2;
	UART_Server2_Config.Init.BaudRate = UART_BAUDRATE_9600;
	UART_Server2_Config.Init.MSBEN = 0;
	UART_Server2_Config.Init.Parity = UART_NOPARITY;
	UART_Server2_Config.Init.StopBits = UART_STOPBITS1;
	UART_Server2_Config.Init.WordLength = UART_BYTESIZE8;
	HAL_UART_Init(&UART_Server2_Config);
	HAL_UART_Receive_IT(&UART_Server2_Config, &rx2_t_data, 1);
}

/* UART3: CI1302 snore / status, 115200 8N1, Modbus CRC. */
void x_uart3_init(void)
{
	pinmux_uart3_init(UART_TX3_PIN, UART_RX3_PIN);

	io_pull_write(UART_RX3_PIN, IO_PULL_UP);
	UART_Server3_Config.UARTX = UART3;
	UART_Server3_Config.Init.BaudRate = UART_BAUDRATE_115200;
	UART_Server3_Config.Init.MSBEN = 0;
	UART_Server3_Config.Init.Parity = UART_NOPARITY;
	UART_Server3_Config.Init.StopBits = UART_STOPBITS1;
	UART_Server3_Config.Init.WordLength = UART_BYTESIZE8;
	HAL_UART_Init(&UART_Server3_Config);
	HAL_UART_Receive_IT(&UART_Server3_Config, &rx3_t_data, 1);
}

/* Debug/status to MFP on UART1 (blocking TX). */
void x_uart_realtimeprint(void)
{
	static uint8_t uartTxbuff[50];
	memset(uartTxbuff, 0, sizeof(uartTxbuff));

	int i = 0;
	uint16_t pa_cur_t = g_sysparam_st.airpump.pa_cur;

	uartTxbuff[i++] = 0x5a;
	uartTxbuff[i++] = 0;
	uartTxbuff[i++] = 0;
	uartTxbuff[i++] = 0;
	uartTxbuff[i++] = VERSION & 0xff;
	uartTxbuff[i++] = (VERSION >> 8) & 0xff;
	uartTxbuff[i++] = g_sysparam_st.humandetection;
	uartTxbuff[i++] = pa_cur_t & 0xff;
	uartTxbuff[i++] = (pa_cur_t >> 8) & 0xff;
	uartTxbuff[i++] = 100;
	uartTxbuff[i++] = g_sysparam_st.airpump.inflate_onff;
	uartTxbuff[i++] = g_sysparam_st.airpump.deflate_onoff;
	uartTxbuff[i++] = g_sysparam_st.ai_adj;

	HAL_UART_Transmit(&UART_Server1_Config, uartTxbuff, i, 0xffff);
}

void uart_BLEvoice_send(void)
{
	static uint8_t IuartTxbuff[50];
	memset(IuartTxbuff, 0, sizeof(IuartTxbuff));

	int i = 0;
	uint32_t temp = 0;
	uint32_t utc;

	utc = x_time_RTC_ToUTC();

	IuartTxbuff[i++] = 0x5a;
	IuartTxbuff[i++] = 0;
	IuartTxbuff[i++] = 0xff;
	IuartTxbuff[i++] = 0xff;

	IuartTxbuff[i++] = g_sysparam_st.snoreIntervention.trigTimer & 0xff;
	IuartTxbuff[i++] = (g_sysparam_st.snoreIntervention.trigTimer >> 8) & 0xff;
	IuartTxbuff[i++] = (g_sysparam_st.snoreIntervention.trigTimer >> 16) & 0xff;
	IuartTxbuff[i++] = (g_sysparam_st.snoreIntervention.trigTimer >> 24) & 0xff;

	IuartTxbuff[i++] = g_sysparam_st.airpump.adjPa & 0xff;
	IuartTxbuff[i++] = (g_sysparam_st.airpump.adjPa >> 8) & 0xff;

	IuartTxbuff[i++] = g_sysparam_st.sf.ThresholdMove & 0xff;
	IuartTxbuff[i++] = (g_sysparam_st.sf.ThresholdMove >> 8) & 0xff;

	IuartTxbuff[i++] = (utc >> 0) & 0xff;
	IuartTxbuff[i++] = (utc >> 8) & 0xff;
	IuartTxbuff[i++] = (utc >> 16) & 0xff;
	IuartTxbuff[i++] = (utc >> 24) & 0xff;

	IuartTxbuff[1] = (uint8_t)(i - 2);
	temp = Modbus_Crc_Compute(IuartTxbuff, i);

	IuartTxbuff[i++] = temp & 0xff;
	IuartTxbuff[i++] = (temp >> 8) & 0xff;

	HAL_UART_Transmit(&UART_Server1_Config, IuartTxbuff, i, 0xffff);
}

/* Query / poke CI1302 internal params on UART3 (blocking TX). */
void x_uart_Internalparameterprint(void)
{
	static uint8_t IuartTxbuff[50];
	memset(IuartTxbuff, 0, sizeof(IuartTxbuff));

	int i = 0;
	uint32_t temp = 0;

	IuartTxbuff[i++] = 0x5a;
	IuartTxbuff[i++] = 0x05;
	IuartTxbuff[i++] = 0xff;
	IuartTxbuff[i++] = 0xff;
	temp = Modbus_Crc_Compute(IuartTxbuff, i);

	IuartTxbuff[i++] = temp & 0xff;
	IuartTxbuff[i++] = (temp >> 8) & 0xff;
	HAL_UART_Transmit(&UART_Server3_Config, IuartTxbuff, i, 0xffff);
}

/*
 * CI1302 offline voice config on UART2: 8-byte frame A5..FB.
 * voice_off / Hello_Ergo (0x21) / Hello_Bed (0x22) fixed payloads.
 */
static const uint8_t s_ci1302_frame_voice_off[8] =
	{ 0xA5, 0xFA, 0x00, 0x81, 0x19, 0x00, 0x39, 0xFB };
static const uint8_t s_ci1302_frame_hello_ergo[8] =
	{ 0xA5, 0xFA, 0x00, 0x81, 0x01, 0x00, 0x21, 0xFB };
static const uint8_t s_ci1302_frame_hello_bed[8] =
	{ 0xA5, 0xFA, 0x00, 0x81, 0x02, 0x00, 0x22, 0xFB };

void ci1302_uart2_voice_config_sync(bool force)
{
	static bool have_shadow;
	static bool sh_en;
	static uint8_t sh_ww;

	bool en = g_offline_voice.key_enable;
	uint8_t ww = g_offline_voice.wake_word;

	if (!force && have_shadow) {
		if (sh_en == en && (!en || sh_ww == ww))
			return;
	}

	const uint8_t *p = s_ci1302_frame_voice_off;
	if (en) {
		if (ww == Hello_Ergo)
			p = s_ci1302_frame_hello_ergo;
		else if (ww == Hello_Bed)
			p = s_ci1302_frame_hello_bed;
	}

	LOG_I("U2 cfg en=%u ww=%u %02X%02X%02X%02X%02X%02X%02X%02X", (unsigned)en, (unsigned)ww,
	      (unsigned)p[0], (unsigned)p[1], (unsigned)p[2], (unsigned)p[3], (unsigned)p[4],
	      (unsigned)p[5], (unsigned)p[6], (unsigned)p[7]);

	(void)HAL_UART_Transmit(&UART_Server2_Config, (uint8_t *)p, 8U, 100U);

	have_shadow = true;
	sh_en = en;
	sh_ww = ww;
}

void x_uart_init(void)
{
	x_uart1_init();
	x_uart2_init();
	x_uart3_init();
	LOG_I("UART init 38400/9600/115200");
	check_offline_voice_keys();
	ci1302_uart2_voice_config_sync(true);
}
