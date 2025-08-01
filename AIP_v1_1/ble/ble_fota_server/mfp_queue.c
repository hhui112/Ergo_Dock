#include "mfp_queue.h"
#include "x_uart.h"

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
				HAL_UART_Transmit(&UART_Server1_Config, req->data, req->len, 10);  // 发数据
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


void prepare_mfp_NORMAL_KET(uint32_t keys,uint8_t repeat) 
{
		static uint8_t uartTxbuff[20];
		memset(uartTxbuff,0,sizeof(uartTxbuff));
		int i = 0;
		uartTxbuff[i++] = 0x05;
		uartTxbuff[i++] = 0x01;
		uartTxbuff[i++] = (keys) & 0xFF;        
		uartTxbuff[i++] = (keys >> 8) & 0xFF;
		uartTxbuff[i++] = (keys >> 16) & 0xFF;
		uartTxbuff[i++] = (keys >> 24) & 0xFF;  
		uartTxbuff[i++] = 0x00;
		uartTxbuff[i++] = syncCalcCheckSum(uartTxbuff,i);	
		// LOG_I("SUM = %d\n", uartTxbuff[i-1]);
	
		if (!mfp_send_request(uartTxbuff, i, repeat))
			
    {
        LOG_E("MFP QUERE ERR! \r\n");
    }
}	


void prepare_mfp_SOFT_START(uint32_t keys,uint8_t pwm, uint8_t tmr,uint8_t repeat) 
{
		static uint8_t uartTxbuff[20];
		memset(uartTxbuff,0,sizeof(uartTxbuff));
		int i = 0;
		uartTxbuff[i++] = 0x07;
		uartTxbuff[i++] = 0x01;
		uartTxbuff[i++] = (keys) & 0xFF;        
		uartTxbuff[i++] = (keys >> 8) & 0xFF;
		uartTxbuff[i++] = (keys >> 16) & 0xFF;
		uartTxbuff[i++] = (keys >> 24) & 0xFF;  
		uartTxbuff[i++] = 0x00;
		uartTxbuff[i++] = pwm;
		uartTxbuff[i++] = tmr;
		uartTxbuff[i++] = syncCalcCheckSum(uartTxbuff,i);	
		// LOG_I("SUM = %d\n", uartTxbuff[i-1]);
	
		if (!mfp_send_request(uartTxbuff, i, repeat))
    {
        LOG_E("MFP QUERE ERR! \r\n");
    }
}


