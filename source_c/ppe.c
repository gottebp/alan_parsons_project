/*
 * Particle Engine - handles particle effects
 * Converted from x86 assembly
 */

#include "ppe.h"
#include "sdl_wrapper.h"
#include "rand.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Particle system state */
PARTICLE* ParticleDataOff = NULL;
uint32_t* SmallParticleFlaresOff = NULL;
uint32_t* LargeParticleFlaresOff = NULL;
int NumParticles = 0;
static int LastParticle = 0;
uint32_t intPixel = 0;

extern float SIN_LOOK[256];
extern float COS_LOOK[256];

/*
 * Initialize particle engine
 */
void InitParticleEngine(void) {
    extern char* SmallParticleFile;
    extern char* LargeParticleFile;

    NumParticles = 0;
    LastParticle = 0;

    /* Allocate particle flare images - mirrors lines 115-127 */
    SmallParticleFlaresOff = (uint32_t*)malloc(SMALL_FLARE_FILE_WIDTH * SMALL_FLARE_FILE_HEIGHT * 4);
    if (!SmallParticleFlaresOff) {
        printf("\nMemory allocation error occured in _InitParticleEngine\n");
        return;
    }

    LargeParticleFlaresOff = (uint32_t*)malloc(LARGE_FLARE_FILE_WIDTH * LARGE_FLARE_FILE_HEIGHT * 4);
    if (!LargeParticleFlaresOff) {
        printf("\nMemory allocation error occured in _InitParticleEngine\n");
        return;
    }

    /* Allocate particle data array - mirrors lines 129-134 */
    ParticleDataOff = (PARTICLE*)malloc(sizeof(PARTICLE) * MAX_PARTICLES);
    if (!ParticleDataOff) {
        printf("\nMemory allocation error occured in _InitParticleEngine\n");
        return;
    }

    /* Load flare images */
    LoadBMP(SmallParticleFlaresOff, SmallParticleFile);
    LoadBMP(LargeParticleFlaresOff, LargeParticleFile);

    /* Setup alpha transparency for flares - mirrors assembly lines 138-164 */
    for (int i = 0; i < SMALL_FLARE_FILE_WIDTH * SMALL_FLARE_FILE_HEIGHT; i++) {
        MakeAlphaFromRGB(SmallParticleFlaresOff[i]);  /* Result in global intPixel */
        SmallParticleFlaresOff[i] = intPixel;
    }

    for (int i = 0; i < LARGE_FLARE_FILE_WIDTH * LARGE_FLARE_FILE_HEIGHT; i++) {
        MakeAlphaFromRGB(LargeParticleFlaresOff[i]);  /* Result in global intPixel */
        LargeParticleFlaresOff[i] = intPixel;
    }

    ResetParticleEngine();
}

/*
 * Destroy particle engine
 */
void DestroyParticleEngine(void) {
    if (SmallParticleFlaresOff) free(SmallParticleFlaresOff);
    if (LargeParticleFlaresOff) free(LargeParticleFlaresOff);
    if (ParticleDataOff) free(ParticleDataOff);

    SmallParticleFlaresOff = NULL;
    LargeParticleFlaresOff = NULL;
    ParticleDataOff = NULL;
}

/*
 * Reset all particles
 */
void ResetParticleEngine(void) {
    if (!ParticleDataOff) return;

    /* Clear all bytes in particle array - mirrors assembly lines 206-212 */
    unsigned char* ptr = (unsigned char*)ParticleDataOff;
    for (int i = 0; i < sizeof(PARTICLE) * MAX_PARTICLES; i++) {
        ptr[i] = 0;
    }
}

/*
 * Add a particle with angle and speed (matches assembly _AddParticle signature)
 * Mirrors assembly lines 223-417: Two separate search loops
 */
void AddParticle(int detect_collisions, int img_size_type, int img_index, float x, float y, int angle, float speed, int max_life, int damage) {
    if (!ParticleDataOff) return;

    PARTICLE* p;
    int idx;
    int angle_idx;
    float x_vel, y_vel;
    uint32_t img_offset;

    /* Calculate velocity components from angle - mirrors lines 295-307 */
    angle_idx = angle & 0xFF;
    x_vel = COS_LOOK[angle_idx] * speed;
    y_vel = SIN_LOOK[angle_idx] * speed;

    /* Calculate image offset - mirrors lines 258-282 */
    if (img_size_type == SMALL_PARTICLE) {
        img_offset = img_index * SMALL_PARTICLE_WIDTH * SMALL_PARTICLE_HEIGHT * 4;
    } else if (img_size_type == LARGE_PARTICLE) {
        img_offset = img_index * LARGE_PARTICLE_WIDTH * LARGE_PARTICLE_HEIGHT * 4;
    } else {
        img_offset = 0;
    }

    /* First loop: Search from LastParticle to MAX_PARTICLES (mirrors lines 241-317) */
    for (idx = LastParticle; idx < MAX_PARTICLES; idx++) {
        p = &ParticleDataOff[idx];

        if (p->IsActive == 0) {
            /* Found free slot - initialize particle */
            p->IsActive = 1;
            p->DetectCollisions = detect_collisions;
            p->ImgSizeType = img_size_type;
            p->ImgOffset = img_offset;
            p->fltX = x;
            p->fltY = y;
            p->intX = lrintf(x);  /* Use lrintf to match fistp instruction (lines 287-288) */
            p->intY = lrintf(y);  /* Use lrintf to match fistp instruction (lines 292-293) */
            p->XV = x_vel;
            p->YV = y_vel;
            p->MaxLife = max_life;
            p->Age = 0;
            p->Damage = damage;

            NumParticles++;
            LastParticle = idx;
            return;
        }
    }

    /* Second loop: Search from 0 to LastParticle (mirrors lines 327-412) */
    for (idx = 0; idx < LastParticle; idx++) {
        p = &ParticleDataOff[idx];

        if (p->IsActive == 0) {
            /* Found free slot - initialize particle */
            p->IsActive = 1;
            p->DetectCollisions = detect_collisions;
            p->ImgSizeType = img_size_type;
            p->ImgOffset = img_offset;
            p->fltX = x;
            p->fltY = y;
            p->intX = lrintf(x);  /* Use lrintf to match fistp instruction (lines 374-375) */
            p->intY = lrintf(y);  /* Use lrintf to match fistp instruction (lines 378-380) */
            p->XV = x_vel;
            p->YV = y_vel;
            p->MaxLife = max_life;
            p->Age = 0;
            p->Damage = damage;

            NumParticles++;
            LastParticle = idx;
            return;
        }
    }

    /* No free slots found */
}

/*
 * Add a particle with velocity vector - mirrors assembly _AddParticleByVector
 * Assembly lines 426-606: Two separate search loops
 */
void AddParticleByVector(int detect_collisions, int img_size_type, int img_index, float x, float y, float x_vel, float y_vel, int max_life, int damage) {
    if (!ParticleDataOff) return;

    PARTICLE* p;
    int idx;
    uint32_t img_offset;

    /* Calculate image offset - mirrors lines 461-485 */
    if (img_size_type == SMALL_PARTICLE) {
        img_offset = img_index * SMALL_PARTICLE_WIDTH * SMALL_PARTICLE_HEIGHT * 4;
    } else if (img_size_type == LARGE_PARTICLE) {
        img_offset = img_index * LARGE_PARTICLE_WIDTH * LARGE_PARTICLE_HEIGHT * 4;
    } else {
        img_offset = 0;
    }

    /* First loop: Search from LastParticle to MAX_PARTICLES (mirrors lines 444-513) */
    for (idx = LastParticle; idx < MAX_PARTICLES; idx++) {
        p = &ParticleDataOff[idx];

        if (p->IsActive == 0) {
            /* Found free slot - initialize particle */
            p->IsActive = 1;
            p->DetectCollisions = detect_collisions;
            p->ImgSizeType = img_size_type;
            p->ImgOffset = img_offset;
            p->fltX = x;
            p->fltY = y;
            p->intX = lrintf(x);  /* Use lrintf to match fistp instruction (lines 490-491) */
            p->intY = lrintf(y);  /* Use lrintf to match fistp instruction (lines 494-496) */
            p->XV = x_vel;
            p->YV = y_vel;
            p->MaxLife = max_life;
            p->Age = 0;
            p->Damage = damage;

            NumParticles++;
            LastParticle = idx;
            return;
        }
    }

    /* Second loop: Search from 0 to LastParticle (mirrors lines 523-601) */
    for (idx = 0; idx < LastParticle; idx++) {
        p = &ParticleDataOff[idx];

        if (p->IsActive == 0) {
            /* Found free slot - initialize particle */
            p->IsActive = 1;
            p->DetectCollisions = detect_collisions;
            p->ImgSizeType = img_size_type;
            p->ImgOffset = img_offset;
            p->fltX = x;
            p->fltY = y;
            p->intX = lrintf(x);  /* Use lrintf to match fistp instruction (lines 570-571) */
            p->intY = lrintf(y);  /* Use lrintf to match fistp instruction (lines 574-576) */
            p->XV = x_vel;
            p->YV = y_vel;
            p->MaxLife = max_life;
            p->Age = 0;
            p->Damage = damage;

            NumParticles++;
            LastParticle = idx;
            return;
        }
    }

    /* No free slots found */
}

/*
 * Update all active particles
 * Range parameter: only updates particles with DetectCollisions >= Range
 * Mirrors assembly lines 615-718
 */
void UpdateParticles(int range) {
    if (!ParticleDataOff) return;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        PARTICLE* p = &ParticleDataOff[i];

        /* Skip if collision detection level is below range - mirrors line 630 */
        if (p->DetectCollisions < range) continue;
        if (!p->IsActive) continue;

        /* Check if particle has expired - mirrors lines 636-641 */
        if (p->Age >= p->MaxLife) {
            p->IsActive = 0;
            continue;
        }

        /* Update position - mirrors lines 644-654 */
        /* Use lrintf for FPU-style rounding (fist instruction) instead of truncation */
        p->fltX += p->XV;
        p->intX = lrintf(p->fltX);

        p->fltY += p->YV;
        p->intY = lrintf(p->fltY);

        /* Wrap X coordinate at map boundaries - mirrors lines 657-679 */
        if (p->intX >= MAP_WIDTH) {
            p->fltX -= MAP_WIDTH;
            p->intX = lrintf(p->fltX);
        }
        if (p->intX <= 0) {
            p->fltX += MAP_WIDTH;
            p->intX = lrintf(p->fltX);
        }

        /* Wrap Y coordinate at map boundaries - mirrors lines 682-704 */
        if (p->intY >= MAP_HEIGHT) {
            p->fltY -= MAP_HEIGHT;
            p->intY = lrintf(p->fltY);
        }
        if (p->intY <= 0) {
            p->fltY += MAP_HEIGHT;
            p->intY = lrintf(p->fltY);
        }

        /* Increment age - mirrors line 707 */
        p->Age++;
    }
}

/*
 * Render all active particles
 * layer parameter: only renders particles with DetectCollisions >= layer
 */
void RenderParticles(int layer) {
    if (!ParticleDataOff) return;

    extern int intPlayerX, intPlayerY;

    for (int i = 0; i < MAX_PARTICLES; i++) {
        PARTICLE* p = &ParticleDataOff[i];

        /* Skip if collision detection level is below layer */
        if (p->DetectCollisions < layer) continue;
        if (!p->IsActive) continue;

        /* Calculate screen position relative to player */
        int screen_x = p->intX - intPlayerX + (SCREEN_WIDTH / 2);
        int screen_y = p->intY - intPlayerY + (SCREEN_HEIGHT / 2);

        /* Get particle sprite and dimensions */
        uint32_t* sprite;
        int width, height;

        if (p->ImgSizeType == SMALL_PARTICLE) {
            sprite = SmallParticleFlaresOff + (p->ImgOffset / 4);  /* ImgOffset is in bytes, divide for uint32_t pointer */
            width = SMALL_PARTICLE_WIDTH;
            height = SMALL_PARTICLE_HEIGHT;

            /* Center the particle */
            screen_x -= SMALL_PARTICLE_WIDTH / 2;
            screen_y -= SMALL_PARTICLE_HEIGHT / 2;

            /* Handle map wrapping for screen coordinates */
            if (screen_x < (-MAP_WIDTH + SCREEN_WIDTH)) {
                screen_x += MAP_WIDTH;
            }
            if (screen_x > MAP_WIDTH) {
                screen_x -= MAP_WIDTH;
            }
            if (screen_y < (-MAP_HEIGHT + SCREEN_HEIGHT)) {
                screen_y += MAP_HEIGHT;
            }
            if (screen_y > MAP_HEIGHT) {
                screen_y -= MAP_HEIGHT;
            }

            AlphaBlit(screen_x, screen_y, sprite, width, height);
        } else if (p->ImgSizeType == LARGE_PARTICLE) {
            sprite = LargeParticleFlaresOff + (p->ImgOffset / 4);  /* ImgOffset is in bytes, divide for uint32_t pointer */
            width = LARGE_PARTICLE_WIDTH;
            height = LARGE_PARTICLE_HEIGHT;

            /* Center the particle */
            screen_x -= LARGE_PARTICLE_WIDTH / 2;
            screen_y -= LARGE_PARTICLE_HEIGHT / 2;

            /* Handle map wrapping for screen coordinates */
            if (screen_x < (-MAP_WIDTH + SCREEN_WIDTH)) {
                screen_x += MAP_WIDTH;
            }
            if (screen_x > MAP_WIDTH) {
                screen_x -= MAP_WIDTH;
            }
            if (screen_y < (-MAP_HEIGHT + SCREEN_HEIGHT)) {
                screen_y += MAP_HEIGHT;
            }
            if (screen_y > MAP_HEIGHT) {
                screen_y -= MAP_HEIGHT;
            }

            AlphaBlit(screen_x, screen_y, sprite, width, height);
        }
    }
}

/*
 * Convert RGB pixel to ARGB with alpha based on brightness - mirrors assembly _MakeAlphaFromRGB (lines 841-888)
 * Assembly uses pusha/popa so result is returned via global intPixel, NOT via eax
 * Takes pixel in 0x00RRGGBB format, stores 0xAARRGGBB in global intPixel
 * Alpha value is the average of R,G,B (if >= 4)
 */
void MakeAlphaFromRGB(uint32_t pixel) {
    /* Extract color components - mirrors assembly lines 847-859 */
    uint8_t red = (pixel >> 16) & 0xFF;
    uint8_t green = (pixel >> 8) & 0xFF;
    uint8_t blue = pixel & 0xFF;

    /* Calculate average brightness - mirrors assembly lines 863-872 */
    uint32_t avg = (red + green + blue) / 3;

    /* Construct result - RGB portion only - mirrors assembly lines 874-875 */
    uint32_t result = pixel & 0x00FFFFFF;

    /* Set alpha if brightness is sufficient - mirrors assembly lines 877-881 */
    if (avg >= 4) {
        result |= (avg << 24);
    }

    /* Store in global variable - mirrors assembly line 885 */
    intPixel = result;
    /* Assembly uses popa which restores eax, so no value returned in eax */
}
