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
 * ��ʼ������
 */
void mfp_tx_queue_init(void);

/**
 * ����������һ����������
 * @param data Ҫ���͵�����ָ��
 * @param len  ���ݳ���
 * @param repeat �ظ�����
 * @return true �ɹ���false ������
 */
bool mfp_send_request(const uint8_t *data, uint8_t len, uint8_t repeat);

/**
 * ��ն���
 */
void mfp_tx_queue_clear(void);

/**
 * �����Ƿ�Ϊ��
 * @return true �գ�false ��Ϊ��
 */
bool mfp_tx_queue_is_empty(void);

/**
 * ��MFP����CRC������ɺ���ã�������зǿջ�ִ��һ�η���
 */
void mfp_tx_task(void);


void prepare_mfp_NORMAL_KET(uint32_t keys,uint8_t repeat);		/* ��ͨ��ֵ��� */
void prepare_mfp_SOFT_START(uint32_t keys,uint8_t pwm, uint8_t tmr,uint8_t repeat); /* ��������ֵ���*/

	
#endif
