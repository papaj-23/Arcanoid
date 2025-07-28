#include<stdint.h>
#include<stdbool.h>

#ifndef SINGLEPLAYER_H

#define SINGLEPLAYER_H

typedef enum {
	DEG45,
	DEG135,
	DEG225,
	DEG315
} direction_t;

typedef enum {
	X2_SCORE,
	WIDER_PLATFORM
} bonus_type_t;

typedef struct {
	uint16_t duration_ms;
	bool is_active;
} bonus_t;

typedef struct {
	uint8_t gameover : 1;
	uint8_t game_restart : 1;
	uint8_t reserved : 6;
} game_t;

typedef struct {
	unsigned int score;
	uint8_t remaining_lives;
	bonus_t bonuses[2];
} gui_data_t;

typedef struct {
	int x;
	int x_previous;
	int width;
	int velocity;
	int shift_velocity;
} platform_t;

typedef struct{
	int x;
	int y;
	int x_previous;
	int y_previous;
	direction_t direction;
} ball_t;

//collision points structure, [0] = x, [1] = y
typedef struct {
	uint8_t p1[2];
	uint8_t p2[2];
	uint8_t p3[2];
	uint8_t p4[2];
	uint8_t p5[2];
} collision_points_t;

typedef struct {
	int col;
	int row;
} BlockID_t;

void Sp_GameInit(void);
void Sp_PlatformMove(void);
void Sp_InitShift(void);
void Sp_BallMove(void);
void Sp_Summary(void);
void Sp_Display_Score(void);
bool Sp_Is_Game_Over(void);
void Sp_Set_Game_Over(bool state);
bool Sp_Is_Game_Restarted(void);
void Sp_Set_Game_Restart(bool state);
void Sp_Game_Restart(void);

#endif
