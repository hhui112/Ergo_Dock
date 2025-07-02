#include "x_ble_com.h"
#include "g.h"

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
//		case 0://打开充气
//				app_airpump_inflate_on();
//			break;
//		case 1://关闭充气
//				app_airpump_inflate_off();
//			break;
//		case 2://打开放气
//				app_airpump_deflate_on();
//			break;
//		case 3://关闭放气
//				app_airpump_deflate_off();
//			break;	
//		case 4://模式切换
//				if(pbuff[4] <= 1)				
//					g_sysparam_st.ai_adj = pbuff[4];			
//			break;	
//		case 5://设置目标气压值
//					//todo
//			break;					
//		case 6://设置目标气压值
//					pa_t = pbuff[5]<<8|pbuff[4];		
//					adjust_pressure(pa_t,100);
//			break;
//		case 7://拉伸模式开关
//					if(pbuff[4] <= 1)		
//					{
//						if(pbuff[4] == 0)				
//							StretchingModeStop();				
//						else if(pbuff[4] == 1)			
//							StretchingModeStart();
//						
//					}					
//			break;	
//		case 13://开启舒展开关
//					if(pbuff[4] == 0)								
//						StretchingModeStop();
//					else
//						StretchingModeStart();
//							break;	
//		case 14://打鼾使能
//					if(pbuff[4] == 0)								
//						g_sysparam_st.snoreIntervention.enable = 0;
//					else
//						g_sysparam_st.snoreIntervention.enable = 1;			
//			break;
//		case 15://设定舒适气压
//						g_sysparam_st.airpump.adjPa = pbuff[5]<<8|pbuff[4];			
//			break;
//		

//		case 16://抽气阀开
//						if(pbuff[4] == 0)
//						{
//							g_sysparam_st.airpump.aspirator_onff = 0;
//						}
//						else if(pbuff[4] == 1)
//						{
//							g_sysparam_st.airpump.aspirator_onff = 1;
//						}			
//			break;

//		case 17://自适应强度
//					if(pbuff[4]>=1  && pbuff[4]<=3)
//						g_sysparam_st.ai_adj_strength = pbuff[4];		
//			break;

//		case 18://utc时间同步
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
//		case 20://用utc设置系统时间
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



void x_ble_com_txbuffFill(void)
{
	memset(bletxbuff,0,sizeof(bletxbuff));
	
	int i = 0;
	uint32_t temp = 0;		
	uint16_t pa_cur_t = g_sysparam_st.airpump.pa_cur;
	
	bletxbuff[i++] = 0x5a;
	bletxbuff[i++] = 0;
	bletxbuff[i++] = 0;
	bletxbuff[i++] = 0;
	bletxbuff[i++] = VERSION&0xff;
	bletxbuff[i++] = (VERSION>>8)&0xff;
	bletxbuff[i++] = g_sysparam_st.humandetection;
	bletxbuff[i++] = pa_cur_t&0xff;
	bletxbuff[i++] = (pa_cur_t>>8)&0xff;
	bletxbuff[i++] = 100;
	bletxbuff[i++] = g_sysparam_st.airpump.inflate_onff;
	bletxbuff[i++] = g_sysparam_st.airpump.deflate_onoff;
	bletxbuff[i++] = g_sysparam_st.ai_adj;
	bletxbuff[i++] = g_sysparam_st.sf.snoreNub&0xff;
	bletxbuff[i++] = (g_sysparam_st.sf.snoreNub>>8)&0xff;
	
	bletxbuff[i++] = g_sysparam_st.sf.snoreState;
	
	bletxbuff[i++] = g_sysparam_st.sf.breathe;
	bletxbuff[i++] = g_sysparam_st.sf.bigMove&0xff;
	bletxbuff[i++] = (g_sysparam_st.sf.bigMove>>8)&0xff;
	bletxbuff[i++] = g_sysparam_st.sf.smallMove&0xff;
	bletxbuff[i++] = (g_sysparam_st.sf.smallMove>>8)&0xff;
	
	bletxbuff[i++] = g_sysparam_st.sf.snorevolume&0xff;
	bletxbuff[i++] = (g_sysparam_st.sf.snorevolume>>8)&0xff;
		
	bletxbuff[i++] = g_sysparam_st.snoreIntervention.enable;
	bletxbuff[i++] = g_sysparam_st.stretch.stretchTrig;	
	bletxbuff[i++] = g_sysparam_st.sf.snoreIntervenNub&0xff;;
	bletxbuff[i++] = (g_sysparam_st.sf.snoreIntervenNub>>8)&0xff;
	bletxbuff[i++] = g_sysparam_st.sf.stabilize;
	bletxbuff[i++] = g_sysparam_st.snoreIntervention.triging;
	bletxbuff[i++] = g_sysparam_st.airpump.aspirator_onff;
	bletxbuff[i++] = g_sysparam_st.ai_adj_strength;
	
	bletxbuff[i++] = (g_sysparam_st.stretch.timerOut/100)&0xff;;
	bletxbuff[i++] = ((g_sysparam_st.stretch.timerOut/100)>>8)&0xff;
	
	bletxbuff[1] = i-2;	
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
	uint16_t pa_cur_t = g_sysparam_st.airpump.pa_cur;
	
	bletxbuff[i++] = 0x5a;
	bletxbuff[i++] = 0;
	bletxbuff[i++] = 2;
	bletxbuff[i++] = 0;
	
	bletxbuff[i++] = cmd;
	bletxbuff[i++] = subcmd;
	
	bletxbuff[1] = i-2;	

	temp = Modbus_Crc_Compute(bletxbuff,i);
	
	bletxbuff[i++] = temp&0xff;
	bletxbuff[i++] = (temp>>8)&0xff; 
	bletxlen = i;
	ls_bleup_server_send_notification(bletxbuff,bletxlen);//查询是否有数据上行发送
}



void x_ble_timer_sync(void)
{
	
}
