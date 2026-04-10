#include "x_control.h"
#include "g.h"
#include "ble_base_set.h"
#include "app_led_ctrl.h"  // 新的LED控制模块
#include "x_uart.h"

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

	/*
	 * KEY1/KEY2 拨码开关消抖：连续 3 次 10ms 采样一致才认为状态切换（30ms）。
	 * 目的：避免拨动过渡/抖动瞬间误判为 OFF（1&1）导致运行中误关灯/误下发。
	 */
	static uint8_t s_raw_last = 0xFFU;
	static uint8_t s_raw_cnt = 0U;
	static uint8_t s_stable = 0xFFU;

	uint8_t raw = (uint8_t)(((key1 & 1U) << 1) | (key2 & 1U)); /* bit1=key1, bit0=key2 */

	if (raw == s_raw_last) {
		if (s_raw_cnt < 3U)
			s_raw_cnt++;
	} else {
		s_raw_last = raw;
		s_raw_cnt = 1U;
	}

	/* 未稳定满 3 次则不更新业务状态 */
	if (s_raw_cnt < 3U)
		return;

	/* 稳定值未变化则不重复处理 */
	if (raw == s_stable)
		return;

	s_stable = raw;

	bool sw_off = (raw == 0x03U); /* key1=1,key2=1 */

	/* 需求：拨到关闭档时语音芯片不再上报 0x82 结束帧，因此在“开->关”时手动关灯。 */
	bool prev_enable = g_offline_voice.key_enable;

	if (sw_off) {
		g_offline_voice.key_enable = false;
		g_offline_voice.wake_word = 0U;

		if (prev_enable) {
			g_offline_voice.enabled = false;
			led_ctrl_force_off(LED_ID_3);
		}
	} else {
		g_offline_voice.key_enable = true;
		/* 档位映射：Hello_Ergo=0x21，Hello_Bed=0x22（此处保持现有映射不再改动）。 */
		if (key1 == 0U)
			g_offline_voice.wake_word = Hello_Bed;
		else if (key2 == 0U)
			g_offline_voice.wake_word = Hello_Ergo;
	}
		if(g_sysparam_st.ubb == 1){
			g_offline_voice.ubb_enable = true;		// light_on
		}else{
			g_offline_voice.ubb_enable = false;		// litht off
		}
	/* 拨键稳定切换后，经 UART2 下发 CI1302 配置（与上电 force 互补，避免重复） */
	ci1302_uart2_voice_config_sync(false);
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
					g_sysparam_st.ble.ble_pair_time = 3000;  // 30秒超时（3000 * 10ms）
					start_adv();
					led_bt_pairing(false);  // 新的LED控制：蓝牙配对中（闪烁30秒）
			}else{
					g_sysparam_st.ble.ble_pair_flag = 0;
					g_sysparam_st.ble.ble_pair_time = 0;
					stop_adv();
					led_bt_pairing_stop();  // 新的LED控制：停止配对指示
			}
		}
		if(key_repeat == 500 && key == app_key_num3) {
		  // app_led_flash_set();  // 旧的LED控制
			led_snore_disabled_flash();  // 新的LED控制：打鼾功能关闭闪烁
			keys_ignore = true;
			g_sysparam_st.AntiSnore_intensity = 0;
			//关闭
		}
		}else if(last_key != 0 && key == 0) {
			LOG_I("key %x released",last_key);
		if(last_key == app_key_num3 && !keys_ignore) {
			chargeAbnormal_flag = false;
			g_sysparam_st.AntiSnore_intensity = (g_sysparam_st.AntiSnore_intensity < 3)? g_sysparam_st.AntiSnore_intensity + 1:1;
			// uint8_t led_num = g_sysparam_st.AntiSnore_intensity - 1;
			// app_led_set(led_num,blue);  // 旧的LED控制
			// app_key_blue_10s(led_num);  // 旧的LED控制
			led_snore_level_set(g_sysparam_st.AntiSnore_intensity);  // 新的LED控制：设置打鼾档位
		}
		}else {
		  key_repeat = 0;
			keys_ignore = false;
		}
	
	  last_key = key;
		/* 离线语音拨键在 timer_event ls_10ms_timer_cb 最前采样，避免被本函数内耗时逻辑拖慢 */
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
	
	// 充电状态处理（充电优先级高，会自动覆盖打鼾档位LED）
	if(current_charge_status == 0 && !chargeAbnormal_flag)  // 充电异常（CHARGE_ABNORMAL_PIN高电平）
	{
		// 如果之前在正常充电，先关闭绿灯
		if (last_charge_status == 1) {
			led_charge_stop();
		}
		
		chargeAbnormal_flag = true;
		led_charge_error();  // 充电异常指示（红灯30秒，会清除打鼾蓝灯）
	}
	else if(current_charge_status == 1)  // 正常充电（CHARGE_NORMAL_PIN高电平）
	{
		if (last_charge_status != 1)
		{
			// 如果之前是充电异常，先关闭红灯
			if (last_charge_status == 0) {
				io_write_pin(LED_Red_all_PIN, 1);
				led_ctrl_force_off(LED_ID_0);  // 清除LED 0的红色任务
			}
			
			led_charge_normal();  // 充电正常指示（绿灯15秒）
		}
		chargeAbnormal_flag = false;
	}
	else if(current_charge_status == 0xff)  // 没充电（两个引脚都是低电平）
	{
		// 如果之前在正常充电，现在停止了，需要立即熄灭绿灯
		if (last_charge_status == 1) {
			led_charge_stop();  // 停止充电指示（熄灭绿灯）
		}
		
		// 熄灭充电相关的LED（只在红色异常灯时才需要手动关闭红灯）
		if (chargeAbnormal_flag) {
			io_write_pin(LED_Red_all_PIN, 1);
		}
		chargeAbnormal_flag = false;
	}
	
	// 更新充电状态
	last_charge_status = current_charge_status;
		
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
	// 注意：last_charge_status 的更新已经移到充电判断之后（第199行）
}