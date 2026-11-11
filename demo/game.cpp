#include "game.h"
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

    entity->Update(player, &camera, world->GetMap(), dt);

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
        stressSprite = *player->GetSprite();
        stressSprite.x = camera.pos.x + camera.dir.x * 1.5f;
        stressSprite.y = camera.pos.y + camera.dir.y * 1.5f;
        stressSprite.z = 0.0f;
        rasterizer.QueueSprite(&stressSprite);
    }

    rasterizer.DrawSprites(&camera, raycaster, sunDir, true);

    particles.Render(&rasterizer, &camera, raycaster);

    rasterizer.DrawFPS(engine.GetFPS(), 10, 10, 0xF800);
}

void Game::Present(){
    engine.Present(rasterizer.GetPixelBuffer());
    engine.Wait();
}
