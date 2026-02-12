# Architecture Roadmap

## Current State

Clean architecture with single entry point and minimal globals:

```
┌─────────────────────────────────────────────────────────────────┐
│                        CURRENT STATE                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│                      ┌──────────────┐                           │
│                      │   main.c     │  ← Single entry point     │
│                      │              │    (native + WASM)        │
│                      └──────┬───────┘                           │
│                             │                                   │
│                             ▼                                   │
│                      ┌──────────────┐                           │
│                      │     App      │  ← Platform concerns      │
│                      │   struct     │    (window, audio,        │
│                      │              │     input, render ctx)    │
│                      └──────┬───────┘                           │
│                             │                                   │
│              ┌──────────────┼──────────────┐                    │
│              ▼              ▼              ▼                    │
│       ┌──────────┐   ┌──────────┐   ┌──────────┐                │
│       │ Platform │   │   Game   │   │  Render  │                │
│       │  Layer   │   │  Logic   │   │  Layer   │                │
│       │          │   │          │   │          │                │
│       │ - app.c  │   │ - game.c │   │ -render.c│                │
│       │ - sdl_*  │   │ - menu.c │   │ - uses   │                │
│       │ - input  │   │ - weapons│   │   context│                │
│       └──────────┘   └──────────┘   └──────────┘                │
│                                                                 │
│       Render context passed explicitly.                         │
│       Audio context passed explicitly.                          │
│       Game logic is platform-independent.                       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### What Works
- Single `main.c` for native and WASM builds
- `App` struct owns all platform resources (render, audio, visuals)
- `Game` struct centralizes player, enemies, particles, waves
- Pool-based entity management with type-safe iteration
- Vec2 math for toroidal coordinates
- Audio event flags via `audio_bridge_set_context()`
- Render context via `render_set_context()` and `menu_set_render_context()`
- State machine for game flow
- 137 tests passing
- WASM restart support

### Remaining Globals (Low Priority)
- Input globals (KEYBOARD, MOUSE_*) - tied to Emscripten JS callbacks
- Some screen buffers in sdl_wrapper.c (fallback for legacy fade functions)

---

## Migration Complete

### Phase 1: Consolidate Entry Points ✅ COMPLETE
- `main.c` used for both native and WASM builds
- Deleted legacy files: `main_new.c`, `demo_new_arch.c`

### Phase 2: Eliminate Rendering Globals ✅ COMPLETE
- Added `RenderContext` to `App` struct
- All render functions use context with fallback
- `render_set_context()` initializes render module
- `menu_set_render_context()` initializes menu module

### Phase 3: Input Globals ⏳ DEFERRED
- Input tied to Emscripten JS callbacks
- Works as-is, can be improved later

### Phase 4: Eliminate Audio Globals ✅ COMPLETE
- `AudioAssets` struct in App
- `audio_bridge_set_context()` passes audio resources
- Audio bridge uses context pattern

### Phase 5: Visual Assets ✅ COMPLETE
- `VisualAssets` struct in App
- Assets loaded into App struct

### Phase 6: Delete Legacy Code ✅ COMPLETE
- Deleted `source_c/main.c` (old entry point) - 1031 lines
- Deleted `source_c/game/bridge.c` - 541 lines
- Deleted `include_c/game/bridge.h` - 96 lines
- Deleted `include_c/game/bridge_opaque.h` - 121 lines
- Deleted `source_c/menu.c` (legacy menu) - 323 lines
- **Total removed: 2112 lines**

### Phase 7: Unify Menus ✅ COMPLETE
- Renamed `menu_new.c` to `menu.c`
- Renamed `main_pure.c` to `main.c`
- Single menu implementation

### Phase 8: Final Polish ⏳ OPTIONAL
- Could remove SDL includes from game/render.c
- Could add more const correctness
- Could clean up remaining stub files

---

## File Structure (Current)

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
│   ├── render.h         # Game rendering interface
│   ├── sprites.h        # Sprite data
│   └── audio_bridge.h   # Audio event interface
└── platform/
    ├── app.h            # Application lifecycle
    └── platform.h       # Platform abstraction

source_c/
├── main.c               # Entry point (unified)
├── game/
│   ├── game.c           # Game logic
│   ├── weapons.c        # Weapon behavior
│   ├── menu.c           # Menu logic (non-blocking)
│   ├── render.c         # Software rendering
│   ├── sprites.c        # Sprite loading
│   └── audio_bridge.c   # Audio event handling
├── platform/
│   ├── app.c            # App lifecycle
│   └── sdl_platform.c   # SDL platform abstraction
└── (legacy stubs)       # player.c, enemy.c, ai.c, etc.
```

---

## Metrics

| Metric | Before | After |
|--------|--------|-------|
| Entry points | 2 | 1 |
| Legacy files | 8 | 0 |
| Lines of legacy code | 2112 | 0 |
| Test count | 123 | 137 |
| Render globals used | 15+ | 4 (fallback) |
| Audio globals used | 8 | 3 (fallback) |

---

## Testing

All tests pass:
- `make -f Makefile.c` builds without warnings
- `./alan_parsons` runs correctly
- `cd tests && make test` - 137 tests pass
