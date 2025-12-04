/*
 * Player module - handles player ship movement, rendering, and weapons
 * Converted from x86 assembly
 */

#include "player.h"
#include "defs.h"
#include "sdl_wrapper.h"
#include "ppe.h"
#include "rand.h"
#include "ai.h"
#include "mapeng.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL_mixer.h>

/* Player state */
int intPlayerX = 0;
int intPlayerY = 0;
float fltPlayerX = 0.0f;
float fltPlayerY = 0.0f;
float fltPlayerSpeed = 0.0f;
float fltPlayerStrafeSpeed = 0.0f;
int8_t intbPlayerAngle = PLAYER_START_ANGLE;
int8_t intbPlayerTurnDir = 0;
int intPlayerHealth = MAXPLAYERHEALTH;
int intPlayerWeaponsLevel = 0;

/* Player ship sprites */
uint32_t* PlayerShipOff = NULL;
uint32_t* PlayerShipOffL = NULL;
uint32_t* PlayerShipOffR = NULL;

/* Player physics */
float fltPlayerAccel = 1.07f;
float fltPlayerFriction = 0.17f;
float fltPlayerMaxSpeed = 16.0f;
float fltPlayerMinSpeed = -12.0f;
float fltPlayerMass = 100.0f;
float fltPlayerStrafeFriction = 0.20f;
float fltPlayerMaxStrafeSpeed = 12.0f;
float fltPlayerMinStrafeSpeed = -12.0f;

/* External sound effects - declared in main.c */
extern Mix_Chunk* snd_effect_hit;

/* Thruster speeds */
static float thruster_speed = 5.32f;
static float thruster_strafe_speed = 13.32f;

/* Weapon speeds array */
static float speed_array[8] = {9.2f, 10.32f, 12.88f, 14.742f, 15.8821f, 16.2332f, 17.1243f, 18.533f};

/* Weapon timers (need at least 9 slots based on assembly) */
static int weapon_timer_array[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int8_t weapon_mod_angle = 0;
static int8_t wma_direction = 0;

/* Player collision points - mirrors assembly lines 109-117 */
float fltPlayerNoseX = 0.0f, fltPlayerNoseY = 0.0f;
float fltPlayerTailX = 0.0f, fltPlayerTailY = 0.0f;
int intPlayerNoseX = 0, intPlayerNoseY = 0;
int intPlayerTailX = 0, intPlayerTailY = 0;

/* External references */
extern float SIN_LOOK[256];
extern float COS_LOOK[256];
extern uint32_t* ScreenOff;
extern uint8_t game_turnout;

/*
 * Load raw image file - mirrors assembly _LoadRaw (lines 781-813)
 * Assembly signature: 3 parameters (file_str, buffer, size), returns void (no eax set)
 */
void LoadRaw(const char* filename, void* buffer, size_t size) {
    /* Assembly calls fopen, fread, fclose without error checking - mirrors lines 789-792 */
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return;  /* Assembly doesn't check, but we gracefully handle error */
    }

    fread(buffer, size, 1, fp);  /* Assembly line 791: single fread call with size, count=1 */
    fclose(fp);

    /* Convert magenta pixels to transparent - mirrors assembly lines 797-809 */
    uint32_t* pixels = (uint32_t*)buffer;
    size_t pixel_count = size / 4;
    for (size_t i = 0; i < pixel_count; i++) {
        if (pixels[i] == 0xFF00FFFF) {
            pixels[i] = 0x00000000;
        }
    }
    /* Assembly returns with no value in eax - void return */
}

/*
 * Initialize player
 */
void InitPlayer(void) {
    /* Allocate player ship sprites */
    size_t ship_size = PLAYER_IMG_FILE_WIDTH * PLAYER_IMG_FILE_HEIGHT * 4;

    PlayerShipOff = (uint32_t*)malloc(ship_size);
    PlayerShipOffL = (uint32_t*)malloc(ship_size);
    PlayerShipOffR = (uint32_t*)malloc(ship_size);

    if (!PlayerShipOff || !PlayerShipOffL || !PlayerShipOffR) {
        fprintf(stderr, "Failed to allocate player ship memory\n");
        return;
    }

    /* Load ship sprites - mirrors assembly lines 182-184 */
    LoadRaw("./data/player_norm.raw", PlayerShipOff, ship_size);
    LoadRaw("./data/player_left.raw", PlayerShipOffL, ship_size);
    LoadRaw("./data/player_right.raw", PlayerShipOffR, ship_size);
}

/*
 * Destroy player - mirrors assembly lines 194-208
 */
void DestroyPlayer(void) {
    free(PlayerShipOff);
    free(PlayerShipOffL);
    free(PlayerShipOffR);
}

/*
 * Update player position and state
 */
void UpdatePlayer(void) {
    extern int snd_weapon_counter, snd_engines_counter;

    /* Check if player is dead */
    if (intPlayerHealth <= 0) {
        if (game_turnout != PLAYER_DEAD) {
            /* Trigger explosion effects - mirrors assembly lines 225-229 */
            ShipExplode(fltPlayerX, fltPlayerY, 1, 0);
            ShipExplode(fltPlayerX, fltPlayerY, 0, 0);
            ShipExplode(fltPlayerX, fltPlayerY, 1, 0);
            ShipExplode(fltPlayerX, fltPlayerY, 0, 0);
            ShakeMap(150);

            /* Stop weapon sound - mirrors assembly lines 231-235 */
            if (snd_weapon_counter > 0) {
                snd_weapon_counter--;
                Mix_HaltChannel(1);
            }

            /* Stop engine sound - mirrors assembly lines 237-241 */
            if (snd_engines_counter > 0) {
                snd_engines_counter--;
                Mix_HaltChannel(3);
            }

            game_turnout = PLAYER_DEAD;
        }
        return;
    }

    /* Get angle index for lookup tables */
    uint8_t angle_idx = (uint8_t)intbPlayerAngle;

    /* Calculate nose/tail positions BEFORE movement - mirrors assembly lines 251-285 */
    fltPlayerNoseX = fltPlayerX + (60.0f * COS_LOOK[angle_idx]);
    fltPlayerNoseY = fltPlayerY + (60.0f * SIN_LOOK[angle_idx]);
    intPlayerNoseX = lrintf(fltPlayerNoseX);  /* Matches fistp (line 258) */
    intPlayerNoseY = lrintf(fltPlayerNoseY);  /* Matches fistp (line 276) */

    fltPlayerTailX = fltPlayerX - (60.0f * COS_LOOK[angle_idx]);
    fltPlayerTailY = fltPlayerY - (60.0f * SIN_LOOK[angle_idx]);
    intPlayerTailX = lrintf(fltPlayerTailX);  /* Matches fistp (line 267) */
    intPlayerTailY = lrintf(fltPlayerTailY);  /* Matches fistp (line 285) */

    /* THEN update position based on velocity - mirrors assembly lines 288-323 */
    fltPlayerX += COS_LOOK[angle_idx] * fltPlayerSpeed;
    fltPlayerY += SIN_LOOK[angle_idx] * fltPlayerSpeed;
    intPlayerX = lrintf(fltPlayerX);  /* Matches fistp (line 294) */
    intPlayerY = lrintf(fltPlayerY);  /* Matches fistp (line 302) */

    /* Add strafe movement (perpendicular to facing direction) - mirrors lines 305-323 */
    uint8_t strafe_angle = angle_idx + 64; /* 90 degrees in 256-based system */
    fltPlayerX += COS_LOOK[strafe_angle] * fltPlayerStrafeSpeed;
    fltPlayerY += SIN_LOOK[strafe_angle] * fltPlayerStrafeSpeed;
    intPlayerX = lrintf(fltPlayerX);  /* Matches fistp (line 315) */
    intPlayerY = lrintf(fltPlayerY);  /* Matches fistp (line 323) */

    /* Apply friction */
    if (fltPlayerSpeed > 0) {
        fltPlayerSpeed -= fltPlayerFriction;
        if (fltPlayerSpeed < 0) fltPlayerSpeed = 0;
    } else if (fltPlayerSpeed < 0) {
        fltPlayerSpeed += fltPlayerFriction;
        if (fltPlayerSpeed > 0) fltPlayerSpeed = 0;
    }

    if (fltPlayerStrafeSpeed > 0) {
        fltPlayerStrafeSpeed -= fltPlayerStrafeFriction;
        if (fltPlayerStrafeSpeed < 0) fltPlayerStrafeSpeed = 0;
    } else if (fltPlayerStrafeSpeed < 0) {
        fltPlayerStrafeSpeed += fltPlayerStrafeFriction;
        if (fltPlayerStrafeSpeed > 0) fltPlayerStrafeSpeed = 0;
    }

    /* Map wrapping - mirrors assembly lines 385-432 */
    if (intPlayerX >= MAP_WIDTH) {
        fltPlayerX -= MAP_WIDTH;
        intPlayerX = lrintf(fltPlayerX);  /* Matches fistp (line 393) */
    }
    if (intPlayerX <= 0) {
        fltPlayerX += MAP_WIDTH;
        intPlayerX = lrintf(fltPlayerX);  /* Matches fistp (line 405) */
    }
    if (intPlayerY >= MAP_HEIGHT) {
        fltPlayerY -= MAP_HEIGHT;
        intPlayerY = lrintf(fltPlayerY);  /* Matches fistp (line 418) */
    }
    if (intPlayerY <= 0) {
        fltPlayerY += MAP_HEIGHT;
        intPlayerY = lrintf(fltPlayerY);  /* Matches fistp (line 430) */
    }
}

/*
 * Render player ship
 */
void RenderPlayer(void) {
    if (game_turnout == PLAYER_DEAD) {
        return;
    }

    /* Select sprite based on turn direction */
    uint32_t* sprite = PlayerShipOff;
    if (intbPlayerTurnDir < 0) {
        sprite = PlayerShipOffL;
    } else if (intbPlayerTurnDir > 0) {
        sprite = PlayerShipOffR;
    }

    /* Calculate which rotation frame to use */
    uint8_t angle_idx = (uint8_t)intbPlayerAngle;
    int frame = angle_idx; /* Each frame is 128x128 */

    /* Get sprite for this angle */
    uint32_t* frame_data = sprite + (frame * PLAYER_WIDTH * PLAYER_HEIGHT);

    /* Calculate screen position (center player on screen) */
    int screen_x = (SCREEN_WIDTH / 2) - (PLAYER_WIDTH / 2);
    int screen_y = (SCREEN_HEIGHT / 2) - (PLAYER_HEIGHT / 2);

    /* Blit player sprite */
    AlphaBlit(screen_x, screen_y, frame_data, PLAYER_WIDTH, PLAYER_HEIGHT);
}

/*
 * Fire player weapons
 */
void FirePlayerWeapons(void) {
    if (game_turnout == PLAYER_DEAD) return;

    uint8_t angle_idx = (uint8_t)intbPlayerAngle;
    float weapon_speed = fltPlayerSpeed + speed_array[1];

    /* Base weapon (level 0) */
    AddParticle(1, SMALL_PARTICLE, 10, fltPlayerNoseX, fltPlayerNoseY, angle_idx, weapon_speed, 30, 12);

    if (intPlayerWeaponsLevel < 1) return;

    /* Level 1: spread shot */
    weapon_speed = fltPlayerSpeed + speed_array[3];
    int spread = (Rand() % 10) - 5;
    AddParticle(1, SMALL_PARTICLE, 7, fltPlayerNoseX, fltPlayerNoseY, angle_idx + spread, weapon_speed, 30, 4);

    if (intPlayerWeaponsLevel < 2) return;

    /* Level 2: side shots with timer */
    if (weapon_timer_array[0] >= 4) weapon_timer_array[0] = 0;
    weapon_timer_array[0]++;

    if (weapon_timer_array[0] <= 1) {
        weapon_speed = fltPlayerSpeed + speed_array[0];
        AddParticle(1, LARGE_PARTICLE, 6, fltPlayerNoseX, fltPlayerNoseY, angle_idx + 20, weapon_speed, 20, 1);
        AddParticle(1, LARGE_PARTICLE, 6, fltPlayerNoseX, fltPlayerNoseY, angle_idx - 20, weapon_speed, 20, 1);
    }

    if (intPlayerWeaponsLevel < 3) return;

    /* Level 3: sweeping shots */
    weapon_speed = fltPlayerSpeed + speed_array[0];

    if (wma_direction == 0) weapon_mod_angle++;
    if (wma_direction == 1) weapon_mod_angle--;

    if (weapon_mod_angle >= 16) wma_direction = 1;
    if (weapon_mod_angle <= -16) wma_direction = 0;

    AddParticle(1, SMALL_PARTICLE, 12, fltPlayerNoseX, fltPlayerNoseY, angle_idx + weapon_mod_angle, weapon_speed, 30, 4);
    AddParticle(1, SMALL_PARTICLE, 12, fltPlayerNoseX, fltPlayerNoseY, angle_idx - weapon_mod_angle, weapon_speed, 30, 4);

    if (intPlayerWeaponsLevel < 4) return;

    /* Level 4: perpendicular shots with timer */
    if (weapon_timer_array[4] >= 10) weapon_timer_array[4] = 0;
    weapon_timer_array[4]++;

    if (weapon_timer_array[4] <= 8) {
        AddParticle(1, SMALL_PARTICLE, 15, fltPlayerNoseX, fltPlayerNoseY, angle_idx + 64, speed_array[0], 22, 3);
        AddParticle(1, SMALL_PARTICLE, 15, fltPlayerNoseX, fltPlayerNoseY, angle_idx - 64, speed_array[0], 22, 3);
    }

    if (intPlayerWeaponsLevel < 5) return;

    /* Level 5: shockwave every 40 frames */
    if (weapon_timer_array[8] >= 40) weapon_timer_array[8] = 0;
    weapon_timer_array[8]++;

    if (weapon_timer_array[8] <= 1) {
        for (int angle = 0; angle < 256; angle += 8) {
            AddParticle(1, SMALL_PARTICLE, 6, fltPlayerX, fltPlayerY, angle, speed_array[4], 10, 2);
        }
    }
}

/*
 * Fire main thrusters (creates particles behind ship)
 */
void FirePlayerMainThrusters(void) {
    if (game_turnout == PLAYER_DEAD) return;

    float thruster_vel = fltPlayerSpeed - thruster_speed;
    uint8_t angle_idx = (uint8_t)intbPlayerAngle;

    int spread1 = (Rand() % 10) - 5;
    AddParticle(0, LARGE_PARTICLE, 6, fltPlayerTailX, fltPlayerTailY, angle_idx + spread1, thruster_vel, 16, 0);

    int spread2 = (Rand() % 10) - 5;
    AddParticle(0, LARGE_PARTICLE, 7, fltPlayerTailX, fltPlayerTailY, angle_idx + spread2, thruster_vel, 21, 0);
}

/*
 * Fire left thrusters
 */
void FirePlayerLeftThrusters(void) {
    if (game_turnout == PLAYER_DEAD) return;

    uint8_t angle_idx = (uint8_t)intbPlayerAngle + 64;
    AddParticle(0, SMALL_PARTICLE, 11, fltPlayerX, fltPlayerY, angle_idx, thruster_strafe_speed, 2, 0);
}

/*
 * Fire right thrusters
 */
void FirePlayerRightThrusters(void) {
    if (game_turnout == PLAYER_DEAD) return;

    uint8_t angle_idx = (uint8_t)intbPlayerAngle - 64;
    AddParticle(0, SMALL_PARTICLE, 11, fltPlayerX, fltPlayerY, angle_idx, thruster_strafe_speed, 2, 0);
}

/*
 * Draw player health bar (matches assembly - top blue bar)
 */
void DrawPlayerHealth(void) {
    if (game_turnout == PLAYER_DEAD) return;
    if (intPlayerHealth <= 0 || intPlayerHealth > MAXPLAYERHEALTH) return;

    /* Calculate bar width based on health */
    int bar_width = (intPlayerHealth * (SCREEN_WIDTH - 30)) / MAXPLAYERHEALTH;

    /* Draw blue health bar at top of screen */
    for (int row = 0; row < PLAYER_HEALTH_BAR_HEIGHT; row++) {
        int offset = (5 + row) * SCREEN_WIDTH + 15;  /* Start 5 lines down, 15 pixels in */
        for (int col = 0; col < bar_width; col++) {
            uint32_t color = 0x800000FF;  /* Semi-transparent blue */
            ScreenOff[offset + col] = ComputeAlpha(color, ScreenOff[offset + col]);
        }
    }
}

/*
 * Detect collisions with player (matches assembly exactly)
 */
void DetectPlayerCollisions(void) {
    if (!ParticleDataOff) return;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        PARTICLE* p = &ParticleDataOff[i];

        if (!p->IsActive) continue;
        if (p->DetectCollisions == 0) continue;
        if (p->DetectCollisions == 1) continue;  /* Player's own particles */

        int particle_hit = 0;

        /* Check collisions for small particles */
        if (p->ImgSizeType == SMALL_PARTICLE) {
            int px1 = p->intX - SMALL_PARTICLE_WIDTH / 2;
            int py1 = p->intY - SMALL_PARTICLE_HEIGHT / 2;
            int px2 = px1 + SMALL_PARTICLE_WIDTH;
            int py2 = py1 + SMALL_PARTICLE_HEIGHT;

            /* Check player middle */
            if (intPlayerX >= px1 && intPlayerX <= px2 &&
                intPlayerY >= py1 && intPlayerY <= py2) {
                particle_hit = 1;
            }

            /* Check player nose */
            if (!particle_hit && intPlayerNoseX >= px1 && intPlayerNoseX <= px2 &&
                intPlayerNoseY >= py1 && intPlayerNoseY <= py2) {
                particle_hit = 1;
            }

            /* Check player tail */
            if (!particle_hit && intPlayerTailX >= px1 && intPlayerTailX <= px2 &&
                intPlayerTailY >= py1 && intPlayerTailY <= py2) {
                particle_hit = 1;
            }
        }
        /* Check collisions for large particles */
        else if (p->ImgSizeType == LARGE_PARTICLE) {
            int px1 = p->intX - LARGE_PARTICLE_WIDTH / 2;
            int py1 = p->intY - LARGE_PARTICLE_HEIGHT / 2;
            int px2 = px1 + LARGE_PARTICLE_WIDTH;
            int py2 = py1 + LARGE_PARTICLE_HEIGHT;

            /* Check player middle */
            if (intPlayerX >= px1 && intPlayerX <= px2 &&
                intPlayerY >= py1 && intPlayerY <= py2) {
                particle_hit = 1;
            }

            /* Check player nose */
            if (!particle_hit && intPlayerNoseX >= px1 && intPlayerNoseX <= px2 &&
                intPlayerNoseY >= py1 && intPlayerNoseY <= py2) {
                particle_hit = 1;
            }

            /* Check player tail */
            if (!particle_hit && intPlayerTailX >= px1 && intPlayerTailX <= px2 &&
                intPlayerTailY >= py1 && intPlayerTailY <= py2) {
                particle_hit = 1;
            }
        }

        if (particle_hit) {
            p->IsActive = 0;
            intPlayerHealth -= p->Damage;

            /* Create explosion effects */
            ShipExplode(p->fltX, p->fltY, 10, 1);
            ShipExplode(p->fltX, p->fltY, 10, 1);

            /* Play hit sound - mirrors assembly line 1046 */
            if (snd_effect_hit) {
                Mix_PlayChannelTimed(-1, snd_effect_hit, 0, -1);
            }
        }
    }
}
