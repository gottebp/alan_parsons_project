/*
 * test_playthrough.c - Automated game playthrough test
 *
 * This test simulates playing through levels using the new architecture,
 * verifying that all systems work together correctly.
 */

#include "test_framework.h"
#include "../include_c/game/game.h"

/*============================================================================
 * SIMULATION HELPERS
 *============================================================================*/

static Game game;

static void reset_game(void) {
    game_init(&game);
}

static void start_level(int level_idx) {
    game_start_level(&game, level_idx);
}

/* Simulate one frame of gameplay */
static void simulate_frame(float dt, int thrust, int turn, int fire) {
    /* Apply player input */
    if (thrust > 0) {
        game.player.speed += PLAYER_ACCELERATION * dt * 60.0f;
        if (game.player.speed > PLAYER_MAX_SPEED) {
            game.player.speed = PLAYER_MAX_SPEED;
        }
    }

    if (turn != 0) {
        game.player.angle += turn * 2;  /* Simple turn */
    }

    /* Update player physics */
    player_physics(&game.player, dt);

    /* Update particles */
    particles_update(&game, dt);

    /* Update waves (spawn enemies over time) */
    waves_update(&game);

    /* Update enemies */
    enemies_update(&game, dt);

    /* Check collisions */
    collisions_check(&game);
    collisions_check_bodies(&game);

    /* Fire weapons */
    if (fire) {
        player_fire_weapons(&game);
    }

    game.frame_count++;
}

/* Simulate multiple frames */
static void simulate_frames(int count, int thrust, int turn, int fire) {
    for (int i = 0; i < count; i++) {
        simulate_frame(0.016f, thrust, turn, fire);
    }
}

/*============================================================================
 * PLAYTHROUGH TESTS
 *============================================================================*/

TEST(playthrough_level_init) {
    reset_game();
    start_level(0);  /* Shire */

    /* Player should be in center */
    TEST_ASSERT(game.player.position.x > 1500 && game.player.position.x < 1700);
    TEST_ASSERT(game.player.position.y > 1100 && game.player.position.y < 1300);

    /* Should have full health */
    TEST_ASSERT_EQ(PLAYER_MAX_HEALTH, game.player.health);

    /* Should be in playing state */
    TEST_ASSERT_EQ(STATE_PLAYING, game.state);

    /* First wave should be active */
    int active_enemies = enemy_pool_count(&game.enemies);
    TEST_ASSERT(active_enemies >= 16);  /* Shire wave 1 has 16 enemies */
}

TEST(playthrough_player_moves) {
    reset_game();
    start_level(0);

    Vec2 start_pos = game.player.position;

    /* Thrust forward for 60 frames (1 second) */
    simulate_frames(60, 1, 0, 0);

    /* Player should have moved */
    float dist = vec2_distance(start_pos, game.player.position);
    TEST_ASSERT(dist > 50.0f);  /* Should have moved significantly */
}

TEST(playthrough_player_turns) {
    reset_game();
    start_level(0);

    uint8_t start_angle = game.player.angle;

    /* Turn right for 30 frames */
    simulate_frames(30, 0, 1, 0);

    /* Angle should have changed */
    TEST_ASSERT(game.player.angle != start_angle);
}

TEST(playthrough_weapons_fire) {
    reset_game();
    start_level(0);

    int particles_before = particle_pool_count(&game.particles);

    /* Fire weapons for a few frames */
    simulate_frames(10, 0, 0, 1);

    int particles_after = particle_pool_count(&game.particles);

    /* Should have spawned projectiles */
    TEST_ASSERT(particles_after > particles_before);
}

TEST(playthrough_enemies_move) {
    reset_game();
    start_level(0);

    /* Record initial enemy positions */
    Vec2 initial_positions[16];
    int idx = 0;
    POOL_FOREACH(&game.enemies, e, Enemy, {
        if (idx < 16) {
            initial_positions[idx] = e->position;
            idx++;
        }
    });

    /* Simulate 120 frames (2 seconds) */
    simulate_frames(120, 0, 0, 0);

    /* Check that enemies have moved (gravitational AI) */
    int moved_count = 0;
    idx = 0;
    POOL_FOREACH(&game.enemies, e, Enemy, {
        if (idx < 16) {
            float dist = vec2_distance(initial_positions[idx], e->position);
            if (dist > 10.0f) moved_count++;
            idx++;
        }
    });

    /* At least half should have moved significantly */
    TEST_ASSERT(moved_count >= 8);
}

TEST(playthrough_particles_age) {
    reset_game();
    start_level(0);

    /* Spawn some particles via weapon fire */
    simulate_frames(5, 0, 0, 1);

    int initial_count = particle_pool_count(&game.particles);
    TEST_ASSERT(initial_count > 0);

    /* Let them age out (most projectiles have max_age around 30-60 frames) */
    simulate_frames(100, 0, 0, 0);

    int final_count = particle_pool_count(&game.particles);

    /* Particles should have expired */
    TEST_ASSERT(final_count < initial_count);
}

TEST(playthrough_wave_progression) {
    reset_game();
    start_level(0);

    /* Initial wave - after init, current_wave is 1 (meaning wave 0 is active, wave 1 is next) */
    int initial_wave = game.waves.current_wave;

    /* Simulate enough time for next wave
     * Wave 1 trigger = wave 0 trigger + delay
     * Each delay is 800-1823 frames, so wave 1 can trigger at 1600-3646 frames
     * Use 4000 frames to ensure wave 1 has spawned */
    simulate_frames(4000, 0, 0, 0);

    /* Should have progressed to next wave */
    TEST_ASSERT(game.waves.current_wave > initial_wave);
}

TEST(playthrough_combat_damage) {
    reset_game();
    start_level(0);

    /* Get initial enemy health */
    Enemy* target = NULL;
    POOL_FOREACH(&game.enemies, e, Enemy, {
        target = e;
        break;
    });

    TEST_ASSERT_NOT_NULL(target);
    int initial_health = target->health;

    /* Move player near enemy and fire */
    game.player.position = vec2_add(target->position, vec2(-100, 0));
    game.player.angle = 0;  /* Face right toward enemy */

    /* Fire at enemy */
    for (int i = 0; i < 60 && target->active && target->health == initial_health; i++) {
        simulate_frame(0.016f, 0, 0, 1);
    }

    /* Enemy should have taken damage or died */
    if (target->active) {
        TEST_ASSERT(target->health < initial_health);
    } else {
        /* Enemy died - that's even better */
        TEST_ASSERT(1);
    }
}

TEST(playthrough_player_damage) {
    reset_game();
    start_level(0);

    int initial_health = game.player.health;

    /* Deal damage to player */
    player_take_damage(&game, 100, game.player.position);

    TEST_ASSERT_EQ(initial_health - 100, game.player.health);
}

TEST(playthrough_kill_enemy) {
    reset_game();
    start_level(0);

    int initial_count = enemy_pool_count(&game.enemies);

    /* Kill an enemy */
    Enemy* target = NULL;
    POOL_FOREACH(&game.enemies, e, Enemy, {
        target = e;
        break;
    });

    TEST_ASSERT_NOT_NULL(target);
    enemy_damage(&game, target, 10000, target->position);

    int final_count = enemy_pool_count(&game.enemies);

    /* One less enemy */
    TEST_ASSERT_EQ(initial_count - 1, final_count);
}

/*============================================================================
 * BODY COLLISION TESTS
 *============================================================================*/

TEST(body_collision_damages_both) {
    reset_game();
    start_level(0);

    /* Find an enemy */
    Enemy* target = NULL;
    POOL_FOREACH(&game.enemies, e, Enemy, {
        target = e;
        break;
    });
    TEST_ASSERT_NOT_NULL(target);

    int player_initial = game.player.health;
    int enemy_initial = target->health;

    /* Position player directly on top of enemy with high velocity */
    game.player.position = target->position;
    game.player.speed = PLAYER_MAX_SPEED;  /* Moving fast */
    game.player.angle = 0;
    game.player.invuln_frames = 0;
    target->invuln_frames = 0;

    /* Run one collision check */
    collisions_check_bodies(&game);

    /* Both should have taken damage */
    TEST_ASSERT(game.player.health < player_initial);
    if (target->active) {
        TEST_ASSERT(target->health < enemy_initial);
    }
}

TEST(body_collision_knockback) {
    reset_game();
    start_level(0);

    /* Spawn an enemy at known location */
    Vec2 enemy_pos = vec2(1600, 1200);
    Enemy* target = enemy_spawn(&game, ENEMY_STANDARD, ENEMY_SIZE_SMALL, enemy_pos, 50.0f);
    TEST_ASSERT_NOT_NULL(target);
    target->velocity = vec2_zero();  /* Start stationary */

    /* Position player overlapping with high velocity toward enemy */
    game.player.position = vec2(enemy_pos.x - 30, enemy_pos.y);  /* Slightly left */
    game.player.speed = PLAYER_MAX_SPEED;
    game.player.angle = 0;  /* Facing right, toward enemy */
    game.player.invuln_frames = 0;
    target->invuln_frames = 0;

    float player_speed_before = game.player.speed;

    /* Run collision */
    collisions_check_bodies(&game);

    /* Player speed should decrease (knocked back) */
    TEST_ASSERT(game.player.speed < player_speed_before);

    /* Enemy should have gained velocity (knocked away) */
    if (target->active) {
        float enemy_speed_after = vec2_length(target->velocity);
        TEST_ASSERT(enemy_speed_after > 0.0f);  /* Was stationary, now moving */
    }
}

TEST(body_collision_invulnerability) {
    reset_game();
    start_level(0);

    /* Find an enemy */
    Enemy* target = NULL;
    POOL_FOREACH(&game.enemies, e, Enemy, {
        target = e;
        break;
    });
    TEST_ASSERT_NOT_NULL(target);

    /* Position player on enemy */
    game.player.position = target->position;
    game.player.speed = PLAYER_MAX_SPEED;
    game.player.invuln_frames = 0;
    target->invuln_frames = 0;

    int health_after_first = game.player.health;
    collisions_check_bodies(&game);
    health_after_first = game.player.health;

    /* Both should now have invulnerability frames */
    TEST_ASSERT(game.player.invuln_frames > 0);
    if (target->active) {
        TEST_ASSERT(target->invuln_frames > 0);
    }

    /* Second collision should NOT deal additional damage */
    game.player.position = target->position;  /* Still overlapping */
    collisions_check_bodies(&game);
    TEST_ASSERT_EQ(health_after_first, game.player.health);
}

TEST(body_collision_mass_ratio) {
    reset_game();
    start_level(0);

    /* Spawn a heavy enemy (boss-like mass) */
    Vec2 pos = vec2(1600, 1200);
    Enemy* heavy = enemy_spawn(&game, ENEMY_TANK, ENEMY_SIZE_SMALL, pos, 500.0f);  /* 5x player mass */
    TEST_ASSERT_NOT_NULL(heavy);
    heavy->health = 10000;  /* Don't let it die */

    int player_initial = game.player.health;
    int enemy_initial = heavy->health;

    /* Collide */
    game.player.position = pos;
    game.player.speed = PLAYER_MAX_SPEED;
    game.player.invuln_frames = 0;
    heavy->invuln_frames = 0;

    collisions_check_bodies(&game);

    int player_damage = player_initial - game.player.health;
    int enemy_damage = enemy_initial - heavy->health;

    /* Player should take more damage than the heavy enemy due to mass ratio */
    TEST_ASSERT(player_damage > enemy_damage);
}

TEST(playthrough_full_minute) {
    reset_game();
    start_level(0);

    /* Simulate a full minute of gameplay with random-ish input */
    for (int second = 0; second < 60; second++) {
        int thrust = (second % 3 == 0) ? 1 : 0;
        int turn = (second % 5 == 0) ? 1 : ((second % 5 == 2) ? -1 : 0);
        int fire = (second % 2 == 0) ? 1 : 0;

        simulate_frames(60, thrust, turn, fire);

        /* Game should still be running (not crashed) */
        TEST_ASSERT(game.state == STATE_PLAYING ||
                   game.state == STATE_VICTORY ||
                   game.state == STATE_DEFEAT);
    }

    /* After a minute, we should have progressed */
    TEST_ASSERT(game.frame_count >= 3600);
}

TEST(playthrough_all_levels_init) {
    /* Verify all 6 levels can be initialized */
    for (size_t level = 0; level < LEVEL_COUNT; level++) {
        reset_game();
        start_level((int)level);

        TEST_ASSERT_EQ(STATE_PLAYING, game.state);
        TEST_ASSERT_EQ((int)level, game.current_level_idx);

        /* Each level should have enemies */
        int enemy_count = enemy_pool_count(&game.enemies);
        TEST_ASSERT(enemy_count >= LEVELS[level].enemies_per_wave);
    }
}

/*============================================================================
 * STRESS TESTS
 *============================================================================*/

TEST(stress_many_particles) {
    reset_game();
    start_level(0);

    /* Spawn lots of particles */
    for (int i = 0; i < 100; i++) {
        effect_explosion_small(&game, vec2(1600 + (i % 10) * 50, 1200 + (i / 10) * 50));
    }

    /* Should have spawned 6000 particles (100 * 60) */
    int count = particle_pool_count(&game.particles);
    TEST_ASSERT(count >= 5000);  /* Allow some expiration */

    /* Simulate and verify no crashes */
    simulate_frames(100, 0, 0, 0);

    /* Particles should have partially expired */
    int final_count = particle_pool_count(&game.particles);
    TEST_ASSERT(final_count < count);
}

TEST(stress_many_enemies) {
    reset_game();
    start_level(5);  /* Mordor has the most enemies */

    /* Wait for all waves to spawn */
    for (int i = 0; i < 5; i++) {
        simulate_frames(600, 0, 0, 0);
    }

    /* Should have many enemies active */
    int count = enemy_pool_count(&game.enemies);
    TEST_ASSERT(count >= 40);  /* Mordor has 4 waves of 40 */

    /* Simulate combat and verify no crashes */
    simulate_frames(300, 1, 1, 1);
}

/*============================================================================
 * PERFORMANCE STRESS TESTS
 *============================================================================*/

TEST(perf_max_particles) {
    reset_game();
    start_level(0);

    /* Spawn maximum particles (10000) */
    printf("      Spawning %d particles...\n", MAX_PARTICLES);
    int spawned = 0;
    while (particle_pool_count(&game.particles) < MAX_PARTICLES - 100) {
        effect_explosion_large(&game, vec2(
            (float)(game_rand_range(&game, MAP_WIDTH)),
            (float)(game_rand_range(&game, MAP_HEIGHT))
        ));
        spawned++;
        if (spawned > 200) break;  /* Safety limit */
    }

    int count = particle_pool_count(&game.particles);
    printf("      Active particles: %d\n", count);
    TEST_ASSERT(count >= 8000);

    /* Time 60 frames of particle updates */
    clock_t start = clock();
    simulate_frames(60, 0, 0, 0);
    clock_t end = clock();

    double ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    printf("      60 frames with %d particles: %.2f ms (%.2f ms/frame)\n",
           count, ms, ms / 60.0);

    /* Should complete in reasonable time (< 100ms for 60 frames on modern CPU) */
    TEST_ASSERT(ms < 500.0);  /* Very generous limit */
}

TEST(perf_max_enemies) {
    reset_game();
    start_level(0);

    /* Manually spawn maximum enemies */
    printf("      Spawning %d enemies...\n", MAX_ENEMIES);
    for (int i = 0; i < MAX_ENEMIES - 1; i++) {
        Vec2 pos = vec2(
            (float)(game_rand_range(&game, MAP_WIDTH)),
            (float)(game_rand_range(&game, MAP_HEIGHT))
        );
        EnemyType type = (EnemyType)(i % 4);
        enemy_spawn(&game, type, ENEMY_SIZE_SMALL, pos, 50.0f);
    }

    int count = enemy_pool_count(&game.enemies);
    printf("      Active enemies: %d\n", count);
    TEST_ASSERT(count >= MAX_ENEMIES - 10);

    /* Time 60 frames of enemy AI updates (the O(n²) repulsion) */
    clock_t start = clock();
    simulate_frames(60, 0, 0, 0);
    clock_t end = clock();

    double ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    printf("      60 frames with %d enemies: %.2f ms (%.2f ms/frame)\n",
           count, ms, ms / 60.0);

    /* Should complete in reasonable time */
    TEST_ASSERT(ms < 2000.0);  /* Allow more time for O(n²) */
}

TEST(perf_combat_chaos) {
    reset_game();
    start_level(5);  /* Mordor - most enemies */

    /* Spawn all waves */
    for (int i = 0; i < 5; i++) {
        simulate_frames(600, 0, 0, 0);
    }

    /* Add lots of particles from explosions */
    for (int i = 0; i < 50; i++) {
        effect_explosion_large(&game, vec2(
            (float)(game_rand_range(&game, MAP_WIDTH)),
            (float)(game_rand_range(&game, MAP_HEIGHT))
        ));
    }

    int enemies = enemy_pool_count(&game.enemies);
    int particles = particle_pool_count(&game.particles);
    printf("      Combat chaos: %d enemies, %d particles\n", enemies, particles);

    /* Time 120 frames of full combat (particles, enemies, collisions, firing) */
    clock_t start = clock();
    simulate_frames(120, 1, 1, 1);  /* Player moving and firing */
    clock_t end = clock();

    double ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    printf("      120 frames of combat: %.2f ms (%.2f ms/frame)\n", ms, ms / 120.0);

    /* At 60fps, each frame should be < 16.67ms. Allow 2x for safety. */
    double ms_per_frame = ms / 120.0;
    TEST_ASSERT(ms_per_frame < 33.0);
}

TEST(perf_sustained_gameplay) {
    reset_game();
    start_level(5);

    printf("      Simulating 5 minutes of gameplay...\n");

    clock_t start = clock();

    /* 5 minutes at 60fps = 18000 frames */
    for (int minute = 0; minute < 5; minute++) {
        for (int second = 0; second < 60; second++) {
            int thrust = (second % 3 == 0) ? 1 : 0;
            int turn = (second % 5 == 0) ? 1 : ((second % 5 == 2) ? -1 : 0);
            int fire = (second % 2 == 0) ? 1 : 0;
            simulate_frames(60, thrust, turn, fire);
        }
    }

    clock_t end = clock();
    double ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    double realtime_ratio = (5.0 * 60.0 * 1000.0) / ms;

    printf("      18000 frames in %.2f ms\n", ms);
    printf("      Realtime ratio: %.1fx (>1 means faster than realtime)\n", realtime_ratio);

    /* Should run at least 10x realtime on modern CPU */
    TEST_ASSERT(realtime_ratio > 5.0);
    TEST_ASSERT(game.frame_count >= 18000);
}

TEST(perf_absolute_worst_case) {
    reset_game();
    start_level(0);

    /* Spawn maximum enemies */
    printf("      Spawning max enemies and particles...\n");
    for (int i = 0; i < MAX_ENEMIES - 1; i++) {
        Vec2 pos = vec2(
            (float)(game_rand_range(&game, MAP_WIDTH)),
            (float)(game_rand_range(&game, MAP_HEIGHT))
        );
        enemy_spawn(&game, (EnemyType)(i % 4), ENEMY_SIZE_SMALL, pos, 50.0f);
    }

    /* Spawn maximum particles */
    while (particle_pool_count(&game.particles) < MAX_PARTICLES - 100) {
        effect_explosion_small(&game, vec2(
            (float)(game_rand_range(&game, MAP_WIDTH)),
            (float)(game_rand_range(&game, MAP_HEIGHT))
        ));
    }

    int enemies = enemy_pool_count(&game.enemies);
    int particles = particle_pool_count(&game.particles);
    printf("      WORST CASE: %d enemies, %d particles\n", enemies, particles);

    /* Time 60 frames */
    clock_t start = clock();
    simulate_frames(60, 1, 1, 1);
    clock_t end = clock();

    double ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
    double ms_per_frame = ms / 60.0;
    printf("      60 frames: %.2f ms (%.2f ms/frame)\n", ms, ms_per_frame);

    /* This is the absolute worst case - we're lenient here */
    /* But it should still be playable (< 50ms/frame = 20fps minimum) */
    TEST_ASSERT(ms_per_frame < 50.0);
}

/*============================================================================
 * MAIN
 *============================================================================*/

int main(void) {
    TEST_SUITE_BEGIN("Playthrough Tests");

    printf("\n  Level Initialization:\n");
    RUN_TEST(playthrough_level_init);
    RUN_TEST(playthrough_all_levels_init);

    printf("\n  Player Movement:\n");
    RUN_TEST(playthrough_player_moves);
    RUN_TEST(playthrough_player_turns);

    printf("\n  Weapons:\n");
    RUN_TEST(playthrough_weapons_fire);

    printf("\n  Enemy AI:\n");
    RUN_TEST(playthrough_enemies_move);

    printf("\n  Particles:\n");
    RUN_TEST(playthrough_particles_age);

    printf("\n  Wave System:\n");
    RUN_TEST(playthrough_wave_progression);

    printf("\n  Combat:\n");
    RUN_TEST(playthrough_combat_damage);
    RUN_TEST(playthrough_player_damage);
    RUN_TEST(playthrough_kill_enemy);

    printf("\n  Body Collision:\n");
    RUN_TEST(body_collision_damages_both);
    RUN_TEST(body_collision_knockback);
    RUN_TEST(body_collision_invulnerability);
    RUN_TEST(body_collision_mass_ratio);

    printf("\n  Extended Gameplay:\n");
    RUN_TEST(playthrough_full_minute);

    printf("\n  Stress Tests:\n");
    RUN_TEST(stress_many_particles);
    RUN_TEST(stress_many_enemies);

    printf("\n  Performance Stress Tests:\n");
    RUN_TEST(perf_max_particles);
    RUN_TEST(perf_max_enemies);
    RUN_TEST(perf_combat_chaos);
    RUN_TEST(perf_sustained_gameplay);
    RUN_TEST(perf_absolute_worst_case);

    TEST_SUITE_END();
    TEST_REPORT();

    return TEST_EXIT_CODE();
}
