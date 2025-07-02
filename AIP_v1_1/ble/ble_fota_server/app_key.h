#ifndef APP_KEY_H
#define APP_KEY_H

#define app_key_num1       0x01
#define app_key_num2       0x02
#define app_key_num3       0x04

void on_key_10ms_handle(void);
uint8_t app_slide_key_get(void);               //»¬¶¯°´¼ü
uint8_t app_touch_key_get(void);               //´¥¿Ø°´¼ü
uint8_t app_key_get(void);
#endif
