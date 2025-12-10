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
	CircleAttack(float cx, float cy, float r, float dmg = 25.0f)
		:Attack(cx, cy, r, dmg), warningTime(75), activeTime(30) {
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

//可反弹攻击类型
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
		: Attack(cx, cy, 10.0f, dmg),  // 子弹半径为8
		directionX(dirX), directionY(dirY),
		speed(spd), bounceCount(0), maxBounces(maxBounce),
		rotation(0.0f), rotationSpeed(0.1f), warningTime(30)  // 预警时间设为60帧（1秒）
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
		if (phase == WARNING)
		{
			// 预警阶段在BOSS位置显示"!"
			DrawText("!", (int)x - 6, (int)y - 6, 20, YELLOW);

			// 绘制延伸至屏幕边缘的方向指示器（全屏幕长度）
			// 计算射线与屏幕边缘的交点
			float t;
			float edgeX, edgeY;

			// 计算射线与屏幕边界的交点（使用射线与直线相交算法）
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
				t = FLT_MAX; // 垂直方向射线，先不考虑X方向
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
				tY = FLT_MAX; // 水平方向射线，先不考虑Y方向
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
	}

	// 其余方法保持不变...
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
		: Attack(cx, cy, r, dmg), warningTime(20), activeTime(20) {
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
	int attackPattern;
	int aimedAttackCounter;  // 瞄准攻击计数器
	bool isDoingAimedAttack; // 是否正在进行连续瞄准攻击
	int chainAttackInterval; // 连续攻击的间隔（用于精确控制）

public:
	Boss01(float cx, float cy)
		: Boss(cx, cy, 500.0f, "Boss01"), attackPattern(0),
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

		// 处理攻击更新（从基类复制）
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
			chainAttackInterval = 15;
		}
	}

	virtual void doAttack(float playerX, float playerY) override
	{
		float distance = getDistanceToPlayer(playerX, playerY);

		// 远距离：使用瞄准攻击
		if (distance > 150.0f)
		{
			if (attackPattern % 4 == 0)  // 每4次攻击使用1次反弹子弹
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
					auto bullet = std::make_shared<BounceBulletAttack>(x, y, dirX, dirY, 3.5f, 3);
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
					auto bullet = std::make_shared<BounceBulletAttack>(x, y, dirX, dirY, 3.5f, 3);
					attacks.push_back(bullet);
				}
			}
			else
			{
				float r = 200.0f;
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

//玩家类
// 修改Player类的私有成员，添加攻击方向和位移相关变量
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

public:
	Player()
		:x(100), y(100), hp(100), maxHp(100), speed(4), isAttacking(false),
		attackTimer(0), damageCooldown(false), damageTimer(35),
		attackDirX(0), attackDirY(0), attackDisplaced(false) {
	}

	void update()
	{
		// 记录攻击前的移动方向作为攻击方向
		float moveDirX = 0, moveDirY = 0;
		if (IsKeyDown(KEY_W)) moveDirY -= 1;
		if (IsKeyDown(KEY_S)) moveDirY += 1;
		if (IsKeyDown(KEY_A)) moveDirX -= 1;
		if (IsKeyDown(KEY_D)) moveDirX += 1;

		// 如果没有移动输入，默认向上为攻击方向
		if (moveDirX == 0 && moveDirY == 0) {
			moveDirY = -1;  // 默认向上
		}
		else {
			// 标准化方向向量
			float len = sqrtf(moveDirX * moveDirX + moveDirY * moveDirY);
			moveDirX /= len;
			moveDirY /= len;
		}

		// 攻击逻辑
		if (IsKeyPressed(KEY_SPACE) && !isAttacking)
		{
			isAttacking = true;
			attackTimer = 15;  // 攻击持续时间缩短为15帧
			attackDirX = moveDirX;
			attackDirY = moveDirY;
			attackDisplaced = false;  // 重置位移标记
		}

		// 攻击过程处理
		if (isAttacking)
		{
			// 攻击开始时执行一次位移
			if (!attackDisplaced) {
				x += attackDirX * 15;  // 向前位移15像素
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
		else
		{
			if (IsKeyDown(KEY_W)) y -= speed;
			if (IsKeyDown(KEY_S)) y += speed;
			if (IsKeyDown(KEY_A)) x -= speed;
			if (IsKeyDown(KEY_D)) x += speed;
		}

		// 边界检测
		x = std::max((float)PLAYER_SIZE, std::min((float)SCREEN_WIDTH - PLAYER_SIZE, x));
		y = std::max((float)PLAYER_SIZE, std::min((float)SCREEN_HEIGHT - PLAYER_SIZE, y));

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
		Color playerColor = damageCooldown ? RED : BLUE;
		DrawCircle((int)x, (int)y, PLAYER_SIZE / 2, playerColor);

		// 绘制半圆形攻击范围
		if (isAttacking)
		{
			const int segments = 20;  // 半圆的线段数量
			const float radius = PLAYER_SIZE / 2 + 20;  // 攻击范围半径
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

			// 绘制连接玩家到半圆两端的线段（形成扇形）
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