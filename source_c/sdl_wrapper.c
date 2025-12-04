/*
 * Graphics system using SDL2
 * Converted from x86 assembly
 */

#include "sdl_wrapper.h"
#include "sse_mem.h"
#include "input.h"
#include <stdio.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* Global variables */
SDL_Window* screen_window = NULL;
SDL_Renderer* screen_renderer = NULL;
SDL_Texture* screen_texture = NULL;
uint32_t* ScreenOff = NULL;
uint32_t ScreenTemp[SCREEN_WIDTH * SCREEN_HEIGHT];

/*
 * Initialize graphics system
 */
int InitGraphics(int fullscreen) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

#ifdef __EMSCRIPTEN__
    /* EMSCRIPTEN: Disable vsync hint to prevent eglSwapInterval errors
     * Frame timing is controlled by emscripten_set_main_loop instead */
    SDL_SetHint(SDL_HINT_EMSCRIPTEN_ASYNCIFY, "0");
#endif

    uint32_t flags = SDL_WINDOW_SHOWN;
    if (fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

    screen_window = SDL_CreateWindow(
        "The Alan Parsons Project",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        flags
    );

    if (!screen_window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }

    /* Enable vsync for efficient frame limiting */
    /* EMSCRIPTEN NOTE: Don't use PRESENTVSYNC in browser - frame rate controlled by emscripten_set_main_loop */
#ifdef __EMSCRIPTEN__
    screen_renderer = SDL_CreateRenderer(screen_window, -1, SDL_RENDERER_ACCELERATED);
#else
    screen_renderer = SDL_CreateRenderer(screen_window, -1,
                                         SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#endif
    if (!screen_renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return -1;
    }

    screen_texture = SDL_CreateTexture(
        screen_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );

    if (!screen_texture) {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        return -1;
    }

    /* Allocate screen buffer */
    ScreenOff = (uint32_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    if (!ScreenOff) {
        fprintf(stderr, "Failed to allocate screen buffer\n");
        return -1;
    }

    /* Initialize SDL_mixer */
    if (Mix_OpenAudio(22050, AUDIO_S16LSB, 2, 4096) < 0) {
        fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
        return -1;
    }

    Mix_AllocateChannels(16);

    return 0;
}

/*
 * Destroy graphics system
 */
void DestroyGraphics(void) {
    if (ScreenOff) {
        free(ScreenOff);
        ScreenOff = NULL;
    }

    if (screen_texture) {
        SDL_DestroyTexture(screen_texture);
        screen_texture = NULL;
    }

    if (screen_renderer) {
        SDL_DestroyRenderer(screen_renderer);
        screen_renderer = NULL;
    }

    if (screen_window) {
        SDL_DestroyWindow(screen_window);
        screen_window = NULL;
    }

    Mix_CloseAudio();
    SDL_Quit();
}

/*
 * Load BMP file into buffer
 */
int LoadBMP(uint32_t* buffer, const char* filename) {
    SDL_Surface* surface = SDL_LoadBMP(filename);
    if (!surface) {
        fprintf(stderr, "Failed to load %s: %s\n", filename, SDL_GetError());
        return -1;
    }

    /* Convert BGR to ARGB */
    uint8_t* pixels = (uint8_t*)surface->pixels;
    int size = surface->w * surface->h;

    for (int i = 0; i < size; i++) {
        uint8_t b = pixels[i * 3];
        uint8_t g = pixels[i * 3 + 1];
        uint8_t r = pixels[i * 3 + 2];
        buffer[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    SDL_FreeSurface(surface);
    return 0;
}

/*
 * Alpha blend two pixels
 * Matches assembly implementation in macros.inc ComputeAlpha macro
 * Uses divide by 256 (shift right 8) and rounding factor of 128
 */
uint32_t ComputeAlpha(uint32_t src, uint32_t dst) {
    uint8_t src_a = (src >> 24) & 0xFF;
    uint8_t src_r = (src >> 16) & 0xFF;
    uint8_t src_g = (src >> 8) & 0xFF;
    uint8_t src_b = src & 0xFF;

    uint8_t dst_r = (dst >> 16) & 0xFF;
    uint8_t dst_g = (dst >> 8) & 0xFF;
    uint8_t dst_b = dst & 0xFF;

    /* Use 16-bit intermediates to prevent overflow, add rounding factor 128, divide by 256
     * This matches assembly: (src * alpha + dst * (255-alpha) + 128) >> 8
     */
    uint16_t out_r = ((uint16_t)src_r * src_a + (uint16_t)dst_r * (255 - src_a) + 128) >> 8;
    uint16_t out_g = ((uint16_t)src_g * src_a + (uint16_t)dst_g * (255 - src_a) + 128) >> 8;
    uint16_t out_b = ((uint16_t)src_b * src_a + (uint16_t)dst_b * (255 - src_a) + 128) >> 8;

    return 0xFF000000 | (out_r << 16) | (out_g << 8) | out_b;
}

/*
 * Alpha blit with clipping
 */
void AlphaBlit(int x, int y, uint32_t* src, int src_width, int src_height) {
    int orig_width = src_width;
    (void)src_height;  /* Suppress warning - not needed in current implementation */

    /* Clip left */
    if (x < 0) {
        if (-x >= src_width) return;
        src += -x;
        src_width += x;
        x = 0;
    }

    /* Clip top */
    if (y < 0) {
        if (-y >= src_height) return;
        src += (-y) * orig_width;
        src_height += y;
        y = 0;
    }

    /* Clip right */
    if (x + src_width > SCREEN_WIDTH) {
        src_width = SCREEN_WIDTH - x;
        if (src_width <= 0) return;
    }

    /* Clip bottom */
    if (y + src_height > SCREEN_HEIGHT) {
        src_height = SCREEN_HEIGHT - y;
        if (src_height <= 0) return;
    }

    /* Blit with alpha blending */
    for (int row = 0; row < src_height; row++) {
        for (int col = 0; col < src_width; col++) {
            int dst_idx = (y + row) * SCREEN_WIDTH + (x + col);
            int src_idx = row * orig_width + col;
            ScreenOff[dst_idx] = ComputeAlpha(src[src_idx], ScreenOff[dst_idx]);
        }
    }
}

/*
 * Update screen from buffer
 */
void UpdateScreen(void) {
    SDL_UpdateTexture(screen_texture, NULL, ScreenOff, SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(screen_renderer);
    SDL_RenderCopy(screen_renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(screen_renderer);
}

/*
 * Fade effects (simplified implementations)
 */
void FadeFromBlack(int frames, int speed) {
    /* Mirrors assembly _FadeFromBlack (main.asm:1473-1541) */

    /* Wait for all keys and mouse button to be released - mirrors assembly lines 1482-1503 */
    while (1) {
        UpdateInput();

        /* Check if escape is pressed - skip wait if so */
        if (KEYBOARD[SDL_SCANCODE_ESCAPE]) {
            break;
        }

        /* Count how many keys are pressed */
        int num_keys_pressed = 0;
        for (int i = 0; i < 128; i++) {
            if (KEYBOARD[i]) num_keys_pressed++;
        }

        /* Exit wait when no keys pressed and mouse not clicked */
        if (num_keys_pressed == 0 && MOUSE_LBUTTON == 0) {
            break;
        }

#ifdef __EMSCRIPTEN__
        /* EMSCRIPTEN: Yield to browser to prevent freezing */
        emscripten_sleep(16);
#endif
    }

    /* Start with fully opaque black, reduce alpha each frame - mirrors assembly line 1505 */
    uint32_t fade_color = 0xFF000000;

    for (int frame = 0; frame < frames; frame++) {
        /* Check for early exit during fade - mirrors assembly lines 1510-1526 */
        UpdateInput();

        int num_keys_pressed = 0;
        for (int i = 0; i < 128; i++) {
            if (KEYBOARD[i]) num_keys_pressed++;
        }

        if (MOUSE_LBUTTON == 1 || num_keys_pressed > 0) {
            break;  /* Exit fade early if key/mouse pressed */
        }

        /* Copy saved screen and apply fade overlay - mirrors assembly lines 1528-1530 */
        sseMemcpy32(ScreenOff, ScreenTemp, SCREEN_WIDTH * SCREEN_HEIGHT);

        /* Apply black overlay with current alpha (_FadeScreen) */
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
            ScreenOff[i] = ComputeAlpha(fade_color, ScreenOff[i]);
        }

        /* Update display - mirrors assembly lines 1532-1533 */
        UpdateScreen();
#ifdef __EMSCRIPTEN__
        /* EMSCRIPTEN: Use emscripten_sleep for proper yielding with ASYNCIFY */
        emscripten_sleep(10);
#else
        SDL_Delay(10);
#endif

        /* Reduce alpha (SaturatedAlphaSub) - mirrors assembly line 1531 */
        uint8_t alpha = (fade_color >> 24) & 0xFF;
        if (alpha >= speed) {
            alpha -= speed;
        } else {
            alpha = 0;
        }
        fade_color = (fade_color & 0x00FFFFFF) | (alpha << 24);
    }
}

void FadeToBlack(int frames, int speed) {
    /* Mirrors assembly _FadeToBlack (main.asm:1548-1610) */

    /* Wait for all keys and mouse button to be released - mirrors assembly lines 1552-1573 */
    while (1) {
        UpdateInput();

        /* Check if escape is pressed - skip wait if so */
        if (KEYBOARD[SDL_SCANCODE_ESCAPE]) {
            break;
        }

        /* Count how many keys are pressed */
        int num_keys_pressed = 0;
        for (int i = 0; i < 128; i++) {
            if (KEYBOARD[i]) num_keys_pressed++;
        }

        /* Exit wait when no keys pressed and mouse not clicked */
        if (num_keys_pressed == 0 && MOUSE_LBUTTON == 0) {
            break;
        }

#ifdef __EMSCRIPTEN__
        /* EMSCRIPTEN: Yield to browser to prevent freezing */
        emscripten_sleep(16);
#endif
    }

    /* Start with fully transparent black, increase alpha each frame - mirrors assembly line 1575 */
    uint32_t fade_color = 0x00000000;

    for (int frame = 0; frame < frames; frame++) {
        /* Check for early exit during fade - mirrors assembly lines 1579-1596 */
        UpdateInput();

        int num_keys_pressed = 0;
        for (int i = 0; i < 128; i++) {
            if (KEYBOARD[i]) num_keys_pressed++;
        }

        if (MOUSE_LBUTTON == 1 || num_keys_pressed > 0) {
            break;  /* Exit fade early if key/mouse pressed */
        }

        /* Copy saved screen and apply fade overlay - mirrors assembly lines 1598-1600 */
        sseMemcpy32(ScreenOff, ScreenTemp, SCREEN_WIDTH * SCREEN_HEIGHT);

        /* Apply black overlay with current alpha (_FadeScreen) */
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
            ScreenOff[i] = ComputeAlpha(fade_color, ScreenOff[i]);
        }

        /* Update display - mirrors assembly lines 1602-1603 */
        UpdateScreen();
#ifdef __EMSCRIPTEN__
        emscripten_sleep(10);
#else
        SDL_Delay(10);
#endif

        /* Increase alpha (SaturatedAlphaAdd) - mirrors assembly line 1601 */
        uint8_t alpha = (fade_color >> 24) & 0xFF;
        if (alpha <= 255 - speed) {
            alpha += speed;
        } else {
            alpha = 255;
        }
        fade_color = (fade_color & 0x00FFFFFF) | (alpha << 24);
    }
}

/*
 * Fade from white - mirrors assembly main.asm lines 1330-1398
 */
void FadeFromWhite(int frames, int speed) {
    extern void FlushKeyboard(void);
    extern uint8_t KEYBOARD[320];
    extern uint16_t MOUSE_LBUTTON;
    extern void UpdateInput(void);

    /* Wait for keys to be released */
    FlushKeyboard();
    UpdateInput();

    int done = 0;
    while (!done) {
        int num_keys_pressed = 0;
        for (int i = 0; i < 128; i++) {
            if (KEYBOARD[i]) num_keys_pressed++;
        }
        if (num_keys_pressed == 0 && MOUSE_LBUTTON == 0) {
            done = 1;
        }
        UpdateInput();
#ifdef __EMSCRIPTEN__
        emscripten_sleep(16);
#endif
    }

    /* Start with fully opaque WHITE, reduce alpha each frame */
    uint32_t fade_color = 0xFFFFFFFF;

    for (int frame = 0; frame < frames; frame++) {
        UpdateInput();

        int num_keys_pressed = 0;
        for (int i = 0; i < 128; i++) {
            if (KEYBOARD[i]) num_keys_pressed++;
        }
        if (MOUSE_LBUTTON == 1 || num_keys_pressed > 0) break;

        sseMemcpy32(ScreenOff, ScreenTemp, SCREEN_WIDTH * SCREEN_HEIGHT);

        /* Apply white fade overlay */
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
            ScreenOff[i] = ComputeAlpha(fade_color, ScreenOff[i]);
        }

        UpdateScreen();
#ifdef __EMSCRIPTEN__
        emscripten_sleep(10);
#else
        SDL_Delay(10);
#endif

        /* Reduce alpha */
        uint8_t alpha = (fade_color >> 24) & 0xFF;
        if (alpha >= (uint8_t)speed) {
            alpha -= speed;
        } else {
            alpha = 0;
        }
        fade_color = (fade_color & 0x00FFFFFF) | ((uint32_t)alpha << 24);
    }
}

/*
 * Fade to white - mirrors assembly main.asm lines 1406-1470
 */
void FadeToWhite(int frames, int speed) {
    extern void FlushKeyboard(void);
    extern uint8_t KEYBOARD[320];
    extern uint16_t MOUSE_LBUTTON;
    extern void UpdateInput(void);

    /* Wait for keys to be released */
    FlushKeyboard();
    UpdateInput();

    int done = 0;
    while (!done) {
        int num_keys_pressed = 0;
        for (int i = 0; i < 128; i++) {
            if (KEYBOARD[i]) num_keys_pressed++;
        }
        if (num_keys_pressed == 0 && MOUSE_LBUTTON == 0) {
            done = 1;
        }
        UpdateInput();
#ifdef __EMSCRIPTEN__
        emscripten_sleep(16);
#endif
    }

    /* Start with transparent WHITE, increase alpha each frame */
    uint32_t fade_color = 0x00FFFFFF;

    for (int frame = 0; frame < frames; frame++) {
        UpdateInput();

        int num_keys_pressed = 0;
        for (int i = 0; i < 128; i++) {
            if (KEYBOARD[i]) num_keys_pressed++;
        }
        if (MOUSE_LBUTTON == 1 || num_keys_pressed > 0) break;

        sseMemcpy32(ScreenOff, ScreenTemp, SCREEN_WIDTH * SCREEN_HEIGHT);

        /* Apply white fade overlay */
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
            ScreenOff[i] = ComputeAlpha(fade_color, ScreenOff[i]);
        }

        UpdateScreen();
#ifdef __EMSCRIPTEN__
        emscripten_sleep(10);
#else
        SDL_Delay(10);
#endif

        /* Increase alpha */
        uint8_t alpha = (fade_color >> 24) & 0xFF;
        if (alpha <= 255 - speed) {
            alpha += speed;
        } else {
            alpha = 255;
        }
        fade_color = (fade_color & 0x00FFFFFF) | ((uint32_t)alpha << 24);
    }
}
