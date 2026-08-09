#ifndef RASTERIZER_H
#define RASTERIZER_H

#include "Sprite.h"
#include <string>
#include <cstdint>
#include <algorithm>
#include <vector>

struct ColumnGeometry;
struct RowGeometry;
struct Texture;
struct SpriteColumn;
struct Sprite;
struct Vector2;
class Raycaster;
class AssetMgr;
class Palette;
class Camera;

class Rasterizer{
private:
    int width;
    int height;

    uint16_t* pixels;
    uint16_t* display = nullptr;
    uint32_t* shadowBuffer = nullptr;

    std::vector<Sprite*> renderQueue;

    SpriteDrawInfo* sortedSprites = nullptr;
    int visibleSprites = 0;

private:
    int CalculateFog(float distance, Camera* cam);

    void SortDrawList(Camera* cam);
    
    void DrawVertSprite(int x, int startY, int endY, SpriteColumn* column, int frameHeight, int fog);
    
    void DrawSprite(Sprite& sprite, Camera* cam, Raycaster* raycaster);
    
    void DrawVertShadow(int x, int startY, int endY, SpriteColumn* col, int frameHeight, int fog, int drawEndY, float shearX, float scaleY, ColumnGeometry* colBuffer);

    void DrawShadowSprite(Sprite& sprite, Camera* cam, Raycaster* ray, Vector2 sunDir);

public:
    Rasterizer();
    ~Rasterizer();

    bool Initialize(int scrWidth, int scrHeight);

    void QueueSprite(Sprite* sprite) {renderQueue.push_back(sprite);}

    void TransposeTo(uint16_t* dest, int pitch);

    void CleanUp();

    void Clear(uint16_t color);

    void DrawPixel(int x, int y, uint16_t color);
    
    void DrawFPS(int fps, int x, int y, uint16_t color);

    void DrawChar(int x, int y, char c, uint16_t color);
   
    void DrawStr(int x, int y, const std::string& s, uint16_t color);
    
    inline void DrawVLine(int x, int startY, int endY, uint16_t color){
        // stored rotated 
        int pixIndex = x * height + startY;
        for(int y = startY; y < endY; y++){
            pixels[pixIndex] = color;
            pixIndex++;
        } 
    }

    void DrawTexturedVLine(int x, int startY, int endY, float texPos, float texStep, uint16_t* slice, int fog);
    
    void DrawWalls(ColumnGeometry* colBuffer, AssetMgr* assets, Palette* palette, Camera* cam);

    void DrawHorizon(ColumnGeometry* colBuffer, uint16_t ceil, uint16_t floor);

    void DrawTexturedHorizon(ColumnGeometry* colBuffer, RowGeometry* rowBuffer, Texture* floor, Texture* ceil, Camera* cam, bool drawCeil);

    void DrawSprites(Camera* cam, Raycaster* raycaster, Vector2 sunDir, bool drawShadow);

    
    int GetWidth() const {return width;}
    int GetHeight() const {return height;}
    
    const uint16_t* GetPixelBuffer(){
        TransposeTo(display, width * sizeof(uint16_t));
        return display;
    }
};

#endif
