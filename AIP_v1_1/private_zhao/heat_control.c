#include "heat_control.h"
#include "APP_io_init.h"
#include<math.h>
uint8_t control_sta=0;
float pillotmpe=0;//当前温度
float pillosettmpe=0;//目标温度
pid_def pid;
auto_mode_per_def auto_mode_per;

void off_heat(parameter_set *file)
{
	//setfile->time_down=0;
	file->time_n=0;//按键定时切换复位
	file->heat=0;//关闭加热标志
	file->manual_mode.timerfunction=0;//手动模式定时器关
	heat_pwmset(0);//关闭PWM
	control_sta=0;
}

float temperature_get(AD_Value_init *ADfortemp)
{
	float Rt;
	float Rp=10.0;//单位K
	float T2=273.15+25.0;
	float Bx=3950.0;//3435.0
	float Ka=273.15;
	float temp;
	
	//温度计算
	Rt=1.4*(float)ADfortemp->temp_AD.AD_AVG/4095;//转换为电压值 V
	Rt=Rt*51/(3.3-Rt);//转换为NTC阻值 算出来阻值单位为K
	
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
	////////////////控制逻辑/////////////////////////	
if(file->mode==0)//手动模式
{
	if(file->time_down>0)//倒计时大于0时
	{
		switch(control_sta)
		{
			case 0://PID参数初始化
				   PlD_init(file->manual_mode.temperater, pillotmpe,&pid);//初始化PID控制参数
			     pillosettmpe=file->manual_mode.temperater;//手动模式设置温度赋值给控制变量
			     control_sta=1;
			     
				break;
			case 1://PID控制中
				
//					temp=PlD_realize(pillosettmpe,pillotmpe);
//			LOG_I("PWM:%f  settemp:%f temp:%f", temp,pillosettmpe,pillotmpe);
//		  heat_pwmset((uint8_t)temp);//PID调整PWM控制
			
				//LOG_I("STA:%d", control_sta);
				break;
			default:break;
		}
		file->heat=1;//打开加热标志
		
	}
	else//倒计时结束，关闭加热
	{
		off_heat(file);
	}
}
else if(file->mode==1)//自动模式1
{
	if(file->time_down>0)
	{
		switch(control_sta)
		{
			case 0://根据自动模式设置参数计算控制逻辑
			auto_mode_per.temp_num=file->auto1_mode.end_temp-file->auto1_mode.start_temp;
			auto_mode_per.time_d=file->auto1_mode.processtime/auto_mode_per.temp_num;
			control_sta=1;
				break;
			case 1://PID参数初始化
				   pillosettmpe=file->auto1_mode.start_temp;//手动模式设置温度赋值给控制变量

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
					if(auto_mode_per.cyclic_n>=11)//做最大限制
					{
						control_sta=3;
					}
				
				  break;
			case 3://PID控制中
				
//					temp=PlD_realize(pillosettmpe,pillotmpe);
//			LOG_I("PWM:%f  settemp:%f temp:%f", temp,pillosettmpe,pillotmpe);
//		  heat_pwmset((uint8_t)temp);//PID调整PWM控制
			
				//LOG_I("STA:%d", control_sta);
				break;
			default:break;
		}
		//setfile->auto_mode.start_temp>
	}
	else//倒计时结束，关闭加热
	{
		off_heat(file);
	}
 }
 else if(file->mode==2)//自动模式2
 {
	if(file->time_down>0)//倒计时大于0时
	{
		switch(control_sta)
		{
			case 0:
			//PID参数初始化

			     pillosettmpe=file->auto2_mode.temperater;//手动模式设置温度赋值给控制变量
			     control_sta=1;
			break;
			case 1://PID控制中
				break;
			default:break;
		}
		file->heat=1;//打开加热标志
	}
	else//倒计时结束，关闭加热
 {
		off_heat(file);
		
	}
}
}


