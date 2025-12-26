#include "app_led_ctrl.h"
#include "main.h"
#include "x_drive.h"
#include "g.h"
#include <string.h>

/* ========== LED引脚映射表 ========== */
// 注意：PA00等于0，所以不能用0表示"不支持"，改用0xFF
#define LED_PIN_NONE 0xFF  // 表示不支持的引脚

typedef struct {
    uint8_t blue_pin;    // 蓝色引脚（0xFF表示不支持）
    uint8_t green_pin;   // 绿色引脚（0xFF表示不支持）
    uint8_t red_pin;     // 红色引脚（0xFF表示不支持）
} led_pin_map_t;

// LED引脚映射表（根据物理连接定义）
static const led_pin_map_t led_pin_map[LED_ID_MAX] = {
    {LED_Blue_0_PIN,  LED_PIN_NONE,    LED_Red_all_PIN},  // LED 0
    {LED_Blue_1_PIN,  LED_Green_1_PIN, LED_Red_all_PIN},  // LED 1
    {LED_Blue_2_PIN,  LED_PIN_NONE,    LED_Red_all_PIN},  // LED 2
    {LED_Blue_3_PIN,  LED_Green_3_PIN, LED_PIN_NONE}      // LED 3
};

/* ========== LED控制状态 ========== */
static led_ctrl_t led_ctrl[LED_ID_MAX];

/* ========== 内部函数声明 ========== */
static void led_hw_set_pin(uint8_t pin, bool on);
static void led_hw_turn_on(led_id_t led_id, led_color_t color);
static void led_hw_turn_off(led_id_t led_id, led_color_t color);
static void led_hw_turn_off_all_colors(led_id_t led_id);
static uint8_t led_get_pin_by_color(led_id_t led_id, led_color_t color);
static void led_task_finish(led_id_t led_id);

/* ========== 硬件控制函数 ========== */

/**
 * @brief 设置引脚状态（低电平亮，高电平灭）
 */
static void led_hw_set_pin(uint8_t pin, bool on)
{
    if (pin == LED_PIN_NONE) return;  // 不支持的引脚
    io_write_pin(pin, on ? 0 : 1);  // 低电平亮，高电平灭
}

/**
 * @brief 根据颜色获取引脚号
 */
static uint8_t led_get_pin_by_color(led_id_t led_id, led_color_t color)
{
    if (led_id >= LED_ID_MAX) return LED_PIN_NONE;
    
    switch (color) {
        case LED_COLOR_BLUE:
            return led_pin_map[led_id].blue_pin;
        case LED_COLOR_GREEN:
            return led_pin_map[led_id].green_pin;
        case LED_COLOR_RED:
            return led_pin_map[led_id].red_pin;
        default:
            return LED_PIN_NONE;
    }
}

/**
 * @brief 点亮指定LED的指定颜色
 */
static void led_hw_turn_on(led_id_t led_id, led_color_t color)
{
    uint8_t pin = led_get_pin_by_color(led_id, color);
    if (pin != LED_PIN_NONE) {
        led_hw_set_pin(pin, true);
        LOG_I("LED %d ON color=%d pin=%d", led_id, color, pin);
    }
}

/**
 * @brief 熄灭指定LED的指定颜色
 */
static void led_hw_turn_off(led_id_t led_id, led_color_t color)
{
    uint8_t pin = led_get_pin_by_color(led_id, color);
    if (pin != LED_PIN_NONE) {
        led_hw_set_pin(pin, false);
        LOG_I("LED %d OFF color=%d pin=%d", led_id, color, pin);
    }
}

/**
 * @brief 熄灭指定LED的所有颜色
 */
static void led_hw_turn_off_all_colors(led_id_t led_id)
{
    if (led_id >= LED_ID_MAX) return;
    
    // 熄灭所有可能的颜色
    if (led_pin_map[led_id].blue_pin != LED_PIN_NONE) {
        led_hw_set_pin(led_pin_map[led_id].blue_pin, false);
    }
    if (led_pin_map[led_id].green_pin != LED_PIN_NONE) {
        led_hw_set_pin(led_pin_map[led_id].green_pin, false);
    }
    // 注意：红色引脚是公共的，不在这里单独关闭
}

/* ========== LED任务管理 ========== */

/**
 * @brief LED任务完成处理
 */
static void led_task_finish(led_id_t led_id)
{
    if (led_id >= LED_ID_MAX) return;
    
    led_ctrl_t *ctrl = &led_ctrl[led_id];
    
    LOG_I("LED %d task done, mode=%d", led_id, ctrl->mode);
    
    // 熄灭LED
    led_hw_turn_off(led_id, ctrl->color);
    
    // 执行回调（如果有）
    void (*callback)(void) = ctrl->callback;
    
    // 清除状态
    ctrl->mode = LED_MODE_OFF;
    ctrl->color = LED_COLOR_NONE;
    ctrl->timeout_100ms = 0;
    ctrl->priority = 0;
    ctrl->callback = NULL;
    
    // 执行回调（在清除状态之后，避免回调中重入问题）
    if (callback != NULL) {
        callback();
    }
}

/* ========== 核心API实现 ========== */

/**
 * @brief 初始化LED控制模块
 */
void led_ctrl_init(void)
{
    // 清零所有控制结构
    memset(led_ctrl, 0, sizeof(led_ctrl));
    
    // 设置LED编号
    for (int i = 0; i < LED_ID_MAX; i++) {
        led_ctrl[i].led_id = (led_id_t)i;
        led_ctrl[i].mode = LED_MODE_OFF;
        led_ctrl[i].color = LED_COLOR_NONE;
    }
    
    // 确保所有LED硬件处于关闭状态
    led_ctrl_clear_all();
    
    LOG_I("LED ctrl init done");
}

/**
 * @brief 设置LED常亮（带超时）
 */
bool led_ctrl_set_on(led_id_t led_id, led_color_t color, uint16_t timeout_s, 
                     uint8_t priority, void (*callback)(void))
{
    if (led_id >= LED_ID_MAX || color == LED_COLOR_NONE) {
        return false;
    }
    
    // 检查该LED是否支持此颜色
    if (led_get_pin_by_color(led_id, color) == LED_PIN_NONE) {
        LOG_I("LED %d not support color %d", led_id, color);
        return false;
    }
    
    led_ctrl_t *ctrl = &led_ctrl[led_id];
    
    // 优先级检查：如果当前有更高优先级的任务，拒绝
    if (ctrl->mode != LED_MODE_OFF && ctrl->priority > priority) {
        LOG_I("LED %d priority low (cur=%d new=%d)", led_id, ctrl->priority, priority);
        return false;
    }
    
    // 熄灭当前颜色（如果有）
    if (ctrl->color != LED_COLOR_NONE && ctrl->color != color) {
        led_hw_turn_off(led_id, ctrl->color);
    }
    
    // 设置新状态
    ctrl->color = color;
    ctrl->mode = (timeout_s == 0) ? LED_MODE_ON_PERMANENT : LED_MODE_ON;
    ctrl->timeout_100ms = timeout_s * 10;  // 转换为100ms单位
    ctrl->priority = priority;
    ctrl->callback = callback;
    
    // 点亮LED
    led_hw_turn_on(led_id, color);
    
    LOG_I("LED %d set ON: color=%d timeout=%ds pri=%d", led_id, color, timeout_s, priority);
    
    return true;
}

/**
 * @brief 设置LED闪烁
 */
bool led_ctrl_set_flash(led_id_t led_id, led_color_t color, uint16_t interval_ms,
                        uint8_t times, uint16_t timeout_s, uint8_t priority, 
                        void (*callback)(void))
{
    if (led_id >= LED_ID_MAX || color == LED_COLOR_NONE) {
        return false;
    }
    
    // 检查该LED是否支持此颜色
    if (led_get_pin_by_color(led_id, color) == LED_PIN_NONE) {
        LOG_I("LED %d not support color %d", led_id, color);
        return false;
    }
    
    led_ctrl_t *ctrl = &led_ctrl[led_id];
    
    // 优先级检查
    if (ctrl->mode != LED_MODE_OFF && ctrl->priority > priority) {
        LOG_I("LED %d priority low (cur=%d new=%d)", led_id, ctrl->priority, priority);
        return false;
    }
    
    // 熄灭当前颜色（如果有）
    if (ctrl->color != LED_COLOR_NONE && ctrl->color != color) {
        led_hw_turn_off(led_id, ctrl->color);
    }
    
    // 设置新状态
    ctrl->color = color;
    ctrl->mode = LED_MODE_FLASH;
    ctrl->flash_interval_10ms = interval_ms / 10;  // 转换为10ms单位
    ctrl->flash_counter = 0;
    ctrl->flash_times = times * 2;  // 亮灭算一次，所以乘2
    ctrl->timeout_100ms = timeout_s * 10;  // 转换为100ms单位
    ctrl->priority = priority;
    ctrl->callback = callback;
    
    // 先熄灭，闪烁从亮开始
    led_hw_turn_off(led_id, color);
    
    LOG_I("LED %d set FLASH: color=%d interval=%dms times=%d timeout=%ds pri=%d", 
          led_id, color, interval_ms, times, timeout_s, priority);
    
    return true;
}

/**
 * @brief 关闭指定LED
 */
void led_ctrl_set_off(led_id_t led_id)
{
    if (led_id >= LED_ID_MAX) return;
    
    led_ctrl_t *ctrl = &led_ctrl[led_id];
    
    // 熄灭LED
    if (ctrl->color != LED_COLOR_NONE) {
        led_hw_turn_off(led_id, ctrl->color);
    }
    
    // 清除状态
    ctrl->mode = LED_MODE_OFF;
    ctrl->color = LED_COLOR_NONE;
    ctrl->timeout_100ms = 0;
    ctrl->priority = 0;
    ctrl->callback = NULL;
    
    LOG_I("LED %d set OFF", led_id);
}

/**
 * @brief 强制关闭指定LED（忽略优先级）
 */
void led_ctrl_force_off(led_id_t led_id)
{
    led_ctrl_set_off(led_id);
}

/**
 * @brief 关闭所有LED
 */
void led_ctrl_clear_all(void)
{
    // 关闭所有LED的所有颜色
    for (int i = 0; i < LED_ID_MAX; i++) {
        led_hw_turn_off_all_colors((led_id_t)i);
        led_ctrl[i].mode = LED_MODE_OFF;
        led_ctrl[i].color = LED_COLOR_NONE;
        led_ctrl[i].priority = 0;
        led_ctrl[i].callback = NULL;
    }
    
    // 特别处理红色公共引脚
    led_hw_set_pin(LED_Red_all_PIN, false);
    
    LOG_I("All LED OFF");
}

/**
 * @brief 检查LED是否激活
 */
bool led_ctrl_is_active(led_id_t led_id)
{
    if (led_id >= LED_ID_MAX) return false;
    return (led_ctrl[led_id].mode != LED_MODE_OFF);
}

/**
 * @brief LED控制10ms定时处理
 */
void led_ctrl_10ms_handler(void)
{
    static uint8_t timer_100ms = 0;
    timer_100ms++;
    
    bool is_100ms_tick = (timer_100ms >= 10);
    if (is_100ms_tick) {
        timer_100ms = 0;
    }
    
    // 遍历所有LED
    for (int i = 0; i < LED_ID_MAX; i++) {
        led_ctrl_t *ctrl = &led_ctrl[i];
        
        if (ctrl->mode == LED_MODE_OFF) {
            continue;  // 跳过关闭的LED
        }
        
        // 处理闪烁模式
        if (ctrl->mode == LED_MODE_FLASH) {
            ctrl->flash_counter++;
            
            if (ctrl->flash_counter >= ctrl->flash_interval_10ms) {
                ctrl->flash_counter = 0;
                
                // 切换LED状态
                uint8_t pin = led_get_pin_by_color(ctrl->led_id, ctrl->color);
                if (pin != LED_PIN_NONE) {
                    uint8_t current_state = io_read_pin(pin);
                    led_hw_set_pin(pin, current_state == 1);  // 反转状态
                }
                
                // 减少剩余闪烁次数
                if (ctrl->flash_times > 0) {
                    ctrl->flash_times--;
                    if (ctrl->flash_times == 0) {
                        // 闪烁完成
                        led_task_finish(ctrl->led_id);
                        continue;
                    }
                }
            }
        }
        
        // 处理超时（每100ms检查一次）
        if (is_100ms_tick && ctrl->timeout_100ms > 0) {
            ctrl->timeout_100ms--;
            if (ctrl->timeout_100ms == 0) {
                // 超时完成
                led_task_finish(ctrl->led_id);
            }
        }
    }
}

/* ========== 业务封装API实现 ========== */

/**
 * @brief 打鼾档位指示
 */
void led_snore_level_set(uint8_t level)
{
    if (level == 0) {
        // 关闭所有打鼾档位LED
        led_ctrl_set_off(LED_ID_0);
        led_ctrl_set_off(LED_ID_1);
        led_ctrl_set_off(LED_ID_2);
        LOG_I("Snore level OFF");
    } else if (level >= 1 && level <= 3) {
        led_id_t led_id = (led_id_t)(level - 1);  // level 1/2/3 对应 LED 0/1/2
        
        // 先关闭其他档位LED
        for (int i = 0; i < 3; i++) {
            if (i != led_id) {
                led_ctrl_set_off((led_id_t)i);
            }
        }
        
        // 点亮对应档位LED（蓝色，10秒）
        led_ctrl_set_on(led_id, LED_COLOR_BLUE, 10, LED_PRIORITY_NORMAL, NULL);
        LOG_I("Snore level=%d LED=%d", level, led_id);
    }
}

/**
 * @brief 打鼾功能关闭指示
 */
void led_snore_disabled_flash(void)
{
    // LED 0/1/2 同时闪烁2次（500ms间隔）
    led_ctrl_set_flash(LED_ID_0, LED_COLOR_BLUE, 200, 2, 3, LED_PRIORITY_CRITICAL, NULL);
    led_ctrl_set_flash(LED_ID_1, LED_COLOR_BLUE, 200, 2, 3, LED_PRIORITY_CRITICAL, NULL);
    led_ctrl_set_flash(LED_ID_2, LED_COLOR_BLUE, 200, 2, 3, LED_PRIORITY_CRITICAL, NULL);
    LOG_I("Snore disabled flash");
}

/**
 * @brief 语音唤醒后续动作：蓝灯8秒
 */
static void led_voice_wakeup_callback(void)
{
    // 绿灯3秒结束后，点亮蓝灯8秒
    led_ctrl_set_on(LED_ID_3, LED_COLOR_BLUE, 8, LED_PRIORITY_NORMAL, NULL);
    LOG_I("Voice cmd confirm: blue 8s");
}

/**
 * @brief 离线语音唤醒指示
 */
void led_voice_wakeup(void)
{
    // LED 3 蓝灯亮8秒（检查是否在配对中）
    if (led_ctrl[LED_ID_3].priority > LED_PRIORITY_NORMAL) {
        LOG_I("LED3 busy, skip voice wakeup");
        return;
    }
    
    led_ctrl_set_on(LED_ID_3, LED_COLOR_BLUE, 8, LED_PRIORITY_NORMAL, NULL);
    LOG_I("Voice wakeup: blue 8s");
}

/**
 * @brief 离线语音命令确认指示
 */
void led_voice_command_confirm(void)
{
    // LED 3 绿灯亮3秒，然后蓝灯亮8秒
    if (led_ctrl[LED_ID_3].priority > LED_PRIORITY_NORMAL) {
        LOG_I("LED3 busy, skip voice cmd");
        return;
    }
    
    led_ctrl_set_on(LED_ID_3, LED_COLOR_GREEN, 3, LED_PRIORITY_NORMAL, led_voice_wakeup_callback);
    LOG_I("Voice cmd: green 3s");
}

/**
 * @brief 离线语音关闭指示
 */
void led_voice_close_flash(void)
{
    // LED 3 闪烁2次
    led_ctrl_set_flash(LED_ID_3, LED_COLOR_BLUE, 200, 2, 3, LED_PRIORITY_NORMAL, NULL);
    LOG_I("Voice close: flash 2x");
}

/**
 * @brief 无线充正常指示
 */
void led_charge_normal(void)
{
    // LED 1 绿灯亮15秒
    led_ctrl_set_on(LED_ID_1, LED_COLOR_GREEN, 15, LED_PRIORITY_NORMAL, NULL);
    LOG_I("Charge normal: green 15s");
}

/**
 * @brief 无线充异常指示
 */
void led_charge_error(void)
{
    // LED 0/1/2 红灯亮30秒（公共引脚，只需点亮一次）
    // 使用LED 0作为代表，设置红色
    led_ctrl_set_on(LED_ID_0, LED_COLOR_RED, 30, LED_PRIORITY_CRITICAL, NULL);
    LOG_I("Charge error: red 30s");
}

/**
 * @brief LED测试函数 - 依次点亮所有蓝灯
 */
void led_test_all_blue(void)
{
    LOG_I("LED test: Blue LED 0");
    led_ctrl_set_on(LED_ID_0, LED_COLOR_BLUE, 3, LED_PRIORITY_NORMAL, NULL);
}

void led_test_all_blue_step2(void)
{
    LOG_I("LED test: Blue LED 1");
    led_ctrl_set_on(LED_ID_1, LED_COLOR_BLUE, 3, LED_PRIORITY_NORMAL, NULL);
}

void led_test_all_blue_step3(void)
{
    LOG_I("LED test: Blue LED 2");
    led_ctrl_set_on(LED_ID_2, LED_COLOR_BLUE, 3, LED_PRIORITY_NORMAL, NULL);
}

/**
 * @brief 蓝牙配对中指示
 */
void led_bt_pairing(bool connected)
{
    if (connected) {
        // 配对成功：LED 3 蓝灯常亮30秒
        led_ctrl_set_on(LED_ID_3, LED_COLOR_BLUE, 30, LED_PRIORITY_HIGH, NULL);
        LOG_I("BT pair success: blue 30s");
    } else {
        // 配对中：LED 3 蓝灯闪烁30秒（500ms间隔）
        led_ctrl_set_flash(LED_ID_3, LED_COLOR_BLUE, 500, 0, 30, LED_PRIORITY_HIGH, NULL);
        LOG_I("BT pairing: flash 30s");
    }
}

/**
 * @brief 停止蓝牙配对指示
 */
void led_bt_pairing_stop(void)
{
    // 强制关闭LED 3
    led_ctrl_force_off(LED_ID_3);
    LOG_I("BT pair stop");
}

