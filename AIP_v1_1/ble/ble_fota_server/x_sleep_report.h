#ifndef X_SLEEP_REPORT_H
#define X_SLEEP_REPORT_H

#include "stdint.h"

#define MAXRECORDVOLUM    (12)

#define	SUBCONTRACT_SIZE 	150   //数据包大小

#pragma pack(1)

typedef struct
{
	uint8_t trig;
	uint32_t runtime;
}report_sys_t;



typedef struct
{
	uint64_t sleeptimer_come;		//躺下时刻
	uint64_t sleeptimer_leave;	//离枕时刻
	uint16_t sleeptimer_total;	//睡眠总时长

	uint8_t Snoringindex;				//鼾声指数	
	uint16_t snoringNub;				//打鼾次数
	uint16_t snoringFre;				//打鼾频率
	uint16_t snoringTime;				//打鼾时长
	uint16_t snoringVolume;			//平均鼾声强度
	
	uint16_t interventionsNub;	//干预次数
	
	uint16_t tidongNub;					//体动次数
	uint16_t adjNubl;						//自适应调节次数
	
	uint16_t tidongSmallNub;		//小体动次数
	uint16_t tidongBigNub;			//大体动次数
	uint16_t tidongFre;					//体动频率
	uint16_t tidongInterval;		//体动间隔
	
	uint16_t snoringVolumeNub;	
	uint8_t snoringVolumebuff[MAXRECORDVOLUM*60]; //一分钟记录一次 最大13小时
	
	uint16_t tidongRecordNub;
	uint8_t tidongRecord[180];//位计算  1分钟记录一次
	
	uint16_t adjRecordNub;
	uint8_t adjRecord[180];//位计算  1分钟记录一次
	
	uint16_t snoringinterveneNub;
	uint8_t snoringinterveneRecord[180];//位计算  1分钟记录一次
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