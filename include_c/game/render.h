/*
 * render.h - Game Rendering
 *
 * All rendering functions read from the Game struct.
 * Uses legacy AlphaBlit infrastructure with clean data interfaces.
 */

#ifndef RENDER_H
#define RENDER_H

#include "game.h"
#include <stdint.h>

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

/* Set the render context (call once during app init)
 * This allows render.c to use these resources without global dependencies */
void render_set_context(uint32_t* screen, uint32_t* temp, void* renderer, void* texture, int width, int height);

/*============================================================================
 * RENDERING FUNCTIONS
 *
 * All functions take const Game* and render to screen.
 *============================================================================*/

/* Render map background (camera follows player) */
void render_map(const Game* game);

/* Render player ship */
void render_player(const Game* game);

/* Render all enemies */
void render_enemies(const Game* game);

/* Render minimap radar */
void render_minimap(const Game* game);

/* Render player health bar */
void render_health_bar(const Game* game);

/* Render nuke icons */
void render_nuke_icons(const Game* game);

/* Render complete game frame (map, entities, UI) */
void render_frame(const Game* game);

/* Render particles using hardware acceleration */
void render_particles_hw(const Game* game);

/* Complete render + present cycle */
void render_present(const Game* game);

#endif /* RENDER_H */
