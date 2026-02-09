/*
 * config.h - Build Configuration for The Alan Parsons Project
 *
 * This header documents the build flags and their effects on the codebase.
 * Include this to understand which code paths are active.
 *
 * BUILD CONFIGURATIONS:
 *
 * 1. LEGACY (default):
 *    make -f Makefile.c
 *    - Game logic uses legacy globals
 *    - Rendering uses legacy RenderPlayer(), RenderEnemies(), etc.
 *    - Audio triggered directly by legacy code
 *
 * 2. NEW ENGINE (USE_NEW_ENGINE):
 *    make -f Makefile.c new-engine
 *    - Game logic uses Game struct and game_update()
 *    - Bridge syncs Game state TO globals
 *    - Rendering still uses legacy functions (reads from globals)
 *    - Audio triggered by legacy code
 *
 * 3. NEW RENDER (USE_NEW_ENGINE + USE_NEW_RENDER):
 *    make -f Makefile.c new-render
 *    - Game logic uses Game struct and game_update()
 *    - Rendering uses render_bridge (reads FROM Game struct)
 *    - Audio uses audio_bridge (reads FROM Game struct)
 *    - Particles still sync via bridge (GPU rendering needs globals)
 *
 * DEPENDENCY GRAPH (new-render):
 *
 *   Input → Game struct → game_update()
 *                       ↓
 *                bridge_sync_to_globals() (for particles only)
 *                       ↓
 *           ┌──────────┴──────────┐
 *           ↓                     ↓
 *   render_bridge_frame()   audio_bridge_update()
 *           ↓                     ↓
 *      Display               SDL_mixer
 */

#ifndef GAME_CONFIG_H
#define GAME_CONFIG_H

/*============================================================================
 * BUILD FLAG DOCUMENTATION
 *============================================================================*/

/*
 * USE_NEW_ENGINE
 * - Enables Game struct for game logic
 * - game_update() processes all game logic
 * - Bridge syncs state to/from legacy globals
 * - Defined via: -DUSE_NEW_ENGINE
 */

/*
 * USE_NEW_RENDER
 * - Requires USE_NEW_ENGINE
 * - Enables render_bridge for all rendering
 * - Enables audio_bridge for sound
 * - Reads directly from Game struct for display
 * - Defined via: -DUSE_NEW_ENGINE -DUSE_NEW_RENDER
 */

/*============================================================================
 * ARCHITECTURE CONSTANTS
 *============================================================================*/

/* These document which systems are "new" vs "legacy" */

#ifdef USE_NEW_ENGINE
    #define GAME_LOGIC_NEW      1
    #define GAME_LOGIC_LEGACY   0
#else
    #define GAME_LOGIC_NEW      0
    #define GAME_LOGIC_LEGACY   1
#endif

#ifdef USE_NEW_RENDER
    #define RENDER_NEW          1
    #define RENDER_LEGACY       0
    #define AUDIO_NEW           1
    #define AUDIO_LEGACY        0
#else
    #define RENDER_NEW          0
    #define RENDER_LEGACY       1
    #define AUDIO_NEW           0
    #define AUDIO_LEGACY        1
#endif

/* Particles always use legacy rendering (GPU path needs globals) */
#define PARTICLES_LEGACY        1

#endif /* GAME_CONFIG_H */
