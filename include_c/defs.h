#ifndef _DEFS_H_
#define _DEFS_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* All game constants defined in one place */
#include "core/constants.h"

/*============================================================================
 * LEGACY ALIASES
 * Old names still used by legacy modules (player.c, enemy.c, ai.c, etc.)
 *============================================================================*/

#define MAXPLAYERHEALTH         PLAYER_MAX_HEALTH
#define SHAKE_FACTOR            SHAKE_INTENSITY
#define NUM_SMALL_FLARE_TYPES   SMALL_FLARE_TYPES
#define NUM_LARGE_FLARE_TYPES   LARGE_FLARE_TYPES
#define PARTICLE_STRUCT_SIZE    46
#define NUM_NUKES               PLAYER_NUKE_COUNT

#define RANDSPAWNDIVISOR        SPAWN_RANDOM_DIVISOR
#define MASS_HEALTH_MULTIPLIER  ENEMY_HEALTH_MULTIPLIER
#define ENEMYSHIFTSIZE          ENEMY_SPRITE_FRAMES
#define SMALLENEMYMEMSIZE       SMALL_ENEMY_MEM_SIZE
#define BOSSENEMYMEMSIZE        BOSS_MEM_SIZE

#define INNER_RADIUS_CUTOFF     ENEMY_INNER_RADIUS
#define OUTER_RADIUS_CUTOFF     ENEMY_OUTER_RADIUS
#define MAX_ENEMY_TURNING_SPEED ENEMY_MAX_TURN_RATE
#define MAX_ENEMY_SPEED         ENEMY_MAX_SPEED

/* Player States */
#define PLAYER_NORMAL           GAME_PLAYING
#define PLAYER_DEAD             GAME_PLAYER_DEAD
#define PLAYER_WIN              GAME_PLAYER_WIN

/* Sound Definitions */
#define DMABUFFSIZE             2000
#define SOUND_KILL              1
#define SOUND_END               0
#define SOUND_REPEAT            0x80
#define SOUND_NOREPEAT          0x00

/* Menu Definitions */
#define MENU_LOAD_SHIRE         MENU_SHIRE
#define MENU_LOAD_MORDOR        MENU_MORDOR
#define MENU_LOAD_MIDKEMIA      MENU_MIDKEMIA
#define MENU_LOAD_ARCHIPELAGO   MENU_ARCHIPELAGO
#define MENU_LOAD_DUNE          MENU_DUNE
#define MENU_LOAD_OCEANIA       MENU_OCEANIA

/* Camera Constants */
#define CAMERA_FREEDOM_RADIUS   150.0f

/* Legacy collision radii */
#define SMALL_ENEMY_BODY_RADIUS 50.0f
#define BOSS_BODY_RADIUS        114.0f

/*============================================================================
 * STRUCTURE DEFINITIONS
 *============================================================================*/

/* Enemy Structure */
typedef struct {
    uint32_t enemy_type;
    float enemy_x_float;
    float enemy_y_float;
    float enemy_x_vel_float;
    float enemy_y_vel_float;
    float enemy_mass;
    float enemy_angular_vel;  /* Angular velocity for smooth turning */
    uint8_t enemy_angle;
    uint8_t enemy_size;
    int8_t enemy_active;  /* Can be -1 (inactive) or 1 (active) */
    uint8_t padding;
    int32_t enemy_x_int;   /* SIGNED - used with signed comparisons (jl/jg) in assembly */
    int32_t enemy_y_int;   /* SIGNED - used with signed comparisons (jl/jg) in assembly */
    int32_t enemy_health;  /* SIGNED - can go negative when killed */
    int32_t enemy_invuln_frames;  /* Frames of invulnerability after body collision */
} Enemy;

/* NOTE: Particle structure is defined in ppe.h as PARTICLE (46 bytes) */

/* Global Constants */
extern const float flt_deg_to_rad_360;
extern const float flt_rad_to_deg_360;
extern const float flt_deg_to_rad_256;
extern const float flt_rad_to_deg_256;

/* Lookup Tables */
extern float sin_look[256];
extern float cos_look[256];

/* Sound Effect Counters */
extern int snd_engines_counter;
extern int snd_weapon_counter;

#endif /* _DEFS_H_ */
