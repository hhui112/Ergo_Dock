#ifndef X_SLEEP_REPORT_H
#define X_SLEEP_REPORT_H

#include "stdint.h"

#define MAXRECORDVOLUM    (12)

#define	SUBCONTRACT_SIZE 	150   //���ݰ���С

#pragma pack(1)

typedef struct
{
	uint8_t trig;
	uint32_t runtime;
}report_sys_t;



typedef struct
{
	uint64_t sleeptimer_come;		//����ʱ��
	uint64_t sleeptimer_leave;	//����ʱ��
	uint16_t sleeptimer_total;	//˯����ʱ��

	uint8_t Snoringindex;				//����ָ��	
	uint16_t snoringNub;				//��������
	uint16_t snoringFre;				//����Ƶ��
	uint16_t snoringTime;				//����ʱ��
	uint16_t snoringVolume;			//ƽ������ǿ��
	
	uint16_t interventionsNub;	//��Ԥ����
	
	uint16_t tidongNub;					//�嶯����
	uint16_t adjNubl;						//����Ӧ���ڴ���
	
	uint16_t tidongSmallNub;		//С�嶯����
	uint16_t tidongBigNub;			//���嶯����
	uint16_t tidongFre;					//�嶯Ƶ��
	uint16_t tidongInterval;		//�嶯���
	
	uint16_t snoringVolumeNub;	
	uint8_t snoringVolumebuff[MAXRECORDVOLUM*60]; //һ���Ӽ�¼һ�� ���13Сʱ
	
	uint16_t tidongRecordNub;
	uint8_t tidongRecord[180];//λ����  1���Ӽ�¼һ��
	
	uint16_t adjRecordNub;
	uint8_t adjRecord[180];//λ����  1���Ӽ�¼һ��
	
	uint16_t snoringinterveneNub;
	uint8_t snoringinterveneRecord[180];//λ����  1���Ӽ�¼һ��
}report_t;

extern report_t  report;

#pragma pack()

void x_report_get(uint8_t type);
void x_report_get_snoringVolume(uint8_t type);
void x_report_get_tidongRecord(uint8_t type);
void x_report_get_adjRecord(uint8_t type);
void x_report_get_snoringinterveneRecord(uint8_t type);
void x_report_run(void);
void x_report_time_10ms(void);
#endif