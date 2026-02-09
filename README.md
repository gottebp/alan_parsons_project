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
make
./alan_parsons_pure
```

### WASM Build (Browser)

```bash
# Requires Emscripten SDK
source /path/to/emsdk/emsdk_env.sh

# Build
make -f Makefile.emscripten

# Serve locally
make -f Makefile.emscripten serve
# Open http://localhost:8000/wasm_build/game.html
```

## Controls

| Key | Action |
|-----|--------|
| Arrow keys | Thrust / Turn |
| X | Fire weapon |
| Z / C | Strafe left / right |
| Space | Drop nuke |
| Escape | Menu |
| F | Toggle fullscreen |

## Command Line Options

```bash
./alan_parsons_pure --fullscreen      # Start in fullscreen
./alan_parsons_pure --captainplanet   # Invincibility mode
```

## Build Targets

| Command | Output | Description |
|---------|--------|-------------|
| `make` | `alan_parsons_pure` | New clean architecture (default) |
| `make legacy` | `alan_parsons_c` | Legacy architecture with bridge |
| `make test` | | Run test suite (118 tests) |
| `make -f Makefile.emscripten` | `wasm_build/game.html` | Browser build |

## Project Structure

```
include_c/
├── core/       # Types, constants, pool
├── math/       # Vec2 toroidal math
├── game/       # Game logic headers
└── platform/   # Platform abstraction

source_c/
├── main_pure.c # Entry point (new arch)
├── main.c      # Entry point (legacy)
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

## Links

- Original website: http://www.particlefield.com
