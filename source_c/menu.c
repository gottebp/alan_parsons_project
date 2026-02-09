/*
 * Menu system - level selection and game menu
 * Converted from x86 assembly
 */

#include "menu.h"
#include "sdl_wrapper.h"
#include "input.h"
#include "sse_mem.h"
#include "defs.h"
#include "rand.h"
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_mixer.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* Menu state */
int MenuResult = 0;
uint8_t IsMenuRunning = 0;

/* Menu assets - exported for menu_new.c */
uint32_t* menu_background = NULL;
uint32_t* mouse_cursor = NULL;

/* Legacy masks - kept for legacy RunMenu */
static uint32_t darkness_mask[143 * 122];
static uint32_t random_mask[143 * 122];

extern uint32_t* ScreenOff;
extern int game_level;
extern int snd_weapon_counter;
extern int snd_engines_counter;

/*
 * Initialize menu system
 */
void InitMenu(void) {
    MenuResult = 0;

    /* Allocate menu background */
    menu_background = (uint32_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * 4);
    if (!menu_background) {
        fprintf(stderr, "Memory allocation error in init_menu (background)\n");
        return;
    }

    /* Allocate mouse cursor */
    mouse_cursor = (uint32_t*)malloc(CURSOR_WIDTH * CURSOR_HEIGHT * 4);
    if (!mouse_cursor) {
        fprintf(stderr, "Memory allocation error in init_menu (cursor)\n");
        return;
    }

    /* Load menu background */
    if (LoadBMP(menu_background, "./data/main_menu.bmp") < 0) {
        fprintf(stderr, "Failed to load main_menu.bmp\n");
    }

    /* Load mouse cursor */
    if (LoadBMP(mouse_cursor, "./data/cursor.bmp") < 0) {
        fprintf(stderr, "Failed to load cursor.bmp\n");
    }

    /* Setup cursor alpha transparency */
    for (int i = 0; i < CURSOR_WIDTH * CURSOR_HEIGHT; i++) {
        if (mouse_cursor[i] == 0xFF00FFFF) {
            mouse_cursor[i] = 0x00000000;
        }
    }

    /* Initialize darkness mask */
    for (int i = 0; i < 143 * 122; i++) {
        darkness_mask[i] = 0xB0000000;
        random_mask[i] = 0xB0000000;
    }
}

/*
 * Destroy menu system
 */
void DestroyMenu(void) {
    if (menu_background) {
        free(menu_background);
        menu_background = NULL;
    }

    if (mouse_cursor) {
        free(mouse_cursor);
        mouse_cursor = NULL;
    }
}

/*
 * Check if mouse is in button bounds
 */
static int mouse_in_bounds(int x1, int y1, int x2, int y2) {
    return (MOUSE_X >= x1 && MOUSE_X <= x2 && MOUSE_Y >= y1 && MOUSE_Y <= y2);
}

/*
 * Generate random alpha mask for button highlights - mirrors assembly _MakeRandomMask
 */
static void make_random_mask(void) {
    /* Generate random alpha values 0-99 for each pixel - mirrors assembly lines 450-464 */
    for (int i = 0; i < 143 * 122; i++) {
        uint32_t rand_val = Rand();
        uint8_t alpha = (uint8_t)(rand_val % 100);
        random_mask[i] = (alpha << 24) | 0x00000000;  /* Alpha in high byte, RGB stays 0 */
    }
}

/*
 * Run menu and handle input
 */
int RunMenu(int game_running) {
    (void)game_running;  /* Currently unused - may be used for resume functionality */
    if (!IsMenuRunning) {
        IsMenuRunning = 1;
        Mix_FadeOutMusic(100);
    }

    /* Handle sound counters - mirrors assembly lines 132-142 */
    if (snd_weapon_counter > 0) {
        snd_weapon_counter--;
        Mix_HaltChannel(1);
    }
    if (snd_engines_counter > 0) {
        snd_engines_counter--;
        Mix_HaltChannel(3);
    }

    /* Initial screen setup with fade - mirrors assembly lines 144-147 */
    sseMemset32(ScreenOff, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
    AlphaBlit(0, 0, menu_background, 640, 480);
    sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
    FadeFromBlack(80, 3);

    SDL_ShowCursor(SDL_DISABLE);
    MenuResult = 0;

    /* Wait for escape key to be released - mirrors assembly lines 152-155 */
    while (KEYBOARD[SDL_SCANCODE_ESCAPE]) {
        UpdateInput();
#ifdef __EMSCRIPTEN__
        emscripten_sleep(16);
#endif
    }

    /* Menu loop */
    /* EMSCRIPTEN NOTE: This loop now yields with emscripten_sleep */
    while (1) {
        /* Generate random mask for button highlights - mirrors assembly line 159 */
        make_random_mask();

        /* Clear and redraw */
        sseMemset32(ScreenOff, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
        AlphaBlit(0, 0, menu_background, 640, 480);

        UpdateInput();

        /* Check for escape (always exit to start screen) */
        if (KEYBOARD[SDL_SCANCODE_ESCAPE]) {
            MenuResult = 0;  /* Always exit when escape pressed in menu */
            IsMenuRunning = 0;
            Mix_FadeOutMusic(200);
            break;
        }

        /* Check Exit button */
        if (mouse_in_bounds(13, 13, 94, 115)) {
            AlphaBlit(8, 2, random_mask, 143, 122);
            if (MOUSE_LBUTTON) {
                while (MOUSE_LBUTTON) {
                    UpdateInput();
#ifdef __EMSCRIPTEN__
                    emscripten_sleep(16);
#endif
                }
                MenuResult = 0;
                break;
            }
        }

        /* Check Shire button */
        if (mouse_in_bounds(163, 19, 306, 155)) {
            AlphaBlit(163, 13, random_mask, 143, 122);
            if (MOUSE_LBUTTON) {
                while (MOUSE_LBUTTON) {
                    UpdateInput();
#ifdef __EMSCRIPTEN__
                    emscripten_sleep(16);
#endif
                }
                MenuResult = MENU_LOAD_SHIRE;
                break;
            }
        }

        /* Check Archipelago button (requires level 1) - mirrors assembly lines 234-259 */
        /* Always draw darkness mask if level not reached */
        if (game_level < 1) {
            AlphaBlit(483, 30, darkness_mask, 143, 122);
        }
        /* Then check if mouse in bounds and level reached, draw random mask on top */
        if (mouse_in_bounds(483, 36, 626, 153) && game_level >= 1) {
            AlphaBlit(483, 30, random_mask, 143, 122);
            if (MOUSE_LBUTTON) {
                while (MOUSE_LBUTTON) {
                    UpdateInput();
#ifdef __EMSCRIPTEN__
                    emscripten_sleep(16);
#endif
                }
                MenuResult = MENU_LOAD_ARCHIPELAGO;
                break;
            }
        }

        /* Check Dune button (requires level 2) - mirrors assembly lines 262-287 */
        if (game_level < 2) {
            AlphaBlit(480, 186, darkness_mask, 143, 122);
        }
        if (mouse_in_bounds(480, 192, 623, 309) && game_level >= 2) {
            AlphaBlit(480, 186, random_mask, 143, 122);
            if (MOUSE_LBUTTON) {
                while (MOUSE_LBUTTON) {
                    UpdateInput();
#ifdef __EMSCRIPTEN__
                    emscripten_sleep(16);
#endif
                }
                MenuResult = MENU_LOAD_DUNE;
                break;
            }
        }

        /* Check Midkemia button (requires level 3) - mirrors assembly lines 290-315 */
        if (game_level < 3) {
            AlphaBlit(20, 310, darkness_mask, 143, 122);
        }
        if (mouse_in_bounds(20, 316, 163, 433) && game_level >= 3) {
            AlphaBlit(20, 310, random_mask, 143, 122);
            if (MOUSE_LBUTTON) {
                while (MOUSE_LBUTTON) {
                    UpdateInput();
#ifdef __EMSCRIPTEN__
                    emscripten_sleep(16);
#endif
                }
                MenuResult = MENU_LOAD_MIDKEMIA;
                break;
            }
        }

        /* Check Oceania button (requires level 4) - mirrors assembly lines 318-343 */
        if (game_level < 4) {
            AlphaBlit(454, 336, darkness_mask, 143, 122);
        }
        if (mouse_in_bounds(454, 342, 597, 459) && game_level >= 4) {
            AlphaBlit(454, 336, random_mask, 143, 122);
            if (MOUSE_LBUTTON) {
                while (MOUSE_LBUTTON) {
                    UpdateInput();
#ifdef __EMSCRIPTEN__
                    emscripten_sleep(16);
#endif
                }
                MenuResult = MENU_LOAD_OCEANIA;
                break;
            }
        }

        /* Check Mordor button (requires level 5) - mirrors assembly lines 346-371 */
        if (game_level < 5) {
            AlphaBlit(14, 151, darkness_mask, 143, 122);
        }
        if (mouse_in_bounds(14, 157, 157, 274) && game_level >= 5) {
            AlphaBlit(14, 151, random_mask, 143, 122);
            if (MOUSE_LBUTTON) {
                while (MOUSE_LBUTTON) {
                    UpdateInput();
#ifdef __EMSCRIPTEN__
                    emscripten_sleep(16);
#endif
                }
                MenuResult = MENU_LOAD_MORDOR;
                break;
            }
        }

        /* Draw cursor */
        AlphaBlit(MOUSE_X - CURSOR_WIDTH / 2, MOUSE_Y - CURSOR_HEIGHT / 2,
                  mouse_cursor, CURSOR_WIDTH, CURSOR_HEIGHT);

        UpdateScreen();

#ifdef __EMSCRIPTEN__
        /* EMSCRIPTEN: Yield to browser to prevent freezing and allow input processing */
        emscripten_sleep(16);  /* ~60 FPS */
#endif
    }

    /* Wait for ESC key release before exiting - mirrors assembly lines 412-414 */
    while (KEYBOARD[SDL_SCANCODE_ESCAPE]) {
        UpdateInput();
#ifdef __EMSCRIPTEN__
        emscripten_sleep(16);
#endif
    }

    /* Exit with fade to black - mirrors assembly lines 417-420 */
    sseMemset32(ScreenOff, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
    AlphaBlit(0, 0, menu_background, 640, 480);
    sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
    FadeToBlack(100, 3);

    IsMenuRunning = 0;
    return MenuResult;
}
