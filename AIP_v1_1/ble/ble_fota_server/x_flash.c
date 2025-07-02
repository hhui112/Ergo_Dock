#include "x_flash.h"
#include "g.h"
#include "hal_flash_int.h"

#define FLASH_ADDR  0x1807BE00

static uint8_t 			old_ai_adj;
static uint8_t 			old_ai_adj_strength;
static uint16_t 		old_adjPa;       				//自适应气压
static uint8_t 			old_snoreIntervention_enable;	//打鼾使能标志位

void x_flash_static_Init(void);

void set_savetoflash(uint8_t *p,uint8_t len)
{
	uint8_t i;
  hal_flash_page_erase(FLASH_ADDR);
	hal_flash_page_program(FLASH_ADDR,p,len);
}


void x_flash_load(void)
{
	uint8_t *p = (uint8_t*)FLASH_ADDR;
	uint16_t crc_t = 0;
	flash_save_data_st  flash_save_data;	
	
	memcpy(&flash_save_data,p,sizeof(flash_save_data));
	
	crc_t=Modbus_Crc_Compute((uint8_t*)&flash_save_data, sizeof(flash_save_data)-2);//CRC校验
	
	if(crc_t == flash_save_data.crc)
	{
		LOG_I("flsh_read OK");
		
		g_sysparam_st.ai_adj = flash_save_data.ai_adj;
		g_sysparam_st.airpump.adjPa = flash_save_data.adjPa; 
		g_sysparam_st.ai_adj_strength = flash_save_data.ai_adj_strength;
	}
	else
	{
//		adaptivecontrol_default();
		LOG_I("flsh_read FAIL");
	}
	x_flash_static_Init();
}

void x_flash_save(void)
{
	uint16_t crc_t = 0;
	flash_save_data_st  flash_save_data;	
	memset(&flash_save_data,0,sizeof(flash_save_data));
	flash_save_data.ai_adj =g_sysparam_st.ai_adj;
	flash_save_data.ai_adj_strength = g_sysparam_st.ai_adj_strength;
	flash_save_data.adjPa = g_sysparam_st.airpump.adjPa;
	crc_t=Modbus_Crc_Compute((uint8_t*)&flash_save_data, sizeof(flash_save_data)-2);//CRC校验
	flash_save_data.crc = crc_t;
	set_savetoflash((uint8_t*)&flash_save_data,sizeof(flash_save_data));
}


void x_flash_static_Init(void)
{
	old_ai_adj_strength =  g_sysparam_st.ai_adj_strength;
	old_ai_adj = g_sysparam_st.ai_adj;
	old_adjPa = g_sysparam_st.airpump.adjPa;
	old_snoreIntervention_enable = g_sysparam_st.snoreIntervention.enable;
}


void x_flash_run(void)
{

	if(old_ai_adj != g_sysparam_st.ai_adj || 
		 old_ai_adj_strength !=  g_sysparam_st.ai_adj_strength||
		 old_adjPa != g_sysparam_st.airpump.adjPa || 
		 old_snoreIntervention_enable != g_sysparam_st.snoreIntervention.enable)
	{
		x_flash_save();
	}
	
	
	old_ai_adj = g_sysparam_st.ai_adj;
	old_adjPa = g_sysparam_st.airpump.adjPa;
	old_snoreIntervention_enable = g_sysparam_st.snoreIntervention.enable;
	old_ai_adj_strength =  g_sysparam_st.ai_adj_strength;
}


void flash_run(void)
{

	if(old_snoreIntervention_enable != g_sysparam_st.snoreIntervention.enable)
	{
		x_flash_save();
	}
	old_snoreIntervention_enable = g_sysparam_st.snoreIntervention.enable;
}
