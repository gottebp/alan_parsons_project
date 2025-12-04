#ifndef _MAPENG_H_
#define _MAPENG_H_

#include <stdint.h>

/* Map engine */
extern uint32_t* MapOff;
extern uint32_t* MapPadTop;
extern uint32_t* MapPadBottom;
extern int intShakeMap;

void InitMapEngine(void);
void DestroyMapEngine(void);
int LoadMap(const char* filename);
void RenderMap(void);
void ShakeMap(int frames);

#endif /* _MAPENG_H_ */
