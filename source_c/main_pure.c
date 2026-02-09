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

/* Memory and screen operations */
extern void sseMemset32(uint32_t* dst, uint32_t value, int count);
extern uint32_t* ScreenOff;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

/* Global app for Emscripten callback */
static App* g_app = NULL;

static void emscripten_frame(void) {
    if (!g_app || !app_running(g_app)) {
        emscripten_cancel_main_loop();
        return;
    }

    /* Frame timing */
    float dt = app_frame_begin(g_app);

    /* Input */
    app_poll_input(g_app);

    /* State-based update - mirrors native loop */
    switch (g_app->game.state) {

        case STATE_MENU:
            /* Update menu (non-blocking) */
            if (menu_update(&g_app->game, &g_app->input)) {
                /* Menu complete - handle result */
                app_handle_menu_result(g_app, g_app->game.menu_result);
                /* State may have changed - don't render menu */
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

            /* Check for death */
            if (g_app->game.player.health <= 0) {
                g_app->game.state = STATE_DEFEAT;
                g_app->game.outcome_timer = OUTCOME_DISPLAY_FRAMES;
            }

            /* Render game */
            sseMemset32(ScreenOff, 0, 1280 * 800);
            render_frame(&g_app->game);
            render_present(&g_app->game);
            break;

        case STATE_VICTORY:
            /* Handle victory on first frame */
            if (g_app->game.outcome_timer == 0) {
                g_app->game.outcome_timer = OUTCOME_DISPLAY_FRAMES;

                /* Level progression */
                if (g_app->game.current_level_idx >= game_level) {
                    game_level++;
                    g_app->game.unlocked_level = game_level;
                    g_app->game.weapons_level++;

                    /* Save progress to localStorage */
                    EM_ASM({
                        localStorage.setItem('alan_parsons_level', $0);
                        localStorage.setItem('alan_parsons_weapons', $1);
                    }, game_level, g_app->game.weapons_level);
                }
            }

            /* Countdown timer */
            g_app->game.outcome_timer--;
            if (g_app->game.outcome_timer <= 0) {
                /* Ending sequence after Mordor */
                if (g_app->game.current_level_idx >= 5) {
                    app_ending_sequence(g_app);
                }
                app_show_menu(g_app);
            }

            /* Continue rendering */
            sseMemset32(ScreenOff, 0, 1280 * 800);
            render_frame(&g_app->game);
            render_present(&g_app->game);
            break;

        case STATE_DEFEAT:
            /* Countdown timer */
            if (g_app->game.outcome_timer == 0) {
                g_app->game.outcome_timer = OUTCOME_DISPLAY_FRAMES;
            }
            g_app->game.outcome_timer--;
            if (g_app->game.outcome_timer <= 0) {
                app_show_menu(g_app);
            }

            /* Continue rendering */
            sseMemset32(ScreenOff, 0, 1280 * 800);
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
                /* Update menu (non-blocking) */
                if (menu_update(&app.game, &app.input)) {
                    /* Menu complete - handle result */
                    app_handle_menu_result(&app, app.game.menu_result);
                    /* State may have changed - don't render menu */
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

                /* Check for death */
                if (app.game.player.health <= 0) {
                    app.game.state = STATE_DEFEAT;
                    app.game.outcome_timer = OUTCOME_DISPLAY_FRAMES;
                }

                /* Render game */
                sseMemset32(ScreenOff, 0, 1280 * 800);
                render_frame(&app.game);
                render_present(&app.game);
                break;

            case STATE_VICTORY:
                /* Handle victory on first frame */
                if (app.game.outcome_timer == 0) {
                    app.game.outcome_timer = OUTCOME_DISPLAY_FRAMES;

                    /* Level progression */
                    if (app.game.current_level_idx >= game_level) {
                        game_level++;
                        app.game.unlocked_level = game_level;
                        app.game.weapons_level++;

                        /* Save progress */
                        FILE* fp = fopen("level.dat", "wb");
                        if (fp) {
                            fwrite(&game_level, sizeof(int), 1, fp);
                            fwrite(&app.game.weapons_level, sizeof(int), 1, fp);
                            fclose(fp);
                        }
                    }
                }

                /* Keep game running - player can fly around during victory */
                game_update(&app.game, &app.input, dt);

                /* Countdown timer */
                app.game.outcome_timer--;
                if (app.game.outcome_timer <= 0) {
                    /* Ending sequence after Mordor */
                    if (app.game.current_level_idx >= 5) {
                        app_ending_sequence(&app);
                    }
                    app_show_menu(&app);
                }

                /* Continue rendering */
                sseMemset32(ScreenOff, 0, 1280 * 800);
                render_frame(&app.game);
                render_present(&app.game);
                break;

            case STATE_DEFEAT:
                /* Countdown timer */
                if (app.game.outcome_timer == 0) {
                    app.game.outcome_timer = OUTCOME_DISPLAY_FRAMES;
                }

                /* Keep game running - particles continue during defeat */
                game_update(&app.game, &app.input, dt);

                app.game.outcome_timer--;
                if (app.game.outcome_timer <= 0) {
                    app_show_menu(&app);
                }

                /* Continue rendering */
                sseMemset32(ScreenOff, 0, 1280 * 800);
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
