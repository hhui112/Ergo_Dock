#ifndef	_WFSensor_H_
#define	_WFSensor_H_

typedef   signed          char s8;
typedef   signed short     int s16;
typedef   signed           int s32;
//typedef   signed       __INT64 s64;

typedef unsigned          char u8;
typedef unsigned short     int u16;
typedef unsigned           int u32;
//typedef unsigned       __INT64 u64;
 

void WFSensor_WriteByte(u8 addr,u8 Data);
u8 WFSensor_ReadByte(u8 addr);

void WFSensor_indicateGroupConvert(void);
void WFSensor_getTPData(void);
void WFSensor_indicateOneByOneConvert(void);
//void calculatePress(float Press_Data);
float calculatePress(void);//ÆøÑ¹»»Ëã
float calculatetemp(void);//ÎÂ¶È»»Ëã
void get_decData(void);
void getwfData(void);
#endif //_WFSensor_H_
