#include "Rasterizer.h"
#include "Mathutil.h"
#include "Raycaster.h"
#include "AssetMgr.h"
#include "Palette.h"
#include "Sprite.h"
#include "Camera.h"
#include "Font.h"
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <cmath>

const int aoThreshold = 16;
const int minShadowBrightness = 128;
const int shadowRange = 256 - minShadowBrightness;

const int MAX_VISIBLE_SPRITES = 4096;
static SpriteDrawInfo drawList[MAX_VISIBLE_SPRITES];
static SpriteDrawInfo tempSwapList[MAX_VISIBLE_SPRITES];

inline uint16_t ShadePixel(uint16_t color, int fog){
    if(fog >= 256) return color;
    if(fog <= 0) return 0x0000;

    uint32_t r = (color >> 11) & 0x1F;
    uint32_t g = (color >> 5) & 0x3F;
    uint32_t b = color & 0x1F;

    r = ((r * fog) + 128) >> 8;
    g = ((g * fog) + 128) >> 8;
    b = ((b * fog) + 128) >> 8;

    return (r << 11) | (g << 5) | b;
}

int Rasterizer::CalculateFog(float distance, Camera* cam){
    if(!cam->distanceFogEnabled) return 256;

    float fogMaxDist = cam->maxViewDist;
    float fogMinDist = cam->maxViewDist * 0.5f;
    
    if(distance <= fogMinDist) return 256;
    if(distance >= fogMaxDist) return 0;

    const float fogScale = 256.0f / (fogMaxDist - fogMinDist);
    return 256 - (int)((distance - fogMinDist) * fogScale); 
}

Rasterizer::Rasterizer() : width(0), height(0), pixels(nullptr) {}

Rasterizer::~Rasterizer(){
    CleanUp();
}

bool Rasterizer::Initialize(int scrWidth, int scrHeight){
    width  = scrWidth;
    height = scrHeight;

    pixels = new uint16_t[width * height];
    display = new uint16_t[width * height];
    shadowBuffer = new uint32_t[(width * height) / 32 + 1];
    return true;
}

void Rasterizer::CleanUp(){
    if(pixels != nullptr){
        delete[] pixels;
        pixels = nullptr;
    }
    if(display != nullptr){
        delete[] display;
        display = nullptr;
    }
    if(shadowBuffer != nullptr){
        delete[] shadowBuffer;
        shadowBuffer = nullptr;
    }
    return;
}

void Rasterizer::Clear(uint16_t color){
    int size = width * height;
    std::fill_n(pixels, size, color);
}


void Rasterizer::TransposeTo(uint16_t* dest, int pitch){
    int destWidth = pitch / 2;
    const int BLOCK_SIZE = 32;

    #pragma omp parallel for collapse(2) schedule(static)
    for(int i = 0; i < width; i += BLOCK_SIZE){
        for(int j = 0; j < height; j += BLOCK_SIZE){
            int maxX = std::min(i + BLOCK_SIZE, width);
            int maxY = std::min(j + BLOCK_SIZE, height);

            for(int y = j; y < maxY; y++){
                int destOffset = y * destWidth;
                for(int x = i; x < maxX; x++){
                    dest[destOffset + x] = pixels[x * height + y];
                }
            }
        }
    }
}

void Rasterizer::DrawPixel(int x, int y, uint16_t color){
    if(x >= 0 && x < width && y >= 0 && y < height){
        pixels[x * height + y] = color;
    }
}

void Rasterizer::DrawChar(int x, int y, char c, uint16_t color){
    if(c < '0' || c > '9') return;
    uint64_t charData = font[c - '0'];

    for(int i = 0; i < 64; i++){
        if(charData & (1ULL << (63 - i))){
            int px = x + (i & 7);
            int py = y + (i >> 3);

            int pixIndex = px * height + py;
            if(pixIndex >= 0 && pixIndex < width * height){
                pixels[pixIndex] = color;
            }
        }
    }
}

void Rasterizer::DrawStr(int x, int y, const std::string& s, uint16_t color){
    int curX = x;
    for(char c : s){
        DrawChar(curX, y, c, color);
        curX += 8;
    }
}

void Rasterizer::DrawFPS(int fps, int x, int y, uint16_t color){
    int curX = x;
    
    if(fps >= 100){
        DrawChar(curX, y, '0' + ((fps / 100) % 10), color);
        curX += 8;
    }

    if(fps >= 10){
        DrawChar(curX, y, '0' + ((fps / 10) % 10), color);
        curX += 8;
    }

    DrawChar(curX, y, '0' + (fps % 10), color);
}

void Rasterizer::DrawTexturedVLine(int x, int startY, int endY, float texPos, float texStep, uint16_t* slice, int fog){
    uint16_t* __restrict__ dst = pixels;
    const uint16_t* __restrict__ src = slice;

    int32_t fixedPos  = (int32_t)(texPos * 65536.0f);
    int32_t fixedStep = (int32_t)(texStep * 65536.0f);

    // stored rotated to avoide cache misses
    int pixIndex = x * height + startY;
    for(int y = startY; y < endY; y++){
        int currFog = fog;
        int disFromTop = y - startY;
        int disFromBot = (endY - 1) - y;
        int dis = std::min(disFromTop, disFromBot);

        if(dis < aoThreshold){
            int multiplier = minShadowBrightness + ((shadowRange * dis) >> 4);
            currFog = (currFog * multiplier) >> 8;
        }

        dst[pixIndex] = ShadePixel(src[fixedPos >> 16], currFog);
        fixedPos += fixedStep;
        pixIndex++;
    }
}

void Rasterizer::DrawWalls(ColumnGeometry* colBuffer, AssetMgr* assets, Palette* palette, Camera* cam){
    float invHeight = 1.0f / (float)height;

    #pragma omp parallel for schedule(guided)
    for(int x = 0; x < width; x++){

        ColumnGeometry& column = colBuffer[x];
        if(column.distance >= cam->maxViewDist){
            DrawVLine(x, column.drawStart, column.drawEnd, 0x0000);
            continue;
        }

        // fog based on distance
        int depthFog = CalculateFog(column.distance, cam);

        // fog combined with directional lighting
        int finalFog = (depthFog * column.lightLevel) >> 8;

        Texture* tex = assets->GetTexture(column.tileID);

        if(tex != nullptr){
            int texX = (float)(column.wallX * tex->width);

            float invDistance = 1.0f / column.distance;
            float exactVertHeight = (float)height * invDistance;
            float step = column.distance * ((float)tex->height * invHeight);

            float camOffset = cam->pitch + (cam->z * invDistance);
            float exactDrawStart = ((float)height * 0.5f) - (exactVertHeight * 0.5f) + camOffset;

            float texPos = ((float)column.drawStart - exactDrawStart) * step;

            uint16_t* slice = tex->pixels + (texX << tex->shift);
            DrawTexturedVLine(x, column.drawStart, column.drawEnd, texPos, step, slice, finalFog);
        }
        else{
            uint16_t color = palette->GetColor(column.tileID);
            if(column.side == 1) color = (color >> 1) & 0x7BEF;
            DrawVLine(x, column.drawStart, column.drawEnd, color);
        }
    }
}

void Rasterizer::DrawHorizon(ColumnGeometry* colBuffer, uint16_t ceil, uint16_t floor){
    for(int x = 0; x < width; x++){
        ColumnGeometry& col = colBuffer[x];

        DrawVLine(x, 0, col.drawStart, ceil);
        DrawVLine(x, col.drawEnd, height, floor);
    } 
}

void Rasterizer::DrawTexturedHorizon(ColumnGeometry* colBuffer, RowGeometry* rowBuffer, Texture* floorTex, Texture* ceilTex, Camera* cam, bool isSky){
    if(!floorTex || !ceilTex) return;
    int horizon = (height >> 1) + cam->pitch;

    uint16_t* __restrict__ dst = pixels;
    const uint16_t* __restrict__ floorPx = floorTex->pixels;
    const uint16_t* __restrict__ ceilPx  = ceilTex->pixels;

    int fShift = 16 - floorTex->shift;
    int cShift = 16 - ceilTex->shift; 

    int rowFog[height];
    for(int y = 0; y < height; y++){
        rowFog[y] = CalculateFog(rowBuffer[y].distance, cam);
    }

    int skyTexOffset[width];
    int skyTexY_array[height];
    int skyHeight = 0;
    int skyWidth = 0;

    if(isSky){
        skyWidth = ceilTex->width;
        skyHeight = ceilTex->height;
        
        for(int y = 0; y < height; y++){
            int texY = ((y - cam->pitch - (height >> 1)) * skyHeight) / (height >> 1) + skyHeight - 1;
            if(texY < 0) texY = 0;
            if(texY >= skyHeight) texY = skyHeight - 1;
            skyTexY_array[y] = texY;
        }

        float centerSkyX = (cam->absAngle / 4096.0f) * skyWidth;
        float skyVisibleWidth = skyWidth * 0.185f;
        float skyStep = skyVisibleWidth / (float)width;
        float startSkyX = centerSkyX - (skyVisibleWidth * 0.5f);

        for(int x = 0; x < width; x++){
            int texX = (int)(startSkyX + x * skyStep) % skyWidth;
            texX = (skyWidth - texX) % skyWidth;
            if(texX < 0) texX += skyWidth;
            skyTexOffset[x] = texX * skyHeight;
        }
    }

    const int BLOCK = 32;

    #pragma omp parallel for collapse(2) schedule(static)
    for(int startX = 0; startX < width; startX += BLOCK){
        for(int startY = 0; startY < height; startY += BLOCK){
            int endX = std::min(startX + BLOCK, width);
            int endY = std::min(startY + BLOCK, height);

            for(int x = startX; x < endX; x++){
                int pixIndex = x * height;
                for(int y = startY; y < endY; y++){
                    if(y == horizon) continue;
                   
                    if(y >= horizon){
                        if(rowBuffer[y].distance >= cam->maxViewDist){
                            if(y >= colBuffer[x].drawEnd) dst[pixIndex + y] = 0x0000;
                        }
                        else{
                            if(y >= colBuffer[x].drawEnd){
                                int32_t floorX = rowBuffer[y].startFloorX + x * rowBuffer[y].stepX;
                                int32_t floorY = rowBuffer[y].startFloorY + x * rowBuffer[y].stepY;

                                uint16_t color = floorPx[(((floorY >> fShift) & floorTex->mask) << floorTex->shift) + ((floorX >> fShift) & floorTex->mask)];
                                
                                int currFog = rowFog[y];
                                int disFromWallBase = y - colBuffer[x].drawEnd;
                                if(disFromWallBase < aoThreshold){
                                    int multiplier = minShadowBrightness + ((shadowRange * disFromWallBase) >> 4);
                                    currFog = (currFog * multiplier) >> 8;
                                }

                                dst[pixIndex + y] = ShadePixel(color, currFog);
                            }
                        }
                    }
                    else{ //draw sky/ ceiling
                        if(y < colBuffer[x].drawStart){
                            if(isSky){
                                dst[pixIndex + y] = ceilPx[skyTexOffset[x] + skyTexY_array[y]];
                            }
                            else{
                                if(rowBuffer[y].distance >= cam->maxViewDist){
                                    if(y < colBuffer[x].drawStart) dst[pixIndex + y] = 0x0000;
                                }
                                else{
                                    int32_t floorX = rowBuffer[y].startFloorX + x * rowBuffer[y].stepX;
                                    int32_t floorY = rowBuffer[y].startFloorY + x * rowBuffer[y].stepY;

                                    uint16_t color = ceilPx[(((floorY >> cShift) & ceilTex->mask) << ceilTex->shift) + ((floorX >> cShift) & ceilTex->mask)];

                                    int currFog = rowFog[y];
                                    int disFromWallTop = (colBuffer[x].drawStart - 1) - y;
                                    if(disFromWallTop < aoThreshold){
                                        int multiplier = minShadowBrightness + ((shadowRange * disFromWallTop) >> 4);
                                        currFog = (currFog * multiplier) >> 8;
                                    }

                                    dst[pixIndex + y] = ShadePixel(color, currFog);
                                }
                            }
                        }
                    }
                }
            }
        }
    }       
}

void Rasterizer::DrawSprite(Sprite& sprite, Camera* cam, Raycaster* raycaster){
    SpriteFrame* frame = &sprite.sheet->frames[sprite.currentFrame];

    SpriteProjection proj;
    raycaster->ProjectSprite(sprite.x, sprite.y, sprite.z, cam, &proj, frame->width, frame->height);

    if(proj.distance <= 0.1f) return;
    if(proj.distance >= cam->maxViewDist) return;

    int drawStartX = std::max(0, proj.drawStartX);
    int drawEndX   = std::min(width, proj.drawEndX);

    int spriteWidth = proj.drawEndX - proj.drawStartX;
    if(spriteWidth <= 0) return;

    int32_t texX_step = (frame->width << 16) / spriteWidth;
    int32_t texX_FP   = (drawStartX - proj.drawStartX) * texX_step;
        
    int fog = CalculateFog(proj.distance, cam);
    
    for(int x = drawStartX; x < drawEndX; x++){
        int texX = texX_FP >> 16;
        texX_FP += texX_step;

        if(proj.distance >= raycaster->GetColBuffer()[x].distance) continue;       

        SpriteColumn* col = &frame->columns[texX];
    
        if(col->numRuns == 0) continue;

        DrawVertSprite(x, proj.drawStartY, proj.drawEndY, col, frame->height, fog);
    }
}

void Rasterizer::DrawVertSprite(int x, int startY, int endY, SpriteColumn* column, int frameHeight, int fog){
    int spriteHeight = endY - startY;
    if(spriteHeight <= 0) return;

    int32_t step = (frameHeight * 65536) / spriteHeight;        // for runData index
    
    int32_t invStep = (spriteHeight << 16) / frameHeight;       //for screenY and runHeight

    for(int i = 0; i < column->numRuns; i++){
        SpriteRun& run = column->runs[i];
    
        int screenY = startY + ((run.start * invStep) >> 16);
        int runHeight = ((run.size * invStep) >> 16);
        int screenEndY = screenY + runHeight;

        int drawStartY = std::max(0, screenY);
        int drawEndY   = std::min(height, screenEndY);

        int32_t texPos = 0;
        if(screenY < 0){
            texPos += step * (-screenY);
        }

        // pointer aliasing SMID
        uint16_t* __restrict__ dst = pixels;
        const uint16_t* __restrict__ src = run.runData;

        int pixIndex = x * height + drawStartY;
        for(int y = drawStartY; y < drawEndY; y++){
            dst[pixIndex] = ShadePixel(src[texPos >> 16], fog);
            texPos += step;
            pixIndex++;
        }
    }
}

void Rasterizer::DrawVertShadow(int x, int startY, int endY, SpriteColumn* col, int frameHeight, int drawEndY, float shearX, float scaleY, ColumnGeometry* colBuffer){
    int spriteHeight = endY - startY; 
    if(spriteHeight <= 0) return;

    int32_t invStep = (spriteHeight << 16) / frameHeight; 

    int32_t fixedShearX = (int32_t)(shearX * 65536.0f);
    int32_t fixedScaleY = (int32_t)(scaleY * 65536.0f);

    for(int i = 0; i < col->numRuns; i++){
        SpriteRun& run = col->runs[i];

        int screenY = startY + ((run.start * invStep) >> 16);
        int runHeight = ((run.size * invStep) >> 16);
        int screenEndY = screenY + runHeight;

        int h_start = drawEndY - screenY;
        if(h_start < 0) h_start = 0;

        int32_t shadowX_FP = (x << 16) + h_start * fixedShearX;
        int32_t shadowY_FP = (drawEndY << 16) - h_start * fixedScaleY;

        for(int y = screenY; y < screenEndY; y++){
            int cx = shadowX_FP >> 16;
            int cy = shadowY_FP >> 16;

            if(cx >= 0 && cx < width && cy >= 0 && cy < height){
                if(cy >= colBuffer[cx].drawEnd){
                    int pixIndex = cx * height + cy;
                    int intIndex = pixIndex >> 5; 
                    uint32_t bitMask = 1U << (pixIndex & 31);
                    
                    if(!(shadowBuffer[intIndex] & bitMask)){
                        pixels[pixIndex] = ShadePixel(pixels[pixIndex], 170); 
                        shadowBuffer[intIndex] |= bitMask;
                    }
                }
            }

            shadowX_FP -= fixedShearX;
            shadowY_FP += fixedScaleY;
        }
    }
}

void Rasterizer::DrawShadowSprite(Sprite& sprite, Camera* cam, Raycaster* ray, Vector2 sunDir){
    SpriteFrame* frame = &sprite.sheet->frames[sprite.currentFrame];

    SpriteProjection proj;
    ray->ProjectSprite(sprite.x, sprite.y, 0.0f, cam, &proj, frame->width, frame->height);

    if(proj.distance < 0.1f) return;

    int spriteWidth = proj.drawEndX - proj.drawStartX;
    if(spriteWidth <= 0) return;

    Vector2 shadowDir(-sunDir.x, -sunDir.y);
    float localY = shadowDir.Dot(cam->dir);
    float localX = shadowDir.Dot(cam->plane);

    float scaleY = localY * 0.4f;
    float shearX = localX * 1.5f;

    int drawStartX = proj.drawStartX;
    int drawEndX = proj.drawEndX;

    int32_t texX_step = (frame->width << 16) / spriteWidth;
    int32_t texX_FP = 0; 

    for(int x = drawStartX; x < drawEndX; x++){
        int texX = texX_FP >> 16;
        texX_FP += texX_step;

        if(texX < 0 || texX >= frame->width) continue;

        if(x >= 0 && x < width){
            if(proj.distance >= ray->GetColBuffer()[x].distance) continue;
        }

        SpriteColumn* col = &frame->columns[texX];
        if(col->numRuns == 0) continue;

        DrawVertShadow(x, proj.drawStartY, proj.drawEndY, col, frame->height, proj.drawEndY, shearX, scaleY, ray->GetColBuffer());
    }
}

void Rasterizer::SortDrawList(Camera* cam){
    float halfFov = std::atan(cam->fov);
    float padding = 0.4f;
    float threshold = cosf(halfFov + padding);
    float thresholdSqr = threshold * threshold;

    int spriteCount = 0;

    for(Sprite* sprite : renderQueue){
        if(spriteCount >= MAX_VISIBLE_SPRITES) break;

        Vector2 spritePos(sprite->x, sprite->y);
        Vector2 spriteVec = spritePos - cam->pos;

        float dotUnnormalized = spriteVec.Dot(cam->dir);    // is sprite in direction of our camera dir
        if(dotUnnormalized <= 0.0f) continue;

        float distSqr = spriteVec.Dot(spriteVec);           //just pythagorus
        if(distSqr >= cam->maxViewDistSqr) continue;

        if(((dotUnnormalized * dotUnnormalized) / distSqr) < thresholdSqr) continue;

        uint32_t distBits;
        std::memcpy(&distBits, &distSqr, sizeof(uint32_t));

        drawList[spriteCount].sprite = sprite;
        drawList[spriteCount].disSqrBits = distBits;
        spriteCount++;
    }

    // Radix sort
    SpriteDrawInfo* src = drawList;
    SpriteDrawInfo* dst = tempSwapList;

    for(int shift = 0; shift <= 24; shift += 8){
        int count[256] = {0};

        for(int i = 0; i < spriteCount; i++){
            count[(src[i].disSqrBits >> shift) & 0xFF]++;
        }

        for(int i = 254; i >= 0; i--){
            count[i] += count[i + 1];
        }

        // descending order
        for(int i = spriteCount - 1; i >= 0; i--){
            int buckets = (src[i].disSqrBits >> shift) & 0xFF;
            dst[--count[buckets]] = src[i];
        }

        SpriteDrawInfo* temp = src;
        src = dst;
        dst = temp;
    }

    this->sortedSprites = src;
    this->visibleSprites = spriteCount;
}

void Rasterizer::DrawSprites(Camera* cam, Raycaster* raycaster, Vector2 sunDir, bool drawShadow){
    SortDrawList(cam);

    if(drawShadow){
        std::memset(shadowBuffer, 0, ((width * height) / 32 + 1) * sizeof(uint32_t));
    }

    for(int i = 0; i < visibleSprites; i++){
        if(drawShadow){
            DrawShadowSprite(*(sortedSprites[i].sprite), cam, raycaster, sunDir);
        }
        DrawSprite(*(sortedSprites[i].sprite), cam, raycaster);
    } 
    renderQueue.clear();
}
