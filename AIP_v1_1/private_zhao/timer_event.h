#ifndef __TIMER_EVENT_H
#define __TIMER_EVENT_H
#include "stdint.h"

#include "builtin_timer.h"
#include "ls_hal_adc.h"
#include "ls_hal_timer.h"
#include "ls_hal_adc.h"
#include "ls_hal_dmac.h"
#include "ble_base_set.h"
#include "Judger.h"

#define pingtang_pressset  1200//ƽ��ʱ����������ѹ
#define cetang_pressset  2000//����ʱ����������ѹ
#define  pump_type 1//0 ��ͨ����  1 ѹ�羲������
enum ADC_DMA_NUM
{
	ADCpress_R=0X00,
	ADCvalve_I,
	ADCpump_I,
	ADCheat_I,
	ADCtemp_I,
};

/*
VX.Y.Z
X:��������ش�ı䵼�¶ϴ�,ǰ�󲻿ɼ���
Y:Ӳ���Ƿ�����ж�,���һ��˵��Ӳ�����Լ���,�����������
Z:�����б����BUG�޸���
*/
#define programVdef  "V1.0.0"      
#define programVdef_X    1
#define programVdef_Y    0
#define programVdef_Z    0

typedef struct
{
	uint8_t ble_data_receive[50];
	uint8_t lengh;
}ble_data_init;

typedef struct {
	uint16_t NEWAD;
	uint16_t AD_AVG;
}ADValue_def;

typedef struct {
	ADValue_def temp_AD;
	ADValue_def Iheat_AD;
	ADValue_def pump_AD;
	ADValue_def PRESS_AD;
	ADValue_def valve_AD;
}AD_Value_init;

//__attribute__((packed))�����Ǹ��߱�����������ʵ��ռ���ֽ������ж��룬ȡ���ṹ�ڱ�������е��Ż����롣

//���ҿ����������нṹ��
typedef struct {
	uint8_t programV[3];//����汾
	uint8_t flag;//�����־
	uint16_t airpressure;//��ǰ��ѹֵ
	uint16_t bump_I;//���õ�ǰ����
	uint16_t valve_I;//������ǰ����
	uint8_t postureflag;//0:ƽ��  1������
	uint16_t snore_n;//����������¼����Ӣ̩����Ƶ����
	uint8_t snore_sta;//������ǰ״̬����Ӣ̩����Ƶ����
}__attribute__((packed))airbag_parameter_def;//���ҿ��Ʋ�������


//ѹ��̺���ݽṹ��
#define clothpress_ch_max 16
typedef struct {
	uint8_t read_flag;//AD��ȡ��־��ÿ��ͨ���л���Ҫһ����ʱ��ͨ���ñ�־�ж϶�ȡ���ݵ�ʱ��
	uint8_t index;
	uint16_t value[clothpress_ch_max];
	uint16_t lastvalue[clothpress_ch_max];
}__attribute__((packed))clothpress_def;


typedef enum 
{
	air_outstop=0,
	base_state1,
	base_state2,
	air_outstart,
	base_state3,
	air_outing,
	air_inputstart,
	base_state5,
	base_state6,
	air_inputing,
	air_inputstop,
	base_state8,
	base_state9,
	base_state10,
}airbag_ctrbase_def;

typedef struct
{
	uint8_t been_sta;//����������״̬
	uint8_t been_n;//��������Ĵ���
	uint8_t been_tup;//����������ʱ�����
	uint8_t been_on_t;//��������ʱ��
	uint8_t been_off_t;//�������ر�ʱ��
	uint8_t PWM_duty;//ռ�ձ�
	uint8_t PWM_cycle;//����
	uint8_t PWM_upcount;//PWM�жϼ���ֵ
}been_ctr_def;



extern uint16_t snore_timevoer;//ֹͣ������ʱ��ʱ��
extern airbag_ctrbase_def airbag_ctrbase;
extern been_ctr_def been_ctr;
extern ble_data_init ble_data_receive;
extern  uint16_t adddd;
extern airbag_parameter_def *airbagsetfile;
extern void ls_even_timer_init(void);
extern struct builtin_timer *xTimer_5ms_inst; 
extern struct builtin_timer *xTimer_10ms_inst; 
extern struct builtin_timer *xTimer_100ms_inst; 
extern struct builtin_timer *xTimer_1000ms_inst; 
void parameter_init(void);
void app_data_receive_manage(ble_data_init *p);

#endif
