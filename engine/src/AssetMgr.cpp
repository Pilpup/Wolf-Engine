#include "AssetMgr.h"
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_surface.h>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <cstring>
#include "Map.h"
#include "Sprite.h"

AssetMgr::AssetMgr() {
    for(int i = 0; i < MAX_TILE_SIZE; i++){
        textures[i] = nullptr;
    }
}

AssetMgr::~AssetMgr() {
    for (auto& pair : loadedMaps) {
        delete pair.second;
    }
    loadedMaps.clear();

    for(int i = 0; i < MAX_TILE_SIZE; i++){
        if(textures[i] != nullptr){
            delete textures[i];
            textures[i] = nullptr;
        }
    }

    for(auto& pair : loadedSprites){
        delete pair.second; 
    }
    loadedSprites.clear();
}

Map* AssetMgr::LoadMap(const char* filePath, int requestedSize){                       
    std::string pathKey(filePath);

    auto it = loadedMaps.find(pathKey);
    if (it != loadedMaps.end()) {
        std::cout << "SUCCESS: Loaded " << filePath << " from cache!" << std::endl;
        return it->second;
    }

    if (requestedSize <= 0 || (requestedSize & (requestedSize - 1)) != 0) {     
        std::cerr << "ERROR: Map size " << requestedSize << " is not a Power of 2! Defaulting to 64." << std::endl;                                               
        requestedSize = 64;                                                     
    }                                                                           
  
    Map* newMap = new Map(requestedSize);

    std::ifstream in(filePath);
    if (in.is_open()){
        bool success = true;
        for (int y = 0; y < requestedSize && success; y++){
            for (int x = 0; x < requestedSize && success; x++){
                int val;      
                if (!(in >> val)){
                    std::cerr << "WARNING: Map file corrupted" << std::endl;    
                    success = false;
                }
                else{
                    newMap->SetTile(x, y, (unsigned char)val);
                    if (in.good() && in.peek() == ',') in.ignore();
                }
            }
        }
        if(success){
            std::cout << "SUCCESS: Loaded " << filePath << " into a " << requestedSize << "x" << requestedSize << std::endl;
        }
    } 
    else {
        std::cerr << "Could not find " << filePath << ". Returning empty map." << std::endl;
    }

    loadedMaps[pathKey] = newMap;
    return newMap;
}

Texture* AssetMgr::LoadTexture(unsigned char tileID, const char* filepath){
    if(textures[tileID] != nullptr) return textures[tileID];

    SDL_Surface* surface = SDL_LoadBMP(filepath);
    if(!surface){
        std::cerr << "Failed to load Texture " << filepath << " - " << SDL_GetError() << "\n";
        return nullptr;
    }

    SDL_Surface* converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB565, 0);
    SDL_FreeSurface(surface);
    if(!converted) return nullptr;

    Texture* tex = new Texture();
    tex->width  = converted->h;
    tex->height = converted->w;

    tex->shift = __builtin_ctz(tex->width);
    tex->mask = tex->width - 1;

    int numBytes = tex->width * tex->height * sizeof(uint16_t);
    tex->pixels = new uint16_t[tex->width * tex->height];
    std::memcpy(tex->pixels, converted->pixels, numBytes);

    SDL_FreeSurface(converted);
    textures[tileID] = tex;
    return tex;
}

SpriteSheet* AssetMgr::LoadSpriteSheet(const char* filepath, int frameWidth, int frameHeight, uint16_t bgColor){
    std::string pathKey(filepath);
    auto it = loadedSprites.find(pathKey);
    if(it != loadedSprites.end()) return it->second;

    SDL_Surface* surface = SDL_LoadBMP(filepath);
    if(!surface){
        std::cerr << "Failed to load sprite " << filepath << ": " << SDL_GetError() << "\n";
        return nullptr;
    }

    SDL_Surface* converted = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGB565, 0);
    SDL_FreeSurface(surface);
    if(!converted) return nullptr;

    int sheetWidth = converted->w;
    int sheetHeight = converted->h;

    uint16_t* pixels = (uint16_t*)converted->pixels;

    SpriteSheet* sheet = new SpriteSheet();
    int cols = sheetWidth / frameWidth;
    int rows = sheetHeight / frameHeight;

    sheet->numFrames = cols * rows;
    sheet->frames = new SpriteFrame[sheet->numFrames];
    
    sheet->solidPixelData = new uint16_t[sheetWidth * sheetHeight];
    uint16_t* writePtr = sheet->solidPixelData;

    // frame extract
    int frameIdx = 0;
    for(int y = 0; y < rows; y++){
        for(int x = 0; x < cols; x++){
            SpriteFrame& frame = sheet->frames[frameIdx++];
            frame.width  = frameWidth;
            frame.height = frameHeight;
            frame.columns = new SpriteColumn[frameWidth];

            int startX = x * frameWidth;
            int startY = y * frameHeight;
            
            for(int i = 0; i < frameWidth; i++){
                int imgX = startX + i;
                
                std::vector<SpriteRun> tempRuns;
                int j = 0;
        
                while(j < frameHeight){
                    while(j < frameHeight){
                        uint16_t color = pixels[(startY + j) * sheetWidth + imgX];
                        if(color != bgColor) break;
                        j++;
                    }
                    
                    if(j >= frameHeight) break;

                    SpriteRun run;
                    run.start = j;
                    run.size = 0;
                    run.runData = writePtr;

                    while(j < frameHeight){
                        uint16_t color = pixels[(startY + j) * sheetWidth + imgX];
                        if(color == bgColor) break;
                        
                        *writePtr = color;
                        writePtr++;
                        run.size++;
                        j++;
                    }
                    tempRuns.push_back(run);
                }

                frame.columns[i].numRuns = tempRuns.size();
                if(tempRuns.empty()){
                    frame.columns[i].runs = nullptr;    
                }
                else{
                    frame.columns[i].runs = new SpriteRun[tempRuns.size()];
                    for(size_t k = 0; k < tempRuns.size(); k++){
                        frame.columns[i].runs[k] = tempRuns[k]; 
                    }
                }
            }
        }
    }

    SDL_FreeSurface(converted);
    loadedSprites[pathKey] = sheet;
    return sheet;
}
