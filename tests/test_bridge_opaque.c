/*
 * test_bridge_opaque.c - Test the opaque bridge interface
 *
 * This test simulates how main.c would use the bridge:
 * - Includes legacy defs.h (with old Enemy type)
 * - Uses opaque Game* handle through bridge_opaque.h
 * - Never includes game.h directly
 *
 * This verifies that the type conflict is properly avoided.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

/* Include legacy definitions - this defines the OLD Enemy type */
#include "../include_c/defs.h"
#include "../include_c/enemy.h"  /* For MAX_ENEMIES */

/* Include opaque bridge - does NOT expose new Enemy type */
#include "../include_c/game/bridge_opaque.h"

/*============================================================================
 * MOCK LEGACY GLOBALS (same as test_bridge.c)
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

/* Use the legacy Enemy type from defs.h */
Enemy Enemies[MAX_ENEMIES];

/* Particle globals - need to match ppe.h layout */
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
} MockPARTICLE;

MockPARTICLE* ParticleDataOff = NULL;
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

#define TEST(name) static void test_##name(void)
#define RUN_TEST(name) do { \
    tests_run++; \
    test_##name(); \
    if (tests_failed == 0 || tests_passed == tests_run - 1) { \
        printf("    PASS: %s\n", #name); \
        tests_passed++; \
    } \
} while(0)

static void reset_globals(void) {
    intPlayerX = intPlayerY = 0;
    fltPlayerX = fltPlayerY = 0.0f;
    fltPlayerSpeed = fltPlayerStrafeSpeed = 0.0f;
    intbPlayerAngle = intbPlayerTurnDir = 0;
    intPlayerHealth = MAXPLAYERHEALTH;
    intPlayerWeaponsLevel = 0;

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemies[i].enemy_active = -1;
    }

    intShakeMap = 0;
    FrameNum = NextWaveNumber = 0;
}

/*============================================================================
 * OPAQUE INTERFACE TESTS
 *============================================================================*/

TEST(create_destroy_game) {
    Game* game = bridge_create_game();
    ASSERT(game != NULL, "bridge_create_game returns non-null");
    bridge_destroy_game(game);
}

TEST(seed_random) {
    Game* game = bridge_create_game();
    bridge_seed_random(game, 12345);
    /* Just verify it doesn't crash */
    bridge_destroy_game(game);
}

TEST(start_level) {
    reset_globals();
    Game* game = bridge_create_game();
    bridge_start_level(game, 0);

    ASSERT(bridge_is_playing(game), "game is playing after start_level");
    ASSERT(bridge_get_player_health(game) > 0, "player has health");
    ASSERT(bridge_get_enemy_count(game) > 0, "enemies spawned");

    bridge_destroy_game(game);
}

TEST(sync_to_globals) {
    reset_globals();
    Game* game = bridge_create_game();
    bridge_start_level(game, 0);

    /* Sync to legacy globals */
    bridge_sync_all_to_globals(game);

    /* Verify legacy globals were updated */
    ASSERT(intPlayerHealth > 0, "player health synced");
    /* Player should be in center of map */
    ASSERT(fltPlayerX > 100.0f, "player X synced");
    ASSERT(fltPlayerY > 100.0f, "player Y synced");

    /* Check that enemies were synced */
    int active_count = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (Enemies[i].enemy_active == 1) active_count++;
    }
    ASSERT(active_count > 0, "enemies synced to globals");

    bridge_destroy_game(game);
}

TEST(sync_from_globals) {
    reset_globals();

    /* Set up some legacy state */
    fltPlayerX = 1500.0f;
    fltPlayerY = 1200.0f;
    intPlayerHealth = 5000;
    intShakeMap = 10;

    Game* game = bridge_create_game();
    bridge_sync_all_from_globals(game);

    /* The new game should have the legacy state */
    /* We can only verify through opaque interface */
    ASSERT(bridge_get_player_health(game) == 5000, "health synced from globals");

    bridge_destroy_game(game);
}

TEST(outcome_detection) {
    reset_globals();
    Game* game = bridge_create_game();
    bridge_start_level(game, 0);

    /* Initially playing */
    ASSERT(bridge_get_outcome(game) == 0, "outcome is playing");

    /* Set player health to 0 in globals and sync */
    bridge_sync_all_to_globals(game);
    intPlayerHealth = 0;
    bridge_sync_all_from_globals(game);

    /* Should detect death */
    ASSERT(bridge_get_outcome(game) == 1, "outcome is dead after health=0");

    bridge_destroy_game(game);
}

TEST(legacy_enemy_type_usable) {
    /* This test verifies we can use the legacy Enemy type from defs.h */
    Enemy e;
    e.enemy_type = 0;
    e.enemy_x_float = 100.0f;
    e.enemy_y_float = 200.0f;
    e.enemy_active = 1;
    e.enemy_health = 500;

    /* Verify the legacy struct works */
    ASSERT(e.enemy_type == 0, "can set enemy_type");
    ASSERT(e.enemy_x_float == 100.0f, "can set enemy_x_float");
    ASSERT(e.enemy_active == 1, "can set enemy_active");
}

/*============================================================================
 * MAIN
 *============================================================================*/

int main(void) {
    printf("=== Opaque Bridge Interface Tests ===\n\n");
    printf("These tests verify the bridge works without exposing Game internals\n\n");

    RUN_TEST(create_destroy_game);
    RUN_TEST(seed_random);
    RUN_TEST(start_level);
    RUN_TEST(sync_to_globals);
    RUN_TEST(sync_from_globals);
    RUN_TEST(outcome_detection);
    RUN_TEST(legacy_enemy_type_usable);

    printf("\n=====================================\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("=====================================\n");

    if (tests_failed == 0) {
        printf("ALL TESTS PASSED\n");
        printf("\nThe opaque bridge interface successfully avoids type conflicts!\n");
        printf("main.c can now use bridge_opaque.h without including game.h\n");
        return 0;
    } else {
        printf("SOME TESTS FAILED\n");
        return 1;
    }
}
