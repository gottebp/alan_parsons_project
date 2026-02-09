/*
 * sprites.c - Sprite Loading and Management
 *
 * Consolidates all sprite loading from legacy player.c, enemy.c, and ppe.c.
 * This is the single source of truth for sprite data.
 */

#include "../../include_c/game/sprites.h"
#include "../../include_c/defs.h"
#include "../../include_c/sdl_wrapper.h"
#include "../../include_c/ppe.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

/*============================================================================
 * SPRITE BUFFERS
 *============================================================================*/

/* Player ship sprites - 256 rotation frames each */
uint32_t* PlayerShipOff = NULL;
uint32_t* PlayerShipOffL = NULL;
uint32_t* PlayerShipOffR = NULL;

/* Enemy sprites */
uint32_t* SmallEnemyImageData = NULL;
uint32_t* BossImageData = NULL;

/* Particle textures (GPU) */
SDL_Texture* SmallParticleTexture = NULL;
SDL_Texture* LargeParticleTexture = NULL;

/* Particle flare source data (CPU) - temporary, freed after texture upload */
static uint32_t* small_flare_data = NULL;
static uint32_t* large_flare_data = NULL;

/*============================================================================
 * FILE PATHS
 *============================================================================*/

static const char* PLAYER_SPRITE_NORMAL = "./data/player_norm.raw";
static const char* PLAYER_SPRITE_LEFT   = "./data/player_left.raw";
static const char* PLAYER_SPRITE_RIGHT  = "./data/player_right.raw";
static const char* SMALL_ENEMY_FILE     = "./data/small_enemies.bmp";
static const char* BOSS_ENEMY_FILE      = "./data/large_enemies.bmp";
static const char* SMALL_FLARE_FILE     = "./data/small_flares.bmp";
static const char* LARGE_FLARE_FILE     = "./data/large_flares.bmp";

/* External SDL renderer from sdl_wrapper.c */
extern SDL_Renderer* screen_renderer;

/*============================================================================
 * RAW FILE LOADING
 * Loads raw pixel data and converts magenta (0xFF00FFFF) to transparent.
 *============================================================================*/

void LoadRaw(const char* filename, void* buffer, size_t size) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "sprites: Failed to open %s\n", filename);
        return;
    }

    size_t read = fread(buffer, 1, size, fp);
    fclose(fp);

    if (read != size) {
        fprintf(stderr, "sprites: Short read from %s (%zu/%zu bytes)\n",
                filename, read, size);
    }

    /* Convert magenta pixels to transparent */
    uint32_t* pixels = (uint32_t*)buffer;
    size_t pixel_count = size / 4;
    for (size_t i = 0; i < pixel_count; i++) {
        if (pixels[i] == 0xFF00FFFF) {
            pixels[i] = 0x00000000;
        }
    }
}

/*============================================================================
 * PLAYER SPRITE LOADING
 *============================================================================*/

static int load_player_sprites(void) {
    size_t ship_size = PLAYER_IMG_FILE_WIDTH * PLAYER_IMG_FILE_HEIGHT * 4;

    /* Allocate sprite buffers */
    PlayerShipOff = (uint32_t*)malloc(ship_size);
    PlayerShipOffL = (uint32_t*)malloc(ship_size);
    PlayerShipOffR = (uint32_t*)malloc(ship_size);

    if (!PlayerShipOff || !PlayerShipOffL || !PlayerShipOffR) {
        fprintf(stderr, "sprites: Failed to allocate player sprite memory\n");
        return -1;
    }

    /* Load sprite data */
    LoadRaw(PLAYER_SPRITE_NORMAL, PlayerShipOff, ship_size);
    LoadRaw(PLAYER_SPRITE_LEFT, PlayerShipOffL, ship_size);
    LoadRaw(PLAYER_SPRITE_RIGHT, PlayerShipOffR, ship_size);

    return 0;
}

static void free_player_sprites(void) {
    free(PlayerShipOff);
    free(PlayerShipOffL);
    free(PlayerShipOffR);
    PlayerShipOff = NULL;
    PlayerShipOffL = NULL;
    PlayerShipOffR = NULL;
}

/*============================================================================
 * ENEMY SPRITE LOADING
 *============================================================================*/

static int load_enemy_sprites(void) {
    /* Allocate small enemy sprites */
    SmallEnemyImageData = (uint32_t*)malloc(SMALLENEMYMEMSIZE);
    if (!SmallEnemyImageData) {
        fprintf(stderr, "sprites: Failed to allocate small enemy memory\n");
        return -1;
    }

    /* Load small enemy BMP */
    if (LoadBMP(SmallEnemyImageData, SMALL_ENEMY_FILE) != 0) {
        fprintf(stderr, "sprites: Failed to load %s\n", SMALL_ENEMY_FILE);
    }

    /* Convert magenta to transparent */
    for (size_t i = 0; i < SMALLENEMYMEMSIZE / 4; i++) {
        if (SmallEnemyImageData[i] == 0xFF00FFFF) {
            SmallEnemyImageData[i] = 0x00000000;
        }
    }

    /* Allocate boss enemy sprites */
    BossImageData = (uint32_t*)malloc(BOSSENEMYMEMSIZE);
    if (!BossImageData) {
        fprintf(stderr, "sprites: Failed to allocate boss enemy memory\n");
        return -1;
    }

    /* Load boss enemy BMP */
    if (LoadBMP(BossImageData, BOSS_ENEMY_FILE) != 0) {
        fprintf(stderr, "sprites: Failed to load %s\n", BOSS_ENEMY_FILE);
    }

    /* Convert magenta to transparent */
    for (size_t i = 0; i < BOSSENEMYMEMSIZE / 4; i++) {
        if (BossImageData[i] == 0xFF00FFFF) {
            BossImageData[i] = 0x00000000;
        }
    }

    return 0;
}

static void free_enemy_sprites(void) {
    free(SmallEnemyImageData);
    free(BossImageData);
    SmallEnemyImageData = NULL;
    BossImageData = NULL;
}

/*============================================================================
 * PARTICLE TEXTURE LOADING
 * Loads flare BMPs, applies alpha conversion, uploads to GPU textures.
 *============================================================================*/

static int load_particle_textures(void) {
    /* Allocate CPU buffers for flare images */
    small_flare_data = (uint32_t*)malloc(SMALL_FLARE_FILE_WIDTH * SMALL_FLARE_FILE_HEIGHT * 4);
    if (!small_flare_data) {
        fprintf(stderr, "sprites: Failed to allocate small flare memory\n");
        return -1;
    }

    large_flare_data = (uint32_t*)malloc(LARGE_FLARE_FILE_WIDTH * LARGE_FLARE_FILE_HEIGHT * 4);
    if (!large_flare_data) {
        fprintf(stderr, "sprites: Failed to allocate large flare memory\n");
        return -1;
    }

    /* Load flare images */
    if (LoadBMP(small_flare_data, SMALL_FLARE_FILE) != 0) {
        fprintf(stderr, "sprites: Failed to load %s\n", SMALL_FLARE_FILE);
    }
    if (LoadBMP(large_flare_data, LARGE_FLARE_FILE) != 0) {
        fprintf(stderr, "sprites: Failed to load %s\n", LARGE_FLARE_FILE);
    }

    /* Apply alpha conversion - uses MakeAlphaFromRGB from sdl_wrapper.c */
    extern uint32_t intPixel;  /* Global used by MakeAlphaFromRGB */

    for (int i = 0; i < SMALL_FLARE_FILE_WIDTH * SMALL_FLARE_FILE_HEIGHT; i++) {
        MakeAlphaFromRGB(small_flare_data[i]);
        small_flare_data[i] = intPixel;
    }

    for (int i = 0; i < LARGE_FLARE_FILE_WIDTH * LARGE_FLARE_FILE_HEIGHT; i++) {
        MakeAlphaFromRGB(large_flare_data[i]);
        large_flare_data[i] = intPixel;
    }

    /* Create GPU textures */
    if (screen_renderer) {
        /* Small particle texture */
        SmallParticleTexture = SDL_CreateTexture(
            screen_renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STATIC,
            SMALL_FLARE_FILE_WIDTH,
            SMALL_FLARE_FILE_HEIGHT
        );
        if (SmallParticleTexture) {
            SDL_UpdateTexture(SmallParticleTexture, NULL, small_flare_data,
                              SMALL_FLARE_FILE_WIDTH * sizeof(uint32_t));
            SDL_SetTextureBlendMode(SmallParticleTexture, SDL_BLENDMODE_BLEND);
        }

        /* Large particle texture */
        LargeParticleTexture = SDL_CreateTexture(
            screen_renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STATIC,
            LARGE_FLARE_FILE_WIDTH,
            LARGE_FLARE_FILE_HEIGHT
        );
        if (LargeParticleTexture) {
            SDL_UpdateTexture(LargeParticleTexture, NULL, large_flare_data,
                              LARGE_FLARE_FILE_WIDTH * sizeof(uint32_t));
            SDL_SetTextureBlendMode(LargeParticleTexture, SDL_BLENDMODE_BLEND);
        }
    }

    /* Free CPU buffers - data is now on GPU */
    free(small_flare_data);
    free(large_flare_data);
    small_flare_data = NULL;
    large_flare_data = NULL;

    return 0;
}

static void free_particle_textures(void) {
    if (SmallParticleTexture) SDL_DestroyTexture(SmallParticleTexture);
    if (LargeParticleTexture) SDL_DestroyTexture(LargeParticleTexture);
    SmallParticleTexture = NULL;
    LargeParticleTexture = NULL;
}

/*============================================================================
 * PUBLIC INTERFACE
 *============================================================================*/

void sprites_init(void) {
    load_player_sprites();
    load_enemy_sprites();
    load_particle_textures();
}

void sprites_destroy(void) {
    free_player_sprites();
    free_enemy_sprites();
    free_particle_textures();
}
