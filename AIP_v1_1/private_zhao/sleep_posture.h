#ifndef _SLEEP_POSTURE_H
#define _SLEEP_POSTURE_H
#include "stdint.h"

#define  Dpp_press  100//过滤动态气压的阈值
#define  posture_judge  150//睡姿判断阈值
#define  pressdatanum  20

typedef enum 
{
	off=0,
	on,
}onoff_def;
//睡姿算法数据结构体
typedef struct
{
	uint8_t openflag:1;//1:打开算法  0关闭算法  在充气或放气过程中建议关闭算法
	uint8_t reinit_flag:1;//1:上次静态气压平均值（press_average[0]）重新初始化标志，解决充气或放气过程中关闭算法，导致前后气压变化引起的睡姿误判
	uint8_t data_index;//气压采集数据缓存索引
	uint16_t data[pressdatanum];//气压数据缓存数组
	uint16_t press_average[2];//去除动态气压（翻身过程中的气压）后的平均气压，数组数据越靠前数据越老
	uint8_t posture;//最终判断睡姿结果  0:平躺   1：侧躺
	uint8_t posture_last;//上次判断结果记录
}__attribute__((packed))sleepposture_def;



#endif
