#include "x_control.h"
#include "g.h"


void control_timer10ms(void)
{
	  static uint8_t last_key = 0;
	  uint8_t key = app_touch_key_get();
	  if(last_key == 0 && key != 0)  {
		  LOG_I("key %x pressed",key);
		} else if(last_key == key && key != 0) {
		
		}else if(last_key != 0 && key == 0) {
		
		}
	
	  last_key = key;
}

