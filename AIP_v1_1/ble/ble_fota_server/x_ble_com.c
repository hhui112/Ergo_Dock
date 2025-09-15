#include "x_ble_com.h"
#include "g.h"
#include "mfp_queue.h"

uint8_t blerxbuff[150];

uint8_t bletxbuff[150];

uint8_t bletxlen;
void x_ble_server_ack(uint8_t cmd,uint8_t subcmd);

//void x_ble_com_handle(uint8_t *pbuff,uint8_t len)
//{
//uint8_t cmd = pbuff[2];
//uint8_t subcmd = pbuff[3];
//uint16_t pa_t;
//uint64_t t_utc = 0;
//memset(blerxbuff,0,150);

//memcpy(blerxbuff,pbuff,len);


//if(cmd == 1)
//{
//	switch(subcmd)
//	{
//		case 0://�򿪳���
//				app_airpump_inflate_on();
//			break;
//		case 1://�رճ���
//				app_airpump_inflate_off();
//			break;
//		case 2://�򿪷���
//				app_airpump_deflate_on();
//			break;
//		case 3://�رշ���
//				app_airpump_deflate_off();
//			break;	
//		case 4://ģʽ�л�
//				if(pbuff[4] <= 1)				
//					g_sysparam_st.ai_adj = pbuff[4];			
//			break;	
//		case 5://����Ŀ����ѹֵ
//					//todo
//			break;					
//		case 6://����Ŀ����ѹֵ
//					pa_t = pbuff[5]<<8|pbuff[4];		
//					adjust_pressure(pa_t,100);
//			break;
//		case 7://����ģʽ����
//					if(pbuff[4] <= 1)		
//					{
//						if(pbuff[4] == 0)				
//							StretchingModeStop();				
//						else if(pbuff[4] == 1)			
//							StretchingModeStart();
//						
//					}					
//			break;	
//		case 13://������չ����
//					if(pbuff[4] == 0)								
//						StretchingModeStop();
//					else
//						StretchingModeStart();
//							break;	
//		case 14://����ʹ��
//					if(pbuff[4] == 0)								
//						g_sysparam_st.snoreIntervention.enable = 0;
//					else
//						g_sysparam_st.snoreIntervention.enable = 1;			
//			break;
//		case 15://�趨������ѹ
//						g_sysparam_st.airpump.adjPa = pbuff[5]<<8|pbuff[4];			
//			break;
//		

//		case 16://��������
//						if(pbuff[4] == 0)
//						{
//							g_sysparam_st.airpump.aspirator_onff = 0;
//						}
//						else if(pbuff[4] == 1)
//						{
//							g_sysparam_st.airpump.aspirator_onff = 1;
//						}			
//			break;

//		case 17://����Ӧǿ��
//					if(pbuff[4]>=1  && pbuff[4]<=3)
//						g_sysparam_st.ai_adj_strength = pbuff[4];		
//			break;

//		case 18://utcʱ��ͬ��
//				x_rtc_set((pbuff[5]<<8)|pbuff[4],pbuff[6],pbuff[7],pbuff[8],pbuff[9],pbuff[10],pbuff[11]);		
//			break;

//		case 19:
//						x_report_get(BLE_UPDATA);
//			break;	
//		case 21:
//						x_report_get_snoringVolume(BLE_UPDATA);
//			break;
//		case 22:
//						x_report_get_tidongRecord(BLE_UPDATA);
//			break;
//		case 23:
//						x_report_get_adjRecord(BLE_UPDATA);
//			break;
//		case 24:
//						x_report_get_snoringinterveneRecord(BLE_UPDATA);
//			break;
//		case 20://��utc����ϵͳʱ��
//						t_utc = pbuff[4]|pbuff[5]<<8|pbuff[6]<<16|pbuff[7]<<24|
//								(uint64_t)pbuff[8]<<32|(uint64_t)pbuff[9]<<40|(uint64_t)pbuff[10]<<48|(uint64_t)pbuff[11]<<56;

//						x_time_UTC_ToRTC(t_utc);
//			break;	
//		case 25:
//						app_flatten();
//			break;
//	
//		default:
//			break;			
//	}
//	if((subcmd != 19) && (subcmd != 21) && (subcmd != 22) )
//		x_ble_server_ack(cmd,subcmd);
//}	
//}


void x_ble_com_handle(uint8_t *pbuff,uint8_t len)
{
	uint8_t cmd = pbuff[2];
	uint8_t subcmd = pbuff[3];
	uint32_t key;
	uint8_t pwm;
	uint8_t tmr;
	uint16_t crc1_t = 0 , crc2_t = 0;
	// memset(blerxbuff,0,150);

	// memcpy(blerxbuff,pbuff,len);

	if(pbuff[0] != 0x5A) return;

	uint8_t total_len = pbuff[1] + 4;		// ���ݳ��� + ֡ͷ + ����λ + cmd + subcmd
	if(len < total_len) return; 

	crc1_t = ((uint16_t)pbuff[total_len + 1] << 8) | pbuff[total_len];
	LOG_I("CRC:   0x%04X, 0x%04X", pbuff[total_len + 1],  pbuff[total_len]);
	crc2_t = Modbus_Crc_Compute(pbuff, total_len);
	if(crc2_t != crc1_t){
			LOG_I("CRC ERR: calc=0x%04X, recv=0x%04X", crc2_t, crc1_t);
			return;
	}
		
	if(cmd == 0){
		
		switch(subcmd)
		{
			case 0x00:		// ���/ֹͣ����
				prepare_mfp_NORMAL_KET(KEY_MASSAGE_STOP_ALL,1); 
				break;
			default:
				break;			
		}
		
	}else if(cmd == 1)
	{
		switch(subcmd)
		{
			case 0x05:  // ��ͨ��ֵ����
				
			key = ( (uint32_t)pbuff[4] << 24 ) |
						( (uint32_t)pbuff[5] << 16 ) |
						( (uint32_t)pbuff[6] << 8  ) |
						( (uint32_t)pbuff[7]       );
			LOG_I("CMD =  0x%02X, SUBCMD = 0x%02X key = 0x%08X",cmd,subcmd,key);
			
			prepare_mfp_NORMAL_KET(key,3);

				break;
			case 0x07:		// ��������ֵ����
			key = ( (uint32_t)pbuff[4] << 24 ) |
						( (uint32_t)pbuff[5] << 16 ) |
						( (uint32_t)pbuff[6] << 8  ) |
						( (uint32_t)pbuff[7]       );
			
			pwm = pbuff[8];
			tmr = pbuff[9];
			prepare_mfp_SOFT_START(key, pwm, tmr,3);
			LOG_I("CMD =  0x%02X, SUBCMD = 0x%02X key = 0x%08X",cmd,subcmd,key);
				break;
			default:
				break;
		}
		
		
		if((subcmd != 19) && (subcmd != 21) && (subcmd != 22) )
			x_ble_server_ack(cmd,subcmd);
	}	
}


void x_ble_com_txbuffFill(void)
{
	memset(bletxbuff,0,sizeof(bletxbuff));
	
	int i = 0;
	uint32_t temp = 0;		
	uint16_t pa_cur_t = g_sysparam_st.airpump.pa_cur;
	
	bletxbuff[i++] = 0x5a;
	bletxbuff[i++] = 0;		//lenth
	bletxbuff[i++] = 0;		// cmd
	bletxbuff[i++] = 2;		// subcmd
	bletxbuff[i++] = VERSION&0xff;
	bletxbuff[i++] = (VERSION>>8)&0xff;

	bletxbuff[i++] = g_sysparam_st.ubb;		// ���׵�
	bletxbuff[i++] = g_sysparam_st.snoreIntervention.enable;		// ������Ԥʹ��
	bletxbuff[i++] = g_sysparam_st.AntiSnore_intensity;		    //������Ԥǿ��
	bletxbuff[i++] = g_sysparam_st.snoreIntervention.triging;  // ������Ԥ������־λ

	bletxbuff[i++] = g_offline_voice.key_enable; 	// ��������ʹ��
	bletxbuff[i++] = g_offline_voice.wake_word;		 // ���Ѵ�����
	bletxbuff[i++] = g_offline_voice.enabled;		// ������������
	
	bletxbuff[i++] = g_sysparam_st.charge_state; // ���߳��־λ
	bletxbuff[i++] = g_sysparam_st.ble.ble_pair_flag; // ������Ա�־λ
	bletxbuff[i++] = g_sysparam_st.ble.ble_connect; // �������ӱ�־λ
	
	bletxbuff[1] = i-4;	
	temp = Modbus_Crc_Compute(bletxbuff,i);
	
	bletxbuff[i++] = temp&0xff;
	bletxbuff[i++] = (temp>>8)&0xff; 
	
	bletxlen = i;
}


void x_ble_server_ack(uint8_t cmd,uint8_t subcmd)
{
	memset(bletxbuff,0,sizeof(bletxbuff));
	
	int i = 0;
	uint32_t temp = 0;		
	// uint16_t pa_cur_t = g_sysparam_st.airpump.pa_cur;
	
	bletxbuff[i++] = 0x5a;
	bletxbuff[i++] = 2;		//lenth
	bletxbuff[i++] = 0;		// cmd
	bletxbuff[i++] = 2;		// subcmd
	
	bletxbuff[i++] = cmd;
	bletxbuff[i++] = subcmd;
	
	bletxbuff[1] = i-4;	

	temp = Modbus_Crc_Compute(bletxbuff,i);
	
	bletxbuff[i++] = temp&0xff;
	bletxbuff[i++] = (temp>>8)&0xff; 
	bletxlen = i;
	ls_bleup_server_send_notification(bletxbuff,bletxlen);//��ѯ�Ƿ����������з���
}



void x_ble_timer_sync(void)
{
	
}
