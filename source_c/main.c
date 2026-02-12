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
extern void sseMemcpy32(uint32_t* dst, const uint32_t* src, int count);

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
                    app_ending_sequence(g_app);
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
            /* Handled by app_ending_sequence() */
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
