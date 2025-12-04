#ifndef _SDL_WRAPPER_H_
#define _SDL_WRAPPER_H_

#include <SDL.h>
#include <SDL_mixer.h>
#include <stdint.h>
#include "defs.h"

/* SDL Graphics System */

extern SDL_Window* screen_window;
extern SDL_Renderer* screen_renderer;
extern SDL_Texture* screen_texture;
extern uint32_t* ScreenOff;
extern uint32_t ScreenTemp[SCREEN_WIDTH * SCREEN_HEIGHT];

/* Graphics Functions */
int InitGraphics(int fullscreen);
void DestroyGraphics(void);
int LoadBMP(uint32_t* buffer, const char* filename);
void AlphaBlit(int x, int y, uint32_t* src, int src_width, int src_height);
void UpdateScreen(void);

/* Alpha blending */
uint32_t ComputeAlpha(uint32_t src, uint32_t dst);

/* Screen effects */
void FadeFromBlack(int frames, int speed);
void FadeToBlack(int frames, int speed);
void FadeFromWhite(int frames, int speed);
void FadeToWhite(int frames, int speed);

#endif /* _SDL_WRAPPER_H_ */
