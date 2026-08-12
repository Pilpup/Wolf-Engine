#include "enemy.h"
#include "entity_mgr.h"
#include "player.h"
#include <cmath>
#include <cstdlib>

Enemy::Enemy(AssetMgr* assets, EnemyType type, float startX, float startY){
    sprite.x = startX;
    sprite.y = startY;
    sprite.z = 0.0f;
    sprite.currentFrame = 0;
    currentState = EnemyState::IDLE;
    faceAngle = 0.0f;

    if(type == EnemyType::ALOMORA){
        animSpd = 0.2f;
        moveSpd = 1.2f;
        maxHealth = 100;
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/auromora/Idle-Anim.bmp", 44, 48, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/auromora/Walk-Anim.bmp", 44, 78, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/auromora/Rotate-Anim.bmp", 44, 48, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/auromora/Shoot-Anim.bmp", 158, 78, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/auromora/Charge-Anim.bmp", 38, 46, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/auromora/Hop-Anim.bmp", 44, 48, 0xF81F));
        animations.push_back(animations[0]);
    }    
    else if(type == EnemyType::BULBASAUR){
        animSpd = 0.1f;
        maxHealth = 50;
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/bulbasaur/Idle-Anim.bmp", 24, 22, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/bulbasaur/Walk-Anim.bmp", 24, 22, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/bulbasaur/Rotate-Anim.bmp", 214, 22, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/bulbasaur/Shoot-Anim.bmp", 20, 22, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/bulbasaur/Charge-Anim.bmp", 24, 40, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/bulbasaur/Hop-Anim.bmp", 24, 22, 0xF81F));
        animations.push_back(animations[0]);
    }
    else if(type == EnemyType::BLASTOICE){
        animSpd = 0.3f;
        maxHealth = 150;
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/blastoice/Idle-Anim.bmp", 30, 30, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/blastoice/Walk-Anim.bmp", 28, 30, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/blastoice/Rotate-Anim.bmp", 28, 30, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/blastoice/Shoot-Anim.bmp", 32, 34, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/blastoice/Charge-Anim.bmp", 30, 28, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/blastoice/Hop-Anim.bmp", 320, 50, 0xF81F));
        animations.push_back(animations[0]);
    }
    else if(type == EnemyType::FURRET){
        animSpd = 0.2f;
        maxHealth = 75;
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/furret/Idle-Anim.bmp", 26, 26, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/furret/Walk-Anim.bmp", 40, 36, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/furret/Rotate-Anim.bmp", 26, 26, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/furret/Shoot-Anim.bmp", 36, 34, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/furret/Charge-Anim.bmp", 396, 24, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/furret/Hop-Anim.bmp", 34, 34, 0xF81F));
        animations.push_back(animations[0]);
    }    
    else if(type == EnemyType::CHARIZARD){
        animSpd = 0.3f;
        maxHealth = 100;
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/charizard/Idle-Anim.bmp", 34, 30, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/charizard/Walk-Anim.bmp", 34, 30, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/charizard/Rotate-Anim.bmp", 30, 30, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/charizard/Shoot-Anim.bmp", 40, 32, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/charizard/Charge-Anim.bmp", 400, 32, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/charizard/Hop-Anim.bmp", 390, 32, 0xF81F));
        animations.push_back(assets->LoadSpriteSheet("assets/sprites/entity/charizard/Faint-Anim.bmp", 42, 28, 0xF81F));
    }

    currHealth = maxHealth;

    if(!animations.empty()){
        sprite.sheet = animations[static_cast<int>(currentState)];
    }
}

void Enemy::Initialize(std::vector<SpriteSheet*> sheets, float startX, float startY){
    animations = sheets;
    sprite.x = startX;
    sprite.y = startY;

    if(!animations.empty()){
        sprite.sheet = animations[static_cast<int>(currentState)];
    }
}

void Enemy::Update(Entity* manager, Player* player, Camera* cam, Map* map, float dt){
    if(currentState == EnemyState::DEAD){
        Animate(dt);
        return;
    }
    
    float distanceSqr = 0.0f;
    bool canSeePlayer = UpdateShit(player, cam, map, distanceSqr);

    if(!canSeePlayer && currentState == EnemyState::WALK && hasLastKnown){
        UpdatePath(map);
    }

    switch(currentState){
        case EnemyState::IDLE:
            UpdateIdle(distanceSqr);
            break;
        case EnemyState::WALK:
            UpdateWalk(manager, player, distanceSqr, canSeePlayer, dt, map);
            break;
        case EnemyState::SHOOT:
            UpdateShoot(distanceSqr, canSeePlayer);
            break;
        case EnemyState::ROTATE:
            UpdateRotate(dt, canSeePlayer, map);
            break;
        default:
            break;
    }
    
    UpdateVisuals(cam, dt);
}

bool Enemy::UpdateShit(Player* player, Camera* cam, Map* map, float& distanceSqr){
    float diffX = player->GetSprite()->x - sprite.x;
    float diffY = player->GetSprite()->y - sprite.y;
    distanceSqr = diffX * diffX + diffY * diffY;

    float radToPlayer = (float)std::atan2((double)diffY, (double)diffX);
    if(radToPlayer < 0.0f) radToPlayer += 2.0f * PI;

    bool canSeePlayer = false;
    if(distanceSqr <= aggroRangeSqr){
        if(CheckFOV(radToPlayer) && CalcLineOfSight(cam->pos.x, cam->pos.y, map)){
            canSeePlayer = true;
        }
    }
    if(canSeePlayer){
        lastKnowX = cam->pos.x;
        lastKnowY = cam->pos.y;
        hasLastKnown = true;
        roamCount = 0;
        path.clear();
        faceAngle = (radToPlayer / (2.0f * PI)) * 4096.0f;
    }
    return canSeePlayer;
}

void Enemy::UpdatePath(Map* map){
    if(path.empty()){
        bool found = FindPath((int)sprite.x, (int)sprite.y, (int)lastKnowX, (int)lastKnowY, map);
        if(!found){
            lastKnowX = sprite.x;
            lastKnowY = sprite.y;
        }
    }
    
    if(!path.empty() && currentPathIndex < (int)path.size()){
        float radToTarget = (float)std::atan2(
            (double)(path[currentPathIndex].y - sprite.y), 
            (double)(path[currentPathIndex].x - sprite.x)
        );
        
        if(radToTarget < 0.0f) radToTarget += 2.0f * PI;
        faceAngle = (radToTarget / (2.0f * PI)) * 4096.0f;
    }
}

void Enemy::UpdateVisuals(Camera* cam, float dt){
    float camDiffX = cam->pos.x - sprite.x;
    float camDiffY = cam->pos.y - sprite.y;
    float radToCamera = (float)std::atan2((double)camDiffY, (double)camDiffX);
    
    if(radToCamera < 0.0f) radToCamera += 2.0f * PI;

    CalculateFacingAngle(radToCamera);
    Animate(dt);
}

void Enemy::UpdateIdle(bool canSeePlayer){
    if(canSeePlayer){
        ChangeState(EnemyState::WALK);
    }
}

void Enemy::UpdateWalk(Entity* manager, Player* player, float distanceSqr, bool canSeePlayer, float dt, Map* map){
    float targetX, targetY;
    if(canSeePlayer){
        if(distanceSqr < (stopRangeSqr - hysteresis)){
            ChangeState(EnemyState::SHOOT);
            return;
        }
        targetX = lastKnowX;
        targetY = lastKnowY;
    }
    else if(hasLastKnown){
        if(!path.empty() && currentPathIndex < (int)path.size()){
            targetX = path[currentPathIndex].x;
            targetY = path[currentPathIndex].y;
            
            float diffX = targetX - sprite.x;
            float diffY = targetY - sprite.y;
            
            if(diffX * diffX + diffY * diffY < 0.02f){ 
                currentPathIndex++;
                if(currentPathIndex >= (int)path.size()){
                    path.clear();
                    ChangeState(EnemyState::ROTATE);
                    return;
                }
                targetX = path[currentPathIndex].x;
                targetY = path[currentPathIndex].y;
            }
        }
        else{
            ChangeState(EnemyState::ROTATE);
            return;
        }
    }
    else{
        ChangeState(EnemyState::IDLE);
        return;
    }
   
    float diffX = targetX - sprite.x;
    float diffY = targetY - sprite.y;
    float realDistance = std::sqrt(diffX * diffX + diffY * diffY);

    if(realDistance > 0.001f){
        float moveX = (diffX / realDistance) * moveSpd * dt;
        float moveY = (diffY / realDistance) * moveSpd * dt;

        float newX = sprite.x + moveX;
        float newY = sprite.y + moveY;

        if (!manager->CheckWallCollision(newX, sprite.y, hitbox, map) &&
            !manager->CheckEntityCollision(newX, sprite.y, hitbox, this) &&
            !manager->CheckPlayerCollision(newX, sprite.y, hitbox, player)){
            sprite.x = newX; 
        }

        if (!manager->CheckWallCollision(sprite.x, newY, hitbox, map) &&
            !manager->CheckEntityCollision(sprite.x, newY, hitbox, this) && 
            !manager->CheckPlayerCollision(sprite.x, newY, hitbox, player)){
            sprite.y = newY;
        }
    }    
}

void Enemy::UpdateShoot(float distanceSqr, bool canSeePlayer){
    if(!canSeePlayer || distanceSqr > (stopRangeSqr + hysteresis)){
        ChangeState(EnemyState::WALK);
    }
}

void Enemy::UpdateRotate(float dt, bool canSeePlayer, Map* map){
    if(canSeePlayer){
        ChangeState(EnemyState::WALK);
        return;
    }

    float turnSpd = 3000.0f * dt;
    faceAngle += turnSpd;
    rotations += turnSpd;

    if(faceAngle >= 4096.0f) faceAngle -= 4096.0f;

    if(rotations >= 4096.0f){
        if(roamCount < 3){
            bool foundTile = false;
            for(int i = 0; i < 10; i++){
                int rx = (int)sprite.x + (std::rand() % 13 - 6);
                int ry = (int)sprite.y + (std::rand() % 13 - 6);
                if(rx >= 1 && rx < map->GetWidth() - 1 && ry >= 1 && ry < map->GetHeight() - 1){
                    if(map->GetTile(rx, ry) == 0){
                        lastKnowX = rx + 0.5f;
                        lastKnowY = ry + 0.5f;
                        foundTile = true;
                        break;
                    }
                }
            }
            if(foundTile){
                hasLastKnown = true;
                roamCount++;
                ChangeState(EnemyState::WALK);
                return;
            }
        }
        
        roamCount = 0;
        hasLastKnown = false;
        ChangeState(EnemyState::IDLE);
    }
}

void Enemy::CalculateFacingAngle(float radToPlayer){
    int targetAngle = (int)((radToPlayer / (2.0f * PI)) * 4096.0f) % 4096;
    int relativeAngle = (targetAngle - (int)faceAngle) % 4096;
    if(relativeAngle < 0) relativeAngle += 4096;

    int shiftedAngle = (relativeAngle + 256) % 4096;
    curDir = shiftedAngle >> 9;
}

void Enemy::Animate(float dt){
    int state = static_cast<int>(currentState);
    if((size_t)state >= animations.size() || animations[state] == nullptr) return;
    
    int numDirections = 8;
    int framesPerAnim = animations[state]->numFrames / numDirections;
    if (framesPerAnim <= 0) return;
    
    animTimer += dt;
    while(animTimer >= animSpd){
        animTimer -= animSpd;
        animFrame++;

        if(animFrame >= framesPerAnim){
            if(currentState == EnemyState::DEAD){
                animFrame = framesPerAnim - 1;
                animTimer = 0.0f;
                break;
            }
            else{
                animFrame = 0;
            }
        }
    }

    sprite.currentFrame = animFrame + (curDir * framesPerAnim);
}

void Enemy::ChangeState(EnemyState newState){
    if(currentState == newState) return;
    
    currentState = newState;
    animFrame  = 0;
    animTimer = 0.0f;

    if(newState == EnemyState::ROTATE){
        rotations = 0.0f;
    }

    int state = static_cast<int>(currentState);
    if((size_t)state < animations.size() && animations[state] != nullptr){
        sprite.sheet = animations[state];
        sprite.currentFrame = 0;
    }
}

bool Enemy::CheckFOV(float radToPlayer){
    int targetAngle = (int)((radToPlayer / (2.0f * PI)) * 4096.0f) % 4096;
    int relativeAngle = (targetAngle - (int)faceAngle) % 4096;
    if(relativeAngle < 0) relativeAngle += 4096;

    return (relativeAngle <= 512 || relativeAngle >= 3584);
}

bool Enemy::CalcLineOfSight(float targetX, float targetY, Map* map){
    float rayDirX = targetX - sprite.x;
    float rayDirY = targetY - sprite.y;

    float distance = std::sqrt(rayDirX * rayDirX + rayDirY * rayDirY);    
    if(distance == 0.0f) return true;

    rayDirX /= distance;
    rayDirY /= distance;

    int mapX = (int)sprite.x;
    int mapY = (int)sprite.y;

    float deltaDistX = (rayDirX == 0) ? 1e30f : std::abs(1.0f / rayDirX);
    float deltaDistY = (rayDirY == 0) ? 1e30f : std::abs(1.0f / rayDirY);

    float sideDistX, sideDistY;
    int stepX, stepY;

    if(rayDirX < 0){
        stepX = -1;
        sideDistX = (sprite.x - mapX) * deltaDistX;
    }
    else{
        stepX = 1;
        sideDistX = (mapX + 1.0f - sprite.x) * deltaDistX;
    }

    if(rayDirY < 0){
        stepY = -1;
        sideDistY = (sprite.y - mapY) * deltaDistY;
    }
    else{
        stepY = 1;
        sideDistY = (mapY + 1.0f - sprite.y) * deltaDistY;
    }

    float currentDist = 0.0f;
    while(currentDist < distance){
        if(sideDistX < sideDistY){
            sideDistX += deltaDistX;
            mapX += stepX;
            currentDist = sideDistX - deltaDistX;
        }
        else {
            sideDistY += deltaDistY;
            mapY += stepY;
            currentDist = sideDistY - deltaDistY;
        }

        if(currentDist >= distance) break;
        if(map->GetTile(mapX, mapY) != 0) return false;
    }
    
    return true;
}

bool Enemy::FindPath(int startX, int startY, int targetX, int targetY, Map* map){
    path.clear();
    currentPathIndex = 0;
    
    if(startX == targetX && startY == targetY) return true;
    
    int width = map->GetWidth();
    int height = map->GetHeight();
    
    if(cameFrom.size() != (size_t)(width * height)){
        cameFrom.resize(width * height);
    }
    std::fill(cameFrom.begin(), cameFrom.end(), -1);
    
    frontier.clear();
    int head = 0;
    
    int startIdx = startY * width + startX;
    int targetIdx = targetY * width + targetX;
    
    frontier.push_back(startIdx);
    cameFrom[startIdx] = startIdx;
    
    int dx[] = {1, -1, 0, 0};
    int dy[] = {0, 0, 1, -1};
    
    bool found = false;
    while(head < (int)frontier.size()){
        int current = frontier[head++];
        
        if(current == targetIdx){
            found = true;
            break;
        }
        
        int cx = current % width;
        int cy = current / width;
        
        for(int i = 0; i < 4; i++){
            int nx = cx + dx[i];
            int ny = cy + dy[i];
            
            if(nx >= 0 && nx < width && ny >= 0 && ny < height){
                if(map->GetTile(nx, ny) == 0){
                    int nIdx = ny * width + nx;
                    if(cameFrom[nIdx] == -1){
                        frontier.push_back(nIdx);
                        cameFrom[nIdx] = current;
                    }
                }
            }
        }
    }
    
    if(!found) return false;
    
    int curr = targetIdx;
    std::vector<Vector2> tempPath;
    while(curr != startIdx){
        int cx = curr % width;
        int cy = curr / width;
        tempPath.push_back(Vector2(cx + 0.5f, cy + 0.5f));
        curr = cameFrom[curr];
    }
    
    for(int i = (int)tempPath.size() - 1; i >= 0; i--){
        path.push_back(tempPath[i]);
    }
    
    return true;
}

void Enemy::TakeDamage(int dmgAmnt){
    if(currentState == EnemyState::DEAD) return;

    currHealth -= dmgAmnt;

    if(currHealth <= 0){
        currHealth = 0;
        ChangeState(EnemyState::DEAD);
    }
}
