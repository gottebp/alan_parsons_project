# Architecture Roadmap

## Current State

The codebase has two parallel architectures:

```
┌─────────────────────────────────────────────────────────────────┐
│                        CURRENT STATE                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────┐         ┌──────────────┐                      │
│  │  main_pure.c │         │   main.c     │                      │
│  │  (new arch)  │         │  (legacy)    │                      │
│  └──────┬───────┘         └──────┬───────┘                      │
│         │                        │                              │
│         ▼                        ▼                              │
│  ┌──────────────┐         ┌──────────────┐                      │
│  │ platform/    │         │  bridge.c    │                      │
│  │   app.c      │         │ (adapters)   │                      │
│  └──────┬───────┘         └──────┬───────┘                      │
│         │                        │                              │
│         └────────────┬───────────┘                              │
│                      ▼                                          │
│              ┌──────────────┐                                   │
│              │   Game       │  ← New unified state              │
│              │   struct     │                                   │
│              └──────┬───────┘                                   │
│                     │                                           │
│         ┌──────────┬┴──────────┐                                │
│         ▼          ▼           ▼                                │
│    ┌─────────┐ ┌─────────┐ ┌─────────┐                          │
│    │player.c │ │enemy.c  │ │  ppe.c  │  ← Legacy modules        │
│    │(globals)│ │(globals)│ │(globals)│    still using globals   │
│    └─────────┘ └─────────┘ └─────────┘                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### What Works
- `Game` struct centralizes player, enemies, particles, waves
- Pool-based entity management with type-safe iteration
- Vec2 math for toroidal coordinates
- Audio event flags (decoupled from game logic)
- State machine for game flow
- 118 tests passing

### What's Messy
- Two main entry points (`main.c`, `main_pure.c`)
- Legacy modules still access globals (`ScreenOff`, `MapOff`, etc.)
- Bridge/adapter layers for compatibility
- Dead code from incremental refactoring
- Mixed rendering (software buffer + hardware particles)

---

## Target Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                       TARGET STATE                              │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                      ┌──────────────┐                           │
│                      │    main.c    │                           │
│                      │  (unified)   │                           │
│                      └──────┬───────┘                           │
│                             │                                   │
│                             ▼                                   │
│                      ┌──────────────┐                           │
│                      │     App      │  ← Platform concerns      │
│                      │   struct     │    (window, audio ctx,    │
│                      │              │     input polling)        │
│                      └──────┬───────┘                           │
│                             │                                   │
│              ┌──────────────┼──────────────┐                    │
│              ▼              ▼              ▼                    │
│       ┌──────────┐   ┌──────────┐   ┌──────────┐                │
│       │ Platform │   │   Game   │   │  Render  │                │
│       │  Layer   │   │  Logic   │   │  Layer   │                │
│       │          │   │          │   │          │                │
│       │ - SDL    │   │ - Player │   │ - Map    │                │
│       │ - Audio  │   │ - Enemies│   │ - Sprites│                │
│       │ - Input  │   │ - Weapons│   │ - Particles│              │
│       │ - Timer  │   │ - Waves  │   │ - UI     │                │
│       └──────────┘   │ - AI     │   └──────────┘                │
│                      └──────────┘                               │
│                                                                 │
│       No globals. All state passed explicitly.                  │
│       Game logic is platform-independent.                       │
│       Render reads Game state, writes to screen.                │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### Design Principles

1. **Zero Globals**: All state lives in `App` or `Game` structs
2. **Explicit Dependencies**: Functions receive what they need as parameters
3. **Platform Isolation**: Game logic doesn't know about SDL
4. **Testability**: Game can run headless for automated testing
5. **Single Source of Truth**: One `Game` struct, one `main.c`

---

## Module Breakdown

### Core (`include_c/core/`)
Pure C, no dependencies. Foundation types.

| File | Status | Notes |
|------|--------|-------|
| `types.h` | Done | `u8`, `u16`, `u32`, `i8`, etc. |
| `constants.h` | Done | Screen size, map size, limits |
| `pool.h` | Done | Generic pool with iteration macros |

### Math (`include_c/math/`)
Pure math, no side effects.

| File | Status | Notes |
|------|--------|-------|
| `vec2.h` | Done | Toroidal math, angle conversion |
| `fixed.h` | Future | Fixed-point for determinism (optional) |

### Game (`include_c/game/`, `source_c/game/`)
Game logic. No SDL, no rendering, no audio playback.

| File | Status | Notes |
|------|--------|-------|
| `game.h/c` | Done | Game struct, update loop |
| `weapons.h/c` | Done | Weapon definitions |
| `render.h/c` | 80% | Reads Game, writes to buffer (has some globals) |
| `menu_new.h/c` | Done | Menu state machine |
| `sprites.h/c` | Done | Sprite loading |
| `ai_player.h/c` | Dead | Not integrated (could be useful for testing) |
| `audio.h/c` | Dead | Replaced by audio_bridge |
| `bridge.h/c` | Legacy | Adapter layer, remove when globals gone |

### Platform (`include_c/platform/`, `source_c/platform/`)
SDL and system calls. The only place SDL headers are included.

| File | Status | Notes |
|------|--------|-------|
| `app.h/c` | Done | App lifecycle, frame timing |
| `sdl_platform.c` | Partial | SDL init, window, audio context |
| `render.c` | Dead | Was going to be clean render, unused |
| `platform.h` | Stub | Platform abstraction interface |

### Legacy (`source_c/`)
Original modules, being phased out.

| File | Status | Notes |
|------|--------|-------|
| `player.c` | Stub | Minimal, logic moved to `game.c` |
| `enemy.c` | Stub | Minimal, logic moved to `game.c` |
| `ai.c` | Stub | Minimal, logic moved to `game.c` |
| `ppe.c` | Stub | Particle engine, logic in `game.c` |
| `mapeng.c` | Active | Map loading, needs globals |
| `sdl_wrapper.c` | Active | Core SDL functions, many globals |
| `menu.c` | Active | Legacy menu (parallel to menu_new) |

---

## Migration Path

### Phase 1: Consolidate Entry Points
**Goal**: Single `main.c` using new architecture

1. Merge best of `main.c` and `main_pure.c` into one
2. Remove `bridge.c` adapter layer
3. Delete `main_new.c` (reference file, no longer needed)
4. Delete `demo_new_arch.c` (served its purpose)

**Result**: One entry point, cleaner build

### Phase 2: Eliminate Rendering Globals
**Goal**: Render functions take explicit buffers

Current globals to eliminate:
- `ScreenOff` (screen buffer pointer)
- `ScreenTemp` (temp buffer for fades)
- `MapOff` (map pixel data)
- `SpriteOff[]` (sprite data)

Steps:
1. Add `RenderContext` to `App` struct:
   ```c
   typedef struct {
       uint32_t* screen;      // Was ScreenOff
       uint32_t* temp;        // Was ScreenTemp
       uint32_t* map;         // Was MapOff
       Sprite sprites[MAX_SPRITES];
       SDL_Texture* texture;
   } RenderContext;
   ```
2. Pass `RenderContext*` to all render functions
3. Update `game/render.c` to use context instead of globals
4. Remove globals from `sdl_wrapper.c`

### Phase 3: Eliminate Input Globals
**Goal**: Input flows through `InputState` struct only

Current:
- `KeyPressed[]` array (global)
- Various key state globals

Steps:
1. `App` already has `InputState input` - use it everywhere
2. Remove `KeyPressed[]` global
3. Update `input.c` to write to passed `InputState*`

### Phase 4: Eliminate Audio Globals
**Goal**: Audio context passed explicitly

Current:
- `Mix_Chunk*` globals for each sound
- `Mix_Music*` for background music

Steps:
1. Add `AudioContext` to `App`:
   ```c
   typedef struct {
       Mix_Chunk* sounds[SOUND_COUNT];
       Mix_Music* music[MUSIC_COUNT];
       int music_volume;
       int sfx_volume;
   } AudioContext;
   ```
2. Integrate `game/audio.c` (currently dead) or evolve `audio_bridge.c`
3. Pass `AudioContext*` to audio functions
4. Remove globals from `sdl_wrapper.c`

### Phase 5: Clean Up Map Loading
**Goal**: Map data in `Game` or `RenderContext`

Steps:
1. Move map loading to use explicit buffers
2. Remove `MapOff` global
3. Map collision data already in `Game` (waves, spawn points)

### Phase 6: Delete Dead Code
**Goal**: Remove unused files

Files to delete:
- `source_c/main_new.c` (aspirational reference, superseded)
- `source_c/demo_new_arch.c` (headless demo, tests cover this)
- `source_c/game/audio.c` (never integrated)
- `source_c/game/ai_player.c` (never integrated)
- `source_c/platform/render.c` (never integrated)
- `source_c/game/bridge.c` (after Phase 1)
- `include_c/game/bridge.h` (after Phase 1)
- `include_c/game/bridge_opaque.h` (after Phase 1)

### Phase 7: Unify Menus
**Goal**: Single menu implementation

Current:
- `menu.c` (legacy, blocking)
- `game/menu_new.c` (new, non-blocking state machine)

Steps:
1. Ensure `menu_new.c` handles all cases
2. Remove `menu.c`
3. Rename `menu_new.c` to `menu.c`

### Phase 8: Final Polish
**Goal**: Clean module boundaries

1. Audit all `#include` statements
2. Game modules should never include `<SDL.h>`
3. Platform modules should never include game logic
4. Add `const` correctness throughout
5. Document public APIs in headers

---

## File Structure (Target)

```
include_c/
├── core/
│   ├── types.h          # Primitive types
│   ├── constants.h      # Game constants
│   └── pool.h           # Entity pool
├── math/
│   └── vec2.h           # Vector math
├── game/
│   ├── game.h           # Game struct and API
│   ├── weapons.h        # Weapon definitions
│   ├── menu.h           # Menu state machine
│   ├── sprites.h        # Sprite data
│   └── render.h         # Game rendering interface
└── platform/
    ├── app.h            # Application lifecycle
    ├── audio.h          # Audio context
    └── render.h         # Platform render context

source_c/
├── main.c               # Entry point (unified)
├── game/
│   ├── game.c           # Game logic
│   ├── weapons.c        # Weapon behavior
│   ├── menu.c           # Menu logic
│   ├── sprites.c        # Sprite loading
│   └── render.c         # Software rendering
└── platform/
    ├── app.c            # App lifecycle
    ├── audio.c          # SDL_mixer wrapper
    ├── render.c         # SDL rendering
    └── input.c          # SDL input
```

---

## Metrics

Track progress by counting globals:

| Module | Current Globals | Target |
|--------|-----------------|--------|
| sdl_wrapper.c | ~15 | 0 |
| mapeng.c | ~5 | 0 |
| menu.c | ~3 | 0 |
| input.c | ~2 | 0 |
| rand.c | 1 | 0 (move to Game) |
| **Total** | **~26** | **0** |

---

## Testing Strategy

Each phase should:
1. Keep all 118 tests passing
2. Add tests for newly exposed APIs
3. Run WASM build to verify browser compatibility

Headless testing remains valuable - `Game` can be updated without SDL.

---

## Notes

- Don't rush. Each phase should result in a working game.
- Commit after each phase.
- The legacy path (`make legacy`) can be kept for comparison until Phase 6.
- WASM build may need special handling for some globals (Emscripten quirks).
