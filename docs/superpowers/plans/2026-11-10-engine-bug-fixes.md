# Engine Bug Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix correctness, safety, and lifetime issues in the Doom-style raycasting engine (`engine/`) without changing demo game behavior except where required for engine API safety.

**Architecture:** Make engine hot paths and loaders defensive by default (initialized geometry, bounds-checked map access, validated assets), clarify ownership/lifetime contracts for engine resources, and remove shared mutable renderer state. Keep existing rendering math and data flow intact; prefer small API additions over broad rewrites.

**Tech Stack:** C++17, SDL2, OpenMP, Makefile (`Makefile`), CPU software renderer in `engine/src/*`.

**Spec:** Derived from engine-only deep review in this session (2026-11-10): horizon-row UB, unchecked map/DDA access, input snapshot ordering, asset/texture ownership and loader hazards, shared Rasterizer sort buffers, engine lifecycle gaps, particle bounds/full-buffer behavior.

## Global Constraints

- Scope: engine only (`engine/src/*`, `engine/shlong.h` as needed). Do not modify `demo/*` or `assets/*` unless a compile break from an engine signature change requires the minimal adapter.
- Preserve existing public behavior and performance characteristics where safe (DDA raycasting, transposed framebuffer, fixed-point texture steps, OpenMP loops).
- No new third-party dependencies or test frameworks.
- Follow existing style: C++17, manual resource management, project headers, `#pragma omp` usage, `SDL2` APIs.
- Every task must leave the project buildable with `make` (PC target). If a verification command cannot run in this environment, record exact expected result and the blocker.

## Review Focus

- `RowGeometry` horizon row (`p == 0`): `distance/startFloor*/step*` must be deterministic and never read uninitialized by `Rasterizer::DrawTexturedHorizon`.
- Out-of-bounds world/map coordinates: `Map` getters and ray traversal must not perform OOB reads/writes for negative, oversized, or NaN/inf-cast inputs; define and enforce empty-tile/outside-map behavior.
- Input edge detection across poll/update ordering: `isKeyPressed`/`isGamepadPressed` must report transitions once per logical frame, not depend on stale snapshot timing.
- Asset loading failures/malformed metadata: zero/negative/misaligned frame sizes, oversized sprite data, and texture load/format failures must not corrupt memory; failure paths must be explicit.
- Resource ownership/lifetime: `Texture`, `SpriteFrame`, `SpriteSheet`, and sort buffers must have clear non-aliasing ownership; avoid double-free and cross-instance shared mutable state.

---

### Task 1: Initialize Horizon Row Geometry

**Files:**
- Modify: `engine/src/Raycaster.cpp:148-181`
- Modify: `engine/src/Raycaster.h` (only if helper/signature comment needed)

**Interfaces:**
- Consumes: `Camera::pitch`, `Camera::z`, `Camera::maxViewDist`, screen dimensions.
- Produces: fully initialized `rowBuffer[y]` for all `y in [0, scrHeight)`; `p == 0` row has safe `distance` (>= max fog/no-draw sentinel) and zeroed fixed-point steps/start values.

- [ ] **Step 1: Write the failing invariant check (source-level)**

  Add/adjust code path so horizon row is explicitly assigned:
  ```cpp
  int p = y - horizon;
  if (p == 0) {
      rowBuffer[y].startFloorX = 0;
      rowBuffer[y].startFloorY = 0;
      rowBuffer[y].stepX = 0;
      rowBuffer[y].stepY = 0;
      rowBuffer[y].distance = camera->maxViewDist + 1.0f;
      continue;
  }
  ```
  Keep existing `if (rowDistance >= camera->maxViewDist)` behavior for other rows.

- [ ] **Step 2: Verify horizon branch leaves no uninitialized fields**

  Run: `rg -n "p == 0|rowBuffer\\[y\\]" engine/src/Raycaster.cpp`
  Expected: `p == 0` branch assigns all five `RowGeometry` fields before `continue`.

- [ ] **Step 3: Build**

  Run: `make`
  Expected: compiles/links successfully (or same pre-existing unrelated failures documented).

- [ ] **Step 4: Commit**

  ```bash
  git add engine/src/Raycaster.cpp
  git commit -m "fix(engine): initialize horizon row geometry"
  ```

### Task 2: Make Map Access Bounds-Safe

**Files:**
- Modify: `engine/src/Map.h:14-38`
- Modify: `engine/src/Map.cpp:11-14`

**Interfaces:**
- Consumes: tile coordinates/world positions from raycasting, collision, gameplay adapters.
- Produces: safe query API that never indexes outside `mapData`; define out-of-bounds as solid/non-walkable tile (return non-zero, e.g. `1`) for getters, and no-op for `SetTile` OOB.

- [ ] **Step 1: Add explicit bounds checks**

  Implement/inline helpers with clear semantics:
  ```cpp
  inline bool InBounds(int x, int y) const {
      return (unsigned)x < (unsigned)mapSize && (unsigned)y < (unsigned)mapSize;
  }
  inline unsigned char GetTile(int x, int y) const {
      if (!InBounds(x, y)) return 1;
      return mapData[(y << mapShift) + x];
  }
  inline unsigned char GetWorldTile(float x, float y) const {
      int gx = (int)std::floor(x);
      int gy = (int)std::floor(y);
      return GetTile(gx, gy);
  }
  ```
  Update `SetTile` to reject via `InBounds`.

- [ ] **Step 2: Verify no unchecked direct indexing remains in Map API**

  Run: `rg -n "mapData\\[\\(y << mapShift\\) \\+ x\\]" engine/src/Map.h engine/src/Map.cpp`
  Expected: only inside guarded path(s), never after unchecked coordinates.

- [ ] **Step 3: Build**

  Run: `make`
  Expected: PASS.

- [ ] **Step 4: Commit**

  ```bash
  git add engine/src/Map.h engine/src/Map.cpp
  git commit -m "fix(engine): bounds-check map tile access"
  ```

### Task 3: Guard DDA Traversal Against OOB Map Indices

**Files:**
- Modify: `engine/src/Raycaster.cpp:18-147`

**Interfaces:**
- Consumes: `Map::GetWidth/GetHeight/GetRawData/GetMapShift`, camera/ray state.
- Produces: DDA loop that cannot read outside map buffer even if ray marches unexpectedly; terminates with far-wall sentinel when leaving bounds.

- [ ] **Step 1: Add traversal bounds guard**

  Track map bounds (`w`, `h` or `mapSize`) and guard each stepped cell:
  ```cpp
  if ((unsigned)mapX >= (unsigned)mapW || (unsigned)mapY >= (unsigned)mapH) {
      tile = 1; // treat outside as wall / stop
      break;
  }
  // compute index/read tile
  ```
  Ensure loop still terminates when `sideDist` exceeds `camera->maxViewDist` (existing behavior).

- [ ] **Step 2: Verify every mapData access is preceded by bounds validation**

  Run: `rg -n "mapData\\[mapIndex\\]|mapX|mapY" engine/src/Raycaster.cpp`
  Expected: `mapData[mapIndex]` reachable only after in-bounds condition for current `mapX/mapY`.

- [ ] **Step 3: Build**

  Run: `make`
  Expected: PASS.

- [ ] **Step 4: Commit**

  ```bash
  git add engine/src/Raycaster.cpp
  git commit -m "fix(engine): guard DDA map traversal bounds"
  ```

### Task 4: Fix Input Snapshot Ordering

**Files:**
- Modify: `engine/src/Input.h`
- Modify: `engine/src/Input.cpp`
- Modify: `engine/src/Engine.cpp` (only if call order must change)

**Interfaces:**
- Consumes: SDL keyboard/gamecontroller state.
- Produces: per-frame `previous*` representing prior frame state and current state representing this frame after event pump/read; `isKeyPressed`/`isGamepadPressed` detect rising edge once.

- [ ] **Step 1: Implement correct frame sequencing**

  Preferred contract:
  - `Input::BeginFrame()` (or equivalent): copy current snapshots into `previousKeyState/prevGamepadState`.
  - Event pump / `SDL_PollEvent` occurs.
  - `Input::Capture()` (or equivalent): sample current SDL keyboard/controller state.
  Preserve existing public methods if possible; if API changes, update all engine call sites. Do not query live SDL button state inside `is*Pressed` comparisons except against captured current snapshot.

- [ ] **Step 2: Verify previous/current separation**

  Run: `rg -n "previousKeyState|prevGamepadState|SDL_GetKeyboardState|SDL_GameControllerGetButton|SDL_PollEvent" engine/src/Input.cpp engine/src/Engine.cpp`
  Expected: previous-state copy uses prior captured state; current capture happens after pump; pressed checks compare captured current vs previous arrays.

- [ ] **Step 3: Build**

  Run: `make`
  Expected: PASS.

- [ ] **Step 4: Commit**

  ```bash
  git add engine/src/Input.h engine/src/Input.cpp engine/src/Engine.cpp
  git commit -m "fix(engine): correct input frame snapshot ordering"
  ```

### Task 5: Harden Engine Lifecycle and Timing

**Files:**
- Modify: `engine/src/Engine.h`
- Modify: `engine/src/Engine.cpp`

**Interfaces:**
- Consumes: SDL init/quit, frame timing config.
- Produces: fully initialized timing members, idempotent/safe shutdown, bounded `deltaTime`.

- [ ] **Step 1: Initialize and clamp runtime state**

  - Initialize `performanceFreq`, `deltaTime`, counters/timers, and `isRunning` consistently in ctor and/or `Initialize`.
  - After `deltaTime` compute, clamp to sane max (e.g. `<= 0.25f`) to avoid spiral on stalls/debug breaks.
  - Make `Quit()` safe to call repeatedly without double-destroy assumptions; keep `SDL_Quit()` pairing coherent.

- [ ] **Step 2: Verify lifecycle guards**

  Run: `rg -n "performanceFreq|deltaTime|isRunning|SDL_Quit|Quit\\(" engine/src/Engine.h engine/src/Engine.cpp`
  Expected: no uninitialized timing reads on valid flow; quit path guarded/idempotent.

- [ ] **Step 3: Build**

  Run: `make`
  Expected: PASS.

- [ ] **Step 4: Commit**

  ```bash
  git add engine/src/Engine.h engine/src/Engine.cpp
  git commit -m "fix(engine): harden lifecycle and frame timing"
  ```

### Task 6: Define Resource Ownership and Harden Asset Loaders

**Files:**
- Modify: `engine/src/AssetMgr.h`
- Modify: `engine/src/AssetMgr.cpp`
- Modify: `engine/src/Sprite.h`
- Modify: `engine/src/Rasterizer.cpp` (only if texture orientation contract changes)

**Interfaces:**
- Consumes: BMP files/map files, frame metadata (`frameWidth`, `frameHeight`, bg color), tile IDs.
- Produces: explicit ownership rules and validated loads:
  - `Texture`, `SpriteSheet`, `SpriteFrame`: non-copyable, movable or explicitly deleted moves per current codebase needs (minimum: delete copy ctor/assign).
  - `LoadTexture`: validate surface/format/dimensions/power-of-two expectations used by `shift/mask`; fail cleanly on invalid input.
  - `LoadSpriteSheet`: validate positive dimensions, `sheetWidth % frameWidth == 0`, `sheetHeight % frameHeight == 0`, frame count > 0, allocation sizes, and pixel-write bounds against `solidPixelData` capacity.
  - Texture orientation: either preserve current transposed storage with explicit named contract/comment/helper, or perform real transpose on load and update consumers consistently (choose one and apply end-to-end).

- [ ] **Step 1: Lock ownership semantics**

  Add:
  ```cpp
  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
  // similar for SpriteSheet/SpriteFrame or carefully implemented move-only
  ```
  Ensure destructors remain sole owners.

- [ ] **Step 2: Add loader validation and failure cleanup**

  - Return `nullptr` on invalid metadata/alloc failures.
  - Free SDL surfaces/temporary buffers on every failure path.
  - Avoid writing past allocated sprite pixel buffers.

- [ ] **Step 3: Resolve texture orientation contract**

  Run: `rg -n "width = converted->h|height = converted->w|texX << tex->shift|pixels" engine/src/AssetMgr.cpp engine/src/Rasterizer.cpp`
  Expected: single documented contract; loader and wall sampler agree. If storage changes, update `DrawWalls` slice addressing accordingly.

- [ ] **Step 4: Build**

  Run: `make`
  Expected: PASS.

- [ ] **Step 5: Commit**

  ```bash
  git add engine/src/AssetMgr.h engine/src/AssetMgr.cpp engine/src/Sprite.h engine/src/Rasterizer.cpp
  git commit -m "fix(engine): harden asset ownership and loading"
  ```

### Task 7: Remove Shared Mutable Rasterizer Sort State

**Files:**
- Modify: `engine/src/Rasterizer.h`
- Modify: `engine/src/Rasterizer.cpp:18-21,501-575`

**Interfaces:**
- Consumes: render queue and camera.
- Produces: per-instance (or otherwise non-shared) sprite sort buffers; `SortDrawList/DrawSprites` safe for independent `Rasterizer` instances.

- [ ] **Step 1: Move sort buffers into instance scope**

  Replace file-static `drawList/tempSwapList` with members sized to `MAX_VISIBLE_SPRITES` (or allocated in `Initialize`), and make `SortDrawList` use them. Keep radix-sort behavior/order unchanged.

- [ ] **Step 2: Verify no file-static mutable sprite sort buffers remain**

  Run: `rg -n "static SpriteDrawInfo drawList|static SpriteDrawInfo tempSwapList" engine/src/Rasterizer.cpp`
  Expected: no matches.

- [ ] **Step 3: Build**

  Run: `make`
  Expected: PASS.

- [ ] **Step 4: Commit**

  ```bash
  git add engine/src/Rasterizer.h engine/src/Rasterizer.cpp
  git commit -m "fix(engine): isolate rasterizer sprite sort buffers"
  ```

### Task 8: Tighten Particle Bounds and Full-Buffer Behavior

**Files:**
- Modify: `engine/src/Particle.h` (if needed)
- Modify: `engine/src/Particle.cpp:13-102`

**Interfaces:**
- Consumes: emitted particles and camera/ray depth buffer.
- Produces: deterministic behavior at capacity and safe 2x2 pixel writes within framebuffer bounds.

- [ ] **Step 1: Define capacity behavior and render bounds**

  - Keep silent drop or add explicit documented policy (e.g. overwrite-oldest/ring) — choose one simple deterministic policy.
  - In `Render`, guard each plotted pixel individually:
  ```cpp
  if (screenX >= 0 && screenX < scrWidth && screenY >= 0 && screenY < scrHeight) ...
  ```
  Apply to `(x,y),(x+1,y),(x,y+1),(x+1,y+1)` with depth check per screen column as appropriate.

- [ ] **Step 2: Verify edge checks cover all 4 pixels**

  Run: `rg -n "DrawPixel\\(screenX \\+ 1|DrawPixel\\(screenX, screenY \\+ 1" engine/src/Particle.cpp`
  Expected: adjacent-pixel writes cannot occur without bounds validation.

- [ ] **Step 3: Build**

  Run: `make`
  Expected: PASS.

- [ ] **Step 4: Commit**

  ```bash
  git add engine/src/Particle.h engine/src/Particle.cpp
  git commit -m "fix(engine): bound particle rendering and capacity behavior"
  ```

### Task 9: Final Verification Pass

**Files:**
- Verify: `engine/src/*`, `Makefile` (read-only)

**Interfaces:**
- Consumes: completed engine fixes.
- Produces: clean build and recorded verification notes for environment-specific checks.

- [ ] **Step 1: Full rebuild**

  Run: `make clean && make`
  Expected: all engine and demo translation units compile and link (demo compiled only because Makefile includes it; no demo source edits expected).

- [ ] **Step 2: Static sanity checks**

  Run:
  - `rg -n "TODO|FIXME|HACK" engine/src` (informational)
  - `rg -n "static SpriteDrawInfo drawList|static SpriteDrawInfo tempSwapList" engine/src/Rasterizer.cpp` (expect no matches)
  - `rg -n "if \\(p == 0\\)" engine/src/Raycaster.cpp` (expect initialized horizon row)
  Expected: targeted invariants hold.

- [ ] **Step 3: Runtime smoke (if feasible)**

  Run: `./bin/game` or `make run` (note: `make run` requires `perf`)
  Expected: window launches/runs without immediate crash in this environment; if not runnable headless/no SDL/perf, document blocker.

- [ ] **Step 4: Commit plan/final fixups if any**

  ```bash
  git add -A
  git commit -m "chore(engine): finalize bug fix verification"
  ```
  (Only if there are remaining tracked fixup changes; otherwise skip.)

## Self-Review Notes

- Spec coverage: horizon UB (Task 1), Map bounds (Task 2), DDA bounds (Task 3), input ordering (Task 4), lifecycle/timing (Task 5), ownership+loaders+texture contract (Task 6), shared sort buffers (Task 7), particle bounds/capacity (Task 8), verification (Task 9).
- No automated C++ unit test framework exists in repo; each task uses code invariants + build verification. If a harness is added later, convert invariant steps into tests first (TDD) before behavior changes.
- Coordinate math uses float→int casts in existing codebase; bounds checks must handle negative casts via unsigned comparison/flooring as specified.
- API changes should be minimized; if `Input` public sequencing changes, update engine call sites in same task.
- Demo untouched unless compile break from engine API change forces minimal adapter.