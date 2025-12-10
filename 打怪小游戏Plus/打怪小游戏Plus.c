#include<stdio.h>
#include<raylib.h>
int main(void)
{
	InitWindow(1500, 1394, u8"打怪小游戏Plus");

	SetTargetFPS(60);

	Texture2D bgTex = LoadTexture("Image/地图1x2.png");

	if (!IsTextureValid(bgTex)) {
		printf("图片加载失败\n");
	}

	Texture2D knightForward = LoadTexture("Image/向前.png");
	Texture2D knightBack = LoadTexture("Image/向后.png");
	Texture2D knightLeft = LoadTexture("Image/向左.png");
	Texture2D knightRight = LoadTexture("Image/向右.png");

	Vector2 pos = { 250,250 };


	while (!WindowShouldClose())
	{
		BeginDrawing();

		ClearBackground(WHITE);

		DrawTexture(bgTex, 0, 0, WHITE);

		DrawFPS(0, 0);

		DrawTextureV(knightForward, pos, WHITE);

		EndDrawing();

		if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
			pos.y -= 2;
		}
		else if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
			pos.y += 2;
		}
		else if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
			pos.x -= 2;
		}
		else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
			pos.x += 2;
		}
	}
	UnloadTexture(bgTex);
	CloseWindow();

	return 0;
}

