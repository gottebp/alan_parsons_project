#ifndef _PPE_H_
#define _PPE_H_

#include <stdint.h>
#include "defs.h"

/* Particle structure - 46 bytes */
typedef struct {
    uint8_t IsActive;
    uint8_t DetectCollisions;
    uint32_t ImgSizeType;    /* SMALL_PARTICLE or LARGE_PARTICLE */
    uint32_t ImgOffset;
    float fltX;
    float fltY;
    int intX;
    int intY;
    float XV;
    float YV;
    uint32_t MaxLife;
    uint32_t Age;
    uint32_t Damage;
} PARTICLE;

/* Particle system */
extern PARTICLE* ParticleDataOff;
extern uint32_t* SmallParticleFlaresOff;
extern uint32_t* LargeParticleFlaresOff;
extern int NumParticles;
extern uint32_t intPixel;

void InitParticleEngine(void);
void DestroyParticleEngine(void);
void ResetParticleEngine(void);
void AddParticle(int detect_collisions, int img_size_type, int img_index, float x, float y, int angle, float speed, int max_life, int damage);
void AddParticleByVector(int detect_collisions, int img_size_type, int img_index, float x, float y, float x_vel, float y_vel, int max_life, int damage);
void UpdateParticles(int collision_detect);
void RenderParticles(int layer);
void MakeAlphaFromRGB(uint32_t pixel);  /* Assembly returns result via global intPixel, not eax (uses pusha/popa) */

#endif /* _PPE_H_ */
