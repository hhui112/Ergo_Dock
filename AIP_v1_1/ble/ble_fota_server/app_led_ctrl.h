#ifndef APP_LED_CTRL_H
#define APP_LED_CTRL_H

#include <stdint.h>
#include <stdbool.h>

/* ========== LED物理定义 ========== */
// 注意：这些定义应该在main.h中，这里仅作为引用说明
// LED 0: 蓝色(PA02) + 红色(PB13公共)
// LED 1: 蓝色(PA00) + 绿色(PA01) + 红色(PB13公共)
// LED 2: 蓝色(PB15) + 红色(PB13公共)
// LED 3: 蓝色(PB09) + 绿色(PB10)

/* ========== LED编号定义 ========== */
typedef enum {
    LED_ID_0 = 0,    // 打鼾档位Low
    LED_ID_1 = 1,    // 打鼾档位Med / 无线充指示
    LED_ID_2 = 2,    // 打鼾档位High
    LED_ID_3 = 3,    // 语音唤醒 / 蓝牙配对
    LED_ID_MAX
} led_id_t;

/* ========== LED颜色定义 ========== */
typedef enum {
    LED_COLOR_NONE  = 0,
    LED_COLOR_BLUE  = 1,
    LED_COLOR_GREEN = 2,
    LED_COLOR_RED   = 3
} led_color_t;

/* ========== LED模式定义 ========== */
typedef enum {
    LED_MODE_OFF,           // 关闭
    LED_MODE_ON,            // 常亮（带超时自动关闭）
    LED_MODE_ON_PERMANENT,  // 常亮（永久，不超时）
    LED_MODE_FLASH          // 闪烁
} led_mode_t;

/* ========== LED优先级定义 ========== */
#define LED_PRIORITY_NORMAL      0   // 普通（档位指示、语音唤醒）
#define LED_PRIORITY_HIGH        5   // 高（配对）
#define LED_PRIORITY_CRITICAL    10  // 关键（充电异常、功能关闭闪烁）

/* ========== LED控制结构 ========== */
typedef struct {
    led_id_t led_id;            // LED编号
    led_color_t color;          // 当前颜色
    led_mode_t mode;            // 当前模式
    uint16_t timeout_100ms;     // 超时时间（100ms单位，0=永久）
    uint16_t flash_interval_10ms; // 闪烁间隔（10ms单位）
    uint16_t flash_counter;     // 闪烁计数器（内部使用）
    uint8_t flash_times;        // 闪烁次数（亮灭算一次，0=一直闪）
    uint8_t priority;           // 当前任务优先级
    void (*callback)(void);     // 完成回调函数（可选）
} led_ctrl_t;

/* ========== 核心API ========== */

/**
 * @brief 初始化LED控制模块
 */
void led_ctrl_init(void);

/**
 * @brief LED控制10ms定时处理（需要在10ms定时器中调用）
 */
void led_ctrl_10ms_handler(void);

/**
 * @brief 设置LED常亮（带超时）
 * @param led_id LED编号
 * @param color LED颜色
 * @param timeout_s 超时时间（秒），0表示永久亮
 * @param priority 优先级
 * @param callback 完成回调函数（可选，传NULL表示无回调）
 * @return true=成功, false=失败（优先级不足）
 */
bool led_ctrl_set_on(led_id_t led_id, led_color_t color, uint16_t timeout_s, 
                     uint8_t priority, void (*callback)(void));

/**
 * @brief 设置LED闪烁
 * @param led_id LED编号
 * @param color LED颜色
 * @param interval_ms 闪烁间隔（毫秒）
 * @param times 闪烁次数（亮灭算一次），0表示一直闪烁
 * @param timeout_s 总超时时间（秒），0表示无限制（根据times结束）
 * @param priority 优先级
 * @param callback 完成回调函数（可选）
 * @return true=成功, false=失败（优先级不足）
 */
bool led_ctrl_set_flash(led_id_t led_id, led_color_t color, uint16_t interval_ms,
                        uint8_t times, uint16_t timeout_s, uint8_t priority, 
                        void (*callback)(void));

/**
 * @brief 关闭指定LED
 * @param led_id LED编号
 */
void led_ctrl_set_off(led_id_t led_id);

/**
 * @brief 强制关闭指定LED（忽略优先级）
 * @param led_id LED编号
 */
void led_ctrl_force_off(led_id_t led_id);

/**
 * @brief 关闭所有LED
 */
void led_ctrl_clear_all(void);

/**
 * @brief 检查LED是否激活
 * @param led_id LED编号
 * @return true=激活中, false=关闭
 */
bool led_ctrl_is_active(led_id_t led_id);

/* ========== 业务封装API ========== */

/**
 * @brief 打鼾档位指示
 * @param level 档位：0=关闭, 1=Low, 2=Med, 3=High
 * @note 蓝色LED亮10秒后自动熄灭
 */
void led_snore_level_set(uint8_t level);

/**
 * @brief 打鼾功能关闭指示
 * @note LED 0/1/2 闪烁2次
 */
void led_snore_disabled_flash(void);

/**
 * @brief 离线语音唤醒指示
 * @note LED 3 蓝灯亮8秒后熄灭
 */
void led_voice_wakeup(void);

/**
 * @brief 离线语音命令确认指示
 * @note LED 3 绿灯亮3秒，然后蓝灯亮8秒
 */
void led_voice_command_confirm(void);

/**
 * @brief 离线语音关闭指示
 * @note LED 3 闪烁2次
 */
void led_voice_close_flash(void);

/**
 * @brief 无线充正常指示
 * @note LED 1 绿灯亮15秒后熄灭
 */
void led_charge_normal(void);

/**
 * @brief 无线充异常指示
 * @note LED 0/1/2 红灯亮30秒后熄灭（公共引脚）
 */
void led_charge_error(void);

/**
 * @brief LED测试函数 - 依次点亮所有蓝灯
 */
void led_test_all_blue(void);
void led_test_all_blue_step2(void);
void led_test_all_blue_step3(void);

/**
 * @brief 蓝牙配对中指示
 * @param connected 是否已连接（false=闪烁，true=常亮）
 * @note LED 3 蓝灯闪烁或常亮，总共30秒超时
 */
void led_bt_pairing(bool connected);

/**
 * @brief 停止蓝牙配对指示
 */
void led_bt_pairing_stop(void);

#endif // APP_LED_CTRL_H

