#include "x_drive.h"
#include "ls_hal_dmac.h"
#include "timer_event.h"
#include "reg_gpio.h"
#include <stdint.h>
#include "main.h"
#include "g.h"

#define MAX_COUNT 4
static const uint8_t Led_blue_pin[4]  = {LED_Blue_0_PIN ,LED_Blue_1_PIN ,LED_Blue_2_PIN ,LED_Blue_3_PIN};
static const uint8_t Led_Green_pin[2] = {                LED_Green_1_PIN,                LED_Green_3_PIN };
static const uint8_t Led_red_pin  = LED_Red_all_PIN ;
		
static bool led_flash_on_active = false;
static bool BLEPairing_led_on_active = false;
static uint8_t m_flash_times = 0;
uint8_t m_flash_ledlist[MAX_COUNT] = {0};
uint8_t led_flash_num = 0;

void app_led_reset_all(void)          
{
    for (uint8_t i = 0; i < 3; i++) {
      io_write_pin(Led_blue_pin[i], 1);
    }
    io_write_pin(Led_Green_pin[0], 1);
    io_write_pin(Led_red_pin, 1);

}

void BLEPairing_led_reset(void)
{
	  BLEPairing_led_on_active = false; 
    io_write_pin(Led_blue_pin[3], 1);
	  io_write_pin(Led_Green_pin[1], 1);
}

void BLEPairing_led_set(void)
{
	  BLEPairing_led_on_active = true;
    io_write_pin(Led_blue_pin[3], 0);
}

void app_led_set(uint8_t num,m_color_t color)          //ÌØ¶¨µÆ³£ÁÁ µÆºÅ£ºnum£¨ff È«ÁÁ£© ÑÕÉ«£ºcolor
{
	  if(num != 3)
		  app_led_reset_all();
		else 
		  BLEPairing_led_reset();
		switch(color)
		{
		  case blue: if(num < 4)  io_write_pin(Led_blue_pin[num],0);
			           else if(num == 0xff) {
							     for (uint8_t i = 0; i < 3; i++) {
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

void on_led_period_set(uint8_t num, m_color_t color, uint8_t time_1S)          //ÌØ¶¨µÆ¶ÌÔÝÁÁ µÆºÅ£ºnum ÑÕÉ«£ºcolor Ê±¼ä£ºtime_1S
{
    app_led_set(num,color);
	  uint8_t led_pin = 0;
	  if(color == blue)          led_pin = Led_blue_pin[num];
	  else if(color == green )   led_pin = (num == 1) ? Led_Green_pin[0]:((num == 3) ? Led_Green_pin[1] : 0);
	  else if(color == red )     led_pin = Led_red_pin;
	  on_led_timeout_set(led_pin,time_1S);
}

void app_led_flash_set(void)        //ÉÁË¸3´Î                            
{
	  memcpy(m_flash_ledlist, Led_blue_pin, sizeof(Led_blue_pin) - 1);
	  app_led_reset_all();
	  led_flash_num = 3;
    m_flash_times = 3 * 2;
}

bool on_led_flash_isActive(void)
{
	  return (m_flash_times > 0)?true:false;
}

bool on_BLEPairing_led_isActive(void)
{
	  return BLEPairing_led_on_active ;
}

//ÓïÒôµÆ»½ÐÑ8sÀ¶µÆ
void app_Receive_Wakeup_LedOn(void)
{
    on_led_period_set(3,blue,8);
}
//ÓïÒôµÆÂÌµÆÁÁ3sºó À¶µÆÔÙÁÁ5s
void app_Receive_Wakeup_LedOn_5s(void)
{
    on_led_period_set(3,blue,5);
}

//ÓïÒôµÆÉÁË¸2´ÎÀ¶µÆ
void app_NotReceive_LedFlash(void)
{
	  m_flash_ledlist[0] = LED_Blue_3_PIN;
	  BLEPairing_led_reset();
	  led_flash_num = 1;
    m_flash_times = 2 * 2;
}
//ÓïÒôµÆÁÁÆð3sÂÌµÆ + 5sÀ¶µÆ
void app_ReceiveCommand_LedOn(void)
{
	  on_led_period_set(3,green,3);
	  app_ReceiveCommand_LedEnable();
}
//
void on_led_10ms_handle(void)
{
    static uint8_t flash_interval = 20;
	  if(flash_interval > 0)
      flash_interval --;
    else if(flash_interval == 0 && m_flash_times > 0) {
		  flash_interval = 20;
			for(uint8_t idx = 0;idx < led_flash_num;idx++) {
				  io_toggle_pin(m_flash_ledlist[idx]);
			}
			m_flash_times --;
		}
}



