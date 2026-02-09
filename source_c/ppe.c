/*
 * Particle Engine - Legacy Support
 *
 * This module provides minimal legacy support for the particle system.
 * The new architecture uses:
 *   - game/game.c: particles_update(), particle_spawn()
 *   - game/render.c: render_particles_hw()
 *   - game/sprites.c: texture loading
 *
 * STILL NEEDED:
 *   - InitParticleEngine() - allocates legacy array for bridge compatibility
 *   - DestroyParticleEngine() - frees the array
 *   - ResetParticleEngine() - clears all particles
 *   - MakeAlphaFromRGB() - alpha conversion for nuke icon
 */

#include "ppe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Legacy particle system state - kept for bridge compatibility */
PARTICLE* ParticleDataOff = NULL;
int NumParticles = 0;
uint32_t intPixel = 0;

/*
 * Initialize particle engine
 * Allocates the legacy particle array for bridge compatibility.
 */
void InitParticleEngine(void) {
    NumParticles = 0;

    /* Allocate legacy particle data array for bridge compatibility */
    ParticleDataOff = (PARTICLE*)malloc(sizeof(PARTICLE) * MAX_PARTICLES);
    if (!ParticleDataOff) {
        printf("\nMemory allocation error in InitParticleEngine\n");
        return;
    }

    ResetParticleEngine();
}

/*
 * Destroy particle engine
 */
void DestroyParticleEngine(void) {
    if (ParticleDataOff) free(ParticleDataOff);
    ParticleDataOff = NULL;
}

/*
 * Reset all particles
 */
void ResetParticleEngine(void) {
    if (!ParticleDataOff) return;
    memset(ParticleDataOff, 0, sizeof(PARTICLE) * MAX_PARTICLES);
}

/*
 * Convert RGB pixel to ARGB with alpha based on brightness
 * Used for nuke icon alpha conversion.
 * Takes pixel in 0x00RRGGBB format, stores 0xAARRGGBB in global intPixel
 */
void MakeAlphaFromRGB(uint32_t pixel) {
    uint8_t red = (pixel >> 16) & 0xFF;
    uint8_t green = (pixel >> 8) & 0xFF;
    uint8_t blue = pixel & 0xFF;

    uint32_t avg = (red + green + blue) / 3;
    uint32_t result = pixel & 0x00FFFFFF;

    if (avg >= 4) {
        result |= (avg << 24);
    }

    intPixel = result;
}

/*
 * Legacy particle functions - STUBS
 * These are no longer used but still referenced by legacy code being phased out.
 * The new architecture uses particle_spawn() in game/game.c
 */
void AddParticle(int detect_collisions, int img_size_type, int img_index,
                 float x, float y, int angle, float speed, int max_life, int damage) {
    (void)detect_collisions; (void)img_size_type; (void)img_index;
    (void)x; (void)y; (void)angle; (void)speed; (void)max_life; (void)damage;
    /* No-op stub */
}

void AddParticleByVector(int detect_collisions, int img_size_type, int img_index,
                         float x, float y, float x_vel, float y_vel, int max_life, int damage) {
    (void)detect_collisions; (void)img_size_type; (void)img_index;
    (void)x; (void)y; (void)x_vel; (void)y_vel; (void)max_life; (void)damage;
    /* No-op stub */
}

void RenderParticlesHW(int layer) {
    (void)layer;
    /* No-op stub - use render_particles_hw() from game/render.c */
}

void RenderParticlesHW_Camera(int layer, int cam_x, int cam_y) {
    (void)layer; (void)cam_x; (void)cam_y;
    /* No-op stub - use render_particles_hw() from game/render.c */
}
