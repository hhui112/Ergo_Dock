#include "x_drive.h"
#include "ls_hal_dmac.h"
#include "timer_event.h"
#include "reg_gpio.h"
#include <stdint.h>
#include "main.h"
#include "g.h"
#include "app_key.h"

static uint8_t m_button_list[] = {KEY3_PIN};

static uint8_t current_key = 0;
#define DEBOUNCE_MAX_COUNT      3
uint16_t analog_key_deebounce(uint16_t button)    //消抖
{
    static uint8_t deb_arr[DEBOUNCE_MAX_COUNT] = {0,0,0};
    static uint8_t deb_cnt = 0;
    static uint16_t deb_old_val = 0;
    static uint16_t button_status = 0;
    uint16_t deb_val,tmp = 0;
    uint8_t i;

    deb_val = button;

    deb_arr[deb_cnt++] = deb_old_val ^ deb_val;             //保存按键变化状态
    deb_old_val = deb_val;
    deb_val = 0;
    i = DEBOUNCE_MAX_COUNT;
    while(i--)
        deb_val |= deb_arr[i];                            //按键综合变化
    tmp = (deb_old_val & (~deb_val));                     //将发生变化的 位 清零
    tmp |= (deb_val & button_status);                     //只有当存在 连续三次 即超过30ms时 回返回键值
    button_status = tmp;
    if(deb_cnt >= DEBOUNCE_MAX_COUNT) {
        deb_cnt = 0;
    }
    return button_status;
}

void on_key_10ms_handle(void)                //触控按键获取并消抖
{
    uint16_t key = 0;
    uint32_t idx;
    uint8_t key_nums = 0;
    static int8_t stop_key = 0;
    for(idx = 0;idx < sizeof(m_button_list)/sizeof(uint8_t); idx++ ) {
        if(!io_read_pin(m_button_list[idx])) {
            key |= (uint16_t)1u << idx;
            key_nums ++;
        }
    }
    current_key = analog_key_deebounce(key);
}

uint8_t app_slide_key_get(void)               //滑动按键
{
    if(!io_read_pin(KEY1_PIN)) {
		  return app_key_num1;
		}else if(!io_read_pin(KEY2_PIN)) {
		  return app_key_num2;
		}else 
		  return 0;
}

#define BUTTON_NUM    1
uint8_t app_touch_key_get(void)               //触控按键
{
	  uint8_t ret = 0;
	  uint8_t key_nums = 0;
	  uint8_t code_mask = 1;
    const uint8_t ButtonPosTable[BUTTON_NUM] =
//	|    0     |    S1    |    S2    |    S3    |    S4    |    S5    |    S6    |    S7    |
	  {  app_key_num3 };
    for(uint8_t i=0;i<BUTTON_NUM;i++){
        if((current_key & code_mask) != 0) {
            ret |= ButtonPosTable[i];
            key_nums ++;
        }
        code_mask <<= 1;		
    }

    return ret;
}

uint8_t app_key_get(void)
{
    uint8_t key = 0;
	  key = app_slide_key_get()|app_touch_key_get();
    return key;
}