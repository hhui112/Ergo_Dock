#include "APP_io_init.h"
#include "ls_hal_dmac.h"
#include "timer_event.h"
#include "reg_gpio.h"
#include <stdint.h>
#include "g.h"
#define TIM_PRESCALER     (8-1)//(SDK_HCLK_MHZ-1)  8=32K 35=7.2K
#define TIM_PERIOD        (250 - 1) /* Period Value  */
#define TIM_PULSE1        125       /* Capture Compare 1 Value  */
#define TIM_PULSE2        200       /* Capture Compare 2 Value  */
#define TIM_PULSE3        100       /* Capture Compare 3 Value  */
#define TIM_PULSE4        50        /* Capture Compare 4 Value  */

#define TIMEB_PRESCALER (8-1)//(SDK_HCLK_MHZ-1)
#define TIMEB_PERIOD (80 - 1) /* Period Value  */

DEF_DMA_CONTROLLER(dmac1_inst,DMAC1);

//TIM_HandleTypeDef TimHandle;

//void changePWM_PULSE(uint8_t PULSE_SET)//PWM?????
//{	
//		/* Set the Capture Compare Register value */
//  LSGPTIMA->CCR3 = PULSE_SET;
//}

//void changeLEDPWM_PULSE(uint8_t R_SET,uint8_t G_SET,uint8_t B_SET)//PWM
//{	
//		/* Set the Capture Compare Register value */
//  LSGPTIMA->CCR1 = 255-G_SET;//G
//	LSGPTIMA->CCR2 = 255-R_SET;//R
//	LSGPTIMA->CCR4 = 255-B_SET;//B
//}
//气阀PWM输出设置



TIM_HandleTypeDef TimHandle;
static void Basic_PWM_Output_Cfg(void)//PWM
{
	
    TIM_OC_InitTypeDef sConfig = {0};

 //   gptimb1_ch1_io_init(PA00, true, 0);
//    gptimb1_ch2_io_init(PA01, true, 0);
		pinmux_gptima1_ch1_init(PA15, true, 0);//气阀
		pinmux_gptima1_ch2_init(PA14, true, 0);//加热布
    pinmux_gptima1_ch3_init(PB02, true, 0);//气泵
	//	pinmux_gptima1_ch4_init(PB07, true, 0);//蜂鸣器
//    gptimb1_ch4_io_init(PB15, true, 0);
		LOG_I("PWM_IO OK ");
    /*##-1- Configure the TIM peripheral #######################################*/
    TimHandle.Instance = LSGPTIMA;//选择定时器A
    TimHandle.Init.Prescaler = TIM_PRESCALER; //预分频设置
    TimHandle.Init.Period = TIM_PERIOD;//自动重载寄存器
    TimHandle.Init.ClockDivision = 0;
    TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Init(&TimHandle);//
LOG_I("TIM_Init OK ");
    /*##-2- Configure the PWM channels #########################################*/
    sConfig.OCMode = TIM_OCMODE_PWM1;
    sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfig.OCFastMode = TIM_OCFAST_DISABLE;

    sConfig.Pulse = TIM_PULSE1;
    HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_1);

    sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfig.Pulse = TIM_PULSE2;
    HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_2);

    sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfig.Pulse = TIM_PULSE3;
    HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_3);
LOG_I("PWM channel OK ");
//    sConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
//    sConfig.Pulse = TIM_PULSE4;
//    HAL_TIM_PWM_ConfigChannel(&TimHandle, &sConfig, TIM_CHANNEL_4);

    /*##-3- Start PWM signals generation #######################################*/
    HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_3);
   // HAL_TIM_PWM_Start(&TimHandle, TIM_CHANNEL_4);

//buzzer_pwmset(128);
//changeLEDPWM_PULSE(128,128,128);
}



TIM_HandleTypeDef TimBHandle;

void Basic_Timer_Cfg(void)//10us中断一次
{ 

    TimBHandle.Instance           = LSGPTIMB;
    TimBHandle.Init.Prescaler     = TIMEB_PRESCALER;
    TimBHandle.Init.Period        = TIMEB_PERIOD;
    TimBHandle.Init.ClockDivision = 0;
    TimBHandle.Init.CounterMode   = TIM_COUNTERMODE_UP;
    HAL_TIM_Init(&TimBHandle);
    HAL_TIM_Base_Start_IT(&TimBHandle);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == LSGPTIMB)
    {
//			if(been_ctr.been_sta==1)
//			{
//				if(been_ctr.PWM_upcount<been_ctr.PWM_cycle)
//					been_ctr.PWM_upcount++;
//				else
//					been_ctr.PWM_upcount=0;
//				
//				if(been_ctr.PWM_upcount==been_ctr.PWM_duty)
//					io_write_pin(PB07,0);
//				if(been_ctr.PWM_upcount==0)
//					io_write_pin(PB07,1);
//		   }
//			else
//			{
//				io_write_pin(PB07,0);
//			}
        //io_toggle_pin(PB07);
    }
}

void driver_init(void)
{
	//io init
	//io_cfg_output(PB08);   //PB09 config output
	//io_cfg_output(PA15);   //PB09 config output
	//io_cfg_output(PB13);   //PB09 config output
	
	io_cfg_output(PA05);//EN1
	io_cfg_pushpull(PA05);//设置为推挽
	
	io_cfg_output(PA06);//A0
	io_cfg_pushpull(PA06);//设置为推挽
	
	io_cfg_output(PA07);//EN2
	io_cfg_pushpull(PA07);//设置为推挽
	
	io_cfg_output(PA08);//A1
	io_cfg_pushpull(PA08);//设置为推挽
	
	io_cfg_output(PA09);//A2
	io_cfg_pushpull(PA09);//设置为推挽
	
	io_cfg_output(PA12);//drive1
	io_cfg_opendrain(PA12);//设置为推挽
	
	io_cfg_output(PA13);//drive2
	io_cfg_opendrain(PA13);//设置为推挽
	
		io_cfg_input(PB07);   //鼾声识别信号输出
//	io_cfg_pushpull(PB07);//设置为上拉
	io_pull_write(PB07,IO_PULL_UP);
	
//	io_cfg_output(PB07);   //蜂鸣器
//	io_cfg_pushpull(PB07);//设置为推挽

//  io_write_pin(PB08,1);  //PB09 write low power 
 // io_write_pin(PA15,1);  //PB09 write low power
	//io_write_pin(PB13,1);  //PB09 write low power
	
	io_write_pin(PA05,0);//EN1
	io_write_pin(PA06,0);//A0
	io_write_pin(PA07,0);//EN2
	io_write_pin(PA08,0);//A1
	io_write_pin(PA09,0);//A2
	io_write_pin(PA12,0);//drive1
	io_write_pin(PA13,0);//drive2
//	io_write_pin(PB07,0);  //蜂鸣器
	//////////////模拟I2C初始化////////////////////
	//PB03:I2C_SCL   PB04:I2C_SDA

	///////////////PWM//////////////////
	Basic_PWM_Output_Cfg();
	LOG_I("init PWM OK ");
	//////////////AD/////////////////////
	//PA00(AIN4:压力布AD)   
	//PA01(AIN5:电磁阀电流AD)   
	//PA02(AIN6:气泵电流AD)  
	//PA03(AIN7:加热布电流AD)  
	//PA04(AIN8:加热布温度AD)
	//adc_io_init();
	//lsadc_multi_channel_single_sampling_init();
		LOG_I("init ADC OK ");
	//////////TIMEB定时器中断/////////////
	Basic_Timer_Cfg();
	////////////配置参数初始化////////////
	parameter_init();
	
}

