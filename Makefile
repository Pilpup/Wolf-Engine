-include .env

PLATFORM ?= pc
TARGET = bin/game

SRC = engine/src/Camera.cpp \
      engine/src/Engine.cpp \
      engine/src/Input.cpp \
      engine/src/Map.cpp \
      engine/src/Rasterizer.cpp \
      engine/src/Raycaster.cpp \
      engine/src/Palette.cpp \
      engine/src/AssetMgr.cpp \
      engine/src/Particle.cpp \
      demo/main.cpp \
      demo/player.cpp \
      demo/enemy.cpp \
      demo/entity_mgr.cpp \
      demo/world.cpp \
      demo/game.cpp

ifeq ($(PLATFORM), rg35xx)
    # RG35XX Plus Setup
    CXX = aarch64-linux-gnu-g++
    CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -ffast-math -flto -mcpu=cortex-a53 -mtune=cortex-a53 -fopenmp
else
    # PC Setup (Default)
    CXX = g++
    CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -ffast-math -flto -fopenmp
endif

LDFLAGS = -lSDL2 -fopenmp
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p $(dir $@)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	OMP_WAIT_POLICY=PASSIVE OMP_NUM_THREADS=4 OMP_STACKSIZE=128k perf stat -d -o benchmark.txt ./$(TARGET)

deploy: $(TARGET)
ifndef DEVICE_IP
	$(error DEVICE_IP is not set. Please copy .env.example to .env and set your device IP)
endif
	ssh root@$(DEVICE_IP) "mkdir -p /userdata/roms/ports/wolf_game/assets"
	scp $(TARGET) root@$(DEVICE_IP):/userdata/roms/ports/wolf_game/game
	scp -r assets/* root@$(DEVICE_IP):/userdata/roms/ports/wolf_game/assets/

.PHONY: all clean run deploy

