/*
 * render.h - Rendering Interface
 *
 * Clean rendering functions that read from Game state.
 * No globals accessed - all state passed explicitly.
 * This is the visual manifestation of the game's truth.
 */

#ifndef RENDER_H
#define RENDER_H

#include "../game/game.h"
#include <SDL.h>

/*============================================================================
 * RENDER CONTEXT
 *
 * Holds all rendering resources: textures, surfaces, the renderer itself.
 * Created once at startup, passed to all render functions.
 *============================================================================*/

typedef struct {
    SDL_Renderer* renderer;
    SDL_Texture* screen_texture;

    /* Player ship sprites (3 banks: normal, left-turn, right-turn) */
    uint32_t* player_sprites[3];

    /* Enemy sprites */
    uint32_t* small_enemy_sprites;
    uint32_t* boss_sprites;

    /* Particle flares */
    uint32_t* small_flares;
    uint32_t* large_flares;
    SDL_Texture* small_flare_texture;
    SDL_Texture* large_flare_texture;

    /* Map data */
    uint32_t* map_data;
    uint32_t* map_pad_top;
    uint32_t* map_pad_bottom;

    /* Screen buffer (software rendering target) */
    uint32_t* screen_buffer;
    uint32_t* screen_temp;  /* For fade effects */

    /* UI images */
    uint32_t* nuke_icon;
    uint32_t* victory_screen;
    uint32_t* defeat_screen;
} RenderContext;

/*============================================================================
 * INITIALIZATION AND SHUTDOWN
 *============================================================================*/

/* Initialize render context with SDL renderer */
int render_init(RenderContext* ctx, SDL_Renderer* renderer);

/* Load all game sprites and textures */
int render_load_assets(RenderContext* ctx);

/* Free all rendering resources */
void render_shutdown(RenderContext* ctx);

/* Load a map for the current level */
int render_load_map(RenderContext* ctx, const char* filename);

/*============================================================================
 * FRAME RENDERING
 *
 * The main rendering pipeline, reading from Game state.
 *============================================================================*/

/* Render complete frame - main entry point */
void render_frame(RenderContext* ctx, const Game* game);

/* Clear screen to black */
void render_clear(RenderContext* ctx);

/* Present screen buffer to display */
void render_present(RenderContext* ctx);

/*============================================================================
 * COMPONENT RENDERING
 *
 * Individual render functions for each game element.
 * All read from Game state, write to screen buffer.
 *============================================================================*/

/* Render the map background with toroidal wrapping */
void render_map(RenderContext* ctx, const Game* game);

/* Render player ship at center of screen */
void render_player(RenderContext* ctx, const Player* player);

/* Render all active enemies relative to camera */
void render_enemies(RenderContext* ctx, const Game* game);

/* Render all active particles (hardware accelerated) */
void render_particles_hw(RenderContext* ctx, const Game* game);

/* Render all active particles (software fallback) */
void render_particles_sw(RenderContext* ctx, const Game* game);

/*============================================================================
 * UI RENDERING
 *============================================================================*/

/* Render player health bar */
void render_health_bar(RenderContext* ctx, const Player* player);

/* Render minimap with player and enemy positions */
void render_minimap(RenderContext* ctx, const Game* game);

/* Render nuke icons */
void render_nuke_icons(RenderContext* ctx, int nukes_remaining);

/* Render victory screen overlay */
void render_victory_screen(RenderContext* ctx);

/* Render defeat screen overlay */
void render_defeat_screen(RenderContext* ctx);

/*============================================================================
 * UTILITY FUNCTIONS
 *============================================================================*/

/* Alpha blend a sprite onto the screen buffer */
void render_alpha_blit(RenderContext* ctx, int x, int y,
                       const uint32_t* sprite, int width, int height);

/* Compute alpha blend of two pixels */
uint32_t render_compute_alpha(uint32_t src, uint32_t dst);

/* Apply screen shake offset */
void render_apply_shake(const Game* game, int* offset_x, int* offset_y);

/*============================================================================
 * CAMERA
 *
 * Camera position derived from player position.
 * In this game, camera always centers on player.
 *============================================================================*/

typedef struct {
    int x, y;           /* Integer camera position (world coords) */
    float fx, fy;       /* Float camera position for precision */
    int shake_x;        /* Current shake offset */
    int shake_y;
} Camera;

/* Update camera to follow player */
void camera_update(Camera* cam, const Player* player, int shake_frames, uint32_t rand_state);

/* Convert world position to screen position */
static inline void camera_world_to_screen(const Camera* cam, int world_x, int world_y,
                                          int* screen_x, int* screen_y) {
    *screen_x = world_x - cam->x + (SCREEN_WIDTH / 2) + cam->shake_x;
    *screen_y = world_y - cam->y + (SCREEN_HEIGHT / 2) + cam->shake_y;
}

#endif /* RENDER_H */
