/*
 * bridge_opaque.h - Opaque Bridge Interface for Legacy Code
 *
 * This header can be included by code that uses legacy defs.h types.
 * It provides an opaque Game handle without exposing the internal types.
 *
 * Use this from main.c to integrate the new game logic while keeping
 * the legacy type system intact for rendering.
 */

#ifndef BRIDGE_OPAQUE_H
#define BRIDGE_OPAQUE_H

#include <stdint.h>

/* Opaque handle to Game struct - actual struct defined in game.h */
typedef struct Game Game;

/* Forward declaration for InputState */
typedef struct InputState InputState;

/*============================================================================
 * LIFECYCLE
 *============================================================================*/

/* Allocate and initialize a new Game struct */
Game* bridge_create_game(void);

/* Free a Game struct */
void bridge_destroy_game(Game* game);

/* Seed the game's random number generator */
void bridge_seed_random(Game* game, uint32_t seed);

/*============================================================================
 * LEVEL MANAGEMENT
 *============================================================================*/

/* Start a level (0-5) */
void bridge_start_level(Game* game, int level_index);

/* Set captain planet mode (18 nukes instead of 4) */
void bridge_set_captain_planet(Game* game, int enabled);

/* Set player weapons level */
void bridge_set_weapons_level(Game* game, int level);

/* Get player weapons level */
int bridge_get_weapons_level(const Game* game);

/* Increment weapons level (caps at 5) */
void bridge_increment_weapons_level(Game* game);

/*============================================================================
 * SYNC FUNCTIONS
 *============================================================================*/

/* Full sync from Game to globals - call before rendering */
void bridge_sync_all_to_globals(const Game* game);

/* Full sync from globals to Game - call after legacy input */
void bridge_sync_all_from_globals(Game* game);

/*============================================================================
 * GAME UPDATE
 *============================================================================*/

/* Update game with legacy input (reads from global KEYBOARD etc.) */
void bridge_update_from_legacy_input(Game* game, float dt);

/* Check if game is in playing state */
int bridge_is_playing(const Game* game);

/* Get player health */
int bridge_get_player_health(const Game* game);

/* Check victory/defeat status: 0=playing, 1=dead, 2=win */
int bridge_get_outcome(const Game* game);

/* Get number of active enemies */
int bridge_get_enemy_count(const Game* game);

/* Get player position (for camera) */
float bridge_get_player_x(const Game* game);
float bridge_get_player_y(const Game* game);

/*============================================================================
 * MENU STATE
 *============================================================================*/

/* Check if menu should be shown */
int bridge_menu_requested(const Game* game);

/* Clear menu request flag */
void bridge_clear_menu_request(Game* game);

/* Set menu result (from RunMenu) */
void bridge_set_menu_result(Game* game, int result);

/* Request menu to be shown */
void bridge_request_menu(Game* game);

/*============================================================================
 * AUDIO BRIDGE
 *
 * These functions integrate the audio bridge into the game engine.
 *============================================================================*/

/* Called when a level starts - plays level music */
void bridge_audio_level_start(int level_index);

/* Called when an enemy is destroyed - plays explosion */
void bridge_audio_enemy_destroyed(void);

/* Called when player takes damage - plays hit sound */
void bridge_audio_player_hit(void);

/* Called on defeat - plays defeat sound */
void bridge_audio_defeat(void);

#endif /* BRIDGE_OPAQUE_H */
