#ifndef _DEFS_H_
#define _DEFS_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Screen and Display Constants */
#define SCREEN_WIDTH            1280
#define SCREEN_HEIGHT           800
#define SCREEN_BPP              32

#define MAP_WIDTH               3200
#define MAP_HEIGHT              2400

#define MINIMAP_WIDTH           128
#define MINIMAP_HEIGHT          96

#define PLAYER_WIDTH            128
#define PLAYER_HEIGHT           128

#define MAXPLAYERHEALTH         10000
#define PLAYER_HEALTH_BAR_HEIGHT 10

#define PLAYER_ROTATE_SPEED     4
#define PLAYER_START_ANGLE      -64

#define PLAYER_IMG_FILE_WIDTH   PLAYER_WIDTH
#define PLAYER_IMG_FILE_HEIGHT  (PLAYER_HEIGHT * 256)

#define SHAKE_FACTOR            40

/* Enemy Constants */
#define MAX_ENEMIES             100

/* Particle System Constants */
#define MAX_PARTICLES           10000
#define SMALL_PARTICLE_WIDTH    32
#define SMALL_PARTICLE_HEIGHT   32
#define LARGE_PARTICLE_WIDTH    64
#define LARGE_PARTICLE_HEIGHT   64
#define PARTICLE_STRUCT_SIZE    46
#define SMALL_PARTICLE          0
#define LARGE_PARTICLE          1

#define NUM_SMALL_FLARE_TYPES   16
#define NUM_LARGE_FLARE_TYPES   10

#define SMALL_FLARE_FILE_WIDTH  32
#define SMALL_FLARE_FILE_HEIGHT 512
#define LARGE_FLARE_FILE_WIDTH  64
#define LARGE_FLARE_FILE_HEIGHT 640

/* Player States */
#define PLAYER_NORMAL           0
#define PLAYER_DEAD             1
#define PLAYER_WIN              2

#define NUM_NUKES               4

/* Enemy Definitions */
#define RANDSPAWNDIVISOR        2
#define MASS_HEALTH_MULTIPLIER  20
#define ENEMYSHIFTSIZE          16
#define SMALLENEMYMEMSIZE       (128*128*4*16*4)
#define BOSSENEMYMEMSIZE        (256*256*4*16*4)

#define INNER_RADIUS_CUTOFF     300
#define OUTER_RADIUS_CUTOFF     2500

#define MAX_ENEMY_TURNING_SPEED 4
#define MAX_ENEMY_SPEED         4

/* Sound Definitions */
#define DMABUFFSIZE             2000
#define SOUND_KILL              1
#define SOUND_END               0
#define SOUND_REPEAT            0x80
#define SOUND_NOREPEAT          0x00

/* Menu Definitions */
#define MENU_LOAD_SHIRE         2
#define MENU_LOAD_MORDOR        7
#define MENU_LOAD_MIDKEMIA      5
#define MENU_LOAD_ARCHIPELAGO   3
#define MENU_LOAD_DUNE          4
#define MENU_LOAD_OCEANIA       6

#define CURSOR_WIDTH            64
#define CURSOR_HEIGHT           64

/* Camera Constants */
#define CAMERA_FREEDOM_RADIUS   150.0f  /* Player can move this far from camera center before camera follows */

/* Body Collision Constants */
#define PLAYER_BODY_RADIUS      40.0f   /* Actual body collision radius */
#define SMALL_ENEMY_BODY_RADIUS 50.0f   /* Small enemy collision radius */
#define BOSS_BODY_RADIUS        114.0f  /* Boss collision radius */
#define BODY_COLLISION_DAMAGE_SCALE  0.5f   /* Damage = relative_speed * scale */
#define BODY_COLLISION_MIN_DAMAGE    5      /* Minimum damage from any collision */
#define BODY_COLLISION_KNOCKBACK     8.0f   /* Knockback velocity multiplier */
#define PLAYER_INVULN_FRAMES         30     /* Frames of invulnerability after body hit */
#define ENEMY_INVULN_FRAMES          15     /* Shorter for enemies */

/* Structure Definitions */

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
