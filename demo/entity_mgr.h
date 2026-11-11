#ifndef ENTITY_MGR_H
#define ENTITY_MGR_H

#include "../engine/shlong.h"
#include <vector>

class Enemy;
class Player;

class Entity{
private:
    std::vector<Enemy*> enemies;

public:
    Entity() {}
    ~Entity();

    void AddEnemy(Enemy* enemy);
    void Update(Player* player, Camera* cam, Map* map, float dt);
    void QueueSprites(Rasterizer* rasterer);
    void Clear();
    Enemy* GetEnemy(int index){return (index >= 0 && index < (int)enemies.size()) ? enemies[index] : nullptr;}

    bool CheckWallCollision(float targetX, float targetY, float hitbox, Map* map);
    bool CheckEntityCollision(float targetX, float targetY, float hitbox, Enemy* self);
    bool CheckPlayerCollision(float targetX, float targetY, float hitbox, Player* player);

    void PlayerMeleeAttack(float px, float py, float dirX, float dirY, float range, float damage);
};

#endif
