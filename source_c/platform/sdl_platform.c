/*
 * sdl_platform.c - SDL2 Implementation of Platform Layer
 *
 * This implements the platform.h interface using SDL2.
 * It bridges to the existing sdl_wrapper.c and input.c code,
 * allowing incremental migration to the new architecture.
 *
 * NOTE: This file avoids including headers that contain conflicting types.
 * Instead, it uses extern declarations and SDL headers directly.
 */

#include "../../include_c/platform/platform.h"

#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* Screen dimensions - must match defs.h */
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

/*============================================================================
 * EXTERNAL REFERENCES TO EXISTING CODE
 * (Declared directly to avoid header conflicts)
 *============================================================================*/

/* From sdl_wrapper.c */
extern uint32_t* ScreenOff;
extern uint32_t ScreenTemp[];
extern SDL_Window* screen_window;
extern SDL_Renderer* screen_renderer;
extern SDL_Texture* screen_texture;

/* Functions from sdl_wrapper.c */
extern int InitGraphics(int fullscreen);
extern void DestroyGraphics(void);
extern int LoadBMP(uint32_t* buffer, const char* filename);
extern void UpdateScreen(void);
extern uint32_t ComputeAlpha(uint32_t src, uint32_t dst);
extern void AlphaBlit(int x, int y, uint32_t* src, int src_width, int src_height);
extern void FadeFromBlack(int frames, int speed);
extern void FadeToBlack(int frames, int speed);
extern void FadeFromWhite(int frames, int speed);
extern void FadeToWhite(int frames, int speed);

/* From input.c */
extern uint8_t KEYBOARD[];
extern uint16_t MOUSE_X, MOUSE_Y;
extern uint16_t MOUSE_LBUTTON, MOUSE_RBUTTON;
extern int QUIT_SIGNAL;
extern void UpdateInput(void);

/* From sse_mem.c */
extern void sseMemcpy32(void* dst, const void* src, size_t count);
extern void sseMemset32(void* dst, uint32_t value, size_t count);

#ifdef __EMSCRIPTEN__
extern float mobile_tilt_steer;
extern float mobile_tilt_thrust;
extern int mobile_controls_active;
#endif

/*============================================================================
 * INITIALIZATION AND SHUTDOWN
 *============================================================================*/

int platform_init(int fullscreen) {
    return InitGraphics(fullscreen);
}

void platform_shutdown(void) {
    DestroyGraphics();
}

/*============================================================================
 * GRAPHICS
 *============================================================================*/

uint32_t* platform_get_screen_buffer(void) {
    return ScreenOff;
}

int platform_get_screen_width(void) {
    return SCREEN_WIDTH;
}

int platform_get_screen_height(void) {
    return SCREEN_HEIGHT;
}

void platform_clear_screen(uint32_t color) {
    sseMemset32(ScreenOff, color, SCREEN_WIDTH * SCREEN_HEIGHT);
}

void platform_present(void) {
    UpdateScreen();
}

int platform_load_bmp(uint32_t* buffer, const char* filename) {
    return LoadBMP(buffer, filename);
}

int platform_load_raw(uint32_t* buffer, const char* filename, size_t size) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return -1;
    }

    size_t read = fread(buffer, 1, size, f);
    fclose(f);

    if (read != size) {
        fprintf(stderr, "Failed to read %zu bytes from %s\n", size, filename);
        return -1;
    }

    return 0;
}

void platform_blit_alpha(int x, int y, const uint32_t* src, int width, int height) {
    AlphaBlit(x, y, (uint32_t*)src, width, height);
}

uint32_t platform_blend_alpha(uint32_t src, uint32_t dst) {
    return ComputeAlpha(src, dst);
}

void platform_fade_from_black(int frames, int speed) {
    /* Save current screen to temp buffer first */
    sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
    FadeFromBlack(frames, speed);
}

void platform_fade_to_black(int frames, int speed) {
    sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
    FadeToBlack(frames, speed);
}

void platform_fade_from_white(int frames, int speed) {
    sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
    FadeFromWhite(frames, speed);
}

void platform_fade_to_white(int frames, int speed) {
    sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
    FadeToWhite(frames, speed);
}

/*============================================================================
 * INPUT
 *============================================================================*/

void platform_update_input(PlatformInputState* input) {
    /* Update SDL input state */
    UpdateInput();

    /* Map to our InputState structure */
    input->up = KEYBOARD[SDL_SCANCODE_UP] || KEYBOARD[SDL_SCANCODE_W];
    input->down = KEYBOARD[SDL_SCANCODE_DOWN] || KEYBOARD[SDL_SCANCODE_S];
    input->left = KEYBOARD[SDL_SCANCODE_LEFT] || KEYBOARD[SDL_SCANCODE_A];
    input->right = KEYBOARD[SDL_SCANCODE_RIGHT] || KEYBOARD[SDL_SCANCODE_D];
    input->fire = KEYBOARD[SDL_SCANCODE_SPACE] || MOUSE_LBUTTON;
    input->nuke = KEYBOARD[SDL_SCANCODE_LCTRL] || KEYBOARD[SDL_SCANCODE_RCTRL] || MOUSE_RBUTTON;
    input->pause = KEYBOARD[SDL_SCANCODE_P];
    input->escape = KEYBOARD[SDL_SCANCODE_ESCAPE];

    input->mouse_x = MOUSE_X;
    input->mouse_y = MOUSE_Y;
    input->mouse_left = MOUSE_LBUTTON;
    input->mouse_right = MOUSE_RBUTTON;

#ifdef __EMSCRIPTEN__
    /* Mobile tilt controls */
    if (mobile_controls_active) {
        input->analog_steer = mobile_tilt_steer;
        input->analog_thrust = mobile_tilt_thrust;
    } else {
        input->analog_steer = 0.0f;
        input->analog_thrust = 0.0f;
    }
#else
    input->analog_steer = 0.0f;
    input->analog_thrust = 0.0f;
#endif
}

int platform_quit_requested(void) {
    return QUIT_SIGNAL;
}

/*============================================================================
 * AUDIO
 *============================================================================*/

SoundHandle platform_load_sound(const char* filename) {
    Mix_Chunk* chunk = Mix_LoadWAV(filename);
    if (!chunk) {
        fprintf(stderr, "Failed to load sound %s: %s\n", filename, Mix_GetError());
        return -1;
    }
    return (SoundHandle)(intptr_t)chunk;
}

MusicHandle platform_load_music(const char* filename) {
    Mix_Music* music = Mix_LoadMUS(filename);
    if (!music) {
        fprintf(stderr, "Failed to load music %s: %s\n", filename, Mix_GetError());
        return -1;
    }
    return (MusicHandle)(intptr_t)music;
}

void platform_play_sound(SoundHandle sound) {
    if (sound >= 0) {
        Mix_PlayChannel(-1, (Mix_Chunk*)(intptr_t)sound, 0);
    }
}

void platform_play_sound_loop(SoundHandle sound, int channel) {
    if (sound >= 0) {
        Mix_PlayChannel(channel, (Mix_Chunk*)(intptr_t)sound, -1);
    }
}

void platform_stop_channel(int channel) {
    Mix_HaltChannel(channel);
}

void platform_play_music(MusicHandle music, int loops) {
    if (music >= 0) {
        Mix_PlayMusic((Mix_Music*)(intptr_t)music, loops);
    }
}

void platform_fade_out_music(int ms) {
    Mix_FadeOutMusic(ms);
}

void platform_stop_music(void) {
    Mix_HaltMusic();
}

void platform_set_sound_volume(SoundHandle sound, int volume) {
    if (sound >= 0) {
        Mix_VolumeChunk((Mix_Chunk*)(intptr_t)sound, volume);
    }
}

void platform_set_music_volume(int volume) {
    Mix_VolumeMusic(volume);
}

void platform_free_sound(SoundHandle sound) {
    if (sound >= 0) {
        Mix_FreeChunk((Mix_Chunk*)(intptr_t)sound);
    }
}

void platform_free_music(MusicHandle music) {
    if (music >= 0) {
        Mix_FreeMusic((Mix_Music*)(intptr_t)music);
    }
}

/*============================================================================
 * TIME AND SLEEP
 *============================================================================*/

uint32_t platform_get_ticks(void) {
    return SDL_GetTicks();
}

void platform_sleep(int ms) {
#ifdef __EMSCRIPTEN__
    emscripten_sleep(ms);
#else
    SDL_Delay(ms);
#endif
}

/*============================================================================
 * PERSISTENCE
 *============================================================================*/

/* Simple file-based persistence */
static char persistence_path[256] = "save.dat";

void platform_save_int(const char* key, int value) {
    /* For now, just save to a simple file */
    /* A proper implementation would use a key-value store */
    FILE* f = fopen(persistence_path, "a");
    if (f) {
        fprintf(f, "%s=%d\n", key, value);
        fclose(f);
    }
}

int platform_load_int(const char* key, int default_value) {
    /* Simple implementation - in production would parse file properly */
    (void)key;
    return default_value;
}

/*============================================================================
 * CURSOR
 *============================================================================*/

void platform_show_cursor(int show) {
    SDL_ShowCursor(show ? SDL_ENABLE : SDL_DISABLE);
}

/*============================================================================
 * UTILITY
 *============================================================================*/

void platform_memcpy32(void* dst, const void* src, size_t count) {
    sseMemcpy32(dst, src, count);
}

void platform_memset32(void* dst, uint32_t value, size_t count) {
    sseMemset32(dst, value, count);
}

void platform_log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}
