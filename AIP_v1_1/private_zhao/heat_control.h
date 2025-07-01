#ifndef __HEAT_CONTROL_H
#define __HEAT_CONTROL_H
#include "timer_event.h"
#include "PID_function.h"
#include "stdint.h"

//�Զ�ģʽ1���Ʋ�������
typedef struct {
	uint8_t temp_num;
	uint16_t time_d;
	uint8_t cyclic_n;
}auto_mode_per_def;

typedef struct {
	uint8_t  temperater;//�ֶ�ģʽ��Ŀ���¶�����
	uint8_t  timerfunction:1;//��ʱ���ܿ��أ�0�ض�ʱ��1����ʱ
	uint16_t  timeset;//��ʱʱ������
}__attribute__((packed))manual_init;//�ֶ�ģʽ��������

typedef struct {
	uint8_t  start_temp;//��ʼ�¶�
	uint8_t  end_temp;//�����¶�
	uint16_t processtime;//����ʱ��
}__attribute__((packed))auto1_init;//�Զ�ģʽ��������

typedef struct {
	uint8_t  temperater;//�¶�����
	uint16_t processtime;//����ʱ��
}__attribute__((packed))auto2_init;//�Զ�ģʽ��������

typedef struct {
	uint8_t app_B;
	uint8_t app_G;
	uint8_t app_R;
}__attribute__((packed))APPctr_ligth_set_init;//APP����ʱ�����������ɫ����

typedef struct {
	uint8_t time_B;//
	uint8_t time_G;//
	uint8_t time_R;//
	uint16_t time_set;//��ʱʱ��
	uint8_t temp;//�¶�
}__attribute__((packed))keyctr_ligth_time_set_init;//

typedef struct {
	uint8_t programV[3];//����汾
	uint8_t mode:4;//ģʽ��0�ֶ�ģʽ��1�Զ�ģʽ1��2�Զ�ģʽ2��
	uint8_t heat:1;//���ȿ��أ�0�أ�1��
	uint8_t time_n:3;//��ǰ�����л���ʱ��¼
	uint8_t temperature;//��ǰʵʱ�¶ȣ����϶�
	uint16_t I_heat;//��ǰ���Ȳ�����
	uint16_t time_down;//����ʱʵʱ����
	manual_init manual_mode;//�ֶ�ģʽ�²�������
	auto1_init auto1_mode;//�Զ�ģʽ1�²�������
	auto2_init auto2_mode;//�Զ�ģʽ2�²�������
	APPctr_ligth_set_init APPctr_ligth_set;
	keyctr_ligth_time_set_init keyctr_ligth_time_set[3];
}__attribute__((packed))parameter_set;	

extern uint8_t control_sta;
extern float pillotmpe;//��ǰ�¶�
extern float pillosettmpe;//Ŀ���¶�
extern pid_def pid;

float temperature_get(AD_Value_init *ADfortemp);
void temperature_ctr(parameter_set *file);
void off_heat(parameter_set *file);
#endif
