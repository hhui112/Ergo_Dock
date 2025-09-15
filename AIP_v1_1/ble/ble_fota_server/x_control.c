#include "x_control.h"
#include "g.h"
#include "ble_base_set.h"

static bool chargeAbnormal_flag = false;
static uint8_t chargeAbnormal_timeout = 0;
static bool keys_ignore = false;
static uint8_t led_off_count = 0;
static uint8_t led_off_pin = 0;
static bool ReceiveCommand_flag = false;
#define LED_MAX 4
typedef struct {
    uint8_t pin;
    uint8_t timeout;
} led_task_t;
static led_task_t led_task[LED_MAX];


void check_offline_voice_keys(void)
{
    uint8_t key1 = io_read_pin(KEY1_PIN);
    uint8_t key2 = io_read_pin(KEY2_PIN);

    if (key1 == 1 && key2 == 1)
    {
        g_offline_voice.key_enable = false; 
				g_offline_voice.wake_word	= 0;	
    }
    else
    {
        g_offline_voice.key_enable = true;   
        if (key1 == 0)
            g_offline_voice.wake_word = Hello_Ergo; // 1=Hello Ergo
        else if (key2 == 0)
            g_offline_voice.wake_word = Hello_Bed; // 2=Hello Bed
    }
		if(g_sysparam_st.ubb == 1){
			g_offline_voice.ubb_enable = true;		// light_on
		}else{
			g_offline_voice.ubb_enable = false;		// litht off
		}
} 

void control_timer10ms(void)
{
	  static uint8_t last_key = 0;
	  uint8_t key = app_touch_key_get();
	  static uint16_t key_repeat = 0;
	  if(last_key == 0 && key != 0)  {

		} else if(last_key == key && key != 0) {
		  if(key_repeat < UINT16_MAX) 
				key_repeat ++;
			if(key_repeat == 300 && key == app_key_num4) {
			  keys_ignore = true;
				if(g_sysparam_st.ble.ble_pair_flag == 0){
						g_sysparam_st.ble.ble_pair_flag = 1;
						start_adv();
				}else{
						// g_sysparam_st.ble.ble_pair_flag = 0;
						g_sysparam_st.ble.ble_pair_time = 1;
						stop_adv();
				}

		/*
				if(!on_BLEPairing_led_isActive()){
					start_adv();
				  BLEPairing_led_set();
				}
        else{
				  BLEPairing_led_reset();
					stop_adv();
				}
		*/
			}
			if(key_repeat == 500 && key == app_key_num3) {
			  app_led_flash_set();
				keys_ignore = true;
				g_sysparam_st.AntiSnore_intensity = 0;
				//关闭
			}
		}else if(last_key != 0 && key == 0) {
			LOG_I("key %x released",last_key);
			if(last_key == app_key_num3 && !keys_ignore) {
				chargeAbnormal_flag = false;
				g_sysparam_st.AntiSnore_intensity = (g_sysparam_st.AntiSnore_intensity < 3)? g_sysparam_st.AntiSnore_intensity + 1:1;
				uint8_t led_num = g_sysparam_st.AntiSnore_intensity - 1;
				// app_led_set(led_num,blue);
				app_key_blue_10s(led_num); // 调整为短按触发后指示灯亮10秒自动熄灭
			}
		}else {
		  key_repeat = 0;
			keys_ignore = false;
		}
	
	  last_key = key;
		
		check_offline_voice_keys();
}

void on_led_timeout_set(uint8_t led_pin,uint8_t time_out)
{
    //led_off_pin = led_pin;
    //led_off_count = time_out;
	    // 如果已存在相同pin，就更新倒计时
	    if (led_pin == LED_Blue_3_PIN && g_sysparam_st.ble.ble_pair_flag && g_sysparam_st.ble.ble_pair_time > 0) {
        return;  // 蓝牙配对期间，不管理LED3超时
    }
				
    for (int i = 0; i < LED_MAX; i++) {
        if (led_task[i].pin == led_pin) {
            led_task[i].timeout = time_out;
						LOG_I("PIN = %d time_out = %d \n",led_pin,time_out);
            return;
        }
    }
    // 否则找个空位存新的
    for (int i = 0; i < LED_MAX; i++) {
        if (led_task[i].pin == 0) {
            led_task[i].pin = led_pin;
            led_task[i].timeout = time_out;
					LOG_I("NEW PIN = %d time_out = %d \n",led_pin,time_out);
            return;
        }
    }
}

// 清除指定PIN的定时任务
void on_led_timeout_clear(uint8_t led_pin)
{
    for (int i = 0; i < LED_MAX; i++) {
        if (led_task[i].pin == led_pin) {
            led_task[i].pin = 0;
            led_task[i].timeout = 0;
            LOG_I("清除LED定时任务 PIN = %d\n", led_pin);
            return;
        }
    }
}

void app_ReceiveCommand_LedEnable(void)
{
    ReceiveCommand_flag = true;
}
void control_timer1000ms(void)
{
	static uint8_t last_charge_status = 0xff;
	uint8_t current_charge_status = on_WirelessCharege_status_get();
	static uint8_t charge_green_timeout = 0;	// 绿灯倒计时15s
	
	  // if(g_sysparam_st.AntiSnore_intensity == 0 && !on_led_flash_isActive())
	  if(!on_led_flash_isActive())		// 不在闪烁中
		{
      if(!current_charge_status && !chargeAbnormal_flag)	// 充电异常
		  {
			  chargeAbnormal_flag = true;
		    app_led_set(0xFF,red);
		    chargeAbnormal_timeout = 30;
		  }
		  else if(current_charge_status == 1) 	// 正常充电
		  {
				if (last_charge_status != 1)
				{
						app_led_set(1, green);
						charge_green_timeout = 15;
				}
				chargeAbnormal_flag = false;
				chargeAbnormal_timeout = 0;
		  }
			else if(current_charge_status == 0xff)	// 没充电
			{
					// app_led_reset_all();
					io_write_pin(LED_Red_all_PIN, 1);
					io_write_pin(LED_Green_1_PIN, 1);
					chargeAbnormal_flag = false;
					chargeAbnormal_timeout = 0;
					charge_green_timeout = 0;
			}
			
		  if(chargeAbnormal_timeout > 0) chargeAbnormal_timeout --;  
		  if(chargeAbnormal_timeout == 0 && chargeAbnormal_flag) {
		  //30s计时结束后 业务逻辑待定 清楚红灯
			  app_led_reset_all();
		  }			
        if (charge_green_timeout > 0)
        {
            charge_green_timeout--;
            if (charge_green_timeout == 0)
            {
                io_write_pin(LED_Green_1_PIN, 1);
            }
        }				
		}
		
/*		
    if(led_off_count > 0) 
		{
		  led_off_count --;
			if(led_off_count == 0 ) {
		    io_write_pin(led_off_pin ,1);
				if(ReceiveCommand_flag) {
					ReceiveCommand_flag = false;
				  app_Receive_Wakeup_LedOn_5s();
				}
					
		  } 
		}
		*/
		for (int i = 0; i < LED_MAX; i++) {
    if (led_task[i].timeout > 0) {
        led_task[i].timeout--;
        if (led_task[i].timeout == 0) {
					/*
						if (led_task[i].pin == LED_Blue_3_PIN && g_sysparam_st.ble.ble_pair_flag && g_sysparam_st.ble.ble_pair_time > 0) {
							// 蓝牙配对期间，不执行熄灭，重置超时
							led_task[i].timeout = 1;  // 下次继续检查
								continue;
						}
					*/				
            io_write_pin(led_task[i].pin, 1);  // 熄灭
						LOG_I("i = %d, PIN = %d time_out = %d \n",i,led_task[i].pin,led_task[i].timeout);
            // 灯熄灭后触发特殊逻辑
            if (ReceiveCommand_flag) {
                ReceiveCommand_flag = false;
                app_Receive_Wakeup_LedOn_5s();
            }

            led_task[i].pin = 0;  // 清空任务
        }
    }
}
		 last_charge_status = current_charge_status;
		// LOG_I("cur_s = %d , Lst_s = %d, g_timeout = %d \n",current_charge_status,last_charge_status,charge_green_timeout);
}