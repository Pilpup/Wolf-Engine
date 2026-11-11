#include "game.h"
#include <SDL2/SDL.h>
#include "enemy.h"
#include "entity_mgr.h"
#include "player.h"
#include "world.h"

Game::Game() : raycaster(nullptr), entity(nullptr), world(nullptr), player(nullptr), level(0) {}

Game::~Game(){
    if(raycaster) delete raycaster; 
    if(player)    delete player;
    if(world)     delete world;
    if(entity)    delete entity;
}

bool Game::Initialize(int width, int height, const char* title){
    scrHeight = height;
    scrWidth  = width;

    if(!engine.Initialize(scrWidth, scrHeight, title)) return false;
    engine.TargetFPS(benchSeconds > 0.0f ? 0 : 100);

    rasterizer.Initialize(width, height);
    camera.SetViewDistance(20.0f, true);
    
    raycaster = new Raycaster(width, height);

    player = new Player(Vector2(2.5f, 2.5f), 0.0f, &assets);
    
    world = new World();
    world->Initialize(&assets);

    world->LoadLevel(level);

    entity = new Entity();
    Enemy* blastoice = new Enemy(&assets, EnemyType::BLASTOICE, 3.5f, 3.5f);
    entity->AddEnemy(blastoice);

    Enemy* charizard = new Enemy(&assets, EnemyType::CHARIZARD, 1.5f, 1.5f);
    entity->AddEnemy(charizard);

    return true;
}

void Game::Run(){
    while(engine.IsRunning()){
        Update();
        Render();
        Present();

        if(benchSeconds > 0.0f && engine.GetRunTime() >= benchSeconds){
            engine.Stop();
        }
    }

    engine.WriteStats("stats.txt");
}

void Game::Update(){
    engine.Update();
    float dt = engine.GetDeltaTime();

    // Directional Lighting
    sunAngle += dt * 10.0f;
    if(sunAngle >= 4096.0f) sunAngle -= 4096.0f;
    
    player->Update(entity, &camera, engine, &particles, world->GetMap(), dt);

    if(!stressMode) entity->Update(player, &camera, world->GetMap(), dt);
    else { camera.absAngle = 2048; camera.Update(); }

    particles.Update(dt);
}

void Game::Render(){
    Vector2 sunDir(fcos((int)sunAngle), fsin((int)sunAngle));

    rasterizer.DrawWalls(raycaster, &camera, world->GetMap(), sunDir, &assets, &palette);
    raycaster->CalculateRowGeometry(&camera);

    rasterizer.DrawTexturedHorizon(
        raycaster->GetColBuffer(),
        raycaster->GetRowBuffer(),
        world->Getfloor()[0],
        world->GetCeil()[0],
        &camera,
        false
    );

    entity->QueueSprites(&rasterizer);
    rasterizer.QueueSprite(player->GetSprite());

    if(stressMode){
        Enemy* e = entity->GetEnemy(0);
        if(e == nullptr) return;
        stressSprite = *e->GetSprite();
        stressSprite.currentFrame = e->GetSprite()->currentFrame;
        stressSprite.x = camera.pos.x + camera.dir.x * 1.2f;
        stressSprite.y = camera.pos.y + camera.dir.y * 1.2f;
        stressSprite.z = 0.0f;
        rasterizer.QueueSprite(&stressSprite);
    }

    rasterizer.DrawSprites(&camera, raycaster, sunDir, true);

    particles.Render(&rasterizer, &camera, raycaster);

    rasterizer.DrawFPS(engine.GetFPS(), 10, 10, 0xF800);
}

void Game::Present(){
    const uint16_t* pixels = rasterizer.GetPixelBuffer();

    if(shotFrame > 0 && engine.GetTotalFrames() == (uint64_t)shotFrame){
        SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormatFrom((void*)pixels, scrWidth, scrHeight, 16, scrWidth * 2, SDL_PIXELFORMAT_RGB565);
        if(surf){
            SDL_SaveBMP(surf, "shot.bmp");
            SDL_FreeSurface(surf);
        }
    }

    engine.Present(pixels);
    engine.Wait();
}
