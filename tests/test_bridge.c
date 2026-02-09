/*
 * test_bridge.c - Tests for the Bridge between new Game and legacy globals
 *
 * This test creates mock legacy globals and verifies that bridge_sync
 * functions correctly transfer state in both directions.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "../include_c/game/game.h"
#include "../include_c/game/bridge.h"

/*============================================================================
 * MOCK LEGACY GLOBALS
 * (These simulate what exists in player.c, enemy.c, ppe.c, mapeng.c)
 *============================================================================*/

/* Input globals (for bridge_update_from_legacy_input) */
uint8_t KEYBOARD[320] = {0};

/* Player globals */
int intPlayerX = 0, intPlayerY = 0;
float fltPlayerX = 0.0f, fltPlayerY = 0.0f;
float fltPlayerSpeed = 0.0f, fltPlayerStrafeSpeed = 0.0f;
int8_t intbPlayerAngle = 0, intbPlayerTurnDir = 0;
int intPlayerHealth = 0;
int intPlayerWeaponsLevel = 0;
float fltPlayerNoseX = 0.0f, fltPlayerNoseY = 0.0f;
float fltPlayerTailX = 0.0f, fltPlayerTailY = 0.0f;
int intPlayerNoseX = 0, intPlayerNoseY = 0;
int intPlayerTailX = 0, intPlayerTailY = 0;
float fltPlayerAngularVel = 0.0f;
int intPlayerInvulnFrames = 0;

/*
 * BridgeEnemy - Compatible with defs.h Enemy struct layout.
 * Used for mock globals in tests.
 */
typedef struct {
    uint32_t enemy_type;
    float enemy_x_float;
    float enemy_y_float;
    float enemy_x_vel_float;
    float enemy_y_vel_float;
    float enemy_mass;
    float enemy_angular_vel;
    uint8_t enemy_angle;
    uint8_t enemy_size;
    int8_t enemy_active;
    uint8_t padding;
    int32_t enemy_x_int;
    int32_t enemy_y_int;
    int32_t enemy_health;
    int32_t enemy_invuln_frames;
} BridgeEnemy;

BridgeEnemy Enemies[MAX_ENEMIES];

/*
 * BridgePARTICLE - Compatible with ppe.h PARTICLE struct layout.
 */
typedef struct {
    uint8_t IsActive;
    uint8_t DetectCollisions;
    uint32_t ImgSizeType;
    uint32_t ImgOffset;
    float fltX;
    float fltY;
    int intX;
    int intY;
    float XV;
    float YV;
    uint32_t MaxLife;
    uint32_t Age;
    uint32_t Damage;
} BridgePARTICLE;

/* Particle globals */
BridgePARTICLE* ParticleDataOff = NULL;
int NumParticles = 0;

/* Map/shake globals */
int intShakeMap = 0;
float fltCameraX = 0.0f, fltCameraY = 0.0f;
int intCameraX = 0, intCameraY = 0;

/* Wave system globals */
uint32_t FrameNum = 0;
uint32_t NextWaveNumber = 0;

/*============================================================================
 * TEST INFRASTRUCTURE
 *============================================================================*/

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("    FAIL: %s\n", msg); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FLOAT_EQ(a, b, msg) ASSERT(fabsf((a) - (b)) < 0.001f, msg)
#define ASSERT_INT_EQ(a, b, msg) ASSERT((a) == (b), msg)

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    printf("    "); \
    tests_run++; \
    test_##name(); \
    if (tests_failed == 0 || tests_passed == tests_run - 1) { \
        printf("PASS: %s\n", #name); \
        tests_passed++; \
    } \
} while(0)

static void reset_globals(void) {
    /* Reset player */
    intPlayerX = intPlayerY = 0;
    fltPlayerX = fltPlayerY = 0.0f;
    fltPlayerSpeed = fltPlayerStrafeSpeed = 0.0f;
    intbPlayerAngle = intbPlayerTurnDir = 0;
    intPlayerHealth = intPlayerWeaponsLevel = 0;
    fltPlayerNoseX = fltPlayerNoseY = 0.0f;
    fltPlayerTailX = fltPlayerTailY = 0.0f;
    intPlayerNoseX = intPlayerNoseY = 0;
    intPlayerTailX = intPlayerTailY = 0;
    fltPlayerAngularVel = 0.0f;
    intPlayerInvulnFrames = 0;

    /* Reset enemies */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemies[i].enemy_active = -1;
    }

    /* Reset particles */
    if (ParticleDataOff) {
        for (int i = 0; i < MAX_PARTICLES; i++) {
            ParticleDataOff[i].IsActive = 0;
        }
    }
    NumParticles = 0;

    /* Reset map/shake */
    intShakeMap = 0;
    fltCameraX = fltCameraY = 0.0f;
    intCameraX = intCameraY = 0;

    /* Reset waves */
    FrameNum = NextWaveNumber = 0;
}

/*============================================================================
 * PLAYER SYNC TESTS
 *============================================================================*/

TEST(player_to_globals) {
    reset_globals();
    Game game;
    game_init(&game);

    /* Set up player in Game struct */
    game.player.position = vec2(1234.5f, 5678.9f);
    game.player.speed = 12.5f;
    game.player.strafe_speed = -3.2f;
    game.player.angle = 128;
    game.player.turn_direction = -1;
    game.player.health = 8500;
    game.player.weapons_level = 3;
    game.player.nose = vec2(1240.0f, 5680.0f);
    game.player.tail = vec2(1220.0f, 5670.0f);
    game.player.angular_vel = 2.5f;
    game.player.invuln_frames = 15;

    bridge_sync_player_to_globals(&game);

    ASSERT_FLOAT_EQ(fltPlayerX, 1234.5f, "fltPlayerX");
    ASSERT_FLOAT_EQ(fltPlayerY, 5678.9f, "fltPlayerY");
    ASSERT_INT_EQ(intPlayerX, 1234, "intPlayerX");
    ASSERT_INT_EQ(intPlayerY, 5678, "intPlayerY");
    ASSERT_FLOAT_EQ(fltPlayerSpeed, 12.5f, "fltPlayerSpeed");
    ASSERT_FLOAT_EQ(fltPlayerStrafeSpeed, -3.2f, "fltPlayerStrafeSpeed");
    ASSERT_INT_EQ(intbPlayerAngle, -128, "intbPlayerAngle (128 as signed)");
    ASSERT_INT_EQ(intbPlayerTurnDir, -1, "intbPlayerTurnDir");
    ASSERT_INT_EQ(intPlayerHealth, 8500, "intPlayerHealth");
    ASSERT_INT_EQ(intPlayerWeaponsLevel, 3, "intPlayerWeaponsLevel");
    ASSERT_FLOAT_EQ(fltPlayerNoseX, 1240.0f, "fltPlayerNoseX");
    ASSERT_FLOAT_EQ(fltPlayerAngularVel, 2.5f, "fltPlayerAngularVel");
    ASSERT_INT_EQ(intPlayerInvulnFrames, 15, "intPlayerInvulnFrames");
}

TEST(player_from_globals) {
    reset_globals();
    Game game;
    game_init(&game);

    /* Set up legacy globals */
    fltPlayerX = 999.1f;
    fltPlayerY = 888.2f;
    fltPlayerSpeed = 8.0f;
    fltPlayerStrafeSpeed = 2.1f;
    intbPlayerAngle = 64;
    intbPlayerTurnDir = 1;
    intPlayerHealth = 5000;
    intPlayerWeaponsLevel = 2;
    fltPlayerNoseX = 1005.0f;
    fltPlayerNoseY = 890.0f;
    fltPlayerAngularVel = -1.5f;
    intPlayerInvulnFrames = 10;

    bridge_sync_player_from_globals(&game);

    ASSERT_FLOAT_EQ(game.player.position.x, 999.1f, "position.x");
    ASSERT_FLOAT_EQ(game.player.position.y, 888.2f, "position.y");
    ASSERT_FLOAT_EQ(game.player.speed, 8.0f, "speed");
    ASSERT_FLOAT_EQ(game.player.strafe_speed, 2.1f, "strafe_speed");
    ASSERT_INT_EQ(game.player.angle, 64, "angle");
    ASSERT_INT_EQ(game.player.turn_direction, 1, "turn_direction");
    ASSERT_INT_EQ(game.player.health, 5000, "health");
    ASSERT_INT_EQ(game.player.weapons_level, 2, "weapons_level");
    ASSERT_FLOAT_EQ(game.player.angular_vel, -1.5f, "angular_vel");
    ASSERT_INT_EQ(game.player.invuln_frames, 10, "invuln_frames");
}

TEST(player_roundtrip) {
    reset_globals();
    Game game1, game2;
    game_init(&game1);
    game_init(&game2);

    /* Set up player */
    game1.player.position = vec2(1500.0f, 2000.0f);
    game1.player.speed = 15.0f;
    game1.player.health = 7777;

    /* Sync to globals then back to different game */
    bridge_sync_player_to_globals(&game1);
    bridge_sync_player_from_globals(&game2);

    ASSERT_FLOAT_EQ(game2.player.position.x, 1500.0f, "roundtrip x");
    ASSERT_FLOAT_EQ(game2.player.position.y, 2000.0f, "roundtrip y");
    ASSERT_FLOAT_EQ(game2.player.speed, 15.0f, "roundtrip speed");
    ASSERT_INT_EQ(game2.player.health, 7777, "roundtrip health");
}

/*============================================================================
 * ENEMY SYNC TESTS
 *============================================================================*/

TEST(enemies_to_globals) {
    reset_globals();
    Game game;
    game_init(&game);

    /* Spawn some enemies in the new system */
    Enemy* e1 = enemy_spawn(&game, ENEMY_SCOUT, ENEMY_SIZE_SMALL, vec2(100, 200), 40.0f);
    e1->velocity = vec2(1.5f, -2.0f);
    e1->angle = 45;
    e1->health = 800;

    Enemy* e2 = enemy_spawn(&game, ENEMY_HUNTER, ENEMY_SIZE_BOSS, vec2(1000, 1500), 300.0f);
    e2->health = 6000;

    bridge_sync_enemies_to_globals(&game);

    /* Verify first enemy */
    ASSERT_INT_EQ(Enemies[0].enemy_active, 1, "e1 active");
    ASSERT_INT_EQ((int)Enemies[0].enemy_type, ENEMY_SCOUT, "e1 type");
    ASSERT_FLOAT_EQ(Enemies[0].enemy_x_float, 100.0f, "e1 x");
    ASSERT_FLOAT_EQ(Enemies[0].enemy_y_float, 200.0f, "e1 y");
    ASSERT_INT_EQ(Enemies[0].enemy_health, 800, "e1 health");

    /* Verify second enemy */
    ASSERT_INT_EQ(Enemies[1].enemy_active, 1, "e2 active");
    ASSERT_INT_EQ((int)Enemies[1].enemy_type, ENEMY_HUNTER, "e2 type");
    ASSERT_INT_EQ((int)Enemies[1].enemy_size, ENEMY_SIZE_BOSS, "e2 size");
    ASSERT_INT_EQ(Enemies[1].enemy_health, 6000, "e2 health");

    /* Verify third slot is inactive */
    ASSERT_INT_EQ(Enemies[2].enemy_active, -1, "e3 inactive");
}

TEST(enemies_from_globals) {
    reset_globals();
    Game game;
    game_init(&game);

    /* Set up legacy enemies */
    Enemies[0].enemy_active = 1;
    Enemies[0].enemy_type = ENEMY_TANK;
    Enemies[0].enemy_x_float = 500.0f;
    Enemies[0].enemy_y_float = 600.0f;
    Enemies[0].enemy_x_vel_float = 2.0f;
    Enemies[0].enemy_y_vel_float = -1.0f;
    Enemies[0].enemy_mass = 60.0f;
    Enemies[0].enemy_angle = 100;
    Enemies[0].enemy_size = ENEMY_SIZE_SMALL;
    Enemies[0].enemy_health = 1200;

    Enemies[5].enemy_active = 1;  /* Non-contiguous */
    Enemies[5].enemy_type = ENEMY_STANDARD;
    Enemies[5].enemy_x_float = 800.0f;
    Enemies[5].enemy_y_float = 900.0f;
    Enemies[5].enemy_health = 400;

    bridge_sync_enemies_from_globals(&game);

    ASSERT_INT_EQ(game.enemies.count, 2, "enemy count");

    /* Verify enemies were imported (order may differ) */
    int found_tank = 0, found_standard = 0;
    POOL_FOREACH(&game.enemies, e, Enemy, {
        if (e->type == ENEMY_TANK) {
            found_tank = 1;
            ASSERT_FLOAT_EQ(e->position.x, 500.0f, "tank x");
            ASSERT_INT_EQ(e->health, 1200, "tank health");
        }
        if (e->type == ENEMY_STANDARD) {
            found_standard = 1;
            ASSERT_FLOAT_EQ(e->position.x, 800.0f, "standard x");
        }
    });
    ASSERT(found_tank, "found tank");
    ASSERT(found_standard, "found standard");
}

/*============================================================================
 * PARTICLE SYNC TESTS
 *============================================================================*/

TEST(particles_to_globals) {
    /* Allocate mock particle array */
    static BridgePARTICLE mock_particles[MAX_PARTICLES];
    ParticleDataOff = mock_particles;
    reset_globals();

    Game game;
    game_init(&game);

    /* Spawn some particles */
    particle_spawn(&game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_SMALL, 5,
                   vec2(100, 200), 64, 10.0f, 30, 12);
    particle_spawn(&game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 3,
                   vec2(500, 600), 128, 8.0f, 50, 8);

    bridge_sync_particles_to_globals(&game);

    ASSERT_INT_EQ(NumParticles, 2, "num particles");
    ASSERT_INT_EQ((int)ParticleDataOff[0].IsActive, 1, "p1 active");
    ASSERT_INT_EQ((int)ParticleDataOff[0].DetectCollisions, COLLISION_PLAYER_OWNED, "p1 collision");
    ASSERT_FLOAT_EQ(ParticleDataOff[0].fltX, 100.0f, "p1 x");
    ASSERT_INT_EQ((int)ParticleDataOff[0].Damage, 12, "p1 damage");

    ASSERT_INT_EQ((int)ParticleDataOff[1].IsActive, 1, "p2 active");
    ASSERT_INT_EQ((int)ParticleDataOff[1].ImgSizeType, PARTICLE_SIZE_LARGE, "p2 size");
}

TEST(particles_from_globals) {
    static BridgePARTICLE mock_particles[MAX_PARTICLES];
    ParticleDataOff = mock_particles;
    reset_globals();

    Game game;
    game_init(&game);

    /* Set up legacy particles */
    ParticleDataOff[0].IsActive = 1;
    ParticleDataOff[0].DetectCollisions = COLLISION_PLAYER_OWNED;
    ParticleDataOff[0].ImgSizeType = PARTICLE_SIZE_SMALL;
    ParticleDataOff[0].ImgOffset = 5 * SMALL_PARTICLE_WIDTH * SMALL_PARTICLE_HEIGHT * 4;
    ParticleDataOff[0].fltX = 300.0f;
    ParticleDataOff[0].fltY = 400.0f;
    ParticleDataOff[0].XV = 5.0f;
    ParticleDataOff[0].YV = -3.0f;
    ParticleDataOff[0].MaxLife = 60;
    ParticleDataOff[0].Age = 10;
    ParticleDataOff[0].Damage = 15;

    bridge_sync_particles_from_globals(&game);

    ASSERT_INT_EQ(game.particles.count, 1, "particle count");

    Particle* p = &game.particles.entities[0];
    ASSERT(p->active, "particle active");
    ASSERT_INT_EQ(p->collision_layer, COLLISION_PLAYER_OWNED, "collision layer");
    ASSERT_INT_EQ(p->flare_index, 5, "flare index");
    ASSERT_FLOAT_EQ(p->position.x, 300.0f, "x");
    ASSERT_FLOAT_EQ(p->velocity.x, 5.0f, "vel x");
    ASSERT_INT_EQ(p->max_age, 60, "max age");
    ASSERT_INT_EQ(p->damage, 15, "damage");
}

/*============================================================================
 * GAME STATE SYNC TESTS
 *============================================================================*/

TEST(game_state_to_globals) {
    reset_globals();
    Game game;
    game_init(&game);

    game.shake_frames = 25;
    game.waves.frame_counter = 1234;
    game.waves.current_wave = 3;
    game.player.position = vec2(1600, 1200);

    bridge_sync_game_state_to_globals(&game);

    ASSERT_INT_EQ(intShakeMap, 25, "shake frames");
    ASSERT_INT_EQ((int)FrameNum, 1234, "frame num");
    ASSERT_INT_EQ((int)NextWaveNumber, 3, "next wave");
    ASSERT_FLOAT_EQ(fltCameraX, 1600.0f, "camera x");
    ASSERT_FLOAT_EQ(fltCameraY, 1200.0f, "camera y");
}

TEST(game_state_from_globals) {
    reset_globals();
    Game game;
    game_init(&game);

    intShakeMap = 10;
    FrameNum = 5000;
    NextWaveNumber = 5;

    bridge_sync_game_state_from_globals(&game);

    ASSERT_INT_EQ(game.shake_frames, 10, "shake frames");
    ASSERT_INT_EQ(game.waves.frame_counter, 5000, "frame counter");
    ASSERT_INT_EQ(game.waves.current_wave, 5, "current wave");
}

/*============================================================================
 * FULL SYNC TESTS
 *============================================================================*/

TEST(full_sync_roundtrip) {
    static BridgePARTICLE mock_particles[MAX_PARTICLES];
    ParticleDataOff = mock_particles;
    reset_globals();

    Game game1, game2;
    game_init(&game1);

    /* Set up complete game state */
    game1.player.position = vec2(1000, 1000);
    game1.player.health = 9000;
    game1.player.weapons_level = 4;
    game1.shake_frames = 15;

    enemy_spawn(&game1, ENEMY_SCOUT, ENEMY_SIZE_SMALL, vec2(500, 500), 40.0f);
    enemy_spawn(&game1, ENEMY_TANK, ENEMY_SIZE_SMALL, vec2(700, 700), 60.0f);

    particle_spawn(&game1, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_SMALL, 5,
                   vec2(550, 550), 0, 10.0f, 30, 10);

    /* Sync to globals */
    bridge_sync_all_to_globals(&game1);

    /* Create new game and sync from globals */
    game_init(&game2);
    bridge_sync_all_from_globals(&game2);

    /* Verify */
    ASSERT_FLOAT_EQ(game2.player.position.x, 1000.0f, "player x roundtrip");
    ASSERT_INT_EQ(game2.player.health, 9000, "player health roundtrip");
    ASSERT_INT_EQ(game2.enemies.count, 2, "enemy count roundtrip");
    ASSERT_INT_EQ(game2.particles.count, 1, "particle count roundtrip");
    ASSERT_INT_EQ(game2.shake_frames, 15, "shake roundtrip");
}

TEST(init_from_globals) {
    static BridgePARTICLE mock_particles[MAX_PARTICLES];
    ParticleDataOff = mock_particles;
    reset_globals();

    /* Set up some legacy state */
    fltPlayerX = 2000.0f;
    fltPlayerY = 1500.0f;
    intPlayerHealth = 7500;
    intShakeMap = 5;

    Enemies[0].enemy_active = 1;
    Enemies[0].enemy_type = ENEMY_HUNTER;
    Enemies[0].enemy_x_float = 800.0f;
    Enemies[0].enemy_y_float = 900.0f;
    Enemies[0].enemy_health = 5000;

    Game game;
    bridge_init_game_from_globals(&game);

    ASSERT_FLOAT_EQ(game.player.position.x, 2000.0f, "init player x");
    ASSERT_INT_EQ(game.player.health, 7500, "init player health");
    ASSERT_INT_EQ(game.shake_frames, 5, "init shake");
    ASSERT_INT_EQ(game.enemies.count, 1, "init enemy count");
}

/*============================================================================
 * UPDATE FROM LEGACY INPUT TESTS
 *============================================================================*/

/* SDL scancode values (matching bridge.c) */
#define KEY_UP     82
#define KEY_FIRE   27   /* X key */

TEST(update_from_legacy_input) {
    reset_globals();
    Game game;
    game_init(&game);
    game_start_level(&game, 0);  /* Start Shire */

    /* Clear keyboard */
    memset(KEYBOARD, 0, sizeof(KEYBOARD));

    /* Initial state */
    int initial_frame = (int)game.frame_count;

    /* Call update with no input - use proper 60 FPS dt */
    float dt = 1.0f / 60.0f;
    bridge_update_from_legacy_input(&game, dt);

    /* Frame count should increase */
    ASSERT_INT_EQ((int)game.frame_count, initial_frame + 1, "frame count increases");
}

TEST(update_with_thrust_input) {
    reset_globals();
    Game game;
    game_init(&game);
    game_start_level(&game, 0);

    /* Clear and set UP key */
    memset(KEYBOARD, 0, sizeof(KEYBOARD));
    KEYBOARD[KEY_UP] = 1;

    float initial_speed = game.player.speed;

    /* Run several frames with thrust - use proper 60 FPS dt */
    float dt = 1.0f / 60.0f;
    for (int i = 0; i < 10; i++) {
        bridge_update_from_legacy_input(&game, dt);
    }

    /* Player should have accelerated */
    ASSERT(game.player.speed > initial_speed, "player accelerated with UP key");
}

TEST(update_with_fire_input) {
    reset_globals();
    Game game;
    game_init(&game);
    game_start_level(&game, 0);

    /* Clear and set FIRE key */
    memset(KEYBOARD, 0, sizeof(KEYBOARD));
    KEYBOARD[KEY_FIRE] = 1;

    int initial_particles = game.particles.count;

    /* Run a frame with fire - use proper 60 FPS dt */
    float dt = 1.0f / 60.0f;
    bridge_update_from_legacy_input(&game, dt);

    /* Particles should have been created (weapons fire) */
    ASSERT(game.particles.count > initial_particles, "particles created with fire");
}

/*============================================================================
 * MAIN
 *============================================================================*/

int main(void) {
    printf("=== Bridge Tests ===\n\n");

    printf("  Player Sync:\n");
    RUN_TEST(player_to_globals);
    RUN_TEST(player_from_globals);
    RUN_TEST(player_roundtrip);

    printf("\n  Enemy Sync:\n");
    RUN_TEST(enemies_to_globals);
    RUN_TEST(enemies_from_globals);

    printf("\n  Particle Sync:\n");
    RUN_TEST(particles_to_globals);
    RUN_TEST(particles_from_globals);

    printf("\n  Game State Sync:\n");
    RUN_TEST(game_state_to_globals);
    RUN_TEST(game_state_from_globals);

    printf("\n  Full Sync:\n");
    RUN_TEST(full_sync_roundtrip);
    RUN_TEST(init_from_globals);

    printf("\n  Update From Legacy Input:\n");
    RUN_TEST(update_from_legacy_input);
    RUN_TEST(update_with_thrust_input);
    RUN_TEST(update_with_fire_input);

    printf("\n=====================================\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("=====================================\n");

    if (tests_failed == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    } else {
        printf("SOME TESTS FAILED\n");
        return 1;
    }
}
