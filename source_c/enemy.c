/*
 * Enemy system - handles enemy spawning, updating, and rendering
 * Converted from x86 assembly - mirrors assembly implementation
 */

#include "enemy.h"
#include "sdl_wrapper.h"
#include "sse_mem.h"
#include "player.h"
#include "rand.h"
#include "ppe.h"
#include "ai.h"
#include "mapeng.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Enemy data */
Enemy Enemies[100];
uint32_t* SmallEnemyImageData = NULL;
uint32_t* BossImageData = NULL;

/* Spawn timing - matches assembly's _SpawnFrames structure */
uint32_t SpawnFrames[18];      /* Frame times and enemy counts */
uint32_t FrameNum = 0;
uint32_t NextWaveNumber = 0;

/* Enemy mass lookup - matches assembly's _EnemyMassLookup */
static const float EnemyMassLookup[4] = {35.0f, 40.0f, 60.0f, 80.0f};

/* Map data structure for each level - matches assembly's _MapData */
typedef struct {
    uint8_t num_waves;
    uint8_t enemies_per_wave;
    uint8_t boss_id;
    uint8_t padding;
    float boss_x;
    float boss_y;
    float boss_mass;
} MapData;

/* Map data array - using proper enemy counts (assembly has testing value of 1 commented) */
static const MapData MAP_DATA_ARRAY[6] = {
    /* SHIRE */      {3, 8, 0, 0, 400.0f, 400.0f, 300.0f},    /* Should be 8 per wave */
    /* ARCHIPELAGO */ {4, 10, 1, 0, 400.0f, 400.0f, 400.0f},
    /* DUNE */       {2, 30, 1, 0, 400.0f, 400.0f, 450.0f},
    /* MIDKEMIA */   {7, 10, 2, 0, 400.0f, 400.0f, 500.0f},
    /* OCEANIA */    {4, 20, 2, 0, 400.0f, 400.0f, 650.0f},
    /* MORDOR */     {4, 24, 3, 0, 400.0f, 400.0f, 800.0f}
};

/* External references */
extern int intPlayerX, intPlayerY;
extern uint8_t game_turnout;  /* Matches assembly _GameTurnout (byte) */
extern float SIN_LOOK[256];
extern float COS_LOOK[256];
extern int MapMidX, MapMidY;

/*
 * Load enemy image data
 */
void LoadEnemyData(void) {
    extern char* SmallEnemyFile;
    extern char* LargeEnemyFile;

    /* Allocate small enemy sprites - mirrors assembly lines 204-209 */
    SmallEnemyImageData = (uint32_t*)malloc(SMALLENEMYMEMSIZE);
    if (!SmallEnemyImageData) {
        printf("\nMemory allocation error occured in _LoadEnemyData\n");
        return;
    }

    /* Load small enemy BMP - mirrors assembly line 211 */
    LoadBMP(SmallEnemyImageData, SmallEnemyFile);

    /* Setup alpha transparency for small enemies - mirrors assembly lines 213-228 */
    for (size_t i = 0; i < SMALLENEMYMEMSIZE / 4; i++) {
        if (SmallEnemyImageData[i] == 0xFF00FFFF) {
            SmallEnemyImageData[i] = 0x00000000;
        }
    }

    /* Allocate boss enemy sprites - mirrors assembly lines 231-236 */
    BossImageData = (uint32_t*)malloc(BOSSENEMYMEMSIZE);
    if (!BossImageData) {
        printf("\nMemory allocation error occured in _LoadEnemyData\n");
        return;
    }

    /* Load boss enemy BMP - mirrors assembly line 238 */
    LoadBMP(BossImageData, LargeEnemyFile);

    /* Setup alpha transparency for boss enemies */
    for (size_t i = 0; i < BOSSENEMYMEMSIZE / 4; i++) {
        if (BossImageData[i] == 0xFF00FFFF) {
            BossImageData[i] = 0x00000000;
        }
    }
}

/*
 * Destroy enemy data - mirrors assembly lines 269-279
 */
void DestroyEnemyData(void) {
    free(SmallEnemyImageData);
    free(BossImageData);
}

/*
 * Initialize enemies for a level - mirrors assembly implementation
 */
void InitializeEnemies(int map_id) {
    /* Clear enemy array - mirrors assembly's sseMemset32 */
    sseMemset32((uint32_t*)Enemies, 0, sizeof(Enemies) / 4);

    /* Clear specific fields - mirrors assembly loop at lines 301-309 */
    for (int i = 0; i < 100; i++) {
        Enemies[i].enemy_type = -1;
        Enemies[i].enemy_active = -1;
        Enemies[i].enemy_health = -1;
    }

    /* Reset frame counters */
    FrameNum = 0;
    NextWaveNumber = 0;

    /* Get map data */
    if (map_id < 0 || map_id >= 6) return;
    const MapData* md = &MAP_DATA_ARRAY[map_id];

    /* Generate wave spawn timing - mirrors assembly lines 314-349 */
    uint32_t cumulative_frame = 0;
    uint32_t cumulative_enemies = 0;

    for (int wave = 0; wave < md->num_waves; wave++) {
        /* Generate random spawn time - mirrors assembly's RAND + bit operations */
        uint32_t rand_val = Rand();
        rand_val &= 0x00000FFF;              /* Matches: and eax, 00000FFFh */
        rand_val >>= RANDSPAWNDIVISOR;       /* Matches: shr eax, RANDSPAWNDIVISOR */
        cumulative_frame += 800 + rand_val;  /* Base 800 + random */

        SpawnFrames[wave * 2] = cumulative_frame;
        cumulative_enemies += md->enemies_per_wave;
        SpawnFrames[wave * 2 + 1] = cumulative_enemies;
    }

    /* Set boss wave marker - mirrors assembly line 348 */
    SpawnFrames[md->num_waves * 2] = (uint32_t)-1;
    SpawnFrames[md->num_waves * 2 + 1] = cumulative_enemies + 1; /* +1 for boss */

    /* Setup spawn timing for waves */
    int enemy_idx = 0;

    /* Generate all small enemies - mirrors assembly lines 352-425 */
    for (int wave = 0; wave < md->num_waves && enemy_idx < 100; wave++) {
        for (int e = 0; e < md->enemies_per_wave && enemy_idx < 100; e++) {
            /* Random enemy type 0-3 - mirrors assembly line 387 */
            uint32_t rand_val = Rand();
            Enemies[enemy_idx].enemy_type = rand_val & 0x00000003;  /* Bitwise AND */

            /* Random spawn position ANYWHERE on map - mirrors assembly lines 363-383 */
            Enemies[enemy_idx].enemy_x_float = (float)(Rand() % MAP_WIDTH);
            Enemies[enemy_idx].enemy_y_float = (float)(Rand() % MAP_HEIGHT);

            /* Use lrintf to match fistp instruction (lines 372, 382) */
            Enemies[enemy_idx].enemy_x_int = lrintf(Enemies[enemy_idx].enemy_x_float);
            Enemies[enemy_idx].enemy_y_int = lrintf(Enemies[enemy_idx].enemy_y_float);

            /* Velocity starts at zero */
            Enemies[enemy_idx].enemy_x_vel_float = 0.0f;
            Enemies[enemy_idx].enemy_y_vel_float = 0.0f;

            /* Mass lookup - FIX for assembly bug at lines 391-394
             * Original ASM: shr eax, 2 which gives 0 for all enemy types 0-3
             * This meant ALL enemies got the same mass (35.0f)
             * FIX: Use enemy_type directly to give different types different masses */
            Enemies[enemy_idx].enemy_mass = EnemyMassLookup[Enemies[enemy_idx].enemy_type];

            /* Health = mass * multiplier - use lrintf to match fistp (line 399) */
            Enemies[enemy_idx].enemy_health = lrintf(Enemies[enemy_idx].enemy_mass * MASS_HEALTH_MULTIPLIER);

            Enemies[enemy_idx].enemy_angle = 0;
            Enemies[enemy_idx].enemy_size = 0; /* Small enemy */

            /* NOT active yet - mirrors assembly line 414 */
            Enemies[enemy_idx].enemy_active = -1;

            enemy_idx++;
        }
    }

    /* Generate boss - mirrors assembly lines 428-465 */
    if (enemy_idx < 100) {
        Enemies[enemy_idx].enemy_type = md->boss_id;

        /* Boss position from map data - mirrors assembly lines 439-447 */
        Enemies[enemy_idx].enemy_x_float = md->boss_x;
        Enemies[enemy_idx].enemy_y_float = md->boss_y;
        Enemies[enemy_idx].enemy_x_int = 400;  /* Hardcoded in assembly (lines 441, 447) */
        Enemies[enemy_idx].enemy_y_int = 400;  /* Hardcoded in assembly */

        Enemies[enemy_idx].enemy_x_vel_float = 0.0f;
        Enemies[enemy_idx].enemy_y_vel_float = 0.0f;
        Enemies[enemy_idx].enemy_mass = md->boss_mass;
        Enemies[enemy_idx].enemy_angle = 0;
        Enemies[enemy_idx].enemy_size = 1; /* Boss/large enemy */

        /* Boss NOT active yet - mirrors assembly line 465 */
        Enemies[enemy_idx].enemy_active = -1;

        /* Health = mass * multiplier - use lrintf to match fistp (line 457) */
        Enemies[enemy_idx].enemy_health = lrintf(md->boss_mass * MASS_HEALTH_MULTIPLIER);

        printf("Boss created: idx=%d, type=%d, mass=%.1f, health=%d\n",
               enemy_idx, Enemies[enemy_idx].enemy_type,
               Enemies[enemy_idx].enemy_mass, Enemies[enemy_idx].enemy_health);
    }

    /* Activate ONLY first wave - mirrors assembly lines 472-478 */
    uint32_t first_wave_count = SpawnFrames[1];
    for (uint32_t i = 0; i < first_wave_count && i < 100; i++) {
        Enemies[i].enemy_active = 1;
    }
}

/*
 * Enemy collision detection - mirrors assembly _EnemyCollisionDetection (lines 759-833)
 * Takes byte offset into Enemies array, NOT a pointer (matches assembly signature)
 */
static void EnemyCollisionDetection(int enemy_byte_offset) {
    if (!ParticleDataOff) return;

    /* Access enemy via byte offset - mirrors assembly line 763: mov edi, [ebp+.EnemyPtr] */
    Enemy* enemy = (Enemy*)((uint8_t*)Enemies + enemy_byte_offset);

    /* Calculate bounding box based on enemy size - mirrors assembly lines 765-785 */
    int left, right, top, bottom;
    int size_offset;

    /* Assembly lines 765-770: size check and offset setup */
    if (enemy->enemy_size == 1) {
        size_offset = 114;  /* Boss size */
    } else {
        size_offset = 50;   /* Small enemy */
    }

    /* Mirrors assembly lines 773-785: calculate bounds */
    left = enemy->enemy_x_int - size_offset;
    right = enemy->enemy_x_int + size_offset;
    top = enemy->enemy_y_int - size_offset;
    bottom = enemy->enemy_y_int + size_offset;

    /* Check all particles - mirrors assembly lines 793-829 */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        PARTICLE* p = &ParticleDataOff[i];

        /* Skip inactive particles or non-collision particles - mirrors lines 795-798 */
        /* Assembly checks IsActive == 1, not != 0 */
        if (p->IsActive != 1) continue;
        if (p->DetectCollisions != 1) continue;

        /* Check bounding box collision - mirrors assembly lines 803-810 */
        if (p->intX < left) continue;
        if (p->intX > right) continue;
        if (p->intY < top) continue;
        if (p->intY > bottom) continue;

        /* Collision detected! - mirrors assembly lines 815-822 */
        p->IsActive = 0;

        /* Subtract damage from enemy health - mirrors assembly line 817 */
        enemy->enemy_health -= p->Damage;

        /* Create explosion effect - mirrors assembly line 818 */
        ShipExplode(p->fltX, p->fltY, 0, 1);

        /* Check if enemy died - mirrors assembly line 821-822 */
        if (enemy->enemy_health <= 0) {
            return; /* Enemy died, exit early */
        }
    }
}

/*
 * Update all active enemies - mirrors assembly implementation
 */
void UpdateEnemies(void) {
    /* AI movement for all enemies FIRST - mirrors assembly line 492 */
    EnemyMove();

    /* Increment frame counter - mirrors assembly line 494 */
    FrameNum++;

    /* Determine how many enemies to check - mirrors assembly lines 497-499 */
    int max_enemies = 0;
    if (NextWaveNumber < 18) {
        max_enemies = SpawnFrames[NextWaveNumber * 2 + 1];
    }
    if (max_enemies > 100) max_enemies = 100;

    /* Update each active enemy - mirrors assembly lines 487-647 */
    for (int i = 0; i < max_enemies; i++) {
        if (Enemies[i].enemy_active == -1) continue;

        /* Collision detection - mirrors assembly line 510 */
        /* Assembly passes byte offset (esi = 0, 40, 80...), not pointer */
        EnemyCollisionDetection(i * sizeof(Enemy));

        /* Check if enemy died - mirrors assembly lines 511-539 */
        /* Assembly uses jge (signed comparison), so enemy dies when health < 0 */
        if (Enemies[i].enemy_health < 0) {
            /* Create explosion - mirrors assembly lines 513-515 */
            /* Assembly passes enemy_size directly (0 or 1), not calculated value */
            ShipExplode(Enemies[i].enemy_x_float, Enemies[i].enemy_y_float, Enemies[i].enemy_size, 0);

            /* NOTE: Explosion sound effect is commented out in assembly (lines 517-527) */
            /* Assembly has: ;invoke _AddSound, dword [_SNDExplodeEnemy], dword 35536, dword 00h, byte 12 */
            /* This was never enabled in the original, so we match that behavior */

            /* Deactivate enemy - mirrors assembly line 528 */
            Enemies[i].enemy_active = -1;

            /* Check if boss died - mirrors assembly lines 530-536 */
            if (Enemies[i].enemy_size == 1) {
                /* Boss died! Set win condition if player not already dead */
                if (game_turnout != PLAYER_DEAD) {
                    game_turnout = PLAYER_WIN;
                }
            }
        }

        /* Update position - mirrors assembly lines 544-559 */
        /* Use lrintf to match fistp instruction for proper rounding */
        Enemies[i].enemy_x_float += Enemies[i].enemy_x_vel_float;
        Enemies[i].enemy_y_float += Enemies[i].enemy_y_vel_float;
        Enemies[i].enemy_x_int = lrintf(Enemies[i].enemy_x_float);  /* Matches fistp (line 549) */
        Enemies[i].enemy_y_int = lrintf(Enemies[i].enemy_y_float);  /* Matches fistp (line 556) */

        /* Map wrapping - mirrors assembly lines 561-606 */
        if (Enemies[i].enemy_x_int >= MAP_WIDTH) {
            Enemies[i].enemy_x_float -= MAP_WIDTH;
            Enemies[i].enemy_x_int = lrintf(Enemies[i].enemy_x_float);  /* Matches fistp (line 569) */
        }
        if (Enemies[i].enemy_x_int <= 0) {
            Enemies[i].enemy_x_float += MAP_WIDTH;
            Enemies[i].enemy_x_int = lrintf(Enemies[i].enemy_x_float);  /* Matches fistp (line 581) */
        }
        if (Enemies[i].enemy_y_int >= MAP_HEIGHT) {
            Enemies[i].enemy_y_float -= MAP_HEIGHT;
            Enemies[i].enemy_y_int = lrintf(Enemies[i].enemy_y_float);  /* Matches fistp (line 594) */
        }
        if (Enemies[i].enemy_y_int <= 0) {
            Enemies[i].enemy_y_float += MAP_HEIGHT;
            Enemies[i].enemy_y_int = lrintf(Enemies[i].enemy_y_float);  /* Matches fistp (line 606) */
        }
    }

    /* Wave spawning logic AFTER update loop - mirrors assembly lines 616-647 */
    if (NextWaveNumber < 18) {
        uint32_t next_spawn_frame = SpawnFrames[NextWaveNumber * 2];

        /* Check if it's time to spawn next wave */
        if (next_spawn_frame != (uint32_t)-1 && FrameNum >= next_spawn_frame) {
            /* Increment wave number first */
            NextWaveNumber++;

            /* Check if this is boss wave or normal wave - mirrors assembly lines 629-647 */
            if (NextWaveNumber < 18 && SpawnFrames[NextWaveNumber * 2] == (uint32_t)-1) {
                /* Boss wave! Activate boss - mirrors assembly lines 643-646 */
                /* Assembly does NOT increment NextWaveNumber again after boss activation */
                uint32_t boss_idx = SpawnFrames[NextWaveNumber * 2 + 1] - 1;
                if (boss_idx < 100) {
                    Enemies[boss_idx].enemy_active = 1;
                    printf("BOSS ACTIVATED! idx=%d, type=%d, size=%d\n",
                           boss_idx, Enemies[boss_idx].enemy_type, Enemies[boss_idx].enemy_size);
                    /* Play evil laugh - mirrors assembly line 645 */
                    extern Mix_Chunk* snd_effect_evil_laugh;
                    if (snd_effect_evil_laugh) {
                        Mix_PlayChannel(-1, snd_effect_evil_laugh, 0);
                    }
                    /* Trigger screen shake - mirrors assembly line 646 */
                    ShakeMap(150);
                }
                /* NO second increment - assembly jumps directly to .NotNewWave (line 647) */
            } else {
                /* Normal wave - activate enemies - mirrors assembly lines 633-640 */
                uint32_t prev_count = SpawnFrames[(NextWaveNumber - 1) * 2 + 1];
                uint32_t curr_count = SpawnFrames[NextWaveNumber * 2 + 1];

                for (uint32_t i = prev_count; i < curr_count && i < 100; i++) {
                    Enemies[i].enemy_active = 1;
                }
            }
        }
    }
}

/*
 * Render all active enemies - mirrors assembly implementation
 */
void RenderEnemies(void) {
    /* Determine how many enemies to render - mirrors assembly lines 666-668 */
    int max_enemies = 0;
    if (NextWaveNumber < 18) {
        max_enemies = SpawnFrames[NextWaveNumber * 2 + 1];
    }
    if (max_enemies > 100) max_enemies = 100;

    for (int i = 0; i < max_enemies; i++) {
        if (Enemies[i].enemy_active == -1) continue;

        /* Calculate screen position relative to player - mirrors assembly lines 690-699 */
        int screen_x = Enemies[i].enemy_x_int - intPlayerX + (SCREEN_WIDTH / 2);
        int screen_y = Enemies[i].enemy_y_int - intPlayerY + (SCREEN_HEIGHT / 2);

        /* Get enemy sprite */
        uint32_t* sprite;
        int width, height;

        if (Enemies[i].enemy_size == 0) {
            /* Small enemy - 128x128 */
            sprite = SmallEnemyImageData;
            width = 128;
            height = 128;

            /* Calculate sprite offset - mirrors assembly lines 675-683 */
            int type_offset = Enemies[i].enemy_type << (4 + ENEMYSHIFTSIZE);  /* type * 16 * bytes_per_frame */
            int angle_frame = Enemies[i].enemy_angle >> 4;                     /* angle / 16 */
            int angle_offset = angle_frame << ENEMYSHIFTSIZE;                  /* frame * bytes_per_frame */

            sprite += (type_offset + angle_offset) / 4;  /* Divide by 4 for uint32_t pointer arithmetic */

            /* Center sprite - mirrors assembly lines 722-723 */
            screen_x -= 64;
            screen_y -= 64;

            /* Map wrapping - mirrors assembly lines 704-720 */
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

        } else {
            /* Boss enemy - 256x256 */
            sprite = BossImageData;
            width = 256;
            height = 256;

            /* Calculate sprite offset - mirrors assembly lines 675-683, then 734 */
            /* Assembly calculates offset same as small enemy, THEN multiplies by 4 */
            int type_offset = Enemies[i].enemy_type << (4 + ENEMYSHIFTSIZE);
            int angle_frame = Enemies[i].enemy_angle >> 4;
            int angle_offset = angle_frame << ENEMYSHIFTSIZE;

            /* Assembly: shl ebx, 2 - multiply offset by 4 for boss sprites */
            /* In C with uint32_t*: offset_bytes * 4 / 4 = offset_bytes (no division needed) */
            sprite += (type_offset + angle_offset);

            /* Center sprite - double offset for boss - mirrors assembly lines 737-738 */
            screen_x -= 128;
            screen_y -= 128;

            /* Map wrapping for boss */
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
        }

        /* Render enemy */
        AlphaBlit(screen_x, screen_y, sprite, width, height);
    }
}
