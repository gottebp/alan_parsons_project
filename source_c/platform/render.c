/*
 * render.c - Clean Rendering Implementation
 *
 * All rendering reads from Game state.
 * No globals accessed directly.
 * This module transforms game truth into visual form.
 */

#include "../../include_c/platform/render.h"
#include "../../include_c/core/constants.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*============================================================================
 * EXTERNAL DECLARATIONS
 *
 * These are temporary - used to interface with legacy asset loading.
 * Will be removed when asset loading is fully integrated into render.c
 *============================================================================*/

/* Legacy asset loading function */
extern int LoadBMP(uint32_t* buffer, const char* filename);

/* Alpha computation from legacy sdl_wrapper */
extern uint32_t ComputeAlpha(uint32_t src, uint32_t dst);

/* Random number for shake (will be replaced with game->rand_state) */
extern uint32_t Rand(void);

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

int render_init(RenderContext* ctx, SDL_Renderer* renderer) {
    memset(ctx, 0, sizeof(RenderContext));
    ctx->renderer = renderer;

    /* Allocate screen buffer */
    ctx->screen_buffer = (uint32_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    ctx->screen_temp = (uint32_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));

    if (!ctx->screen_buffer || !ctx->screen_temp) {
        fprintf(stderr, "Failed to allocate screen buffers\n");
        return -1;
    }

    /* Create screen texture */
    ctx->screen_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH, SCREEN_HEIGHT
    );

    if (!ctx->screen_texture) {
        fprintf(stderr, "Failed to create screen texture: %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}

int render_load_assets(RenderContext* ctx) {
    /* Load player sprites */
    size_t ship_size = PLAYER_WIDTH * PLAYER_HEIGHT * PLAYER_SPRITE_FRAMES * sizeof(uint32_t);

    for (int i = 0; i < 3; i++) {
        ctx->player_sprites[i] = (uint32_t*)malloc(ship_size);
        if (!ctx->player_sprites[i]) {
            fprintf(stderr, "Failed to allocate player sprite memory\n");
            return -1;
        }
    }

    /* Load player ship variants - using raw format with magenta transparency */
    FILE* fp;
    const char* player_files[] = {
        "./data/player_norm.raw",
        "./data/player_left.raw",
        "./data/player_right.raw"
    };

    for (int i = 0; i < 3; i++) {
        fp = fopen(player_files[i], "rb");
        if (fp) {
            fread(ctx->player_sprites[i], ship_size, 1, fp);
            fclose(fp);

            /* Convert magenta to transparent */
            for (size_t j = 0; j < ship_size / 4; j++) {
                if (ctx->player_sprites[i][j] == 0xFF00FFFF) {
                    ctx->player_sprites[i][j] = 0x00000000;
                }
            }
        }
    }

    /* Load small enemy sprites */
    ctx->small_enemy_sprites = (uint32_t*)malloc(SMALL_ENEMY_MEM_SIZE);
    if (ctx->small_enemy_sprites) {
        LoadBMP(ctx->small_enemy_sprites, "./data/small_enemies.bmp");

        /* Convert magenta to transparent */
        for (size_t i = 0; i < SMALL_ENEMY_MEM_SIZE / 4; i++) {
            if (ctx->small_enemy_sprites[i] == 0xFF00FFFF) {
                ctx->small_enemy_sprites[i] = 0x00000000;
            }
        }
    }

    /* Load boss sprites */
    ctx->boss_sprites = (uint32_t*)malloc(BOSS_MEM_SIZE);
    if (ctx->boss_sprites) {
        LoadBMP(ctx->boss_sprites, "./data/large_enemies.bmp");

        /* Convert magenta to transparent */
        for (size_t i = 0; i < BOSS_MEM_SIZE / 4; i++) {
            if (ctx->boss_sprites[i] == 0xFF00FFFF) {
                ctx->boss_sprites[i] = 0x00000000;
            }
        }
    }

    /* Load particle flares */
    ctx->small_flares = (uint32_t*)malloc(SMALL_FLARE_FILE_WIDTH * SMALL_FLARE_FILE_HEIGHT * 4);
    ctx->large_flares = (uint32_t*)malloc(LARGE_FLARE_FILE_WIDTH * LARGE_FLARE_FILE_HEIGHT * 4);

    if (ctx->small_flares) {
        LoadBMP(ctx->small_flares, "./data/small_flares.bmp");
        /* Apply alpha from brightness */
        for (int i = 0; i < SMALL_FLARE_FILE_WIDTH * SMALL_FLARE_FILE_HEIGHT; i++) {
            uint32_t pixel = ctx->small_flares[i];
            uint8_t r = (pixel >> 16) & 0xFF;
            uint8_t g = (pixel >> 8) & 0xFF;
            uint8_t b = pixel & 0xFF;
            uint32_t avg = (r + g + b) / 3;
            if (avg >= 4) {
                ctx->small_flares[i] = (pixel & 0x00FFFFFF) | (avg << 24);
            } else {
                ctx->small_flares[i] = pixel & 0x00FFFFFF;
            }
        }
    }

    if (ctx->large_flares) {
        LoadBMP(ctx->large_flares, "./data/large_flares.bmp");
        /* Apply alpha from brightness */
        for (int i = 0; i < LARGE_FLARE_FILE_WIDTH * LARGE_FLARE_FILE_HEIGHT; i++) {
            uint32_t pixel = ctx->large_flares[i];
            uint8_t r = (pixel >> 16) & 0xFF;
            uint8_t g = (pixel >> 8) & 0xFF;
            uint8_t b = pixel & 0xFF;
            uint32_t avg = (r + g + b) / 3;
            if (avg >= 4) {
                ctx->large_flares[i] = (pixel & 0x00FFFFFF) | (avg << 24);
            } else {
                ctx->large_flares[i] = pixel & 0x00FFFFFF;
            }
        }
    }

    /* Create hardware textures for particles */
    if (ctx->renderer && ctx->small_flares) {
        ctx->small_flare_texture = SDL_CreateTexture(
            ctx->renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STATIC,
            SMALL_FLARE_FILE_WIDTH, SMALL_FLARE_FILE_HEIGHT
        );
        if (ctx->small_flare_texture) {
            SDL_UpdateTexture(ctx->small_flare_texture, NULL, ctx->small_flares,
                            SMALL_FLARE_FILE_WIDTH * sizeof(uint32_t));
            SDL_SetTextureBlendMode(ctx->small_flare_texture, SDL_BLENDMODE_BLEND);
        }
    }

    if (ctx->renderer && ctx->large_flares) {
        ctx->large_flare_texture = SDL_CreateTexture(
            ctx->renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STATIC,
            LARGE_FLARE_FILE_WIDTH, LARGE_FLARE_FILE_HEIGHT
        );
        if (ctx->large_flare_texture) {
            SDL_UpdateTexture(ctx->large_flare_texture, NULL, ctx->large_flares,
                            LARGE_FLARE_FILE_WIDTH * sizeof(uint32_t));
            SDL_SetTextureBlendMode(ctx->large_flare_texture, SDL_BLENDMODE_BLEND);
        }
    }

    /* Load UI images */
    ctx->nuke_icon = (uint32_t*)malloc(32 * 32 * sizeof(uint32_t));
    if (ctx->nuke_icon) {
        LoadBMP(ctx->nuke_icon, "./data/nuke.bmp");
        /* Apply transparency */
        for (int i = 0; i < 32 * 32; i++) {
            if (ctx->nuke_icon[i] != 0xFF000000) {
                ctx->nuke_icon[i] = (ctx->nuke_icon[i] & 0x00FFFFFF) | 0xB8000000;
            } else {
                /* Black pixels - compute alpha from brightness */
                uint32_t pixel = ctx->nuke_icon[i];
                uint8_t r = (pixel >> 16) & 0xFF;
                uint8_t g = (pixel >> 8) & 0xFF;
                uint8_t b = pixel & 0xFF;
                uint32_t avg = (r + g + b) / 3;
                if (avg >= 4) {
                    ctx->nuke_icon[i] = (pixel & 0x00FFFFFF) | (avg << 24);
                } else {
                    ctx->nuke_icon[i] = pixel & 0x00FFFFFF;
                }
            }
        }
    }

    ctx->victory_screen = (uint32_t*)malloc(460 * 345 * sizeof(uint32_t));
    if (ctx->victory_screen) {
        LoadBMP(ctx->victory_screen, "./data/victory.bmp");
        for (int i = 0; i < 460 * 345; i++) {
            ctx->victory_screen[i] = (ctx->victory_screen[i] & 0x00FFFFFF) | 0xC8000000;
        }
    }

    ctx->defeat_screen = (uint32_t*)malloc(460 * 345 * sizeof(uint32_t));
    if (ctx->defeat_screen) {
        LoadBMP(ctx->defeat_screen, "./data/defeat.bmp");
        for (int i = 0; i < 460 * 345; i++) {
            ctx->defeat_screen[i] = (ctx->defeat_screen[i] & 0x00FFFFFF) | 0xC8000000;
        }
    }

    return 0;
}

int render_load_map(RenderContext* ctx, const char* filename) {
    /* Allocate map memory with padding if not already done */
    if (!ctx->map_pad_top) {
        size_t total_size = (MAP_WIDTH * MAP_HEIGHT + MAP_WIDTH * 2) * sizeof(uint32_t);
        ctx->map_pad_top = (uint32_t*)malloc(total_size);
        if (!ctx->map_pad_top) return -1;

        ctx->map_data = ctx->map_pad_top + MAP_WIDTH;
        ctx->map_pad_bottom = ctx->map_data + (MAP_WIDTH * MAP_HEIGHT);
    }

    /* Load the map */
    if (LoadBMP(ctx->map_data, filename) < 0) {
        return -1;
    }

    /* Copy rows for seamless wrapping */
    memcpy(ctx->map_pad_top, ctx->map_data, MAP_WIDTH * sizeof(uint32_t));
    memcpy(ctx->map_pad_bottom, ctx->map_data + MAP_WIDTH * (MAP_HEIGHT - 1),
           MAP_WIDTH * sizeof(uint32_t));

    return 0;
}

void render_shutdown(RenderContext* ctx) {
    /* Free player sprites */
    for (int i = 0; i < 3; i++) {
        if (ctx->player_sprites[i]) free(ctx->player_sprites[i]);
    }

    /* Free enemy sprites */
    if (ctx->small_enemy_sprites) free(ctx->small_enemy_sprites);
    if (ctx->boss_sprites) free(ctx->boss_sprites);

    /* Free particle flares */
    if (ctx->small_flares) free(ctx->small_flares);
    if (ctx->large_flares) free(ctx->large_flares);

    /* Free textures */
    if (ctx->small_flare_texture) SDL_DestroyTexture(ctx->small_flare_texture);
    if (ctx->large_flare_texture) SDL_DestroyTexture(ctx->large_flare_texture);
    if (ctx->screen_texture) SDL_DestroyTexture(ctx->screen_texture);

    /* Free map */
    if (ctx->map_pad_top) free(ctx->map_pad_top);

    /* Free screen buffers */
    if (ctx->screen_buffer) free(ctx->screen_buffer);
    if (ctx->screen_temp) free(ctx->screen_temp);

    /* Free UI images */
    if (ctx->nuke_icon) free(ctx->nuke_icon);
    if (ctx->victory_screen) free(ctx->victory_screen);
    if (ctx->defeat_screen) free(ctx->defeat_screen);

    memset(ctx, 0, sizeof(RenderContext));
}

/*============================================================================
 * FRAME RENDERING
 *============================================================================*/

void render_clear(RenderContext* ctx) {
    memset(ctx->screen_buffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
}

void render_present(RenderContext* ctx) {
    SDL_UpdateTexture(ctx->screen_texture, NULL, ctx->screen_buffer,
                     SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderCopy(ctx->renderer, ctx->screen_texture, NULL, NULL);
    SDL_RenderPresent(ctx->renderer);
}

void render_frame(RenderContext* ctx, const Game* game) {
    render_clear(ctx);

    if (game->state == STATE_PLAYING ||
        game->state == STATE_VICTORY ||
        game->state == STATE_DEFEAT) {

        render_map(ctx, game);
        render_player(ctx, &game->player);
        render_enemies(ctx, game);
        render_health_bar(ctx, &game->player);
        render_minimap(ctx, game);
        render_nuke_icons(ctx, game->player.nukes_remaining);

        if (game->state == STATE_VICTORY) {
            render_victory_screen(ctx);
        } else if (game->state == STATE_DEFEAT) {
            render_defeat_screen(ctx);
        }
    }

    /* Particles rendered separately via hardware */
}

/*============================================================================
 * COMPONENT RENDERING
 *============================================================================*/

void render_map(RenderContext* ctx, const Game* game) {
    if (!ctx->map_data) return;

    /* Calculate camera position (player at center) */
    int cam_x = (int)game->player.position.x;
    int cam_y = (int)game->player.position.y;

    /* Apply screen shake */
    if (game->shake_frames > 0) {
        cam_x += (int)(Rand() % SHAKE_INTENSITY) - (SHAKE_INTENSITY / 2);
        cam_y += (int)(Rand() % SHAKE_INTENSITY) - (SHAKE_INTENSITY / 2);

        /* Wrap camera position */
        if (cam_x >= MAP_WIDTH) cam_x -= MAP_WIDTH;
        if (cam_x < 0) cam_x += MAP_WIDTH;
        if (cam_y >= MAP_HEIGHT) cam_y -= MAP_HEIGHT;
        if (cam_y < 0) cam_y += MAP_HEIGHT;
    }

    /* Calculate map offsets for toroidal rendering */
    int top_offset = (cam_x - SCREEN_WIDTH / 2) * 4;
    int bottom_offset = ((MAP_HEIGHT - 1) * MAP_WIDTH + cam_x - SCREEN_WIDTH / 2) * 4;
    int map_offset = ((cam_y - SCREEN_HEIGHT / 2) * MAP_WIDTH + cam_x - SCREEN_WIDTH / 2) * 4;

    /* Render each scanline */
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        /* Handle vertical wrapping */
        if (map_offset < top_offset) {
            map_offset += MAP_WIDTH * MAP_HEIGHT * 4;
        }
        if (map_offset > bottom_offset) {
            map_offset -= MAP_WIDTH * MAP_HEIGHT * 4;
        }

        /* Copy scanline */
        uint32_t* src = (uint32_t*)((uint8_t*)ctx->map_data + map_offset);
        uint32_t* dst = ctx->screen_buffer + y * SCREEN_WIDTH;
        memcpy(dst, src, SCREEN_WIDTH * sizeof(uint32_t));

        map_offset += MAP_WIDTH * 4;
    }
}

void render_player(RenderContext* ctx, const Player* player) {
    if (player->health <= 0) return;

    /* Select sprite based on turn direction */
    uint32_t* sprite = ctx->player_sprites[0];  /* Normal */
    if (player->turn_direction < 0) {
        sprite = ctx->player_sprites[1];  /* Left turn */
    } else if (player->turn_direction > 0) {
        sprite = ctx->player_sprites[2];  /* Right turn */
    }

    if (!sprite) return;

    /* Get rotation frame */
    int frame = (uint8_t)player->angle;
    uint32_t* frame_data = sprite + frame * PLAYER_WIDTH * PLAYER_HEIGHT;

    /* Player always at center of screen */
    int screen_x = (SCREEN_WIDTH - PLAYER_WIDTH) / 2;
    int screen_y = (SCREEN_HEIGHT - PLAYER_HEIGHT) / 2;

    render_alpha_blit(ctx, screen_x, screen_y, frame_data, PLAYER_WIDTH, PLAYER_HEIGHT);
}

void render_enemies(RenderContext* ctx, const Game* game) {
    /* Camera is at player position */
    int cam_x = (int)game->player.position.x;
    int cam_y = (int)game->player.position.y;

    POOL_FOREACH_CONST(&game->enemies, e, Enemy, {
        int ex = (int)e->position.x;
        int ey = (int)e->position.y;

        /* Calculate screen position */
        int screen_x = ex - cam_x + SCREEN_WIDTH / 2;
        int screen_y = ey - cam_y + SCREEN_HEIGHT / 2;

        uint32_t* sprite;
        int width, height;

        if (e->size == ENEMY_SIZE_SMALL) {
            if (!ctx->small_enemy_sprites) continue;

            sprite = ctx->small_enemy_sprites;
            width = SMALL_ENEMY_WIDTH;
            height = SMALL_ENEMY_HEIGHT;

            /* Calculate sprite offset: type * 16 frames * frame_size + angle_frame * frame_size */
            int type_offset = (int)e->type * 16 * SMALL_ENEMY_WIDTH * SMALL_ENEMY_HEIGHT;
            int angle_frame = e->angle >> 4;  /* 256 angles / 16 frames */
            int angle_offset = angle_frame * SMALL_ENEMY_WIDTH * SMALL_ENEMY_HEIGHT;
            sprite += type_offset + angle_offset;

            screen_x -= SMALL_ENEMY_WIDTH / 2;
            screen_y -= SMALL_ENEMY_HEIGHT / 2;
        } else {
            if (!ctx->boss_sprites) continue;

            sprite = ctx->boss_sprites;
            width = BOSS_WIDTH;
            height = BOSS_HEIGHT;

            /* Boss uses same frame calculation but larger sprites */
            int type_offset = (int)e->type * 16 * BOSS_WIDTH * BOSS_HEIGHT;
            int angle_frame = e->angle >> 4;
            int angle_offset = angle_frame * BOSS_WIDTH * BOSS_HEIGHT;
            sprite += type_offset + angle_offset;

            screen_x -= BOSS_WIDTH / 2;
            screen_y -= BOSS_HEIGHT / 2;
        }

        /* Handle map wrapping for screen position */
        if (screen_x < -MAP_WIDTH + SCREEN_WIDTH) screen_x += MAP_WIDTH;
        else if (screen_x > MAP_WIDTH) screen_x -= MAP_WIDTH;
        if (screen_y < -MAP_HEIGHT + SCREEN_HEIGHT) screen_y += MAP_HEIGHT;
        else if (screen_y > MAP_HEIGHT) screen_y -= MAP_HEIGHT;

        /* Cull if completely offscreen */
        if (screen_x + width < 0 || screen_x >= SCREEN_WIDTH ||
            screen_y + height < 0 || screen_y >= SCREEN_HEIGHT) {
            continue;
        }

        render_alpha_blit(ctx, screen_x, screen_y, sprite, width, height);
    });
}

void render_particles_hw(RenderContext* ctx, const Game* game) {
    if (!ctx->small_flare_texture || !ctx->large_flare_texture) {
        render_particles_sw(ctx, game);
        return;
    }

    int cam_x = (int)game->player.position.x;
    int cam_y = (int)game->player.position.y;

    POOL_FOREACH_CONST(&game->particles, p, Particle, {
        int px = (int)p->position.x;
        int py = (int)p->position.y;

        int screen_x = px - cam_x + SCREEN_WIDTH / 2;
        int screen_y = py - cam_y + SCREEN_HEIGHT / 2;

        SDL_Texture* tex;
        SDL_Rect src_rect, dst_rect;

        if (p->size == PARTICLE_SIZE_SMALL) {
            tex = ctx->small_flare_texture;
            src_rect.x = 0;
            src_rect.y = p->flare_index * SMALL_PARTICLE_HEIGHT;
            src_rect.w = SMALL_PARTICLE_WIDTH;
            src_rect.h = SMALL_PARTICLE_HEIGHT;

            dst_rect.x = screen_x - SMALL_PARTICLE_WIDTH / 2;
            dst_rect.y = screen_y - SMALL_PARTICLE_HEIGHT / 2;
            dst_rect.w = SMALL_PARTICLE_WIDTH;
            dst_rect.h = SMALL_PARTICLE_HEIGHT;
        } else {
            tex = ctx->large_flare_texture;
            src_rect.x = 0;
            src_rect.y = p->flare_index * LARGE_PARTICLE_HEIGHT;
            src_rect.w = LARGE_PARTICLE_WIDTH;
            src_rect.h = LARGE_PARTICLE_HEIGHT;

            dst_rect.x = screen_x - LARGE_PARTICLE_WIDTH / 2;
            dst_rect.y = screen_y - LARGE_PARTICLE_HEIGHT / 2;
            dst_rect.w = LARGE_PARTICLE_WIDTH;
            dst_rect.h = LARGE_PARTICLE_HEIGHT;
        }

        /* Map wrapping */
        if (dst_rect.x < -MAP_WIDTH + SCREEN_WIDTH) dst_rect.x += MAP_WIDTH;
        else if (dst_rect.x > MAP_WIDTH) dst_rect.x -= MAP_WIDTH;
        if (dst_rect.y < -MAP_HEIGHT + SCREEN_HEIGHT) dst_rect.y += MAP_HEIGHT;
        else if (dst_rect.y > MAP_HEIGHT) dst_rect.y -= MAP_HEIGHT;

        /* Culling */
        if (dst_rect.x + dst_rect.w < 0 || dst_rect.x >= SCREEN_WIDTH ||
            dst_rect.y + dst_rect.h < 0 || dst_rect.y >= SCREEN_HEIGHT) {
            continue;
        }

        SDL_RenderCopy(ctx->renderer, tex, &src_rect, &dst_rect);
    });
}

void render_particles_sw(RenderContext* ctx, const Game* game) {
    int cam_x = (int)game->player.position.x;
    int cam_y = (int)game->player.position.y;

    POOL_FOREACH_CONST(&game->particles, p, Particle, {
        int px = (int)p->position.x;
        int py = (int)p->position.y;

        int screen_x = px - cam_x + SCREEN_WIDTH / 2;
        int screen_y = py - cam_y + SCREEN_HEIGHT / 2;

        uint32_t* sprite;
        int width, height;

        if (p->size == PARTICLE_SIZE_SMALL) {
            if (!ctx->small_flares) continue;
            sprite = ctx->small_flares + p->flare_index * SMALL_PARTICLE_WIDTH * SMALL_PARTICLE_HEIGHT;
            width = SMALL_PARTICLE_WIDTH;
            height = SMALL_PARTICLE_HEIGHT;
        } else {
            if (!ctx->large_flares) continue;
            sprite = ctx->large_flares + p->flare_index * LARGE_PARTICLE_WIDTH * LARGE_PARTICLE_HEIGHT;
            width = LARGE_PARTICLE_WIDTH;
            height = LARGE_PARTICLE_HEIGHT;
        }

        screen_x -= width / 2;
        screen_y -= height / 2;

        /* Map wrapping */
        if (screen_x < -MAP_WIDTH + SCREEN_WIDTH) screen_x += MAP_WIDTH;
        else if (screen_x > MAP_WIDTH) screen_x -= MAP_WIDTH;
        if (screen_y < -MAP_HEIGHT + SCREEN_HEIGHT) screen_y += MAP_HEIGHT;
        else if (screen_y > MAP_HEIGHT) screen_y -= MAP_HEIGHT;

        /* Culling */
        if (screen_x + width < 0 || screen_x >= SCREEN_WIDTH ||
            screen_y + height < 0 || screen_y >= SCREEN_HEIGHT) {
            continue;
        }

        render_alpha_blit(ctx, screen_x, screen_y, sprite, width, height);
    });
}

/*============================================================================
 * UI RENDERING
 *============================================================================*/

void render_health_bar(RenderContext* ctx, const Player* player) {
    if (player->health <= 0 || player->health > PLAYER_MAX_HEALTH) return;

    int bar_width = (player->health * (SCREEN_WIDTH - 30)) / PLAYER_MAX_HEALTH;

    for (int row = 0; row < PLAYER_HEALTH_BAR_HEIGHT; row++) {
        int y = 5 + row;
        for (int col = 0; col < bar_width; col++) {
            int x = 15 + col;
            int offset = y * SCREEN_WIDTH + x;
            uint32_t color = 0x800000FF;  /* Semi-transparent blue */
            ctx->screen_buffer[offset] = render_compute_alpha(color, ctx->screen_buffer[offset]);
        }
    }
}

void render_minimap(RenderContext* ctx, const Game* game) {
    if (game->player.health <= 0) return;

    /* Draw dark overlay for minimap area */
    uint32_t dark = 0x80000000;
    for (int row = 0; row < MINIMAP_HEIGHT; row++) {
        int y = MINIMAP_Y + row;
        for (int col = 0; col < MINIMAP_WIDTH; col++) {
            int x = MINIMAP_X + col;
            int offset = y * SCREEN_WIDTH + x;
            ctx->screen_buffer[offset] = render_compute_alpha(dark, ctx->screen_buffer[offset]);
        }
    }

    /* Draw player (blue) */
    int px = (int)(game->player.position.x / (MAP_WIDTH / MINIMAP_WIDTH)) + MINIMAP_X;
    int py = (int)(game->player.position.y / (MAP_HEIGHT / MINIMAP_HEIGHT)) + MINIMAP_Y;

    /* Simple cross pattern for player */
    uint32_t blue = 0x800000FF;
    uint32_t white = 0xA0FFFFFF;

    if (px > MINIMAP_X && px < MINIMAP_X + MINIMAP_WIDTH - 1 &&
        py > MINIMAP_Y && py < MINIMAP_Y + MINIMAP_HEIGHT - 1) {
        int offset = py * SCREEN_WIDTH + px;
        ctx->screen_buffer[offset] = render_compute_alpha(blue, ctx->screen_buffer[offset]);
        ctx->screen_buffer[offset - 1] = render_compute_alpha(blue, ctx->screen_buffer[offset - 1]);
        ctx->screen_buffer[offset + 1] = render_compute_alpha(blue, ctx->screen_buffer[offset + 1]);
        ctx->screen_buffer[offset - SCREEN_WIDTH] = render_compute_alpha(blue, ctx->screen_buffer[offset - SCREEN_WIDTH]);
        ctx->screen_buffer[offset + SCREEN_WIDTH] = render_compute_alpha(blue, ctx->screen_buffer[offset + SCREEN_WIDTH]);
        ctx->screen_buffer[offset - 2] = render_compute_alpha(white, ctx->screen_buffer[offset - 2]);
        ctx->screen_buffer[offset + 2] = render_compute_alpha(white, ctx->screen_buffer[offset + 2]);
    }

    /* Draw enemies (red) */
    uint32_t red = 0xA0FF0000;
    POOL_FOREACH_CONST(&game->enemies, e, Enemy, {
        int ex = (int)(e->position.x / (MAP_WIDTH / MINIMAP_WIDTH)) + MINIMAP_X;
        int ey = (int)(e->position.y / (MAP_HEIGHT / MINIMAP_HEIGHT)) + MINIMAP_Y;

        if (ex > MINIMAP_X && ex < MINIMAP_X + MINIMAP_WIDTH - 1 &&
            ey > MINIMAP_Y && ey < MINIMAP_Y + MINIMAP_HEIGHT - 1) {
            int offset = ey * SCREEN_WIDTH + ex;
            ctx->screen_buffer[offset] = render_compute_alpha(red, ctx->screen_buffer[offset]);
            ctx->screen_buffer[offset - 1] = render_compute_alpha(red, ctx->screen_buffer[offset - 1]);
            ctx->screen_buffer[offset + 1] = render_compute_alpha(red, ctx->screen_buffer[offset + 1]);

            /* Larger marker for bosses */
            if (e->size == ENEMY_SIZE_BOSS) {
                ctx->screen_buffer[offset - 2] = render_compute_alpha(red, ctx->screen_buffer[offset - 2]);
                ctx->screen_buffer[offset + 2] = render_compute_alpha(red, ctx->screen_buffer[offset + 2]);
            }
        }
    });
}

void render_nuke_icons(RenderContext* ctx, int nukes_remaining) {
    if (!ctx->nuke_icon || nukes_remaining <= 0) return;

    for (int i = 0; i < nukes_remaining; i++) {
        int x = SCREEN_WIDTH - 46 - (i * 34);
        int y = 18;
        render_alpha_blit(ctx, x, y, ctx->nuke_icon, 32, 32);
    }
}

void render_victory_screen(RenderContext* ctx) {
    if (!ctx->victory_screen) return;

    int x = (SCREEN_WIDTH - 460) / 2;
    int y = (SCREEN_HEIGHT - 345) / 2;
    render_alpha_blit(ctx, x, y, ctx->victory_screen, 460, 345);
}

void render_defeat_screen(RenderContext* ctx) {
    if (!ctx->defeat_screen) return;

    int x = (SCREEN_WIDTH - 460) / 2;
    int y = (SCREEN_HEIGHT - 345) / 2;
    render_alpha_blit(ctx, x, y, ctx->defeat_screen, 460, 345);
}

/*============================================================================
 * UTILITY FUNCTIONS
 *============================================================================*/

uint32_t render_compute_alpha(uint32_t src, uint32_t dst) {
    /* Use legacy ComputeAlpha for now - this will be replaced */
    return ComputeAlpha(src, dst);
}

void render_alpha_blit(RenderContext* ctx, int x, int y,
                       const uint32_t* sprite, int width, int height) {
    if (!ctx->screen_buffer || !sprite) return;

    for (int row = 0; row < height; row++) {
        int screen_y = y + row;
        if (screen_y < 0 || screen_y >= SCREEN_HEIGHT) continue;

        for (int col = 0; col < width; col++) {
            int screen_x = x + col;
            if (screen_x < 0 || screen_x >= SCREEN_WIDTH) continue;

            uint32_t src_pixel = sprite[row * width + col];

            /* Skip fully transparent pixels */
            if ((src_pixel >> 24) == 0) continue;

            int offset = screen_y * SCREEN_WIDTH + screen_x;
            ctx->screen_buffer[offset] = render_compute_alpha(src_pixel, ctx->screen_buffer[offset]);
        }
    }
}

/*============================================================================
 * CAMERA
 *============================================================================*/

void camera_update(Camera* cam, const Player* player, int shake_frames, uint32_t rand_state) {
    cam->fx = player->position.x;
    cam->fy = player->position.y;
    cam->x = (int)cam->fx;
    cam->y = (int)cam->fy;

    /* Apply shake */
    if (shake_frames > 0) {
        /* Simple shake using rand_state bits */
        cam->shake_x = ((int)(rand_state & 0x7F) - 64) % SHAKE_INTENSITY;
        cam->shake_y = ((int)((rand_state >> 8) & 0x7F) - 64) % SHAKE_INTENSITY;
    } else {
        cam->shake_x = 0;
        cam->shake_y = 0;
    }
}
