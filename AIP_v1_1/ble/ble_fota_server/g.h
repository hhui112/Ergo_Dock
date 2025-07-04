#ifndef _G_H
#define _G_H
#include "wfsensor.h"
#include "uart_base.h"
#include "stdint.h"
#include "timer_event.h"
#include "x_control.h"
#include "x_sf.h"
#include "x_ble_com.h"
#include "log.h"
#include "x_sf.h"
#include "ls_hal_trng.h"
#include "x_snoreintervention.h"
#include "x_flash.h"
#include "main.h"
#include "x_drive.h"
#include "x_uart.h"
#include "ls_hal_rtc.h"
#include "x_sleep_report.h"
#include "app_key.h"
#include "app_led.h"

#define VERSION  105


#define MODE_MANUAL  0
#define MODE_ADJ  	 1
#define MODE_STRETCH 2


#define UART_UPDATA 0
#define BLE_UPDATA  1


enum stretchState
{
	STRETCH_OFF,
	STRETCH_NO
	
};


#define AIRPUMP_ON  	0x01
#define AIRPUMP_OFF   0x00

#define UART_TX_BUF_SIZE                256                                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE                256     

#define MOTOR_NUM 	2

// 普通按键键值
#define		KEY_MOTOR_STOP					0x00000000	
#define		KEY_M1_OUT						0x00000001	//头部驱动器伸出；
#define		KEY_M1_IN						0x00000002	//头部驱动器缩进；
#define		KEY_M2_OUT						0x00000004	//脚部驱动器伸出；
#define		KEY_M2_IN						0x00000008	//脚部驱动器缩进；
#define		KEY_M3_OUT						0x00000010	//腰部顶出； 
#define		KEY_M3_IN						0x00000020	//腰部缩进； 
#define		KEY_M4_OUT						0x00000040	
#define		KEY_M4_IN						0x00000080	
#define		KEY_MASSAGE_All					0x00000100	
#define		KEY_MASSAGE_TIMER				0x00000200	
#define		KEY_MASSAGE_FEET				0x00000400		
#define		KEY_MASSAGE_HEAD				0x00000800	
#define		KEY_ZEROG					  	0x00001000	
#define		KEY_MEMORY2						0x00002000	//阅读位置
#define		KEY_MEMORY3						0x00004000	//TV	
#define		KEY_MEMORY4						0x00008000	//打鼾位置
#define		KEY_MEMORY5						0x00010000	//音乐位置
#define		KEY_UBB							0x00020000	
#define		KEY_STRECHMOVE					0x00040000	
#define		KEY_INTENSITY1					0x00080000	
#define		KEY_INTENSITY2					0x00100000	
#define		KEY_INTENSITY3					0x00200000	
#define		KEY_MASSAGE_WAIST				0x00400000	
#define		KEY_MASSAGE_HEAD_MINUS      	0x00800000	
#define		KEY_MASSAGE_FEET_MIUNS	        0x01000000	
#define		KEY_MASSAGE_STOP_ALL	    	0x02000000	
#define		KEY_MASSAGE_MODE				0x04000000	
#define		KEY_ALLFATE						0x08000000	// 所有电机放平
#define		KEY_MEMORY8						0x10000000	
#define		KEY_ANGLEADJUST					0x20000000	
#define		KEY_EXTMEMORY1				    0x40000000	
#define		KEY_EXTMEMORY2					0x80000000	

// 缓启动控制按键键值
#define		KEY_HEAD_LIFT					0x00008000	// 头部抬升
#define		KEY_HEAD_LOWER					0x08000000	// 头部放下
#define		KEY_ALL_MOTORS_LEVEL			0x08000000	// 所有电机放平


//组合键值
#define   KEY_FLAT_ZEROG      		(KEY_ZEROG|KEY_ALLFATE)
#define 	KEY_MASSAGE_LED			(KEY_UBB|KEY_MASSAGE_HEAD_MINUS|KEY_MASSAGE_FEET_MIUNS|KEY_MASSAGE_STOP_ALL|KEY_MASSAGE_MODE|KEY_MASSAGE_All|KEY_MASSAGE_TIMER|KEY_MASSAGE_FEET|KEY_MASSAGE_HEAD)
#define   KEY_SET_AB                (KEY_M1_OUT|KEY_M1_IN|KEY_M2_OUT|KEY_M2_IN)
#define   KEY_BED_UP                (KEY_M4_OUT|KEY_M3_OUT)
#define   KEY_BED_IN                (KEY_M4_IN|KEY_M3_IN)

// 按键类型
#define   KEYS_TYPE_NORMAL          0    // 普通键值
#define   KEYS_TYPE_SOFT_START      1    // 缓启动
#define   KEYS_TYPE_MASSAGE         2    // 按摩枚举

//一键助眠
#define KEY_ZEROG_MASSAGEALL      (KEY_ZEROG|KEY_MASSAGE_All)
#define KEY_ZEROG_MASSAGESTOP     (KEY_ZEROG|KEY_MASSAGE_STOP_ALL)
#define KEY_FLAT_MASSAGESTOP      (KEY_ALLFATE|KEY_MASSAGE_STOP_ALL)


struct asyncCtrlMode_t
{
    uint8_t addr  			: 2;
    uint8_t mode  			: 2;
    uint8_t strechMove	: 2;
    uint8_t music 			: 1;
    uint8_t reserved 		:1;
} __attribute__ ((packed));

union SyncCommunicationData_t
{
	struct
	{
		uint8_t length;
		uint8_t type;
		uint8_t data[UART_RX_BUF_SIZE];
	}__attribute__ ((packed))Syncdata;
	
	struct
	{
		unsigned char length;
		unsigned char type;
		unsigned long keys;
		unsigned char ctrlMode;
		uint8_t checksum;
	} __attribute__ ((packed))PlugInPacket;

	struct
	{
		unsigned char length;    // 7
		unsigned char type;			//0x01
		unsigned long keys;			//键值
		unsigned char ctrlMode;	//预留填0即可
		unsigned char drivePwm;	//占空比 1-255  1 最慢  255 最快
		unsigned char driveTmr;	//时间   0-255   单位秒
		uint8_t checksum;
	} __attribute__ ((packed))btPacket;
	
	struct
	{
			uint8_t length;														//数据长度    总数针长度   length+3(type,checkSum,length)
			uint8_t type;															//命令类型    固定07
			uint32_t keys;														//键值
			uint8_t ledData[5];												//保留
			uint8_t UBB													:	1;	//床底灯状态
			uint8_t stopAll											:	1;	//打断标志位  		  1打断  0正常
			uint8_t automaticMovementIsActive		:	1;	//自动运行标志位  1自动运行  走记忆位置或者flat
			uint8_t sync												:	1;	//保留
			uint8_t lock												:	1;	//保留
			uint8_t angleAdj        						: 1;  //用于缓启动运行标志位  1表示启动了缓期动不可走记忆位置  0 表示可以走记忆位置
			uint8_t factoryMode									: 1;  //工厂模式   1工厂模式 0正常模式
			uint8_t	addr												: 1;  //两个控制盒同步的时候区分两个控制盒的地址
			uint8_t massage_status[2];								//头脚按摩器按摩强度
			uint32_t massageTimer;										//按摩时间			10ms
			uint16_t pulseCounter[MOTOR_NUM];					//驱动器纹波	 	需要结合驱动器数量目前有 2电机 3电机 4电机
			int16_t  current[MOTOR_NUM];							//驱动器电流   需要结合驱动器数量目前有 2电机 3电机 4电机
			uint16_t U_div_2; 												//基准电流
			uint16_t massageCurrent;  								//按摩马达电流
			uint16_t mfpCurrent; 											//灯带电流		
			uint8_t	dummy[2];													//保留
			uint8_t slow_pwm;													//用于缓启动同步
			uint8_t slow_timer;												//用于缓启动同步
			uint8_t	bedtype;													//床的类型
			struct asyncCtrlMode_t asyncCtrlFrame;		//用于左右床
			uint8_t heating;													//加热垫标志位
			uint8_t aromaswitch;											//香薰标志位
			uint8_t rgb;        											//不知道干嘛的
			uint8_t reddata;													//rgb灯占空比 用于rgb灯带   0~255
			uint8_t greendata;												//rgb灯占空比 用于rgb灯带		0~255
			uint8_t bluedata;													//rgb灯占空比	用于rgb灯带	  0~255
			uint8_t massge_mode;											//按摩模式
			uint8_t brightness;												//普通床低灯占空比用于单色灯带  0~255
			uint8_t checkSum;
	} __attribute__ ((packed))syncPacket;

	
	uint8_t rawData[UART_RX_BUF_SIZE];	
};

typedef struct {
    volatile uint8_t rxindex;
    uint32_t timeout;
    union SyncCommunicationData_t rx;  
    union SyncCommunicationData_t tx;  
} mfp_data_st;		// MFP口数据

typedef struct
{
	uint32_t check_err;  			// 串口校验错误次数
	uint32_t check_timerout;  // 串口接收数据超时次数
}MFPDatacheck_t;

// 打鼾干预

typedef struct
{
	uint32_t timerOut;
	uint32_t step;
	uint32_t stretchTrig;
}stretch_t;

typedef struct
{
	uint32_t trig;						//触发打鼾检测
	uint32_t triging;					//打鼾干预触发中
	uint32_t trigTimer;				//检测触发时间			
	uint16_t limit_pa;  			//打鼾最高气压
	
	bool enable ;							//0关闭 1开启
	

	uint32_t triggered_time_s;                 // 干预触发时间
    bool     is_intervening;                        // 打鼾干预中
    bool     triggered_flag;                        // 需要干预
	uint8_t snoreIntervention_pwm;                   // 缓启动速度
  	uint8_t snoreIntervention_tmr;                   // 缓启动时间 
	uint8_t snoreIntervention_threshold;            // 打鼾次数阈值
	
}snoreIntervention_t;

typedef struct
{
	uint8_t  ai_adjNub;
	uint8_t  small_movecount;
	uint32_t big_movecount_change;
	uint32_t big_movecount_unchange;
}cboxlog_t;

typedef struct
{
	uint16_t  version;
	uint16_t  snorevolume;
	uint8_t 	snoreState;
}CI1302_t;


typedef struct
{
	int 	pa_cur;							//当前气压
	int	 	pa_tar;							//目标气压
	int 	oldpa_leave;				//离枕气压
	uint8_t trig;       			//触发根据目标充放气命令  0空闲 1 触发   2 充气  3 放气 4二阶段放气	
	int 	pa_tar_over;				//过充目标气压
	int 	pa_tar_low;					//过放目标气压
	uint8_t trig_over;    		//触发过充标志位
	uint8_t trig_low;     		//触发过充标志为
	uint8_t trig_type;     		//触发类型 0先充 ->放->冲   1->放->充->放
	int 		tolerance;				//误差范围
	uint8_t inflate_onff;			//气磅开关
	uint8_t aspirator_onff;		//抽气磅开关
	uint8_t deflate_onoff;		//气阀开关
	uint8_t faultcode;				//故障代码
	uint8_t flatFlag;   			//放平标志位
	uint8_t Stableflag;  			//气压稳定标志位
	uint16_t adjPa;       		//自适应气压
	float 	temp;
} airpump_t;

//系统参数
typedef struct 
{
	uint32_t	leave_timer;
	float			leave_pa;
	uint32_t 	leave_thresholdvalue;
	uint32_t 	leave_state;
	uint8_t		breathe;  				//呼吸频率
	uint16_t	snoreNub; 				//打鼾次数
	uint16_t	snoreNubperiod; 	//打鼾周期次数
	uint16_t	snoring_score; 		//打鼾分数
	uint8_t		snoreState; 			//打鼾状态
	uint16_t	snorevolume;			//打鼾音量
 	uint16_t	snoreIntervenNub;	//打鼾干预次数
		
	uint16_t	  bigMove;				//大体动
	uint16_t	  smallMove;			//小体动
	uint16_t    ThresholdMove;	//体动阈值	
	uint8_t stabilize;					//气压稳定

}x_sf_st;//系统参数


//系统参数
typedef struct 
{
	uint32_t  		timer;   						//系统时间 10ms 
	uint8_t 			ai_adj;							//自适应调节开关 0普通   1 自适应 
	uint8_t 			ai_adj_strength;		//自适应强度
	uint8_t 			humandetection;			//有无人  0 无人  1 有人
	uint8_t 			true_humandetection;//真实有无人
	uint8_t 			sleepingPosture;		//睡姿监测  0 平躺  1侧躺
	cboxlog_t 		cboxlog_st;					//控制盒日志
	airpump_t 		airpump;						//气囊状态
	uint8_t 			cursor; 						//一个游标指向执行气压的大小
	stretch_t 		stretch;						//模式模式相关参数
	x_sf_st sf;
	snoreIntervention_t snoreIntervention;//打鼾干预
	CI1302_t  ci1302;
	calendar_cal_t calendar_cal;
	calendar_time_t calendar_time;
	uint8_t       AntiSnore_intensity;
}sysparam_st;//系统参数


//故障检出
typedef struct 
{
	uint8_t selfCheck;  							//自检标志位
	uint8_t airPumpFailure;						//气泵故障
	uint8_t gasvalveFailure;					//气阀故障
	uint8_t airPressureSensorFailure;	//气压传感器故障
}faultDetect_st;										//故障检出标志位


//掉电要保存的数据
typedef struct 
{
	uint8_t 			ai_adj;
	uint16_t 			adjPa;       							//自适应气压
	uint8_t 			snoreIntervention_enable;	//打鼾使能标志位
	uint8_t 			ai_adj_strength;					//自适应强度
	uint16_t 			crc;      				 				//自适应气压
}flash_save_data_st;//故障检出标志位

extern void x_time_UTC_ToRTC(uint64_t t_utc);
extern uint32_t x_time_RTC_ToUTC(void);

extern int in_state;           //在离枕状态 0离床 1在床
extern uint16_t Modbus_Crc_Compute(const uint8_t *buf, uint16_t bufLen);
extern sysparam_st g_sysparam_st;
extern uint8_t syncCalcCheckSum(const uint8_t *data, uint8_t len);
#endif