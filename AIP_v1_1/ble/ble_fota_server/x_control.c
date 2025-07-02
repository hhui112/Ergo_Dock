#include "x_control.h"
#include "g.h"

#define DEPARTURETIMER 500  // �뿪5s�����Զ���ƽ

#define TOLERANCE	 		 100		//���ֵ
#define FLATPRESSURE 	 1200   //������ƽ��ѹ

#define STRETCH_HIGH 9000  			// ��չ��߳���Ϊ5Kpa
#define STRETCH_LOW  2500  			// ���Ϊ2500
#define STRETCH_TIMEROUT  (10*60*100)  // �����ʱ��


#define TEMP_PA  					8000// ���Ϊ2500
#define TEMP_PA_OVER  		1500// ���Ϊ2500



uint16_t lieLowbuff[] = {1200,1700,2300,};
uint16_t liesidebuff[] = {1500,2000,2300,};
uint16_t originalbuff[] = {1660,2220,2700};




void adjust_pressure_run(void);
void transition_ztox(void);
void adjust_pressure_Overchargedischargerun(void);
void self_adaptiveTrigger(void);
void StretchingModeRun(void);
uint32_t cf_timerout;
void adaptivecontrol_run(void);
void StretchingModeStop(void);

void control_timer10ms(void)
{
	int i = 0;
	
	for(i = 0; i <2;i++)
	{
		if(cf_timerout>0)
			cf_timerout--;	
	}
	
	if(g_sysparam_st.stretch.stretchTrig == 1)
	{
		
		
		if(g_sysparam_st.stretch.timerOut >0)
		{
			g_sysparam_st.stretch.timerOut--;		
			if(g_sysparam_st.stretch.timerOut == 0)
				StretchingModeStop();
		}
	}
	
	adjust_pressure_run();
	adjust_pressure_Overchargedischargerun();
	transition_ztox();
	adaptivecontrol_run();
	x_report_run();
}


void transition_ztox(void)
{
	static uint8_t oldadj =0xff;
	
	if((airbagsetfile->flag&0x02) == 0)
		g_sysparam_st.airpump.deflate_onoff = 0;
	else
		g_sysparam_st.airpump.deflate_onoff = 1;
	
	if((airbagsetfile->flag&0x01) == 0)
		g_sysparam_st.airpump.inflate_onff = 0;
	else
		g_sysparam_st.airpump.inflate_onff = 1;
	
	
	if(g_sysparam_st.ai_adj!= oldadj)
	{
		if(g_sysparam_st.ai_adj == 1 && g_sysparam_st.humandetection == 1)//����⵽����ͬʱ����AIģʽ�ᴥ��һ������Ӧ����
			self_adaptiveTrigger();
		
		if(g_sysparam_st.ai_adj == 1 && g_sysparam_st.humandetection == 0)//��ʼλ��
		{
				adjust_pressure(FLATPRESSURE,TOLERANCE);
		}
	}
	

	
	oldadj = g_sysparam_st.ai_adj;
}

void app_airpump_stopall(void)
{
	airbag_ctrbase = air_outstop;
}

void app_airpump_inflate_on(void)
{
	airbag_ctrbase = air_inputstart;
}
void app_airpump_inflate_off(void)
{
	airbag_ctrbase = air_inputstop;
}

void app_airpump_deflate_on(void)
{
	airbag_ctrbase = air_outstart;
}

void app_airpump_deflate_off(void)
{
	airbag_ctrbase = air_outstop;
}

void adjust_pressure_run(void)
{
	if(g_sysparam_st.timer <150)
		return;
	
	if(g_sysparam_st.airpump.trig == 1)
	{				
			if(g_sysparam_st.airpump.pa_tar > g_sysparam_st.airpump.pa_cur)
			{
					app_airpump_inflate_on();	
					g_sysparam_st.airpump.trig = 2;
					cf_timerout = 10000;
			}
			else
			{
				app_airpump_deflate_on();	
				g_sysparam_st.airpump.trig = 3;
				cf_timerout = 10000;
			}				
	}	
	else if(g_sysparam_st.airpump.trig == 2)//����
	{			
			if((g_sysparam_st.airpump.pa_tar <= g_sysparam_st.airpump.pa_cur) || (cf_timerout == 0))
			{
					app_airpump_inflate_off();	
					g_sysparam_st.airpump.trig = 0;				
			}			
	}
	else if(g_sysparam_st.airpump.trig == 3)//����
	{
		if((g_sysparam_st.airpump.pa_tar >= (g_sysparam_st.airpump.pa_cur)) || (cf_timerout == 0))
		{
			app_airpump_deflate_off();
			g_sysparam_st.airpump.trig = 0;
		}		
	}
}

void adjust_pressure_Overchargedischargerun(void)
{ 
	if(g_sysparam_st.airpump.trig == 0)
	{
		if(g_sysparam_st.airpump.trig_over == 1)
		{
			adjust_pressure(g_sysparam_st.airpump.pa_tar_over,TOLERANCE);
			g_sysparam_st.airpump.trig_over = 0;
		}
		else if(g_sysparam_st.airpump.trig_low == 1)
		{
			adjust_pressure(g_sysparam_st.airpump.pa_tar_low,TOLERANCE);
			g_sysparam_st.airpump.trig_over = 1;
			g_sysparam_st.airpump.trig_low = 0;
		}
	}
}


//������ѹֵ
void adjust_pressure(int target_pa,int tolerance) 
{
	g_sysparam_st.airpump.pa_tar = target_pa;
	g_sysparam_st.airpump.trig = 1;
	g_sysparam_st.airpump.tolerance = tolerance;
}

//ֹͣ����
void adjust_pressure_stop(void) 
{
	g_sysparam_st.airpump.trig = 0;
	g_sysparam_st.airpump.trig_low = 0;
	g_sysparam_st.airpump.trig_over = 0;
	cf_timerout = 0;
	app_airpump_deflate_off();
}

//������ѹֵ�����
/*
	target_pa Ŀ��ֵ��ѹֵ
	tolerance ��Χ
	overvlue	�����ֵ
*/
void adjust_pressure_Overchargedischarge(int target_pa,uint32_t overvlue,int tolerance) 
{
	uint32_t temp_t = 0;
//	if(g_sysparam_st.airpump.pa_cur > target_pa)
//	{
//		if(target_pa-overvlue>=800)		
//			g_sysparam_st.airpump.pa_tar = (float)((target_pa-overvlue/2) );
//		else
//			g_sysparam_st.airpump.pa_tar = (float)((target_pa+overvlue) );
//		g_sysparam_st.airpump.trig = 1;
//		g_sysparam_st.airpump.tolerance = tolerance;
//		g_sysparam_st.airpump.pa_tar_over = (float)((target_pa));
//		g_sysparam_st.airpump.trig_over = 1;
//	}
//	else
//	{
//		g_sysparam_st.airpump.pa_tar = (float)((target_pa+overvlue) );
//		g_sysparam_st.airpump.trig = 1;
//		g_sysparam_st.airpump.tolerance = tolerance;
//		g_sysparam_st.airpump.pa_tar_over = (float)((target_pa));
//		g_sysparam_st.airpump.trig_over = 1;
//	}
	HAL_TRNG_GenerateRandomNumber(&temp_t);	
	g_sysparam_st.airpump.trig_type = temp_t%2;
	
	if(g_sysparam_st.airpump.trig_type == 0)
	{
		LOG_I("trig_type  = 0");
		g_sysparam_st.airpump.pa_tar = target_pa+overvlue+500;
		g_sysparam_st.airpump.trig = 1;
		g_sysparam_st.airpump.tolerance = tolerance;
		g_sysparam_st.airpump.pa_tar_over =target_pa;
		g_sysparam_st.airpump.pa_tar_low = target_pa-overvlue;
		g_sysparam_st.airpump.trig_low = 1;
	}
	else
	{
		LOG_I("trig_type  = 1");
		g_sysparam_st.airpump.pa_tar = target_pa-overvlue ;
		g_sysparam_st.airpump.trig = 1;
		g_sysparam_st.airpump.tolerance = tolerance;
		g_sysparam_st.airpump.pa_tar_over =target_pa;
		g_sysparam_st.airpump.pa_tar_low =target_pa+overvlue +500;
		g_sysparam_st.airpump.trig_low = 1;		
	}
}

uint8_t departuretrigger_s;

void departuredetection(void)
{
	static uint8_t oldhumandetection;
	static uint32_t time_s;
	
	static uint32_t SearchCursorTirg;
	
	int i = 0;
	
	if(oldhumandetection!= g_sysparam_st.humandetection)
	{
		
		if(g_sysparam_st.humandetection == 0)
		{
			SearchCursorTirg = 0;
			//�뿪
			if(time_s == 0)
				time_s = g_sysparam_st.timer;	
		}
		else
		{		
			if(departuretrigger_s == 0)//��ǰ�����ڷ����׶�
			{
				SearchCursorTirg = 1;
			}
			else//��ǰ���ڷ����׶� ��ô�رշ��� ��ѹ������ǰ��
			{
				departuretrigger_s = 0;
				adjust_pressure_stop();
			}
		}
	}
	
	if( (SearchCursorTirg == 1) && (g_sysparam_st.airpump.Stableflag == 2))//�ȴ��ȶ������ж�
	{
		SearchCursorTirg = 0;
		
		if(g_sysparam_st.airpump.flatFlag == 1)
		{
			g_sysparam_st.airpump.flatFlag = 0;
			for(i = 0; i <sizeof(originalbuff)/2;i++) //�ж��ʺϵ��α�
			{
				if(g_sysparam_st.airpump.pa_cur < originalbuff[i])
					break;
			}
			
			if(i>=3)		
				g_sysparam_st.cursor =2;		
			else			
				g_sysparam_st.cursor =i;					
			self_adaptiveTrigger();//����һ��
		}
	}

	if((time_s !=0) && (g_sysparam_st.timer - time_s >DEPARTURETIMER))
	{
		time_s = 0;
		adjust_pressure_stop();
		adjust_pressure(FLATPRESSURE,TOLERANCE);
		SnoringInterventStateClear();
		departuretrigger_s = 1;
		//��������
	}
	else if((time_s !=0) && (g_sysparam_st.timer - time_s <=DEPARTURETIMER))
	{
		if(g_sysparam_st.humandetection == 1)
		{
			time_s = 0;
			adjust_pressure_stop();
			//�м�⵽�ڴ���ϴ���
		}
	}
	
	if(departuretrigger_s == 1 && g_sysparam_st.airpump.trig == 0)
	{
		departuretrigger_s = 0;
	}
	
	if(g_sysparam_st.humandetection == 0 && g_sysparam_st.airpump.pa_cur <=600)
	{
		g_sysparam_st.airpump.flatFlag = 1;
		departuretrigger_s = 0;
	}
	
	
	oldhumandetection = g_sysparam_st.humandetection;
}

//����Ӧ����
void self_adaptive(void)
{
	static uint8_t oldsleepingPosture;
	
	uint32_t temp = 0;
	static cboxlog_t oldcboxlog;
	
	if((oldcboxlog.small_movecount != g_sysparam_st.cboxlog_st.small_movecount)||
		 (oldcboxlog.big_movecount_unchange != g_sysparam_st.cboxlog_st.big_movecount_unchange))
	{	
			adjust_pressure_Overchargedischarge(g_sysparam_st.airpump.adjPa,TEMP_PA_OVER,TOLERANCE);	
	}
	
	
	if(oldsleepingPosture != g_sysparam_st.sleepingPosture)
	{
		if(g_sysparam_st.sleepingPosture == 0)	
			//adjust_pressure_Overchargedischarge(lieLowbuff[g_sysparam_st.cursor],800,TOLERANCE); //ƽ��
		adjust_pressure_Overchargedischarge(g_sysparam_st.airpump.adjPa,TEMP_PA_OVER,TOLERANCE); //ƽ��
		else		
			adjust_pressure_Overchargedischarge(g_sysparam_st.airpump.adjPa,TEMP_PA_OVER,TOLERANCE); //ƽ��
			//adjust_pressure_Overchargedischarge(liesidebuff[g_sysparam_st.cursor],800,TOLERANCE);//����		
	}

	oldcboxlog = g_sysparam_st.cboxlog_st;	
	oldsleepingPosture = g_sysparam_st.sleepingPosture;
}

void self_adaptiveTrigger(void)
{
		if(g_sysparam_st.sleepingPosture == 0)	
			//adjust_pressure_Overchargedischarge(lieLowbuff[g_sysparam_st.cursor],800,TOLERANCE);//ƽ��
			adjust_pressure_Overchargedischarge(g_sysparam_st.airpump.adjPa,TEMP_PA_OVER,TOLERANCE); //ƽ��		
		else	
			//adjust_pressure_Overchargedischarge(liesidebuff[g_sysparam_st.cursor],800,TOLERANCE);//����
			adjust_pressure_Overchargedischarge(g_sysparam_st.airpump.adjPa+300,TEMP_PA_OVER,TOLERANCE); //ƽ��
}

void adaptivecontrol_default(void)
{
	g_sysparam_st.airpump.adjPa = TEMP_PA;
	g_sysparam_st.ai_adj = 1;
	g_sysparam_st.ai_adj_strength = 2;
}

void adaptivecontrol_run(void)
{
	
	static uint8_t oldstretchTrig;
	
	if(g_sysparam_st.stretch.stretchTrig == 1)
	{
		StretchingModeRun();
	}
	else
	{
		departuredetection();
		if((g_sysparam_st.ai_adj == 1) &&  (g_sysparam_st.snoreIntervention.triging == 0))//ʹ��������Ӧ ͬʱ���ڴ���������
		{			
			self_adaptive();
		}
	}  
	
	if(g_sysparam_st.stretch.stretchTrig != oldstretchTrig && g_sysparam_st.stretch.stretchTrig == 0)//�˳���չģʽ
	{
		StretchingModeStop();
	}
	
	oldstretchTrig = g_sysparam_st.stretch.stretchTrig ;
}

void StretchingModeRun(void)
{
	
	if(g_sysparam_st.stretch.stretchTrig == 1)
	{
		if(g_sysparam_st.stretch.step == 0 )
		{
			adjust_pressure(STRETCH_HIGH,TOLERANCE);
			g_sysparam_st.stretch.step++;
		}
		
		if(g_sysparam_st.stretch.step == 1 && g_sysparam_st.airpump.trig == 0)
		{
			g_sysparam_st.stretch.step++;
		}
		
		if(g_sysparam_st.stretch.step == 2)
		{
			adjust_pressure(STRETCH_LOW,TOLERANCE);
			g_sysparam_st.stretch.step++;
		} 
		
		if(g_sysparam_st.stretch.step == 3 && g_sysparam_st.airpump.trig == 0)
		{
			g_sysparam_st.stretch.step  = 0;
		}	
	}
	
}

void StretchingModeStop(void)
{
	g_sysparam_st.stretch.step = 0;
	g_sysparam_st.stretch.timerOut = 0;
	g_sysparam_st.stretch.stretchTrig = 0;
	adjust_pressure_stop();
}

void StretchingModeStart(void)
{
	g_sysparam_st.stretch.step = 0;
	g_sysparam_st.stretch.timerOut = STRETCH_TIMEROUT;
	g_sysparam_st.stretch.stretchTrig = 1;
}

void x_control_init(void)
{
	g_sysparam_st.cursor = 1;	
	airbag_ctrbase=air_outstop;
}

void app_flatten(void)
{
	adjust_pressure_stop();
	
	SnoringInterventStateClear();
	StretchingModeStop();
	g_sysparam_st.ai_adj = 0;
	adjust_pressure(FLATPRESSURE,TOLERANCE);
}



