//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//按键扫描处理函数库
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
#include "key_check.h"
//#include "Modbus_lib.h"
#include "main.h"
//extern com_UART COM1Data;
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
uchar8 keytest(void)//按键扫描函数
{
uchar8  key=0;
//uchar8 	key=0;
	
	io_read_pin(PB07);//
	if(io_read_pin(PB07)==0)//
	{
	  key |=0X01;
	}

	switch(key) 
	{
	case 0X01:return 1;//break;
	default :return 0;//break;
	}
}
//返回值含义：0:无按键按下; 1:K1; 2:K2; 3:K3; 4:K4;
//&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
uchar8 GetKey(void)//按键扫描值再加工函数
{
static uchar8 s_u8KeyState=KEY_STATE_INIT;
static uchar8 s_u8LastKey=KEY_NULL;
static uint16 IN_TimeCount=0; 
static uchar8 xiaodou=0;
uchar8 KeyTemp=KEY_NULL;
KeyTemp=keytest();            
switch(s_u8KeyState) 
     { 
         case KEY_STATE_INIT :  
                     if(KEY_NULL != (KeyTemp)) 
                     { 
                         s_u8KeyState = KEY_STATE_PRESS ; 
                     }  
                     break ; 
         case KEY_STATE_WOBBLE :       //消抖  
				             if(++xiaodou>1)
										 {
                     s_u8KeyState = KEY_STATE_PRESS ;  
										 xiaodou=0;
									   }
                     break ; 
         case KEY_STATE_PRESS :  
                     if(KEY_NULL != (KeyTemp)) 
                     { 
                         s_u8LastKey = KeyTemp ; //保存键值,以便在释放按键状态返回键值 
                        // s_u8LastKey |= KEY_DOWN ;   //
											   KeyTemp=s_u8LastKey | KEY_DOWN ;
                         s_u8KeyState = KEY_STATE_LONG3 ; //
                     } 
                     else 
                     { 
                         s_u8KeyState = KEY_STATE_RELEASE ; //
                     }  
                     break ; 
         case KEY_STATE_LONG3 : //长按3秒 
                     if(KEY_NULL != (KeyTemp)) 
                     { 				   
												if(++IN_TimeCount==KEY_LONG_PERIOD)
											 {   
												 //IN_TimeCount=0;
												 KeyTemp = s_u8LastKey|KEY_LONG3 ;   //
												 //s_u8LastKey=0; 
												 //s_u8KeyState = KEY_STATE_RELEASE; //状态转移
											 }
											 else
                        KeyTemp = s_u8LastKey|KEY_keepon;//按键持续按下									 
                     } 
                     else 
                     { 
											 s_u8KeyState = KEY_STATE_RELEASE ;//
											 IN_TimeCount=0;
                     }  
                     break ; 

         case KEY_STATE_RELEASE :  
		             if(KeyTemp==0)
					        {
                     KeyTemp = s_u8LastKey|KEY_UP ; 
                     s_u8KeyState = KEY_STATE_INIT ;//状态转移  
					         }
                     break ; 
         default : break ; 
     }      
return KeyTemp ; //返回键值，参照返回值说明表格
}

