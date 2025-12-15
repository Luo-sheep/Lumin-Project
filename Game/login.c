#include "raylib.h"
#include "sqlite3.h"
#include <stdio.h>
#include <string.h>

// 数据库
sqlite3* db = NULL;

// 初始化数据库
void init_db(void) {
    sqlite3_open("users.db", &db);

    const char* sql = "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "username TEXT UNIQUE, "
        "password TEXT);";
    sqlite3_exec(db, sql, NULL, NULL, NULL);
}

// 检查用户是否存在
int user_exists(const char* username) {
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id FROM users WHERE username = ?";

    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    int exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);

    return exists;
}

// 注册
int register_user(const char* username, const char* password) {
    if (user_exists(username)) return 0; // 用户名已存在

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?)";

    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);

    int success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    return success ? 1 : -1;
}

// 登录
int login_user(const char* username, const char* password) {
    if (!user_exists(username)) return -1; // 用户不存在

    sqlite3_stmt* stmt;
    const char* sql = "SELECT password FROM users WHERE username = ?";

    sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* db_pass = (const char*)sqlite3_column_text(stmt, 0);
        if (strcmp(db_pass, password) == 0) {
            sqlite3_finalize(stmt);
            return 0; // 登录成功
        }
    }

    sqlite3_finalize(stmt);
    return -2; // 密码错误
}

// 输入框
typedef struct {
    Rectangle rect;
    char text[256];
    bool active;
    bool isPassword;
} InputBox;

// 按钮
bool button(Rectangle rect, const char* text) {
    bool hover = CheckCollisionPointRec(GetMousePosition(), rect);
    bool click = hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

    DrawRectangleRec(rect, hover ? BLUE : DARKBLUE);
    DrawText(text, rect.x + 10, rect.y + 10, 20, WHITE);

    return click;
}

// 方法 A: 使用宏控制是否编译 main
#ifdef BUILD_LOGIN_MAIN
int main(void) {
#else
// 将原 main 改名为库函数，供其它模块调用（如果需要）
int login_main(void) {
#endif
    InitWindow(800, 600, "Login System");
    SetTargetFPS(60);

    init_db();

    // 输入框
    InputBox username_box = { {300, 200, 200, 40}, "", false, false };
    InputBox password_box = { {300, 270, 200, 40}, "", false, true };

    // 状态
    int screen = 0; // 0=登录, 1=注册, 2=主界面
    char message[100] = "";
    char current_user[100] = "";

    while (!WindowShouldClose()) {
        // 更新输入框
        Vector2 mouse = GetMousePosition();

        InputBox* boxes[] = { &username_box, &password_box };
        for (int i = 0; i < 2; i++) {
            if (CheckCollisionPointRec(mouse, boxes[i]->rect)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    boxes[i]->active = true;
                }
            }
            else if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                boxes[i]->active = false;
            }

            if (boxes[i]->active) {
                int key = GetCharPressed();
                while (key > 0 && strlen(boxes[i]->text) < 255) {
                    boxes[i]->text[strlen(boxes[i]->text)] = (char)key;
                    key = GetCharPressed();
                }

                if (IsKeyPressed(KEY_BACKSPACE) && strlen(boxes[i]->text) > 0) {
                    boxes[i]->text[strlen(boxes[i]->text) - 1] = '\0';
                }
            }
        }

        // 处理逻辑
        if (screen == 0) { // 登录界面
            if (button((Rectangle) { 350, 330, 100, 40 }, "Login") || IsKeyPressed(KEY_ENTER)) {
                int result = login_user(username_box.text, password_box.text);

                if (result == 0) {
                    strcpy(current_user, username_box.text);
                    screen = 2; // 进入主界面
                }
                else if (result == -1) {
                    strcpy(message, "User not found");
                }
                else {
                    strcpy(message, "Wrong password");
                }
            }

            if (button((Rectangle) { 350, 400, 100, 40 }, "Register")) {
                screen = 1;
                strcpy(message, "");
            }
        }
        else if (screen == 1) { // 注册界面
            if (button((Rectangle) { 350, 330, 100, 40 }, "Register") || IsKeyPressed(KEY_ENTER)) {
                int result = register_user(username_box.text, password_box.text);

                if (result == 1) {
                    strcpy(message, "Registration successful!");
                    username_box.text[0] = '\0';
                    password_box.text[0] = '\0';
                    screen = 0;
                }
                else if (result == 0) {
                    strcpy(message, "Username already exists");
                }
                else {
                    strcpy(message, "Registration failed");
                }
            }

            if (button((Rectangle) { 350, 400, 100, 40 }, "Back")) {
                screen = 0;
                strcpy(message, "");
            }
        }
        else if (screen == 2) { // 主界面
            if (button((Rectangle) { 350, 300, 100, 40 }, "Logout")) {
                screen = 0;
                username_box.text[0] = '\0';
                password_box.text[0] = '\0';
                current_user[0] = '\0';
            }
        }

        // 绘制
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (screen == 0) {
            DrawText("LOGIN", 370, 100, 40, BLACK);
        }
        else if (screen == 1) {
            DrawText("REGISTER", 350, 100, 40, BLACK);
        }
        else {
            DrawText("WELCOME", 350, 100, 40, BLACK);
            DrawText(TextFormat("Hello, %s", current_user), 300, 200, 30, BLUE);
        }

        // 绘制输入框
        for (int i = 0; i < 2; i++) {
            InputBox* box = boxes[i];

            DrawRectangleRec(box->rect, LIGHTGRAY);
            DrawRectangleLinesEx(box->rect, 2, box->active ? BLUE : DARKGRAY);

            const char* labels[] = { "Username:", "Password:" };
            DrawText(labels[i], box->rect.x, box->rect.y - 25, 20, DARKGRAY);

            if (box->isPassword) {
                char stars[256] = { 0 };
                for (int j = 0; j < strlen(box->text); j++) stars[j] = '*';
                DrawText(stars, box->rect.x + 10, box->rect.y + 10, 20, BLACK);
            }
            else {
                DrawText(box->text, box->rect.x + 10, box->rect.y + 10, 20, BLACK);
            }
        }

        // 显示消息
        DrawText(message, 300, 380, 20, RED);

        EndDrawing();
    }

    sqlite3_close(db);
    CloseWindow();
    return 0;
}