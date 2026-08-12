#ifndef ASSETMGR_H
#define ASSETMGR_H

#define MAX_TILE_SIZE   256

#include <string>
#include <unordered_map>
#include <cstdint>

class Map;

struct SpriteSheet;

struct Texture{
    int height, width;
    uint16_t* pixels;

    int shift;
    int mask;

    Texture() : pixels(nullptr) {}
    ~Texture() {
        if(pixels) delete[] pixels;
    }
};

class AssetMgr{
private:
    std::unordered_map<std::string, SpriteSheet*> loadedSprites;
    std::unordered_map<std::string, Map*> loadedMaps;
    Texture* textures[MAX_TILE_SIZE];
    
public:
    AssetMgr();
    ~AssetMgr();

    Map* LoadMap(const char* filepath, int requestedSize);

    Texture* LoadTexture(unsigned char tileID, const char* filepath);

    SpriteSheet* LoadSpriteSheet(const char* filepath, int frameWidth, int frameHeight, uint16_t bgColor);

    Texture* GetTexture(unsigned char tileID) {return textures[tileID];}
};

#endif
