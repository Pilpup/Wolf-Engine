#ifndef GAME_H
#define GAME_H

#include "../engine/shlong.h"
#include "player.h"
#include "world.h"
#include "entity_mgr.h"

class Game{
private:
    Engine engine;
    Rasterizer rasterizer;
    Raycaster* raycaster;
    AssetMgr assets;
    Palette palette;
    Camera camera;
  
    Entity* entity;
    World*  world;
    Player* player;
    Particle particles;

    int scrWidth, scrHeight, level;

    float sunAngle = 0.0f;
    float benchSeconds = 0.0f;

public:
    Game();
    ~Game();

    bool Initialize(int width, int height, const char* title);
    void Run();

    void SetBenchDuration(float seconds) {benchSeconds = seconds;}

private:
    void LoadGlobalAssets();
    void Update();
    void Render();
    void Present();
};

#endif
