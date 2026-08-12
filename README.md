# Custom Raycasting Engine

A 2.5D software rendering engine written from scratch in C++.

## What it does:
- Raycasts walls, textured floors/ceilings, and skyboxes
- Draws billboard sprites with depth sorting and shadows
- Particle system with physics
- Loads 16-bit BMP textures and spritesheet animations (RLE encoded)
- Camera with pitch, zoom, and adjustable view distance
- Keyboard + gamepad input
- Frame rate limiter

## Optimizations:
The whole thing is CPU-rendered, no GPU:

- DDA raycasting (no sqrt, no fish-eye correction needed)
- Bresenham-style shadow projection
- Trig lookup tables instead of calling sin/cos every frame
- Fixed-point math for texture mapping 
- OpenMP parallelism across cores for column and floor rendering
- Radix sort for sprite ordering (memcpy the float bits into uint32 and sort those)
- 1D z-buffer per column, not per pixel
- Cache-blocked 32x32 tile rendering so the CPU isn't wasting L1
- `__restrict__` hints for the inner pixel loops
- Transposed pixel buffer so vertical line draws are sequential in memory
- Static arrays for sprite draw lists (no heap alloc during rendering)
- Frustum culling + distance culling to skip sprites early
- Bit-shifting for fog/shadow blending instead of division
- Bitmask array to prevent double-drawing shadows
- SoA particle layout with swap-and-pop removal

## Building for PC
```bash
make clean run
```

## Building & Deploying to RG35XX Plus
Cross-compiles for `aarch64`. Needs `aarch64-linux-gnu-g++`.

1. Copy `.env.example` to `.env` and set `DEVICE_IP`
2. Run:
```bash
make PLATFORM=rg35xx deploy
```
This compiles and SCPs the binary + assets to `/userdata/roms/ports/wolf_game/` over SSH.

## Credits
- Sprites: **CHUNSOFT** - https://www.spike-chunsoft.com/

## Resources I used
- https://lodev.org/cgtutor/raycasting.html
- https://wynnliam.github.io/raycaster/news/tutorial/2019/03/23/raycaster-part-01.html
- https://youtu.be/K1xEkA46CuM
- https://wiki.libsdl.org/SDL3/Tutorials
