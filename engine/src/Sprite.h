#ifndef SPRITE_H
#define SPRITE_H

#include <cstdint>

// Run-Length Encoding(RLE) - in testing

struct SpriteRun{
    int16_t start;    // where run starts
    int16_t size;     // size of the run 
    uint16_t* runData;
};

struct SpriteColumn{
    int numRuns;
    SpriteRun* runs;
};

struct SpriteFrame{
    int width, height;
    SpriteColumn* columns;

    ~SpriteFrame(){
        if(columns){
            for(int i = 0; i < width; i++){
                if(columns[i].runs){
                    delete[] columns[i].runs;
                }
            }
            delete[] columns;
        }
    }
};

struct SpriteSheet{
    SpriteFrame* frames;
    int numFrames;
    uint16_t* solidPixelData;

    ~SpriteSheet(){
        if(frames) delete[] frames;   
        if(solidPixelData) delete[] solidPixelData;
    }
};

struct Sprite{
    SpriteSheet* sheet;
    float x, y, z;
    int currentFrame;
};

struct SpriteProjection{
    int drawStartX;
    int drawStartY;
    int drawEndX;
    int drawEndY;
    float distance;
};

struct SpriteDrawInfo{
    Sprite* sprite;
    uint32_t disSqrBits;
};

#endif
