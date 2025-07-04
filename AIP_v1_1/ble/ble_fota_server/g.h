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

// ��ͨ������ֵ
#define		KEY_MOTOR_STOP					0x00000000	
#define		KEY_M1_OUT						0x00000001	//ͷ�������������
#define		KEY_M1_IN						0x00000002	//ͷ��������������
#define		KEY_M2_OUT						0x00000004	//�Ų������������
#define		KEY_M2_IN						0x00000008	//�Ų�������������
#define		KEY_M3_OUT						0x00000010	//���������� 
#define		KEY_M3_IN						0x00000020	//���������� 
#define		KEY_M4_OUT						0x00000040	
#define		KEY_M4_IN						0x00000080	
#define		KEY_MASSAGE_All					0x00000100	
#define		KEY_MASSAGE_TIMER				0x00000200	
#define		KEY_MASSAGE_FEET				0x00000400		
#define		KEY_MASSAGE_HEAD				0x00000800	
#define		KEY_ZEROG					  	0x00001000	
#define		KEY_MEMORY2						0x00002000	//�Ķ�λ��
#define		KEY_MEMORY3						0x00004000	//TV	
#define		KEY_MEMORY4						0x00008000	//����λ��
#define		KEY_MEMORY5						0x00010000	//����λ��
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
#define		KEY_ALLFATE						0x08000000	// ���е����ƽ
#define		KEY_MEMORY8						0x10000000	
#define		KEY_ANGLEADJUST					0x20000000	
#define		KEY_EXTMEMORY1				    0x40000000	
#define		KEY_EXTMEMORY2					0x80000000	

// ���������ư�����ֵ
#define		KEY_HEAD_LIFT					0x00008000	// ͷ��̧��
#define		KEY_HEAD_LOWER					0x08000000	// ͷ������
#define		KEY_ALL_MOTORS_LEVEL			0x08000000	// ���е����ƽ


//��ϼ�ֵ
#define   KEY_FLAT_ZEROG      		(KEY_ZEROG|KEY_ALLFATE)
#define 	KEY_MASSAGE_LED			(KEY_UBB|KEY_MASSAGE_HEAD_MINUS|KEY_MASSAGE_FEET_MIUNS|KEY_MASSAGE_STOP_ALL|KEY_MASSAGE_MODE|KEY_MASSAGE_All|KEY_MASSAGE_TIMER|KEY_MASSAGE_FEET|KEY_MASSAGE_HEAD)
#define   KEY_SET_AB                (KEY_M1_OUT|KEY_M1_IN|KEY_M2_OUT|KEY_M2_IN)
#define   KEY_BED_UP                (KEY_M4_OUT|KEY_M3_OUT)
#define   KEY_BED_IN                (KEY_M4_IN|KEY_M3_IN)

// ��������
#define   KEYS_TYPE_NORMAL          0    // ��ͨ��ֵ
#define   KEYS_TYPE_SOFT_START      1    // ������
#define   KEYS_TYPE_MASSAGE         2    // ��Ħö��

//һ������
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
		unsigned long keys;			//��ֵ
		unsigned char ctrlMode;	//Ԥ����0����
		unsigned char drivePwm;	//ռ�ձ� 1-255  1 ����  255 ���
		unsigned char driveTmr;	//ʱ��   0-255   ��λ��
		uint8_t checksum;
	} __attribute__ ((packed))btPacket;
	
	struct
	{
			uint8_t length;														//���ݳ���    �����볤��   length+3(type,checkSum,length)
			uint8_t type;															//��������    �̶�07
			uint32_t keys;														//��ֵ
			uint8_t ledData[5];												//����
			uint8_t UBB													:	1;	//���׵�״̬
			uint8_t stopAll											:	1;	//��ϱ�־λ  		  1���  0����
			uint8_t automaticMovementIsActive		:	1;	//�Զ����б�־λ  1�Զ�����  �߼���λ�û���flat
			uint8_t sync												:	1;	//����
			uint8_t lock												:	1;	//����
			uint8_t angleAdj        						: 1;  //���ڻ��������б�־λ  1��ʾ�����˻��ڶ������߼���λ��  0 ��ʾ�����߼���λ��
			uint8_t factoryMode									: 1;  //����ģʽ   1����ģʽ 0����ģʽ
			uint8_t	addr												: 1;  //�������ƺ�ͬ����ʱ�������������ƺеĵ�ַ
			uint8_t massage_status[2];								//ͷ�Ű�Ħ����Ħǿ��
			uint32_t massageTimer;										//��Ħʱ��			10ms
			uint16_t pulseCounter[MOTOR_NUM];					//�������Ʋ�	 	��Ҫ�������������Ŀǰ�� 2��� 3��� 4���
			int16_t  current[MOTOR_NUM];							//����������   ��Ҫ�������������Ŀǰ�� 2��� 3��� 4���
			uint16_t U_div_2; 												//��׼����
			uint16_t massageCurrent;  								//��Ħ������
			uint16_t mfpCurrent; 											//�ƴ�����		
			uint8_t	dummy[2];													//����
			uint8_t slow_pwm;													//���ڻ�����ͬ��
			uint8_t slow_timer;												//���ڻ�����ͬ��
			uint8_t	bedtype;													//��������
			struct asyncCtrlMode_t asyncCtrlFrame;		//�������Ҵ�
			uint8_t heating;													//���ȵ��־λ
			uint8_t aromaswitch;											//��޹��־λ
			uint8_t rgb;        											//��֪�������
			uint8_t reddata;													//rgb��ռ�ձ� ����rgb�ƴ�   0~255
			uint8_t greendata;												//rgb��ռ�ձ� ����rgb�ƴ�		0~255
			uint8_t bluedata;													//rgb��ռ�ձ�	����rgb�ƴ�	  0~255
			uint8_t massge_mode;											//��Ħģʽ
			uint8_t brightness;												//��ͨ���͵�ռ�ձ����ڵ�ɫ�ƴ�  0~255
			uint8_t checkSum;
	} __attribute__ ((packed))syncPacket;

	
	uint8_t rawData[UART_RX_BUF_SIZE];	
};

typedef struct {
    volatile uint8_t rxindex;
    uint32_t timeout;
    union SyncCommunicationData_t rx;  
    union SyncCommunicationData_t tx;  
} mfp_data_st;		// MFP������

typedef struct
{
	uint32_t check_err;  			// ����У��������
	uint32_t check_timerout;  // ���ڽ������ݳ�ʱ����
}MFPDatacheck_t;

// ������Ԥ

typedef struct
{
	uint32_t timerOut;
	uint32_t step;
	uint32_t stretchTrig;
}stretch_t;

typedef struct
{
	uint32_t trig;						//�����������
	uint32_t triging;					//������Ԥ������
	uint32_t trigTimer;				//��ⴥ��ʱ��			
	uint16_t limit_pa;  			//���������ѹ
	
	bool enable ;							//0�ر� 1����
	

	uint32_t triggered_time_s;                 // ��Ԥ����ʱ��
    bool     is_intervening;                        // ������Ԥ��
    bool     triggered_flag;                        // ��Ҫ��Ԥ
	uint8_t snoreIntervention_pwm;                   // �������ٶ�
  	uint8_t snoreIntervention_tmr;                   // ������ʱ�� 
	uint8_t snoreIntervention_threshold;            // ����������ֵ
	
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
	int 	pa_cur;							//��ǰ��ѹ
	int	 	pa_tar;							//Ŀ����ѹ
	int 	oldpa_leave;				//������ѹ
	uint8_t trig;       			//��������Ŀ����������  0���� 1 ����   2 ����  3 ���� 4���׶η���	
	int 	pa_tar_over;				//����Ŀ����ѹ
	int 	pa_tar_low;					//����Ŀ����ѹ
	uint8_t trig_over;    		//���������־λ
	uint8_t trig_low;     		//���������־Ϊ
	uint8_t trig_type;     		//�������� 0�ȳ� ->��->��   1->��->��->��
	int 		tolerance;				//��Χ
	uint8_t inflate_onff;			//��������
	uint8_t aspirator_onff;		//����������
	uint8_t deflate_onoff;		//��������
	uint8_t faultcode;				//���ϴ���
	uint8_t flatFlag;   			//��ƽ��־λ
	uint8_t Stableflag;  			//��ѹ�ȶ���־λ
	uint16_t adjPa;       		//����Ӧ��ѹ
	float 	temp;
} airpump_t;

//ϵͳ����
typedef struct 
{
	uint32_t	leave_timer;
	float			leave_pa;
	uint32_t 	leave_thresholdvalue;
	uint32_t 	leave_state;
	uint8_t		breathe;  				//����Ƶ��
	uint16_t	snoreNub; 				//��������
	uint16_t	snoreNubperiod; 	//�������ڴ���
	uint16_t	snoring_score; 		//��������
	uint8_t		snoreState; 			//����״̬
	uint16_t	snorevolume;			//��������
 	uint16_t	snoreIntervenNub;	//������Ԥ����
		
	uint16_t	  bigMove;				//���嶯
	uint16_t	  smallMove;			//С�嶯
	uint16_t    ThresholdMove;	//�嶯��ֵ	
	uint8_t stabilize;					//��ѹ�ȶ�

}x_sf_st;//ϵͳ����


//ϵͳ����
typedef struct 
{
	uint32_t  		timer;   						//ϵͳʱ�� 10ms 
	uint8_t 			ai_adj;							//����Ӧ���ڿ��� 0��ͨ   1 ����Ӧ 
	uint8_t 			ai_adj_strength;		//����Ӧǿ��
	uint8_t 			humandetection;			//������  0 ����  1 ����
	uint8_t 			true_humandetection;//��ʵ������
	uint8_t 			sleepingPosture;		//˯�˼��  0 ƽ��  1����
	cboxlog_t 		cboxlog_st;					//���ƺ���־
	airpump_t 		airpump;						//����״̬
	uint8_t 			cursor; 						//һ���α�ָ��ִ����ѹ�Ĵ�С
	stretch_t 		stretch;						//ģʽģʽ��ز���
	x_sf_st sf;
	snoreIntervention_t snoreIntervention;//������Ԥ
	CI1302_t  ci1302;
	calendar_cal_t calendar_cal;
	calendar_time_t calendar_time;
	uint8_t       AntiSnore_intensity;
}sysparam_st;//ϵͳ����


//���ϼ��
typedef struct 
{
	uint8_t selfCheck;  							//�Լ��־λ
	uint8_t airPumpFailure;						//���ù���
	uint8_t gasvalveFailure;					//��������
	uint8_t airPressureSensorFailure;	//��ѹ����������
}faultDetect_st;										//���ϼ����־λ


//����Ҫ���������
typedef struct 
{
	uint8_t 			ai_adj;
	uint16_t 			adjPa;       							//����Ӧ��ѹ
	uint8_t 			snoreIntervention_enable;	//����ʹ�ܱ�־λ
	uint8_t 			ai_adj_strength;					//����Ӧǿ��
	uint16_t 			crc;      				 				//����Ӧ��ѹ
}flash_save_data_st;//���ϼ����־λ

extern void x_time_UTC_ToRTC(uint64_t t_utc);
extern uint32_t x_time_RTC_ToUTC(void);

extern int in_state;           //������״̬ 0�봲 1�ڴ�
extern uint16_t Modbus_Crc_Compute(const uint8_t *buf, uint16_t bufLen);
extern sysparam_st g_sysparam_st;
extern uint8_t syncCalcCheckSum(const uint8_t *data, uint8_t len);
#endif