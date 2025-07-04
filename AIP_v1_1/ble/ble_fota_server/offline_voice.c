#include "x_faultDetect.h"
#include "offline_voice.h"
#include "mfp_queue.h"
#include "g.h"

void offline_voice_wake_up(void){
	LOG_I("offline_voice_wake_up \r\n");
	app_Receive_Wakeup_LedOn();
} 

void offline_voice_wake_off(void){
	LOG_I("offline_voice_wake_off \r\n");
	g_offline_voice.enabled = false;
	app_NotReceive_LedFlash();
} 



void offline_voice_dataHandle(uint8_t cmd)
{
	/* 2�����������ж�ָ�� */
		if(cmd == 0x21 || cmd == 0x22) return;
		app_ReceiveCommand_LedOn();

			switch (cmd) 
			{
					case 0x21:
							break;
					case 0x22:
							break;
					case 0x23:		// STOP
							mfp_tx_queue_clear();
							prepare_mfp_NORMAL_KET(KEY_MOTOR_STOP,3); 
							break;
					case 0x24:		// All Up
							prepare_mfp_NORMAL_KET((KEY_M1_OUT|KEY_M2_OUT),30); 	//  ͷ��̧��6s
							break;
					case 0x25:		// Zero G
							prepare_mfp_NORMAL_KET(KEY_FLAT_ZEROG,3); 
							break;
					case 0x26:		// Flat Preset
							prepare_mfp_NORMAL_KET(KEY_ALLFATE,3); 
							break;				
					case 0x27:		// Favorite preset  // ����λ��
							prepare_mfp_NORMAL_KET(KEY_MEMORY5,3); 
							break;
					case 0x28:		// Tv preset
							prepare_mfp_NORMAL_KET(KEY_MEMORY3,3); 
							break;
					case 0x29:		// Raise-head
							prepare_mfp_NORMAL_KET(KEY_M1_OUT,15); // ͷ̧��3s
							break;
					case 0x2A:		// Lower-head
							prepare_mfp_NORMAL_KET(KEY_M1_IN,15);
							break;
					case 0x2B:		// Raise-foot
							prepare_mfp_NORMAL_KET(KEY_M2_OUT,15);
							break;
					case 0x2C:		// Lower foot
							prepare_mfp_NORMAL_KET(KEY_M2_IN,15);
							break;	
					case 0x2D:		// Massage Low
							prepare_mfp_NORMAL_KET(KEY_MASSAGE_LOW,3);
							break;
					case 0x2E:		// Massage Medium
							prepare_mfp_NORMAL_KET(KEY_MASSAGE_MEDIUM,3);
							break;
					case 0x2F:		// Massage High
							prepare_mfp_NORMAL_KET(KEY_MASSAGE_HIGH,3);
							break;
					case 0x30:		// MASSAGE OFF   
							prepare_mfp_NORMAL_KET(KEY_MASSAGE_STOP_ALL,3);
							break;
					case 0x31:		// LIGHT OFF
						if(g_offline_voice.ubb_enable == true)  prepare_mfp_NORMAL_KET(KEY_UBB,3);
							break;
					case 0x32:		// LIGHT On
							if(g_offline_voice.ubb_enable == false) prepare_mfp_NORMAL_KET(KEY_UBB,3);		// UBB�ر�ʱ��ſ���   ״̬���ñ���(MFP״̬һֱ�ش���)
							break;
					default:
							LOG_I("Invalid command \r\n");
		}
}

typedef enum {
    VOICE_STATE_DISABLED,
    VOICE_STATE_WAKE_WORD_DETECTED,
    VOICE_STATE_ACTIVE
} VoiceState;

void offline_voice_Handle(uint8_t cmd, uint8_t data) 
{
    static VoiceState state = VOICE_STATE_DISABLED;
    /* 0����鰴��״̬ */
		check_offline_voice_keys();
	
		/* 1�����������������û��ʹ�ܣ�ֱ���˳� */
    if(g_offline_voice.key_enable == false) return;
	
		LOG_I( "key_enable = %d, wake_word = %d,cmd = %x ,ubb = %d",g_offline_voice.key_enable,g_offline_voice.wake_word ,data,g_offline_voice.ubb_enable);

		/* 2������оƬ���ͻ���ָ� �жϻ��Ѵʾ����Ƿ��� */
    switch(state) {
        case VOICE_STATE_DISABLED:		// �ж�ָ��
            if((data == 0x21 && g_offline_voice.wake_word == Hello_Ergo) ||
               (data == 0x22 && g_offline_voice.wake_word == Hello_Bed)) {
                state = VOICE_STATE_WAKE_WORD_DETECTED;
                g_offline_voice.enabled = true;
                offline_voice_wake_up();
            }
            break;
            
        case VOICE_STATE_WAKE_WORD_DETECTED:	// ����
		
        case VOICE_STATE_ACTIVE:
            if(cmd == 0x81) {
                offline_voice_dataHandle(data);		/* 2.1������оƬ���Ͳ����� ��ָ���봫���������������� */
                state = VOICE_STATE_ACTIVE;
            } 
            else if(cmd == 0x82) 									/* 3������оƬ���͹ر�ָ� ���������ر�*/
						{
                offline_voice_wake_off();			
                state = VOICE_STATE_DISABLED;			
            }
            break;
    }
}

//void offline_voice_Handle_(uint8_t cmd , uint8_t data)
//{
//		/* 0����鰴��״̬ */
//		check_offline_voice_keys();
//	
//		/* 1�������������û��ʹ�ܣ�ֱ���˳� */
//		if(g_offline_voice.key_enable == false) return;
//	
//		LOG_I( "g_offline_voice.key_enable = %d, g_offline_voice.wake_word = %d,cmd = %x",g_offline_voice.key_enable,g_offline_voice.wake_word ,data );
//	
//		/* 1.1������оƬ���ͻ���ָ� �жϻ��Ѵʾ����Ƿ��� */
//		if(g_offline_voice.wake_word == Hello_Ergo && data == 0x21){
//			g_offline_voice.enabled = true;
//		}
//		
//		if(g_offline_voice.wake_word == Hello_Bed && data == 0x22){
//				g_offline_voice.enabled = true;
//			  LOG_I("g_offline_voice.enabled = true \r\n");
//		}
//		
//		
//		if(g_offline_voice.enabled == true){
//				offline_voice_wake_up(); // ʵ�ʻ���15s ��8s
//		}else{
//			return; 
//		}	
//	
//	
//		/* 2������оƬ���Ͳ����� ��ָ���봫���������������� */
//		if(cmd == 0x81){
//				offline_voice_dataHandle(data);
//		}
//		/* 3������оƬ���͹ر�ָ� ���������ر�*/
//		if(cmd == 0x82){

//			offline_voice_wake_off();
//		}
//}
