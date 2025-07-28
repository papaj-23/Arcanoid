#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Singleplayer.h"
#include "pcd8544.h"
#include "input.h"


//platform
#define PLATFORM_Y 47
#define PLATFORM_X_DEFAULT 34			//left pixel -> base
#define PLATFORM_WIDTH_DEFAULT 8

#define PLATFORM_VELOCITY_THREESHOLD_1 100
#define PLATFORM_VELOCITY_THREESHOLD_2 1000
#define PLATFORM_VELOCITY_THREESHOLD_3 1900

#define PLATFORM_VELOCITY_LOW 1
#define PLATFORM_VELOCITY_MID 2
#define PLATFORM_VELOCITY_HIGH 3

//ball
#define BALL_X_DEFAULT 37				//top left pixel -> base
#define BALL_Y_DEFAULT 45
#define BALL_SIZE 2

//blocks
#define BLOCK_HEIGHT 2
#define BLOCK_WIDTH 3
#define BLOCK_SPACE 1
#define BLOCK_ROWS 7

//board limits
#define RIGHT_EDGE 83
#define BOTTOM_EDGE 47
#define BOARD_DIVISON_LINE 74

#define LIVES_AMOUNT 2

static Buffer_t Bonus_Buffer = {{0}, PCD8544_WIDTH - 1, 0, PCD8544_HEIGHT - 1, 0};
static gui_data_t GUI_data = {0, LIVES_AMOUNT, {{0, false}, {0, false}}};
static platform_t platform;
static ball_t ball;
static collision_points_t collision_points;
static game_t game_info = {0};

extern joystick_t joystick1;
extern joystick_t joystick2;

//static functions declaration
static bool InBlocksRange(void);
static void CalculateCollisionPoints(void);
static void BallReflection(void);
static bool PlatformReflection(void);
static void DiscardBlocks(void);
static bool BlocksEqual(BlockID_t a, BlockID_t b);
static bool Check_Collision_Point(uint8_t *cp);
static void Ball_Init(int x, int y, int x_prevoius, int y_prevoius);
static void Platform_Init(int x, int x_prevoius, int width, int velocity, int shift_velocity);
static void Display_Lives(void);

//collision points states
static bool px_states[5] = {
	false,
	false,
	false,
	false,
	false
};

static bool block_reflection = false;

void Sp_Display_Score() {
	char score_buffer[4];
	sprintf(score_buffer, "%03u", GUI_data.score);
	int i = 0;
	int y = 0;
	PCD8544_DrawFilledRectangle(77, 0, 83, 24, PCD8544_Pixel_Clear, &PCD8544_Buffer);
	while(score_buffer[i] != '\0') {
		gotoXY(77, y);
		PCD8544_Putc(score_buffer[i], PCD8544_Pixel_Set, PCD8544_FontSize_5x7);
		y += 8;
		i++;
	}
}

static void Display_Lives() {
	PCD8544_DrawFilledRectangle(77, 26, 83, 48, PCD8544_Pixel_Clear, &PCD8544_Buffer);
	for(int i = 0; i <= GUI_data.remaining_lives; i++) {
		gotoXY(77, 40 - i * 7/*PCD8544_CHAR5x7_HEIGHT*/);
		PCD8544_Putc(HEART_ID, PCD8544_Pixel_Set, PCD8544_FontSize_5x7);
	}
}

static void Ball_Init(int x, int y, int x_previous, int y_previous) {
	ball.x = x;
	ball.y = y;
	ball.x_previous = x_previous;
	ball.y_previous = y_previous;
	ball.direction = (rand() % 2) ? DEG45 : DEG135;
}

static void Platform_Init(int x, int x_prevoius, int width, int velocity, int shift_velocity) {
	platform.x = x;
	platform.x_previous = x_prevoius;
	platform.width = width;
	platform.velocity = velocity;
	platform.shift_velocity = shift_velocity;
}

void Sp_GameInit(){
	PCD8544_ClearBuffer(&PCD8544_Buffer);
	GUI_data.score = 0;
	GUI_data.remaining_lives = LIVES_AMOUNT;
	memset(&game_info, 0, sizeof(game_info));
	Ball_Init(BALL_X_DEFAULT, BALL_Y_DEFAULT, 0, 0);
	Platform_Init(PLATFORM_X_DEFAULT, 0, PLATFORM_WIDTH_DEFAULT, 0, 0);

	//limiting lines
	PCD8544_DrawLine(0, 0, 0, BOTTOM_EDGE, PCD8544_Pixel_Set, &PCD8544_Buffer);
	PCD8544_DrawLine(1, 0, BOARD_DIVISON_LINE, 0, PCD8544_Pixel_Set, &PCD8544_Buffer);
	PCD8544_DrawLine(BOARD_DIVISON_LINE, 1, BOARD_DIVISON_LINE, BOTTOM_EDGE, PCD8544_Pixel_Set, &PCD8544_Buffer);

	//platform
	PCD8544_DrawLine(platform.x, PLATFORM_Y, platform.x + platform.width - 1, PLATFORM_Y, PCD8544_Pixel_Set, &PCD8544_Buffer);

	//ball
	PCD8544_DrawFilledRectangle(ball.x, ball.y, ball.x + BALL_SIZE - 1, ball.y + BALL_SIZE, PCD8544_Pixel_Set, &PCD8544_Buffer);

	//blocks
	for(int i = BLOCK_SPACE+1; i < BLOCK_ROWS*(BLOCK_HEIGHT + BLOCK_SPACE); i += BLOCK_HEIGHT + BLOCK_SPACE) {
		for(int j = BLOCK_SPACE+1; j <= BOARD_DIVISON_LINE - (BLOCK_WIDTH + BLOCK_SPACE); j += BLOCK_WIDTH + BLOCK_SPACE) {
			PCD8544_DrawFilledRectangle(j, i, j + BLOCK_WIDTH - 1, i + BLOCK_HEIGHT, PCD8544_Pixel_Set, &PCD8544_Buffer);
		}
	}

	//caption
	gotoXY(4, 24);
	PCD8544_Puts("Press OK to start", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
	gotoXY(4, 32);
	PCD8544_Puts("Swipe R/L to move", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);

	Display_Lives();
	Sp_Display_Score();
}

void Sp_Game_Restart() {
	//Clear platform/ball
	PCD8544_DrawFilledRectangle(1, PLATFORM_Y - 2, BOARD_DIVISON_LINE - 1, PCD8544_HEIGHT, PCD8544_Pixel_Clear, &PCD8544_Buffer); //including all possible positions at the time
	//PCD8544_DrawFilledRectangle(ball.x - 1, ball.y - 1, ball.x + BALL_SIZE, ball.y + BALL_SIZE + 1, PCD8544_Pixel_Clear, &PCD8544_Buffer); //including all possible positions at the time

	//Reset ball/platform positions to default
	Ball_Init(BALL_X_DEFAULT, BALL_Y_DEFAULT, 0, 0);
	Platform_Init(PLATFORM_X_DEFAULT, 0, PLATFORM_WIDTH_DEFAULT, 0, 0);

	//Draw ball/platform at default positions
	PCD8544_DrawLine(platform.x, PLATFORM_Y, platform.x + platform.width - 1, PLATFORM_Y, PCD8544_Pixel_Set, &PCD8544_Buffer);
	PCD8544_DrawFilledRectangle(ball.x, ball.y, ball.x + BALL_SIZE - 1, ball.y + BALL_SIZE, PCD8544_Pixel_Set, &PCD8544_Buffer);

	GUI_data.remaining_lives--;
	Display_Lives();
}

void Sp_PlatformMove() {
	//calculate platform velocity
	int sign = (joystick1.x >= 0) ? 1 : -1;
	int abs_value_X = abs(joystick1.x);

	if (abs_value_X < PLATFORM_VELOCITY_THREESHOLD_1) {
	  platform.velocity = 0;
	}
	else if (abs_value_X < PLATFORM_VELOCITY_THREESHOLD_2) {
	  platform.velocity = sign * PLATFORM_VELOCITY_LOW;
	}
	else if (abs_value_X < PLATFORM_VELOCITY_THREESHOLD_3) {
	  platform.velocity = sign * PLATFORM_VELOCITY_MID;
	}
	else {
	  platform.velocity = sign * PLATFORM_VELOCITY_HIGH;
	}

	//velocity reduction at the edge of the board
	if(platform.x < 4 && platform.velocity < -PLATFORM_VELOCITY_LOW)
		platform.velocity = -PLATFORM_VELOCITY_LOW;

	else if(platform.x > BOARD_DIVISON_LINE - platform.width - 3 && platform.velocity > PLATFORM_VELOCITY_LOW)
		platform.velocity = PLATFORM_VELOCITY_LOW;

	//board limits
	if((platform.x < 2 && platform.velocity < 0) || (platform.x > BOARD_DIVISON_LINE - 1 - platform.width && platform.velocity > 0))
		return;

	//velocity = 0
	else if(platform.velocity == 0)
		return;

	//regular movement
	else {
		platform.x_previous = platform.x;
		platform.x += platform.velocity;

		PCD8544_DrawLine(platform.x_previous, PLATFORM_Y, platform.x_previous + platform.width - 1, PLATFORM_Y, PCD8544_Pixel_Clear, &PCD8544_Buffer);
		PCD8544_DrawLine(platform.x, PLATFORM_Y, platform.x + platform.width - 1, PLATFORM_Y, PCD8544_Pixel_Set, &PCD8544_Buffer);
	}
}

void Sp_InitShift() {
	//calculate shift velocity
	if(joystick1.x >= PLATFORM_VELOCITY_THREESHOLD_3)
		platform.shift_velocity = 1;

	else if (joystick1.x <= -PLATFORM_VELOCITY_THREESHOLD_3)
		platform.shift_velocity = -1;

	else
		platform.shift_velocity = 0;

	//board limits
	if((platform.x < 2 && platform.shift_velocity < 0) || (platform.x > BOARD_DIVISON_LINE - 1 - platform.width && platform.shift_velocity > 0))
		return;

	//velocity = 0
	else if(platform.shift_velocity == 0)
		return;

	else {
		platform.x_previous = platform.x;
		ball.x_previous = ball.x;
		platform.x += platform.shift_velocity;
		ball.x += platform.shift_velocity;

		PCD8544_DrawLine(platform.x_previous, PLATFORM_Y, platform.x_previous + platform.width - 1, PLATFORM_Y, PCD8544_Pixel_Clear, &PCD8544_Buffer);
		PCD8544_DrawFilledRectangle(ball.x_previous, ball.y, ball.x_previous + BALL_SIZE - 1, ball.y + BALL_SIZE, PCD8544_Pixel_Clear, &PCD8544_Buffer);
		PCD8544_DrawLine(platform.x, PLATFORM_Y, platform.x + platform.width - 1, PLATFORM_Y, PCD8544_Pixel_Set, &PCD8544_Buffer);
		PCD8544_DrawFilledRectangle(ball.x, ball.y, ball.x + BALL_SIZE - 1, ball.y + BALL_SIZE, PCD8544_Pixel_Set, &PCD8544_Buffer);
	}
}

void Sp_BallMove() {

	if(block_reflection)
		block_reflection = false;

	CalculateCollisionPoints();
	BallReflection();

	if(block_reflection && InBlocksRange())
		DiscardBlocks();

	//clear collison points states
	memset(px_states, 0, sizeof(px_states));

	int sign_x = (ball.direction == DEG45 || ball.direction == DEG315) ? 1 : -1;
	int sign_y = (ball.direction == DEG225 || ball.direction == DEG315) ? 1 : -1;

	ball.x_previous = ball.x;
	ball.y_previous = ball.y;
	ball.x += sign_x;
	ball.y += sign_y;

	if(ball.y > 0 && ball.y < PCD8544_HEIGHT - 2) {
		PCD8544_DrawFilledRectangle(ball.x_previous, ball.y_previous, ball.x_previous + BALL_SIZE - 1, ball.y_previous + BALL_SIZE, PCD8544_Pixel_Clear, &PCD8544_Buffer);
		PCD8544_DrawFilledRectangle(ball.x, ball.y, ball.x + BALL_SIZE - 1, ball.y + BALL_SIZE, PCD8544_Pixel_Set, &PCD8544_Buffer);
	}

	else if(ball.y >= PCD8544_HEIGHT - 2 && GUI_data.remaining_lives >= 1) {
		game_info.game_restart = true;
	}

	else if(ball.y >= PCD8544_HEIGHT - 2 && GUI_data.remaining_lives < 1){
		game_info.gameover = true;
	}
}

static bool InBlocksRange() {
	if((ball.x > 1 && ball.x < BOARD_DIVISON_LINE - 2) && (ball.y > 1 && ball.y < BLOCK_ROWS*((BLOCK_SPACE + BLOCK_HEIGHT) + (BLOCK_SPACE + 1) )))
		return true;
	else
		return false;

}

static void CalculateCollisionPoints() {
	switch(ball.direction) {

	case 0:		// 45 degrees case
		collision_points.p1[0] = ball.x;
		collision_points.p1[1] = ball.y - 1;
		collision_points.p2[0] = ball.x + 1;
		collision_points.p2[1] = ball.y - 1;
		collision_points.p3[0] = ball.x + 2;
		collision_points.p3[1] = ball.y - 1;
		collision_points.p4[0] = ball.x + 2;
		collision_points.p4[1] = ball.y;
		collision_points.p5[0] = ball.x + 2;
		collision_points.p5[1] = ball.y + 1;

		break;

	case 1:		// 135 degrees case
		collision_points.p1[0] = ball.x - 1;
		collision_points.p1[1] = ball.y + 1;
		collision_points.p2[0] = ball.x - 1;
		collision_points.p2[1] = ball.y;
		collision_points.p3[0] = ball.x - 1;
		collision_points.p3[1] = ball.y - 1;
		collision_points.p4[0] = ball.x;
		collision_points.p4[1] = ball.y - 1;
		collision_points.p5[0] = ball.x + 1;
		collision_points.p5[1] = ball.y - 1;

		break;

	case 2:		// 225 degrees case
		collision_points.p1[0] = ball.x + 1;
		collision_points.p1[1] = ball.y + 2;
		collision_points.p2[0] = ball.x;
		collision_points.p2[1] = ball.y + 2;
		collision_points.p3[0] = ball.x - 1;
		collision_points.p3[1] = ball.y + 2;
		collision_points.p4[0] = ball.x - 1;
		collision_points.p4[1] = ball.y + 1;
		collision_points.p5[0] = ball.x - 1;
		collision_points.p5[1] = ball.y;

		break;

	case 3:		// 315 degrees case
		collision_points.p1[0] = ball.x + 2;
		collision_points.p1[1] = ball.y;
		collision_points.p2[0] = ball.x + 2;
		collision_points.p2[1] = ball.y + 1;
		collision_points.p3[0] = ball.x + 2;
		collision_points.p3[1] = ball.y + 2;
		collision_points.p4[0] = ball.x + 1;
		collision_points.p4[1] = ball.y + 2;
		collision_points.p5[0] = ball.x;
		collision_points.p5[1] = ball.y + 2;

		break;
	}
}
//Check state of collision point
static bool Check_Collision_Point(uint8_t *cp) {
	if(PCD8544_Buffer.Content[cp[0] + (cp[1] / 8) * PCD8544_WIDTH] & 1 << (cp[1] % 8)) {
		return true;
	} else {
		return false;
	}
}

static void BallReflection() {
	int right_rotation = 0;
	int left_rotation = 0;
	bool reverse = false;

	if(Check_Collision_Point(collision_points.p1)) {
		right_rotation++;
		px_states[0] = true;
	}

	if(Check_Collision_Point(collision_points.p2)) {
		right_rotation++;
		px_states[1] = true;
	}

	if(Check_Collision_Point(collision_points.p4)) {
		left_rotation++;
		px_states[3] = true;
	}

	if(Check_Collision_Point(collision_points.p5)) {
		left_rotation++;
		px_states[4] = true;
	}

	if(Check_Collision_Point(collision_points.p3)) {
		reverse = true;
		px_states[2] = true;
	}

	//no bounce
	if((left_rotation + right_rotation == 0) && !reverse)
		return;

	//platform bounce
	if(PlatformReflection()) {
		if(ball.x < platform.x + platform.width/2 - 1)
			ball.direction = DEG135;
		else if(ball.x > platform.x + platform.width/2 - 1)
			ball.direction = DEG45;
		else {
			if (rand() % 2 == 0)
				ball.direction = DEG135;
			else
				ball.direction = DEG45;
		}

		return;
	}

	if(right_rotation > 0 && left_rotation == 0) {
		ball.direction = (ball.direction + 3) % 4;
		block_reflection = true;
	}
	else if(left_rotation > 0 && right_rotation == 0) {
		ball.direction = (ball.direction + 1) % 4;
		block_reflection = true;
	}
	else if((reverse && right_rotation + left_rotation == 0) || (right_rotation > 0 && left_rotation > 0)) {
		ball.direction = (ball.direction + 2) % 4;
		block_reflection = true;
	}
}

static bool PlatformReflection() {
	if(ball.y == PCD8544_HEIGHT - 1 - BALL_SIZE)
		return true;
	else
		return false;
}

static void DiscardBlocks() {
	BlockID_t hit_blocks[4];
	int block_count = 0;

	uint8_t* points[] = {
		collision_points.p1,
		collision_points.p2,
		collision_points.p3,
		collision_points.p4,
		collision_points.p5
	};


	for(int i = 0; i < 5; i++) {
		if(px_states[i]) {
			int x = points[i][0];
			int y = points[i][1];

			int col = (x - 1 - BLOCK_SPACE) / (BLOCK_WIDTH + BLOCK_SPACE);
			int row = (y - BLOCK_SPACE) / (BLOCK_HEIGHT + BLOCK_SPACE);

			BlockID_t current_block = {col, row};

			bool already_added = false;
			for(int j = 0; j < block_count; j++) {
				if(BlocksEqual(current_block, hit_blocks[j])) {
					already_added = true;
					break;
				}
			}

			if(!already_added) {
				// zapamiętanie bloku
				hit_blocks[block_count++] = current_block;

				// wyczyszczenie bloku
				int block_x = BLOCK_SPACE + 1 + col * (BLOCK_WIDTH + BLOCK_SPACE);
				int block_y = BLOCK_SPACE + 1 + row * (BLOCK_HEIGHT + BLOCK_SPACE);
				PCD8544_DrawFilledRectangle(block_x, block_y, block_x + BLOCK_WIDTH - 1, block_y + BLOCK_HEIGHT, PCD8544_Pixel_Clear, &PCD8544_Buffer);
			}
		}
	}

	// zaktualizuj wynik
	GUI_data.score += block_count;
}

static bool BlocksEqual(BlockID_t a, BlockID_t b) {
	return (a.col == b.col && a.row == b.row);
}

//Setters and getters for flags ending/restarting game
bool Sp_Is_Game_Over() {
	return game_info.gameover;
}

void Sp_Set_Game_Over(bool state) {
	game_info.gameover = state;
}

bool Sp_Is_Game_Restarted() {
	return game_info.game_restart;
}

void Sp_Set_Game_Restart(bool state) {
	game_info.game_restart = state;
}

void Sp_Summary() {
	char score_buffer[4];
	sprintf(score_buffer, "%d", GUI_data.score);
	gotoXY(12, 26);
	PCD8544_Puts("GAME OVER", PCD8544_Pixel_Set, PCD8544_FontSize_5x7);
	gotoXY(20, 34);
	PCD8544_Puts("SCORE: ", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
	gotoXY(48, 34);
	PCD8544_Puts(score_buffer, PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
	PCD8544_DrawFilledRectangle(77, 26, 83, 48, PCD8544_Pixel_Clear, &PCD8544_Buffer);
	PCD8544_Refresh();
}














