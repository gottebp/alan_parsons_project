/*
 * demo_new_arch.c - Demonstration of the New Architecture
 *
 * This is a standalone demo showing the clean architecture in action.
 * It runs a headless simulation proving the new modules work together.
 *
 * Build: gcc -std=c99 -I../include_c -o demo_new_arch demo_new_arch.c \
 *        game/game.c game/weapons.c -lm
 *
 * Run: ./demo_new_arch
 */

#include "../include_c/game/game.h"
#include "../include_c/game/audio.h"
#include "../include_c/core/constants.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/*============================================================================
 * MOCK INPUT - Simulates player input for the demo
 *============================================================================*/

static void generate_ai_input(InputState* input, const Game* game, int frame) {
    memset(input, 0, sizeof(InputState));

    /* Simple AI: always thrust forward, fire weapons, occasionally turn */
    input->up = 1;
    input->fire = 1;

    /* Turn toward nearest enemy */
    const Player* p = &game->player;
    float min_dist = 999999.0f;
    uint8_t target_angle = p->angle;

    POOL_FOREACH_CONST(&game->enemies, e, Enemy, {
        Vec2 delta = vec2_toroidal_delta(p->position, e->position, MAP_WIDTH, MAP_HEIGHT);
        float dist = vec2_length(delta);
        if (dist < min_dist) {
            min_dist = dist;
            target_angle = vec2_to_angle(delta);
        }
    });

    /* Turn toward target */
    int8_t angle_diff = (int8_t)(target_angle - p->angle);
    if (angle_diff > 2) {
        input->right = 1;
    } else if (angle_diff < -2) {
        input->left = 1;
    }

    /* Drop nuke occasionally when overwhelmed */
    if (frame % 300 == 0 && game->enemies.count > 10) {
        input->nuke = 1;
    }
}

/*============================================================================
 * DEMO MAIN
 *============================================================================*/

int main(void) {
    printf("=====================================\n");
    printf("  New Architecture Demo\n");
    printf("=====================================\n\n");

    /* Initialize game */
    Game game;
    game_init(&game);
    game_rand_seed(&game, (uint32_t)time(NULL));

    printf("Game initialized:\n");
    printf("  State: %d\n", game.state);
    printf("  Player health: %d\n", game.player.health);
    printf("  Max enemies: %d\n", MAX_ENEMIES);
    printf("  Max particles: %d\n", MAX_PARTICLES);
    printf("\n");

    /* Start level 0 (Shire) */
    printf("Starting level 0 (Shire)...\n");
    game_start_level(&game, 0);

    printf("Level started:\n");
    printf("  Enemies spawned: %d\n", game.enemies.count);
    printf("  Waves configured: %d\n", game.waves.wave_count);
    printf("  Boss index: %d\n", game.waves.boss_index);
    printf("\n");

    /* Run simulation */
    int max_frames = 6000;  /* ~100 seconds at 60fps */
    InputState input;
    float dt = 1.0f / 60.0f;

    printf("Running simulation for %d frames...\n", max_frames);

    clock_t start = clock();

    int enemies_killed = 0;
    int initial_enemies = game.enemies.count;

    for (int frame = 0; frame < max_frames; frame++) {
        /* Generate AI input */
        generate_ai_input(&input, &game, frame);

        /* Update game */
        game_update(&game, &input, dt);

        /* Track kills */
        int current_enemies = game.enemies.count;
        if (current_enemies < initial_enemies) {
            enemies_killed += (initial_enemies - current_enemies);
            initial_enemies = current_enemies;
        }

        /* Check for victory/defeat */
        if (game.state == STATE_VICTORY) {
            printf("\nVICTORY at frame %d!\n", frame);
            printf("  Enemies killed: %d\n", enemies_killed);
            break;
        }
        if (game.state == STATE_DEFEAT) {
            printf("\nDEFEAT at frame %d.\n", frame);
            printf("  Enemies killed: %d\n", enemies_killed);
            break;
        }

        /* Progress updates */
        if (frame % 600 == 0) {
            printf("  Frame %d: Health=%d, Enemies=%d, Particles=%d\n",
                   frame, game.player.health, game.enemies.count, game.particles.count);
        }
    }

    clock_t end = clock();
    double elapsed_ms = (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;

    printf("\nSimulation complete:\n");
    printf("  Final state: %s\n",
           game.state == STATE_VICTORY ? "VICTORY" :
           game.state == STATE_DEFEAT ? "DEFEAT" : "PLAYING");
    printf("  Player health: %d\n", game.player.health);
    printf("  Active enemies: %d\n", game.enemies.count);
    printf("  Active particles: %d\n", game.particles.count);
    printf("  Elapsed time: %.2f ms\n", elapsed_ms);
    printf("  Performance: %.1fx realtime\n",
           (max_frames * 16.67) / elapsed_ms);
    printf("\n");

    /* Test audio module (no actual sound, just data structures) */
    printf("Testing audio module...\n");
    AudioContext audio = {0};
    printf("  Sound IDs valid: %s\n",
           (SOUND_WEAPON_FIRE < SOUND_COUNT &&
            MUSIC_MORDOR < SOUND_COUNT) ? "YES" : "NO");
    printf("  Level music mapping:\n");
    for (int i = 0; i < 6; i++) {
        printf("    Level %d -> Music %d\n", i, MUSIC_SHIRE + i);
    }
    (void)audio;  /* Suppress unused warning */
    printf("\n");

    printf("=====================================\n");
    printf("  Demo Complete\n");
    printf("=====================================\n");

    return 0;
}
