#include "Engine.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <cstdio>

Engine::Engine() : window(nullptr), renderer(nullptr), texture(nullptr), lastFrameTime(0),
                   scrWidth(0), deltaTime(0.0f), isRunning(false) {}

Engine::~Engine() {
    Quit();
}

bool Engine::Initialize(int width, int height, const char* title){
    scrWidth = width;
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0){ 
        std::cerr << "SDL Failed to Initialize: " << SDL_GetError() << "\n";
        return false;
    }

    input.Init();

    window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_SHOWN
    );
    
    if(!window){
        std::cerr << "Failed to Create SDL window: " << SDL_GetError() << "\n";
        Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    if(!renderer){
        std::cerr << "Failed to create SDL renderer: " << SDL_GetError() << "\n";
        Quit();
        return false;
    }

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        width, height
    );

    if(!texture) {
        std::cerr << "Failed to create Texture: " << SDL_GetError() << "\n";
        Quit();
        return false;
    }

    isRunning = true;
    lastFrameTime = SDL_GetPerformanceCounter();
    performanceFreq = 1.0f / (double)SDL_GetPerformanceFrequency();

    return true;
}

void Engine::Quit(){    
    if(texture){
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }
    if(renderer){
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if(window){
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
    isRunning = false;
}

void Engine::Update(){
    input.Update();
    
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        if(event.type == SDL_QUIT){
            isRunning = false;
        }
    }

    uint64_t currentFrameTime = SDL_GetPerformanceCounter();
    deltaTime = static_cast<float>((double)(currentFrameTime - lastFrameTime) * performanceFreq);
    lastFrameTime = currentFrameTime;

    frameCount++;
    totalFrames++;
    runTime += deltaTime;
    fpsTimer += deltaTime;
    if(fpsTimer >= 1.0f){
        curFPS = frameCount;
        frameCount = 0;
        fpsTimer -= 1.0f;
    }
}

void Engine::Wait(){
    if(targetFrameTime <= 0.0f) return;

    while(true){
        uint64_t currentFrameTime = SDL_GetPerformanceCounter();
        float elapsed = (float)(currentFrameTime - lastFrameTime) * performanceFreq;

        if(elapsed >= targetFrameTime){
            break;
        }

        if(targetFrameTime - elapsed > 0.002f){
            SDL_Delay(1);
        }
    }
}

void Engine::Present(const uint16_t* pixels){
    SDL_UpdateTexture(texture, nullptr, pixels, scrWidth * sizeof(uint16_t));
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void Engine::WriteStats(const char* path){
    double avg = (runTime > 0.0) ? (double)totalFrames / runTime : 0.0;

    printf("frames=%llu\nseconds=%.3f\navgfps=%.2f\n",
           (unsigned long long)totalFrames, runTime, avg);

    FILE* f = fopen(path, "w");
    if(!f){
        std::cerr << "WARNING: could not write stats to " << path << "\n";
        return;
    }
    fprintf(f, "frames=%llu\nseconds=%.3f\navgfps=%.2f\n",
            (unsigned long long)totalFrames, runTime, avg);
    fclose(f);
}
