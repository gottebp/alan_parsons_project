/*
 * AI system - Legacy stubs
 *
 * The new architecture uses:
 *   - game/game.c: enemies_update() for enemy AI
 *   - game/game.c: effect_explosion(), effect_nuke() for effects
 *
 * KEPT FOR COMPATIBILITY:
 *   - EnemyMove(), ShipExplode(), DropNuke() - stubs that do nothing
 *   - fltEnemyFiringRate[] - still referenced in some places
 */

#include "ai.h"
#include "defs.h"

/* Enemy firing rate constants - kept for any remaining references */
float fltEnemyFiringRate[5] = {12.6f, 14.5331f, 16.31f, 18.2f, 7.3112f};

/* Particle speed constants - kept for any remaining references */
float fltPSpeed1 = 4.2f;
float fltPSpeed2 = 5.33f;
float fltPSpeed3 = 6.112f;
float fltPSpeed4 = 7.5f;
float fltPSpeed5 = 8.82f;

/*
 * AI movement for enemies - STUB
 * Use enemies_update() from game/game.c instead
 */
void EnemyMove(void) {
    /* No-op - enemy AI now in game/game.c */
}

/*
 * Ship explosion effect - STUB
 * Use effect_explosion() from game/game.c instead
 */
void ShipExplode(float x, float y, int large, int damage) {
    (void)x; (void)y; (void)large; (void)damage;
    /* No-op - explosions now in game/game.c */
}

/*
 * Drop nuke shockwave - STUB
 * Use effect_nuke() from game/game.c instead
 */
void DropNuke(float x, float y) {
    (void)x; (void)y;
    /* No-op - nuke effect now in game/game.c */
}
