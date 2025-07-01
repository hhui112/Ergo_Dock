#include "x_uart.h"
#include "mfp_queue.h"
#include "g.h"

uint8_t s_tx_busy;
uint8_t rx1_t_data,rx2_t_data,rx3_t_data;
static uint8_t data_array[1];
// uart_data_st uart1_data_pack;
mfp_data_st uart1_data_pack;
uart_data_st uart2_data_pack;
uart_data_st uart3_data_pack;

UART_HandleTypeDef UART_Server3_Config; 
UART_HandleTypeDef UART_Server2_Config; 
UART_HandleTypeDef UART_Server1_Config;

void x_uart1_dateReceiveHandle(void);
void x_uart2_dateReceiveHandle(void);
void x_uart3_dateReceiveHandle(void);
void x_uart_Internalparameterprint(void);

void x_uart_10ms(void)
{
	if(uart1_data_pack.timeout>0)
		uart1_data_pack.timeout--;
	
	if(uart2_data_pack.timeout>0)
		uart2_data_pack.timeout--;
	
	if(uart3_data_pack.timeout>0)
		uart3_data_pack.timeout--;
	
	x_uart1_dateReceiveHandle();
	x_uart2_dateReceiveHandle();
	x_uart3_dateReceiveHandle();
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)//串口接收回调函数
{

	if(huart == &UART_Server1_Config)
	{
		HAL_UART_Receive_IT(&UART_Server1_Config, &rx1_t_data, 1);//调用此函数，串口接收使能，每次接收1byte，存放到uart_server_rx_byte

		if(uart1_data_pack.rxindex <RXBUFF_LEN)
		{
			uart1_data_pack.rx.rawData[uart1_data_pack.rxindex++] = rx1_t_data;
			uart1_data_pack.timeout = COM_TIMEOUT;
		}
	}

	if(huart == &UART_Server2_Config)
	{
		HAL_UART_Receive_IT(&UART_Server2_Config, &rx2_t_data, 1);//调用此函数，串口接收使能，每次接收1byte，存放到uart_server_rx_byte
		 LOG_I("UART2 Rx interrupt entered rx2_t_data = %x",rx2_t_data);
		if(uart2_data_pack.rxindex <RXBUFF_LEN)
		{
			uart2_data_pack.rxbuf[uart2_data_pack.rxindex++] = rx2_t_data;
			uart2_data_pack.timeout = COM_TIMEOUT;
		}
	}

	if(huart == &UART_Server3_Config)
	{
		// LOG_I("UART3 Rx interrupt entered");
		HAL_UART_Receive_IT(&UART_Server3_Config, &rx3_t_data, 1);//调用此函数，串口接收使能，每次接收1byte，存放到uart_server_rx_byte
		if(uart3_data_pack.rxindex <RXBUFF_LEN)
		{
			uart3_data_pack.rxbuf[uart3_data_pack.rxindex++] = rx3_t_data;
			uart3_data_pack.timeout = COM_TIMEOUT;
		}	
	}
	
}

void prepare_mfp_NORMAL_KET(uint32_t keys) 
{
		static uint8_t uartTxbuff[50];
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
	
		if (!mfp_send_request(uartTxbuff, i, 1))
    {
        LOG_E("MFP QUERE ERR! \r\n");
    }
}	

void x_uart1_cmdHandle(void)
{
		prepare_mfp_NORMAL_KET(KEY_M1_OUT); 
}


unsigned char rxCalcCheckSum()
{
	uint16_t i;
	uint8_t sum;
	sum = 0xff;
	for (i = 0; i < ((uint16_t)2 + (uint16_t)uart1_data_pack.rx.Syncdata.length); i++)
	{
		sum -= uart1_data_pack.rx.rawData[i];
	}
	return sum;
}


void x_uart1_dateReceiveHandle(void)
{
		// for (int i=0; i<uart1_data_pack.rxindex; i++) {LOG_I("UART1 RxData[%d]=0x%02X", i, uart1_data_pack.rx.rawData[i]);}
    if (uart1_data_pack.rxindex != 0 && uart1_data_pack.timeout == 0)
    {
        uart1_data_pack.rxindex = 0;
        memset(uart1_data_pack.rx.rawData, 0, sizeof(uart1_data_pack.rx.rawData));
    }
    if ((uart1_data_pack.rxindex > 0) && (uart1_data_pack.rxindex >= (uart1_data_pack.rx.Syncdata.length + 3)))         // +4
    {

        if (uart1_data_pack.rx.Syncdata.data[uart1_data_pack.rx.Syncdata.length] == rxCalcCheckSum())
        {
            // LOG_I("Crc ok!\r\n");
						// x_uart1_cmdHandle();
						mfp_tx_task(); 				// 队列发送
        }

        uart1_data_pack.rxindex = 0;
        memset(uart1_data_pack.rx.rawData, 0, sizeof(uart1_data_pack.rx.rawData));
    }
}


void x_uart2_dateReceiveHandle(void)
{
    if (uart2_data_pack.rxindex != 0 && uart2_data_pack.timeout == 0)
    {
        uart2_data_pack.rxindex = 0;
        memset(uart2_data_pack.rxbuf, 0, sizeof(uart2_data_pack.rxbuf));
    }
    // 判断是否收满固定8字节
    if (uart2_data_pack.rxindex >= 8)
    {
        // 帧头是0xA5，帧尾是0xFB
        if (uart2_data_pack.rxbuf[0] == 0xA5 && uart2_data_pack.rxbuf[7] == 0xFB)
        {
            LOG_I("uart2 valid frame received\n");
            // x_uart_Internalparameterprint();
        }
        else
        {
            LOG_W("uart2 invalid frame err! \n");
        }

        uart2_data_pack.rxindex = 0;
        memset(uart2_data_pack.rxbuf, 0, sizeof(uart2_data_pack.rxbuf));
    }
}



void x_uart3_dateReceiveHandle(void)
{
	uint16_t crc1_t = 0 , crc2_t = 0;
	
	if(uart3_data_pack.rxindex !=0 && uart3_data_pack.timeout==0)
	{
		uart3_data_pack.rxindex = 0;
		memset(uart3_data_pack.rxbuf,0,sizeof(uart3_data_pack.rxbuf));
	}
	
	if((uart3_data_pack.rxindex > 0) && (uart3_data_pack.rxindex >= (uart3_data_pack.rxbuf[1] + 4)))
	{
		crc1_t = ((uart3_data_pack.rxbuf[ uart3_data_pack.rxbuf[1]+2]<<8)|(uart3_data_pack.rxbuf[ uart3_data_pack.rxbuf[1]+3]));
		crc2_t = Modbus_Crc_Compute(uart3_data_pack.rxbuf,uart3_data_pack.rxbuf[1] + 2);
		
		if(crc2_t == crc1_t)
		{				
			g_sysparam_st.ci1302.version = uart3_data_pack.rxbuf[4]<<8|uart3_data_pack.rxbuf[5];
			g_sysparam_st.ci1302.snorevolume = uart3_data_pack.rxbuf[6]<<8|uart3_data_pack.rxbuf[7];
			g_sysparam_st.ci1302.snoreState = uart3_data_pack.rxbuf[8];
			g_sysparam_st.sf.snorevolume = g_sysparam_st.ci1302.snorevolume;
			// LOG_I("uart3 rece\n");
			x_uart_Internalparameterprint();
		}
		uart3_data_pack.rxindex = 0;
		memset(uart3_data_pack.rxbuf,0,sizeof(uart3_data_pack.rxbuf));
	}
}


void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)//
{
	s_tx_busy = false;
}

// UART_1 -> MFP
void x_uart1_init(void)		
{
	pinmux_uart1_init(UART_TX1_PIN, UART_RX1_PIN);
	
	// io_pull_write(UART_RX1_PIN, IO_PULL_UP);
	UART_Server1_Config.UARTX = UART1;
	UART_Server1_Config.Init.BaudRate = UART_BAUDRATE_38400;// 38400
	UART_Server1_Config.Init.MSBEN = 0;
	UART_Server1_Config.Init.Parity = UART_EVENPARITY;		// 偶
	UART_Server1_Config.Init.StopBits = UART_STOPBITS1;
	UART_Server1_Config.Init.WordLength = UART_BYTESIZE8;
	HAL_UART_Init(&UART_Server1_Config);
	HAL_UART_Receive_IT(&UART_Server1_Config, &rx1_t_data, 1);//调用此函数，串口接收使能，每次接收1byte，存放到uart_server_rx_byte
	//串口缓存初始化	
}

// UART_2 -> 离线语音
void x_uart2_init(void){
	pinmux_uart2_init(UART_TX2_PIN, UART_RX2_PIN);
	
	io_pull_write(UART_RX2_PIN, IO_PULL_UP);
	UART_Server2_Config.UARTX = UART2;
	UART_Server2_Config.Init.BaudRate = UART_BAUDRATE_9600;
	UART_Server2_Config.Init.MSBEN = 0;
	UART_Server2_Config.Init.Parity = UART_NOPARITY;
	UART_Server2_Config.Init.StopBits = UART_STOPBITS1;
	UART_Server2_Config.Init.WordLength = UART_BYTESIZE8;
	HAL_UART_Init(&UART_Server2_Config);
	HAL_UART_Receive_IT(&UART_Server2_Config, &rx2_t_data, 1);//调用此函数，串口接收使能，每次接收1byte，存放到uart_server_rx_byte
}

// UART_3 ->鼾声检测
void x_uart3_init(void)
{
	pinmux_uart3_init(UART_TX3_PIN, UART_RX3_PIN);
	
	io_pull_write(UART_RX3_PIN, IO_PULL_UP);
	UART_Server3_Config.UARTX = UART3;
	UART_Server3_Config.Init.BaudRate = UART_BAUDRATE_115200;
	UART_Server3_Config.Init.MSBEN = 0;
	UART_Server3_Config.Init.Parity = UART_NOPARITY;
	UART_Server3_Config.Init.StopBits = UART_STOPBITS1;
	UART_Server3_Config.Init.WordLength = UART_BYTESIZE8;
	HAL_UART_Init(&UART_Server3_Config);
	HAL_UART_Receive_IT(&UART_Server3_Config, &rx3_t_data, 1);//调用此函数，串口接收使能，每次接收1byte，存放到uart_server_rx_byte
	//串口缓存初始化	
}

// 改为与主控盒MFP通信(打鼾：抬起到记忆位置 几分钟后放平) （蓝牙语音：有多少个指令？协议怎么搞？）
void x_uart_realtimeprint(void)
{
	static uint8_t uartTxbuff[50];
	memset(uartTxbuff,0,sizeof(uartTxbuff));
	
	int i = 0;
	uint32_t temp = 0;		
	uint16_t pa_cur_t = g_sysparam_st.airpump.pa_cur;
	
	uartTxbuff[i++] = 0x5a;
	uartTxbuff[i++] = 0;
	uartTxbuff[i++] = 0;
	uartTxbuff[i++] = 0;
	uartTxbuff[i++] = VERSION&0xff;
	uartTxbuff[i++] = (VERSION>>8)&0xff;
	uartTxbuff[i++] = g_sysparam_st.humandetection;
	uartTxbuff[i++] = pa_cur_t&0xff;
	uartTxbuff[i++] = (pa_cur_t>>8)&0xff;
	uartTxbuff[i++] = 100;
	uartTxbuff[i++] = g_sysparam_st.airpump.inflate_onff;
	uartTxbuff[i++] = g_sysparam_st.airpump.deflate_onoff;
	uartTxbuff[i++] = g_sysparam_st.ai_adj;
	uartTxbuff[i++] = g_sysparam_st.sf.snoreNub&0xff;
	uartTxbuff[i++] = (g_sysparam_st.sf.snoreNub>>8)&0xff;
	
	uartTxbuff[i++] = g_sysparam_st.sf.snoreState;
	
	uartTxbuff[i++] = g_sysparam_st.sf.breathe;
	
	uartTxbuff[i++] = g_sysparam_st.sf.bigMove&0xff;
	uartTxbuff[i++] = (g_sysparam_st.sf.bigMove>>8)&0xff;
	
	uartTxbuff[i++] = g_sysparam_st.sf.smallMove&0xff;
	uartTxbuff[i++] = (g_sysparam_st.sf.smallMove>>8)&0xff;
	

	uartTxbuff[i++] = g_sysparam_st.sf.snorevolume&0xff;
	uartTxbuff[i++] = (g_sysparam_st.sf.snorevolume>>8)&0xff;
	uartTxbuff[i++] = g_sysparam_st.snoreIntervention.enable;
	uartTxbuff[i++] = g_sysparam_st.stretch.stretchTrig;
	
	uartTxbuff[i++] = g_sysparam_st.sf.snoreIntervenNub&0xff;;
	uartTxbuff[i++] = (g_sysparam_st.sf.snoreIntervenNub>>8)&0xff;
	uartTxbuff[i++] = g_sysparam_st.sf.stabilize;
	
	uartTxbuff[i++] = g_sysparam_st.snoreIntervention.triging;
	
	uartTxbuff[i++] = g_sysparam_st.airpump.aspirator_onff;
	uartTxbuff[i++] = g_sysparam_st.ai_adj_strength;
	
	uartTxbuff[i++] = (g_sysparam_st.stretch.timerOut/100)&0xff;;
	uartTxbuff[i++] = ((g_sysparam_st.stretch.timerOut/100)>>8)&0xff;
	
	
	
	uartTxbuff[1] = i-2;	
	temp = Modbus_Crc_Compute(uartTxbuff,i);
	
	uartTxbuff[i++] = temp&0xff;
	uartTxbuff[i++] = (temp>>8)&0xff; 
	
	HAL_UART_Transmit(&UART_Server1_Config, uartTxbuff, i,0xffff);
//	if(s_tx_busy)
//	{
//			LOG_I("Uart tx busy, data discard!");
//	}
//	else
//	{
//			s_tx_busy = true;
//			HAL_UART_Transmit_IT(&UART_Server1_Config, uartTxbuff, i);
//	} 
}


void uart_BLEvoice_send(void)
{
	static uint8_t IuartTxbuff[50];
	memset(IuartTxbuff,0,sizeof(IuartTxbuff));
	
	int i = 0;
	uint32_t temp = 0;		
	uint16_t pa_cur_t = g_sysparam_st.airpump.pa_cur;
	uint32_t utc;
	
	utc = x_time_RTC_ToUTC();
	
	IuartTxbuff[i++] = 0x5a;
	IuartTxbuff[i++] = 0;
	IuartTxbuff[i++] = 0xff;
	IuartTxbuff[i++] = 0xff;
	
	IuartTxbuff[i++] = g_sysparam_st.snoreIntervention.trigTimer&0xff;
	IuartTxbuff[i++] = (g_sysparam_st.snoreIntervention.trigTimer>>8)&0xff;
	IuartTxbuff[i++] = (g_sysparam_st.snoreIntervention.trigTimer>>16)&0xff;
	IuartTxbuff[i++] = (g_sysparam_st.snoreIntervention.trigTimer>>24)&0xff;
	
	IuartTxbuff[i++] = g_sysparam_st.airpump.adjPa&0xff;
	IuartTxbuff[i++] = (g_sysparam_st.airpump.adjPa>>8)&0xff;
	
	IuartTxbuff[i++] = g_sysparam_st.sf.ThresholdMove&0xff;
	IuartTxbuff[i++] = (g_sysparam_st.sf.ThresholdMove>>8)&0xff;
	
	IuartTxbuff[i++] = (utc>>0)&0xff;
	IuartTxbuff[i++] = (utc>>8)&0xff;
	IuartTxbuff[i++] = (utc>>16)&0xff;
	IuartTxbuff[i++] = (utc>>24)&0xff;

	IuartTxbuff[1] = i-2;	
	temp = Modbus_Crc_Compute(IuartTxbuff,i);
	
	IuartTxbuff[i++] = temp&0xff;
	IuartTxbuff[i++] = (temp>>8)&0xff; 
	
	HAL_UART_Transmit(&UART_Server1_Config, IuartTxbuff, i,0xffff);
//	if(s_tx_busy)
//	{
//			LOG_I("Uart tx busy, data discard!");
//	}
//	else
//	{
//			s_tx_busy = true;
//			HAL_UART_Transmit_IT(&UART_Server1_Config, IuartTxbuff, i);
//	} 
}


void x_uart_Internalparameterprint(void)		// uart_Snoring_event_send   算求了 反正也不用发送
{
	static uint8_t IuartTxbuff[50];
	memset(IuartTxbuff,0,sizeof(IuartTxbuff));
	
	int i = 0;
	uint32_t temp = 0;		
	IuartTxbuff[i++] = 0x5a;
	IuartTxbuff[i++] = 0x05;
	IuartTxbuff[i++] = 0xff;
	IuartTxbuff[i++] = 0xff;
	// IuartTxbuff[1] = i-2;	
	temp = Modbus_Crc_Compute(IuartTxbuff,i);
	
	IuartTxbuff[i++] = temp&0xff;
	IuartTxbuff[i++] = (temp>>8)&0xff; 
	//LOG_I("UART3 TX...");
	HAL_UART_Transmit(&UART_Server3_Config, IuartTxbuff, i,0xffff);
	
//	if(s_tx_busy)
//	{
//			LOG_I("Uart tx busy, data discard!");
//	}
//	else
//	{
//			s_tx_busy = true;
//			HAL_UART_Transmit_IT(&UART_Server1_Config, IuartTxbuff, i);
//	} 
}




void x_uart_init(void)
{
	// x_uart1_init();
	x_uart2_init();
	//x_uart3_init();
}
