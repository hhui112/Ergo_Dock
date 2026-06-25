#include "x_flash.h"
#include "g.h"
#include "hal_flash_int.h"

#define FLASH_ADDR  0x1807BE00

static uint8_t old_snore_pwm;
static uint8_t old_snore_tmr;
static uint8_t old_snore_intensity;

static void flash_static_init(void);
static void flash_snore_shadow_update(void);

void set_savetoflash(uint8_t *p,uint8_t len)
{
  	hal_flash_page_erase(FLASH_ADDR);
	hal_flash_page_program(FLASH_ADDR,p,len);
}


void flash_load(void)
{
	uint8_t *p = (uint8_t*)FLASH_ADDR;
	uint16_t crc_t = 0;
	flash_save_data_st  flash_save_data;	
	
	memcpy(&flash_save_data,p,sizeof(flash_save_data));
	
	crc_t=Modbus_Crc_Compute((uint8_t*)&flash_save_data, sizeof(flash_save_data)-2);
	
	if(crc_t == flash_save_data.crc)
	{
		LOG_I("flsh_read OK");
		
		g_sysparam_st.snoreIntervention.snoreIntervention_pwm = flash_save_data.snore_pwm;
		g_sysparam_st.snoreIntervention.snoreIntervention_tmr = flash_save_data.snore_tmr;
		g_sysparam_st.AntiSnore_intensity = (uint8_t)(flash_save_data.snore_intensity > 3U ? 3U : flash_save_data.snore_intensity);
		g_sysparam_st.snoreIntervention.enable = (bool)(g_sysparam_st.AntiSnore_intensity != 0U);
	}
	else
	{
		LOG_I("flsh_read FAIL");
	}
	flash_static_init();
}

static void flash_save(void)
{
	uint16_t crc_t = 0;
	flash_save_data_st  flash_save_data;	
	memset(&flash_save_data,0,sizeof(flash_save_data));
	flash_save_data.snore_pwm = g_sysparam_st.snoreIntervention.snoreIntervention_pwm;
	flash_save_data.snore_tmr = g_sysparam_st.snoreIntervention.snoreIntervention_tmr;
	flash_save_data.snore_intensity = (uint8_t)(g_sysparam_st.AntiSnore_intensity > 3U ? 3U : g_sysparam_st.AntiSnore_intensity);
	crc_t = Modbus_Crc_Compute((uint8_t *)&flash_save_data, sizeof(flash_save_data) - 2U);
	flash_save_data.crc = crc_t;
	set_savetoflash((uint8_t*)&flash_save_data,sizeof(flash_save_data));
}


static void flash_static_init(void)
{
	flash_snore_shadow_update();
}

static void flash_snore_shadow_update(void)
{
	old_snore_pwm = g_sysparam_st.snoreIntervention.snoreIntervention_pwm;
	old_snore_tmr = g_sysparam_st.snoreIntervention.snoreIntervention_tmr;
	old_snore_intensity = g_sysparam_st.AntiSnore_intensity;
}

void flash_save_snore_cfg_if_changed(void)
{
	if (old_snore_pwm != g_sysparam_st.snoreIntervention.snoreIntervention_pwm ||
	    old_snore_tmr != g_sysparam_st.snoreIntervention.snoreIntervention_tmr ||
	    old_snore_intensity != g_sysparam_st.AntiSnore_intensity) {
		flash_save();
	}
	flash_snore_shadow_update();
}


void flash_run(void)
{
	/* Keep periodic polling: key path may update snore intensity without BLE write. */
	flash_save_snore_cfg_if_changed();
}
