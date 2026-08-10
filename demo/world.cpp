#include "world.h"

bool World::Initialize(AssetMgr* assets){
    Level level1;
    level1.map = assets->LoadMap("assets/maps/level1.map", 16);
    level1.ceilTex.push_back (assets->LoadTexture(1, "assets/sprites/ceil/mud_ceiling.bmp"));
    level1.floorTex.push_back(assets->LoadTexture(2, "assets/sprites/floor/mud_grass_floor.bmp"));
    levels.push_back(level1);
    
    //Level level2;  
    //level2.map = assets->LoadMap("assets/maps/level1.map", 16);
    //level2.floorTex.push_back();
    //level2.floorTex.push_back();
    //levels.push_back(level2);    

    // Walls
    assets->LoadTexture(11, "assets/sprites/walls/stone_wall.bmp");
    assets->LoadTexture(12, "assets/sprites/walls/brick_wall1.bmp");
    assets->LoadTexture(13, "assets/sprites/walls/brick_wall2.bmp");
    assets->LoadTexture(14, "assets/sprites/walls/brick_wall3.bmp");
    assets->LoadTexture(15, "assets/sprites/walls/brick_mud_wall.bmp");

    return true;
}
