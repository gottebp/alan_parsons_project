/*
 * Player module - Legacy globals and initialization
 *
 * The new architecture uses:
 *   - game/game.c: player_update(), player_fire_weapons()
 *   - game/render.c: render_player()
 *   - game/sprites.c: sprite loading
 *
 * KEPT FOR BRIDGE COMPATIBILITY:
 *   - Player state globals (position, velocity, health, etc.)
 *   - InitPlayer()/DestroyPlayer() - now no-ops
 */

#include "player.h"
#include "defs.h"

/* Player state - needed by bridge for sync */
int intPlayerX = 0;
int intPlayerY = 0;
float fltPlayerX = 0.0f;
float fltPlayerY = 0.0f;
float fltPlayerSpeed = 0.0f;
float fltPlayerStrafeSpeed = 0.0f;
int8_t intbPlayerAngle = PLAYER_START_ANGLE;
int8_t intbPlayerTurnDir = 0;
int intPlayerHealth = MAXPLAYERHEALTH;
int intPlayerWeaponsLevel = 0;

/* Player physics constants */
float fltPlayerAccel = 1.07f;
float fltPlayerFriction = 0.17f;
float fltPlayerMaxSpeed = 16.0f;
float fltPlayerMinSpeed = -12.0f;
float fltPlayerMass = 100.0f;
float fltPlayerStrafeFriction = 0.20f;
float fltPlayerMaxStrafeSpeed = 12.0f;
float fltPlayerMinStrafeSpeed = -12.0f;

/* Angular momentum - for bridge compatibility */
float fltPlayerAngularVel = 0.0f;

/* Invulnerability frames - for bridge compatibility */
int intPlayerInvulnFrames = 0;

/* Player collision points - needed by bridge */
float fltPlayerNoseX = 0.0f, fltPlayerNoseY = 0.0f;
float fltPlayerTailX = 0.0f, fltPlayerTailY = 0.0f;
int intPlayerNoseX = 0, intPlayerNoseY = 0;
int intPlayerTailX = 0, intPlayerTailY = 0;

/*
 * Initialize player
 * Sprite loading moved to sprites_init() in game/sprites.c
 */
void InitPlayer(void) {
    /* No-op - kept for compatibility */
}

/*
 * Destroy player
 * Sprite cleanup moved to sprites_destroy() in game/sprites.c
 */
void DestroyPlayer(void) {
    /* No-op - kept for compatibility */
}
