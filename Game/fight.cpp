
#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include <string>
#include <memory>
#include <filesystem>

#include "raylib.h"

// 定义全局常量
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PLAYER_SIZE = 48;
const int BOSS_SIZE = 80;  // 增加到80以适应纹理
const float BULLET_SIZE = 30.0f;  // 子弹大小

enum GameState {
    PLAYING,
    GAME_OVER,
    WIN
};

// 定义预警攻击阶段
enum AttackPhase
{
    WARNING,
    ACTIVE,
    COOLDOWN
};

// 资源管理器
class ResourceManager {
private:
    Texture2D bossTexture;
    Texture2D bulletTexture;
    Texture2D backgroundTexture;
    bool resourcesLoaded;

public:
    ResourceManager() : resourcesLoaded(false) {
        bossTexture = { 0 };
        bulletTexture = { 0 };
        backgroundTexture = { 0 };
    }

    bool LoadResources() {
        // 尝试加载敌人纹理
        bossTexture = LoadTexture("../enemy.png");
        if (bossTexture.id == 0) {
            TraceLog(LOG_WARNING, "无法加载 enemy.png，将使用默认图形");
            // 可以在这里创建一个默认纹理
        }

        // 尝试加载子弹纹理（GIF）
        bulletTexture = LoadTexture("../bullet.png");
        if (bulletTexture.id == 0) {
            TraceLog(LOG_WARNING, "无法加载 bullet.png，将使用默认图形");
        }

        // 加载背景纹理
        backgroundTexture = LoadTexture("../background.png");
        if (backgroundTexture.id == 0) {
            TraceLog(LOG_WARNING, "无法加载背景纹理，将使用颜色背景");
        }

        resourcesLoaded = true;
        return resourcesLoaded;
    }

    Texture2D GetBossTexture() const { return bossTexture; }
    Texture2D GetBulletTexture() const { return bulletTexture; }
    Texture2D GetBackgroundTexture() const { return backgroundTexture; }

    void UnloadResources() {
        if (bossTexture.id != 0) UnloadTexture(bossTexture);
        if (bulletTexture.id != 0) UnloadTexture(bulletTexture);
        if (backgroundTexture.id != 0) UnloadTexture(backgroundTexture);
        resourcesLoaded = false;
    }

    ~ResourceManager() {
        UnloadResources();
    }
};

// 全局资源管理器
static ResourceManager resourceManager;

// 方向定义（用于玩家动画）
enum PlayerDirection {
    DIR_DOWN = 0,   // 第1列：下
    DIR_UP = 1,     // 第2列：上
    DIR_LEFT = 2,   // 第3列：左
    DIR_RIGHT = 3   // 第4列：右
};

//基类攻击
class Attack
{
    //基类基础属性
protected:
    AttackPhase phase;
    int timer;
    float x, y;
    float radius;
    float damage;

    //构造函数与析构函数
public:
    Attack(float cx, float cy, float r, float dmg = 1.0f)
        :phase(WARNING), timer(0), x(cx), y(cy), radius(r), damage(dmg) {
    }
    virtual ~Attack() {}

    //更新计时器状态
    virtual void update(float playerX, float playerY)
    {
        timer++;
    }

    //绘制攻击
    virtual void draw() = 0;

    //碰撞检测
    virtual bool checkCollision(float playerX, float playerY, float playerSize)
    {
        return false;
    }

    //安全性之通过函数接口访问类成员
    AttackPhase getPhase() const
    {
        return phase;
    }

    float getDamage() const
    {
        return damage;
    }

    float getX() const { return x; }
    float getY() const { return y; }
    float getRadius() const { return radius; }

    //计算与玩家距离
protected:
    float distanceTo(float playerX, float playerY)
    {
        float dx = playerX - x;
        float dy = playerY - y;
        return sqrtf(dx * dx + dy * dy);
    }

};

//派生类攻击
class CircleAttack :public Attack
{
private:
    int warningTime;
    int activeTime;

public:
    CircleAttack(float cx, float cy, float r, float dmg = 25.0f)
        :Attack(cx, cy, r, dmg), warningTime(75), activeTime(25) {
    }

    void update(float playerX, float playerY) override
    {
        Attack::update(playerX, playerY);

        if (phase == WARNING && timer > warningTime)
        {
            phase = ACTIVE;
            timer = 0;
        }
        else if (phase == ACTIVE && timer > activeTime)
        {
            phase = COOLDOWN;
            timer = 0;
        }
    }

    void draw() override
    {
        // WARNING: 显示黄色圆环（外圈）
        if (phase == WARNING)
        {
            DrawCircleLines((int)x, (int)y, radius, BLACK);
            DrawText("!", (int)x - 6, (int)y - 6, 20, YELLOW);
        }
        // ACTIVE: 红色填充圈
        else if (phase == ACTIVE)
        {
            DrawCircle((int)x, (int)y, radius, Fade(RED, 0.25f));
            DrawCircleLines((int)x, (int)y, radius, RED);
        }
        // COOLDOWN: 轻微灰色淡化
        else if (phase == COOLDOWN)
        {
            DrawCircleLines((int)x, (int)y, radius, LIGHTGRAY);
        }
    }

    bool checkCollision(float playerX, float playerY, float playerSize) override
    {
        if (phase != ACTIVE)
        {
            return false;
        }
        return distanceTo(playerX, playerY) < (radius + playerSize / 2.0f);
    }

};

//可反弹攻击类型 - 修改为使用纹理
class BounceBulletAttack : public Attack
{
private:
    float speed;           // 子弹速度
    float directionX;      // X方向向量
    float directionY;      // Y方向向量
    int bounceCount;       // 当前反弹次数
    int maxBounces;        // 最大反弹次数
    float rotation;        // 旋转角度
    float rotationSpeed;   // 旋转速度（每秒弧度）
    int warningTime;       // 预警时间（帧数）

public:
    BounceBulletAttack(float cx, float cy, float dirX, float dirY, float spd = 10.0f, int maxBounce = 4, float dmg = 15.0f)
        : Attack(cx, cy, BULLET_SIZE / 2.0f, dmg),  // 子弹半径为纹理大小的一半
        directionX(dirX), directionY(dirY),
        speed(spd), bounceCount(0), maxBounces(maxBounce),
        rotation(0.0f), rotationSpeed(0.1f), warningTime(60) // 预警时间设为60帧（1秒）
    {
        phase = WARNING;  // 改为先进入预警阶段
    }

    void update(float playerX, float playerY) override
    {
        Attack::update(playerX, playerY);

        // 预警阶段结束后进入激活阶段
        if (phase == WARNING && timer > warningTime)
        {
            phase = ACTIVE;
            timer = 0;  // 重置计时器
        }
        // 激活阶段才更新位置和旋转
        else if (phase == ACTIVE)
        {
            // 更新位置
            x += directionX * speed;
            y += directionY * speed;

            // 更新旋转
            rotation += rotationSpeed;
            if (rotation > 2 * M_PI) rotation -= 2 * M_PI;

            // 边界检测与反弹
            checkWallCollision();

            // 超过最大反弹次数或飞出屏幕过远则进入冷却（销毁）
            if (bounceCount >= maxBounces || isOutOfBounds())
            {
                phase = COOLDOWN;
            }
        }
    }

    void draw() override
    {
        Texture2D bulletTex = resourceManager.GetBulletTexture();

        if (phase == WARNING)
        {
            // 预警阶段在BOSS位置显示"!"
            DrawText("!", (int)x - 6, (int)y - 6, 20, YELLOW);

            // 绘制延伸至屏幕边缘的方向指示器（全屏幕长度）
            // 计算射线与屏幕边缘的交点
            float t;
            float edgeX, edgeY;

            // 计算射线与屏幕边界的交点
            if (directionX > 0)
            {
                t = (SCREEN_WIDTH - x) / directionX;
            }
            else if (directionX < 0)
            {
                t = (-x) / directionX;
            }
            else
            {
                t = FLT_MAX;
            }

            // 检查Y方向边界
            float tY;
            if (directionY > 0)
            {
                tY = (SCREEN_HEIGHT - y) / directionY;
            }
            else if (directionY < 0)
            {
                tY = (-y) / directionY;
            }
            else
            {
                tY = FLT_MAX;
            }

            // 取较小的t值，确定先与哪个边界相交
            t = fminf(t, tY);
            edgeX = x + directionX * t;
            edgeY = y + directionY * t;

            // 绘制从起点到屏幕边缘的预警线
            DrawLineV({ x, y }, { edgeX, edgeY }, Fade(BLACK, 0.5f));
        }
        else if (phase == ACTIVE)
        {
            if (bulletTex.id != 0)
            {
                // 绘制子弹纹理（旋转）
                Rectangle sourceRec = { 0, 0, (float)bulletTex.width, (float)bulletTex.height };
                Rectangle destRec = { x, y, BULLET_SIZE, BULLET_SIZE };
                Vector2 origin = { BULLET_SIZE / 2, BULLET_SIZE / 2 };

                DrawTexturePro(bulletTex, sourceRec, destRec, origin, rotation * RAD2DEG, WHITE);
            }
            else
            {
                // 如果纹理加载失败，使用三角形作为后备
                Vector2 points[3];
                float triSize = radius * 1.5;

                points[0] = { x + cosf(rotation) * triSize, y + sinf(rotation) * triSize };
                points[1] = { x + cosf(rotation + 2 * M_PI / 3) * triSize, y + sinf(rotation + 2 * M_PI / 3) * triSize };
                points[2] = { x + cosf(rotation + 4 * M_PI / 3) * triSize, y + sinf(rotation + 4 * M_PI / 3) * triSize };

                DrawTriangle(points[0], points[1], points[2], ORANGE);
                DrawTriangleLines(points[0], points[1], points[2], ORANGE);
            }
        }
    }

    bool checkCollision(float playerX, float playerY, float playerSize) override
    {
        if (phase != ACTIVE) return false;
        return distanceTo(playerX, playerY) < (radius + playerSize / 2.0f);
    }

private:
    void checkWallCollision()
    {
        // 左右边界反弹
        if (x - radius < 0)
        {
            x = radius;
            directionX = -directionX;
            bounceCount++;
        }
        else if (x + radius > SCREEN_WIDTH)
        {
            x = SCREEN_WIDTH - radius;
            directionX = -directionX;
            bounceCount++;
        }

        // 上下边界反弹
        if (y - radius < 0)
        {
            y = radius;
            directionY = -directionY;
            bounceCount++;
        }
        else if (y + radius > SCREEN_HEIGHT)
        {
            y = SCREEN_HEIGHT - radius;
            directionY = -directionY;
            bounceCount++;
        }
    }

    bool isOutOfBounds()
    {
        return x < -50 || x > SCREEN_WIDTH + 50 || y < -50 || y > SCREEN_HEIGHT + 50;
    }
};

// 新增瞄准圆形攻击类
class AimedCircleAttack : public Attack
{
private:
    int warningTime;  // 短预警时间
    int activeTime;

public:
    AimedCircleAttack(float cx, float cy, float r, float dmg = 15.0f)
        : Attack(cx, cy, r, dmg), warningTime(30), activeTime(20) {
    }  // 短预警时间（0.5秒）

    void update(float playerX, float playerY) override
    {
        Attack::update(playerX, playerY);

        if (phase == WARNING && timer > warningTime)
        {
            phase = ACTIVE;
            timer = 0;
        }
        else if (phase == ACTIVE && timer > activeTime)
        {
            phase = COOLDOWN;
            timer = 0;
        }
    }

    void draw() override
    {
        if (phase == WARNING)
        {
            DrawCircleLines((int)x, (int)y, radius, ORANGE);
            DrawText("!", (int)x - 6, (int)y - 6, 16, ORANGE);
        }
        else if (phase == ACTIVE)
        {
            DrawCircle((int)x, (int)y, radius, Fade(ORANGE, 0.3f));
            DrawCircleLines((int)x, (int)y, radius, ORANGE);
        }
        else if (phase == COOLDOWN)
        {
            DrawCircleLines((int)x, (int)y, radius, LIGHTGRAY);
        }
    }

    bool checkCollision(float playerX, float playerY, float playerSize) override
    {
        if (phase != ACTIVE)
            return false;
        return distanceTo(playerX, playerY) < (radius + playerSize / 2.0f);
    }
};

//基类Boss
class Boss
{
protected:
    float x, y;
    float hp, maxHp;
    std::string name;
    int attackDelay;
    bool damageCooldown;
    int damageTimer;
    std::vector<std::shared_ptr<Attack>> attacks;

public:
    Boss(float cx, float cy, float health, const std::string n)
        :x(cx), y(cy), hp(health), maxHp(health), name(n), attackDelay(0), damageCooldown(false), damageTimer(35) {
    }
    virtual ~Boss() {}

    virtual void update(float playerX, float playerY, float playerHp)
    {
        if (attackDelay > 0)
        {
            attackDelay--;
        }
        if (attackDelay == 0)
        {
            doAttack(playerX, playerY);
            attackDelay = getAttackDelay();
        }

        //以下小段涉及攻击生成部分正在研究
        for (auto it = attacks.begin(); it != attacks.end();)
        {
            (*it)->update(playerX, playerY);

            if ((*it)->getPhase() == COOLDOWN)
            {
                it = attacks.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (damageCooldown)
        {
            damageTimer--;
            if (damageTimer <= 0)
            {
                damageCooldown = false;
                damageTimer = 35;
            }
        }
    }

    virtual void draw()
    {
        Texture2D bossTex = resourceManager.GetBossTexture();

        if (bossTex.id != 0)
        {
            // 绘制Boss纹理
            Rectangle sourceRec = { 0, 0, (float)bossTex.width, (float)bossTex.height };
            Rectangle destRec = { x - BOSS_SIZE / 2, y - BOSS_SIZE / 2, BOSS_SIZE, BOSS_SIZE };
            Vector2 origin = { 0, 0 };

            // 根据伤害冷却状态调整颜色
            Color tint = damageCooldown ? RED : WHITE;
            DrawTexturePro(bossTex, sourceRec, destRec, origin, 0.0f, tint);
        }
        else
        {
            // 后备：绘制圆形Boss
            Color bossColor = damageCooldown ? RED : MAROON;
            DrawCircle((int)x, (int)y, BOSS_SIZE / 2, bossColor);
        }

        // 名称与血条
        std::string hpText = name + " HP: " + std::to_string((int)hp);
        DrawText(hpText.c_str(), (int)x - 40, (int)y - BOSS_SIZE / 2 - 20, 10, RAYWHITE);

        // 血条
        float barWidth = 80;
        float hpRatio = (maxHp > 0) ? (hp / maxHp) : 0;
        DrawRectangle((int)(x - barWidth / 2), (int)(y + BOSS_SIZE / 2 + 6), (int)barWidth, 6, DARKGRAY);
        DrawRectangle((int)(x - barWidth / 2), (int)(y + BOSS_SIZE / 2 + 6), (int)(barWidth * hpRatio), 6, RED);
    }

    void drawAttacks()
    {
        for (auto& attack : attacks)
        {
            attack->draw();
        }
    }

    bool checkHit(float playerX, float playerY, float playerSize)
    {
        for (auto& attack : attacks)
        {
            if (attack->checkCollision(playerX, playerY, playerSize))
            {
                return true;
            }
        }
        return false;
    }

    // 提供访问攻击以便处理伤害
    const std::vector<std::shared_ptr<Attack>>& getAttacks() const
    {
        return attacks;
    }

    void takeDamage(float damage)
    {
        if (!damageCooldown)
        {
            hp -= damage;
            damageCooldown = true;
            if (hp < 0)
            {
                hp = 0;
            }
        }
    }

    float getX() const
    {
        return x;
    }
    float getY() const
    {
        return y;
    }
    float getHp() const
    {
        return hp;
    }
    std::string getName() const
    {
        return name;
    }

    virtual void doAttack(float playerX, float playerY) = 0;
    virtual int getAttackDelay() = 0;
};

class Boss01 : public Boss
{
private:
    int attackPattern;
    int aimedAttackCounter;  // 瞄准攻击计数器
    bool isDoingAimedAttack; // 是否正在进行连续瞄准攻击
    int chainAttackInterval; // 连续攻击的间隔

public:
    Boss01(float cx, float cy)
        : Boss(cx, cy, 170.0f, "Boss01"), attackPattern(0),
        aimedAttackCounter(0), isDoingAimedAttack(false), chainAttackInterval(0)
    {
        attackDelay = getAttackDelay() / 2;
    }

    // 计算与玩家的距离
    float getDistanceToPlayer(float playerX, float playerY)
    {
        float dx = playerX - x;
        float dy = playerY - y;
        return sqrtf(dx * dx + dy * dy);
    }

    virtual void update(float playerX, float playerY, float playerHp) override
    {
        // 重写update方法，单独处理连续攻击的间隔逻辑
        if (isDoingAimedAttack)
        {
            if (chainAttackInterval > 0)
            {
                chainAttackInterval--;
            }
            else
            {
                // 到达间隔时间，生成新攻击
                doAimedAttack(playerX, playerY);
            }
        }
        else
        {
            // 普通攻击间隔逻辑
            if (attackDelay > 0)
            {
                attackDelay--;
            }
            if (attackDelay == 0)
            {
                doAttack(playerX, playerY);
                attackDelay = getAttackDelay();
            }
        }

        // 处理攻击更新
        for (auto it = attacks.begin(); it != attacks.end();)
        {
            (*it)->update(playerX, playerY);

            if ((*it)->getPhase() == COOLDOWN)
            {
                it = attacks.erase(it);
            }
            else
            {
                ++it;
            }
        }

        if (damageCooldown)
        {
            damageTimer--;
            if (damageTimer <= 0)
            {
                damageCooldown = false;
                damageTimer = 35;
            }
        }
    }

    // 单独的连续瞄准攻击生成函数
    void doAimedAttack(float playerX, float playerY)
    {
        float r = 50.0f;
        auto atk = std::make_shared<AimedCircleAttack>(playerX, playerY, r, 10.0f);
        attacks.push_back(atk);

        aimedAttackCounter++;

        if (aimedAttackCounter >= 8)
        {
            // 结束连续攻击
            isDoingAimedAttack = false;
            aimedAttackCounter = 0;
            attackDelay = getAttackDelay(); // 恢复普通攻击间隔
        }
        else
        {
            // 关键修改：将连续攻击间隔缩短到10帧
            // 攻击生命周期40帧，间隔10帧，会同时存在4个重叠攻击
            chainAttackInterval = 10;
        }
    }

    virtual void doAttack(float playerX, float playerY) override
    {
        float distance = getDistanceToPlayer(playerX, playerY);

        // 远距离：使用瞄准攻击
        if (distance > 100.0f)
        {
            if (attackPattern % 3 == 0)  // 每3次攻击使用1次反弹子弹
            {
                // 反弹子弹逻辑保持不变...
                float dx = playerX - x;
                float dy = playerY - y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist > 0)
                {
                    dx /= dist;
                    dy /= dist;
                }

                for (int i = -1; i <= 1; i++)
                {
                    float angleOffset = i * 0.2f;
                    float dirX = dx * cosf(angleOffset) - dy * sinf(angleOffset);
                    float dirY = dx * sinf(angleOffset) + dy * cosf(angleOffset);
                    auto bullet = std::make_shared<BounceBulletAttack>(x, y, dirX, dirY, 3.5f, 8);
                    attacks.push_back(bullet);
                }
            }
            else  // 开始连续瞄准攻击
            {
                isDoingAimedAttack = true;
                aimedAttackCounter = 0;
                chainAttackInterval = 0; // 立即生成第一个攻击
            }
        }
        else  // 近距离攻击逻辑保持不变
        {
            if (attackPattern % 3 == 0)
            {
                float dx = playerX - x;
                float dy = playerY - y;
                float dist = sqrtf(dx * dx + dy * dy);
                if (dist > 0)
                {
                    dx /= dist;
                    dy /= dist;
                }

                for (int i = -1; i <= 1; i++)
                {
                    float angleOffset = i * 0.2f;
                    float dirX = dx * cosf(angleOffset) - dy * sinf(angleOffset);
                    float dirY = dx * sinf(angleOffset) + dy * cosf(angleOffset);
                    auto bullet = std::make_shared<BounceBulletAttack>(x, y, dirX, dirY, 3.5f, 8);
                    attacks.push_back(bullet);
                }
            }
            else
            {
                float r = 175.0f;
                float dmg = 25.0f;
                auto atk = std::make_shared<CircleAttack>(x, y, r, dmg);
                attacks.push_back(atk);
            }
        }

        attackPattern++;
    }

    virtual int getAttackDelay() override
    {
        return 180;  // 基础攻击间隔保持不变
    }
};

//玩家类 - 修改为使用纹理动画
class Player
{
private:
    float x, y;
    float hp, maxHp;
    float speed;
    bool isAttacking;
    int attackTimer;
    bool damageCooldown;
    int damageTimer;
    float attackDirX;  // 攻击方向X
    float attackDirY;  // 攻击方向Y
    bool attackDisplaced;  // 是否已经完成攻击位移

    // 动画相关
    Texture2D texture;
    Rectangle frameRec;
    bool isMoving;
    int currentFrame;
    float frameTimer;
    PlayerDirection direction;

public:
    Player()
        :x(100), y(100), hp(220), maxHp(220), speed(5), isAttacking(false),
        attackTimer(0), damageCooldown(false), damageTimer(35),
        attackDirX(0), attackDirY(0), attackDisplaced(false),
        texture({ 0 }), frameRec({ 0, 0, PLAYER_SIZE, PLAYER_SIZE }),
        isMoving(false), currentFrame(0), frameTimer(0.0f), direction(DIR_DOWN) {
    }

    bool LoadTexture(const char* path) {
        Image img = LoadImage(path);
        texture = LoadTextureFromImage(img);
        UnloadImage(img);
        return texture.id != 0;
    }

    void update()
    {
        float deltaTime = GetFrameTime();

        // 处理移动输入和动画
        Vector2 movement = { 0, 0 };
        isMoving = false;

        if (IsKeyDown(KEY_W))
        {
            movement.y -= 1;
            direction = DIR_UP;
            isMoving = true;
        }
        if (IsKeyDown(KEY_S))
        {
            movement.y += 1;
            direction = DIR_DOWN;
            isMoving = true;
        }
        if (IsKeyDown(KEY_A))
        {
            movement.x -= 1;
            direction = DIR_LEFT;
            isMoving = true;
        }
        if (IsKeyDown(KEY_D))
        {
            movement.x += 1;
            direction = DIR_RIGHT;
            isMoving = true;
        }

        // 标准化对角线速度
        if (movement.x != 0 && movement.y != 0)
        {
            float length = sqrtf(movement.x * movement.x + movement.y * movement.y);
            movement.x /= length;
            movement.y /= length;
        }

        Vector2 attackDirection = { 0, 0 };
        if (movement.x == 0 && movement.y == 0) {
            // 没有移动输入时，使用当前面向的方向
            switch (direction) {
            case DIR_UP: attackDirection = { 0, -1 }; break;
            case DIR_DOWN: attackDirection = { 0, 1 }; break;
            case DIR_LEFT: attackDirection = { -1, 0 }; break;
            case DIR_RIGHT: attackDirection = { 1, 0 }; break;
            }
        }
        else {
            // 标准化移动向量
            float len = sqrtf(movement.x * movement.x + movement.y * movement.y);
            if (len > 0) {
                attackDirection.x = movement.x / len;
                attackDirection.y = movement.y / len;
            }
        }

        // 攻击逻辑
        if (IsKeyPressed(KEY_SPACE) && !isAttacking)
        {
            isAttacking = true;
            attackTimer = 15;
            attackDirX = attackDirection.x;  
            attackDirY = attackDirection.y;
            attackDisplaced = false;
        }

        // 攻击过程处理
        if (isAttacking)
        {
            // 攻击开始时执行一次位移
            if (!attackDisplaced) {
                x += attackDirX * 15;
                y += attackDirY * 15;
                attackDisplaced = true;
            }

            attackTimer--;
            if (attackTimer <= 0)
            {
                isAttacking = false;
            }
        }
        // 非攻击状态下的移动
        else if(isMoving)
        {
            x += movement.x * speed;
            y += movement.y * speed;
        }

        // 边界检测
        float playerRadius = PLAYER_SIZE / 2.0f;
        x = std::max(playerRadius, std::min((float)SCREEN_WIDTH - playerRadius, x));
        y = std::max(playerRadius, std::min((float)SCREEN_HEIGHT - playerRadius, y));

        // 更新动画帧
        if (isMoving && texture.id != 0)
        {
            frameTimer += deltaTime;

            // 每0.1秒切换一帧
            if (frameTimer >= 0.1f)
            {
                frameTimer = 0.0f;
                currentFrame++;
                if (currentFrame >= 4)  // 假设有4帧动画
                    currentFrame = 0;
            }
        }
        else
        {
            // 静止时使用第一帧
            currentFrame = 0;
            frameTimer = 0.0f;
        }

        // 更新帧矩形（假设纹理是4列x4行的精灵图）
        if (texture.id != 0) {
            int textureFramesPerRow = texture.width / PLAYER_SIZE;
            frameRec.x = (float)direction * PLAYER_SIZE;
            frameRec.y = (float)currentFrame * PLAYER_SIZE;
        }

        if (damageCooldown)
        {
            damageTimer--;
            if (damageTimer <= 0)
            {
                damageCooldown = false;
                damageTimer = 35;
            }
        }
    }

    void draw()
    {
        Color playerColor = damageCooldown ? RED : BLUE;

        // 绘制玩家纹理
        if (texture.id != 0) {
            DrawTextureRec(texture, frameRec, { x - PLAYER_SIZE / 2, y - PLAYER_SIZE / 2 }, playerColor);
        }
        else {
            DrawCircle((int)x, (int)y, PLAYER_SIZE / 2, playerColor);
        }

        // 绘制半圆形攻击范围
        if (isAttacking)
        {
            const int segments = 20; // 半圆的线段数量
            const float radius = PLAYER_SIZE / 2 + 25;  // 攻击范围半径
            const float startAngle = atan2f(attackDirY, attackDirX) - M_PI / 2;  // 半圆起始角度
            const float endAngle = startAngle + M_PI;  // 半圆结束角度（180度）

            // 绘制半圆弧线
            for (int i = 0; i < segments; i++)
            {
                float angle1 = startAngle + (endAngle - startAngle) * i / segments;
                float angle2 = startAngle + (endAngle - startAngle) * (i + 1) / segments;

                Vector2 p1 = {
                    x + cosf(angle1) * radius,
                    y + sinf(angle1) * radius
                };
                Vector2 p2 = {
                    x + cosf(angle2) * radius,
                    y + sinf(angle2) * radius
                };
                DrawLineV(p1, p2, GREEN);
            }

            // 绘制连接玩家到半圆两端的线段
            Vector2 end1 = {
                x + cosf(startAngle) * radius,
                y + sinf(startAngle) * radius
            };
            Vector2 end2 = {
                x + cosf(endAngle) * radius,
                y + sinf(endAngle) * radius
            };
            DrawLineV({ x, y }, end1, GREEN);
            DrawLineV({ x, y }, end2, GREEN);
        }

        // 血条绘制
        float barW = 80;
        float hpR = (maxHp > 0) ? (hp / maxHp) : 0;
        DrawRectangle((int)(x - barW / 2), (int)(y + PLAYER_SIZE / 2 + 6), (int)barW, 6, DARKGRAY);
        DrawRectangle((int)(x - barW / 2), (int)(y + PLAYER_SIZE / 2 + 6), (int)(barW * hpR), 6, GREEN);
    }

    bool checkHit(float bossX, float bossY, float bossSize)
    {
        if (!isAttacking) return false;

        // 计算Boss相对于玩家的位置
        float dx = bossX - x;
        float dy = bossY - y;
        float dist = sqrtf(dx * dx + dy * dy);
        float bossRadius = bossSize / 2;
        float attackRadius = PLAYER_SIZE / 2 + 20;

        // 距离检测
        if (dist > attackRadius + bossRadius) return false;

        // 角度检测（是否在半圆形攻击范围内）
        float bossAngle = atan2f(dy, dx);
        float attackStartAngle = atan2f(attackDirY, attackDirX) - M_PI / 2;
        float attackEndAngle = attackStartAngle + M_PI;

        // 处理角度环绕问题
        if (attackStartAngle < 0) attackStartAngle += 2 * M_PI;
        if (attackEndAngle < 0) attackEndAngle += 2 * M_PI;
        if (bossAngle < 0) bossAngle += 2 * M_PI;

        bool inAngleRange;
        if (attackStartAngle <= attackEndAngle)
        {
            inAngleRange = (bossAngle >= attackStartAngle && bossAngle <= attackEndAngle);
        }
        else
        {
            inAngleRange = (bossAngle >= attackStartAngle || bossAngle <= attackEndAngle);
        }

        return inAngleRange;
    }

    // 其他原有方法保持不变...
    void takeDamage(float damage)
    {
        if (!damageCooldown)
        {
            hp -= damage;
            damageCooldown = true;
            if (hp < 0) hp = 0;
        }
    }

    float getHp() const { return hp; }
    float getX() const { return x; }
    float getY() const { return y; }
};

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Lumin Project - Boss Battle");
    SetTargetFPS(60);

    // 初始化资源管理器
    if (!resourceManager.LoadResources()) {
        TraceLog(LOG_INFO, "部分资源加载失败，将使用默认图形");
    }

    Player player;
    Boss01 boss(SCREEN_WIDTH / 2.0f, 120.0f);
    GameState gameState = PLAYING;  // 初始化游戏状态为正在播放

    // 加载玩家纹理
    player.LoadTexture("../walk.png");

    // 把玩家放在屏幕中间偏下
    player = Player();
    // 使用局部时间控制（本示例仍使用帧计数器）
    player.LoadTexture("../walk.png");

    while (!WindowShouldClose())
    {
        // 检查游戏是否应该结束
        if (player.getHp() <= 0)
        {
            gameState = GAME_OVER;
        }
        else if (boss.getHp() <= 0)
        {
            gameState = WIN;
        }

        // 只有在游戏进行中才更新逻辑
        if (gameState == PLAYING)
        {
            // 逻辑更新
            player.update();
            boss.update(player.getX(), player.getY(), player.getHp());

            // Boss 的攻击命中检测：遍历每个攻击并应用伤害（若在 ACTIVE 且碰撞）
            for (auto& atk : boss.getAttacks())
            {
                if (atk->checkCollision(player.getX(), player.getY(), PLAYER_SIZE))
                {
                    player.takeDamage(atk->getDamage());
                    // 暂不立即移除攻击，攻击生命周期由 Attack 管理
                }
            }

            // 玩家攻击命中 Boss
            if (player.checkHit(boss.getX(), boss.getY(), BOSS_SIZE))
            {
                boss.takeDamage(15.0f);
            }
        }

        // 渲染
        BeginDrawing();

        // 绘制背景
        Texture2D bgTex = resourceManager.GetBackgroundTexture();
        if (bgTex.id != 0) {
            // 平铺背景
            for (int y = 0; y < SCREEN_HEIGHT; y += bgTex.height) {
                for (int x = 0; x < SCREEN_WIDTH; x += bgTex.width) {
                    DrawTexture(bgTex, x, y, Fade(WHITE, 0.8f));
                }
            }
        }
        else {
            ClearBackground({ 30, 30, 30, 255 });
        }

        // 绘制游戏对象
        boss.draw();
        boss.drawAttacks();
        player.draw();

        // HUD
        DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 12, DARKGRAY);
        DrawText(TextFormat("Player HP: %d", (int)player.getHp()), 10, 30, 12, DARKGRAY);
        DrawText(TextFormat("%s HP: %d", boss.getName().c_str(), (int)boss.getHp()), 10, 50, 12, DARKGRAY);

        // 若任一死亡，显示结束信息
        if (gameState == GAME_OVER)
        {
            if (player.getHp() <= 0)
            {
                DrawText("You Died", SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 - 10, 20, RED);
            }
            else if (boss.getHp() <= 0)
            {
                DrawText("Boss Defeated!", SCREEN_WIDTH / 2 - 90, SCREEN_HEIGHT / 2 - 10, 20, GREEN);
            }
            DrawText("Press ESC to exit", SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 + 30, 16, DARKGRAY);
        }

        EndDrawing();
    }

    // 清理资源
    resourceManager.UnloadResources();
    CloseWindow();

    return 0;
}