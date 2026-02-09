#ifndef _ENEMY_H_
#define _ENEMY_H_

#include <stdint.h>
#include "defs.h"

/*============================================================================
 * LEGACY ENEMY MODULE
 *
 * This header provides backwards compatibility for legacy code.
 * The new architecture uses:
 *   - game/game.c: enemies_update(), waves_init(), waves_update()
 *   - game/render.c: render_enemies()
 *   - game/sprites.c: sprite loading
 *============================================================================*/

/*============================================================================
 * ENEMY SPRITES (defined in game/sprites.c, used by render.c)
 *============================================================================*/
extern uint32_t* SmallEnemyImageData;
extern uint32_t* BossImageData;

/*============================================================================
 * ENEMY STATE (defined in enemy.c, used by bridge for sync)
 *============================================================================*/
extern Enemy Enemies[100];
extern uint32_t SpawnFrames[18];
extern uint32_t FrameNum;
extern uint32_t NextWaveNumber;

/*============================================================================
 * LIFECYCLE (no-ops, kept for compatibility)
 *============================================================================*/
void LoadEnemyData(void);
void DestroyEnemyData(void);

#endif /* _ENEMY_H_ */
