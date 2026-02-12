/*
 * Input handling system
 * Converted from x86 assembly - mirrors assembly implementation
 */

#include "input.h"
#include "sdl_wrapper.h"
#include "sse_mem.h"
#include <string.h>

#ifdef __EMSCRIPTEN__
/* JavaScript-tracked mouse position (in game logical coordinates) */
static int js_mouse_x = 0;
static int js_mouse_y = 0;
static int js_mouse_active = 0;
#endif

/* Global input state - mirrors assembly exactly */
uint8_t KEYBOARD[320] = {0};        /* Matches assembly's 320 bytes */
uint16_t MOUSE_X = 0;               /* Matches assembly's word */
uint16_t MOUSE_Y = 0;               /* Matches assembly's word */
uint16_t MOUSE_LBUTTON = 0;         /* Matches assembly's word */
uint16_t MOUSE_RBUTTON = 0;         /* Matches assembly's word */
uint16_t MOUSE_MBUTTON = 0;         /* Matches assembly's word */
uint8_t MOUSE_BUTTON = 0;           /* Raw mouse button state */
int QUIT_SIGNAL = 0;

/*
 * Update input state - mirrors assembly implementation
 * Uses SDL_PumpEvents + SDL_GetMouseState + SDL_GetKeyState
 */
void UpdateInput(void) {
    const uint8_t* kb_ptr;
    uint32_t mouse_state;
    int raw_mx, raw_my;

    /* Pump SDL events */
    SDL_PumpEvents();

    /* Get mouse state into int locals (avoids uint16_t* cast bug) */
    mouse_state = SDL_GetMouseState(&raw_mx, &raw_my);
    MOUSE_BUTTON = (uint8_t)mouse_state;

    /* Convert window coords to logical coords */
    if (screen_renderer) {
        int out_w, out_h;
        SDL_GetRendererOutputSize(screen_renderer, &out_w, &out_h);

        float scale_x = (float)out_w / SCREEN_WIDTH;
        float scale_y = (float)out_h / SCREEN_HEIGHT;
        float scale = (scale_x < scale_y) ? scale_x : scale_y;

        int viewport_w = (int)(SCREEN_WIDTH * scale);
        int viewport_h = (int)(SCREEN_HEIGHT * scale);
        int offset_x = (out_w - viewport_w) / 2;
        int offset_y = (out_h - viewport_h) / 2;

        int lx = (int)((raw_mx - offset_x) / scale);
        int ly = (int)((raw_my - offset_y) / scale);

        /* Clamp to logical bounds */
        if (lx < 0) lx = 0;
        if (lx >= SCREEN_WIDTH) lx = SCREEN_WIDTH - 1;
        if (ly < 0) ly = 0;
        if (ly >= SCREEN_HEIGHT) ly = SCREEN_HEIGHT - 1;

        MOUSE_X = (uint16_t)lx;
        MOUSE_Y = (uint16_t)ly;
    } else {
        /* Renderer not yet initialized, use raw coords */
        MOUSE_X = (uint16_t)raw_mx;
        MOUSE_Y = (uint16_t)raw_my;
    }

#ifdef __EMSCRIPTEN__
    /* Override with JavaScript-tracked position (handles mousemove reliably) */
    if (js_mouse_active) {
        MOUSE_X = (uint16_t)js_mouse_x;
        MOUSE_Y = (uint16_t)js_mouse_y;
    }
#endif

    /* Extract individual button states (mirrors assembly logic) */
    MOUSE_LBUTTON = 0;
    if (MOUSE_BUTTON & 1) {  /* SDL_BUTTON_LMASK = 1 */
        MOUSE_LBUTTON = 1;
    }

    MOUSE_RBUTTON = 0;
    if (MOUSE_BUTTON & 4) {  /* SDL_BUTTON_RMASK = 4 */
        MOUSE_RBUTTON = 1;
    }

    MOUSE_MBUTTON = 0;
    if (MOUSE_BUTTON & 2) {  /* SDL_BUTTON_MMASK = 2 */
        MOUSE_MBUTTON = 1;
    }

    /* Get keyboard state and copy to our buffer (mirrors assembly) */
    kb_ptr = SDL_GetKeyboardState(NULL);

    /* Copy 320 bytes / 4 = 80 dwords (mirrors assembly's invoke _sseMemcpy32) */
    sseMemcpy32(KEYBOARD, kb_ptr, 80);
}

/*
 * Clear keyboard state - mirrors assembly implementation
 */
void FlushKeyboard(void) {
    const uint8_t* kb_ptr;

    /* Get keyboard state pointer from SDL */
    kb_ptr = SDL_GetKeyboardState(NULL);

    /* Clear 80 dwords (320 bytes) directly in SDL's state (mirrors assembly) */
    sseMemset32((void*)kb_ptr, 0, 80);
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

/* Mobile input state - analog values for smooth control */
float mobile_tilt_steer = 0.0f;   /* Left/right tilt: -1.0 (left) to 1.0 (right) */
float mobile_tilt_thrust = 0.0f;  /* Forward/back tilt: -1.0 (back) to 1.0 (forward) */
int mobile_controls_active = 0;    /* Flag indicating mobile controls are being used */

/*
 * Handle virtual key press from mobile buttons
 * EMSCRIPTEN: Called from JavaScript when touch buttons are pressed
 */
EMSCRIPTEN_KEEPALIVE
void handle_virtual_key(int sdl_scancode, int pressed) {
    /* Directly set keyboard state for this scancode */
    if (sdl_scancode >= 0 && sdl_scancode < 320) {
        KEYBOARD[sdl_scancode] = pressed ? 1 : 0;
    }
}

/*
 * Handle device tilt input for analog steering and thrust
 * EMSCRIPTEN: Called from JavaScript with gamma (steer) and beta (thrust) values
 * steer: -1.0 (tilt left) to 1.0 (tilt right) for ship rotation
 * thrust: -1.0 (tilt back) to 1.0 (tilt forward) for acceleration
 */
EMSCRIPTEN_KEEPALIVE
void handle_tilt_input(float steer, float thrust) {
    mobile_controls_active = 1;
    mobile_tilt_steer = steer;
    mobile_tilt_thrust = thrust;

    /* Clamp values to valid range */
    if (mobile_tilt_steer > 1.0f) mobile_tilt_steer = 1.0f;
    if (mobile_tilt_steer < -1.0f) mobile_tilt_steer = -1.0f;
    if (mobile_tilt_thrust > 1.0f) mobile_tilt_thrust = 1.0f;
    if (mobile_tilt_thrust < -1.0f) mobile_tilt_thrust = -1.0f;
}

/*
 * Get the current analog steering value
 * Returns: -1.0 (full left) to 1.0 (full right), 0.0 = no input
 */
EMSCRIPTEN_KEEPALIVE
float get_mobile_steer(void) {
    return mobile_tilt_steer;
}

/*
 * Get the current analog thrust value
 * Returns: -1.0 (full reverse) to 1.0 (full forward), 0.0 = no input
 */
EMSCRIPTEN_KEEPALIVE
float get_mobile_thrust(void) {
    return mobile_tilt_thrust;
}

/*
 * Check if mobile controls are active
 */
EMSCRIPTEN_KEEPALIVE
int is_mobile_active(void) {
    return mobile_controls_active;
}

/*
 * Handle mouse move from JavaScript
 * EMSCRIPTEN: Called from JavaScript mousemove handler with logical coordinates
 */
EMSCRIPTEN_KEEPALIVE
void handle_mouse_move(int x, int y) {
    js_mouse_x = x;
    js_mouse_y = y;
    js_mouse_active = 1;
}

/*
 * Handle touch cursor for menu navigation
 * EMSCRIPTEN: Called from JavaScript with touch coordinates
 */
EMSCRIPTEN_KEEPALIVE
void handle_touch_cursor(int x, int y, int is_pressed) {
    /* Always update position (for both touch-down and touch-move) */
    js_mouse_x = x;
    js_mouse_y = y;
    js_mouse_active = 1;
    MOUSE_LBUTTON = is_pressed ? 1 : 0;
}

#endif  /* __EMSCRIPTEN__ */
