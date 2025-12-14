#include "raylib.h"
#include <math.h>

int main(void)
{
    // 初始化窗口
    const int screenWidth = 1000;
    const int screenHeight = 800;
    InitWindow(screenWidth, screenHeight, "CrocAttack");
    SetTargetFPS(60);

    // 加载精灵图
    Texture2D spriteSheet = LoadTexture("C:/Users/Administrator/Desktop/Knight/SeparateAnim/Walk.png");
	Texture2D background = LoadTexture("C:/Users/Administrator/Desktop/Knight/Dungeon_brick_wall_grey.png.png");

    // 精灵图参数
    const int frameWidth = 32;           // 每帧宽度
    const int frameHeight = 32;          // 每帧高度
    const int framesPerDirection = 4;    // 每个方向的动画帧数（假设4帧）
    const float frameTime = 0.12f;        // 每帧显示时间（秒）

    // 方向枚举（对应精灵图的列）
    enum Direction {
        DIR_DOWN = 0,   // 第0列：下
        DIR_UP = 1,     // 第1列：上
        DIR_LEFT = 2,   // 第2列：左
        DIR_RIGHT = 3   // 第3列：右
    };

    // 角色状态
    Vector2 position = { 400, 300 };    // 角色位置
    int direction = DIR_DOWN;         // 当前方向
    int currentFrame = 0;             // 当前动作帧
    float frameTimer = 0.0f;          // 帧计时器
    bool isMoving = false;            // 是否在移动
    float moveSpeed = 150.0f;         // 移动速度（像素/秒）

    // 主循环
    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();  // 获取帧间隔时间

        // 1. 处理输入和移动
        Vector2 movement = { 0, 0 };
        isMoving = false;

        // 按键检测（WASD控制）
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))
        {
            movement.y -= 1;
            direction = DIR_UP;      // 上方向
            isMoving = true;
        }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))
        {
            movement.y += 1;
            direction = DIR_DOWN;    // 下方向
            isMoving = true;
        }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        {
            movement.x -= 1;
            direction = DIR_LEFT;    // 左方向
            isMoving = true;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        {
            movement.x += 1;
            direction = DIR_RIGHT;   // 右方向
            isMoving = true;
        }

        // 2. 标准化对角线移动速度
        if (movement.x != 0 && movement.y != 0)
        {
            float length = sqrtf(movement.x * movement.x + movement.y * movement.y);
            movement.x /= length;
            movement.y /= length;
        }

        // 3. 更新位置
        position.x += movement.x * moveSpeed * deltaTime;
        position.y += movement.y * moveSpeed * deltaTime;

        // 4. 边界检查
        position.x = fmaxf(0, fminf(position.x, screenWidth - frameWidth));
        position.y = fmaxf(0, fminf(position.y, screenHeight - frameHeight));

        // 5. 更新动画
        if (isMoving)
        {
            // 移动时播放动画
            frameTimer += deltaTime;
            if (frameTimer >= frameTime)
            {
                frameTimer = 0.0f;
                currentFrame = (currentFrame + 1) % framesPerDirection;
            }
        }
        else
        {
            // 静止时重置到第一帧
            currentFrame = 0;
            frameTimer = 0.0f;
        }

        // 6. 计算源矩形（从精灵图中选择正确的帧）
        Rectangle sourceRect = {
            (float)direction * frameWidth,      // x: 根据方向选择列
            (float)currentFrame * frameHeight,  // y: 根据当前帧选择行
            frameWidth,
            frameHeight
        };

        // 7. 绘制
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // 绘制角色
        DrawTexture(background, 0, 0, WHITE);
        DrawTextureRec(spriteSheet, sourceRect, position, WHITE);

        EndDrawing();
    }

    // 清理资源
    UnloadTexture(spriteSheet);
    CloseWindow();

    return 0;
}