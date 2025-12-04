#ifndef _PLAYER_H_
#define _PLAYER_H_

#include <stdint.h>
#include <stdlib.h>

/* Player state variables */
extern int intPlayerX;
extern int intPlayerY;
extern float fltPlayerX;
extern float fltPlayerY;
extern float fltPlayerSpeed;
extern float fltPlayerStrafeSpeed;
extern int8_t intbPlayerAngle;
extern int8_t intbPlayerTurnDir;
extern int intPlayerHealth;
extern int intPlayerWeaponsLevel;

/* Player ship sprites */
extern uint32_t* PlayerShipOff;
extern uint32_t* PlayerShipOffL;
extern uint32_t* PlayerShipOffR;

/* Player physics constants */
extern float fltPlayerAccel;
extern float fltPlayerFriction;
extern float fltPlayerMaxSpeed;
extern float fltPlayerMinSpeed;
extern float fltPlayerMass;
extern float fltPlayerStrafeFriction;
extern float fltPlayerMaxStrafeSpeed;
extern float fltPlayerMinStrafeSpeed;

/* Player collision points */
extern float fltPlayerNoseX, fltPlayerNoseY;
extern float fltPlayerTailX, fltPlayerTailY;
extern int intPlayerNoseX, intPlayerNoseY;
extern int intPlayerTailX, intPlayerTailY;

/* Functions */
void InitPlayer(void);
void DestroyPlayer(void);
void UpdatePlayer(void);
void RenderPlayer(void);
void FirePlayerWeapons(void);
void FirePlayerMainThrusters(void);
void FirePlayerLeftThrusters(void);
void FirePlayerRightThrusters(void);
void DrawPlayerHealth(void);
void DetectPlayerCollisions(void);
void LoadRaw(const char* filename, void* buffer, size_t size);  /* Assembly returns void (no eax set) */

#endif /* _PLAYER_H_ */
