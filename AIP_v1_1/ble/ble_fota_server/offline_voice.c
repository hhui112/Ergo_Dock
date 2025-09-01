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
		if(cmd == 0x21 || cmd == 0x22){
				app_Receive_Wakeup_LedOn();	// ��������8s
				return;
		}
		app_ReceiveCommand_LedOn();	// ����3 ����5s
		g_sysparam_st.snoreIntervention.is_intervening = false;
		g_sysparam_st.snoreIntervention.triggered_flag = false;
			switch (cmd) 
			{
					case 0x21:
							break;
					case 0x22:
							break;
					case 0x23:		// STOP
							mfp_tx_queue_clear();
							prepare_mfp_NORMAL_KET(KEY_MASSAGE_STOP_ALL,1); 
							break;
					case 0x24:		// All Up
							prepare_mfp_NORMAL_KET((KEY_M1_OUT|KEY_M2_OUT),30); 	//  ͷ��̧��6s
							// prepare_mfp_SOFT_START(KEY_MEMORY4, 0x32, 0x10,3);  //KEY_MEMORY4 
							//app_Receive_Wakeup_LedOn();
							break;
					case 0x25:		// Zero G
							prepare_mfp_NORMAL_KET(KEY_ZEROG,3);
							break;
					case 0x26:		// Flat Preset
							prepare_mfp_NORMAL_KET(KEY_ALLFATE,3); 
							// prepare_mfp_SOFT_START(KEY_ALLFATE,0x32,0x20,3);
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
					case 0x2D:		// Massage On  (massageAll ����ͷ�Ű�Ħ)
							prepare_mfp_NORMAL_KET(KEY_MASSAGE_All,3);
							break;
					case 0x2E:		// Massage Up ͷ�Ű�Ħ��ǿ
							prepare_mfp_NORMAL_KET(KEY_MASSAGE_FEET|KEY_MASSAGE_HEAD,3);
							break;
					case 0x2F:		// Massage Down
							prepare_mfp_NORMAL_KET(KEY_MASSAGE_HEAD_MINUS|KEY_MASSAGE_FEET_MIUNS,3);
							break;
					case 0x30:		// MASSAGE OFF   
							prepare_mfp_NORMAL_KET(KEY_MASSAGE_STOP_ALL,3);
							break;
					case 0x31:		// LIGHT OFF
							if(g_offline_voice.ubb_enable == true)  prepare_mfp_NORMAL_KET(KEY_UBB,3);
							// prepare_mfp_NORMAL_KET(KEY_UBB,3);
							break;
					case 0x32:		// LIGHT On
							 if(g_offline_voice.ubb_enable == false) prepare_mfp_NORMAL_KET(KEY_UBB,3);		// UBB�ر�ʱ��ſ���   ״̬���ñ���(MFP״̬һֱ�ش���)
							// prepare_mfp_NORMAL_KET(KEY_UBB,3);
							break;
					case 0x33:		// GOOD NIGHT
							prepare_mfp_NORMAL_KET(KEY_ALLFATE,3);
							break;						
					case 0x34:		// ANTI-SNORE
							prepare_mfp_NORMAL_KET(KEY_MEMORY4,3);
							break;	
					case 0x35:		// RAISE LUMBAR
							prepare_mfp_NORMAL_KET(KEY_M4_OUT,15);
							break;
					case 0x36:		// LOWER LUMBAR
							prepare_mfp_NORMAL_KET(KEY_M4_IN,15);
							break;
					case 0x37:		// RAISE TILT
							prepare_mfp_NORMAL_KET(KEY_M3_OUT,15);
							break;
					case 0x38:		// LOWER TILT
							prepare_mfp_NORMAL_KET(KEY_M3_IN,15);
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
		// check_offline_voice_keys();
	
		/* 1�����������������û��ʹ�ܣ�ֱ���˳� */
    if(g_offline_voice.key_enable == false){
			state = VOICE_STATE_DISABLED;		// ��Ҫ���»���
			return;
		}
	
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
