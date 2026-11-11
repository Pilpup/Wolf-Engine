#include "Raycaster.h"
#include "Camera.h"
#include "Mathutil.h"
#include "Sprite.h"
#include "Map.h"
#include <cstdint>

Raycaster::Raycaster(int w, int h) : scrWidth(w), scrHeight(h) {
    colBuffer = new ColumnGeometry[scrWidth];
    rowBuffer = new RowGeometry[scrHeight];
}

Raycaster::~Raycaster(){
    delete[] colBuffer;
    delete[] rowBuffer;
}

void Raycaster::CalculateColumnGeometry(Camera* camera, Map* map, Vector2 sunDir){
    
    const unsigned char* mapData = map->GetRawData();
    
    int mapShift = map->GetMapShift();

    float invWidth = 2.0f / (float)scrWidth;

    #pragma omp parallel for schedule(static)
    for(int x = 0; x < scrWidth; x++){
        float cameraX = invWidth * x  - 1.0f;                       // normalize camera plane

        Vector2 rayDir = camera->dir + (camera->plane * cameraX);   // parametric equation of line P(t) = C + (V * t)

        int mapX = (int)camera->pos.x;                              // floating position to 2d map position
        int mapY = (int)camera->pos.y;

        Vector2 deltaDist = {                                       // distance ray must travel in it's direction            
            (rayDir.x == 0.0f) ? 1e30f : std::abs(1.0f / rayDir.x), // to shift to next closest gridline
            (rayDir.y == 0.0f) ? 1e30f : std::abs(1.0f / rayDir.y)
        };

        Vector2 sideDist;                                           // distance ray must travel in it's direction
                                                                    // to shift to next closest gridline on the map 
        int stepX, stepY;

        // calculating if the indexing should be left or right for the sideDist
        if(rayDir.x < 0){
            stepX = -1;
            sideDist.x = (camera->pos.x - mapX) * deltaDist.x;
        }
        else{
            stepX = 1;
            sideDist.x = (mapX + 1.0f - camera->pos.x) * deltaDist.x;
        }

        if(rayDir.y < 0){
            stepY = -1;
            sideDist.y = (camera->pos.y - mapY) * deltaDist.y;
        }
        else{
            stepY = 1;
            sideDist.y = (mapY + 1.0f - camera->pos.y) * deltaDist.y;
        }

        int mapIndex = (mapY << mapShift) + mapX;
        int stepYIndex = stepY << mapShift;

        bool side = 0;           // records which axis the ray collided with last. 0 for x, 1 for y
        unsigned char tile = 0;
        while(1){
            if(sideDist.x < sideDist.y){
                sideDist.x += deltaDist.x;
                mapIndex +=stepX;
                side = 0;
            }
            else{
                sideDist.y += deltaDist.y;
                mapIndex += stepYIndex;
                side = 1;
            }

            tile = mapData[mapIndex];
            if(tile > 0) break;

            if(sideDist.x > camera->maxViewDist && sideDist.y > camera->maxViewDist) break;
        }

        // perpendicular distance to the wall, 
        // since I did not use pythagorus theorem to find the distance, there is minimal fish eye effect
        float wallDistance;
        if(tile == 0){
            wallDistance = camera->maxViewDist + 10.0f;
        }
        else{
            wallDistance = (side == 0) ? (sideDist.x - deltaDist.x) : (sideDist.y - deltaDist.y);
        }

        if(wallDistance <= 0.0f) wallDistance = 0.3f;

        float invWallDistance = 1.0f / wallDistance;
        int vertHeight = (int)(scrHeight * invWallDistance);

        int camOffset = camera->pitch + (int)(camera->z * invWallDistance);

        int drawStart = (scrHeight >> 1) - (vertHeight >> 1) + camOffset;
        if(drawStart < 0) drawStart = 0;

        int drawEnd = (scrHeight >> 1) + (vertHeight >> 1) + camOffset;
        if(drawEnd >= scrHeight) drawEnd = scrHeight - 1;

        //calculate where the ray hit the wall exactly
        float wallX = (side == 0) ?     //0 - vertical wall, 1- horizontal wall
                      (camera->pos.y + rayDir.y * wallDistance) :
                      (camera->pos.x + rayDir.x * wallDistance)
        ;

        wallX -= (int)wallX;    // % of the way across the wall

        // flipping the texture if we looking at the back of the wall
        if(side == 0 && rayDir.x < 0) wallX = 1.0f - wallX;
        if(side == 1 && rayDir.y > 0) wallX =  1.0f - wallX;

        //Directional Lighting
        Vector2 wallNormal;

        if(side == 0){
            wallNormal.x = (rayDir.x > 0.0f) ? -1.0f : 1.0f;
            wallNormal.y = 0.0f;
        }
        else{
            wallNormal.x = 0.0f;
            wallNormal.y = (rayDir.y > 0.0f) ? -1.0f : 1.0f;
        }

        float dotPorduct = wallNormal.Dot(sunDir);
        float lightIntensity = std::max(0.0f, dotPorduct);
        
        int calculatedLight = 120 + (int)(lightIntensity * 136.0f);
        
        colBuffer[x].distance  = wallDistance; 
        colBuffer[x].drawStart = drawStart; 
        colBuffer[x].drawEnd   = drawEnd; 
        colBuffer[x].tileID    = tile; 
        colBuffer[x].wallX     = wallX; 
        colBuffer[x].side      = side;
        colBuffer[x].lightLevel= calculatedLight;
    }
}

void Raycaster::CalculateRowGeometry(Camera* camera){
    int horizon = (scrHeight >> 1) + camera->pitch;

    float rayDirX0 = camera->dir.x - camera->plane.x;
    float rayDirY0 = camera->dir.y - camera->plane.y;
    float rayDirX1 = camera->dir.x + camera->plane.x;
    float rayDirY1 = camera->dir.y + camera->plane.y;
    
    float invWidth = 1.0f / scrWidth;

    #pragma omp parallel for schedule(static)
    for(int y = 0; y < scrHeight; y++){
        int p = y - horizon;          //cam position from the centre of screen
        if(p == 0) {
            rowBuffer[y].startFloorX = 0;
            rowBuffer[y].startFloorY = 0;
            rowBuffer[y].stepX = 0;
            rowBuffer[y].stepY = 0;
            rowBuffer[y].distance = camera->maxViewDist + 1.0f;
            continue;
        };

        float camHeight = (p > 0) ? ((0.5f * scrHeight) + camera->z) : ((0.5f * scrHeight) - camera->z);
        float rowDistance = camHeight / (float)std::abs(p);

        if(rowDistance >= camera->maxViewDist){
            rowBuffer[y].distance = rowDistance;
            continue;
        }

        float floorStepX = rowDistance * (rayDirX1 - rayDirX0) * invWidth;
        float floorStepY = rowDistance * (rayDirY1 - rayDirY0) * invWidth;

        // fixed point math
        rowBuffer[y].startFloorX = (int32_t)((camera->pos.x + rowDistance * rayDirX0) * 65536.0f);
        rowBuffer[y].startFloorY = (int32_t)((camera->pos.y + rowDistance * rayDirY0) * 65536.0f);
        rowBuffer[y].stepX = (int32_t)(floorStepX * 65536.0f);
        rowBuffer[y].stepY = (int32_t)(floorStepY * 65536.0f);
        rowBuffer[y].distance = rowDistance;
    }
}

void Raycaster::ProjectSprite(float spriteX, float spriteY, float spriteZ, Camera* cam, SpriteProjection* proj, float frameWidth, float frameHeight){
    float relativeX = spriteX - cam->pos.x;
    float relativeY = spriteY - cam->pos.y;
    
    // inverse determinant 
    float invDet = 1.0f / (cam->plane.x * cam->dir.y - cam->plane.y * cam->dir.x);

    // inverse camear transform matrix
    float transformX = invDet * (cam->dir.y * relativeX - cam->dir.x * relativeY);
    float transformY = invDet * (cam->plane.x * relativeY - cam->plane.y * relativeX);

    proj->distance = transformY;
    if(transformY <= 0.1f) return;

    float invTransformY = 1.0f / transformY;

    int baseHeight = std::abs((int)(scrHeight * invTransformY));
    int floorY = (scrHeight >> 1) + (baseHeight >> 1) + cam->pitch + (int)((cam->z - spriteZ) * invTransformY);

    float scale = frameHeight / 64.0f;
    if(scale >= 0.4f) scale += 0.5f;

    int spriteHeight = (int)(baseHeight * scale);
    int spriteWidth = (spriteHeight * frameWidth) / frameHeight;

    int spriteScreenX = (int)((scrWidth >> 1) * (1.0f + transformX * invTransformY));

    float vOffset = 60.0f; 
    int screenVOffset = (int)(vOffset * invTransformY);
    
    proj->drawStartX = spriteScreenX - (spriteWidth >> 1);
    proj->drawEndX   = spriteScreenX + (spriteWidth >> 1);
    proj->drawStartY = floorY - spriteHeight + screenVOffset;
    proj->drawEndY   = floorY + screenVOffset;
}
