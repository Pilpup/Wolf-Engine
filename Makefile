-include .env

PLATFORM ?= pc
TARGET = bin/game

all: $(TARGET)

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
    CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -ffast-math -flto -mcpu=cortex-a53 -mtune=cortex-a53 -fopenmp -MMD -MP
else
    # PC Setup (Default)
    CXX = g++
    CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -ffast-math -flto -fopenmp -MMD -MP
endif

LDFLAGS = -lSDL2 -fopenmp
OBJ = $(SRC:.cpp=.o)
DEPS = $(OBJ:.o=.d)

-include $(DEPS)

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p $(dir $@)
	$(CXX) $(OBJ) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(DEPS) $(TARGET)

run: $(TARGET)
	OMP_WAIT_POLICY=PASSIVE OMP_NUM_THREADS=4 OMP_STACKSIZE=128k perf stat -d -o benchmark.txt ./$(TARGET)

# Usage: make bench NAME=phase2
# Runs a 60s idle benchmark under perf stat, saves runs/NAME.perf + runs/NAME.stats,
# prints instructions/frame computed from both files.
NAME ?= baseline

bench: $(TARGET)
	mkdir -p runs
	OMP_WAIT_POLICY=PASSIVE OMP_NUM_THREADS=4 OMP_STACKSIZE=128k perf stat -d -o runs/$(NAME).perf -- ./bin/game --bench 60
	@-mv stats.txt runs/$(NAME).stats
	@awk 'NR==FNR{if($$0 ~ /^frames=/){split($$0,a,"="); frames=a[2]} if($$0 ~ /^avgfps=/){split($$0,b,"="); fps=b[2]} next} /instructions:u/{gsub(/,/,""); instr=$$1} END{if(frames>0 && instr>0) printf "instructions/frame = %.2f M   (avg fps %.1f)\n", instr/frames/1000000, fps; else print "WARNING: missing stats or perf data"}' runs/$(NAME).stats runs/$(NAME).perf

deploy: $(TARGET)
ifndef DEVICE_IP
	$(error DEVICE_IP is not set. Please copy .env.example to .env and set your device IP)
endif
	ssh root@$(DEVICE_IP) "mkdir -p /userdata/roms/ports/wolf_game/assets"
	scp $(TARGET) root@$(DEVICE_IP):/userdata/roms/ports/wolf_game/game
	scp -r assets/* root@$(DEVICE_IP):/userdata/roms/ports/wolf_game/assets/

.PHONY: all clean run deploy

