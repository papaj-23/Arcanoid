#include "input.h"

#ifndef GAME_H
#define GAME_H

//stany gry
typedef enum {
	DEFAULT,
	MENU,
	INIT,
	GAME,
	GAMEOVER
} game_state_t;

typedef enum {
	SINGLEPLAYER,
	MULTIPLAYER,
	OPTIONS,
	EXIT,
	DIFFICULTY_EASY,
	DIFFICULTY_MEDIUM,
	DIFFICULTY_HARD
} menu_state_t;

typedef enum {
	EASY = 1,
	MEDIUM,
	HARD
} difficulty_t;

typedef enum {
	MAIN,
	SETTINGS
} menu_level_t;

typedef struct {
	bool game_refresh;
	bool remove_caption;
	bool summary_done;
	bool change_refreshing_freq;
} game_flags_t;

typedef struct {
	bool platform_move;
	bool init_shift;
	bool ball_move;
	bool game_init_done;
} sp_flags_t;

typedef struct {

} mp_flags_t;

extern joystick_t joystick1;
extern joystick_t joystick2;

void Main_Game(void);
void Button1_Handler(void);
void Joystick1_Yaxis_Handler(void);
void Objects_TIM_Handler(void);
void Platforms_TIM_Handler(void);
void Refresh_TIM_Handler(void);

#endif
