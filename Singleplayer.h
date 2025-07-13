
#ifndef SINGLEPLAYER_H

#define SINGLEPLAYER_H

//platforma
#define PLATFORM_Y 47
#define PLATFORM_X_DEFAULT 38			//left pixel -> base
#define PLATFORM_WIDTH_DEFAULT 8

#define PLATFORM_VELOCITY_THREESHOLD_1 100
#define PLATFORM_VELOCITY_THREESHOLD_2 1000
#define PLATFORM_VELOCITY_THREESHOLD_3 1900

#define PLATFORM_VELOCITY_LOW 1
#define PLATFORM_VELOCITY_MID 2
#define PLATFORM_VELOCITY_HIGH 3

//piłka
#define BALL_X_DEFAULT 41				//top left pixel -> base
#define BALL_Y_DEFAULT 45
#define BALL_SIZE 2

//bloki
#define BLOCK_HEIGHT 2
#define BLOCK_WIDTH 3
#define BLOCK_SPACE 1
#define BLOCK_ROWS 7



typedef enum {
	DEG45,
	DEG135,
	DEG225,
	DEG315
} direction_t;

typedef struct {
	int x;
	int x_previous;
	int width;
	int velocity;
} platform_t;

typedef struct{
	int x;
	int y;
	int x_previous;
	int y_previous;
	int raster;
	direction_t direction;
} ball_t;

//collision points structure, [0] = x, [1] = y
typedef struct {
	int p1[2];
	int p2[2];
	int p3[2];
	int p4[2];
	int p5[2];
} collision_points_t;

typedef struct {
	int col;
	int row;
} BlockID_t;

extern bool sp_gameover;

void Sp_GameInit(void);
void Sp_PlatformMove(void);
void Sp_InitShift(void);
void Sp_BallMove(void);
void Sp_Summary(void);

#endif
