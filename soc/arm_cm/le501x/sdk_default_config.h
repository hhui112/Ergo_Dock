#ifndef SDK_DEFAULT_CONFIG_H_
#define SDK_DEFAULT_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SDK_USER_BUILTIN_TIMER_NUM_MAX
#define SDK_USER_BUILTIN_TIMER_NUM_MAX 7
#endif

#ifndef SDK_DCDC_BYPASS
#define SDK_DCDC_BYPASS 1
#endif

#ifndef SDK_DEEP_SLEEP_ENABLE
#define SDK_DEEP_SLEEP_ENABLE 1
#endif

#ifndef SDK_BLE_STORAGE_PEER_MAX
#define SDK_BLE_STORAGE_PEER_MAX 3
#endif

#ifndef SDK_USER_TINYFS_NODE_MAX
#define SDK_USER_TINYFS_NODE_MAX 5
#endif

#ifndef SDK_USER_TASK_NUM
#define SDK_USER_TASK_NUM 0
#endif

#ifndef SDK_MAX_CONN_NUM
#define SDK_MAX_CONN_NUM 1
#endif

#ifndef SDK_MAX_PROFILE_NUM
#define SDK_MAX_PROFILE_NUM 1
#endif

#ifndef SDK_MAX_RAL_NUM
#define SDK_MAX_RAL_NUM 1
#endif

#ifndef SDK_HCLK_MHZ
#define SDK_HCLK_MHZ (64)
#endif

#ifndef SDK_PCLK_DIV
#define SDK_PCLK_DIV 1
#endif

#ifndef SDK_LSI_USED
#define SDK_LSI_USED 1
#endif

#ifndef LSI_RECOUNT_PERIOD_MS
#define LSI_RECOUNT_PERIOD_MS (60*1000)
#endif

#ifndef FPGA
#define FPGA 0
#endif

#ifndef DEBUG_MODE
#define DEBUG_MODE 1
#endif

#ifndef CHIP_TEMP_SENSOR
#define CHIP_TEMP_SENSOR 0
#endif

#ifndef LOG_UART_TXD
#define LOG_UART_TXD (PB00)
#endif
#ifndef LOG_UART_RXD
#define LOG_UART_RXD (PB01)
#endif
#ifndef LOG_UART_BAUDRATE
#define LOG_UART_BAUDRATE UART_BAUDRATE_921600
#endif
#ifndef LOG_UART_WORDLENGTH
#define LOG_UART_WORDLENGTH UART_BYTESIZE8
#endif
#ifndef LOG_UART_STOPBITS
#define LOG_UART_STOPBITS UART_STOPBITS1
#endif
#ifndef LOG_UART_PARITY
#define LOG_UART_PARITY UART_NOPARITY
#endif
#ifndef LOG_UART_MSBEN
#define LOG_UART_MSBEN 0
#endif

#define BLE_MAC_TIMER 2
#define OS_TICK_SOURCE BLE_MAC_TIMER

#define SDK_BUILTIN_TIMER_MAX (SDK_USER_BUILTIN_TIMER_NUM_MAX+SDK_LSI_USED)
#define SDK_PCLK_MHZ (SDK_HCLK_MHZ/SDK_PCLK_DIV)
#define SDK_MAX_ACT_NUM    (SDK_MAX_CONN_NUM + 2)
#define SDK_BUILTIN_TASK_NUM 1
#define SDK_MAX_USER_TASK_NUM (SDK_BUILTIN_TASK_NUM + SDK_USER_TASK_NUM)

#ifdef __cplusplus
}
#endif

#endif

