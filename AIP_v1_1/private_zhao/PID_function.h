#ifndef __PID_FUNCTION_H
#define __PID_FUNCTION_H

typedef struct {
float Set_temp;//定义设置值  在这里是温度值
float Actual_temp;//定义实际值  在这里是当前实际温度
float err;//定义偏差值
float err_last;//定义上一个偏差值
float Kp,Ki,Kd;//定义比例、积分、微分系数
float voltage;//定义电压值（控制执行器的变量）  在这里需要即为PWM
float integral;//定义积分值
unsigned char first_reach;//0：正常PID控制，1：初始化时设置温度大于当前温度；2：初始化时设置温度大于当前温度
}pid_def;

void PlD_init(float Set_temp,float Actual_temp,pid_def *PID);
float PlD_realize(float Set_temp,float Actual_temp,pid_def *PID);
#endif
