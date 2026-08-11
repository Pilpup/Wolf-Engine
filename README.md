# Custom Raycasting Engine

A 2.5D software rendering engine written from scratch in C++.

## Engine Capabilities
The engine is built purely for CPU rasterization and includes the following features:
- Draw walls (with or without textures)
- Draw floors and ceilings (with or without textures or skybox)
- Draw 2D billboard sprites (with depth sorting)
- Draw sprite shadows and distance fog
- Render 3D particle effects
- Load and parse 2D map files
- Load 16-bit BMP texture and sprite sheets
- Manage 3D camera movement (pitch ,view distance, etc)
- Mipmapping for distant walls
- Lock target frame rate
- Handle cross-platform keyboard and gamepad inputs

## Optimizations
To make the engine run extremely fast on the CPU, it uses these tricks:
- **DDA**: The raycaster uses the Digital Differential Analyzer (DDA) algorithm.
- **Bresenham's Algorithm**: The engine draws angled sprite shadows using Bresenham's line logic mixed with DDA. 
- **Math Tables**: It looks up the answers instead of doing hard math.
- **No Decimals**: The engine shifts bits to use whole numbers. This draws textures much faster.
- **Multi-Core**: The game splits the drawing work. It uses all cores on your CPU at the same time.
- **Custom Sorting**: The engine sorts thousands of sprites by reading their bits directly (Radix sort).
- **1D Depth Buffer**: Normal games check depth for every single pixel. This engine only checks depth for vertical columns.
- **Cache Blocking**: The engine draws floors in small 32x32 blocks. The CPU does not have to wait to fetch new memory data.
- **Compiler Hints**: The code uses the `__restrict__` keyword. This tells the compiler exactly where memory is located. It stops memory from overlapping.
- **Memory Transpose**: The engine flips (transposes) the pixel buffer layout in small blocks. This allows it to draw vertical lines without jumping around in memory.
- **Fixed Memory Limits**: The game uses fixed arrays for sprites. It avoids asking the computer for new memory while the game is running.
- **Early Draw Skip**: Sprites are instantly skipped if they are too far away or completely hidden.
- **Bitwise Blending**: Fog and shadow shading avoid division. They shift bits to blend colors rapidly.
- **Shadow Masking**: Shadows use a fast bitmask array. This prevents shadows from overlapping and avoids reading the screen memory twice.
- **Particle Swapping**: Particles are stored in flat lists for fast cache reading. When a particle dies, the engine swaps the last one into its place to avoid shifting memory.

## Building for PC
```bash
make clean run
```

## Building & Deploying to RG35XX (or similar handhelds)
The engine can be cross-compiled for `aarch64` too. 

1. Must have `aarch64-linux-gnu-g++` cross-compiler installed on your PC.
2. Copy `.env.example` to `.env` and set your device's IP address (`DEVICE_IP`).
3. Run the cross-compile and deploy command:
```bash
make PLATFORM=rg35xx deploy
```
This will compile the engine for the ARM processor and `scp` the binary along with the `assets/` directory directly to `/userdata/roms/ports/wolf_game/` on your device via SSH.

## Asset Credits
- **CHUNSOFT - https://www.spike-chunsoft.com/**

## Resources
- *https://lodev.org/cgtutor/raycasting.html*
- *https://wynnliam.github.io/raycaster/news/tutorial/2019/03/23/raycaster-part-01.html*
- *https://youtu.be/K1xEkA46CuM*
- *https://wiki.libsdl.org/SDL3/Tutorials*

Thanks
