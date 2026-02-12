/*
 * platform.h - Platform Abstraction Layer
 *
 * This header defines the interface between the game and the platform.
 * The game never calls SDL, Emscripten, or OS functions directly -
 * it calls these functions, which are implemented per-platform.
 *
 * This separation allows:
 * - Clean game code that doesn't know about platform details
 * - Easy porting to new platforms
 * - Testing game logic without platform dependencies
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stddef.h>  /* For size_t */

/* Forward declaration to avoid including types.h which may conflict */
struct InputState;

/*============================================================================
 * INITIALIZATION AND SHUTDOWN
 *============================================================================*/

/* Initialize the platform (graphics, audio, input) */
int platform_init(int fullscreen);

/* Shutdown and cleanup */
void platform_shutdown(void);

/*============================================================================
 * GRAPHICS
 *============================================================================*/

/* Screen buffer access */
uint32_t* platform_get_screen_buffer(void);
int platform_get_screen_width(void);
int platform_get_screen_height(void);

/* Frame operations */
void platform_clear_screen(uint32_t color);
void platform_present(void);

/* Image loading */
int platform_load_bmp(uint32_t* buffer, const char* filename);
int platform_load_raw(uint32_t* buffer, const char* filename, size_t size);

/* Alpha blitting */
void platform_blit_alpha(int x, int y, const uint32_t* src, int width, int height);
uint32_t platform_blend_alpha(uint32_t src, uint32_t dst);

/* Fade effects */
void platform_fade_from_black(int frames, int speed);
void platform_fade_to_black(int frames, int speed);
void platform_fade_from_white(int frames, int speed);
void platform_fade_to_white(int frames, int speed);

/*============================================================================
 * INPUT
 *============================================================================*/

/* Input state structure - defined here to avoid conflicts */
typedef struct PlatformInputState {
    /* Digital inputs */
    int up, down, left, right;
    int fire, nuke;
    int strafe_left, strafe_right;
    int pause, escape;

    /* Mouse */
    int mouse_x, mouse_y;
    int mouse_left, mouse_right;

    /* Analog (for mobile tilt controls) */
    float analog_steer;   /* -1.0 to 1.0 */
    float analog_thrust;  /* -1.0 to 1.0 */
} PlatformInputState;

/* Update input state - call once per frame */
void platform_update_input(PlatformInputState* input);

/* Check if user requested quit */
int platform_quit_requested(void);

/*============================================================================
 * AUDIO
 *============================================================================*/

/* Sound effect handle */
typedef int SoundHandle;

/* Music handle */
typedef int MusicHandle;

/* Load sounds */
SoundHandle platform_load_sound(const char* filename);
MusicHandle platform_load_music(const char* filename);

/* Play sounds */
void platform_play_sound(SoundHandle sound);
void platform_play_sound_loop(SoundHandle sound, int channel);
void platform_stop_channel(int channel);

/* Play music */
void platform_play_music(MusicHandle music, int loops);
void platform_fade_out_music(int ms);
void platform_stop_music(void);

/* Volume control */
void platform_set_sound_volume(SoundHandle sound, int volume);
void platform_set_music_volume(int volume);

/* Cleanup */
void platform_free_sound(SoundHandle sound);
void platform_free_music(MusicHandle music);

/*============================================================================
 * TIME AND SLEEP
 *============================================================================*/

/* Get milliseconds since program start */
uint32_t platform_get_ticks(void);

/* Sleep for given milliseconds (yields to OS/browser) */
void platform_sleep(int ms);

/*============================================================================
 * PERSISTENCE
 *============================================================================*/

/* Save/load game progress */
void platform_save_int(const char* key, int value);
int platform_load_int(const char* key, int default_value);

/*============================================================================
 * CURSOR
 *============================================================================*/

void platform_show_cursor(int show);

/*============================================================================
 * UTILITY
 *============================================================================*/

/* Fast memory operations */
void platform_memcpy32(void* dst, const void* src, size_t count);
void platform_memset32(void* dst, uint32_t value, size_t count);

/* Debug output */
void platform_log(const char* format, ...);

#endif /* PLATFORM_H */
