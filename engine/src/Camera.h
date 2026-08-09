#ifndef CAMERA_H
#define CAMERA_H

#include "Mathutil.h"

class Camera{
public:
    Vector2 pos;
    Vector2 dir;
    Vector2 plane;
    
    float absAngle;
    float fov;

    float z;
    int pitch;
    
    float maxViewDist = 13.0f;
    float maxViewDistSqr = 169.0f;
    bool distanceFogEnabled = true;

    void SetViewDistance(float dist, bool enableFog) {
        maxViewDist = dist;
        maxViewDistSqr = dist * dist;
        distanceFogEnabled = enableFog;
    }

    Camera();
    Camera(Vector2 startPos, float startAngle, float fov = 0.66f);
    ~Camera() = default;

    void Update();

    void Rotate         (float angleDelta);
}; 

#endif
