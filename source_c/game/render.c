/*
 * render.c - Game Rendering
 *
 * All rendering functions read from the Game struct.
 * Uses legacy AlphaBlit infrastructure with clean data interfaces.
 */

#include "../../include_c/game/game.h"
#include "../../include_c/core/constants.h"
#include <SDL.h>
#include <stdint.h>
#include <string.h>

/*============================================================================
 * RENDER CONTEXT - Internal state set by render_set_context()
 * This replaces direct use of globals, allowing clean initialization.
 *============================================================================*/

static uint32_t* r_screen = NULL;      /* Screen buffer */
static uint32_t* r_temp = NULL;        /* Temp buffer for fades */
static SDL_Renderer* r_renderer = NULL; /* SDL renderer */
static SDL_Texture* r_texture = NULL;   /* Screen texture */
static int r_width = 0;
static int r_height = 0;
static int r_initialized = 0;

void render_set_context(uint32_t* screen, uint32_t* temp, void* renderer, void* texture, int width, int height) {
    r_screen = screen;
    r_temp = temp;
    r_renderer = (SDL_Renderer*)renderer;
    r_texture = (SDL_Texture*)texture;
    r_width = width;
    r_height = height;
    r_initialized = 1;
}

/*============================================================================
 * EXTERNAL DECLARATIONS - Legacy Graphics System
 * NOTE: These are kept for backward compatibility during migration.
 * New code should use the render context above.
 *============================================================================*/

/* Screen buffer - from sdl_wrapper.c (fallback if context not set) */
extern uint32_t* ScreenOff;

/* Alpha blitting - from sdl_wrapper.c */
extern void AlphaBlit(int x, int y, uint32_t* src, int src_width, int src_height);

/* Player sprites - from player.c */
extern uint32_t* PlayerShipOff;
extern uint32_t* PlayerShipOffL;
extern uint32_t* PlayerShipOffR;

/* Enemy sprites - from enemy.c */
extern uint32_t* SmallEnemyImageData;
extern uint32_t* BossImageData;

/* Alpha computation - from sdl_wrapper.c */
extern uint32_t ComputeAlpha(uint32_t src, uint32_t dst);

/* Map data - from mapeng.c */
extern uint32_t* MapOff;

/* SSE memory operations - from sse_mem.c */
extern void sseMemcpy32(uint32_t* dst, const uint32_t* src, int count);

/* Random number generator - from rand.c */
extern uint32_t Rand(void);

/*============================================================================
 * MAP RENDERING FROM GAME STRUCT
 *============================================================================*/

void render_map(const Game* game) {
    /* Use context if set, otherwise fall back to global */
    uint32_t* screen = r_initialized ? r_screen : ScreenOff;
    if (!game || !MapOff || !screen) return;

    int cam_x = (int)game->player.position.x + game->shake_offset_x;
    int cam_y = (int)game->player.position.y + game->shake_offset_y;

    /* Calculate boundary offsets for toroidal wrapping */
    int top_offset = (cam_x - (SCREEN_WIDTH / 2)) * 4;
    int bottom_offset = ((MAP_HEIGHT - 1) * MAP_WIDTH + cam_x - (SCREEN_WIDTH / 2)) * 4;

    /* Calculate starting map position */
    int map_offset = ((cam_y - (SCREEN_HEIGHT / 2)) * MAP_WIDTH + cam_x - (SCREEN_WIDTH / 2)) * 4;

    /* Render each scanline with toroidal wrapping */
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        /* Map wrapping */
        if (map_offset < top_offset) {
            map_offset += (MAP_WIDTH * MAP_HEIGHT) * 4;
        }
        if (map_offset > bottom_offset) {
            map_offset -= (MAP_WIDTH * MAP_HEIGHT) * 4;
        }

        /* Copy scanline from map to screen */
        uint32_t* src = (uint32_t*)((uint8_t*)MapOff + map_offset);
        uint32_t* dst = screen + (y * SCREEN_WIDTH);

        sseMemcpy32(dst, src, SCREEN_WIDTH);

        /* Move to next scanline */
        map_offset += MAP_WIDTH * 4;
    }
}

/*============================================================================
 * PLAYER RENDERING FROM GAME STRUCT
 *============================================================================*/

void render_player(const Game* game) {
    if (!game || game->player.health <= 0) {
        return;
    }

    const Player* p = &game->player;

    /* Select sprite based on turn direction */
    uint32_t* sprite = PlayerShipOff;
    if (p->turn_direction < 0) {
        sprite = PlayerShipOffL;
    } else if (p->turn_direction > 0) {
        sprite = PlayerShipOffR;
    }

    if (!sprite) return;

    /* Calculate which rotation frame to use */
    uint8_t angle_idx = (uint8_t)p->angle;
    int frame = angle_idx;

    /* Get sprite for this angle */
    uint32_t* frame_data = sprite + (frame * PLAYER_WIDTH * PLAYER_HEIGHT);

    /* Player is always centered on screen */
    int screen_x = (SCREEN_WIDTH / 2) - (PLAYER_WIDTH / 2);
    int screen_y = (SCREEN_HEIGHT / 2) - (PLAYER_HEIGHT / 2);

    /* Blit player sprite */
    AlphaBlit(screen_x, screen_y, frame_data, PLAYER_WIDTH, PLAYER_HEIGHT);
}

/*============================================================================
 * ENEMY RENDERING FROM GAME STRUCT
 *============================================================================*/

/* Helper function to render a single enemy */
static void render_single_enemy(const Enemy* e, int cam_x, int cam_y) {
    int ex = (int)e->position.x;
    int ey = (int)e->position.y;

    /* Calculate screen position relative to camera */
    int screen_x = ex - cam_x + (SCREEN_WIDTH / 2);
    int screen_y = ey - cam_y + (SCREEN_HEIGHT / 2);

    uint32_t* sprite;
    int width;
    int height;

    /* ENEMYSHIFTSIZE = 16 (128*128*4 = 65536 = 2^16 bytes per frame) */
    #define ENEMYSHIFTSIZE 16

    if (e->size == ENEMY_SIZE_SMALL) {
        if (!SmallEnemyImageData) return;

        sprite = SmallEnemyImageData;
        width = SMALL_ENEMY_WIDTH;
        height = SMALL_ENEMY_HEIGHT;

        /* Calculate sprite offset:
         * type * 16 frames * frame_size + angle_frame * frame_size */
        int type_offset = (int)e->type << (4 + ENEMYSHIFTSIZE);
        int angle_frame = e->angle >> 4;
        int angle_offset = angle_frame << ENEMYSHIFTSIZE;

        sprite += (type_offset + angle_offset) / 4;  /* Divide by 4 for uint32_t* */

        /* Center sprite on enemy position */
        screen_x -= SMALL_ENEMY_WIDTH / 2;
        screen_y -= SMALL_ENEMY_HEIGHT / 2;

    } else {
        /* Boss enemy - 256x256 */
        if (!BossImageData) return;

        sprite = BossImageData;
        width = BOSS_WIDTH;
        height = BOSS_HEIGHT;

        /* Calculate sprite offset (same calculation, different base) */
        int type_offset = (int)e->type << (4 + ENEMYSHIFTSIZE);
        int angle_frame = e->angle >> 4;
        int angle_offset = angle_frame << ENEMYSHIFTSIZE;

        /* Boss sprites are 4x larger - assembly multiplies by 4 */
        sprite += (type_offset + angle_offset);

        /* Center sprite on enemy position */
        screen_x -= BOSS_WIDTH / 2;
        screen_y -= BOSS_HEIGHT / 2;
    }

    #undef ENEMYSHIFTSIZE

    /* Map wrapping for screen coordinates */
    if (screen_x < (-MAP_WIDTH + SCREEN_WIDTH)) {
        screen_x += MAP_WIDTH;
    }
    if (screen_x > MAP_WIDTH) {
        screen_x -= MAP_WIDTH;
    }
    if (screen_y < (-MAP_HEIGHT + SCREEN_HEIGHT)) {
        screen_y += MAP_HEIGHT;
    }
    if (screen_y > MAP_HEIGHT) {
        screen_y -= MAP_HEIGHT;
    }

    /* Cull if completely offscreen */
    if (screen_x + width < 0 || screen_x >= SCREEN_WIDTH ||
        screen_y + height < 0 || screen_y >= SCREEN_HEIGHT) {
        return;
    }

    AlphaBlit(screen_x, screen_y, sprite, width, height);
}

void render_enemies(const Game* game) {
    if (!game) return;

    int cam_x = (int)game->player.position.x + game->shake_offset_x;
    int cam_y = (int)game->player.position.y + game->shake_offset_y;

    /* Iterate through enemy pool */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy* e = &game->enemies.entities[i];
        if (!e->active) continue;
        render_single_enemy(e, cam_x, cam_y);
    }
}

/*============================================================================
 * MINIMAP RENDERING FROM GAME STRUCT
 *============================================================================*/

/* Helper: convert world coords to minimap pixel, return 0 if out of bounds */
static int minimap_coords(int world_x, int world_y, int margin, int* mx, int* my) {
    *mx = (world_x / (MAP_WIDTH / MINIMAP_WIDTH)) + MINIMAP_X;
    *my = (world_y / (MAP_HEIGHT / MINIMAP_HEIGHT)) + MINIMAP_Y;
    return (*mx >= MINIMAP_X + margin && *mx < MINIMAP_X + MINIMAP_WIDTH - margin &&
            *my >= MINIMAP_Y + margin && *my < MINIMAP_Y + MINIMAP_HEIGHT - margin);
}

/* Draw a filled square dot: size = (2r-1) x (2r-1) pixels */
static void minimap_draw_dot(int world_x, int world_y,
                             uint32_t color, int radius, uint32_t* screen) {
    int mx, my;
    if (!minimap_coords(world_x, world_y, radius, &mx, &my)) return;
    for (int dy = -(radius - 1); dy <= (radius - 1); dy++) {
        for (int dx = -(radius - 1); dx <= (radius - 1); dx++) {
            int offset = (my + dy) * SCREEN_WIDTH + (mx + dx);
            screen[offset] = ComputeAlpha(color, screen[offset]);
        }
    }
}

/* Draw a + cross: arm length = (2r-1) pixels */
static void minimap_draw_cross(int world_x, int world_y,
                               uint32_t color, int radius, uint32_t* screen) {
    int mx, my;
    if (!minimap_coords(world_x, world_y, radius, &mx, &my)) return;
    for (int d = -(radius - 1); d <= (radius - 1); d++) {
        int h = my * SCREEN_WIDTH + (mx + d);
        int v = (my + d) * SCREEN_WIDTH + mx;
        screen[h] = ComputeAlpha(color, screen[h]);
        screen[v] = ComputeAlpha(color, screen[v]);
    }
}

void render_minimap(const Game* game) {
    if (!game || game->player.health <= 0) return;

    /* Use context if set, otherwise fall back to global */
    uint32_t* screen = r_initialized ? r_screen : ScreenOff;
    if (!screen) return;

    /* Draw semi-transparent dark background */
    uint32_t dark_overlay = 0x80000000;
    for (int row = 0; row < MINIMAP_HEIGHT; row++) {
        int y = MINIMAP_Y + row;
        for (int col = 0; col < MINIMAP_WIDTH; col++) {
            int x = MINIMAP_X + col;
            int offset = y * SCREEN_WIDTH + x;
            screen[offset] = ComputeAlpha(dark_overlay, screen[offset]);
        }
    }

    /* Draw player (bright green dot, 3x3) */
    minimap_draw_dot((int)game->player.position.x, (int)game->player.position.y,
                     0xFF00FF00, 2, screen);

    /* Draw enemies (red +, larger for bosses) */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        const Enemy* e = &game->enemies.entities[i];
        if (!e->active) continue;
        int radius = (e->size == ENEMY_SIZE_BOSS) ? 4 : 3;
        minimap_draw_cross((int)e->position.x, (int)e->position.y,
                           0xFFFF0000, radius, screen);
    }
}

/*============================================================================
 * HEALTH BAR RENDERING FROM GAME STRUCT
 *============================================================================*/

void render_health_bar(const Game* game) {
    if (!game || game->player.health <= 0) return;

    /* Use context if set, otherwise fall back to global */
    uint32_t* screen = r_initialized ? r_screen : ScreenOff;
    if (!screen) return;

    int health = game->player.health;
    if (health > PLAYER_MAX_HEALTH) health = PLAYER_MAX_HEALTH;

    int bar_width = (health * (SCREEN_WIDTH - 30)) / PLAYER_MAX_HEALTH;

    /* Draw health bar background area could go here */

    /* Draw health bar fill (blue, semi-transparent) */
    uint32_t bar_color = 0x800000FF;
    for (int row = 0; row < PLAYER_HEALTH_BAR_HEIGHT; row++) {
        int y = 5 + row;
        for (int col = 0; col < bar_width; col++) {
            int x = 15 + col;
            int offset = y * SCREEN_WIDTH + x;
            screen[offset] = ComputeAlpha(bar_color, screen[offset]);
        }
    }
}

/*============================================================================
 * NUKE ICONS RENDERING FROM GAME STRUCT
 *============================================================================*/

/* External nuke image from main.c */
extern uint32_t nuke_img[32 * 32];

void render_nuke_icons(const Game* game) {
    if (!game) return;

    int nukes = game->player.nukes_remaining;
    if (nukes <= 0) return;

    for (int i = 0; i < nukes; i++) {
        int x = SCREEN_WIDTH - 46 - (i * 34);
        int y = 18;
        AlphaBlit(x, y, nuke_img, 32, 32);
    }
}

/*============================================================================
 * COMPLETE FRAME RENDERING FROM GAME STRUCT
 *
 * This function renders everything from the Game struct.
 * Call this instead of the individual legacy render functions when
 * USE_NEW_RENDER is enabled.
 *============================================================================*/

/* External victory/defeat screens */
extern uint32_t* victory_screen;
extern uint32_t* defeat_screen;

/* External SDL renderer and particle textures from ppe.c */
extern SDL_Renderer* screen_renderer;
extern SDL_Texture* SmallParticleTexture;
extern SDL_Texture* LargeParticleTexture;

/*============================================================================
 * HARDWARE-ACCELERATED PARTICLE RENDERING FROM GAME STRUCT
 *
 * This renders particles directly from the Game->particles pool using
 * GPU-accelerated blitting. No need for legacy ParticleDataOff sync.
 *============================================================================*/

void render_particles_hw(const Game* game) {
    if (!game) return;

    /* Use context if set, otherwise fall back to global */
    SDL_Renderer* renderer = r_initialized ? r_renderer : screen_renderer;
    if (!SmallParticleTexture || !LargeParticleTexture || !renderer) {
        return;  /* Textures not loaded yet */
    }

    int cam_x = (int)game->player.position.x + game->shake_offset_x;
    int cam_y = (int)game->player.position.y + game->shake_offset_y;

    /* Iterate through particle pool directly */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        const Particle* p = &game->particles.entities[i];
        if (!p->active) continue;

        /* Calculate screen position relative to camera */
        int screen_x = (int)p->position.x - cam_x + (SCREEN_WIDTH / 2);
        int screen_y = (int)p->position.y - cam_y + (SCREEN_HEIGHT / 2);

        /* Get particle dimensions and texture */
        SDL_Texture* tex;
        SDL_Rect src_rect, dst_rect;

        if (p->size == PARTICLE_SIZE_SMALL) {
            tex = SmallParticleTexture;
            src_rect.x = 0;
            src_rect.y = p->flare_index * SMALL_PARTICLE_HEIGHT;
            src_rect.w = SMALL_PARTICLE_WIDTH;
            src_rect.h = SMALL_PARTICLE_HEIGHT;

            dst_rect.x = screen_x - SMALL_PARTICLE_WIDTH / 2;
            dst_rect.y = screen_y - SMALL_PARTICLE_HEIGHT / 2;
            dst_rect.w = SMALL_PARTICLE_WIDTH;
            dst_rect.h = SMALL_PARTICLE_HEIGHT;
        } else {
            tex = LargeParticleTexture;
            src_rect.x = 0;
            src_rect.y = p->flare_index * LARGE_PARTICLE_HEIGHT;
            src_rect.w = LARGE_PARTICLE_WIDTH;
            src_rect.h = LARGE_PARTICLE_HEIGHT;

            dst_rect.x = screen_x - LARGE_PARTICLE_WIDTH / 2;
            dst_rect.y = screen_y - LARGE_PARTICLE_HEIGHT / 2;
            dst_rect.w = LARGE_PARTICLE_WIDTH;
            dst_rect.h = LARGE_PARTICLE_HEIGHT;
        }

        /* Handle map wrapping */
        if (dst_rect.x < (-MAP_WIDTH + SCREEN_WIDTH)) {
            dst_rect.x += MAP_WIDTH;
        } else if (dst_rect.x > MAP_WIDTH) {
            dst_rect.x -= MAP_WIDTH;
        }
        if (dst_rect.y < (-MAP_HEIGHT + SCREEN_HEIGHT)) {
            dst_rect.y += MAP_HEIGHT;
        } else if (dst_rect.y > MAP_HEIGHT) {
            dst_rect.y -= MAP_HEIGHT;
        }

        /* Early culling */
        if (dst_rect.x + dst_rect.w < 0 || dst_rect.x >= SCREEN_WIDTH ||
            dst_rect.y + dst_rect.h < 0 || dst_rect.y >= SCREEN_HEIGHT) {
            continue;
        }

        /* Hardware-accelerated blit with alpha blending */
        SDL_RenderCopy(renderer, tex, &src_rect, &dst_rect);
    }
}

/*============================================================================
 * COMPLETE SCREEN UPDATE WITH PARTICLE RENDERING
 *
 * This function handles the full render + present cycle, rendering particles
 * directly from the Game struct without needing legacy sync.
 *============================================================================*/

extern SDL_Texture* screen_texture;
extern SDL_Renderer* screen_renderer;

void render_present(const Game* game) {
    /* Use context if set, otherwise fall back to globals */
    uint32_t* screen = r_initialized ? r_screen : ScreenOff;
    SDL_Texture* texture = r_initialized ? r_texture : screen_texture;
    SDL_Renderer* renderer = r_initialized ? r_renderer : screen_renderer;

    if (!game || !texture || !renderer) return;

    /* Upload software-rendered content to GPU */
    SDL_UpdateTexture(texture, NULL, screen, SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);

    /* Render particles using hardware acceleration directly from Game struct */
    render_particles_hw(game);

    /* Present to screen */
    SDL_RenderPresent(renderer);
}

void render_frame(const Game* game) {
    if (!game) return;

    /* Map rendering from Game struct - no longer needs legacy globals */
    render_map(game);

    /* Render game entities from new Game struct */
    render_player(game);
    render_enemies(game);

    /* UI elements */
    render_health_bar(game);
    render_minimap(game);
    render_nuke_icons(game);

    /* Victory/defeat overlays */
    if (game->state == STATE_VICTORY && victory_screen) {
        AlphaBlit(
            SCREEN_WIDTH / 2 - 460 / 2,
            SCREEN_HEIGHT / 2 - 345 / 2,
            victory_screen,
            460, 345
        );
    }

    if (game->state == STATE_DEFEAT && defeat_screen) {
        AlphaBlit(
            SCREEN_WIDTH / 2 - 460 / 2,
            SCREEN_HEIGHT / 2 - 345 / 2,
            defeat_screen,
            460, 345
        );
    }
}
