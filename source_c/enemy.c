/*
 * Enemy system - Legacy globals and initialization
 *
 * The new architecture uses:
 *   - game/game.c: enemies_update(), enemy_spawn()
 *   - game/render.c: render_enemies()
 *   - game/sprites.c: sprite loading
 *
 * KEPT FOR BRIDGE COMPATIBILITY:
 *   - Enemy array and related globals
 *   - LoadEnemyData()/DestroyEnemyData() - now no-ops
 */

#include "enemy.h"
#include <stdint.h>

/* Enemy data - needed by bridge for sync */
Enemy Enemies[100];

/* Spawn timing - needed by bridge for wave sync */
uint32_t SpawnFrames[18];
uint32_t FrameNum = 0;
uint32_t NextWaveNumber = 0;

/*
 * Load enemy image data
 * Sprite loading moved to sprites_init() in game/sprites.c
 */
void LoadEnemyData(void) {
    /* No-op - kept for compatibility */
}

/*
 * Destroy enemy data
 * Sprite cleanup moved to sprites_destroy() in game/sprites.c
 */
void DestroyEnemyData(void) {
    /* No-op - kept for compatibility */
}
