#ifndef _INPUT_H_
#define _INPUT_H_

#include <SDL.h>
#include <stdint.h>

/* Input System - types match assembly exactly */

extern uint8_t KEYBOARD[320];       /* Matches assembly's 320 bytes */
extern uint16_t MOUSE_X;            /* Matches assembly's word */
extern uint16_t MOUSE_Y;            /* Matches assembly's word */
extern uint16_t MOUSE_LBUTTON;      /* Matches assembly's word */
extern uint16_t MOUSE_RBUTTON;      /* Matches assembly's word */
extern uint16_t MOUSE_MBUTTON;      /* Matches assembly's word */
extern uint8_t MOUSE_BUTTON;        /* Raw mouse button state */
extern int QUIT_SIGNAL;

void UpdateInput(void);
void FlushKeyboard(void);

#endif /* _INPUT_H_ */
