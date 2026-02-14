/*
 * types.h - Game Entity Types
 *
 * All the data structures that represent game entities.
 * Clean, coherent, with related data grouped together.
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include "../math/vec2.h"
#include "constants.h"

/*============================================================================
 * PLAYER
 *============================================================================*/

typedef struct {
    /* Position and motion */
    Vec2 position;
    Vec2 velocity;          /* Accumulated from direction * speed + strafe */
    float speed;            /* Forward/backward speed */
    float strafe_speed;     /* Lateral speed */
    uint8_t angle;          /* 256-angle facing direction */
    float angle_remainder;  /* Sub-unit turn accumulator for fractional rotate speeds */
    float angular_vel;      /* Angular velocity for smooth turning */
    int8_t turn_direction;  /* -1 left, 0 straight, +1 right (for sprite selection) */

    /* Collision points (updated each frame) */
    Vec2 nose;
    Vec2 tail;

    /* Status */
    int health;
    int weapons_level;
    int nukes_remaining;
    int nuke_cooldown;      /* Frames until next nuke available */
    int invuln_frames;      /* Frames of invulnerability remaining (body collision) */

    /* Physics constants (could be per-ship-type in future) */
    float acceleration;
    float friction;
    float max_speed;
    float min_speed;
    float strafe_friction;
    float max_strafe;
    float min_strafe;
    float angular_accel;    /* Torque for turning */
    float angular_friction; /* Angular drag */
    float max_angular_vel;  /* Maximum turn rate */
    float mass;
} Player;

/*============================================================================
 * ENEMIES
 *============================================================================*/

typedef enum {
    ENEMY_SCOUT     = 0,    /* Fast, weak, high fire rate */
    ENEMY_STANDARD  = 1,    /* Balanced */
    ENEMY_TANK      = 2,    /* Slow, strong, heavy damage */
    ENEMY_HUNTER    = 3,    /* Spread shot */
} EnemyType;

typedef enum {
    ENEMY_SIZE_SMALL = 0,
    ENEMY_SIZE_BOSS  = 1,
} EnemySize;

typedef struct {
    /* Type and status */
    EnemyType type;
    EnemySize size;
    int active;             /* 1 = active, 0 = inactive/dead */

    /* Position and motion */
    Vec2 position;
    Vec2 velocity;
    uint8_t angle;          /* 256-angle facing direction */
    float angular_vel;      /* Angular velocity for smooth turning */

    /* Combat */
    float mass;
    int health;
    int invuln_frames;      /* Frames of invulnerability remaining (body collision) */

    /* Pool management (see entity.h) */
    int next_free;          /* Index of next free slot, or -1 */
} Enemy;

/*============================================================================
 * PARTICLES
 *============================================================================*/

typedef enum {
    PARTICLE_SIZE_SMALL = 0,
    PARTICLE_SIZE_LARGE = 1,
} ParticleSize;

typedef struct {
    /* Status */
    int active;             /* 1 = active, 0 = free */
    int collision_layer;    /* COLLISION_NONE, _PLAYER_OWNED, _ENEMY_OWNED, _NEUTRAL */

    /* Appearance */
    ParticleSize size;
    int flare_index;        /* Which sprite in the flare sheet */

    /* Position and motion */
    Vec2 position;
    Vec2 velocity;

    /* Lifetime (float for smooth dt scaling) */
    float age;
    int max_age;

    /* Combat */
    int damage;

    /* Pool management */
    int next_free;
} Particle;

/*============================================================================
 * LEVELS
 *============================================================================*/

typedef struct {
    const char* name;
    const char* map_file;
    const char* music_file;
    int wave_count;
    int enemies_per_wave;
    EnemyType boss_type;
    float boss_x;
    float boss_y;
    float boss_mass;
    int unlock_level;       /* What game_level unlocks this */
} LevelDef;

/*
 * Level definitions - restored to original enemy counts.
 * The C port had roughly doubled these values, but with fixed fire rates
 * (50% vs the previous 2-5%) the original counts should provide proper challenge.
 */
static const LevelDef LEVELS[] = {
    {
        .name = "Shire",
        .map_file = "./data/shire.bmp",
        .music_file = "./sound/sound_track1.ogg",
        .wave_count = 3,
        .enemies_per_wave = 8,      /* Original: 8 (total: 24) */
        .boss_type = ENEMY_SCOUT,
        .boss_x = 400.0f,
        .boss_y = 400.0f,
        .boss_mass = 300.0f,
        .unlock_level = 0,
    },
    {
        .name = "Archipelago",
        .map_file = "./data/archipelago.bmp",
        .music_file = "./sound/sound_track2.ogg",
        .wave_count = 4,
        .enemies_per_wave = 10,     /* Original: 10 (total: 40) */
        .boss_type = ENEMY_STANDARD,
        .boss_x = 400.0f,
        .boss_y = 400.0f,
        .boss_mass = 400.0f,
        .unlock_level = 1,
    },
    {
        .name = "Dune",
        .map_file = "./data/dune.bmp",
        .music_file = "./sound/sound_track3.ogg",
        .wave_count = 2,
        .enemies_per_wave = 30,     /* Original: 30 (total: 60) */
        .boss_type = ENEMY_STANDARD,
        .boss_x = 400.0f,
        .boss_y = 400.0f,
        .boss_mass = 450.0f,
        .unlock_level = 2,
    },
    {
        .name = "Midkemia",
        .map_file = "./data/midkemia.bmp",
        .music_file = "./sound/sound_track4.ogg",
        .wave_count = 7,
        .enemies_per_wave = 10,     /* Original: 10 (total: 70) */
        .boss_type = ENEMY_TANK,
        .boss_x = 400.0f,
        .boss_y = 400.0f,
        .boss_mass = 500.0f,
        .unlock_level = 3,
    },
    {
        .name = "Oceania",
        .map_file = "./data/oceania.bmp",
        .music_file = "./sound/sound_track5.ogg",
        .wave_count = 4,
        .enemies_per_wave = 20,     /* Original: 20 (total: 80) */
        .boss_type = ENEMY_TANK,
        .boss_x = 400.0f,
        .boss_y = 400.0f,
        .boss_mass = 650.0f,
        .unlock_level = 4,
    },
    {
        .name = "Mordor",
        .map_file = "./data/mordor.bmp",
        .music_file = "./sound/sound_track6.ogg",
        .wave_count = 4,
        .enemies_per_wave = 24,     /* Original: 24 (total: 96) */
        .boss_type = ENEMY_HUNTER,  /* SHIMDOG */
        .boss_x = 400.0f,
        .boss_y = 400.0f,
        .boss_mass = 800.0f,
        .unlock_level = 5,
    },
};

#define LEVEL_COUNT (sizeof(LEVELS) / sizeof(LEVELS[0]))

/*============================================================================
 * WAVE SPAWNING
 *============================================================================*/

typedef struct {
    int frame;              /* Frame number when this wave activates */
    int first_enemy;        /* Index of first enemy in this wave */
    int enemy_count;        /* Number of enemies in this wave */
} WaveInfo;

/*============================================================================
 * WEAPON DEFINITIONS
 *============================================================================*/

typedef struct {
    ParticleSize particle_size;
    int flare_index;
    float speed;
    int damage;
    int lifetime;
    int spread_angle;       /* 0 = straight, positive = random spread */
    int cooldown_frames;    /* 0 = every frame, >0 = limited */
} WeaponDef;

/* Player weapon definitions by level */
static const WeaponDef PLAYER_WEAPONS[] = {
    /* Level 0: Base weapon */
    { PARTICLE_SIZE_SMALL, 10, 10.32f, 12, 30, 0, 0 },
    /* Level 1: Spread shot */
    { PARTICLE_SIZE_SMALL, 7, 14.742f, 4, 30, 5, 0 },
    /* Level 2: Side shots (cooldown 4) */
    { PARTICLE_SIZE_LARGE, 6, 9.2f, 1, 20, 20, 4 },
    /* Level 3: Sweeping shots */
    { PARTICLE_SIZE_SMALL, 12, 9.2f, 4, 30, 0, 0 },  /* Special handling */
    /* Level 4: Perpendicular shots (cooldown 10) */
    { PARTICLE_SIZE_SMALL, 15, 9.2f, 3, 22, 64, 10 },
    /* Level 5: Shockwave (cooldown 40) */
    { PARTICLE_SIZE_SMALL, 6, 15.8821f, 2, 10, 0, 40 },  /* Special handling */
};

/*============================================================================
 * INPUT
 *============================================================================*/

typedef struct {
    /* Digital inputs */
    int up, down, left, right;
    int fire, nuke;
    int strafe_left, strafe_right;
    int escape;

    /* Mouse */
    int mouse_x, mouse_y;
    int mouse_left, mouse_right;

    /* Mobile twin-stick inputs */
    float stick_left_x;         /* -1 (strafe left) to +1 (strafe right) */
    float stick_left_y;         /* -1 (thrust) to +1 (reverse) */
    int target_angle;           /* 0-255: desired ship angle from right stick */
    int target_angle_active;    /* 1 if right stick is being touched */
    int mobile_active;
} InputState;

/*============================================================================
 * MENU STATE (for non-blocking menu)
 *============================================================================*/

typedef enum {
    MENU_PHASE_FADE_IN,
    MENU_PHASE_ACTIVE,
    MENU_PHASE_FADE_OUT,
    MENU_PHASE_DONE,
} MenuPhase;

typedef struct {
    MenuPhase phase;
    int fade_frame;         /* Current fade frame */
    int fade_total;         /* Total fade frames */
    int selected_level;     /* Level selected (0-5), or -1 for exit */
    int hover_button;       /* Currently hovered button, or -1 */
    int click_pending;      /* Waiting for mouse release */
} MenuState;

/*============================================================================
 * GAME STATE
 *============================================================================*/

typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_VICTORY,
    STATE_DEFEAT,
    STATE_ENDING,
} GameState;

typedef struct {
    GameState state;

    /* Progress */
    int unlocked_level;     /* Highest unlocked level (0-5) */
    int current_level;      /* Currently playing level index */

    /* In-level state */
    int wave_frame;         /* Frame counter for wave spawning */
    int current_wave;       /* Current wave index */
    int outcome_timer;      /* Countdown for victory/defeat screen */
    int shake_frames;       /* Remaining screen shake frames */

    /* Captain Planet mode */
    int captain_planet;     /* 1 = 18 nukes instead of 4 */
} GameProgress;

/*============================================================================
 * RENDERING
 *============================================================================*/

typedef struct {
    int x, y;
    int width, height;
} Rect;

/* Camera follows player, centered on screen */
typedef struct {
    Vec2 position;          /* Center of view in world coords */
    Rect viewport;          /* Screen area to render to */
} Camera;

/*============================================================================
 * HELPER FUNCTIONS
 *============================================================================*/

/* Initialize player with default values */
static inline void player_init(Player* p) {
    p->position = vec2(MAP_WIDTH / 2.0f, MAP_HEIGHT / 2.0f);
    p->velocity = vec2_zero();
    p->speed = 0.0f;
    p->strafe_speed = 0.0f;
    p->angle = (uint8_t)PLAYER_START_ANGLE;
    p->angular_vel = 0.0f;
    p->turn_direction = 0;
    p->nose = p->position;
    p->tail = p->position;
    p->health = PLAYER_MAX_HEALTH;
    p->weapons_level = 0;
    p->nukes_remaining = PLAYER_NUKE_COUNT;
    p->nuke_cooldown = 0;

    p->acceleration = PLAYER_ACCELERATION;
    p->friction = PLAYER_FRICTION;
    p->max_speed = PLAYER_MAX_SPEED;
    p->min_speed = PLAYER_MIN_SPEED;
    p->strafe_friction = PLAYER_STRAFE_FRICTION;
    p->max_strafe = PLAYER_MAX_STRAFE;
    p->min_strafe = PLAYER_MIN_STRAFE;
    p->angular_accel = PLAYER_ANGULAR_ACCEL;
    p->angular_friction = PLAYER_ANGULAR_FRICTION;
    p->max_angular_vel = PLAYER_MAX_ANGULAR_VEL;
    p->mass = PLAYER_MASS;
}

/*============================================================================
 * ENEMY TYPE CHARACTERISTICS
 *============================================================================*/

typedef struct {
    float mass;
    float gravity_mult;     /* Multiplier for gravitational attraction */
    int max_speed;
} EnemyTypeDef;

/*============================================================================
 * SMALL ENEMY FIRING CHARACTERISTICS
 *
 * Fire rate is now unified (ENEMY_SMALL_FIRE_RATE in constants.h) to match
 * original ASM behavior where all small enemies fired at ~50% per frame.
 * Type-specific traits (speed, damage, spread) still differentiate them.
 *============================================================================*/

typedef struct {
    float speed_mult;       /* Multiplier for projectile speed */
    int damage;             /* Damage per projectile */
    int lifetime;           /* Projectile lifetime in frames */
    ParticleSize size;      /* Particle size for projectiles */
    int spread_count;       /* 1 = single shot, 3 = triple shot, etc. */
    int spread_angle;       /* Angle between spread shots */
} SmallEnemyFireDef;

static const SmallEnemyFireDef SMALL_ENEMY_FIRE_DEFS[] = {
    [ENEMY_SCOUT] = {
        .speed_mult = ENEMY_SCOUT_SPEED_MULT,
        .damage = 6,
        .lifetime = 60,
        .size = PARTICLE_SIZE_SMALL,
        .spread_count = 1,
        .spread_angle = 0,
    },
    [ENEMY_STANDARD] = {
        .speed_mult = 1.0f,
        .damage = 10,
        .lifetime = 80,
        .size = PARTICLE_SIZE_LARGE,
        .spread_count = 1,
        .spread_angle = 0,
    },
    [ENEMY_TANK] = {
        .speed_mult = ENEMY_TANK_SPEED_MULT,
        .damage = ENEMY_TANK_DAMAGE,
        .lifetime = 120,
        .size = PARTICLE_SIZE_LARGE,
        .spread_count = 1,
        .spread_angle = 0,
    },
    [ENEMY_HUNTER] = {
        .speed_mult = 1.0f,
        .damage = 8,
        .lifetime = 80,
        .size = PARTICLE_SIZE_LARGE,
        .spread_count = 3,
        .spread_angle = ENEMY_HUNTER_SPREAD,
    },
};

/* Get small enemy firing characteristics */
static inline const SmallEnemyFireDef* enemy_fire_def(EnemyType type) {
    return &SMALL_ENEMY_FIRE_DEFS[type];
}

static const EnemyTypeDef ENEMY_TYPE_DEFS[] = {
    [ENEMY_SCOUT]    = { 35.0f, ENEMY_SCOUT_GRAVITY,    ENEMY_SCOUT_MAX_SPEED },
    [ENEMY_STANDARD] = { 40.0f, ENEMY_STANDARD_GRAVITY, ENEMY_MAX_SPEED },
    [ENEMY_TANK]     = { 60.0f, ENEMY_TANK_GRAVITY,     ENEMY_TANK_MAX_SPEED },
    [ENEMY_HUNTER]   = { 80.0f, ENEMY_HUNTER_GRAVITY,   ENEMY_MAX_SPEED },
};

/* Get enemy mass by type */
static inline float enemy_mass_for_type(EnemyType type) {
    return ENEMY_TYPE_DEFS[type].mass;
}

/* Get gravity multiplier by type */
static inline float enemy_gravity_for_type(EnemyType type) {
    return ENEMY_TYPE_DEFS[type].gravity_mult;
}

/* Get max speed by type */
static inline int enemy_max_speed_for_type(EnemyType type) {
    return ENEMY_TYPE_DEFS[type].max_speed;
}

/*============================================================================
 * BOSS TYPE CHARACTERISTICS
 *============================================================================*/

typedef struct {
    int firing_range;       /* Distance at which boss will fire */
    int fire_rate;          /* Percent chance per frame to fire */
    int shockwave_chance;   /* Percent chance for shockwave attack (0 = none) */
    int spread_count;       /* Number of projectiles in spread pattern */
    int volley_damage;      /* Damage when player overlaps boss during volley */
    const char* name;       /* Boss name for display/debugging */
} BossTypeDef;

static const BossTypeDef BOSS_TYPE_DEFS[] = {
    [ENEMY_SCOUT] = {
        .firing_range = BOSS_TYPE_0_RANGE,
        .fire_rate = BOSS_TYPE_0_FIRE_RATE,
        .shockwave_chance = 0,
        .spread_count = BOSS_TYPE_0_SPREAD,
        .volley_damage = 20,
        .name = "Scout Boss"
    },
    [ENEMY_STANDARD] = {
        .firing_range = BOSS_TYPE_1_RANGE,
        .fire_rate = BOSS_TYPE_1_FIRE_RATE,
        .shockwave_chance = 0,
        .spread_count = 0,
        .volley_damage = 36,
        .name = "Standard Boss"
    },
    [ENEMY_TANK] = {
        .firing_range = BOSS_TYPE_2_RANGE,
        .fire_rate = BOSS_TYPE_2_FIRE_RATE,
        .shockwave_chance = BOSS_TYPE_2_SHOCKWAVE,
        .spread_count = 0,
        .volley_damage = 38,
        .name = "Tank Boss"
    },
    [ENEMY_HUNTER] = {
        .firing_range = SHIMDOG_RANGE,
        .fire_rate = SHIMDOG_FIRE_RATE,
        .shockwave_chance = SHIMDOG_SHOCKWAVE,
        .spread_count = SHIMDOG_PROJECTILES,
        .volley_damage = 40,
        .name = "SHIMDOG"
    },
};

/* Get firing range for boss type */
static inline int boss_firing_range(EnemyType type) {
    return BOSS_TYPE_DEFS[type].firing_range;
}

/* Get firing rate for boss type (percent chance per frame) */
static inline int boss_fire_rate(EnemyType type) {
    return BOSS_TYPE_DEFS[type].fire_rate;
}

/* Get shockwave chance for boss type */
static inline int boss_shockwave_chance(EnemyType type) {
    return BOSS_TYPE_DEFS[type].shockwave_chance;
}

/* Get spread count for boss type */
static inline int boss_spread_count(EnemyType type) {
    return BOSS_TYPE_DEFS[type].spread_count;
}

/* Get volley damage for boss type (contact auto-hit) */
static inline int boss_volley_damage(EnemyType type) {
    return BOSS_TYPE_DEFS[type].volley_damage;
}

/* Get boss name for display */
static inline const char* boss_name(EnemyType type) {
    return BOSS_TYPE_DEFS[type].name;
}

#endif /* TYPES_H */
