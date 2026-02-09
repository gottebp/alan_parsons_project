/*
 * game.h - The Game Context
 *
 * A single structure containing all game state.
 * This is the heart of the game - all state lives here,
 * all systems read and write through this.
 *
 * No more scattered globals. No more hidden mutations.
 * Just one clear source of truth.
 */

#ifndef GAME_H
#define GAME_H

#include "../core/types.h"
#include "../core/pool.h"
#include "../core/constants.h"

/*============================================================================
 * ENTITY POOLS
 *============================================================================*/

/* Define pools for enemies and particles using the pool macros */
DEFINE_POOL(enemy, Enemy, MAX_ENEMIES)
DEFINE_POOL(particle, Particle, MAX_PARTICLES)

/*============================================================================
 * WAVE SYSTEM
 *============================================================================*/

typedef struct {
    int frame_trigger;      /* Frame number when this wave spawns */
    int first_enemy_idx;    /* Index into enemy pool of first enemy */
    int enemy_count;        /* Number of enemies in this wave */
} Wave;

typedef struct {
    Wave waves[MAX_WAVES_PER_LEVEL];
    int wave_count;
    int current_wave;
    int frame_counter;
    int boss_index;         /* Index of boss in enemy pool, or -1 */
    int boss_spawned;       /* Has the boss been activated? */
} WaveSystem;

/*============================================================================
 * THE GAME CONTEXT
 *============================================================================*/

typedef struct {
    /*------------------------------------------------------------------------
     * STATE
     *------------------------------------------------------------------------*/
    GameState state;
    int outcome_timer;      /* Frames remaining on victory/defeat screen */
    int menu_requested;     /* 1 = should show menu next frame */
    int menu_result;        /* Result from last menu: 0=quit, 2-7=level */
    MenuState menu;         /* Non-blocking menu state */

    /*------------------------------------------------------------------------
     * PROGRESS
     *------------------------------------------------------------------------*/
    int unlocked_level;     /* Highest level player has reached */
    int weapons_level;      /* Current weapon upgrade level */
    int captain_planet;     /* Extra nukes mode */
    int current_level_idx;  /* Index into LEVELS array */

    /*------------------------------------------------------------------------
     * ENTITIES
     *------------------------------------------------------------------------*/
    Player player;
    enemyPool enemies;
    particlePool particles;

    /*------------------------------------------------------------------------
     * WAVE SPAWNING
     *------------------------------------------------------------------------*/
    WaveSystem waves;

    /*------------------------------------------------------------------------
     * VISUAL EFFECTS
     *------------------------------------------------------------------------*/
    int shake_frames;       /* Remaining screen shake */

    /*------------------------------------------------------------------------
     * TIMING
     *------------------------------------------------------------------------*/
    uint32_t frame_count;   /* Total frames since level start */

    /*------------------------------------------------------------------------
     * RANDOM STATE
     *------------------------------------------------------------------------*/
    uint32_t rand_state;    /* Mersenne twister state (or simple LCG) */

    /*------------------------------------------------------------------------
     * AUDIO EVENTS (set by game, cleared by app after playing sounds)
     *------------------------------------------------------------------------*/
    int audio_player_hit;       /* Player took damage this frame */
    int audio_enemy_destroyed;  /* Enemy was destroyed this frame */
    int audio_nuke_fired;       /* Nuke was fired this frame */

} Game;

/*============================================================================
 * GAME LIFECYCLE
 *============================================================================*/

/* Initialize a fresh game context */
void game_init(Game* game);

/* Start a new level */
void game_start_level(Game* game, int level_index);

/* Main update - call once per frame */
void game_update(Game* game, const InputState* input, float dt);

/* Check for victory/defeat conditions */
void game_check_outcome(Game* game);

/* Reset player for new level */
void game_reset_player(Game* game);

/*============================================================================
 * PLAYER OPERATIONS
 *============================================================================*/

/* Update player based on input */
void player_update(Player* player, const InputState* input, float dt);

/* Apply physics (friction, position update, wrapping) */
void player_physics(Player* player, float dt);

/* Fire weapons based on current level */
void player_fire_weapons(Game* game);

/* Drop a nuke if available */
void player_fire_nuke(Game* game);

/* Fire main thrusters (creates particles behind ship) */
void player_fire_main_thrusters(Game* game);

/* Fire left strafe thrusters */
void player_fire_left_thrusters(Game* game);

/* Fire right strafe thrusters */
void player_fire_right_thrusters(Game* game);

/* Handle player collision with enemy projectile */
void player_take_damage(Game* game, int damage, Vec2 impact_pos);

/* Create death explosion */
void player_explode(Game* game);

/*============================================================================
 * ENEMY OPERATIONS
 *============================================================================*/

/* Spawn enemies for current wave */
void enemies_spawn_wave(Game* game, int wave_index);

/* Update all enemy AI and physics */
void enemies_update(Game* game, float dt);

/* Create a new enemy in the pool */
Enemy* enemy_spawn(Game* game, EnemyType type, EnemySize size, Vec2 position, float mass);

/* Enemy takes damage, potentially dies */
void enemy_damage(Game* game, Enemy* enemy, int damage, Vec2 impact_pos);

/* Enemy death explosion */
void enemy_explode(Game* game, Enemy* enemy);

/*============================================================================
 * PARTICLE OPERATIONS
 *============================================================================*/

/* Spawn a particle with angle and speed */
Particle* particle_spawn(Game* game, int collision_layer, ParticleSize size,
                         int flare_index, Vec2 position, uint8_t angle,
                         float speed, int lifetime, int damage);

/* Spawn a particle with explicit velocity */
Particle* particle_spawn_vec(Game* game, int collision_layer, ParticleSize size,
                             int flare_index, Vec2 position, Vec2 velocity,
                             int lifetime, int damage);

/* Update all particles (age, position, wrapping) */
void particles_update(Game* game, float dt);

/*============================================================================
 * COLLISION DETECTION
 *============================================================================*/

/* Check all collisions and apply damage */
void collisions_check(Game* game);

/* Check player-enemy body collisions (ramming) */
void collisions_check_bodies(Game* game);

/* Check if particle hits player */
int collision_particle_player(const Particle* p, const Player* player);

/* Check if particle hits enemy */
int collision_particle_enemy(const Particle* p, const Enemy* enemy);

/*============================================================================
 * WAVE SPAWNING
 *============================================================================*/

/* Initialize wave system for a level */
void waves_init(Game* game, int level_index);

/* Check if it's time to spawn the next wave */
void waves_update(Game* game);

/* Activate the boss */
void waves_spawn_boss(Game* game);

/*============================================================================
 * EFFECTS
 *============================================================================*/

/* Trigger screen shake */
void effect_screen_shake(Game* game, int frames);

/* Create small explosion (20 loops, no damage) */
void effect_explosion_small(Game* game, Vec2 position);

/* Create large explosion (100 loops, damage) */
void effect_explosion_large(Game* game, Vec2 position);

/* Create hit sparks (smoke puffs) */
void effect_hit_sparks(Game* game, Vec2 position, int large);

/* Create nuke shockwave */
void effect_nuke(Game* game, Vec2 position);

/*============================================================================
 * RANDOM NUMBERS
 *============================================================================*/

/* Initialize random state */
void game_rand_seed(Game* game, uint32_t seed);

/* Get next random number */
uint32_t game_rand(Game* game);

/* Get random in range [0, max) */
uint32_t game_rand_range(Game* game, uint32_t max);

/* Get random float [0, 1) */
float game_rand_float(Game* game);

/*============================================================================
 * SAVE/LOAD
 *============================================================================*/

/* Save progress to storage */
void game_save_progress(const Game* game);

/* Load progress from storage */
void game_load_progress(Game* game);

/*============================================================================
 * MENU (non-blocking)
 *============================================================================*/

/* Initialize menu state for showing menu */
void menu_enter(Game* game);

/* Update menu - returns non-zero when menu is complete */
int menu_update(Game* game, const InputState* input);

/* Render menu to screen */
void menu_render(const Game* game);

#endif /* GAME_H */
