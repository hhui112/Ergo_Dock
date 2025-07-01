#ifndef __HEAT_CONTROL_H
#define __HEAT_CONTROL_H
#include "timer_event.h"
#include "PID_function.h"
#include "stdint.h"

//自动模式1控制参数定义
typedef struct {
	uint8_t temp_num;
	uint16_t time_d;
	uint8_t cyclic_n;
}auto_mode_per_def;

typedef struct {
	uint8_t  temperater;//手动模式下目标温度设置
	uint8_t  timerfunction:1;//定时功能开关，0关定时，1开定时
	uint16_t  timeset;//定时时间设置
}__attribute__((packed))manual_init;//手动模式参数定义

typedef struct {
	uint8_t  start_temp;//开始温度
	uint8_t  end_temp;//结束温度
	uint16_t processtime;//过程时间
}__attribute__((packed))auto1_init;//自动模式参数定义

typedef struct {
	uint8_t  temperater;//温度设置
	uint16_t processtime;//过程时间
}__attribute__((packed))auto2_init;//自动模式参数定义

typedef struct {
	uint8_t app_B;
	uint8_t app_G;
	uint8_t app_R;
}__attribute__((packed))APPctr_ligth_set_init;//APP控制时按键背光灯颜色设置

typedef struct {
	uint8_t time_B;//
	uint8_t time_G;//
	uint8_t time_R;//
	uint16_t time_set;//定时时间
	uint8_t temp;//温度
}__attribute__((packed))keyctr_ligth_time_set_init;//

typedef struct {
	uint8_t programV[3];//软件版本
	uint8_t mode:4;//模式，0手动模式，1自动模式1，2自动模式2，
	uint8_t heat:1;//加热开关，0关，1开
	uint8_t time_n:3;//当前按键切换定时记录
	uint8_t temperature;//当前实时温度，摄氏度
	uint16_t I_heat;//当前加热布电流
	uint16_t time_down;//倒计时实时数据
	manual_init manual_mode;//手动模式下参数设置
	auto1_init auto1_mode;//自动模式1下参数设置
	auto2_init auto2_mode;//自动模式2下参数设置
	APPctr_ligth_set_init APPctr_ligth_set;
	keyctr_ligth_time_set_init keyctr_ligth_time_set[3];
}__attribute__((packed))parameter_set;	

extern uint8_t control_sta;
extern float pillotmpe;//当前温度
extern float pillosettmpe;//目标温度
extern pid_def pid;

float temperature_get(AD_Value_init *ADfortemp);
void temperature_ctr(parameter_set *file);
void off_heat(parameter_set *file);
#endif
