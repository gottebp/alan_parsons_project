/*
 * menu_new.c - Non-blocking Menu System
 *
 * This is the new architecture menu that integrates with the Game state machine.
 * It replaces the blocking RunMenu() loop with per-frame update/render calls.
 */

#include "../../include_c/game/game.h"
#include "../../include_c/core/types.h"
#include "../../include_c/core/constants.h"
#include <string.h>
#include <stdlib.h>

/* External references for rendering (fallback if context not set) */
extern uint32_t* ScreenOff;
extern uint32_t ScreenTemp[];
extern void sseMemset32(uint32_t* dst, uint32_t value, int count);
extern void sseMemcpy32(uint32_t* dst, const uint32_t* src, int count);
extern void AlphaBlit(int x, int y, uint32_t* src, int src_width, int src_height);
extern void UpdateScreen(void);
extern int LoadBMP(uint32_t* buffer, const char* filename);
extern uint32_t Rand(void);

/*============================================================================
 * RENDER CONTEXT - Internal state set by menu_set_render_context()
 *============================================================================*/
static uint32_t* m_screen = NULL;
static int m_initialized = 0;

void menu_set_render_context(uint32_t* screen) {
    m_screen = screen;
    m_initialized = 1;
}

/*============================================================================
 * MENU ASSETS AND STATE
 *============================================================================*/
uint32_t* menu_background = NULL;
uint32_t* mouse_cursor = NULL;
uint8_t IsMenuRunning = 0;

void InitMenu(void) {
    /* Allocate and load menu background */
    menu_background = (uint32_t*)malloc(640 * 480 * sizeof(uint32_t));
    if (menu_background) {
        if (LoadBMP(menu_background, "data/main_menu.bmp") != 0) {
            free(menu_background);
            menu_background = NULL;
        }
    }

    /* Allocate and load mouse cursor */
    mouse_cursor = (uint32_t*)malloc(CURSOR_WIDTH * CURSOR_HEIGHT * sizeof(uint32_t));
    if (mouse_cursor) {
        if (LoadBMP(mouse_cursor, "data/cursor.bmp") != 0) {
            free(mouse_cursor);
            mouse_cursor = NULL;
        } else {
            /* Convert cyan (R=0, G=255, B=255) to transparent */
            for (int i = 0; i < CURSOR_WIDTH * CURSOR_HEIGHT; i++) {
                uint32_t pixel = mouse_cursor[i];
                uint8_t r = (pixel >> 16) & 0xFF;
                uint8_t g = (pixel >> 8) & 0xFF;
                uint8_t b = pixel & 0xFF;
                if (r == 0 && g == 255 && b == 255) {
                    mouse_cursor[i] = 0x00000000;
                }
            }
        }
    }
}

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

/* Menu asset dimensions (the menu background BMP is 640x480) */
#define MENU_WIDTH 640
#define MENU_HEIGHT 480

/* Offset to center menu on screen */
#define MENU_OFFSET_X ((SCREEN_WIDTH - MENU_WIDTH) / 2)
#define MENU_OFFSET_Y ((SCREEN_HEIGHT - MENU_HEIGHT) / 2)

/* Button definitions */
typedef struct {
    int x1, y1, x2, y2;     /* Button bounds */
    int mask_x, mask_y;     /* Highlight mask position */
    int required_level;     /* Level required to unlock (-1 = always available) */
    int result;             /* Menu result when clicked */
} MenuButton;

static const MenuButton BUTTONS[] = {
    /* Exit button */
    { 13, 13, 94, 115, 8, 2, -1, 0 },
    /* Shire (level 0) */
    { 163, 19, 306, 155, 163, 13, 0, 2 },
    /* Archipelago (level 1) */
    { 483, 36, 626, 153, 483, 30, 1, 3 },
    /* Dune (level 2) */
    { 480, 192, 623, 309, 480, 186, 2, 4 },
    /* Midkemia (level 3) */
    { 20, 316, 163, 433, 20, 310, 3, 5 },
    /* Oceania (level 4) */
    { 454, 342, 597, 459, 454, 336, 4, 6 },
    /* Mordor (level 5) */
    { 14, 157, 157, 274, 14, 151, 5, 7 },
};
#define NUM_BUTTONS 7

/* Mask dimensions */
#define MASK_WIDTH 143
#define MASK_HEIGHT 122

/* Static masks */
static uint32_t darkness_mask[MASK_WIDTH * MASK_HEIGHT];
static uint32_t random_mask[MASK_WIDTH * MASK_HEIGHT];
static int masks_initialized = 0;

/* Fade timing - keep fast to avoid appearing frozen */
#define FADE_IN_FRAMES 20
#define FADE_OUT_FRAMES 25

/*
 * Initialize masks if needed
 */
static void init_masks(void) {
    if (masks_initialized) return;

    for (int i = 0; i < MASK_WIDTH * MASK_HEIGHT; i++) {
        darkness_mask[i] = 0xB0000000;
        random_mask[i] = 0xB0000000;
    }
    masks_initialized = 1;
}

/*
 * Generate random highlight mask - creates TV static effect
 */
static void make_random_mask(void) {
    for (int i = 0; i < MASK_WIDTH * MASK_HEIGHT; i++) {
        uint32_t rand_val = Rand();
        /* Random brightness white pixels with moderate alpha for visible static */
        uint8_t brightness = (uint8_t)(rand_val % 180);
        uint8_t alpha = 60 + (uint8_t)((rand_val >> 8) % 60);  /* Alpha 60-120 */
        random_mask[i] = (alpha << 24) | (brightness << 16) | (brightness << 8) | brightness;
    }
}

/*
 * Check if point is in button bounds (adjusts for menu centering)
 */
static int point_in_button(int x, int y, const MenuButton* btn) {
    /* Adjust mouse coordinates to menu-local space */
    int mx = x - MENU_OFFSET_X;
    int my = y - MENU_OFFSET_Y;
    return (mx >= btn->x1 && mx <= btn->x2 && my >= btn->y1 && my <= btn->y2);
}

/*
 * Enter menu state
 */
void menu_enter(Game* game) {
    init_masks();

    game->state = STATE_MENU;
    game->menu.phase = MENU_PHASE_FADE_IN;
    game->menu.fade_frame = 0;
    game->menu.fade_total = FADE_IN_FRAMES;
    game->menu.selected_level = -1;
    game->menu.hover_button = -1;
    game->menu.click_pending = 0;
    game->menu_result = 0;
}

/*
 * Update menu - one frame
 * Returns 1 when menu is complete and result is ready
 */
int menu_update(Game* game, const InputState* input) {
    MenuState* m = &game->menu;

    switch (m->phase) {
        case MENU_PHASE_FADE_IN:
            m->fade_frame++;
            if (m->fade_frame >= m->fade_total) {
                m->phase = MENU_PHASE_ACTIVE;
            }
            break;

        case MENU_PHASE_ACTIVE:
            /* Generate new random mask each frame */
            make_random_mask();

            /* Handle escape key */
            if (input->escape) {
                m->selected_level = -1;
                m->phase = MENU_PHASE_FADE_OUT;
                m->fade_frame = 0;
                m->fade_total = FADE_OUT_FRAMES;
                game->menu_result = 0;
                break;
            }

            /* Check which button is hovered */
            m->hover_button = -1;
            for (int i = 0; i < NUM_BUTTONS; i++) {
                if (point_in_button(input->mouse_x, input->mouse_y, &BUTTONS[i])) {
                    /* Check if level is unlocked */
                    if (BUTTONS[i].required_level < 0 ||
                        game->unlocked_level >= BUTTONS[i].required_level) {
                        m->hover_button = i;
                    }
                    break;
                }
            }

            /* Handle click */
            if (input->mouse_left) {
                if (!m->click_pending && m->hover_button >= 0) {
                    m->click_pending = 1;
                }
            } else {
                if (m->click_pending && m->hover_button >= 0) {
                    /* Click released on button */
                    m->selected_level = m->hover_button;
                    game->menu_result = BUTTONS[m->hover_button].result;
                    m->phase = MENU_PHASE_FADE_OUT;
                    m->fade_frame = 0;
                    m->fade_total = FADE_OUT_FRAMES;
                }
                m->click_pending = 0;
            }
            break;

        case MENU_PHASE_FADE_OUT:
            m->fade_frame++;
            if (m->fade_frame >= m->fade_total) {
                m->phase = MENU_PHASE_DONE;
                return 1;  /* Menu complete */
            }
            break;

        case MENU_PHASE_DONE:
            return 1;  /* Already complete */
    }

    return 0;  /* Menu still active */
}

/*
 * Render menu - called each frame during STATE_MENU
 */
void menu_render(const Game* game) {
    const MenuState* menu = &game->menu;

    if (!menu_background) return;

    /* Use context if set, otherwise fall back to global */
    uint32_t* screen = m_initialized ? m_screen : ScreenOff;
    if (!screen) return;

    /* Clear full screen and draw centered background */
    sseMemset32(screen, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
    AlphaBlit(MENU_OFFSET_X, MENU_OFFSET_Y, menu_background, MENU_WIDTH, MENU_HEIGHT);

    /* Draw darkness masks on locked levels (offset for centering) */
    for (int i = 1; i < NUM_BUTTONS; i++) {  /* Skip exit button */
        if (BUTTONS[i].required_level > game->unlocked_level) {
            AlphaBlit(MENU_OFFSET_X + BUTTONS[i].mask_x,
                     MENU_OFFSET_Y + BUTTONS[i].mask_y,
                     darkness_mask, MASK_WIDTH, MASK_HEIGHT);
        }
    }

    /* Draw highlight on hovered button (offset for centering) */
    if (menu->hover_button >= 0) {
        AlphaBlit(MENU_OFFSET_X + BUTTONS[menu->hover_button].mask_x,
                 MENU_OFFSET_Y + BUTTONS[menu->hover_button].mask_y,
                 random_mask, MASK_WIDTH, MASK_HEIGHT);
    }

    /* Draw cursor at actual screen position */
    if (mouse_cursor) {
        extern uint16_t MOUSE_X, MOUSE_Y;
        AlphaBlit(MOUSE_X - CURSOR_WIDTH / 2, MOUSE_Y - CURSOR_HEIGHT / 2,
                 mouse_cursor, CURSOR_WIDTH, CURSOR_HEIGHT);
    }

    /* Apply fade effect to full screen */
    if (menu->phase == MENU_PHASE_FADE_IN || menu->phase == MENU_PHASE_FADE_OUT) {
        float progress;
        if (menu->phase == MENU_PHASE_FADE_IN) {
            progress = (float)menu->fade_frame / menu->fade_total;
        } else {
            progress = 1.0f - (float)menu->fade_frame / menu->fade_total;
        }

        /* Apply fade by darkening each pixel */
        uint8_t brightness = (uint8_t)(progress * 255);
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
            uint32_t pixel = screen[i];
            uint8_t r = ((pixel >> 16) & 0xFF) * brightness / 255;
            uint8_t g = ((pixel >> 8) & 0xFF) * brightness / 255;
            uint8_t b = (pixel & 0xFF) * brightness / 255;
            screen[i] = (pixel & 0xFF000000) | (r << 16) | (g << 8) | b;
        }
    }

    UpdateScreen();
}
