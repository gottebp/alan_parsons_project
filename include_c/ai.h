#ifndef _AI_H_
#define _AI_H_

/*============================================================================
 * LEGACY AI MODULE
 *
 * This header provides backwards compatibility for legacy code.
 * The new architecture uses:
 *   - game/game.c: enemies_update() for AI
 *   - game/game.c: effect_explosion(), effect_nuke() for effects
 *============================================================================*/

/*============================================================================
 * LEGACY STUBS (no-ops, kept for compatibility)
 *============================================================================*/
void EnemyMove(void);
void ShipExplode(float x, float y, int large, int damage);
void DropNuke(float x, float y);

/*============================================================================
 * CONSTANTS (still used in some places)
 *============================================================================*/
extern float fltEnemyFiringRate[5];
extern float fltPSpeed1, fltPSpeed2, fltPSpeed3, fltPSpeed4, fltPSpeed5;

#endif /* _AI_H_ */
