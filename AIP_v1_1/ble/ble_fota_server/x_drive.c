#include "x_drive.h"
#include "ls_hal_dmac.h"
#include "timer_event.h"
#include "reg_gpio.h"
#include <stdint.h>
#include "main.h"
#include "g.h"
#include "ls_hal_rtc.h"

#define TIM_PRESCALER     (8-1)//(SDK_HCLK_MHZ-1)  8=32K 35=7.2K
#define TIM_PERIOD        (250 - 1) /* Period Value  */
#define TIM_PULSE1        100       /* Capture Compare 1 Value  */
#define TIM_PULSE2        100       /* Capture Compare 2 Value  */
#define TIM_PULSE3        100       /* Capture Compare 3 Value  */

union SyncCommunicationData_t g_Sync_TX;
union SyncCommunicationData_t g_Sync_TX;


TIM_HandleTypeDef x_TimBHandle;
TIM_HandleTypeDef x_TimHandle;

calendar_cal_t calendar_cal;
calendar_time_t calendar_time;

void io_exti_callback(uint8_t pin,exti_edge_t edge) 
{
    switch (pin)
    {
    case SNORE_OUT_PIN:			
			g_sysparam_st.sf.snoreNub++;
			snore_timevoer=0;
			LOG_I("snoreNub++ \r\n ");
        break;
    default:
        break;
    }
}



void x_pump_pwmset(uint8_t set_value)
{
	LSGPTIMA->CCR3 = set_value;
}

void x_valve_pwmset(uint8_t set_value)
{
	LSGPTIMA->CCR1 = set_value;
}
void x_expump_pwmset(uint8_t set_value)
{
	LSGPTIMA->CCR2 = set_value;
}



static void x_Basic_PWM_Output_Cfg(void)//PWM
{
	
    TIM_OC_InitTypeDef sConfig = {0};


//		pinmux_gptima1_ch1_init(VALVE_CTR_PIN , true, 0);//气泵
//		pinmux_gptima1_ch2_init(EXPUMPCRT_CTR_PIN, true, 0);//加热布
//    pinmux_gptima1_ch3_init(PUMPCRT_CTR_PIN , true, 0);//气泵

		LOG_I("PWM_IO OK ");
    /*##-1- Configure the TIM peripheral #######################################*/
    x_TimHandle.Instance = LSGPTIMA;//选择定时器A
    x_TimHandle.Init.Prescaler = TIM_PRESCALER; //预分频设置
    x_TimHandle.Init.Period = TIM_PERIOD;//自动重载寄存器
    x_TimHandle.Init.ClockDivision = 0;
    x_TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    x_TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Init(&x_TimHandle);//
		LOG_I("TIM_Init OK ");
    /*##-2- Configure the PWM channels #########################################*/
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
		LOG_I("PWM channel OK ");
//    sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
//    sConfig.Pulse = TIM_PULSE4;
//    HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_4);

    /*##-3- Start PWM signals generation #######################################*/
    HAL_TIM_PWM_Start(&x_TimHandle, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&x_TimHandle, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&x_TimHandle, TIM_CHANNEL_3);
   // HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_4);
		x_valve_pwmset(0);//change PWM
		x_expump_pwmset(0);
		x_pump_pwmset(0);

}

// I2C??? MFP_CTR 腿玩输出 拉高
void x_io_init(void)
{
		///////////////LED//////////////////
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
	
	io_write_pin(LED_Blue_0_PIN,1);
	io_write_pin(LED_Blue_1_PIN,1);
	io_write_pin(LED_Blue_2_PIN,1);
	io_write_pin(LED_Blue_3_PIN,1);
	io_write_pin(LED_Green_1_PIN,1);
	io_write_pin(LED_Green_3_PIN,1);
	io_write_pin(LED_Red_all_PIN,1);
	
    ///////////////KEY//////////////////
	io_cfg_input(KEY1_PIN);   
	io_pull_write(KEY1_PIN,IO_PULL_UP);
	
	io_cfg_input(KEY2_PIN);   
	io_pull_write(KEY2_PIN,IO_PULL_UP);
	
	io_cfg_input(KEY3_PIN);   
	io_pull_write(KEY3_PIN,IO_PULL_UP);
	
	io_cfg_input(KEY4_PIN);   
	io_pull_write(KEY4_PIN,IO_PULL_UP);
	
		///////////////MFP_CTRL//////////////////
	io_cfg_output(UART_CTR); 		// 
	io_write_pin(UART_CTR,1);		//拉高 接收模式
}

void app_led_reset_all(void)          
{
		uint8_t Led_blue_pin[4]  = {LED_Blue_0_PIN ,LED_Blue_1_PIN ,LED_Blue_2_PIN ,LED_Blue_3_PIN};
		uint8_t Led_Green_pin[2] = {                LED_Green_1_PIN,                LED_Green_3_PIN };
		uint8_t Led_red_pin  = LED_Red_all_PIN ;
    for (uint8_t i = 0; i < 4; i++) {
      io_write_pin(Led_blue_pin[i], 1);
    }
    io_write_pin(Led_Green_pin[0], 1);
    io_write_pin(Led_Green_pin[1], 1);
    io_write_pin(Led_red_pin, 1);

}

void app_led_set(uint8_t num,m_color_t color)          //num:灯号（ff 全亮） 
{
	  uint8_t Led_blue_pin[4]  = {LED_Blue_0_PIN ,LED_Blue_1_PIN ,LED_Blue_2_PIN ,LED_Blue_3_PIN};
		uint8_t Led_Green_pin[2] = {                LED_Green_1_PIN,                LED_Green_3_PIN };
		uint8_t Led_red_pin  = LED_Red_all_PIN ;
	
		app_led_reset_all();
		
		switch(color)
		{
		  case blue: if(num < 4)  io_write_pin(Led_blue_pin[num],0);
			        else if(num == 0xff) {
							  for (uint8_t i = 0; i < 4; i++) {
                  io_write_pin(Led_blue_pin[i], 0);
                }
							}
				break; 
			case green:if (num == 1) {
                io_write_pin(Led_Green_pin[0], 0);
             } else if (num == 3) {
                io_write_pin(Led_Green_pin[1], 0);
             } else if (num == 0xFF) {
                io_write_pin(Led_Green_pin[0], 0);
                io_write_pin(Led_Green_pin[1], 0);
             }
				break;
		  case red:io_write_pin(Led_red_pin, 0);
				break;
			default:
				break;
		}
		
}


// 上升沿 打鼾
void x_exti_init(void)
{
    io_cfg_input(SNORE_OUT_PIN);    //LOWPOWER_M0_BTN config input  
    io_pull_write(SNORE_OUT_PIN, IO_PULL_DOWN);    //LOWPOWER_M0_BTN config pull down
    io_exti_config(SNORE_OUT_PIN,INT_EDGE_RISING);    //LOWPOWER_M0_BTN interrupt Rising edge
}


void x_Basic_Timer_Cfg(void)//10us中断一次
{ 

    x_TimBHandle.Instance           = LSGPTIMB;
    x_TimBHandle.Init.Prescaler     = (8-1);
    x_TimBHandle.Init.Period        = (80 - 1);
    x_TimBHandle.Init.ClockDivision = 0;
    x_TimBHandle.Init.CounterMode   = TIM_COUNTERMODE_UP;
    HAL_TIM_Init(&x_TimBHandle);
    HAL_TIM_Base_Start_IT(&x_TimBHandle);
}


//RTC Init

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
    RTC_CalendarSet(&calendar_cal,&calendar_time);
}




void x_rtc_set(uint16_t year,uint8_t Month,uint8_t Day,uint8_t Hour,uint8_t Minute,uint8_t Second,uint8_t Week)
{
	
		calendar_cal.year = year%100;
    calendar_cal.mon = Month;
    calendar_cal.date = Day;
    calendar_time.hour = Hour;
    calendar_time.min = Minute;
    calendar_time.sec = Second;
    calendar_time.week = Week; 
		RTC_CalendarSet(&calendar_cal,&calendar_time);
}

void x_rtc_get(void)
{
		RTC_CalendarGet(&g_sysparam_st.calendar_cal,&g_sysparam_st.calendar_time);
		x_time_RTC_ToUTC();
}

void x_driver_init(void)
{
	x_io_init();  // MFP_DE、(灯)
	x_exti_init();
	x_rtc_init();
}
