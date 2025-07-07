#ifndef _X_CONTROL_H
#define _X_CONTROL_H
#include "stdint.h"
#include "stdbool.h"

#define 		Hello_Ergo 		1
#define 		Hello_Bed 		2


void control_timer10ms(void);
void on_led_timeout_set(uint8_t led_num,uint8_t time_out);
void control_timer1000ms(void);
void adjust_pressure(int target_pa,int tolerance);
void adjust_pressure_Overchargedischarge(int target_pa,uint32_t overvlue,int tolerance) ;
void x_control_init(void);
void StretchingModeStop(void);
void StretchingModeStart(void);

void check_offline_voice_keys(void);

void app_airpump_inflate_on(void);

void app_airpump_inflate_off(void);

void app_airpump_deflate_on(void);

void app_airpump_deflate_off(void);

void adaptivecontrol_default(void);

void app_flatten(void);
#endif