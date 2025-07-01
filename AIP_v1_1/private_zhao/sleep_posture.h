#ifndef _SLEEP_POSTURE_H
#define _SLEEP_POSTURE_H
#include "stdint.h"

#define  Dpp_press  100//���˶�̬��ѹ����ֵ
#define  posture_judge  150//˯���ж���ֵ
#define  pressdatanum  20

typedef enum 
{
	off=0,
	on,
}onoff_def;
//˯���㷨���ݽṹ��
typedef struct
{
	uint8_t openflag:1;//1:���㷨  0�ر��㷨  �ڳ�������������н���ر��㷨
	uint8_t reinit_flag:1;//1:�ϴξ�̬��ѹƽ��ֵ��press_average[0]�����³�ʼ����־�������������������йر��㷨������ǰ����ѹ�仯�����˯������
	uint8_t data_index;//��ѹ�ɼ����ݻ�������
	uint16_t data[pressdatanum];//��ѹ���ݻ�������
	uint16_t press_average[2];//ȥ����̬��ѹ����������е���ѹ�����ƽ����ѹ����������Խ��ǰ����Խ��
	uint8_t posture;//�����ж�˯�˽��  0:ƽ��   1������
	uint8_t posture_last;//�ϴ��жϽ����¼
}__attribute__((packed))sleepposture_def;



#endif
