#include <stdio.h>
#include <stdbool.h>
#include <raylib.h>

//”√
#define SCREEN_WIDTH 900
#define SCREEN_HEIGHT 700
#define PLAYER_SPEED 200.0f//√ø√Î200œÒÀÿ
#define ENEMY_SPEED 150.0f
#define PLAYER_SIZE 32


typedef struct {
	float speed;
	Vector2 POS;
	Texture2D texture;
	Rectangle frameRec;
	bool isMoving;

}Player;

typedef struct  {
	float speed;
	Vector2 POS;
};

