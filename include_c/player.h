#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <stdint.h>
#include <stdlib.h>

/*============================================================================
 * LEGACY PLAYER MODULE
 *
 * This header provides backwards compatibility for legacy code.
 * The new architecture uses:
 *   - game/game.c: player_update(), player_fire_*()
 *   - game/render.c: render_player()
 *   - game/sprites.c: sprite loading
 *============================================================================*/

/*============================================================================
 * PLAYER SPRITES (defined in game/sprites.c, used by render.c)
 *============================================================================*/
extern uint32_t* PlayerShipOff;
extern uint32_t* PlayerShipOffL;
extern uint32_t* PlayerShipOffR;

/*============================================================================
 * PLAYER STATE (defined in player.c, used by bridge for sync)
 *============================================================================*/
extern int intPlayerX, intPlayerY;
extern float fltPlayerX, fltPlayerY;
extern float fltPlayerSpeed, fltPlayerStrafeSpeed;
extern int8_t intbPlayerAngle, intbPlayerTurnDir;
extern int intPlayerHealth;
extern int intPlayerWeaponsLevel;  /* Used by main.c for save/load */

/* Physics constants */
extern float fltPlayerAccel, fltPlayerFriction;
extern float fltPlayerMaxSpeed, fltPlayerMinSpeed;
extern float fltPlayerMass;
extern float fltPlayerStrafeFriction;
extern float fltPlayerMaxStrafeSpeed, fltPlayerMinStrafeSpeed;

/* Collision points */
extern float fltPlayerNoseX, fltPlayerNoseY;
extern float fltPlayerTailX, fltPlayerTailY;
extern int intPlayerNoseX, intPlayerNoseY;
extern int intPlayerTailX, intPlayerTailY;

/* Angular momentum (for bridge) */
extern float fltPlayerAngularVel;
extern int intPlayerInvulnFrames;

/*============================================================================
 * LIFECYCLE (no-ops, kept for compatibility)
 *============================================================================*/
void InitPlayer(void);
void DestroyPlayer(void);

/*============================================================================
 * UTILITY (defined in game/sprites.c)
 *============================================================================*/
void LoadRaw(const char* filename, void* buffer, size_t size);

#endif /* _PLAYER_H_ */
