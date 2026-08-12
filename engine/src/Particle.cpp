#include "Particle.h"
#include "Camera.h"
#include "Rasterizer.h"
#include "Raycaster.h"
#include <cmath>

Particle::Particle(){
    activeCount = 0;
}

Particle::~Particle() {}

void Particle::Emit(float px, float py, float pz, float pvx, float pvy, float pvz, float pLife, uint16_t pColor){
    if(activeCount >= MAX_PARTICLES) return;

    int i = activeCount;
    x[i] = px;
    y[i] = py;
    z[i] = pz;
    vx[i] = pvx;
    vy[i] = pvy;
    vz[i] = pvz;
    life[i] = pLife;
    color[i] = pColor;

    activeCount++;
}

void Particle::Update(float dt){
    for(int i = activeCount - 1; i >= 0; i--){
        life[i] -= dt;
        
        if(life[i] <= 0.0f){
            activeCount--;
            if(i != activeCount){
                x[i] = x[activeCount];
                y[i] = y[activeCount];
                z[i] = z[activeCount];

                vx[i] = vx[activeCount];
                vy[i] = vy[activeCount];
                vz[i] = vz[activeCount];

                life[i] = life[activeCount];
                color[i] = color[activeCount];
            }
            continue;
        }

        // Motion physics
        x[i] += vx[i] * dt;
        y[i] += vy[i] * dt;
        z[i] += vz[i] * dt;

        // Gravity
        vz[i] -= 800.0f * dt;

        if(z[i] < 0.0f){
            z[i] = 0.0f;
            vz[i] = -vz[i] * 0.5f;

            // friction when hit floor
            vx[i] *= 0.8f;
            vy[i] *= 0.8f;
        }
    }
}

void Particle::Render(Rasterizer* raster, Camera* cam, Raycaster* ray){
    int scrWidth = raster->GetWidth();
    int scrHeight = raster->GetHeight();
    float invDet = 1.0f / (cam->plane.x * cam->dir.y - cam->plane.y * cam->dir.x);

    ColumnGeometry* colBuffer = ray->GetColBuffer();

    for(int i = 0; i < activeCount; i++){
        float relX = x[i] - cam->pos.x;
        float relY = y[i] - cam->pos.y;

        float transformX = invDet * (cam->dir.y * relX - cam->dir.x * relY);
        float transformY = invDet * (cam->plane.x * relY - cam->plane.y * relX);

        if(transformY <= 0.1f) continue;

        int screenX = (int)((scrWidth >> 1) * (1.0f + transformX / transformY));
        if(screenX < 0 || screenX >= scrWidth - 1) continue;

        // behind a wall
        if(transformY >= colBuffer[screenX].distance) continue;

        int baseHeight = std::abs((int)scrHeight / transformY);
        int floorY = (scrHeight >> 1) + (baseHeight >> 1) + cam->pitch + (int)(cam->z / transformY);

        int screenY = floorY - (int)(z[i] / transformY);
        if(screenY < 0 || screenY >= scrHeight - 1) continue;

        raster->DrawPixel(screenX, screenY, color[i]);
        raster->DrawPixel(screenX + 1, screenY, color[i]);
        raster->DrawPixel(screenX, screenY + 1, color[i]);
        raster->DrawPixel(screenX + 1, screenY + 1, color[i]);
    }
}

