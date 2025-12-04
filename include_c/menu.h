#ifndef _MENU_H_
#define _MENU_H_

#include <stdint.h>

/* Menu system */
extern int MenuResult;
extern uint8_t IsMenuRunning;

void InitMenu(void);
void DestroyMenu(void);
int RunMenu(int game_running);

#endif /* _MENU_H_ */
