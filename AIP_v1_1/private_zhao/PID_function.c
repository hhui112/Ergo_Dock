#include "PID_function.h"

//PID������ʼ��
void PlD_init(float Set_temp,float Actual_temp,pid_def *PID)
{
PID->Set_temp=0.0;//�趨ֵ�����������¶�ֵ
PID->Actual_temp=0.0;//��ǰʵ��ֵ���������ǵ�ǰʵ���¶�
PID->err=0;
PID->err_last=Set_temp-Actual_temp;
PID->voltage=0.0;//���Ʊ�������������Ҫת��ΪPWM
PID->integral=0.0;//����ֵ
PID->Kp=0.2;
PID->Ki=0.1;//0.015
PID->Kd=0.2;
	
if(Set_temp>Actual_temp)
PID->first_reach=1;
else if(Set_temp<Actual_temp)
	PID->first_reach=2;
else if(Set_temp==Actual_temp)
	PID->first_reach=0;

}

//PIC�¶ȿ���  ����ʽ
//���룺Set_temp����Ŀ���¶ȣ�Actual_temp��ǰʵ���¶�
//���أ�PWM����ֵ
float PlD_realize(float Set_temp,float Actual_temp,pid_def *PID)
{
	//��PID���Ʊ�������255ʱ���ƻ���

		PID->Set_temp=Set_temp;
		PID->err=PID->Set_temp-Actual_temp;//���������¶Ⱥ�ʵ���¶ȵĲ�ֵ
	
		if(PID->voltage<255&&PID->voltage>0)//���ƻ�����
			PID->integral+=PID->err;//����
		else if(PID->voltage>255)
		{
			if(PID->err<0)//��������
			PID->integral+=PID->err;//����
		}
		else if(PID->voltage<0)
		{
			if(PID->err>0)//����������
			PID->integral+=PID->err;//����
		}
		
		if(PID->first_reach==1&&Set_temp<Actual_temp)//��ǰ�¶��״γ����趨�¶ȣ�����ֵ���룬���»��֣��൱��һ���򵥵�ģ������
		{
			PID->first_reach=0;//����ģ������
			PID->integral=PID->integral*0.5;
		}
		else if(PID->first_reach==2&&Set_temp>Actual_temp)//��ǰ�¶��״�С���趨�¶ȣ�����ǿ�Ƹ�Ϊ80���൱��һ���򵥵�ģ������
		{
			PID->first_reach=0;
			PID->integral=80;
		}
		
		PID->voltage=PID->Kp*PID->err+PID->Ki*PID->integral+PID->Kd*(PID->err-PID->err_last);
		PID->err_last=PID->err;
	
	if(PID->voltage>255)//���Ʒ���ֵ���Ϊ255
	{
		Actual_temp=255;
		
	}
	else
	{
		Actual_temp=PID->voltage;
	}
	
	if(Actual_temp<0)
		Actual_temp=0;
	
return Actual_temp;
}

