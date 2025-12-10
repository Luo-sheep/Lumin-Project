#include<stdio.h>
#include<raylib.h>
int main(void)
{
	InitWindow(1500, 1394, u8"打怪小游戏Plus");

	SetTargetFPS(60);

	Texture2D bgTex = LoadTexture("Image/主地图.png");
	Texture2D map1 = LoadTexture("Image/map1Plus.png");

	Image img = LoadImage("map1Plus.png");
	
	// 当前地图编号
	int currentMap = 0;
	// 当前显示的地图
	Texture2D currentBg = bgTex;


	// ⭐ 地图1中的传送门（传送到地图2）
	Rectangle portal1 = { 600, 300, 60, 60 };

	// ⭐ 地图2中的传送门（传送回地图1）
	Rectangle portal2 = { 100, 100, 60, 60 };

	Texture2D knightForward = LoadTexture("Image/向前.png");

	Vector2 pos = { 250,250 };

	bool canTeleport = true;


	while (!WindowShouldClose())
	{
		BeginDrawing();

		ClearBackground(WHITE);

		DrawTexture(currentBg, 0, 0, WHITE);

		DrawFPS(0, 0);

		DrawTextureV(knightForward, pos, WHITE);

		if (currentMap == 0)
			DrawRectangleRec(portal1, Fade(RED, 0.4f));
		else
			DrawRectangleRec(portal2, Fade(BLUE, 0.4f));


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

		// ---- 地图切换逻辑 ----
		if (currentMap == 0)
		{
			if (CheckCollisionPointRec(pos, portal1))
			{
				if (canTeleport) {
					currentMap = 1;
					currentBg = map1;
					pos = (Vector2){ 300, 300 };   // 远离传送点
					canTeleport = false;          // 冷却开始
				}
			}
			else {
				canTeleport = true; // 玩家离开传送门后，才能再次传送
			}
		}
		else if (currentMap == 1)
		{
			if (CheckCollisionPointRec(pos, portal2))
			{
				if (canTeleport) {
					currentMap = 0;
					currentBg = bgTex;
					pos = (Vector2){ 500, 400 };
					canTeleport = false;
				}
			}
			else {
				canTeleport = true;
			}
		}
	}
	UnloadTexture(bgTex);
	UnloadTexture(map1);
	UnloadTexture(knightForward);
	CloseWindow();

	return 0;
}

