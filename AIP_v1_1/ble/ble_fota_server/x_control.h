#ifndef _X_CONTROL_H
#define _X_CONTROL_H
#include "stdint.h"

void control_timer10ms(void);
void adjust_pressure(int target_pa,int tolerance);
void adjust_pressure_Overchargedischarge(int target_pa,uint32_t overvlue,int tolerance) ;
void x_control_init(void);
void StretchingModeStop(void);
void StretchingModeStart(void);


void app_airpump_inflate_on(void);

void app_airpump_inflate_off(void);

void app_airpump_deflate_on(void);

void app_airpump_deflate_off(void);

void adaptivecontrol_default(void);

void app_flatten(void);
#endif