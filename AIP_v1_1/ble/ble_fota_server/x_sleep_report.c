#include "x_sleep_report.h"
#include "g.h"

#define AUTO_END_REPORT_TIME    (30*60*100) 



static uint8_t  snoringVolumeupdataFlag = 0;

report_t  report;
report_sys_t ctrl_report;

static uint16_t oldbigMove,oldsmallMove,oldsnoreNub,oldadjNub,oldsnoreIntervenNub;

void x_report_record_once_per_minute(void);
void x_report_snoringVolumeupdataRun(void);


void x_report_time_10ms(void)
{
	
}


void x_report_init(void)
{
	
}

void x_report_start(void)
{
	memset(&report,0,sizeof(report));
	oldbigMove = g_sysparam_st.sf.bigMove;
	oldsmallMove = g_sysparam_st.sf.smallMove;
	oldsnoreNub = g_sysparam_st.sf.snoreNub;
	oldsnoreIntervenNub = g_sysparam_st.sf.snoreIntervenNub;
	oldadjNub = g_sysparam_st.cboxlog_st.ai_adjNub;
	
	
	
	report.sleeptimer_come = x_time_RTC_ToUTC();
}

void x_report_analyse(void)
{
	
	if(g_sysparam_st.sf.bigMove != oldbigMove)
		report.tidongBigNub++;
	
	if(oldsmallMove != g_sysparam_st.sf.smallMove)
		report.tidongSmallNub++;
		
	if(oldsnoreNub != g_sysparam_st.sf.snoreNub)
		report.snoringNub++;	
	
	report.tidongNub = report.tidongSmallNub+report.snoringNub;
		
	if(oldsnoreIntervenNub != g_sysparam_st.sf.snoreIntervenNub)
		report.interventionsNub++;

	if(oldadjNub != g_sysparam_st.cboxlog_st.ai_adjNub)
		report.adjNubl++;	

	oldbigMove = g_sysparam_st.sf.bigMove;
	oldsmallMove = g_sysparam_st.sf.smallMove;
	oldsnoreNub = g_sysparam_st.sf.snoreNub;
	oldsnoreIntervenNub = g_sysparam_st.sf.snoreIntervenNub;
	oldadjNub = g_sysparam_st.cboxlog_st.ai_adjNub;
}

void x_report_record(void)
{
	x_report_record_once_per_minute();
}


//void x_reportReportCtrl(void)
//{
//		if()
//}


void x_report_run(void)
{
	static uint8_t oldReportTrig;
		
	if( (oldReportTrig != ctrl_report.trig) && (ctrl_report.trig == 1))  //初始化
		x_report_start();	
	if((oldReportTrig != ctrl_report.trig) && (ctrl_report.trig == 0))  //结束记录	
		x_report_analyse();
	if((oldReportTrig == ctrl_report.trig) && (ctrl_report.trig == 1))  //记录中	
		x_report_record();	
		
	x_report_snoringVolumeupdataRun();
	ctrl_report.trig = oldReportTrig;
}



void x_report_report_test(void)
{
	int i = 0;
	report.sleeptimer_come = x_time_RTC_ToUTC();
	report.sleeptimer_leave = x_time_RTC_ToUTC()+8*3600;
	report.sleeptimer_total = report.sleeptimer_leave-report.sleeptimer_come;
	
	report.Snoringindex = 10;
	
	report.snoringNub = 11;
	report.snoringFre = 12;
	report.snoringTime = 13;
	report.snoringVolume = 14;
	
	report.interventionsNub = 15;
	
	report.tidongNub = 16;
	report.adjNubl = 17;
	
	report.tidongSmallNub = 18;
	report.tidongBigNub = 19;
	report.tidongFre = 20;
	report.tidongInterval = 21;
	
	
	report.snoringVolumeNub = 720;
	report.tidongRecordNub = 720;
	report.adjRecordNub = 720;
	report.snoringinterveneNub = 720;
	
	for(i = 0;i<720;i++)
	{
		report.snoringVolumebuff[i] = i;
	}
	
	memset(report.tidongRecord,0,sizeof(report.tidongRecord));
	memset(report.adjRecord,0,sizeof(report.adjRecord));
	
	for(i = 0;i<report.tidongRecordNub/4;i++)
	{
			report.tidongRecord[i] = 0x90;	
	}
	
	for(i = 0;i<report.adjRecordNub/8;i++)
	{
			report.adjRecord[i] = i;	
	}
	
	for(i = 0;i<report.snoringinterveneNub/8;i++)
	{
			report.snoringinterveneRecord[i] = i;	
	}
	
}


void x_report_get(uint8_t type)
{
	uint8_t txbuff[200];
	uint16_t crc_t = 0;
	int i = 0;
	memset(txbuff,0,sizeof(txbuff));
	//组包
	uint32_t len;
	
	
	x_report_report_test();
	
	txbuff[i++] = 0x5a;
	txbuff[i++] = 0;
	txbuff[i++] = 0x02;
	txbuff[i++] = 0x01;
	
	len = sizeof(report)-sizeof(report.snoringVolumebuff)-sizeof(report.snoringVolumeNub)-
											 sizeof(report.tidongRecord)-sizeof(report.tidongRecordNub)-
											 sizeof(report.adjRecord)-sizeof(report.tidongRecordNub)-
											 sizeof(report.snoringinterveneRecord)-sizeof(report.snoringinterveneNub);
	memcpy(txbuff+i,&report.sleeptimer_come,len);
	
	i= len+i;
	
	txbuff[1] = i-2;	
	crc_t = Modbus_Crc_Compute(txbuff,i);
	
	txbuff[i++] = crc_t&0xff;
	txbuff[i++] = (crc_t>>8)&0xff; 
	
	if(type == 0)
	{
		HAL_UART_Transmit(&UART_Server1_Config, txbuff, i,0xffff);
	}
	else
	{
		ls_bleup_server_send_notification(txbuff,i);//查询是否有数据上行发送
	}
}


void x_report_get_snoringVolume(uint8_t type)
{
	uint16_t crc_t = 0;
	int i = 0,l;
	uint8_t txbuff[200];
	uint8_t fbNub = 0,len = 0;
	
	memset(txbuff,0,sizeof(txbuff));
	
	if(report.snoringVolumeNub%SUBCONTRACT_SIZE == 0)
		fbNub = report.snoringVolumeNub/SUBCONTRACT_SIZE;
	else
		fbNub = report.snoringVolumeNub/SUBCONTRACT_SIZE+1;
	
	
	for(l = 0; l <fbNub;l++)
	{
		i = 0;
		txbuff[i++] = 0x5a;
		txbuff[i++] = 0;
		txbuff[i++] = 0x02;
		txbuff[i++] = 0x03;
		
		txbuff[i++] = report.sleeptimer_come;
		txbuff[i++] = (report.sleeptimer_come>> 8)&0xff;
		txbuff[i++] = (report.sleeptimer_come>> 16)&0xff;
		txbuff[i++] = (report.sleeptimer_come>> 24)&0xff;
		txbuff[i++] = (report.sleeptimer_come>> 32)&0xff;
		txbuff[i++] = (report.sleeptimer_come>> 40)&0xff;
		txbuff[i++] = (report.sleeptimer_come>> 48)&0xff;
		txbuff[i++] = (report.sleeptimer_come>> 56)&0xff;
		
		
		txbuff[i++] = fbNub;
		txbuff[i++] = l+1;
		
		if(l+1 == fbNub)//最后一包
		{
			memcpy(txbuff+i,report.snoringVolumebuff+l*SUBCONTRACT_SIZE,report.snoringVolumeNub-l*SUBCONTRACT_SIZE);		
			i = i+report.snoringVolumeNub-l*SUBCONTRACT_SIZE;	
		}
		else
		{
			memcpy(txbuff+i,report.snoringVolumebuff+l*SUBCONTRACT_SIZE,SUBCONTRACT_SIZE);		
			i = i+SUBCONTRACT_SIZE;	
		}
		
		txbuff[1] = i-2;	
		
		crc_t = Modbus_Crc_Compute(txbuff,i);
		
		txbuff[i++] = crc_t&0xff;
		txbuff[i++] = (crc_t>>8)&0xff; 
		
		
		
		if(type == UART_UPDATA)
		{
			HAL_UART_Transmit(&UART_Server1_Config, txbuff, i,0xffffffff);
		}
		else
		{
			 snoringVolumeupdataFlag = 1;
		}
	}
}

void x_report_get_tidongRecord(uint8_t type)
{
	uint16_t crc_t = 0;
	int i = 0;
	
	uint8_t txbuff[200];
	
	memset(txbuff,0,sizeof(txbuff));
	
	txbuff[i++] = 0x5a;
	txbuff[i++] = 0;
	txbuff[i++] = 0x02;
	txbuff[i++] = 0x04;
	
	
	txbuff[i++] = report.sleeptimer_come;
	txbuff[i++] = (report.sleeptimer_come>> 8)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 16)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 24)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 32)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 40)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 48)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 56)&0xff;
	
	txbuff[i++] = report.tidongRecordNub;
	txbuff[i++] = report.tidongRecordNub >> 8;
	
	
	if(report.tidongRecordNub%4 == 0)
	{
		memcpy(txbuff+i,report.tidongRecord,report.tidongRecordNub/4);
		i=i+report.tidongRecordNub/4;
	}
	else
	{
		memcpy(txbuff+i,report.tidongRecord,report.tidongRecordNub/4+1);
		i=i+report.tidongRecordNub/4+1;
	}
	txbuff[1] = i-2;	
	crc_t = Modbus_Crc_Compute(txbuff,i);
	
	txbuff[i++] = crc_t&0xff;
	txbuff[i++] = (crc_t>>8)&0xff; 
		
	if(type == UART_UPDATA)
	{	
		HAL_UART_Transmit(&UART_Server1_Config, txbuff, i,0xffffffff);			
	}
	else
	{
		ls_bleup_server_send_notification(txbuff,i);//查询是否有数据上行发送
	}
}


void x_report_get_adjRecord(uint8_t type)
{
	uint16_t crc_t = 0;
	int i = 0;
	
	uint8_t txbuff[200];
	
	memset(txbuff,0,sizeof(txbuff));
	
	txbuff[i++] = 0x5a;
	txbuff[i++] = 0;
	txbuff[i++] = 0x02;
	txbuff[i++] = 0x02;
	
	
	txbuff[i++] = report.sleeptimer_come;
	txbuff[i++] = (report.sleeptimer_come>> 8)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 16)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 24)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 32)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 40)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 48)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 56)&0xff;
	
	txbuff[i++] = report.adjRecordNub;
	txbuff[i++] = report.adjRecordNub >> 8;
	
	
	if(report.adjRecordNub%8 == 0)
	{
		memcpy(txbuff+i,report.adjRecord,report.adjRecordNub/8);
		i=i+report.adjRecordNub/8;
	}
	else
	{
		memcpy(txbuff+i,report.adjRecord,report.adjRecordNub/8+1);
		i=i+report.adjRecordNub/8+1;
	}
	txbuff[1] = i-2;	
	crc_t = Modbus_Crc_Compute(txbuff,i);
	
	txbuff[i++] = crc_t&0xff;
	txbuff[i++] = (crc_t>>8)&0xff; 
		
	if(type == UART_UPDATA)
	{	
		HAL_UART_Transmit(&UART_Server1_Config, txbuff, i,0xffffffff);			
	}
	else
	{
		ls_bleup_server_send_notification(txbuff,i);//查询是否有数据上行发送
	}
}

void x_report_get_snoringinterveneRecord(uint8_t type)
{
	uint16_t crc_t = 0;
	int i = 0;
	
	uint8_t txbuff[200];
	
	memset(txbuff,0,sizeof(txbuff));
	
	txbuff[i++] = 0x5a;
	txbuff[i++] = 0;
	txbuff[i++] = 0x02;
	txbuff[i++] = 0x05;
	
	
	txbuff[i++] = report.sleeptimer_come;
	txbuff[i++] = (report.sleeptimer_come>> 8)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 16)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 24)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 32)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 40)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 48)&0xff;
	txbuff[i++] = (report.sleeptimer_come>> 56)&0xff;
	
	txbuff[i++] = report.snoringinterveneNub;
	txbuff[i++] = report.snoringinterveneNub >> 8;
	
	
	if(report.snoringinterveneNub%8 == 0)
	{
		memcpy(txbuff+i,report.snoringinterveneRecord,report.snoringinterveneNub/8);
		i=i+report.snoringinterveneNub/8;
	}
	else
	{
		memcpy(txbuff+i,report.snoringinterveneRecord,report.snoringinterveneNub/8+1);
		i=i+report.interventionsNub/8+1;
	}
	txbuff[1] = i-2;	
	crc_t = Modbus_Crc_Compute(txbuff,i);
	
	txbuff[i++] = crc_t&0xff;
	txbuff[i++] = (crc_t>>8)&0xff; 
		
	if(type == UART_UPDATA)
	{	
		HAL_UART_Transmit(&UART_Server1_Config, txbuff, i,0xffffffff);			
	}
	else
	{
		ls_bleup_server_send_notification(txbuff,i);//查询是否有数据上行发送
	}
}



void x_report_snoringVolumeupdataRun(void)
{
	uint16_t crc_t = 0;
	int i = 0;
	uint8_t txbuff[200];
	uint8_t fbNub = 0,len = 0;
	
	static uint32_t timer_t,l;
	
	
	
	if(snoringVolumeupdataFlag == 1)
	{		
			memset(txbuff,0,sizeof(txbuff));
			
			if(report.snoringVolumeNub%SUBCONTRACT_SIZE == 0)
				fbNub = report.snoringVolumeNub/SUBCONTRACT_SIZE;
			else
				fbNub = report.snoringVolumeNub/SUBCONTRACT_SIZE+1;
			
			
			
			if(timer_t == 0)
			{
				timer_t = g_sysparam_st.timer;
			}
			
			if(timer_t !=0 && (g_sysparam_st.timer - timer_t >20))
			{
				i = 0;
				txbuff[i++] = 0x5a;
				txbuff[i++] = 0;
				txbuff[i++] = 0x02;
				txbuff[i++] = 0x03;
				
				txbuff[i++] = report.sleeptimer_come;
				txbuff[i++] = (report.sleeptimer_come>> 8)&0xff;
				txbuff[i++] = (report.sleeptimer_come>> 16)&0xff;
				txbuff[i++] = (report.sleeptimer_come>> 24)&0xff;
				txbuff[i++] = (report.sleeptimer_come>> 32)&0xff;
				txbuff[i++] = (report.sleeptimer_come>> 40)&0xff;
				txbuff[i++] = (report.sleeptimer_come>> 48)&0xff;
				txbuff[i++] = (report.sleeptimer_come>> 56)&0xff;
				
				
				txbuff[i++] = fbNub;
				txbuff[i++] = l+1;
				
				if(l+1 == fbNub)//最后一包
				{
					memcpy(txbuff+i,report.snoringVolumebuff+l*SUBCONTRACT_SIZE,report.snoringVolumeNub-l*SUBCONTRACT_SIZE);
					
					 i = i+report.snoringVolumeNub-l*SUBCONTRACT_SIZE;	
				}
				else
				{
					memcpy(txbuff+i,report.snoringVolumebuff+l*SUBCONTRACT_SIZE,SUBCONTRACT_SIZE);
					
					i = i+SUBCONTRACT_SIZE;	
				}
				
				txbuff[1] = i-2;	
				
				crc_t = Modbus_Crc_Compute(txbuff,i);
				
				txbuff[i++] = crc_t&0xff;
				txbuff[i++] = (crc_t>>8)&0xff; 
				l++;
				ls_bleup_server_send_notification(txbuff,i);
				if(l >= fbNub)
				{
					snoringVolumeupdataFlag = 0;
					l = 0;
				}
			}
	}
	else
	{
		l = 0;
	}
}


void x_report_record_once_per_minute(void)
{
	static uint64_t oldutc_t,oldbigMove_t,oldsmallMove_t,oldinterventionsNub_t,oldadjNubl_t;
	uint16_t index_t = 0,indexbig_t = 0;
	
	
	if(oldutc_t == 0)	
	{
		oldutc_t = x_time_RTC_ToUTC();
		oldbigMove_t = report.tidongBigNub;
		oldsmallMove_t = report.tidongSmallNub;
		oldinterventionsNub_t = report.interventionsNub;
		oldadjNubl_t = report.adjNubl;
	}
		
	
	
	if(x_time_RTC_ToUTC() - oldutc_t >= 60)//触发一次记录
	{
		//记录音量		
		report.snoringVolumebuff[report.snoringVolumeNub] = g_sysparam_st.sf.snoreState<<7|g_sysparam_st.sf.snorevolume/10;
		report.snoringVolumeNub++;			
		
		//记录体动
		if( (report.tidongBigNub !=oldbigMove_t) || (report.tidongSmallNub !=oldsmallMove_t))
		{			
			index_t = report.tidongBigNub/4;			
			indexbig_t = report.tidongBigNub%4;
			
			if( (oldbigMove_t !=report.tidongBigNub) && (oldsmallMove_t != report.tidongSmallNub))
			{
				report.tidongRecord[index_t] = (report.tidongRecord[index_t]&(~(0x3<<indexbig_t)))|(0x2<<indexbig_t);
			}			
			else if( oldbigMove_t !=report.tidongBigNub  )
			{
				report.tidongRecord[index_t] = (report.tidongRecord[index_t]&(~(0x3<<indexbig_t)))|(0x2<<indexbig_t);
			}
			else if( oldsmallMove_t != report.tidongSmallNub)
			{
				report.tidongRecord[index_t] = (report.tidongRecord[index_t]&(~(0x3<<indexbig_t)))|(0x1<<indexbig_t);
			}	
			report.tidongRecordNub++;
		}
		
		//记录打鼾干预
		if( report.interventionsNub !=oldinterventionsNub_t)
		{
			index_t = report.snoringinterveneNub/4;			
			indexbig_t = report.snoringinterveneNub%4;
			
			report.snoringinterveneRecord[index_t] = (report.snoringinterveneRecord[index_t]&(~(0x3<<indexbig_t)))|(0x2<<indexbig_t);
			report.snoringinterveneNub++;
		}
		
		//自动调节记录
		if(report.adjNubl != oldadjNubl_t)
		{
			index_t = report.adjRecordNub/8;			
			indexbig_t = report.adjRecordNub%8;
			
			report.adjRecord[index_t] = (report.adjRecord[index_t]&(~(0x3<<indexbig_t)))|(0x2<<indexbig_t);
			report.adjRecordNub++;
		}
		
		
		oldutc_t = 0;		
	}
}


