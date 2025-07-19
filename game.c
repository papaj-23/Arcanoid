#include <stdlib.h>
#include "game.h"
#include "Singleplayer.h"
#include "input.h"
#include "pcd8544.h"
#include "stm32f3xx_hal_tim.h"

static const unsigned int MENU_STATES_MAIN = 4;
static const unsigned int MENU_STATES_SETTINGS = 3;

extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim6;

game_state_t Current_Game_State = DEFAULT;
menu_state_t Current_Menu_State;
game_flags_t game_flags = {0};
sp_flags_t sp_flags = {0};
menu_level_t menu_level = MAIN;

// data fetched from joysticks
joystick_t joystick1;
joystick_t joystick2;

//static functions declarations
static void Welcome_Screen(void);
static void Menu_Handler(void);
static void Game_Refresh(void);
static void Menu_Press_Button1(void);
static void Set_Difficulty(difficulty_t);
static void Reset_flags(void);
static void Set_refreshing(void);

static void Set_refreshing() {
	//if(game_flags.change_refreshing_freq) {
		if(Current_Game_State == INIT || Current_Game_State == GAME) {
			__HAL_TIM_SET_AUTORELOAD(&htim6, 99);
		}
		else {
			__HAL_TIM_SET_AUTORELOAD(&htim6, 1660);
		}

	//	game_flags.change_refreshing_freq = false;
	//}
}

static void Welcome_Screen() {
	PCD8544_ClearBuffer(&PCD8544_Buffer);
	gotoXY(18, 15);
	PCD8544_Puts("Arcanoid", PCD8544_Pixel_Set, PCD8544_FontSize_5x7);
	gotoXY(26, 26);
	PCD8544_Puts("Press OK", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
}

void Button1_Handler() {
	if(Button1_Debouncing()) {

		switch(Current_Game_State) {

		case DEFAULT:
			Current_Game_State = MENU;
			game_flags.change_refreshing_freq = false;
			Current_Menu_State = SINGLEPLAYER;
			break;

		case MENU:
			Menu_Press_Button1();
			break;

		case INIT:
			Current_Game_State = GAME;
			game_flags.change_refreshing_freq = false;
			break;

		case GAME:

			break;

		case GAMEOVER:
			if(game_flags.summary_done) {
				Current_Game_State = MENU;
				game_flags.change_refreshing_freq = false;
				game_flags.summary_done = false;
			}
			break;
		}

	}
}

void Joystick1_Yaxis_Handler() {
	if(Current_Game_State == MENU && menu_level == MAIN) {
		if(joystick1.y > 1800 && Joystick1_Y_Debouncing()) {
			Current_Menu_State = (Current_Menu_State + MENU_STATES_MAIN - 1) % MENU_STATES_MAIN;
		}
		else if (joystick1.y < -1800 && Joystick1_Y_Debouncing()) {
			Current_Menu_State = (Current_Menu_State + 1) % MENU_STATES_MAIN;
		}
	}
	else if(Current_Game_State == MENU && menu_level == SETTINGS) {
		if(joystick1.y > 1800 && Joystick1_Y_Debouncing()) {
			Current_Menu_State = ((Current_Menu_State + 1) % MENU_STATES_SETTINGS) + MENU_STATES_MAIN;
		}
		else if (joystick1.y < -1800 && Joystick1_Y_Debouncing()) {
			Current_Menu_State = (Current_Menu_State % MENU_STATES_SETTINGS) + MENU_STATES_MAIN;
		}
	}
}

void Objects_TIM_Handler() {
	if(Current_Game_State == GAME)
		sp_flags.ball_move = true;
}

void Platforms_TIM_Handler() {
	if(Current_Game_State == GAME) {
		sp_flags.platform_move = true;
	}

	else if(Current_Game_State == INIT) {
		sp_flags.init_shift = true;
	}
}

void Refresh_TIM_Handler() {
	game_flags.game_refresh = true;
}

//sets period for objects timer using ARR
static void Set_Difficulty(difficulty_t difficulty) {
	switch(difficulty) {
	case EASY:
		__HAL_TIM_SET_AUTORELOAD(&htim4, 533);
		break;

	case MEDIUM:
		__HAL_TIM_SET_AUTORELOAD(&htim4, 399);
		break;

	case HARD:
		__HAL_TIM_SET_AUTORELOAD(&htim4, 266);
		break;
	}
}

static void Menu_Press_Button1() {
	switch(Current_Menu_State) {
	case SINGLEPLAYER:
		game_flags.change_refreshing_freq = false;
		Current_Game_State = INIT;
		break;
	case MULTIPLAYER:

		break;

	case OPTIONS:
		menu_level = SETTINGS;
		Current_Menu_State = DIFFICULTY_EASY;
		break;

	case EXIT:
		Current_Game_State = DEFAULT;
		break;

	case DIFFICULTY_EASY:
		Set_Difficulty(EASY);
		menu_level = MAIN;
		Current_Menu_State = SINGLEPLAYER;
		break;

	case DIFFICULTY_MEDIUM:
		Set_Difficulty(MEDIUM);
		menu_level = MAIN;
		Current_Menu_State = SINGLEPLAYER;
		break;

	case DIFFICULTY_HARD:
		Set_Difficulty(HARD);
		menu_level = MAIN;
		Current_Menu_State = SINGLEPLAYER;
		break;
	}
}

static void Menu_Handler() {
	switch(Current_Menu_State) {
	case SINGLEPLAYER:
		PCD8544_ClearBuffer(&PCD8544_Buffer);
		gotoXY(3, 1);
		PCD8544_Puts("SINGLEPLAYER", PCD8544_Pixel_Set, PCD8544_FontSize_5x7);
		gotoXY(3, 10);
		PCD8544_Puts("MULTIPLAYER", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 18);
		PCD8544_Puts("OPTIONS", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 26);
		PCD8544_Puts("EXIT", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		break;
	case MULTIPLAYER:
		PCD8544_ClearBuffer(&PCD8544_Buffer);
		gotoXY(3, 2);
		PCD8544_Puts("SINGLEPLAYER", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 9);
		PCD8544_Puts("MULTIPLAYER", PCD8544_Pixel_Set, PCD8544_FontSize_5x7);
		gotoXY(3, 18);
		PCD8544_Puts("OPTIONS", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 26);
		PCD8544_Puts("EXIT", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		break;

	case OPTIONS:
		PCD8544_ClearBuffer(&PCD8544_Buffer);
		gotoXY(3, 2);
		PCD8544_Puts("SINGLEPLAYER", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 10);
		PCD8544_Puts("MULTIPLAYER", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 17);
		PCD8544_Puts("OPTIONS", PCD8544_Pixel_Set, PCD8544_FontSize_5x7);
		gotoXY(3, 26);
		PCD8544_Puts("EXIT", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		break;

	case EXIT:
		PCD8544_ClearBuffer(&PCD8544_Buffer);
		gotoXY(3, 2);
		PCD8544_Puts("SINGLEPLAYER", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 10);
		PCD8544_Puts("MULTIPLAYER", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 18);
		PCD8544_Puts("OPTIONS", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 25);
		PCD8544_Puts("EXIT", PCD8544_Pixel_Set, PCD8544_FontSize_5x7);
		break;

	case DIFFICULTY_EASY:
		PCD8544_ClearBuffer(&PCD8544_Buffer);
		gotoXY(3, 1);
		PCD8544_Puts("EASY", PCD8544_Pixel_Set, PCD8544_FontSize_5x7);
		gotoXY(3, 10);
		PCD8544_Puts("MEDIUM", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 18);
		PCD8544_Puts("HARD", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		break;

	case DIFFICULTY_MEDIUM:
		PCD8544_ClearBuffer(&PCD8544_Buffer);
		gotoXY(3, 2);
		PCD8544_Puts("EASY", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 9);
		PCD8544_Puts("MEDIUM", PCD8544_Pixel_Set, PCD8544_FontSize_5x7);
		gotoXY(3, 18);
		PCD8544_Puts("HARD", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		break;

	case DIFFICULTY_HARD:
		PCD8544_ClearBuffer(&PCD8544_Buffer);
		gotoXY(3, 2);
		PCD8544_Puts("EASY", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 10);
		PCD8544_Puts("MEDIUM", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
		gotoXY(3, 17);
		PCD8544_Puts("HARD", PCD8544_Pixel_Set, PCD8544_FontSize_5x7);
		break;
	}
}

static void Game_Refresh() {
	if(game_flags.game_refresh){
		PCD8544_Refresh();
		game_flags.game_refresh = false;
	}
}

static void Reset_flags() {
	sp_flags.game_init_done = false;
	game_flags.remove_caption = false;
}

void Main_Game() {
	Set_refreshing();

	switch(Current_Game_State) {

	case DEFAULT:
		Welcome_Screen();
		break;

	case MENU:
		Menu_Handler();
		break;

	case INIT:
		if(!sp_flags.game_init_done) {
			Sp_GameInit();
			sp_flags.game_init_done = true;
		}

		if(Is_Game_Restarted()) {
			Game_Restart();
			Set_Game_Restart(false);
		}

		//choosing start point
		if(sp_flags.init_shift) {
			Sp_InitShift();
			sp_flags.init_shift = false;
		}

		break;

	case GAME:
		//remove caption from init phase
		if(!game_flags.remove_caption) {
			PCD8544_DrawFilledRectangle(4, 24, 73, 38, PCD8544_Pixel_Clear, &PCD8544_Buffer);
			game_flags.remove_caption = true;
		}

		//ruch platformy
		if(sp_flags.platform_move) {
			Sp_PlatformMove();
			Sp_Display_Score();
			sp_flags.platform_move = false;
		}

		if(sp_flags.ball_move) {
			Sp_BallMove();
			sp_flags.ball_move = false;
		}

		if(Is_Game_Restarted()) {
			Current_Game_State = INIT;
		}
		else if(Is_Game_Over()) {
			Current_Game_State = GAMEOVER;
			Set_Game_Over(false);
		}

		break;

	case GAMEOVER:
		if(!game_flags.summary_done) {
			Sp_Summary();
			Reset_flags();
			game_flags.summary_done = true;
		}

		break;
	}
	Game_Refresh();
}

