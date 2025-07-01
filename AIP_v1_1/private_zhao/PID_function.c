#include "PID_function.h"

//PID参数初始化
void PlD_init(float Set_temp,float Actual_temp,pid_def *PID)
{
PID->Set_temp=0.0;//设定值，在这里是温度值
PID->Actual_temp=0.0;//当前实际值，在这里是当前实际温度
PID->err=0;
PID->err_last=Set_temp-Actual_temp;
PID->voltage=0.0;//控制变量，在这里需要转换为PWM
PID->integral=0.0;//积分值
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

//PIC温度控制  增量式
//输入：Set_temp设置目标温度，Actual_temp当前实际温度
//返回：PWM设置值
float PlD_realize(float Set_temp,float Actual_temp,pid_def *PID)
{
	//当PID控制变量大于255时限制积分

		PID->Set_temp=Set_temp;
		PID->err=PID->Set_temp-Actual_temp;//计算设置温度和实际温度的差值
	
		if(PID->voltage<255&&PID->voltage>0)//限制积分项
			PID->integral+=PID->err;//积分
		else if(PID->voltage>255)
		{
			if(PID->err<0)//允许负积分
			PID->integral+=PID->err;//积分
		}
		else if(PID->voltage<0)
		{
			if(PID->err>0)//允许正积分
			PID->integral+=PID->err;//积分
		}
		
		if(PID->first_reach==1&&Set_temp<Actual_temp)//当前温度首次超过设定温度，积分值减半，重新积分，相当于一个简单的模糊控制
		{
			PID->first_reach=0;//结束模糊控制
			PID->integral=PID->integral*0.5;
		}
		else if(PID->first_reach==2&&Set_temp>Actual_temp)//当前温度首次小于设定温度，积分强制改为80，相当于一个简单的模糊控制
		{
			PID->first_reach=0;
			PID->integral=80;
		}
		
		PID->voltage=PID->Kp*PID->err+PID->Ki*PID->integral+PID->Kd*(PID->err-PID->err_last);
		PID->err_last=PID->err;
	
	if(PID->voltage>255)//限制返回值最大为255
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

