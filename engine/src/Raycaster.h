#ifndef RAYCASTER_H
#define RAYCASTER_H

#include "Mathutil.h"
#include "Sprite.h"
#include <cstdint>

struct SpriteProjection;
class Rasterizer;
class AssetMgr;
class Palette;
class Camera;
class Map;

struct ColumnGeometry{
    float distance;
    float wallX;
    uint16_t   drawStart;
    uint16_t   drawEnd;
    unsigned char tileID;
    bool side;
    int lightLevel;
};

struct RowGeometry{
    int32_t startFloorX;
    int32_t startFloorY;
    int32_t stepX;
    int32_t stepY;
    int32_t distance;
};

class Raycaster{
private:
    int scrWidth;
    int scrHeight;

    ColumnGeometry* colBuffer;
    RowGeometry* rowBuffer;

public:
    Raycaster(int w, int h);
    ~Raycaster();

    void RaycastColumn(Camera* camera, Map* map, Vector2 sunDir, int x, ColumnGeometry* out);

    void CalculateRowGeometry(Camera* camera);

    ColumnGeometry* GetColBuffer() {return colBuffer;}
    RowGeometry* GetRowBuffer() {return rowBuffer;}

    void ProjectSprite(float spriteX, float spriteY, float spriteZ, Camera* cam, SpriteProjection* proj, float frameWidth, float frameHeight);
};

#endif
