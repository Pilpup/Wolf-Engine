#include "player.h"
#include "game.h"

Player::Player(Vector2 startPos, float startAngle, AssetMgr* assets) : 
    curPos(startPos), angle(startAngle), movSpeed(1.2f), rotSpeed(1400.0f), hitbox(0.1f),
    currState(PlayerState::IDLE), animTimer(0.0f), animSpd(0.15f){
    
    animations.push_back(assets->LoadSpriteSheet("assets/sprites/player/Idle-Anim.bmp", 18, 20, 0xF81F));
    animations.push_back(assets->LoadSpriteSheet("assets/sprites/player/Walk-Anim.bmp", 18, 20, 0xF81F));
    animations.push_back(assets->LoadSpriteSheet("assets/sprites/player/Eat-Anim.bmp", 18, 4, 0xF81F));
    animations.push_back(assets->LoadSpriteSheet("assets/sprites/player/Hurt-Anim.bmp", 22, 26, 0xF81F));
    animations.push_back(assets->LoadSpriteSheet("assets/sprites/player/Jump-Anim.bmp", 18, 22, 0xF81F));
    animations.push_back(assets->LoadSpriteSheet("assets/sprites/player/Fly-Anim.bmp", 20, 18, 0xF81F));
    animations.push_back(assets->LoadSpriteSheet("assets/sprites/player/Hover-Anim.bmp", 18, 20, 0xF81F));

    sprite.sheet = animations[static_cast<int>(currState)];
    sprite.currentFrame = 0;
    sprite.x = curPos.x;
    sprite.y = curPos.y;
    sprite.z = 0.0f;

    attacks[static_cast<int>(AttackID::TACKLE)] = {
        20, 1.0f, 0.0f, 0.1f, assets->LoadSpriteSheet("assets/sprites/player/Tackle.bmp", 18, 20, 0xF81F)
    };
    attacks[static_cast<int>(AttackID::HEADBUTT)] = {
        20, 1.0f, 0.0f, 7.0f, assets->LoadSpriteSheet("assets/sprites/player/Headbutt-Anim.bmp", 20, 20, 0xF81F)
    };
    attacks[static_cast<int>(AttackID::BUBBLE)] = {
        20, 1.0f, 0.0f, 7.0f, assets->LoadSpriteSheet("assets/sprites/player/Bubble.bmp", 18, 22, 0xF81F)
    };

    attackX = AttackID::TACKLE;
    attackY = AttackID::BUBBLE;
}

Player::~Player(){
}

void Player::Update(Entity* manager, Camera* cam, Engine& engine, Particle* particles, Map* map, float dt){
    Input& input = engine.GetInput();

    //Close Game
    if(input.isGamepadDown(Gamepad::Select) && input.isGamepadDown(Gamepad::Start)) engine.Stop();

    // Cooldown attacks;
    for(int i = 0; i < static_cast<int>(AttackID::COUNT); i++){
        if(attacks[i].currCooldown > 0.0f){
            attacks[i].currCooldown -= dt;
        }
    }

    Vector2 movDir(fcos(angle), fsin(angle));
    float movStep = movSpeed * dt;
    float rotStep = rotSpeed * dt;

    if(currState == PlayerState::ATTACKING){
        //Attack Movement logic
    }

    Vector2 newPos = curPos;
    Vector2 rightDir(-movDir.y, movDir.x);

    float currentMovStep = (currState == PlayerState::FLY || currState == PlayerState::HOVER) ? movStep * 2.0f : movStep; 

    // Movement
    if(currState != PlayerState::ATTACKING){
        if(input.isKeyDown(Keys::W) || input.isGamepadDown(Gamepad::DpadUp))    {newPos = newPos + movDir * currentMovStep;}
        if(input.isKeyDown(Keys::S) || input.isGamepadDown(Gamepad::DpadDown))  {newPos = newPos - movDir * currentMovStep;}
        if(input.isKeyDown(Keys::A) || input.isGamepadDown(Gamepad::DpadLeft))  {newPos = newPos - rightDir * currentMovStep;}
        if(input.isKeyDown(Keys::D) || input.isGamepadDown(Gamepad::DpadRight)) {newPos = newPos + rightDir * currentMovStep;}
    }

    // Camera Rotation
    if(input.isKeyDown(Keys::Q) || input.isGamepadDown(Gamepad::L1)){angle -= rotStep;}
    if(input.isKeyDown(Keys::E) || input.isGamepadDown(Gamepad::R1)){angle += rotStep;}

    // Collision Check
    if (!manager->CheckWallCollision(newPos.x, curPos.y, hitbox, map) &&
        !manager->CheckEntityCollision(newPos.x, curPos.y, hitbox, nullptr)){
        curPos.x = newPos.x;
    }

    if (!manager->CheckWallCollision(curPos.x, newPos.y, hitbox, map) &&
        !manager->CheckEntityCollision(curPos.x, newPos.y, hitbox, nullptr)){
        curPos.y = newPos.y;
    }
    
    // State Logic
    bool isMoving = (input.isKeyDown(Keys::W) || input.isKeyDown(Keys::A) || input.isKeyDown(Keys::S) || input.isKeyDown(Keys::D) ||
                     input.isGamepadDown(Gamepad::DpadUp) || input.isGamepadDown(Gamepad::DpadLeft) ||
                     input.isGamepadDown(Gamepad::DpadDown) || input.isGamepadDown(Gamepad::DpadRight)
    );

    PlayerState nextState = isMoving ? PlayerState::WALK : PlayerState::IDLE;

    // Jump Logic, Attack Logic
    if(currState == PlayerState::JUMP){
        UpdateJump(dt, isMoving, nextState);
    }

    else if(currState == PlayerState::ATTACKING){
        UpdateAttack(manager, map, dt, isMoving, nextState, particles);
        
        if(sprite.z == 0.0f && (input.isKeyPressed(Keys::SPACE) || input.isGamepadPressed(Gamepad::A))){
            nextState = PlayerState::FLY;
            flyTimer = 0.0f;
        }
    }
    
    else if(currState == PlayerState::FLY || currState == PlayerState::HOVER){
        if(input.isKeyPressed(Keys::SPACE) || input.isGamepadPressed(Gamepad::A)){
            flyTimer = 10.0f;
        }

        if((input.isKeyPressed(Keys::R) || input.isGamepadPressed(Gamepad::X)) &&
                attacks[static_cast<int>(attackX)].currCooldown <= 0.0f){
            nextState = PlayerState::ATTACKING;
            activeAtk = attackX;
            animSpd = 0.02f;
            attacks[static_cast<int>(attackX)].currCooldown = attacks[static_cast<int>(attackX)].maxCooldown;
        }
        
        else if((input.isKeyPressed(Keys::F) || input.isGamepadPressed(Gamepad::Y)) &&
                attacks[static_cast<int>(attackY)].currCooldown <= 0.0f){
            nextState = PlayerState::ATTACKING;
            animSpd = 0.05;
            activeAtk = attackY;
            attacks[static_cast<int>(attackY)].currCooldown = attacks[static_cast<int>(attackX)].maxCooldown;
        }

        else{
            UpdateFly(dt, isMoving, nextState);
        }
    }

    else{
        if(input.isKeyPressed(Keys::SPACE) || input.isGamepadPressed(Gamepad::A)){
            nextState = PlayerState::JUMP;
            jumpTimer = 0.0f;
        }

        else if((input.isKeyPressed(Keys::R) || input.isGamepadPressed(Gamepad::X)) &&
                attacks[static_cast<int>(attackX)].currCooldown <= 0.0f){
            nextState = PlayerState::ATTACKING;
            activeAtk = attackX;
            animSpd = 0.02;
            attacks[static_cast<int>(attackX)].currCooldown = attacks[static_cast<int>(attackX)].maxCooldown;
        }

        else if((input.isKeyPressed(Keys::F) || input.isGamepadPressed(Gamepad::Y)) && 
                attacks[static_cast<int>(attackY)].currCooldown <= 0.0f){
            nextState = PlayerState::ATTACKING;
            activeAtk = attackY;
            attacks[static_cast<int>(attackY)].currCooldown = attacks[static_cast<int>(attackY)].maxCooldown;
        }
    }

    // State Change
    if(currState != nextState){
        currState = nextState;   
        if(currState == PlayerState::ATTACKING){
            sprite.sheet = attacks[static_cast<int>(activeAtk)].animation;
            attackBaseZ = sprite.z;
        }
        else{
            sprite.sheet = animations[static_cast<int>(currState)];
            animSpd = 0.15f;
        }
        sprite.currentFrame = 0;
        animTimer = 0.0f;
    }
    
    int numDirections = 8;
    int framesPerAnim = sprite.sheet->numFrames / numDirections;
    if(framesPerAnim == 0) framesPerAnim = 1;

    bool up    = input.isKeyDown(Keys::W) || input.isGamepadDown(Gamepad::DpadUp);
    bool down  = input.isKeyDown(Keys::S) || input.isGamepadDown(Gamepad::DpadDown);
    bool left  = input.isKeyDown(Keys::A) || input.isGamepadDown(Gamepad::DpadLeft);
    bool right = input.isKeyDown(Keys::D) || input.isGamepadDown(Gamepad::DpadRight);

    bool isPressingKey = (up || down || left || right);
    if(isPressingKey){
        int targetDir = visualDir;

        if(up && left) targetDir = 5;
        else if(up && right) targetDir = 3;
        else if(down && left) targetDir = 7;
        else if(down && right) targetDir = 1;
        else if(up) targetDir = 4;
        else if(down) targetDir = 0;
        else if(left) targetDir = 6;
        else if(right) targetDir = 2;

        if(targetDir != currDir){
            currDir = targetDir;
            turnTimer = 0.06f;
        }
        if(turnTimer > 0.0f){
            turnTimer -= dt;
        }
        else{
            visualDir = currDir;
        }
    }
    else{
        currDir = visualDir;
        turnTimer = 0.0f;
    }

    int currAnimFram = sprite.currentFrame % framesPerAnim;
    sprite.currentFrame = (visualDir * framesPerAnim) + currAnimFram;

    Animate(dt);

    // 3rd Person
    sprite.x = curPos.x;
    sprite.y = curPos.y;
    
    Vector2 targetPos = curPos - (movDir * 1.0f) + (rightDir * 0.05f);
    Vector2 camToTarget = targetPos - curPos;

    Vector2 safeCamPos = curPos - (movDir * 0.6f);
    for(int i = 1; i <= 8; i++){
        Vector2 checkPos = curPos + (camToTarget * ((float)i / 8.0f));
        if(map->GetWorldTile(checkPos.x, checkPos.y)== 0) safeCamPos = checkPos;
        else break;
    }

    Vector2 diff = safeCamPos - cam->pos;
    float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
    if(dist > 0.0f){
        float moveDist = std::max(6.0f * dist, 1.0f) * dt;
        if(moveDist >= dist) cam->pos = safeCamPos;
        else cam->pos = cam->pos + (diff * (moveDist / dist));
    }
    cam->z = 1.2f;
    cam->pitch = -90;

    // Camera Update
    cam->absAngle = angle;
    cam->Update();
}

void Player::Animate(float dt){
    if(sprite.sheet == nullptr) return;

    int numDirections = 8;
    int framesPerAnim = sprite.sheet->numFrames / numDirections;
    if(framesPerAnim == 0) framesPerAnim = 1;

    animTimer += dt;
    if(animTimer >= animSpd){
        animTimer -= animSpd;
        int currDir = sprite.currentFrame / framesPerAnim;
        int currAnimFram = sprite.currentFrame % framesPerAnim;
        
        currAnimFram++;
        if(currAnimFram >= framesPerAnim){
            currAnimFram = 0;
        }
        sprite.currentFrame = (currDir * framesPerAnim) + currAnimFram;
    }
}

void Player::UpdateJump(float dt, bool isMoving, PlayerState& nextState){
    nextState = PlayerState::JUMP;

    if(sprite.sheet == nullptr){
        nextState = isMoving ? PlayerState::WALK : PlayerState::IDLE;
        return;
    }

    float jumpDuration = 0.6f;
    float maxJumpHeight = 90.0f;

    jumpTimer += dt;
    float progress = jumpTimer / jumpDuration;

    if(progress >= 1.0f){
        sprite.z = 0.0f;
        nextState = isMoving ? PlayerState::WALK : PlayerState::IDLE;
    }
    else{
        int lutIndex = (int)(progress * 2048.0f);
        sprite.z = fcos(lutIndex) * maxJumpHeight;
    }
}

void Player::UpdateAttack(Entity* manager, Map* map, float dt, bool isMoving, PlayerState& nextState, Particle* particles){
    nextState = PlayerState::ATTACKING;
    
    if(sprite.sheet == nullptr){
        nextState = isMoving ? PlayerState::WALK : PlayerState::IDLE;
        return;
    }

    int numDirections = 8;
    int framesPerAnim = sprite.sheet->numFrames / numDirections;
    if(framesPerAnim == 0) framesPerAnim = 1;

    int currAnimFram = sprite.currentFrame % framesPerAnim;

    bool isHitFrame = (currAnimFram == (framesPerAnim / 2)) && ((animTimer + dt) >= animSpd);

    Attack currStats = attacks[static_cast<int>(activeAtk)];
   
    int facingAngle = (int)angle + ((4 - visualDir) * 512);

    Vector2 facingDir(fcos(facingAngle), fsin(facingAngle));

    switch(activeAtk){
        case AttackID::TACKLE : {
            float dashSpd = 6.0f * dt;
            Vector2 newPos = curPos + facingDir * dashSpd;
            if (!manager->CheckWallCollision(newPos.x, curPos.y, hitbox, map) && 
                !manager->CheckEntityCollision(newPos.x, curPos.y, hitbox, nullptr)){
                curPos.x = newPos.x;
            }
            if (!manager->CheckWallCollision(curPos.x, newPos.y, hitbox, map) && 
                !manager->CheckEntityCollision(curPos.x, newPos.y, hitbox, nullptr)){ 
                curPos.y = newPos.y; 
            }
        } break;

        case AttackID::BUBBLE : {
            sprite.z = attackBaseZ;

            if(isHitFrame){
                for(int i = 0; i < 240; i++){
                    float baseVx = facingDir.x * 4.0f;
                    float baseVy = facingDir.y * 4.0f;

                    float spreadX = ((rand() % 100) - 50) / 50.0f;
                    float spreadY = ((rand() % 100) - 50) / 50.0f;

                    float vz = 100.0f + (rand() % 400);
                    float life = 0.5f + ((rand() % 100) / 100.0f);
                    
                    uint16_t bubbleColor = 0x05FF;

                    particles->Emit(curPos.x, curPos.y, sprite.z, baseVx + spreadX, baseVy + spreadY, vz, life, bubbleColor);
                }
            }
        } break;

        default : break;
    }

    if(isHitFrame){
        manager->PlayerMeleeAttack(curPos.x, curPos.y, facingDir.x, facingDir.y, currStats.range, currStats.damage);
    }

    if(currAnimFram == framesPerAnim - 1 && (animTimer + dt) >= animSpd){
        animSpd = 0.15f;
        
        if(sprite.z > 1.0f){
            nextState = isMoving ? PlayerState::FLY : PlayerState::HOVER;
        }
        else{
            sprite.z = 0.0f;
            nextState = isMoving ? PlayerState::WALK : PlayerState::IDLE;
        }
    }
}
void Player::UpdateFly(float dt, bool isMoving, PlayerState& nextState){
    nextState = isMoving ? PlayerState::FLY : PlayerState::HOVER;
    flyTimer += dt;
   
    if(flyTimer >= 10.0f){
        sprite.z -= 400.0f * dt;
        if (sprite.z <= 0.0f) {
            sprite.z = 0.0f; 
            nextState = isMoving ? PlayerState::WALK : PlayerState::IDLE;
        }
    }
    else{
        if (sprite.z < 180.0f) {
            sprite.z += 500.0f * dt; 
            if (sprite.z > 180.0f) sprite.z = 180.0f;
        }
    }
}
