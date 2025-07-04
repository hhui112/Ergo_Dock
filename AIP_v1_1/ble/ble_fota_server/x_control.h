#ifndef _X_CONTROL_H
#define _X_CONTROL_H
#include "stdint.h"
#include "stdbool.h"
typedef struct {
    bool enabled;           // en = 1 
    uint8_t wake_word;      // 1=Hello Ergo, 2=Hello Bed
} offline_voice_ctrl_t;

void control_timer10ms(void);
void on_led_timeout_set(uint8_t led_num,uint8_t time_out);
void control_timer1000ms(void);
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