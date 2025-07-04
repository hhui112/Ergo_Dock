#ifndef APP_LED_H
#define APP_LED_H

typedef enum
{
  none,
	blue = 1,
	green = 2,
	red = 3,
}m_color_t;

void app_led_reset_all(void);          
void app_led_set(uint8_t num,m_color_t color);          //num:µÆºÅ£¨ff È«ÁÁ£© 
void on_led_10ms_handle(void);
bool on_led_flash_isActive(void);
void app_led_flash_set(void);
void BLEPairing_led_set(void);
void BLEPairing_led_reset(void);
bool on_BLEPairing_led_isActive(void);
void app_Receive_Wakeup_LedOn(void);
void app_NotReceive_LedFlash(void);
void app_ReceiveCommand_LedOn(void);
#endif
