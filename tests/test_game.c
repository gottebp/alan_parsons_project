/*
 * test_game.c - Tests for core game logic
 */

#include "test_framework.h"
#include "../include_c/game/game.h"

/*============================================================================
 * HELPER FUNCTIONS
 *============================================================================*/

static Game* create_test_game(void) {
    static Game game;
    game_init(&game);
    return &game;
}

/*============================================================================
 * RANDOM NUMBER TESTS
 *============================================================================*/

TEST(rand_seed_deterministic) {
    Game* game = create_test_game();

    game_rand_seed(game, 12345);
    uint32_t r1 = game_rand(game);
    uint32_t r2 = game_rand(game);

    game_rand_seed(game, 12345);
    uint32_t r3 = game_rand(game);
    uint32_t r4 = game_rand(game);

    TEST_ASSERT_EQ(r1, r3);
    TEST_ASSERT_EQ(r2, r4);
}

TEST(rand_range) {
    Game* game = create_test_game();
    game_rand_seed(game, 42);

    /* All values should be in range */
    for (int i = 0; i < 1000; i++) {
        uint32_t r = game_rand_range(game, 100);
        TEST_ASSERT(r < 100);
    }
}

TEST(rand_float) {
    Game* game = create_test_game();
    game_rand_seed(game, 42);

    /* All values should be in [0, 1) */
    for (int i = 0; i < 1000; i++) {
        float r = game_rand_float(game);
        TEST_ASSERT(r >= 0.0f);
        TEST_ASSERT(r < 1.0f);
    }
}

/*============================================================================
 * PLAYER INITIALIZATION TESTS
 *============================================================================*/

TEST(player_init_position) {
    Game* game = create_test_game();

    /* Player should start in center of map */
    TEST_ASSERT_FLOAT_EQ(MAP_WIDTH / 2.0f, game->player.position.x, 1.0f);
    TEST_ASSERT_FLOAT_EQ(MAP_HEIGHT / 2.0f, game->player.position.y, 1.0f);
}

TEST(player_init_health) {
    Game* game = create_test_game();
    TEST_ASSERT_EQ(PLAYER_MAX_HEALTH, game->player.health);
}

TEST(player_init_velocity) {
    Game* game = create_test_game();
    TEST_ASSERT_FLOAT_EQ(0.0f, game->player.speed, 0.001f);
    TEST_ASSERT_FLOAT_EQ(0.0f, game->player.strafe_speed, 0.001f);
}

TEST(player_init_weapons) {
    Game* game = create_test_game();
    TEST_ASSERT_EQ(0, game->player.weapons_level);
}

/*============================================================================
 * PLAYER PHYSICS TESTS
 *============================================================================*/

TEST(player_physics_friction) {
    Game* game = create_test_game();
    game->player.speed = 5.0f;

    player_physics(&game->player, 0.016f);

    /* Speed should have decreased due to friction */
    TEST_ASSERT(game->player.speed < 5.0f);
    TEST_ASSERT(game->player.speed > 0.0f);
}

TEST(player_physics_position_update) {
    Game* game = create_test_game();
    Vec2 start = game->player.position;
    game->player.speed = 10.0f;
    game->player.angle = 0;  /* Facing right */

    player_physics(&game->player, 0.016f);

    /* Should have moved right */
    TEST_ASSERT(game->player.position.x > start.x);
}

TEST(player_physics_wrap_x) {
    Game* game = create_test_game();
    game->player.position.x = MAP_WIDTH + 100.0f;

    player_physics(&game->player, 0.016f);

    /* Should have wrapped */
    TEST_ASSERT(game->player.position.x < MAP_WIDTH);
    TEST_ASSERT(game->player.position.x >= 0);
}

TEST(player_physics_wrap_y) {
    Game* game = create_test_game();
    game->player.position.y = -100.0f;

    player_physics(&game->player, 0.016f);

    /* Should have wrapped */
    TEST_ASSERT(game->player.position.y < MAP_HEIGHT);
    TEST_ASSERT(game->player.position.y >= 0);
}

/*============================================================================
 * PARTICLE SYSTEM TESTS
 *============================================================================*/

TEST(particle_spawn_basic) {
    Game* game = create_test_game();

    Particle* p = particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_SMALL,
                                  5, vec2(100, 200), 64, 10.0f, 30, 5);

    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT_EQ(1, p->active);
    TEST_ASSERT_EQ(COLLISION_PLAYER_OWNED, p->collision_layer);
    TEST_ASSERT_EQ(5, p->flare_index);
    TEST_ASSERT_EQ(0, p->age);
    TEST_ASSERT_EQ(30, p->max_age);
    TEST_ASSERT_EQ(5, p->damage);
}

TEST(particle_spawn_velocity) {
    Game* game = create_test_game();

    /* Angle 0 = right, so velocity should be mostly +X */
    Particle* p = particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL,
                                  0, vec2(100, 100), 0, 10.0f, 30, 0);

    TEST_ASSERT_NOT_NULL(p);
    TEST_ASSERT(p->velocity.x > 9.0f);  /* Mostly right */
    TEST_ASSERT(fabsf(p->velocity.y) < 1.0f);  /* Minimal Y */
}

TEST(particle_spawn_many) {
    Game* game = create_test_game();

    /* Spawn 100 particles */
    for (int i = 0; i < 100; i++) {
        Particle* p = particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL,
                                      i % 16, vec2(i * 10, i * 10), i, 5.0f, 50, 1);
        TEST_ASSERT_NOT_NULL(p);
    }

    TEST_ASSERT_EQ(100, particle_pool_count(&game->particles));
}

TEST(particles_update_age) {
    Game* game = create_test_game();

    Particle* p = particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL,
                                  0, vec2(100, 100), 0, 0.0f, 10, 0);

    TEST_ASSERT_FLOAT_EQ(0.0f, p->age, 0.001f);

    /* At dt=1/60, one frame advances age by ~1.0 */
    float dt = 1.0f / 60.0f;
    particles_update(game, dt);
    TEST_ASSERT_FLOAT_EQ(1.0f, p->age, 0.1f);

    particles_update(game, dt);
    TEST_ASSERT_FLOAT_EQ(2.0f, p->age, 0.1f);
}

TEST(particles_update_expire) {
    Game* game = create_test_game();

    Particle* p = particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL,
                                  0, vec2(100, 100), 0, 0.0f, 3, 0);

    /* Age it past max (lifetime=3 frames, need ~4 updates at 60fps) */
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 5; i++) {
        particles_update(game, dt);
    }

    /* Should be gone */
    TEST_ASSERT_EQ(0, p->active);
}

TEST(particles_update_movement) {
    Game* game = create_test_game();

    Particle* p = particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL,
                                  0, vec2(100, 100), 0, 10.0f, 100, 0);
    Vec2 start = p->position;

    particles_update(game, 0.016f);

    /* Should have moved */
    TEST_ASSERT(p->position.x > start.x);
}

/*============================================================================
 * ENEMY SPAWNING TESTS
 *============================================================================*/

TEST(enemy_spawn_basic) {
    Game* game = create_test_game();

    Enemy* e = enemy_spawn(game, ENEMY_SCOUT, ENEMY_SIZE_SMALL, vec2(500, 600), 35.0f);

    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQ(1, e->active);
    TEST_ASSERT_EQ(ENEMY_SCOUT, e->type);
    TEST_ASSERT_EQ(ENEMY_SIZE_SMALL, e->size);
    TEST_ASSERT_FLOAT_EQ(500.0f, e->position.x, 0.01f);
    TEST_ASSERT_FLOAT_EQ(600.0f, e->position.y, 0.01f);
    TEST_ASSERT_FLOAT_EQ(35.0f, e->mass, 0.01f);
    TEST_ASSERT_EQ(35 * ENEMY_HEALTH_MULTIPLIER, e->health);
}

TEST(enemy_spawn_boss) {
    Game* game = create_test_game();

    Enemy* boss = enemy_spawn(game, ENEMY_HUNTER, ENEMY_SIZE_BOSS, vec2(400, 400), 800.0f);

    TEST_ASSERT_NOT_NULL(boss);
    TEST_ASSERT_EQ(ENEMY_SIZE_BOSS, boss->size);
    TEST_ASSERT_EQ(800 * ENEMY_HEALTH_MULTIPLIER, boss->health);
}

TEST(enemy_spawn_many) {
    Game* game = create_test_game();

    for (int i = 0; i < 50; i++) {
        Enemy* e = enemy_spawn(game, i % 4, ENEMY_SIZE_SMALL, vec2(i * 10, i * 20), 40.0f);
        TEST_ASSERT_NOT_NULL(e);
    }

    TEST_ASSERT_EQ(50, enemy_pool_count(&game->enemies));
}

/*============================================================================
 * EFFECT TESTS
 *============================================================================*/

TEST(effect_screen_shake) {
    Game* game = create_test_game();

    effect_screen_shake(game, 100);
    TEST_ASSERT(game->shake_timer > 1.0f);  /* 100 frames at 60fps ≈ 1.67s */
}

TEST(effect_explosion_small_creates_particles) {
    Game* game = create_test_game();

    int before = particle_pool_count(&game->particles);
    effect_explosion_small(game, vec2(500, 500));
    int after = particle_pool_count(&game->particles);

    /* Should create 60 particles (20 loops * 3 per loop) */
    TEST_ASSERT_EQ(60, after - before);
}

TEST(effect_explosion_large_creates_particles) {
    Game* game = create_test_game();

    int before = particle_pool_count(&game->particles);
    effect_explosion_large(game, vec2(500, 500));
    int after = particle_pool_count(&game->particles);

    /* Should create 600 particles (100 loops * 6 per loop) */
    TEST_ASSERT_EQ(600, after - before);
}

TEST(effect_nuke_creates_particles) {
    Game* game = create_test_game();

    int before = particle_pool_count(&game->particles);
    effect_nuke(game, vec2(500, 500));
    int after = particle_pool_count(&game->particles);

    /* Should create 512 particles (256/2 * 4 per angle) */
    TEST_ASSERT_EQ(512, after - before);
}

/*============================================================================
 * GAME STATE TESTS
 *============================================================================*/

TEST(game_init_state) {
    Game* game = create_test_game();

    TEST_ASSERT_EQ(STATE_MENU, game->state);
    TEST_ASSERT_EQ(0, game->unlocked_level);
    TEST_ASSERT_EQ(0, game->weapons_level);
}

TEST(game_start_level_state) {
    Game* game = create_test_game();

    game_start_level(game, 0);

    TEST_ASSERT_EQ(STATE_PLAYING, game->state);
    TEST_ASSERT_EQ(0, game->current_level_idx);
    TEST_ASSERT_EQ(0, game->outcome_timer);
}

TEST(game_start_level_resets_player) {
    Game* game = create_test_game();

    /* Damage player */
    game->player.health = 100;
    game->player.position = vec2(0, 0);

    game_start_level(game, 0);

    TEST_ASSERT_EQ(PLAYER_MAX_HEALTH, game->player.health);
    TEST_ASSERT_FLOAT_EQ(MAP_WIDTH / 2.0f, game->player.position.x, 1.0f);
}

/*============================================================================
 * WAVE SYSTEM TESTS
 *============================================================================*/

TEST(waves_init_creates_enemies) {
    Game* game = create_test_game();

    game_start_level(game, 0);  /* Shire: 3 waves of 16 = 48 enemies + boss */

    /* First wave should be active */
    int active_count = 0;
    POOL_FOREACH(&game->enemies, e, Enemy, {
        if (e->active) active_count++;
    });

    /* First wave of Shire is 8 enemies (original ASM value) */
    TEST_ASSERT(active_count >= 8);
}

TEST(waves_init_creates_boss) {
    Game* game = create_test_game();

    game_start_level(game, 0);

    TEST_ASSERT(game->waves.boss_index >= 0);
    TEST_ASSERT_EQ(0, game->waves.boss_spawned);
}

/*============================================================================
 * LEVEL DATA TESTS
 *============================================================================*/

TEST(level_count) {
    TEST_ASSERT_EQ(6, LEVEL_COUNT);
}

TEST(level_shire) {
    TEST_ASSERT_STR_EQ("Shire", LEVELS[0].name);
    TEST_ASSERT_EQ(3, LEVELS[0].wave_count);
    TEST_ASSERT_EQ(8, LEVELS[0].enemies_per_wave);  /* Original ASM value */
}

TEST(level_mordor) {
    TEST_ASSERT_STR_EQ("Mordor", LEVELS[5].name);
    TEST_ASSERT_EQ(4, LEVELS[5].wave_count);
    TEST_ASSERT_EQ(24, LEVELS[5].enemies_per_wave);  /* Original ASM value */
}

/*============================================================================
 * COLLISION DETECTION TESTS
 *============================================================================*/

TEST(collision_particle_player_direct_hit) {
    Game* game = create_test_game();
    game->player.position = vec2(500, 500);
    game->player.nose = vec2(510, 500);
    game->player.tail = vec2(490, 500);

    /* Particle directly on player */
    Particle p = {0};
    p.active = 1;
    p.position = vec2(500, 500);
    p.size = PARTICLE_SIZE_SMALL;

    TEST_ASSERT_EQ(1, collision_particle_player(&p, &game->player));
}

TEST(collision_particle_player_near_miss) {
    Game* game = create_test_game();
    game->player.position = vec2(500, 500);
    game->player.nose = vec2(510, 500);
    game->player.tail = vec2(490, 500);

    /* Particle far from player */
    Particle p = {0};
    p.active = 1;
    p.position = vec2(600, 600);
    p.size = PARTICLE_SIZE_SMALL;

    TEST_ASSERT_EQ(0, collision_particle_player(&p, &game->player));
}

TEST(collision_particle_enemy_hit) {
    Game* game = create_test_game();

    Enemy* e = enemy_spawn(game, ENEMY_SCOUT, ENEMY_SIZE_SMALL, vec2(300, 300), 40.0f);

    Particle p = {0};
    p.active = 1;
    p.position = vec2(300, 300);  /* Same position */
    p.size = PARTICLE_SIZE_SMALL;

    TEST_ASSERT_EQ(1, collision_particle_enemy(&p, e));
}

TEST(collision_particle_enemy_miss) {
    Game* game = create_test_game();

    Enemy* e = enemy_spawn(game, ENEMY_SCOUT, ENEMY_SIZE_SMALL, vec2(300, 300), 40.0f);

    Particle p = {0};
    p.active = 1;
    p.position = vec2(400, 400);  /* Far away */
    p.size = PARTICLE_SIZE_SMALL;

    TEST_ASSERT_EQ(0, collision_particle_enemy(&p, e));
}

/*============================================================================
 * DAMAGE SYSTEM TESTS
 *============================================================================*/

TEST(player_take_damage) {
    Game* game = create_test_game();
    game->player.health = 1000;

    player_take_damage(game, 100, vec2(500, 500));

    TEST_ASSERT_EQ(900, game->player.health);
}

TEST(player_take_lethal_damage) {
    Game* game = create_test_game();
    game->player.health = 50;

    player_take_damage(game, 100, vec2(500, 500));

    TEST_ASSERT(game->player.health <= 0);
}

TEST(enemy_take_damage) {
    Game* game = create_test_game();

    Enemy* e = enemy_spawn(game, ENEMY_SCOUT, ENEMY_SIZE_SMALL, vec2(300, 300), 40.0f);
    int initial_health = e->health;

    enemy_damage(game, e, 10, vec2(300, 300));

    TEST_ASSERT_EQ(initial_health - 10, e->health);
}

TEST(enemy_death_creates_explosion) {
    Game* game = create_test_game();

    Enemy* e = enemy_spawn(game, ENEMY_SCOUT, ENEMY_SIZE_SMALL, vec2(300, 300), 40.0f);
    int particles_before = particle_pool_count(&game->particles);

    /* Deal lethal damage */
    enemy_damage(game, e, 10000, vec2(300, 300));

    int particles_after = particle_pool_count(&game->particles);

    /* Should have spawned explosion particles */
    TEST_ASSERT(particles_after > particles_before);
    TEST_ASSERT_EQ(0, e->active);  /* Enemy should be dead */
}

/*============================================================================
 * MAIN
 *============================================================================*/

int main(void) {
    TEST_SUITE_BEGIN("Game Logic Tests");

    printf("\n  Random Numbers:\n");
    RUN_TEST(rand_seed_deterministic);
    RUN_TEST(rand_range);
    RUN_TEST(rand_float);

    printf("\n  Player Initialization:\n");
    RUN_TEST(player_init_position);
    RUN_TEST(player_init_health);
    RUN_TEST(player_init_velocity);
    RUN_TEST(player_init_weapons);

    printf("\n  Player Physics:\n");
    RUN_TEST(player_physics_friction);
    RUN_TEST(player_physics_position_update);
    RUN_TEST(player_physics_wrap_x);
    RUN_TEST(player_physics_wrap_y);

    printf("\n  Particle System:\n");
    RUN_TEST(particle_spawn_basic);
    RUN_TEST(particle_spawn_velocity);
    RUN_TEST(particle_spawn_many);
    RUN_TEST(particles_update_age);
    RUN_TEST(particles_update_expire);
    RUN_TEST(particles_update_movement);

    printf("\n  Enemy Spawning:\n");
    RUN_TEST(enemy_spawn_basic);
    RUN_TEST(enemy_spawn_boss);
    RUN_TEST(enemy_spawn_many);

    printf("\n  Effects:\n");
    RUN_TEST(effect_screen_shake);
    RUN_TEST(effect_explosion_small_creates_particles);
    RUN_TEST(effect_explosion_large_creates_particles);
    RUN_TEST(effect_nuke_creates_particles);

    printf("\n  Game State:\n");
    RUN_TEST(game_init_state);
    RUN_TEST(game_start_level_state);
    RUN_TEST(game_start_level_resets_player);

    printf("\n  Wave System:\n");
    RUN_TEST(waves_init_creates_enemies);
    RUN_TEST(waves_init_creates_boss);

    printf("\n  Level Data:\n");
    RUN_TEST(level_count);
    RUN_TEST(level_shire);
    RUN_TEST(level_mordor);

    printf("\n  Collision Detection:\n");
    RUN_TEST(collision_particle_player_direct_hit);
    RUN_TEST(collision_particle_player_near_miss);
    RUN_TEST(collision_particle_enemy_hit);
    RUN_TEST(collision_particle_enemy_miss);

    printf("\n  Damage System:\n");
    RUN_TEST(player_take_damage);
    RUN_TEST(player_take_lethal_damage);
    RUN_TEST(enemy_take_damage);
    RUN_TEST(enemy_death_creates_explosion);

    TEST_SUITE_END();
    TEST_REPORT();

    return TEST_EXIT_CODE();
}
