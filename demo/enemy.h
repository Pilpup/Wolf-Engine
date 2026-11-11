#ifndef ENEMY_H
#define ENEMY_H

#include "../engine/shlong.h"
#include <vector>

class Player;
class Entity;

enum class EnemyState{
    IDLE    = 0,
    WALK    = 1,
    ROTATE  = 2,
    SHOOT   = 3,
    CHARGE  = 4,
    HOP     = 5,
    DEAD    = 6
};

enum class EnemyType{
    ALOMORA,
    BULBASAUR,
    FURRET,
    BLASTOICE,
    CHARIZARD
};

class Enemy{
private:
    Sprite sprite;
    EnemyState currentState;
    std::vector<SpriteSheet*> animations;

    std::vector<Vector2> path;
    int currentPathIndex = 0;
    
    std::vector<int> cameFrom;
    std::vector<int> frontier;

    float animSpd    = 0.15f;
    float animTimer  = 0.0f;
    int animFrame    = 0;
    float actionTime = 0.0f;
    float faceAngle  = 0.0f;
    int curDir       = 0;

    float moveSpd = 1.0f;
    float hitbox = 0.3f;
    float stopRangeSqr = 4.0f;
    float aggroRangeSqr = 36.0f;
    float hysteresis = 1.5f;

    float lastKnowX = 0.0f;
    float lastKnowY = 0.0f;
    bool hasLastKnown = false;
    float rotations = 0.0f;
    int roamCount = 0;

    int maxHealth;
    int currHealth;
    
public:
    Enemy(AssetMgr* assets, EnemyType type, float startX, float startY);
    ~Enemy() = default;

    void Initialize(std::vector<SpriteSheet*> sheets, float startX, float startY);
    void Update(Entity* manager, Player* player, Camera* cam, Map* map, float dt);

    Sprite* GetSprite(){return &sprite;}
    EnemyState GetCurrentState() {return currentState;}
    float GetHitbox() {return hitbox;}

    void TakeDamage(int dmgAmnt);

private:
    void Animate(float dt);
    void ChangeState(EnemyState newState);

    bool UpdateShit(Player* player, Map* map, float& distanceSqr);
    void UpdatePath(Map* map);
    void UpdateVisuals(Camera* cam, float dt);

    void UpdateIdle(bool canSeePlayer);
    void UpdateWalk(Entity* manager, Player* player, float distanceSqr, bool canSeePlayer, float dt, Map* map);
    void UpdateShoot(float distanceSqr, bool canSeePlayer);
    void UpdateRotate(float dt, bool canSeePlayer, Map* map);

    bool CheckFOV(float radToPlayer);
    bool CalcLineOfSight(float targetX, float targetY, Map* map);
    void CalculateFacingAngle(float radToPlayer);

    bool FindPath(int startX, int startY, int targetX, int targetY, Map* map);
};

#endif
