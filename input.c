#include "stm32f3xx_hal.h"
#include "input.h"

recent_times_t RecentTimes = {0, 0};

bool Button1_Debouncing() {
	uint32_t current_time = HAL_GetTick();

	if(current_time - RecentTimes.Joystick1_Button < 200) {
		return false;
	}
	else{
		RecentTimes.Joystick1_Button = current_time;
		return true;
	}
}

bool Joystick1_Y_Debouncing() {
	uint32_t current_time = HAL_GetTick();

	if(current_time - RecentTimes.Joystick1_Yaxis < 250) {
		return false;
	}
	else{
		RecentTimes.Joystick1_Yaxis = current_time;
		return true;
	}
}

joystick_t Joystick_Conditioning(uint16_t* joystick_buffer) {
	joystick_t joystick_values;
	joystick_values.x = joystick_buffer[0] - JOYSTICK_CENTER_VALUE;
	joystick_values.y = joystick_buffer[1] - JOYSTICK_CENTER_VALUE;

	return joystick_values;
}
