#ifndef __TIMER_EVENT_H
#define __TIMER_EVENT_H
#include "stdint.h"

#include "builtin_timer.h"
#include "ls_hal_adc.h"
#include "ls_hal_timer.h"
#include "ls_hal_adc.h"
#include "ls_hal_dmac.h"
#include "ble_base_set.h"
#include "Judger.h"

#define pingtang_pressset  1200//平躺时气囊设置气压
#define cetang_pressset  2000//侧躺时气囊设置气压
#define  pump_type 1//0 普通气泵  1 压电静音气泵
enum ADC_DMA_NUM
{
	ADCpress_R=0X00,
	ADCvalve_I,
	ADCpump_I,
	ADCheat_I,
	ADCtemp_I,
};

/*
VX.Y.Z
X:软件功能重大改变导致断代,前后不可兼容
Y:硬件是否兼容判断,编号一样说明硬件可以兼容,软件可以升级
Z:功能有变更或BUG修复等
*/
#define programVdef  "V1.0.0"      
#define programVdef_X    1
#define programVdef_Y    0
#define programVdef_Z    0

typedef struct
{
	uint8_t ble_data_receive[50];
	uint8_t lengh;
}ble_data_init;

typedef struct {
	uint16_t NEWAD;
	uint16_t AD_AVG;
}ADValue_def;

typedef struct {
	ADValue_def temp_AD;
	ADValue_def Iheat_AD;
	ADValue_def pump_AD;
	ADValue_def PRESS_AD;
	ADValue_def valve_AD;
}AD_Value_init;

//__attribute__((packed))作用是告诉编译器按照你实际占用字节数进行对齐，取消结构在编译过程中的优化对齐。

//气囊控制数据上行结构体
typedef struct {
	uint8_t programV[3];//软件版本
	uint8_t flag;//常规标志
	uint16_t airpressure;//当前气压值
	uint16_t bump_I;//气泵当前电流
	uint16_t valve_I;//气阀当前电流
	uint8_t postureflag;//0:平躺  1：侧躺
	uint16_t snore_n;//打鼾次数记录，启英泰伦音频方案
	uint8_t snore_sta;//打鼾当前状态，启英泰伦音频方案
}__attribute__((packed))airbag_parameter_def;//气囊控制参数定义


//压力毯数据结构体
#define clothpress_ch_max 16
typedef struct {
	uint8_t read_flag;//AD读取标志，每次通道切换需要一段延时，通过该标志判断读取数据的时间
	uint8_t index;
	uint16_t value[clothpress_ch_max];
	uint16_t lastvalue[clothpress_ch_max];
}__attribute__((packed))clothpress_def;


typedef enum 
{
	air_outstop=0,
	base_state1,
	base_state2,
	air_outstart,
	base_state3,
	air_outing,
	air_inputstart,
	base_state5,
	base_state6,
	air_inputing,
	air_inputstop,
	base_state8,
	base_state9,
	base_state10,
}airbag_ctrbase_def;

typedef struct
{
	uint8_t been_sta;//蜂鸣器开关状态
	uint8_t been_n;//蜂鸣器响的次数
	uint8_t been_tup;//蜂鸣器开启时间计数
	uint8_t been_on_t;//蜂鸣器打开时间
	uint8_t been_off_t;//蜂鸣器关闭时间
	uint8_t PWM_duty;//占空比
	uint8_t PWM_cycle;//周期
	uint8_t PWM_upcount;//PWM中断计数值
}been_ctr_def;



extern uint16_t snore_timevoer;//停止打鼾超时计时器
extern airbag_ctrbase_def airbag_ctrbase;
extern been_ctr_def been_ctr;
extern ble_data_init ble_data_receive;
extern  uint16_t adddd;
extern airbag_parameter_def *airbagsetfile;
extern void ls_even_timer_init(void);
extern struct builtin_timer *xTimer_5ms_inst; 
extern struct builtin_timer *xTimer_10ms_inst; 
extern struct builtin_timer *xTimer_100ms_inst; 
extern struct builtin_timer *xTimer_1000ms_inst; 
void parameter_init(void);
void app_data_receive_manage(ble_data_init *p);

#endif
