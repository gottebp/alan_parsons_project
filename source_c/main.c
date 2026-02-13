/*
 * main_pure.c - The Crystalline Main Loop
 *
 * The Alan Parsons Project
 * A particle-based space shooter game
 *
 * Originally written in x86 assembly by Benjamin Gottemoller (ECE291, 2002)
 * Translated to C, then transformed into this clean architecture.
 *
 * This main loop is what the game aspires to be:
 * - Initialize
 * - Loop: input -> update -> render
 * - Shutdown
 *
 * All complexity lives in App (platform) and Game (logic).
 * Here we just orchestrate the dance.
 */

#include "../include_c/platform/app.h"
#include "../include_c/game/game.h"
#include "../include_c/game/render.h"
#include "../include_c/core/constants.h"
#include <stdio.h>

/* From app.c - global for legacy compatibility */
extern int game_level;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

/* Global app for Emscripten callback */
static App* g_app = NULL;

/* Screen update and temp buffer */
extern void UpdateScreen(void);
extern uint32_t ScreenTemp[];
extern uint32_t* ScreenOff;
extern void sseMemcpy32(uint32_t* dst, const uint32_t* src, int count);
extern void sseMemset32(void* dest, uint32_t value, unsigned long count);
extern void AlphaBlit(int x, int y, uint32_t* src, int src_width, int src_height);
extern uint32_t ComputeAlpha(uint32_t src, uint32_t dst);
extern void UpdateInput(void);
extern void FlushKeyboard(void);
extern uint8_t KEYBOARD[320];
extern uint16_t MOUSE_LBUTTON;
extern int Mix_FadeOutMusic(int ms);
extern void* Mix_LoadMUS(const char* file);
extern int Mix_VolumeMusic(int volume);
extern int Mix_PlayMusic(void* music, int loops);
extern int Mix_HaltChannel(int channel);

/*------------------------------------------------------------------------
 * Non-blocking ending sequence state machine (WASM only)
 *
 * The blocking app_ending_sequence() uses emscripten_sleep() inside fade
 * loops, which can't unwind through the main loop callback's ASYNCIFY
 * context. Instead, we drive the ending one frame at a time from the
 * main loop callback — no sleeps, no blocking.
 *------------------------------------------------------------------------*/

enum {
    END_FADE_OUT_MUSIC,     /* Wait ~12 frames for music fade */
    END_START_MUSIC,        /* Load + play ending music */
    END_FADE_TO_WHITE,      /* Fade current screen to white */
    END_SETUP_CLIP,         /* Blit next story clip into ScreenTemp */
    END_FADE_FROM_WHITE,    /* Fade white to reveal clip */
    END_WAIT_INPUT,         /* Wait for keypress/click or timeout */
    END_FADE_TO_BLACK,      /* Final fade out */
    END_DONE                /* Save progress, transition to menu */
};

typedef struct {
    int active;
    int phase;
    int frame;
    int clip_index;         /* Which story clip (0-2) */
    int target_frames;
    int speed;
    uint32_t fade_color;
} EndingState;

static EndingState g_ending = {0};

static void ending_start(App* app) {
    g_ending.active = 1;
    g_ending.phase = END_FADE_OUT_MUSIC;
    g_ending.frame = 0;
    g_ending.clip_index = 0;
    g_ending.fade_color = 0;
    app->in_ending = 1;
    Mix_FadeOutMusic(100);
}

/* Advance one frame of the ending sequence. Called from emscripten_frame. */
static void ending_tick(App* app) {
    uint8_t alpha;

    switch (g_ending.phase) {

    case END_FADE_OUT_MUSIC:
        g_ending.frame++;
        if (g_ending.frame >= 12) { /* ~200ms at 60fps */
            g_ending.phase = END_START_MUSIC;
            g_ending.frame = 0;
        }
        break;

    case END_START_MUSIC: {
        AudioAssets* a = &app->audio;
        if (!a->music_ending)
            a->music_ending = Mix_LoadMUS("./sound/ending_theme.ogg");
        if (a->music_ending) {
            Mix_VolumeMusic(110);
            Mix_PlayMusic(a->music_ending, 1);
        }
        Mix_HaltChannel(-1);

        if (!app->visuals.story_clips) {
            /* No clips — skip straight to done */
            g_ending.phase = END_DONE;
            break;
        }

        /* Set up fade-to-white from the current game screen */
        sseMemcpy32(ScreenTemp, app->render.screen, SCREEN_WIDTH * SCREEN_HEIGHT);
        g_ending.phase = END_FADE_TO_WHITE;
        g_ending.frame = 0;
        g_ending.target_frames = 500;
        g_ending.speed = 2;
        g_ending.fade_color = 0x00FFFFFF; /* transparent white */
        break;
    }

    case END_FADE_TO_WHITE:
        /* One frame of fade: copy base image, blend white overlay, present */
        sseMemcpy32(ScreenOff, ScreenTemp, SCREEN_WIDTH * SCREEN_HEIGHT);
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
            ScreenOff[i] = ComputeAlpha(g_ending.fade_color, ScreenOff[i]);
        UpdateScreen();

        alpha = (g_ending.fade_color >> 24) & 0xFF;
        if (alpha <= 255 - g_ending.speed)
            alpha += g_ending.speed;
        else
            alpha = 255;
        g_ending.fade_color = (g_ending.fade_color & 0x00FFFFFF) | ((uint32_t)alpha << 24);

        g_ending.frame++;
        if (g_ending.frame >= g_ending.target_frames) {
            g_ending.phase = END_SETUP_CLIP;
            g_ending.frame = 0;
        }
        return; /* Already presented, skip normal render */

    case END_SETUP_CLIP: {
        /* Blit the current clip into screen and ScreenTemp */
        int idx = g_ending.clip_index;
        sseMemset32(app->render.screen, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
        sseMemset32(ScreenOff, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
        AlphaBlit(SCREEN_WIDTH / 2 - 320, SCREEN_HEIGHT / 2 - 240,
                  app->visuals.story_clips + (640 * 480 * idx), 640, 480);
        sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);

        g_ending.phase = END_FADE_FROM_WHITE;
        g_ending.frame = 0;
        g_ending.target_frames = 300;
        g_ending.speed = 2;
        g_ending.fade_color = 0xFFFFFFFF; /* opaque white */
        break;
    }

    case END_FADE_FROM_WHITE:
        sseMemcpy32(ScreenOff, ScreenTemp, SCREEN_WIDTH * SCREEN_HEIGHT);
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
            ScreenOff[i] = ComputeAlpha(g_ending.fade_color, ScreenOff[i]);
        UpdateScreen();

        alpha = (g_ending.fade_color >> 24) & 0xFF;
        if (alpha >= g_ending.speed)
            alpha -= g_ending.speed;
        else
            alpha = 0;
        g_ending.fade_color = (g_ending.fade_color & 0x00FFFFFF) | ((uint32_t)alpha << 24);

        g_ending.frame++;
        if (g_ending.frame >= g_ending.target_frames) {
            g_ending.clip_index++;
            if (g_ending.clip_index < 3) {
                /* More clips — fade to white then show next */
                g_ending.phase = END_FADE_TO_WHITE;
                g_ending.frame = 0;
                g_ending.target_frames = 300;
                g_ending.speed = 2;
                g_ending.fade_color = 0x00FFFFFF;
            } else {
                /* All clips shown — wait for input */
                g_ending.phase = END_WAIT_INPUT;
                g_ending.frame = 0;
                FlushKeyboard();
            }
        }
        return;

    case END_WAIT_INPUT:
        UpdateInput();
        for (int i = 0; i < 128; i++) {
            if (KEYBOARD[i]) { g_ending.phase = END_FADE_TO_BLACK; goto setup_final_fade; }
        }
        if (MOUSE_LBUTTON) { g_ending.phase = END_FADE_TO_BLACK; goto setup_final_fade; }
        g_ending.frame++;
        if (g_ending.frame >= 900) {
            g_ending.phase = END_FADE_TO_BLACK;
        setup_final_fade:
            g_ending.frame = 0;
            g_ending.target_frames = 120;
            g_ending.speed = 3;
            g_ending.fade_color = 0x00000000; /* transparent black */
            sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
        }
        break;

    case END_FADE_TO_BLACK:
        sseMemcpy32(ScreenOff, ScreenTemp, SCREEN_WIDTH * SCREEN_HEIGHT);
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
            ScreenOff[i] = ComputeAlpha(g_ending.fade_color, ScreenOff[i]);
        UpdateScreen();

        alpha = (g_ending.fade_color >> 24) & 0xFF;
        if (alpha <= 255 - g_ending.speed)
            alpha += g_ending.speed;
        else
            alpha = 255;
        g_ending.fade_color = (g_ending.fade_color & 0x00FFFFFF) | ((uint32_t)alpha << 24);

        g_ending.frame++;
        if (g_ending.frame >= g_ending.target_frames) {
            g_ending.phase = END_DONE;
        }
        return;

    case END_DONE:
        EM_ASM(
            localStorage.setItem('alan_parsons_beaten', '1');
            console.log('Game beaten! Captain Planet mode unlocked.');
        );
        g_ending.active = 0;
        app->in_ending = 0;
        app_show_menu(app);
        break;
    }
}

/* Forward declaration */
static void emscripten_frame(void);

static void handle_emscripten_quit(void) {
    /* Stop all audio */
    extern int Mix_HaltMusic(void);
    extern int Mix_HaltChannel(int);
    Mix_HaltMusic();
    Mix_HaltChannel(-1);

    /* Clear screen */
    app_clear_screen(g_app, 0);
    UpdateScreen();

    /* Show restart overlay in browser */
    EM_ASM(
        console.log('Game exited - showing restart overlay');
        window.gameStarted = false;
        var overlay = document.getElementById('start-overlay');
        if (overlay) {
            overlay.classList.remove('hidden');
            var h2 = overlay.querySelector('h2');
            if (h2) h2.textContent = 'Click to Play Again';
            var p = overlay.querySelector('p');
            if (p) p.textContent = 'Click anywhere to restart';
        }
        var canvas = document.getElementById('canvas');
        if (canvas) canvas.classList.remove('game-active');
    );

    /* Cancel loop and wait for restart */
    emscripten_cancel_main_loop();
    RestartGame();

    /* Restart the main loop */
    emscripten_set_main_loop(emscripten_frame, 0, 1);
}

static void emscripten_frame(void) {
    if (!g_app) {
        emscripten_cancel_main_loop();
        return;
    }

    /* Check for quit signal */
    extern int QUIT_SIGNAL;
    if (QUIT_SIGNAL) {
        emscripten_cancel_main_loop();
        return;
    }

    /* Frame timing */
    float dt = app_frame_begin(g_app);

    /* Input */
    app_poll_input(g_app);

    /* State-based update */
    switch (g_app->game.state) {

        case STATE_MENU:
            /* Update menu through game_update (fixed timestep) */
            game_update(&g_app->game, &g_app->input, dt);

            /* Check if menu completed */
            if (g_app->game.menu.phase == MENU_PHASE_DONE) {
                /* Check for quit */
                if (g_app->game.menu_result == 0) {
                    handle_emscripten_quit();
                    return;
                }
                /* Menu complete - handle result */
                app_handle_menu_result(g_app, g_app->game.menu_result);
                break;
            }

            /* Render menu */
            menu_render(&g_app->game);
            break;

        case STATE_PLAYING:
            /* Check for escape to menu */
            if (g_app->input.escape) {
                app_show_menu(g_app);
                break;
            }

            /* Update game */
            game_update(&g_app->game, &g_app->input, dt);

            /* Render game */
            app_clear_screen(g_app, 0);
            render_frame(&g_app->game);
            render_present(&g_app->game);
            break;

        case STATE_VICTORY:
            /* Update game (fixed timestep handles outcome_timer) */
            game_update(&g_app->game, &g_app->input, dt);

            /* Check if game logic requests menu transition */
            if (g_app->game.menu_requested) {
                g_app->game.menu_requested = 0;
                if (g_app->game.current_level_idx >= 5) {
                    /* Start non-blocking ending state machine */
                    g_app->game.state = STATE_ENDING;
                    ending_start(g_app);
                    break;
                }
                app_show_menu(g_app);
            }

            /* Continue rendering */
            app_clear_screen(g_app, 0);
            render_frame(&g_app->game);
            render_present(&g_app->game);
            break;

        case STATE_DEFEAT:
            /* Update game (fixed timestep handles outcome_timer) */
            game_update(&g_app->game, &g_app->input, dt);

            /* Check if game logic requests menu transition */
            if (g_app->game.menu_requested) {
                g_app->game.menu_requested = 0;
                app_show_menu(g_app);
            }

            /* Continue rendering */
            app_clear_screen(g_app, 0);
            render_frame(&g_app->game);
            render_present(&g_app->game);
            break;

        case STATE_ENDING:
            if (g_ending.active)
                ending_tick(g_app);
            break;
    }

    /* Audio update */
    app_update_audio(g_app);

    /* Frame end */
    app_frame_end(g_app);
}

#endif

/*
 * The Main Function
 *
 * In its ideal form, main is simple:
 * 1. Initialize
 * 2. Show title
 * 3. Loop until done
 * 4. Shutdown
 */
int main(int argc, char* argv[]) {
    App app;

    /* Initialize everything */
    if (app_init(&app, argc, argv) < 0) {
        fprintf(stderr, "Failed to initialize\n");
        return 1;
    }

#ifdef __EMSCRIPTEN__
    /* Wait for user to click canvas (enables audio) */
    printf("Waiting for user to click canvas...\n");
    while (1) {
        int started = EM_ASM_INT({ return window.gameStarted ? 1 : 0; });
        if (started) break;
        emscripten_sleep(100);
    }
#endif

    /* Title screen with intro */
    app_title_screen(&app);

    /* Initial menu */
    app_show_menu(&app);

#ifdef __EMSCRIPTEN__
    /* Emscripten: use async main loop */
    g_app = &app;
    app.in_main_loop = 1;
    app_set_restart_pointer(&app);
    emscripten_set_main_loop(emscripten_frame, 0, 1);
#else
    /*
     * The Game Loop - Native Build
     *
     * This is the heartbeat of the game:
     * - Begin frame (timing, input)
     * - Process input (menu, quit)
     * - Update game logic
     * - Render
     * - End frame (present, rate limit)
     *
     * The loop handles three states:
     * - STATE_MENU: Non-blocking menu selection
     * - STATE_PLAYING: Active gameplay
     * - STATE_VICTORY/STATE_DEFEAT: Outcome display
     */
    while (app_running(&app)) {
        /* Frame timing */
        float dt = app_frame_begin(&app);

        /* Input */
        app_poll_input(&app);

        /* State-based update */
        switch (app.game.state) {

            case STATE_MENU:
                /* Update menu through game_update (fixed timestep) */
                game_update(&app.game, &app.input, dt);

                /* Check if menu completed */
                if (app.game.menu.phase == MENU_PHASE_DONE) {
                    /* Menu complete - handle result */
                    app_handle_menu_result(&app, app.game.menu_result);
                    break;
                }

                /* Render menu */
                menu_render(&app.game);
                break;

            case STATE_PLAYING:
                /* Check for escape to menu */
                if (app.input.escape) {
                    app_show_menu(&app);
                    break;
                }

                /* Update game */
                game_update(&app.game, &app.input, dt);

                /* Render game */
                app_clear_screen(&app, 0);
                render_frame(&app.game);
                render_present(&app.game);
                break;

            case STATE_VICTORY:
                /* Update game (fixed timestep handles outcome_timer) */
                game_update(&app.game, &app.input, dt);

                /* Check if game logic requests menu transition */
                if (app.game.menu_requested) {
                    app.game.menu_requested = 0;
                    if (app.game.current_level_idx >= 5) {
                        app_ending_sequence(&app);
                    }
                    app_show_menu(&app);
                }

                /* Continue rendering */
                app_clear_screen(&app, 0);
                render_frame(&app.game);
                render_present(&app.game);
                break;

            case STATE_DEFEAT:
                /* Update game (fixed timestep handles outcome_timer) */
                game_update(&app.game, &app.input, dt);

                /* Check if game logic requests menu transition */
                if (app.game.menu_requested) {
                    app.game.menu_requested = 0;
                    app_show_menu(&app);
                }

                /* Continue rendering */
                app_clear_screen(&app, 0);
                render_frame(&app.game);
                render_present(&app.game);
                break;

            case STATE_ENDING:
                /* Handled by app_ending_sequence() */
                break;
        }

        /* Audio update */
        app_update_audio(&app);

        /* Frame end */
        app_frame_end(&app);
    }
#endif

    /* Shutdown */
    app_shutdown(&app);

    return 0;
}
