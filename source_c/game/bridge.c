/*
 * bridge.c - Bridge Between New and Old Architecture
 *
 * This module syncs state between the new Game struct architecture
 * and the legacy global variables. It allows incremental migration.
 *
 * IMPORTANT: This file ONLY includes game.h (which has new types).
 * We declare legacy globals as extern with compatible signatures.
 * The legacy Enemy from defs.h is replicated here as BridgeEnemy
 * to avoid type conflicts.
 */

#include "../../include_c/game/game.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*============================================================================
 * EXTERNAL REFERENCES TO OLD GLOBALS
 *============================================================================*/

/* Player globals from player.c */
extern int intPlayerX, intPlayerY;
extern float fltPlayerX, fltPlayerY;
extern float fltPlayerSpeed, fltPlayerStrafeSpeed;
extern int8_t intbPlayerAngle, intbPlayerTurnDir;
extern int intPlayerHealth;
extern int intPlayerWeaponsLevel;
extern float fltPlayerNoseX, fltPlayerNoseY;
extern float fltPlayerTailX, fltPlayerTailY;
extern int intPlayerNoseX, intPlayerNoseY;
extern int intPlayerTailX, intPlayerTailY;
extern float fltPlayerAngularVel;
extern int intPlayerInvulnFrames;

/*
 * BridgeEnemy - Compatible with defs.h Enemy struct layout.
 * We define it here instead of including defs.h to avoid
 * conflict with types.h Enemy.
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
    int8_t enemy_active;  /* -1 = inactive, 1 = active */
    uint8_t padding;
    int32_t enemy_x_int;
    int32_t enemy_y_int;
    int32_t enemy_health;
    int32_t enemy_invuln_frames;
} BridgeEnemy;

/* Enemy array from enemy.c - legacy uses 100, not MAX_ENEMIES from new architecture */
#define LEGACY_MAX_ENEMIES 100
extern BridgeEnemy Enemies[LEGACY_MAX_ENEMIES];

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

/* Particle array from ppe.c */
extern BridgePARTICLE* ParticleDataOff;
extern int NumParticles;

/* Map/shake state from mapeng.c */
extern int intShakeMap;
extern float fltCameraX, fltCameraY;
extern int intCameraX, intCameraY;

/* Wave system from enemy.c */
extern uint32_t FrameNum;
extern uint32_t NextWaveNumber;

/*============================================================================
 * PLAYER SYNC
 *============================================================================*/

void bridge_sync_player_to_globals(const Game* game) {
    const Player* p = &game->player;

    fltPlayerX = p->position.x;
    fltPlayerY = p->position.y;
    intPlayerX = (int)p->position.x;
    intPlayerY = (int)p->position.y;

    fltPlayerSpeed = p->speed;
    fltPlayerStrafeSpeed = p->strafe_speed;
    intbPlayerAngle = (int8_t)p->angle;
    intbPlayerTurnDir = (int8_t)p->turn_direction;
    intPlayerHealth = p->health;
    intPlayerWeaponsLevel = p->weapons_level;

    fltPlayerNoseX = p->nose.x;
    fltPlayerNoseY = p->nose.y;
    intPlayerNoseX = (int)p->nose.x;
    intPlayerNoseY = (int)p->nose.y;

    fltPlayerTailX = p->tail.x;
    fltPlayerTailY = p->tail.y;
    intPlayerTailX = (int)p->tail.x;
    intPlayerTailY = (int)p->tail.y;

    fltPlayerAngularVel = p->angular_vel;
    intPlayerInvulnFrames = p->invuln_frames;
}

void bridge_sync_player_from_globals(Game* game) {
    Player* p = &game->player;

    p->position.x = fltPlayerX;
    p->position.y = fltPlayerY;
    p->speed = fltPlayerSpeed;
    p->strafe_speed = fltPlayerStrafeSpeed;
    p->angle = (uint8_t)intbPlayerAngle;
    p->turn_direction = intbPlayerTurnDir;
    p->health = intPlayerHealth;
    p->weapons_level = intPlayerWeaponsLevel;

    p->nose.x = fltPlayerNoseX;
    p->nose.y = fltPlayerNoseY;
    p->tail.x = fltPlayerTailX;
    p->tail.y = fltPlayerTailY;

    p->angular_vel = fltPlayerAngularVel;
    p->invuln_frames = intPlayerInvulnFrames;
}

/*============================================================================
 * ENEMY SYNC
 *============================================================================*/

void bridge_sync_enemies_to_globals(const Game* game) {
    /* Clear all legacy enemies first */
    for (int i = 0; i < LEGACY_MAX_ENEMIES; i++) {
        Enemies[i].enemy_active = -1;  /* Sentinel for inactive */
    }

    /* Copy active enemies from new pool to legacy array */
    int legacy_idx = 0;
    POOL_FOREACH_CONST(&game->enemies, e, Enemy, {
        if (legacy_idx >= LEGACY_MAX_ENEMIES) break;

        BridgeEnemy* le = &Enemies[legacy_idx];
        le->enemy_type = (uint32_t)e->type;
        le->enemy_x_float = e->position.x;
        le->enemy_y_float = e->position.y;
        le->enemy_x_vel_float = e->velocity.x;
        le->enemy_y_vel_float = e->velocity.y;
        le->enemy_mass = e->mass;
        le->enemy_angular_vel = e->angular_vel;
        le->enemy_angle = e->angle;
        le->enemy_size = (uint8_t)e->size;
        le->enemy_active = 1;
        le->enemy_x_int = (int32_t)e->position.x;
        le->enemy_y_int = (int32_t)e->position.y;
        le->enemy_health = e->health;
        le->enemy_invuln_frames = e->invuln_frames;

        legacy_idx++;
    });
}

void bridge_sync_enemies_from_globals(Game* game) {
    /* Clear the new enemy pool */
    enemy_pool_clear(&game->enemies);

    /* Copy active legacy enemies to new pool */
    for (int i = 0; i < LEGACY_MAX_ENEMIES; i++) {
        if (Enemies[i].enemy_active != 1) continue;

        BridgeEnemy* le = &Enemies[i];
        Enemy* e = enemy_pool_alloc(&game->enemies);
        if (!e) break;  /* Pool full */

        e->type = (EnemyType)le->enemy_type;
        e->position.x = le->enemy_x_float;
        e->position.y = le->enemy_y_float;
        e->velocity.x = le->enemy_x_vel_float;
        e->velocity.y = le->enemy_y_vel_float;
        e->mass = le->enemy_mass;
        e->angular_vel = le->enemy_angular_vel;
        e->angle = le->enemy_angle;
        e->size = (EnemySize)le->enemy_size;
        e->health = le->enemy_health;
        e->invuln_frames = le->enemy_invuln_frames;
    }
}

/*============================================================================
 * PARTICLE SYNC
 *============================================================================*/

void bridge_sync_particles_to_globals(const Game* game) {
    if (!ParticleDataOff) return;  /* Legacy particle system not initialized */

    /* Clear all legacy particles first */
    NumParticles = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        ParticleDataOff[i].IsActive = 0;
    }

    /* Copy active particles from new pool to legacy array */
    int legacy_idx = 0;
    POOL_FOREACH_CONST(&game->particles, p, Particle, {
        if (legacy_idx >= MAX_PARTICLES) break;

        BridgePARTICLE* lp = &ParticleDataOff[legacy_idx];
        lp->IsActive = 1;
        lp->DetectCollisions = (uint8_t)p->collision_layer;
        lp->ImgSizeType = (uint32_t)p->size;
        /* ImgOffset needs to be calculated from flare_index */
        if (p->size == PARTICLE_SIZE_SMALL) {
            lp->ImgOffset = (uint32_t)(p->flare_index * SMALL_PARTICLE_WIDTH * SMALL_PARTICLE_HEIGHT * 4);
        } else {
            lp->ImgOffset = (uint32_t)(p->flare_index * LARGE_PARTICLE_WIDTH * LARGE_PARTICLE_HEIGHT * 4);
        }
        lp->fltX = p->position.x;
        lp->fltY = p->position.y;
        lp->intX = (int)p->position.x;
        lp->intY = (int)p->position.y;
        lp->XV = p->velocity.x;
        lp->YV = p->velocity.y;
        lp->MaxLife = (uint32_t)p->max_age;
        lp->Age = (uint32_t)p->age;
        lp->Damage = (uint32_t)p->damage;

        legacy_idx++;
        NumParticles++;
    });
}

void bridge_sync_particles_from_globals(Game* game) {
    if (!ParticleDataOff) return;

    /* Clear the new particle pool */
    particle_pool_clear(&game->particles);

    /* Copy active legacy particles to new pool */
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!ParticleDataOff[i].IsActive) continue;

        BridgePARTICLE* lp = &ParticleDataOff[i];
        Particle* p = particle_pool_alloc(&game->particles);
        if (!p) break;  /* Pool full */

        p->collision_layer = (int)lp->DetectCollisions;
        p->size = (ParticleSize)lp->ImgSizeType;
        /* Calculate flare_index from ImgOffset */
        if (lp->ImgSizeType == PARTICLE_SIZE_SMALL) {
            p->flare_index = (int)(lp->ImgOffset / (SMALL_PARTICLE_WIDTH * SMALL_PARTICLE_HEIGHT * 4));
        } else {
            p->flare_index = (int)(lp->ImgOffset / (LARGE_PARTICLE_WIDTH * LARGE_PARTICLE_HEIGHT * 4));
        }
        p->position.x = lp->fltX;
        p->position.y = lp->fltY;
        p->velocity.x = lp->XV;
        p->velocity.y = lp->YV;
        p->max_age = (int)lp->MaxLife;
        p->age = (int)lp->Age;
        p->damage = (int)lp->Damage;
    }
}

/*============================================================================
 * GAME STATE SYNC
 *============================================================================*/

void bridge_sync_game_state_to_globals(const Game* game) {
    intShakeMap = game->shake_frames;

    /* Sync wave system */
    FrameNum = (uint32_t)game->waves.frame_counter;
    NextWaveNumber = (uint32_t)game->waves.current_wave;

    /* Camera follows player in legacy - update camera position */
    fltCameraX = game->player.position.x;
    fltCameraY = game->player.position.y;
    intCameraX = (int)fltCameraX;
    intCameraY = (int)fltCameraY;
}

void bridge_sync_game_state_from_globals(Game* game) {
    game->shake_frames = intShakeMap;
    game->waves.frame_counter = (int)FrameNum;
    game->waves.current_wave = (int)NextWaveNumber;
}

/*============================================================================
 * FULL SYNC
 *============================================================================*/

void bridge_sync_all_to_globals(const Game* game) {
    bridge_sync_player_to_globals(game);
    bridge_sync_enemies_to_globals(game);
    bridge_sync_particles_to_globals(game);
    bridge_sync_game_state_to_globals(game);
}

void bridge_sync_all_from_globals(Game* game) {
    bridge_sync_player_from_globals(game);
    bridge_sync_enemies_from_globals(game);
    bridge_sync_particles_from_globals(game);
    bridge_sync_game_state_from_globals(game);
}

void bridge_init_game_from_globals(Game* game) {
    game_init(game);
    bridge_sync_all_from_globals(game);
}

/*============================================================================
 * OPAQUE INTERFACE FUNCTIONS
 * These allow main.c to use the bridge without including game.h
 *============================================================================*/

Game* bridge_create_game(void) {
    Game* game = (Game*)malloc(sizeof(Game));
    if (game) {
        game_init(game);
    }
    return game;
}

void bridge_destroy_game(Game* game) {
    if (game) {
        free(game);
    }
}

void bridge_seed_random(Game* game, uint32_t seed) {
    if (game) {
        game_rand_seed(game, seed);
    }
}

void bridge_start_level(Game* game, int level_index) {
    if (game) {
        game_start_level(game, level_index);
    }
}

void bridge_set_captain_planet(Game* game, int enabled) {
    if (game) {
        game->captain_planet = enabled;
    }
}

void bridge_set_weapons_level(Game* game, int level) {
    if (game) {
        game->player.weapons_level = level;
    }
}

int bridge_get_weapons_level(const Game* game) {
    return game ? game->player.weapons_level : 0;
}

void bridge_increment_weapons_level(Game* game) {
    if (game && game->player.weapons_level < 5) {
        game->player.weapons_level++;
    }
}

int bridge_is_playing(const Game* game) {
    return game && game->state == STATE_PLAYING;
}

int bridge_get_player_health(const Game* game) {
    return game ? game->player.health : 0;
}

int bridge_get_outcome(const Game* game) {
    if (!game) return 0;
    if (game->player.health <= 0) return 1;  /* PLAYER_DEAD */
    if (game->state == STATE_VICTORY) return 2;  /* PLAYER_WIN */
    return 0;  /* Still playing */
}

int bridge_get_enemy_count(const Game* game) {
    return game ? game->enemies.count : 0;
}

float bridge_get_player_x(const Game* game) {
    return game ? game->player.position.x : 0.0f;
}

float bridge_get_player_y(const Game* game) {
    return game ? game->player.position.y : 0.0f;
}

/*============================================================================
 * MENU STATE
 *============================================================================*/

int bridge_menu_requested(const Game* game) {
    return game ? game->menu_requested : 0;
}

void bridge_clear_menu_request(Game* game) {
    if (game) {
        game->menu_requested = 0;
    }
}

void bridge_set_menu_result(Game* game, int result) {
    if (game) {
        game->menu_result = result;
    }
}

void bridge_request_menu(Game* game) {
    if (game) {
        game->menu_requested = 1;
    }
}

/*============================================================================
 * GAME UPDATE FROM LEGACY INPUT
 * Reads legacy KEYBOARD global and converts to InputState for game_update()
 *============================================================================*/

/* External reference to legacy keyboard state */
extern uint8_t KEYBOARD[320];

#ifdef __EMSCRIPTEN__
/* External references to mobile tilt controls (from input.c, EMSCRIPTEN only) */
extern float mobile_tilt_steer;
extern float mobile_tilt_thrust;
extern int mobile_controls_active;
#endif

/* SDL scancodes we need (matching input.h) */
#define KEY_UP     82
#define KEY_DOWN   81
#define KEY_LEFT   80
#define KEY_RIGHT  79
#define KEY_FIRE   27   /* X key */
#define KEY_NUKE   44   /* Space */
#define KEY_STRAFE_L 29 /* Z key */
#define KEY_STRAFE_R 6  /* C key */
#define KEY_ESCAPE 41

void bridge_update_from_legacy_input(Game* game, float dt) {
    if (!game) return;

    /* Build InputState from legacy KEYBOARD global */
    InputState input = {0};

    input.up = KEYBOARD[KEY_UP] ? 1 : 0;
    input.down = KEYBOARD[KEY_DOWN] ? 1 : 0;
    input.left = KEYBOARD[KEY_LEFT] ? 1 : 0;
    input.right = KEYBOARD[KEY_RIGHT] ? 1 : 0;
    input.fire = KEYBOARD[KEY_FIRE] ? 1 : 0;
    input.nuke = KEYBOARD[KEY_NUKE] ? 1 : 0;
    input.strafe_left = KEYBOARD[KEY_STRAFE_L] ? 1 : 0;
    input.strafe_right = KEYBOARD[KEY_STRAFE_R] ? 1 : 0;
    input.escape = KEYBOARD[KEY_ESCAPE] ? 1 : 0;

#ifdef __EMSCRIPTEN__
    /* Add mobile tilt controls (EMSCRIPTEN only) */
    input.tilt_steer = mobile_tilt_steer;
    input.tilt_thrust = mobile_tilt_thrust;
    input.mobile_active = mobile_controls_active;
#endif

    /* Call the unified game update */
    game_update(game, &input, dt);

    /* Update audio bridge with game state and input */
    extern void bridge_update_audio(Game* game, const InputState* input);
    bridge_update_audio(game, &input);
}

/*============================================================================
 * AUDIO BRIDGE INTEGRATION
 *============================================================================*/

#include "../../include_c/game/audio_bridge.h"

static AudioBridgeState audio_state;
static int audio_bridge_initialized = 0;

void bridge_update_audio(Game* game, const InputState* input) {
    if (!audio_bridge_initialized) {
        audio_bridge_init(&audio_state);
        audio_bridge_initialized = 1;
    }

    audio_bridge_update(&audio_state, game, input);
}

/* Called from main.c when level starts */
void bridge_audio_level_start(int level_index) {
    if (!audio_bridge_initialized) {
        audio_bridge_init(&audio_state);
        audio_bridge_initialized = 1;
    }
    audio_bridge_level_start(&audio_state, level_index);
}

/* Called from main.c when enemy is destroyed */
void bridge_audio_enemy_destroyed(void) {
    if (audio_bridge_initialized) {
        audio_bridge_enemy_destroyed(&audio_state);
    }
}

/* Called from main.c when player is hit */
void bridge_audio_player_hit(void) {
    if (audio_bridge_initialized) {
        audio_bridge_player_hit(&audio_state);
    }
}

/* Called from main.c on defeat */
void bridge_audio_defeat(void) {
    if (audio_bridge_initialized) {
        audio_bridge_defeat(&audio_state);
    }
}
