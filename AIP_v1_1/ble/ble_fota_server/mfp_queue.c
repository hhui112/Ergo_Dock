#include "mfp_queue.h"
#include "x_uart.h"
#include <string.h>

// 队列实现（循环队列FIFO）
static mfp_tx_request_t tx_queue[MFP_TX_QUEUE_MAX_ITEMS];
static uint8_t head = 0;
static uint8_t tail = 0;
static uint8_t count = 0;

void mfp_tx_queue_init(void)
{
    head = 0;
    tail = 0;
    count = 0;
}

bool mfp_send_request(const uint8_t *data, uint8_t len, uint8_t repeat)	// 暂不加锁
{
    if (count >= MFP_TX_QUEUE_MAX_ITEMS || len > MFP_TX_DATA_MAX_LEN)
        return false;

    memcpy(tx_queue[tail].data, data, len);
    tx_queue[tail].len = len;
    tx_queue[tail].repeat = repeat;

    tail = (tail + 1) % MFP_TX_QUEUE_MAX_ITEMS;
    count++;
    return true;
}

void mfp_tx_queue_clear(void)
{
    head = 0;
    tail = 0;
    count = 0;
}

bool mfp_tx_queue_is_empty(void)
{
    return (count == 0);
}

void mfp_tx_task(void)
{
    if (count == 0)
        return;

    mfp_tx_request_t *req = &tx_queue[head];

    if (req->repeat > 0)
    {
				io_write_pin(UART_CTR,0);	
				HAL_UART_Transmit(&UART_Server1_Config, req->data, req->len, 1000);  // 发数据
				io_write_pin(UART_CTR,1);	
			
        req->repeat--;

        if (req->repeat == 0)
        {
            // 当前请求已完成，出队
            head = (head + 1) % MFP_TX_QUEUE_MAX_ITEMS;
            count--;
        }
    }
}


