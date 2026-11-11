#include "game.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

int main(int argc, char** argv){
    int width = 640, height = 480;
    float benchSeconds = 0.0f;
    bool stressMode = false;
    int shotFrame = 0;

    for(int i = 1; i < argc; i++){
        if(strcmp(argv[i], "--bench") == 0 && i + 1 < argc){
            benchSeconds = (float)atof(argv[++i]);
        }
        else if(strcmp(argv[i], "--stress") == 0){
            stressMode = true;
        }
        else if(strcmp(argv[i], "--shot") == 0 && i + 1 < argc){
            shotFrame = atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "--res") == 0 && i + 1 < argc){
            if(sscanf(argv[++i], "%dx%d", &width, &height) != 2){
                fprintf(stderr, "bad --res, expected WxH (e.g. 960x720)\n");
                return 1;
            }
        }
    }

    Game game;
    game.SetBenchDuration(benchSeconds);
    game.SetStressMode(stressMode);
    game.SetShotFrame(shotFrame);

    if(game.Initialize(width, height, "Doom")){
        game.Run();
    }
    return 0;
}
