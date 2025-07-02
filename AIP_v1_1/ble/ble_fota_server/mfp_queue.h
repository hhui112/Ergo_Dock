#ifndef __MFP_QUEUE_H__
#define __MFP_QUEUE_H__

#include <stdint.h>
#include <stdbool.h>

#define MFP_TX_QUEUE_MAX_ITEMS 8
#define MFP_TX_DATA_MAX_LEN    32

typedef struct {
    uint8_t data[MFP_TX_DATA_MAX_LEN];
    uint8_t len;
    uint8_t repeat;
} mfp_tx_request_t;

/**
 * 初始化队列
 */
void mfp_tx_queue_init(void);

/**
 * 队列中新增一个发送请求
 * @param data 要发送的数据指针
 * @param len  数据长度
 * @param repeat 重复次数
 * @return true 成功；false 队列满
 */
bool mfp_send_request(const uint8_t *data, uint8_t len, uint8_t repeat);

/**
 * 清空队列
 */
void mfp_tx_queue_clear(void);

/**
 * 队列是否为空
 * @return true 空；false 不为空
 */
bool mfp_tx_queue_is_empty(void);

/**
 * 由MFP接收CRC检验完成后调用；如果队列非空会执行一次发送
 */
void mfp_tx_task(void);


void prepare_mfp_NORMAL_KET(uint32_t keys,uint8_t repeat);		/* 普通键值入队 */
void prepare_mfp_SOFT_START(uint32_t keys,uint8_t pwm, uint8_t tmr,uint8_t repeat); /* 缓启动键值入队*/

	
#endif
