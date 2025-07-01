#include "heat_control.h"
#include "APP_io_init.h"
#include<math.h>
uint8_t control_sta=0;
float pillotmpe=0;//��ǰ�¶�
float pillosettmpe=0;//Ŀ���¶�
pid_def pid;
auto_mode_per_def auto_mode_per;

void off_heat(parameter_set *file)
{
	//setfile->time_down=0;
	file->time_n=0;//������ʱ�л���λ
	file->heat=0;//�رռ��ȱ�־
	file->manual_mode.timerfunction=0;//�ֶ�ģʽ��ʱ����
	heat_pwmset(0);//�ر�PWM
	control_sta=0;
}

float temperature_get(AD_Value_init *ADfortemp)
{
	float Rt;
	float Rp=10.0;//��λK
	float T2=273.15+25.0;
	float Bx=3950.0;//3435.0
	float Ka=273.15;
	float temp;
	
	//�¶ȼ���
	Rt=1.4*(float)ADfortemp->temp_AD.AD_AVG/4095;//ת��Ϊ��ѹֵ V
	Rt=Rt*51/(3.3-Rt);//ת��ΪNTC��ֵ �������ֵ��λΪK
	
	//like this R=5000,T2=273.15+25,B=3470,RT=5000*EXP(3470*(1/T1-1/(273.15+25)),
	temp = Rt/Rp;
	temp = log(temp);//ln(Rt/Rp)
	temp/=Bx;//Ln(Rt/Rp)/B
	temp+=(1/T2);
	temp = 1/(temp);
	temp-=Ka;
	
	return temp;
}

void temperature_ctr(parameter_set *file)
{
	uint16_t data16=0;
	////////////////�����߼�/////////////////////////	
if(file->mode==0)//�ֶ�ģʽ
{
	if(file->time_down>0)//����ʱ����0ʱ
	{
		switch(control_sta)
		{
			case 0://PID������ʼ��
				   PlD_init(file->manual_mode.temperater, pillotmpe,&pid);//��ʼ��PID���Ʋ���
			     pillosettmpe=file->manual_mode.temperater;//�ֶ�ģʽ�����¶ȸ�ֵ�����Ʊ���
			     control_sta=1;
			     
				break;
			case 1://PID������
				
//					temp=PlD_realize(pillosettmpe,pillotmpe);
//			LOG_I("PWM:%f  settemp:%f temp:%f", temp,pillosettmpe,pillotmpe);
//		  heat_pwmset((uint8_t)temp);//PID����PWM����
			
				//LOG_I("STA:%d", control_sta);
				break;
			default:break;
		}
		file->heat=1;//�򿪼��ȱ�־
		
	}
	else//����ʱ�������رռ���
	{
		off_heat(file);
	}
}
else if(file->mode==1)//�Զ�ģʽ1
{
	if(file->time_down>0)
	{
		switch(control_sta)
		{
			case 0://�����Զ�ģʽ���ò�����������߼�
			auto_mode_per.temp_num=file->auto1_mode.end_temp-file->auto1_mode.start_temp;
			auto_mode_per.time_d=file->auto1_mode.processtime/auto_mode_per.temp_num;
			control_sta=1;
				break;
			case 1://PID������ʼ��
				   pillosettmpe=file->auto1_mode.start_temp;//�ֶ�ģʽ�����¶ȸ�ֵ�����Ʊ���

			     auto_mode_per.cyclic_n=1;
			     control_sta=2;
				break;
			case 2:
				   data16=file->auto1_mode.processtime-file->time_down;
					if(data16==(auto_mode_per.cyclic_n*auto_mode_per.time_d))
					{
						pillosettmpe++;
						auto_mode_per.cyclic_n++;
					}
					if(auto_mode_per.cyclic_n>=11)//���������
					{
						control_sta=3;
					}
				
				  break;
			case 3://PID������
				
//					temp=PlD_realize(pillosettmpe,pillotmpe);
//			LOG_I("PWM:%f  settemp:%f temp:%f", temp,pillosettmpe,pillotmpe);
//		  heat_pwmset((uint8_t)temp);//PID����PWM����
			
				//LOG_I("STA:%d", control_sta);
				break;
			default:break;
		}
		//setfile->auto_mode.start_temp>
	}
	else//����ʱ�������رռ���
	{
		off_heat(file);
	}
 }
 else if(file->mode==2)//�Զ�ģʽ2
 {
	if(file->time_down>0)//����ʱ����0ʱ
	{
		switch(control_sta)
		{
			case 0:
			//PID������ʼ��

			     pillosettmpe=file->auto2_mode.temperater;//�ֶ�ģʽ�����¶ȸ�ֵ�����Ʊ���
			     control_sta=1;
			break;
			case 1://PID������
				break;
			default:break;
		}
		file->heat=1;//�򿪼��ȱ�־
	}
	else//����ʱ�������رռ���
 {
		off_heat(file);
		
	}
}
}


