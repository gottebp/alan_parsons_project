/*
 * sprites.h - Sprite Loading and Management
 *
 * Consolidates all sprite loading from legacy player.c and enemy.c.
 * Sprites are loaded once at startup and used by render.c.
 */

#ifndef _SPRITES_H_
#define _SPRITES_H_

#include <stdint.h>
#include <stdlib.h>

/*============================================================================
 * PLAYER SPRITES
 * 256 rotation frames for each ship state (normal, banking left, banking right)
 *============================================================================*/
extern uint32_t* PlayerShipOff;   /* Normal orientation */
extern uint32_t* PlayerShipOffL;  /* Banking left */
extern uint32_t* PlayerShipOffR;  /* Banking right */

/*============================================================================
 * ENEMY SPRITES
 * Small enemies: 4 types x 16 rotation frames x 128x128 pixels
 * Boss enemies: 4 types x 16 rotation frames x 256x256 pixels
 *============================================================================*/
extern uint32_t* SmallEnemyImageData;
extern uint32_t* BossImageData;

/*============================================================================
 * PARTICLE TEXTURES (GPU)
 * Used by render.c for hardware-accelerated particle rendering.
 * Created from flare sprite sheets with alpha conversion.
 *============================================================================*/
struct SDL_Texture;  /* Forward declaration */
extern struct SDL_Texture* SmallParticleTexture;
extern struct SDL_Texture* LargeParticleTexture;

/*============================================================================
 * SPRITE LOADING FUNCTIONS
 *============================================================================*/

/*
 * Initialize all game sprites.
 * Call once at startup after graphics initialization.
 */
void sprites_init(void);

/*
 * Free all sprite memory.
 * Call at shutdown.
 */
void sprites_destroy(void);

/*============================================================================
 * RAW FILE LOADING UTILITY
 * Loads raw pixel data with magenta-to-transparent conversion.
 *============================================================================*/
void LoadRaw(const char* filename, void* buffer, size_t size);

#endif /* _SPRITES_H_ */
