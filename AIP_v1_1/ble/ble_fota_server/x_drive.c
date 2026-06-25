#include "x_drive.h"
#include "ls_hal_dmac.h"
#include "timer_event.h"
#include "reg_gpio.h"
#include <stdint.h>
#include "main.h"
#include "g.h"
#include "ls_hal_rtc.h"

/* LSGPTIMA PWM：预分频 8，周期 250，具体频率依 HCLK 与公式计算 */
#define TIM_PRESCALER     (8 - 1)
#define TIM_PERIOD        (250 - 1)
#define TIM_PULSE1        100
#define TIM_PULSE2        100
#define TIM_PULSE3        100

TIM_HandleTypeDef x_TimBHandle;
TIM_HandleTypeDef x_TimHandle;

calendar_cal_t calendar_cal;
calendar_time_t calendar_time;

/* 外部中断回调：鼾声检测脚脉冲时累加计数并清超时 */
void io_exti_callback(uint8_t pin, exti_edge_t edge)
{
	(void)edge;
	switch (pin) {
	case SNORE_OUT_PIN:
		g_sysparam_st.sf.snoreNub++;
		snore_timevoer = 0;
		LOG_I("snore cnt++");
		break;
	default:
		break;
	}
}

/* 气泵 PWM 占空比（GPTIMA 通道 3 / CCR3） */
void x_pump_pwmset(uint8_t set_value)
{
	LSGPTIMA->CCR3 = set_value;
}

/* 气阀 PWM 占空比（GPTIMA 通道 1 / CCR1） */
void x_valve_pwmset(uint8_t set_value)
{
	LSGPTIMA->CCR1 = set_value;
}

/* 抽气阀 PWM 占空比（GPTIMA 通道 2 / CCR2） */
void x_expump_pwmset(uint8_t set_value)
{
	LSGPTIMA->CCR2 = set_value;
}

/* 初始化 GPTIMA 三路 PWM 并启动，占空比清零（当前工程未在入口调用） */
static void x_Basic_PWM_Output_Cfg(void)
{
	TIM_OC_InitTypeDef sConfig = {0};

	x_TimHandle.Instance = LSGPTIMA;
	x_TimHandle.Init.Prescaler = TIM_PRESCALER;
	x_TimHandle.Init.Period = TIM_PERIOD;
	x_TimHandle.Init.ClockDivision = 0;
	x_TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
	x_TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	HAL_TIM_Init(&x_TimHandle);

	sConfig.OCMode = TIM_OCMODE_PWM1;
	sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfig.OCFastMode = TIM_OCFAST_DISABLE;

	sConfig.Pulse = TIM_PULSE1;
	HAL_TIM_PWM_ConfigChannel(&x_TimHandle, &sConfig, TIM_CHANNEL_1);

	sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfig.Pulse = TIM_PULSE2;
	HAL_TIM_PWM_ConfigChannel(&x_TimHandle, &sConfig, TIM_CHANNEL_2);

	sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfig.Pulse = TIM_PULSE3;
	HAL_TIM_PWM_ConfigChannel(&x_TimHandle, &sConfig, TIM_CHANNEL_3);

	HAL_TIM_PWM_Start(&x_TimHandle, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&x_TimHandle, TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&x_TimHandle, TIM_CHANNEL_3);
	x_valve_pwmset(0);
	x_expump_pwmset(0);
	x_pump_pwmset(0);
}

/* 配置 LED、拨键、无线充电检测脚及 MFP RS485 方向（UART_CTR） */
void x_io_init(void)
{
	io_cfg_output(LED_Blue_0_PIN);
	io_cfg_pushpull(LED_Blue_0_PIN);
	io_cfg_output(LED_Blue_1_PIN);
	io_cfg_pushpull(LED_Blue_1_PIN);
	io_cfg_output(LED_Blue_2_PIN);
	io_cfg_pushpull(LED_Blue_2_PIN);
	io_cfg_output(LED_Blue_3_PIN);
	io_cfg_pushpull(LED_Blue_3_PIN);
	io_cfg_output(LED_Green_1_PIN);
	io_cfg_pushpull(LED_Green_1_PIN);
	io_cfg_output(LED_Green_3_PIN);
	io_cfg_pushpull(LED_Green_3_PIN);
	io_cfg_output(LED_Red_all_PIN);
	io_cfg_pushpull(LED_Red_all_PIN);

	io_write_pin(LED_Blue_0_PIN, 1);
	io_write_pin(LED_Blue_1_PIN, 1);
	io_write_pin(LED_Blue_2_PIN, 1);
	io_write_pin(LED_Blue_3_PIN, 1);
	io_write_pin(LED_Green_1_PIN, 1);
	io_write_pin(LED_Green_3_PIN, 1);
	io_write_pin(LED_Red_all_PIN, 1);

	io_cfg_input(KEY1_PIN);
	io_pull_write(KEY1_PIN, IO_PULL_UP);
	io_cfg_input(KEY2_PIN);
	io_pull_write(KEY2_PIN, IO_PULL_UP);
	io_cfg_input(KEY3_PIN);
	io_pull_write(KEY3_PIN, IO_PULL_UP);
	io_cfg_input(KEY4_PIN);
	io_pull_write(KEY4_PIN, IO_PULL_UP);

	io_cfg_input(CHARGE_ABNORMAL_PIN);
	io_pull_write(CHARGE_ABNORMAL_PIN, IO_PULL_DOWN);
	io_cfg_input(CHARGE_NORMAL_PIN);
	io_pull_write(CHARGE_NORMAL_PIN, IO_PULL_DOWN);

	io_cfg_output(UART_CTR);
	io_write_pin(UART_CTR, 1); /* 1= 默认接收使能（依硬件接法） */
}

/* 查询无线充电状态：0 异常、1 正常、0xFF 非充电态 */
uint8_t on_WirelessCharege_status_get(void)
{
	if (io_read_pin(CHARGE_ABNORMAL_PIN))
		return 0;
	if (io_read_pin(CHARGE_NORMAL_PIN))
		return 1;
	return 0xff;
}

/* 鼾声检测脚：输入下拉、上升沿触发外部中断 */
void x_exti_init(void)
{
	io_cfg_input(SNORE_OUT_PIN);
	io_pull_write(SNORE_OUT_PIN, IO_PULL_DOWN);
	io_exti_config(SNORE_OUT_PIN, INT_EDGE_RISING);
}

/* RTC 初始化（LSI），并写入一组默认日历时间 */
void x_rtc_init(void)
{
	HAL_RTC_Init(RTC_CKSEL_LSI);

	calendar_cal.year = 25;
	calendar_cal.mon = 6;
	calendar_cal.date = 7;
	calendar_time.hour = 23;
	calendar_time.min = 59;
	calendar_time.sec = 55;
	calendar_time.week = 7;
	RTC_CalendarSet(&calendar_cal, &calendar_time);
}

/* 按年月日时分秒星期写入 RTC */
void x_rtc_set(uint16_t year, uint8_t Month, uint8_t Day, uint8_t Hour, uint8_t Minute, uint8_t Second,
	       uint8_t Week)
{
	calendar_cal.year = year % 100;
	calendar_cal.mon = Month;
	calendar_cal.date = Day;
	calendar_time.hour = Hour;
	calendar_time.min = Minute;
	calendar_time.sec = Second;
	calendar_time.week = Week;
	RTC_CalendarSet(&calendar_cal, &calendar_time);
}

/* 从 RTC 读到 g_sysparam_st 并更新内部 UTC 换算 */
void x_rtc_get(void)
{
	RTC_CalendarGet(&g_sysparam_st.calendar_cal, &g_sysparam_st.calendar_time);
	x_time_RTC_ToUTC();
}

/* 板级驱动初始化入口：GPIO、鼾声 EXTI、RTC */
void x_driver_init(void)
{
	x_io_init();
	x_exti_init();
	x_rtc_init();
}
