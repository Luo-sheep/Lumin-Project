// Lumin Project.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdio>
#include <string>
#include <memory>

#include "raylib.h"

//定义全局常量
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int PLAYER_SIZE = 20;
const int BOSS_SIZE = 60;


//定义攻击类型枚举
enum AttackType
{
	CIRCLE_ATTACK
};

//定义预警攻击阶段
enum AttackPhase
{
	WARNING,
	ACTIVE,
	COOLDOWN
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
	CircleAttack(float cx, float cy, float r, float dmg = 20.0f)
		:Attack(cx, cy, r, dmg), warningTime(90), activeTime(30) {
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

//其他攻击类型
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

public:
	BounceBulletAttack(float cx, float cy, float dirX, float dirY, float spd = 3.0f, int maxBounce = 3, float dmg = 5.0f)
		: Attack(cx, cy, 8.0f, dmg),  // 子弹半径为8
		directionX(dirX), directionY(dirY),
		speed(spd), bounceCount(0), maxBounces(maxBounce),
		rotation(0.0f), rotationSpeed(0.1f)  // 自转速度
	{
		phase = ACTIVE;  // 子弹没有预警阶段，直接激活
	}

	void update(float playerX, float playerY) override
	{
		Attack::update(playerX, playerY);

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

	void draw() override
	{
		if (phase != ACTIVE) return;

		// 绘制旋转的三角形子弹
		Vector2 points[3];
		float triSize = radius * 2;  // 三角形大小

		// 计算旋转后的三角形顶点
		points[0] = { x + cosf(rotation) * triSize, y + sinf(rotation) * triSize };
		points[1] = { x + cosf(rotation + 2 * M_PI / 3) * triSize, y + sinf(rotation + 2 * M_PI / 3) * triSize };
		points[2] = { x + cosf(rotation + 4 * M_PI / 3) * triSize, y + sinf(rotation + 4 * M_PI / 3) * triSize };

		DrawTriangle(points[0], points[1], points[2], ORANGE);
		DrawTriangleLines(points[0], points[1], points[2], ORANGE);
	}

	bool checkCollision(float playerX, float playerY, float playerSize) override
	{
		if (phase != ACTIVE) return false;
		return distanceTo(playerX, playerY) < (radius + playerSize / 2.0f);
	}

private:
	// 检测墙壁碰撞并反弹
	void checkWallCollision()
	{
		// 左右边界反弹
		if (x - radius < 0)
		{
			x = radius;  // 防止卡在墙内
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

	// 检测是否飞出屏幕过远（防止反弹后无限循环）
	bool isOutOfBounds()
	{
		return x < -50 || x > SCREEN_WIDTH + 50 || y < -50 || y > SCREEN_HEIGHT + 50;
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
		//绘制Boss各种东西
		DrawCircle((int)x, (int)y, BOSS_SIZE / 2, MAROON);
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
	int attackPattern;  // 攻击模式切换

public:
	Boss01(float cx, float cy)
		: Boss(cx, cy, 200.0f, "Boss01"), attackPattern(0)
	{
		attackDelay = getAttackDelay() / 2;
	}

	// 改进攻击逻辑，交替使用两种攻击模式
	virtual void doAttack(float playerX, float playerY) override
	{
		if (attackPattern % 3 == 0)  // 每3次攻击使用1次反弹子弹
		{
			// 计算朝向玩家的方向
			float dx = playerX - x;
			float dy = playerY - y;
			float dist = sqrtf(dx * dx + dy * dy);
			if (dist > 0)
			{
				dx /= dist;  // 标准化方向向量
				dy /= dist;
			}

			// 生成3个略微分散的反弹子弹
			for (int i = -1; i <= 1; i++)
			{
				float angleOffset = i * 0.2f;  // 角度偏移
				float dirX = dx * cosf(angleOffset) - dy * sinf(angleOffset);
				float dirY = dx * sinf(angleOffset) + dy * cosf(angleOffset);
				auto bullet = std::make_shared<BounceBulletAttack>(x, y, dirX, dirY, 3.5f, 3);
				attacks.push_back(bullet);
			}
		}
		else  // 普通圆形攻击
		{
			float r = 100.0f;
			float dmg = 10.0f;
			auto atk = std::make_shared<CircleAttack>(x, y, r, dmg);
			attacks.push_back(atk);
		}

		attackPattern++;  // 切换攻击模式
	}

	virtual int getAttackDelay() override
	{
		return 180;  // 保持60FPS下约3秒的攻击间隔
	}
};

//玩家类
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

public:
	Player()
		:x(100), y(100), hp(100), maxHp(100), speed(4), isAttacking(false), attackTimer(0), damageCooldown(false), damageTimer(35) {
	}

	void update()
	{
		//移动（使用 raylib 的按键接口）
		if (IsKeyDown(KEY_W) && !isAttacking)
			y -= speed;
		if (IsKeyDown(KEY_S) && !isAttacking)
			y += speed;
		if (IsKeyDown(KEY_A) && !isAttacking)
			x -= speed;
		if (IsKeyDown(KEY_D) && !isAttacking)
			x += speed;

		if (IsKeyPressed(KEY_SPACE) && !isAttacking)
		{
			isAttacking = true;
			attackTimer = 20; // 攻击持续帧数
		}

		//边界检测（以中心点为准）
		x = std::max((float)PLAYER_SIZE, std::min((float)SCREEN_WIDTH - PLAYER_SIZE, x));
		y = std::max((float)PLAYER_SIZE, std::min((float)SCREEN_HEIGHT - PLAYER_SIZE, y));

		if (isAttacking)
		{
			attackTimer--;
			if (attackTimer <= 0)
			{
				isAttacking = false;
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

	void draw()
	{
		// 绘制玩家主体
		DrawCircle((int)x, (int)y, PLAYER_SIZE / 2, BLUE);

		// 攻击表现：短时间放大圈
		if (isAttacking)
		{
			DrawCircleLines((int)x, (int)y, PLAYER_SIZE / 2 + 12, GREEN);
		}

		// 血条
		float barW = 80;
		float hpR = (maxHp > 0) ? (hp / maxHp) : 0;
		DrawRectangle((int)(x - barW / 2), (int)(y + PLAYER_SIZE / 2 + 6), (int)barW, 6, DARKGRAY);
		DrawRectangle((int)(x - barW / 2), (int)(y + PLAYER_SIZE / 2 + 6), (int)(barW * hpR), 6, GREEN);
	}

	bool checkHit(float bossX, float bossY, float bossSize)
	{
		if (!isAttacking)
		{
			return false;
		}

		float dx = bossX - x;
		float dy = bossY - y;
		return sqrtf(dx * dx + dy * dy) < (PLAYER_SIZE / 2 + bossSize / 2 + 10);
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

	float getHp() const
	{
		return hp;
	}
	float getX() const
	{
		return x;
	}
	float getY() const
	{
		return y;
	}
};

int main()
{
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Demo - Boss & Player (raylib)");
	SetTargetFPS(60);

	Player player;
	Boss01 boss(SCREEN_WIDTH / 2.0f, 120.0f);

	// 把玩家放在屏幕中间偏下
	player = Player();
	// 使用局部时间控制（本示例仍使用帧计数器）

	while (!WindowShouldClose())
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

		// 渲染
		BeginDrawing();
		ClearBackground(RAYWHITE);

		// 绘制场景
		boss.draw();
		boss.drawAttacks();
		player.draw();

		// HUD
		DrawText(TextFormat("FPS: %d", GetFPS()), 10, 10, 12, DARKGRAY);
		DrawText(TextFormat("Player HP: %d", (int)player.getHp()), 10, 30, 12, DARKGRAY);
		DrawText(TextFormat("%s HP: %d", boss.getName().c_str(), (int)boss.getHp()), 10, 50, 12, DARKGRAY);

		// 若任一死亡，显示结束信息
		if (player.getHp() <= 0)
		{
			DrawText("You Died", SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 - 10, 20, RED);

		}
		else if (boss.getHp() <= 0)
		{
			DrawText("Boss Defeated!", SCREEN_WIDTH / 2 - 90, SCREEN_HEIGHT / 2 - 10, 20, GREEN);

		}

		EndDrawing();
	}

	CloseWindow();

	return 0;
}