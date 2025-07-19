#include <stdbool.h>
#include <stdint.h>

#ifndef INPUT_H
#define INPUT_H

typedef struct{
	uint32_t Joystick1_Button;
	uint32_t Joystick1_Yaxis;
}recent_times_t;

typedef struct{
	int x;
	int y;
}joystick_t;


// Debouncing functions for inputs
bool Button1_Debouncing(void);
bool Joystick1_Y_Debouncing(void);

// Joystick signals conditioning
joystick_t Joystick_Conditioning(uint16_t*);


#endif
