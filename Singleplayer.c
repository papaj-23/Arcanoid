#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "Singleplayer.h"
#include "pcd8544.h"
#include "input.h"

static platform_t platform;
static ball_t ball;
static collision_points_t collision_points;
bool sp_gameover = false;

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

//collision points states
bool px_states[5] = {
	false,
	false,
	false,
	false,
	false
};

int shift_velocity = 0;
static unsigned int score = 0;
bool block_reflection = false;

void Sp_GameInit(){
	PCD8544_ClearBuffer(&PCD8544_Buffer);
	score = 0;

	ball.x = BALL_X_DEFAULT;
	ball.y = BALL_Y_DEFAULT;
	ball.x_previous = 0;
	ball.y_previous = 0;
	ball.raster = 1;
	if(rand() % 2)
		ball.direction = DEG45;
	else
		ball.direction = DEG135;

	platform.x = PLATFORM_X_DEFAULT;
	platform.x_previous = 0;
	platform.width = PLATFORM_WIDTH_DEFAULT;
	platform.velocity = 0;


	//limiting lines
	PCD8544_DrawLine(0, 0, 0, 47, PCD8544_Pixel_Set, &PCD8544_Buffer);
	PCD8544_DrawLine(1, 0, 83, 0, PCD8544_Pixel_Set, &PCD8544_Buffer);
	PCD8544_DrawLine(83, 1, 83, 47, PCD8544_Pixel_Set, &PCD8544_Buffer);

	//platform
	PCD8544_DrawLine(platform.x, PLATFORM_Y, platform.x + platform.width - 1, PLATFORM_Y, PCD8544_Pixel_Set, &PCD8544_Buffer);

	//ball
	PCD8544_DrawFilledRectangle(ball.x, ball.y, ball.x + BALL_SIZE - 1, ball.y + BALL_SIZE, PCD8544_Pixel_Set, &PCD8544_Buffer);

	//blocks
	for(int i = BLOCK_SPACE+1; i < BLOCK_ROWS*(BLOCK_HEIGHT + BLOCK_SPACE); i += BLOCK_HEIGHT + BLOCK_SPACE){
		for(int j = BLOCK_SPACE+1; j < PCD8544_WIDTH - (BLOCK_WIDTH + BLOCK_SPACE); j += BLOCK_WIDTH + BLOCK_SPACE){
			PCD8544_DrawFilledRectangle(j, i, j + BLOCK_WIDTH - 1, i + BLOCK_HEIGHT, PCD8544_Pixel_Set, &PCD8544_Buffer);
		}
	}

	//caption
	gotoXY(8, 26);
	PCD8544_Puts("Press OK to start", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
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

	if(platform.x > PCD8544_WIDTH - platform.width - 4 && platform.velocity > PLATFORM_VELOCITY_LOW)
		platform.velocity = PLATFORM_VELOCITY_LOW;

	//board limits
	if((platform.x < 2 && platform.velocity < 0) || (platform.x > 82 - platform.width && platform.velocity > 0))
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
		shift_velocity = 1;

	else if (joystick1.x <= -PLATFORM_VELOCITY_THREESHOLD_3)
		shift_velocity = -1;

	else
		shift_velocity = 0;

	//board limits
	if((platform.x < 2 && shift_velocity < 0) || (platform.x > 82 - platform.width && shift_velocity > 0))
		return;

	//velocity = 0
	else if(shift_velocity == 0)
		return;

	else {
		platform.x_previous = platform.x;
		ball.x_previous = ball.x;
		platform.x += shift_velocity;
		ball.x += shift_velocity;

		PCD8544_DrawLine(platform.x_previous, PLATFORM_Y, platform.x_previous + platform.width - 1, PLATFORM_Y, PCD8544_Pixel_Clear, &PCD8544_Buffer);
		PCD8544_DrawFilledRectangle(ball.x_previous, ball.y, ball.x_previous + BALL_SIZE - 1, ball.y + BALL_SIZE, PCD8544_Pixel_Clear, &PCD8544_Buffer);
		PCD8544_DrawLine(platform.x, PLATFORM_Y, platform.x + platform.width - 1, PLATFORM_Y, PCD8544_Pixel_Set, &PCD8544_Buffer);
		PCD8544_DrawFilledRectangle(ball.x, ball.y, ball.x + BALL_SIZE - 1, ball.y + BALL_SIZE, PCD8544_Pixel_Set, &PCD8544_Buffer);
	}
}

void Sp_BallMove() {

	if(block_reflection)
		block_reflection = !block_reflection;

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
	ball.x += ball.raster * sign_x;
	ball.y += ball.raster * sign_y;

	if(ball.y > 0 && ball.y < PCD8544_HEIGHT - 2) {
		PCD8544_DrawFilledRectangle(ball.x_previous, ball.y_previous, ball.x_previous + BALL_SIZE - 1, ball.y_previous + BALL_SIZE, PCD8544_Pixel_Clear, &PCD8544_Buffer);
		PCD8544_DrawFilledRectangle(ball.x, ball.y, ball.x + BALL_SIZE - 1, ball.y + BALL_SIZE, PCD8544_Pixel_Set, &PCD8544_Buffer);
	}

	else if(ball.y >= PCD8544_HEIGHT - 2){
		sp_gameover = true;
	}
}

static bool InBlocksRange() {
	if((ball.x > 1 && ball.x < PCD8544_WIDTH - 3) && (ball.y > 1 && ball.y < BLOCK_ROWS*((BLOCK_SPACE + BLOCK_HEIGHT) + (BLOCK_SPACE + 1) )))
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
		else{
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
	if((ball.y == PCD8544_HEIGHT - 1 - BALL_SIZE) && (ball.x > 1 && ball.x < PCD8544_WIDTH - BALL_SIZE - 1))
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
	score += block_count;
}

static bool BlocksEqual(BlockID_t a, BlockID_t b) {
	return (a.col == b.col && a.row == b.row);
}


void Sp_Summary() {
	char score_buffer[3];
	sprintf(score_buffer, "%d", score);
	gotoXY(14, 26);
	PCD8544_Puts("GAME OVER", PCD8544_Pixel_Set, PCD8544_FontSize_5x7);
	gotoXY(22, 34);
	PCD8544_Puts("SCORE: ", PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
	gotoXY(50, 34);
	PCD8544_Puts(score_buffer, PCD8544_Pixel_Set, PCD8544_FontSize_3x5);
	PCD8544_Refresh();
}














