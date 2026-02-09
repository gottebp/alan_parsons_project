#ifndef _PPE_H_
#define _PPE_H_

#include <stdint.h>
#include "defs.h"

/*============================================================================
 * LEGACY PARTICLE ENGINE
 *
 * This header provides backwards compatibility for legacy code.
 * The new architecture uses:
 *   - game/game.c: particle_spawn(), particles_update()
 *   - game/render.c: render_particles_hw()
 *   - game/sprites.c: texture loading
 *============================================================================*/

/*============================================================================
 * PARTICLE STRUCTURE (used by bridge for sync)
 *============================================================================*/
typedef struct {
    uint8_t IsActive;
    uint8_t DetectCollisions;
    uint32_t ImgSizeType;
    uint32_t ImgOffset;
    float fltX, fltY;
    int intX, intY;
    float XV, YV;
    uint32_t MaxLife;
    uint32_t Age;
    uint32_t Damage;
} PARTICLE;

/*============================================================================
 * PARTICLE STATE (defined in ppe.c, used by bridge)
 *============================================================================*/
extern PARTICLE* ParticleDataOff;
extern int NumParticles;
extern uint32_t intPixel;

/*============================================================================
 * LIFECYCLE
 *============================================================================*/
void InitParticleEngine(void);
void DestroyParticleEngine(void);
void ResetParticleEngine(void);

/*============================================================================
 * UTILITY
 *============================================================================*/
void MakeAlphaFromRGB(uint32_t pixel);  /* Result stored in global intPixel */

/*============================================================================
 * LEGACY STUBS (no-ops, kept for compatibility)
 *============================================================================*/
void AddParticle(int detect_collisions, int img_size_type, int img_index,
                 float x, float y, int angle, float speed, int max_life, int damage);
void AddParticleByVector(int detect_collisions, int img_size_type, int img_index,
                         float x, float y, float x_vel, float y_vel, int max_life, int damage);
void RenderParticlesHW(int layer);
void RenderParticlesHW_Camera(int layer, int cam_x, int cam_y);

#endif /* _PPE_H_ */
