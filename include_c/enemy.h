#ifndef _ENEMY_H_
#define _ENEMY_H_

#include <stdint.h>
#include "defs.h"

/* Enemy array - 100 enemies max */
extern Enemy Enemies[100];
extern uint32_t* SmallEnemyImageData;
extern uint32_t* BossImageData;

/* Enemy spawn timing */
extern uint32_t SpawnFrames[18];
extern uint32_t FrameNum;
extern uint32_t NextWaveNumber;

/* Functions */
void LoadEnemyData(void);
void DestroyEnemyData(void);
void InitializeEnemies(int map_id);
void UpdateEnemies(void);
void RenderEnemies(void);

#endif /* _ENEMY_H_ */
