# The Alan Parsons Project

A particle-based space shooter game.

Originally written in x86 assembly (ECE291, UIUC, 2002), ported to C with SDL2 for native builds and Emscripten for browser deployment.

## Quick Start

### Native Build (macOS/Linux)

```bash
# Install dependencies
# macOS:
brew install sdl2 sdl2_mixer

# Ubuntu/Debian:
sudo apt-get install libsdl2-dev libsdl2-mixer-dev

# Build and run
make -f Makefile.c
./alan_parsons
```

### WASM Build (Browser)

```bash
# Requires Emscripten SDK
source /path/to/emsdk/emsdk_env.sh

# Build and serve
./build_wasm.sh
./host.sh
# Open http://localhost:8000/game.html
```

## Gameplay

- Six levels with increasing difficulty, each ending with a boss fight
- Weapons upgrade automatically as you progress through levels
- 6 nukes per level — screen-clearing shockwave
- **Body collisions**: flying into a small enemy destroys it but deals heavy damage to the player
- Bosses are immune to body collisions — you must shoot them down
- Damaged bosses billow smoke when low on health

## Controls

| Key | Action |
|-----|--------|
| Arrow keys | Thrust / Turn |
| X | Fire weapon |
| Z / C | Strafe left / right |
| Space | Drop nuke |
| Escape | Menu |
| F | Toggle fullscreen |

Mobile (WASM build): twin-stick controls — left stick for thrust/strafe, right stick to aim + auto-fire, NUKE button top-right.

## Command Line Options

```bash
./alan_parsons --fullscreen      # Start in fullscreen
./alan_parsons --captainplanet   # Invincibility mode
```

## Build Targets

| Command | Output | Description |
|---------|--------|-------------|
| `make -f Makefile.c` | `alan_parsons` | Build game |
| `make -f Makefile.c test` | | Run test suite |
| `./build_wasm.sh` | `wasm_build/game.html` | Browser build (clean + compress) |
| `./host.sh` | | Serve WASM build on localhost:8000 |
| `./clean.sh` | | Remove all build artifacts |

## Project Structure

```
include_c/
├── core/       # Types, constants, pool
├── math/       # Vec2 toroidal math
├── game/       # Game logic headers
└── platform/   # Platform abstraction

source_c/
├── main.c      # Entry point
├── game/       # Game logic implementation
└── platform/   # SDL/platform code

data/           # Map bitmaps, sprites
sound/          # Audio files (wav, ogg)
tests/          # Test suite
```

## Architecture

The game uses a clean architecture with explicit state passing:

- **Game struct**: All game state (player, enemies, particles, waves)
- **App struct**: Platform concerns (window, audio, input)
- **Pool-based entities**: Type-safe iteration macros
- **Audio event flags**: Decoupled sound triggering

See [ROADMAP.md](ROADMAP.md) for the migration plan toward zero globals.

## History

- **2002**: Original x86 assembly (ECE291 final project)
- **2002**: SDL port for Linux/Windows
- **2024**: C port with WASM support
- **2025**: Clean architecture refactoring
- **2026**: Body collisions, balance tuning, mobile controls

## Links

- Original website: http://www.particlefield.com
