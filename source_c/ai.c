/*
 * AI system - enemy movement AI and special effects
 * Converted from x86 assembly
 */

#include "ai.h"
#include "enemy.h"
#include "player.h"
#include "ppe.h"
#include "rand.h"
#include "defs.h"
#include "sdl_wrapper.h"
#include <math.h>

/* AI constants */
#define G 450.0f
#define INNER_RADIUS_CUTOFF_SQ (INNER_RADIUS_CUTOFF * INNER_RADIUS_CUTOFF)
#define OUTER_RADIUS_CUTOFF_SQ (OUTER_RADIUS_CUTOFF * OUTER_RADIUS_CUTOFF)

extern float SIN_LOOK[256];
extern float COS_LOOK[256];

/* Sound effects - exported from main.c */
extern Mix_Chunk* snd_effect_explosion[5];

/* Enemy firing rate constants - mirrors assembly ai.asm:97-101 */
float fltEnemyFiringRate[5] = {12.6f, 14.5331f, 16.31f, 18.2f, 7.3112f};

/* Particle speed constants - mirrors assembly ai.asm:103-107 */
float fltPSpeed1 = 4.2f;
float fltPSpeed2 = 5.33f;
float fltPSpeed3 = 6.112f;
float fltPSpeed4 = 7.5f;
float fltPSpeed5 = 8.82f;

/*
 * AI movement for enemies - gravitational attraction to player
 */
void EnemyMove(void) {
    extern float fltPlayerMass;
    float g_mass = fltPlayerMass * G;

    for (int i = 0; i < 100; i++) {
        if (Enemies[i].enemy_type == (uint32_t)-1) break;
        if (Enemies[i].enemy_active == -1) continue;

        /* Calculate delta X considering map wrap - mirrors assembly lines 153-175 */
        float delta_x1_flt = fltPlayerX - Enemies[i].enemy_x_float;
        int delta_x1_int = lrintf(delta_x1_flt);  /* fistp on line 161 */

        float delta_x2_flt;
        int delta_x2_int;

        /* Assembly checks the INT version (line 164-166) */
        if (delta_x1_int >= 0) {
            delta_x2_flt = delta_x1_flt - MAP_WIDTH;  /* Line 171 */
        } else {
            delta_x2_flt = delta_x1_flt + MAP_WIDTH;  /* Line 168 */
        }
        delta_x2_int = (int)lrintf(fabsf(delta_x2_flt));  /* fabs + fistp on lines 174-175 */

        /* Choose shortest distance - mirrors assembly lines 279-288 */
        /* NOTE: Must compare absolute values, not signed vs unsigned! */
        int delta_x1_abs = (delta_x1_int < 0) ? -delta_x1_int : delta_x1_int;
        float delta_x = (delta_x1_abs < delta_x2_int) ? delta_x1_flt : delta_x2_flt;

        /* Calculate delta Y considering map wrap - mirrors assembly lines 179-201 */
        float delta_y1_flt = fltPlayerY - Enemies[i].enemy_y_float;
        int delta_y1_int = lrintf(delta_y1_flt);  /* fistp on line 187 */

        float delta_y2_flt;
        int delta_y2_int;

        /* Assembly checks the INT version (line 190-192) */
        if (delta_y1_int >= 0) {
            delta_y2_flt = delta_y1_flt - MAP_HEIGHT;  /* Line 197 */
        } else {
            delta_y2_flt = delta_y1_flt + MAP_HEIGHT;  /* Line 194 */
        }
        delta_y2_int = (int)lrintf(fabsf(delta_y2_flt));  /* fabs + fistp on lines 200-201 */

        /* Choose shortest distance - mirrors assembly lines 290-299 */
        /* NOTE: Must compare absolute values, not signed vs unsigned! */
        int delta_y1_abs = (delta_y1_int < 0) ? -delta_y1_int : delta_y1_int;
        float delta_y = (delta_y1_abs < delta_y2_int) ? delta_y1_flt : delta_y2_flt;

        /* Calculate distance squared - mirrors assembly lines 301-312 */
        float radius_sq = delta_x * delta_x + delta_y * delta_y;
        int radius_int = lrintf(sqrtf(radius_sq));  /* Matches fistp at line 312 */

        /* Calculate gravitational force - mirrors assembly lines 399-404 */
        /* Assembly: fld [_RadiusFlt] loads R^2, then divides G_Mass by R^2 */
        float radius = sqrtf(radius_sq);
        float force = g_mass / radius_sq;  /* Force = G*M / R^2 */

        /* Calculate acceleration components - mirrors assembly lines 412-427 */
        /* Assembly normalizes delta by dividing by R, then multiplies by force */
        float norm_x = delta_x / radius;  /* Normalized X direction */
        float norm_y = delta_y / radius;  /* Normalized Y direction */
        float acc_x = norm_x * force;
        float acc_y = norm_y * force;

        /* Apply acceleration to velocity - mirrors assembly lines 433-441 */
        float new_vel_x = Enemies[i].enemy_x_vel_float + acc_x;
        float new_vel_y = Enemies[i].enemy_y_vel_float + acc_y;

        /* INNER_RADIUS_CUTOFF check - mirrors assembly lines 429-431
         * IMPORTANT: Assembly jumps to .Done which STILL allows firing!
         * Only velocity updates are skipped when too close, NOT firing!
         * This was incorrectly using 'continue' which skipped firing entirely. */
        if (radius_int >= INNER_RADIUS_CUTOFF) {
            /* Only update velocity if NOT too close - mirrors assembly lines 433-467 */

            /* Limit velocity COMPONENT-WISE (not magnitude!) */
            /* NOTE: Assembly has bug - only limits positive velocities (uses jg without abs)
             * This causes negative velocities to grow unbounded. Fixed here to use abs() */
            int vel_x_int = lrintf(fabsf(new_vel_x));  /* Use absolute value */
            if (vel_x_int <= MAX_ENEMY_SPEED) {
                Enemies[i].enemy_x_vel_float = new_vel_x;
            }

            int vel_y_int = lrintf(fabsf(new_vel_y));  /* Use absolute value */
            if (vel_y_int <= MAX_ENEMY_SPEED) {
                Enemies[i].enemy_y_vel_float = new_vel_y;
            }
        }
        /* If radius < INNER_RADIUS_CUTOFF, skip velocity updates but CONTINUE to firing check */

        /* Calculate angle to player - mirrors assembly lines 340-346 */
        /* Assembly uses fpatan then multiplies by fltRadToDeg256 and uses fistp for rounding */
        float angle_rad = atan2f(delta_y, delta_x);
        extern float fltRadToDeg256;
        uint8_t target_angle = (uint8_t)lrintf(angle_rad * fltRadToDeg256);

        /* Enemy turning rate limiter - mirrors assembly lines 357-393 EXACTLY
         * Assembly logic:
         *   cx = current - target (as signed byte)
         *   if cx > MAX_ENEMY_TURNING_SPEED: goto DecreaseTurningAngle
         *   neg cx (cx = target - current)
         *   if cx < MAX_ENEMY_TURNING_SPEED: goto DontChangeTurningAngle
         *   bl += MAX_ENEMY_TURNING_SPEED * 2 (fall through to DecreaseTurningAngle)
         * DecreaseTurningAngle:
         *   bl -= MAX_ENEMY_TURNING_SPEED
         * DontChangeTurningAngle:
         *   store bl
         *
         * This means: NO snapping to target - only increment/decrement by MAX_ENEMY_TURNING_SPEED
         */
        uint8_t current_angle = Enemies[i].enemy_angle;
        int8_t angle_diff = (int8_t)(current_angle - target_angle);  /* Signed byte like assembly */

        if (angle_diff > MAX_ENEMY_TURNING_SPEED) {
            /* Turn clockwise toward target - mirrors .DecreaseTurningAngle */
            Enemies[i].enemy_angle = current_angle - MAX_ENEMY_TURNING_SPEED;
        } else if (-angle_diff >= MAX_ENEMY_TURNING_SPEED) {
            /* Turn counter-clockwise toward target - mirrors add bl, MAX*2 then sub bl, MAX */
            Enemies[i].enemy_angle = current_angle + MAX_ENEMY_TURNING_SPEED;
        }
        /* else: don't change angle - mirrors .DontChangeTurningAngle (no snap!) */

        /* Enemy firing logic - mirrors assembly lines 478-516 */
        if ((int)radius < 400) {  /* Boss firing radius: 400 (not 400*400!) - mirrors line 478 */
            if (Enemies[i].enemy_size == 1) {
                /* Boss weapons - mirrors assembly lines 536-668 */
                if (Enemies[i].enemy_type != 3) {
                    /* Regular bosses (types 0, 1, 2) - mirrors assembly lines 540-604 */
                    uint32_t fire_check = Rand() % 100;
                    /* Assembly uses < 60, NOT >= 60! (line 544-545: cmp edx,60 / jae near .NoFire) */
                    if (fire_check < 60) {
                        uint8_t angle = Enemies[i].enemy_angle;

                        /* All boss types fire main weapon - mirrors lines 548-551 */
                        AddParticle(3, LARGE_PARTICLE, 5, Enemies[i].enemy_x_float, Enemies[i].enemy_y_float,
                                  angle, fltEnemyFiringRate[1], 30, 4);

                        /* Boss type 1+ fires 3 additional weapons - mirrors lines 554-576 */
                        if (Enemies[i].enemy_type >= 1) {
                            AddParticle(3, LARGE_PARTICLE, 7, Enemies[i].enemy_x_float, Enemies[i].enemy_y_float,
                                      angle + 10, fltEnemyFiringRate[1], 30, 6);
                            AddParticle(3, LARGE_PARTICLE, 7, Enemies[i].enemy_x_float, Enemies[i].enemy_y_float,
                                      angle - 10, fltEnemyFiringRate[1], 30, 6);
                            AddParticle(3, SMALL_PARTICLE, 8, Enemies[i].enemy_x_float, Enemies[i].enemy_y_float,
                                      angle, fltEnemyFiringRate[3], 30, 10);
                        }

                        /* Boss type 2+ has ~4.1% chance for shockwave - mirrors lines 578-603 */
                        if (Enemies[i].enemy_type >= 2) {
                            uint32_t shock_check = Rand() % 1000;
                            /* Assembly: cmp edx,40 / ja .NoShockWeapon - fires if edx <= 40 */
                            if (shock_check <= 40) {  /* 4.1% chance (41/1000) */
                                /* Fire 256 particles in a circle - mirrors lines 592-600 */
                                for (int shock_angle = 0; shock_angle < 256; shock_angle++) {
                                    AddParticle(3, SMALL_PARTICLE, 8, Enemies[i].enemy_x_float, Enemies[i].enemy_y_float,
                                              shock_angle, fltEnemyFiringRate[4], 100, 1);
                                }
                            }
                        }
                    }
                } else {
                    /* SHIMDOG special boss (type 3) - mirrors assembly lines 607-668 */
                    uint32_t fire_check = Rand() % 100;
                    /* Assembly uses < 20, NOT >= 60! (line 614-615: cmp edx,20 / jae near .NoFire) */
                    if (fire_check < 20) {
                        uint8_t angle = Enemies[i].enemy_angle;
                        /* Fire 4 projectiles at cardinal directions - mirrors lines 618-644 */
                        AddParticle(3, LARGE_PARTICLE, 9, Enemies[i].enemy_x_float, Enemies[i].enemy_y_float,
                                  angle, fltEnemyFiringRate[0], 30, 10);
                        AddParticle(3, LARGE_PARTICLE, 9, Enemies[i].enemy_x_float, Enemies[i].enemy_y_float,
                                  angle + 64, fltEnemyFiringRate[0], 30, 10);
                        AddParticle(3, LARGE_PARTICLE, 9, Enemies[i].enemy_x_float, Enemies[i].enemy_y_float,
                                  angle + 128, fltEnemyFiringRate[0], 30, 10);
                        AddParticle(3, LARGE_PARTICLE, 9, Enemies[i].enemy_x_float, Enemies[i].enemy_y_float,
                                  angle - 64, fltEnemyFiringRate[0], 30, 10);

                        /* SHIMDOG has ~6.1% chance for shockwave - mirrors lines 647-665 */
                        uint32_t shock_check = Rand() % 1000;
                        /* Assembly: cmp edx,60 / ja .NoShimDogShockWeapon - fires if edx <= 60 */
                        if (shock_check <= 60) {  /* 6.1% chance (61/1000) */
                            for (int shock_angle = 0; shock_angle < 256; shock_angle++) {
                                AddParticle(3, LARGE_PARTICLE, 9, Enemies[i].enemy_x_float, Enemies[i].enemy_y_float,
                                          shock_angle, fltEnemyFiringRate[4], 100, 1);
                            }
                        }
                    }
                }
            } else {
                /* Regular enemy weapons - fire about 50% of the time - mirrors assembly lines 485-513 */
                uint32_t fire_check = Rand() & 0xFF;
                if (fire_check >= 0x7F) {
                    /* Calculate firing velocity: enemy velocity + firing direction */
                    uint8_t angle_idx = Enemies[i].enemy_angle;
                    float fire_x_vel = Enemies[i].enemy_x_vel_float +
                                     (COS_LOOK[angle_idx] * fltEnemyFiringRate[0]);
                    float fire_y_vel = Enemies[i].enemy_y_vel_float +
                                     (SIN_LOOK[angle_idx] * fltEnemyFiringRate[0]);

                    /* Use enemy type + 4 as flare index */
                    int flare_idx = Enemies[i].enemy_type + 4;

                    AddParticleByVector(
                        2,                              /* detect_collisions = 2 for enemy bullets */
                        LARGE_PARTICLE,                 /* img_size_type */
                        flare_idx,                      /* img_index */
                        Enemies[i].enemy_x_float,       /* x */
                        Enemies[i].enemy_y_float,       /* y */
                        fire_x_vel,                     /* x_vel */
                        fire_y_vel,                     /* y_vel */
                        80,                             /* max_life */
                        10                              /* damage */
                    );
                }
            }
        }
    }
}

/*
 * Helper to play random explosion sound - mirrors assembly pattern at lines 774-781, 850-884
 */
static void PlayExplosionSound(void) {
    int sound_idx = Rand() % 5;
    if (snd_effect_explosion[sound_idx]) {
        Mix_PlayChannelTimed(-1, snd_effect_explosion[sound_idx], 0, -1);
    }
}

/*
 * Create ship explosion effect - mirrors assembly _ShipExplode (ai.asm:718-940)
 * The assembly uses different particle configurations based on size and hit parameters:
 * - Small explosions (size=0, hit=0): 20 loops x 3 particles = 60 particles, large flares, no damage
 * - Big explosions (size=1, hit=0): 100 loops x 6 particles = 600 particles, mixed flares, damage=100
 * - Hit effects (hit=1): 3-6 particles, smoke puffs, no damage
 */
void ShipExplode(float x, float y, int large, int damage) {
    extern float fltPSpeed1, fltPSpeed2, fltPSpeed3, fltPSpeed4;
    float fltTemp3 = 5.3f;

    if (damage == 1) {
        /* Hit effect - creates smoke puffs - mirrors assembly lines 888-934 */
        uint8_t angle = Rand() % 256;

        AddParticle(0, SMALL_PARTICLE, 14, x, y, angle, fltTemp3, 15, 0);
        AddParticle(0, LARGE_PARTICLE, 2, x, y, angle + 85, fltTemp3, 15, 0);
        AddParticle(0, LARGE_PARTICLE, 3, x, y, angle + 170, fltTemp3, 15, 0);

        /* Extra particles when large==10 - mirrors assembly lines 914-933 */
        if (large == 10) {
            AddParticle(0, SMALL_PARTICLE, 14, x, y, angle + 193, fltTemp3, 15, 0);  /* +23 from 170 */
            AddParticle(0, SMALL_PARTICLE, 14, x, y, angle + 261, fltTemp3, 15, 0);  /* +68 from 193 */
            AddParticle(0, SMALL_PARTICLE, 14, x, y, angle + 353, fltTemp3, 15, 0);  /* +92 from 261 - WAS MISSING! */
        }
    } else if (large == 1) {
        /* Big explosion - 100 loops, 6 particles each - mirrors assembly lines 785-886 */
        for (int i = 0; i < 100; i++) {
            AddParticle(1, SMALL_PARTICLE, 4, x, y, Rand() % 256, fltPSpeed1, 200, 100);
            AddParticle(1, SMALL_PARTICLE, 4, x, y, Rand() % 256, fltPSpeed3, 200, 100);
            AddParticle(1, LARGE_PARTICLE, 5, x, y, Rand() % 256, fltPSpeed2, 210, 100);
            AddParticle(1, SMALL_PARTICLE, 5, x, y, Rand() % 256, fltPSpeed1, 220, 100);
            AddParticle(1, SMALL_PARTICLE, 5, x, y, Rand() % 256, fltPSpeed3, 220, 100);
            AddParticle(1, SMALL_PARTICLE, 6, x, y, Rand() % 256, fltPSpeed4, 230, 100);
        }

        /* Big explosion plays 4 random explosion sounds - mirrors assembly lines 850-884 */
        PlayExplosionSound();
        PlayExplosionSound();
        PlayExplosionSound();
        PlayExplosionSound();
    } else {
        /* Small explosion - 20 loops, 3 particles each - mirrors assembly lines 737-783 */
        for (int i = 0; i < 20; i++) {
            AddParticle(0, LARGE_PARTICLE, 3, x, y, Rand() % 256, fltPSpeed1, 10, 0);
            AddParticle(0, LARGE_PARTICLE, 7, x, y, Rand() % 256, fltPSpeed2, 20, 0);
            AddParticle(0, LARGE_PARTICLE, 8, x, y, Rand() % 256, fltPSpeed3, 30, 0);
        }

        /* Small explosion plays 1 random explosion sound - mirrors assembly lines 774-781 */
        PlayExplosionSound();
    }
}

/*
 * Drop a nuke - massive circular shockwave - mirrors assembly _DropNuke (ai.asm:675-710)
 * Creates 4 particles per angle increment in a circular pattern
 *
 * Assembly loop structure (lines 683-708):
 *   edx = 0 (angle)
 * .ShockLoop:
 *   AddParticle(..., edx, fltEnemyFiringRate[4], ...)
 *   add edx, 1
 *   AddParticle(..., edx, fltEnemyFiringRate[2], ...)
 *   add edx, 2
 *   AddParticle(..., edx, fltEnemyFiringRate[1], ...)
 *   add edx, 3
 *   AddParticle(..., edx, fltEnemyFiringRate[0], ...)
 *   add edx, 2  <-- increment by 2 after all 4 particles
 *   cmp edx, 256
 *   jb .ShockLoop
 *
 * So angles are: 0,1,3,6 then 8,9,11,14 then 16,17,19,22...
 * Total: 128 iterations * 4 particles = 512 particles
 */
void DropNuke(float x, float y) {
    int edx = 0;  /* Use assembly variable name for clarity */

    while (edx < 256) {
        /* First particle at angle edx - mirrors line 687 */
        AddParticle(1, LARGE_PARTICLE, 5, x, y, (uint8_t)edx, fltEnemyFiringRate[4], 100, 20);

        edx += 1;  /* mirrors line 691: add edx, 1 */

        /* Second particle at angle edx - mirrors line 692 */
        AddParticle(1, SMALL_PARTICLE, 15, x, y, (uint8_t)edx, fltEnemyFiringRate[2], 100, 20);

        edx += 2;  /* mirrors line 697: add edx, 2 */

        /* Third particle at angle edx - mirrors line 698 */
        AddParticle(1, LARGE_PARTICLE, 5, x, y, (uint8_t)edx, fltEnemyFiringRate[1], 100, 20);

        edx += 3;  /* mirrors line 702: add edx, 3 */

        /* Fourth particle at angle edx - mirrors line 703 */
        AddParticle(1, SMALL_PARTICLE, 15, x, y, (uint8_t)edx, fltEnemyFiringRate[0], 100, 20);

        edx += 2;  /* mirrors line 706: add edx, 2 */
    }
}
