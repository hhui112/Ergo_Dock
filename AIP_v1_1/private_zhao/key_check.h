#ifndef _key_check_H
#define _key_check_H
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
#include "main.h"//语音识别系统宏定义
#define uchar8 unsigned char
#define uint16 unsigned int
#define ulong32 unsigned long

#define key1lv    P1_1  

#define KEY_NULL           0x00
#define KEY_DOWN           0x80 
#define KEY_LONG3          0x40 
#define KEY_LONG5          0x20 
#define KEY_LONG7          0x50
#define KEY_UP             0x10
#define KEY_keepon         0x30//按键持续按下

#define KEY_LONG_PERIOD           300 
#define KEY_CONTINUE_PERIOD       500
#define KEY_long_cl_PERIOD        700

#define KEY_STATE_INIT         0 
#define KEY_STATE_WOBBLE       1 
#define KEY_STATE_PRESS        2 
#define KEY_STATE_LONG3         3 
#define KEY_STATE_LONG5         4
#define KEY_STATE_LONG7         5
#define KEY_STATE_CONTINUE     6 
#define KEY_STATE_RELEASE      7

uchar8 keytest(void);//按键扫描函数
uchar8 GetKey(void);//按键扫描值再加工函数
//uchar8 receive_data(void);
#endif
