/*
 * constants.h - Named Constants for The Alan Parsons Project
 *
 * Every magic number from the original code, given a name and a home.
 * Constants are organized by domain, documented with intent.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

/*============================================================================
 * TIME
 *============================================================================*/

/* Target frame rate - physics tuned for this rate */
#define TARGET_FPS              60.0f

/* Convert frame-based value to time-based: multiply by (dt * TARGET_FPS) */
/* At 60 FPS: dt = 1/60, so dt * 60 = 1.0 (same as before) */
/* At 144 FPS: dt = 1/144, so dt * 60 ≈ 0.417 (smoother motion) */

/*============================================================================
 * DISPLAY
 *============================================================================*/

/* Screen dimensions */
#define SCREEN_WIDTH            1280
#define SCREEN_HEIGHT           800
#define SCREEN_BPP              32

/* Map dimensions (the toroidal world) */
#define MAP_WIDTH               3200
#define MAP_HEIGHT              2400

/* Minimap in top-left corner */
#define MINIMAP_WIDTH           128
#define MINIMAP_HEIGHT          96
#define MINIMAP_X               16
#define MINIMAP_Y               20

/* Visual effects */
#define SHAKE_INTENSITY         40      /* Pixels of screen shake (+-20px per frame, cumulative) */

/*============================================================================
 * PLAYER
 *============================================================================*/

/* Ship dimensions */
#define PLAYER_WIDTH            128
#define PLAYER_HEIGHT           128
#define PLAYER_SPRITE_FRAMES    256     /* One frame per angle */
#define PLAYER_IMG_FILE_WIDTH   PLAYER_WIDTH
#define PLAYER_IMG_FILE_HEIGHT  (PLAYER_HEIGHT * PLAYER_SPRITE_FRAMES)

/* Physics */
#define PLAYER_MAX_HEALTH       10000
#define PLAYER_ACCELERATION     1.07f
#define PLAYER_FRICTION         0.17f
#define PLAYER_MAX_SPEED        16.0f
#define PLAYER_MIN_SPEED        -12.0f  /* Reverse speed */
#define PLAYER_STRAFE_FRICTION  0.20f
#define PLAYER_MAX_STRAFE       12.0f
#define PLAYER_MIN_STRAFE       -12.0f
#define PLAYER_MASS             100.0f
#define PLAYER_ROTATE_SPEED     4       /* Max angular velocity (legacy compatibility) */
#define PLAYER_START_ANGLE      (-64)   /* Facing up (north) */

/* Player angular momentum */
#define PLAYER_ANGULAR_ACCEL    1.5f    /* Torque applied when turning */
#define PLAYER_ANGULAR_FRICTION 0.15f   /* Angular drag/damping */
#define PLAYER_MAX_ANGULAR_VEL  6.0f    /* Maximum rotation speed */

/* Collision points (distance from center) */
#define PLAYER_NOSE_OFFSET      60.0f
#define PLAYER_TAIL_OFFSET      60.0f
#define PLAYER_COLLISION_RADIUS 80.0f   /* Bounding box for quick collision check */
#define PLAYER_BODY_RADIUS      40.0f   /* Actual body collision radius */

/* Body collision physics */
#define BODY_COLLISION_DAMAGE_SCALE  0.5f   /* Damage = relative_speed * scale */
#define BODY_COLLISION_MIN_DAMAGE    5      /* Minimum damage from any collision */
#define BODY_COLLISION_KNOCKBACK     8.0f   /* Knockback velocity multiplier */
#define PLAYER_INVULN_FRAMES         30     /* Frames of invulnerability after body hit */
#define ENEMY_INVULN_FRAMES          15     /* Shorter for enemies */

/* UI */
#define PLAYER_HEALTH_BAR_HEIGHT 10

/* Weapons */
#define PLAYER_MAX_WEAPON_LEVEL 5
#define PLAYER_NUKE_COUNT       4
#define PLAYER_NUKE_COOLDOWN    25      /* Frames between nukes */

/*============================================================================
 * ENEMIES
 *============================================================================*/

/* Population limits */
#define MAX_ENEMIES             250
#define MAX_WAVES_PER_LEVEL     20

/* Enemy sprite dimensions */
#define SMALL_ENEMY_WIDTH       128
#define SMALL_ENEMY_HEIGHT      128
#define BOSS_WIDTH              256
#define BOSS_HEIGHT             256
#define ENEMY_SPRITE_FRAMES     16      /* Per type */
#define ENEMY_TYPES             4       /* Type 0-3 */

/* Memory sizes for sprite sheets */
#define SMALL_ENEMY_MEM_SIZE    (SMALL_ENEMY_WIDTH * SMALL_ENEMY_HEIGHT * 4 * ENEMY_SPRITE_FRAMES * ENEMY_TYPES)
#define BOSS_MEM_SIZE           (BOSS_WIDTH * BOSS_HEIGHT * 4 * ENEMY_SPRITE_FRAMES * ENEMY_TYPES)

/*----------------------------------------------------------------------------
 * AI BEHAVIOR SWITCHES (compile-time)
 *
 * The C port added some "elegant" behaviors that changed gameplay feel.
 * These switches disable them to restore original ASM behavior.
 *----------------------------------------------------------------------------*/

/* Original: enemies just coast inside inner radius, no active orbit */
#define CLASSIC_AI_NO_ORBIT         1

/* Original: no inter-enemy repulsion, they can stack */
#define CLASSIC_AI_NO_REPULSION     1

/* Original: direct angle snapping, no angular momentum (prevents vertigo) */
#define CLASSIC_AI_DIRECT_TURNING   1

/*----------------------------------------------------------------------------
 * Enemy AI
 *----------------------------------------------------------------------------*/
#define ENEMY_INNER_RADIUS      300     /* Stop accelerating when this close */
#define ENEMY_OUTER_RADIUS      2500    /* AI activation range */
#define ENEMY_MAX_TURN_RATE     4       /* Max angle change per frame (original) */
#define ENEMY_MAX_SPEED         4       /* Component-wise velocity cap */
#define ENEMY_GRAVITY_CONSTANT  450.0f  /* Gravitational attraction to player */

#if !CLASSIC_AI_NO_REPULSION
#define ENEMY_REPULSION_RADIUS  150.0f  /* Enemies repel within this range */
#define ENEMY_REPULSION_FORCE   800.0f  /* Strength of inter-enemy repulsion */
#endif

#if !CLASSIC_AI_DIRECT_TURNING
/* Angular momentum (disabled by default - causes vertigo) */
#define ENEMY_ANGULAR_ACCEL     0.8f
#define ENEMY_ANGULAR_FRICTION  0.12f
#endif

/* Health multiplier: mass * this = health */
#define ENEMY_HEALTH_MULTIPLIER 20

/* Spawn timing */
#define SPAWN_BASE_DELAY        800     /* Frames before first wave */
#define SPAWN_RANDOM_DIVISOR    2       /* Right-shift for random delay component */

/*----------------------------------------------------------------------------
 * Small Enemy Fire Rates
 *
 * ORIGINAL: All small enemies fired at ~50% per frame (rand & 0xFF >= 0x7F)
 * The type-differentiated rates were an invention that made enemies too passive.
 *----------------------------------------------------------------------------*/
#define ENEMY_SMALL_FIRE_RATE   20      /* 20% per frame - balanced between original 50% and too passive */

/* Keep type-specific traits for variety (speed, gravity, etc.) */
#define ENEMY_SCOUT_SPEED_MULT  1.3f    /* Faster projectiles */
#define ENEMY_SCOUT_GRAVITY     1.4f    /* More attracted to player */
#define ENEMY_SCOUT_MAX_SPEED   (ENEMY_MAX_SPEED + 3)

#define ENEMY_STANDARD_GRAVITY  1.0f

#define ENEMY_TANK_SPEED_MULT   0.7f    /* Slower projectiles */
#define ENEMY_TANK_DAMAGE       18      /* Higher damage */
#define ENEMY_TANK_GRAVITY      0.6f    /* Less attracted */
#define ENEMY_TANK_MAX_SPEED    (ENEMY_MAX_SPEED - 2)

#define ENEMY_HUNTER_SPREAD     12      /* Angle spread for 3-shot burst */
#define ENEMY_HUNTER_GRAVITY    1.1f

/*----------------------------------------------------------------------------
 * Boss Characteristics - Progressive Difficulty
 *
 * ORIGINAL: Regular bosses all fired at 60%, SHIMDOG at 20%.
 * Progression came from more projectiles per volley and shockwave attacks.
 * We keep slight fire rate progression for extra challenge on later levels.
 *----------------------------------------------------------------------------*/
#define BOSS_TYPE_0_RANGE       560
#define BOSS_TYPE_0_FIRE_RATE   60      /* Original: 60% */
#define BOSS_TYPE_0_SPREAD      5       /* 5-way spread shot */

#define BOSS_TYPE_1_RANGE       700
#define BOSS_TYPE_1_FIRE_RATE   65      /* Slightly harder */

#define BOSS_TYPE_2_RANGE       850
#define BOSS_TYPE_2_FIRE_RATE   70      /* Harder still */
#define BOSS_TYPE_2_SHOCKWAVE   4       /* Original: 4% when firing (was 12% - way too high) */

/* SHIMDOG - Final boss, fires less often but 4-way + shockwaves */
#define SHIMDOG_RANGE           1000
#define SHIMDOG_FIRE_RATE       20      /* Original: 20% (was 60% - way too high) */
#define SHIMDOG_PROJECTILES     4       /* Cardinal directions */
#define SHIMDOG_SHOCKWAVE       6       /* Original: 6% when firing */

/* Collision bounds */
#define SMALL_ENEMY_COLLISION   50
#define BOSS_COLLISION          114

/*============================================================================
 * PARTICLES
 *============================================================================*/

/* System limits */
#define MAX_PARTICLES           10000

/* Particle sprite dimensions */
#define SMALL_PARTICLE_WIDTH    32
#define SMALL_PARTICLE_HEIGHT   32
#define LARGE_PARTICLE_WIDTH    64
#define LARGE_PARTICLE_HEIGHT   64

/* Number of flare types in sprite sheets */
#define SMALL_FLARE_TYPES       16
#define LARGE_FLARE_TYPES       10

/* Sprite sheet dimensions */
#define SMALL_FLARE_FILE_WIDTH  32
#define SMALL_FLARE_FILE_HEIGHT 512     /* 16 types * 32 height */
#define LARGE_FLARE_FILE_WIDTH  64
#define LARGE_FLARE_FILE_HEIGHT 640     /* 10 types * 64 height */

/* Particle size types */
#define PARTICLE_SMALL          0
#define PARTICLE_LARGE          1

/* Collision layer markers */
#define COLLISION_NONE          0
#define COLLISION_PLAYER_OWNED  1       /* Player projectiles, damage enemies */
#define COLLISION_ENEMY_OWNED   2       /* Enemy projectiles, damage player */
#define COLLISION_NEUTRAL       3       /* Explosions, nukes - damage all */

/*============================================================================
 * WEAPONS
 *============================================================================*/

/* Projectile speeds */
#define WEAPON_SPEED_BASE       9.2f
#define WEAPON_SPEED_1          10.32f
#define WEAPON_SPEED_2          12.88f
#define WEAPON_SPEED_3          14.742f
#define WEAPON_SPEED_4          15.8821f
#define WEAPON_SPEED_5          16.2332f
#define WEAPON_SPEED_6          17.1243f
#define WEAPON_SPEED_7          18.533f

/* Enemy firing speeds */
#define ENEMY_FIRE_SPEED_0      12.6f
#define ENEMY_FIRE_SPEED_1      14.5331f
#define ENEMY_FIRE_SPEED_2      16.31f
#define ENEMY_FIRE_SPEED_3      18.2f
#define ENEMY_FIRE_SPEED_4      7.3112f  /* Shockwave speed */

/* Thruster particle speeds */
#define THRUSTER_MAIN_SPEED     5.32f
#define THRUSTER_STRAFE_SPEED   13.32f

/*============================================================================
 * GAME STATE
 *============================================================================*/

/* Player outcome flags */
#define GAME_PLAYING            0
#define GAME_PLAYER_DEAD        1
#define GAME_PLAYER_WIN         2

/* Menu return values */
#define MENU_EXIT               0
#define MENU_RESUME             1
#define MENU_SHIRE              2
#define MENU_ARCHIPELAGO        3
#define MENU_DUNE               4
#define MENU_MIDKEMIA           5
#define MENU_OCEANIA            6
#define MENU_MORDOR             7

/* Victory/defeat screen timing */
#define OUTCOME_DISPLAY_FRAMES  420     /* 7 seconds at 60fps */

/* Ending sequence */
#define ENDING_DELAY_FRAMES     250
#define STORY_CLIP_COUNT        4
#define STORY_CLIP_WIDTH        640
#define STORY_CLIP_HEIGHT       480

/*============================================================================
 * UI AND MENU
 *============================================================================*/

#define CURSOR_WIDTH            64
#define CURSOR_HEIGHT           64
#define MENU_FADE_FRAMES        80
#define MENU_FADE_SPEED         3

/* Level button unlock requirements */
#define LEVEL_SHIRE_UNLOCK      0
#define LEVEL_ARCHIPELAGO_UNLOCK 1
#define LEVEL_DUNE_UNLOCK       2
#define LEVEL_MIDKEMIA_UNLOCK   3
#define LEVEL_OCEANIA_UNLOCK    4
#define LEVEL_MORDOR_UNLOCK     5

/*============================================================================
 * AUDIO
 *============================================================================*/

#define AUDIO_SAMPLE_RATE       22050
#define AUDIO_BUFFER_SIZE       4096
#define AUDIO_CHANNELS          16      /* Mixing channels */

#define MUSIC_VOLUME            110
#define SFX_HIT_VOLUME          10
#define SFX_WEAPON_VOLUME       80
#define SFX_EXPLOSION_VOLUME    128

/*============================================================================
 * HELPER MACROS
 *============================================================================*/

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(x, lo, hi) (MIN(MAX(x, lo), hi))

#endif /* CONSTANTS_H */
