/*
 * bridge.h - Bridge Between New and Old Architecture
 *
 * Provides functions to sync state between the new Game struct
 * and the legacy global variables. This allows incremental migration:
 *
 * Usage during migration:
 *   1. Call bridge_sync_all_from_globals() at start to import legacy state
 *   2. Run new game logic on Game struct
 *   3. Call bridge_sync_all_to_globals() before rendering with legacy code
 *
 * Eventually, when all systems use Game directly, this bridge is removed.
 */

#ifndef BRIDGE_H
#define BRIDGE_H

#include "game.h"

/*============================================================================
 * PLAYER SYNC
 *============================================================================*/

/* Sync player data from new Game struct to old globals */
void bridge_sync_player_to_globals(const Game* game);

/* Sync player data from old globals to new Game struct */
void bridge_sync_player_from_globals(Game* game);

/*============================================================================
 * ENEMY SYNC
 *============================================================================*/

/* Sync all enemies from new Game struct to old globals */
void bridge_sync_enemies_to_globals(const Game* game);

/* Sync all enemies from old globals to new Game struct */
void bridge_sync_enemies_from_globals(Game* game);

/*============================================================================
 * PARTICLE SYNC
 *============================================================================*/

/* Sync all particles from new Game struct to old globals */
void bridge_sync_particles_to_globals(const Game* game);

/* Sync all particles from old globals to new Game struct */
void bridge_sync_particles_from_globals(Game* game);

/*============================================================================
 * GAME STATE SYNC
 *============================================================================*/

/* Sync game state (shake, waves, camera) to globals */
void bridge_sync_game_state_to_globals(const Game* game);

/* Sync game state from globals */
void bridge_sync_game_state_from_globals(Game* game);

/*============================================================================
 * FULL SYNC
 *============================================================================*/

/* Full sync from Game to globals - call before rendering */
void bridge_sync_all_to_globals(const Game* game);

/* Full sync from globals to Game - call after legacy input/updates */
void bridge_sync_all_from_globals(Game* game);

/* Initialize Game struct from current global state */
void bridge_init_game_from_globals(Game* game);

/*============================================================================
 * GAME UPDATE
 *============================================================================*/

/* Update game using legacy KEYBOARD input - calls game_update() internally */
void bridge_update_from_legacy_input(Game* game, float dt);

/*============================================================================
 * GAME CONFIGURATION
 *============================================================================*/

/* Set captain planet mode (18 nukes instead of 4) */
void bridge_set_captain_planet(Game* game, int enabled);

/* Set player weapons level */
void bridge_set_weapons_level(Game* game, int level);

/* Get player weapons level */
int bridge_get_weapons_level(const Game* game);

/* Increment weapons level (caps at 5) */
void bridge_increment_weapons_level(Game* game);

#endif /* BRIDGE_H */
