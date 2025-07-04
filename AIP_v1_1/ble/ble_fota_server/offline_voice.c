#include "x_faultDetect.h"
#include "offline_voice.h"
#include "mfp_queue.h"
#include "g.h"

#define 		Hello_Ergo 		1
#define 		Hello_Bed 		2

offline_voice_ctrl_t g_offline_voice = {
    .enabled = false,
    .wake_word = 0,
};

void check_offline_voice_keys(void)
{
    uint8_t key1 = io_read_pin(KEY1_PIN);
    uint8_t key2 = io_read_pin(KEY2_PIN);

    if (key1 == 1 && key2 == 1)
    {
        g_offline_voice.enabled = false;  
    }
    else
    {
        g_offline_voice.enabled = true;   
        if (key1 == 0)
            g_offline_voice.wake_word = Hello_Ergo; // 1=Hello Ergo
        else if (key2 == 0)
            g_offline_voice.wake_word = Hello_Bed; // 2=Hello Bed
    }
}



void offline_voice_cmdHandle(uint8_t cmd)
{

	/* 1¡¢ÅÐ¶Ï»½ÐÑ´Ê¾ö¶¨ÊÇ·ñ»½ÐÑ */
	if(g_offline_voice.wake_word == Hello_Ergo && cmd == 0x21){
	
	}
	
	/* 2¡¢ */

			switch (cmd) 
			{
					case 0x21:		// Hello Ergo
							offline_voice_wake_up(); // Êµ¼Ê»½ÐÑ15s µÆ8s
							break;
					case 0x22:		// Hello Bed
							offline_voice_wake_up(); 
							break;
					case 0x23:		// STOP
							mfp_tx_queue_clear();
							prepare_mfp_NORMAL_KET(KEY_MOTOR_STOP,3); 
							break;
					case 0x24:		// All Up
							prepare_mfp_NORMAL_KET((KEY_M1_OUT|KEY_M2_OUT),33); 	//  Í·½ÅÌ§Éý6s
							break;
					case 0x25:		// Zero G
							prepare_mfp_NORMAL_KET(KEY_FLAT_ZEROG,3); 
							break;
					case 0x26:		// Flat Preset
							prepare_mfp_NORMAL_KET(KEY_ALLFATE,3); 
							break;				
					case 0x27:		// Favorite preset
					 
							break;
					case 0x28:		// Tv preset
							prepare_mfp_NORMAL_KET(KEY_MEMORY3,3); 
							break;
					case 0x29:		// Raise-head
							prepare_mfp_NORMAL_KET(KEY_M1_OUT,15); // Í·Ì§Éý3s
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
							prepare_mfp_NORMAL_KET(KEY_UBB,3);
							break;
					case 0x32:		// LIGHT On
							prepare_mfp_NORMAL_KET(KEY_UBB,3);
							break;
					default:
							LOG_I("Invalid command \r\n");
		}
}