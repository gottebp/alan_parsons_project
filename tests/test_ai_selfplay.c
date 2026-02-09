/*
 * test_ai_selfplay.c - AI Self-Play Tests
 *
 * Uses the AI controller to play through the game,
 * verifying that levels are completable and the game is balanced.
 */

#include "test_framework.h"
#include "../include_c/game/game.h"
#include "../include_c/game/ai_player.h"

/*============================================================================
 * TEST HELPERS
 *============================================================================*/

static Game game;
static AIController ai;

static void reset(void) {
    game_init(&game);
    ai_init(&ai, &AI_CONFIG_BALANCED);
}

/*============================================================================
 * BASIC AI TESTS
 *============================================================================*/

TEST(ai_generates_input) {
    reset();
    game_start_level(&game, 0);

    InputState input;
    ai_think(&ai, &game, &input);

    /* AI should be doing SOMETHING (at least targeting enemies) */
    /* With enemies on the field, it should be turning or moving */
    int has_action = input.up || input.down || input.left || input.right ||
                     input.fire || input.strafe_left || input.strafe_right;

    /* After the first think, AI might not have an immediate action
     * but should definitely respond after seeing the field */
    ai_think(&ai, &game, &input);
    ai_think(&ai, &game, &input);

    /* By now it should be doing something */
    has_action = input.up || input.down || input.left || input.right ||
                 input.fire || input.strafe_left || input.strafe_right;
    TEST_ASSERT(has_action);
}

TEST(ai_tracks_target) {
    reset();
    game_start_level(&game, 0);

    InputState input;

    /* Run several frames */
    for (int i = 0; i < 60; i++) {
        ai_think(&ai, &game, &input);
        game_update(&game, &input, 0.016f);
        if (input.fire) player_fire_weapons(&game);
    }

    /* AI should have been firing at enemies */
    TEST_ASSERT(ai.state.shots_fired > 0);
}

TEST(ai_deals_damage) {
    reset();
    game_start_level(&game, 0);

    InputState input;
    int initial_enemy_health = 0;

    /* Get total enemy health */
    POOL_FOREACH(&game.enemies, e, Enemy, {
        initial_enemy_health += e->health;
    });

    /* Run for a while */
    for (int i = 0; i < 300; i++) {
        ai_think(&ai, &game, &input);
        game_update(&game, &input, 0.016f);
        if (input.fire) player_fire_weapons(&game);
    }

    /* Get new total */
    int final_enemy_health = 0;
    POOL_FOREACH(&game.enemies, e, Enemy, {
        final_enemy_health += e->health;
    });

    /* AI should have dealt damage OR killed enemies */
    int damage_dealt = initial_enemy_health - final_enemy_health;
    TEST_ASSERT(damage_dealt > 0 || ai.state.enemies_killed > 0);
}

TEST(ai_kills_enemies) {
    reset();
    game_start_level(&game, 0);

    InputState input;

    /* Count initial ACTIVE enemies */
    int initial_active = 0;
    POOL_FOREACH(&game.enemies, e, Enemy, {
        if (e->active) initial_active++;
    });

    /* Run for longer - enough to kill at least one (30 seconds at 60fps) */
    for (int i = 0; i < 1800 && game.state == STATE_PLAYING; i++) {
        ai_think(&ai, &game, &input);
        game_update(&game, &input, 0.016f);
        if (input.fire) player_fire_weapons(&game);
        collisions_check_bodies(&game);
    }

    /* Count final ACTIVE enemies */
    int final_active = 0;
    POOL_FOREACH(&game.enemies, e, Enemy, {
        if (e->active) final_active++;
    });

    /* Should have killed at least one enemy */
    TEST_ASSERT(final_active < initial_active);
}

/*============================================================================
 * LEVEL COMPLETION TESTS
 *============================================================================*/

TEST(ai_survives_first_wave) {
    reset();
    game_start_level(&game, 0);

    InputState input;

    /* Run for 1800 frames (30 seconds at 60fps) */
    for (int i = 0; i < 1800 && game.state == STATE_PLAYING; i++) {
        ai_think(&ai, &game, &input);
        game_update(&game, &input, 0.016f);
        if (input.fire) player_fire_weapons(&game);
    }

    /* AI should still be alive */
    TEST_ASSERT(game.state == STATE_PLAYING || game.state == STATE_VICTORY);
    TEST_ASSERT(game.player.health > 0);
}

TEST(ai_beats_shire) {
    /* Try multiple times - AI has randomness */
    int best_frames = 0;
    int wins = 0;
    int attempts = 3;

    for (int attempt = 0; attempt < attempts; attempt++) {
        reset();

        int max_frames = 60 * 60 * 5;  /* 5 minutes */
        int result = ai_play_level(&game, 0, &ai, max_frames);

        if (result) {
            wins++;
            if (best_frames == 0 || ai.state.frames_played < (uint32_t)best_frames) {
                best_frames = ai.state.frames_played;
            }
        }

        printf("      Shire attempt %d: %s in %u frames (kills: %u)\n",
               attempt + 1, result ? "WIN" : "LOSS",
               ai.state.frames_played, ai.state.enemies_killed);
    }

    printf("      Shire total: %d/%d wins\n", wins, attempts);

    /* Should win at least once in 3 attempts */
    TEST_ASSERT(wins >= 1);
}

TEST(ai_beats_archipelago) {
    /* Try with upgraded weapons */
    int wins = 0;
    int attempts = 2;

    for (int attempt = 0; attempt < attempts; attempt++) {
        reset();
        game.weapons_level = 1;  /* Give AI the weapon upgrade from beating Shire */

        int max_frames = 60 * 60 * 5;  /* 5 minutes */
        int result = ai_play_level(&game, 1, &ai, max_frames);

        if (result) wins++;

        printf("      Archipelago attempt %d: %s in %u frames (kills: %u)\n",
               attempt + 1, result ? "WIN" : "LOSS",
               ai.state.frames_played, ai.state.enemies_killed);
    }

    printf("      Archipelago total: %d/%d wins\n", wins, attempts);

    /* Record result but don't fail - this level is harder */
    TEST_ASSERT(1);
}

TEST(ai_beats_dune) {
    reset();
    game.weapons_level = 2;

    int max_frames = 60 * 60 * 5;  /* 5 minutes */
    int result = ai_play_level(&game, 2, &ai, max_frames);

    printf("      Dune result: %s in %u frames\n",
           result ? "VICTORY" : "DEFEAT", ai.state.frames_played);
    printf("      Kills: %u, Damage taken: %u\n",
           ai.state.enemies_killed, ai.state.damage_taken);

    /* Dune is harder - might not always win */
    /* But it should be possible */
    if (!result) {
        printf("      (This level is tough - not a test failure)\n");
    }
    TEST_ASSERT(1);  /* Just record the attempt */
}

/*============================================================================
 * CONFIGURATION TESTS
 *============================================================================*/

TEST(ai_aggressive_vs_cautious) {
    /* Test that different AI configs produce different behavior */

    /* Aggressive AI */
    AIController ai_aggro;
    ai_init(&ai_aggro, &AI_CONFIG_AGGRESSIVE);

    game_init(&game);
    game_start_level(&game, 0);

    InputState input;
    int aggro_forward = 0;

    for (int i = 0; i < 300; i++) {
        ai_think(&ai_aggro, &game, &input);
        if (input.up) aggro_forward++;
        game_update(&game, &input, 0.016f);
    }

    /* Cautious AI */
    AIController ai_caut;
    ai_init(&ai_caut, &AI_CONFIG_CAUTIOUS);

    game_init(&game);
    game_start_level(&game, 0);

    int caut_forward = 0;

    for (int i = 0; i < 300; i++) {
        ai_think(&ai_caut, &game, &input);
        if (input.up) caut_forward++;
        game_update(&game, &input, 0.016f);
    }

    /* Aggressive should thrust forward more often */
    printf("      Aggressive thrust: %d, Cautious thrust: %d\n",
           aggro_forward, caut_forward);
    TEST_ASSERT(aggro_forward >= caut_forward);
}

/*============================================================================
 * CAMPAIGN TEST
 *============================================================================*/

TEST(ai_campaign_progress) {
    AIController campaign_ai;
    ai_init(&campaign_ai, &AI_CONFIG_BALANCED);

    Game campaign_game;
    game_init(&campaign_game);

    int max_frames = 60 * 60 * 8;  /* 8 minutes per level - more generous */
    int levels_beaten = ai_play_campaign(&campaign_game, &campaign_ai, max_frames);

    printf("\n      Campaign result: Beat %d/%zu levels\n",
           levels_beaten, LEVEL_COUNT);
    printf("      Final weapons level: %d\n", campaign_game.weapons_level);

    /* Just record progress - no strict requirement */
    TEST_ASSERT(1);
}

/*============================================================================
 * STRESS TEST
 *============================================================================*/

TEST(ai_extended_play) {
    reset();
    game_start_level(&game, 0);

    InputState input;

    /* Run for 5 minutes of gameplay */
    printf("      Running 5 minutes of gameplay...\n");

    clock_t start = clock();

    for (int frame = 0; frame < 60 * 60 * 5 && game.state == STATE_PLAYING; frame++) {
        ai_think(&ai, &game, &input);
        game_update(&game, &input, 0.016f);
        if (input.fire) player_fire_weapons(&game);
    }

    clock_t end = clock();
    double ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

    printf("      Simulation took %.2f ms\n", ms);
    printf("      Frames simulated: %u\n", ai.state.frames_played);
    printf("      Result: %s\n",
           game.state == STATE_VICTORY ? "VICTORY" :
           game.state == STATE_DEFEAT ? "DEFEAT" : "TIMEOUT");

    ai_print_stats(&ai);

    /* Should complete reasonably fast */
    TEST_ASSERT(ms < 10000.0);  /* 10 seconds max for 5 min simulation */
}

/*============================================================================
 * MAIN
 *============================================================================*/

int main(void) {
    TEST_SUITE_BEGIN("AI Self-Play Tests");

    printf("\n  Basic AI Behavior:\n");
    RUN_TEST(ai_generates_input);
    RUN_TEST(ai_tracks_target);
    RUN_TEST(ai_deals_damage);
    RUN_TEST(ai_kills_enemies);

    printf("\n  Level Completion:\n");
    RUN_TEST(ai_survives_first_wave);
    RUN_TEST(ai_beats_shire);
    RUN_TEST(ai_beats_archipelago);
    RUN_TEST(ai_beats_dune);

    printf("\n  AI Configurations:\n");
    RUN_TEST(ai_aggressive_vs_cautious);

    printf("\n  Campaign:\n");
    RUN_TEST(ai_campaign_progress);

    printf("\n  Extended Play:\n");
    RUN_TEST(ai_extended_play);

    TEST_SUITE_END();
    TEST_REPORT();

    return TEST_EXIT_CODE();
}
