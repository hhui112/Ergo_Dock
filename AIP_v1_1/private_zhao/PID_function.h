#ifndef __PID_FUNCTION_H
#define __PID_FUNCTION_H

typedef struct {
float Set_temp;//��������ֵ  ���������¶�ֵ
float Actual_temp;//����ʵ��ֵ  �������ǵ�ǰʵ���¶�
float err;//����ƫ��ֵ
float err_last;//������һ��ƫ��ֵ
float Kp,Ki,Kd;//������������֡�΢��ϵ��
float voltage;//�����ѹֵ������ִ�����ı�����  ��������Ҫ��ΪPWM
float integral;//�������ֵ
unsigned char first_reach;//0������PID���ƣ�1����ʼ��ʱ�����¶ȴ��ڵ�ǰ�¶ȣ�2����ʼ��ʱ�����¶ȴ��ڵ�ǰ�¶�
}pid_def;

void PlD_init(float Set_temp,float Actual_temp,pid_def *PID);
float PlD_realize(float Set_temp,float Actual_temp,pid_def *PID);
#endif
