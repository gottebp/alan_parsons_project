/*
 * main_new.c - The Clean Main Loop
 *
 * This is the vision of what main.c becomes when the refactoring is complete.
 * Pure orchestration: init, loop, shutdown.
 *
 * COMPILE FLAG: This file is not part of the build yet.
 * It exists as a reference implementation and goal.
 *
 * When the refactoring is complete:
 *   - Rename this to main.c
 *   - Remove the old main.c
 *   - Build with -DNEW_MAIN
 */

#include "../include_c/game/game.h"
#include "../include_c/game/audio.h"
#include "../include_c/platform/render.h"
#include "../include_c/platform/platform.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/*============================================================================
 * APPLICATION STATE
 *
 * Everything the application needs in one place.
 * No globals - state is passed explicitly.
 *============================================================================*/

typedef struct {
    /* Platform */
    SDL_Window* window;
    SDL_Renderer* renderer;
    int running;
    int fullscreen;

    /* Game */
    Game game;

    /* Subsystems */
    RenderContext render;
    AudioContext audio;

    /* Timing */
    uint32_t prev_tick;
    float delta_time;

    /* UI State */
    int menu_active;
    int current_level;

    /* Input */
    InputState input;
    int quit_requested;
} App;

/* Global app instance for Emscripten callback */
static App g_app;

/*============================================================================
 * INPUT HANDLING
 *============================================================================*/

static void update_input(App* app) {
    const Uint8* keyboard = SDL_GetKeyboardState(NULL);

    memset(&app->input, 0, sizeof(InputState));

    app->input.up = keyboard[SDL_SCANCODE_UP];
    app->input.down = keyboard[SDL_SCANCODE_DOWN];
    app->input.left = keyboard[SDL_SCANCODE_LEFT];
    app->input.right = keyboard[SDL_SCANCODE_RIGHT];
    app->input.fire = keyboard[SDL_SCANCODE_X];
    app->input.nuke = keyboard[SDL_SCANCODE_SPACE];
    app->input.strafe_left = keyboard[SDL_SCANCODE_Z];
    app->input.strafe_right = keyboard[SDL_SCANCODE_C];
    app->input.escape = keyboard[SDL_SCANCODE_ESCAPE];

    /* Process SDL events */
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            app->quit_requested = 1;
        }
    }
}

/*============================================================================
 * MAIN LOOP
 *============================================================================*/

static void main_loop_iteration(void* arg) {
    App* app = (App*)arg;

    /* Timing */
    uint32_t current_tick = SDL_GetTicks();
    app->delta_time = (current_tick - app->prev_tick) / 1000.0f;
    app->prev_tick = current_tick;

    /* Cap delta time to prevent spiral of death */
    if (app->delta_time > 0.1f) app->delta_time = 0.1f;

    /* Input */
    update_input(app);

    if (app->quit_requested) {
        app->running = 0;
#ifdef __EMSCRIPTEN__
        emscripten_cancel_main_loop();
#endif
        return;
    }

    /* Menu handling */
    if (app->input.escape && app->game.state == STATE_PLAYING) {
        app->menu_active = 1;
    }

    /* Update game */
    if (!app->menu_active) {
        game_update(&app->game, &app->input, app->delta_time);
    }

    /* Render */
    render_clear(&app->render);

    if (app->game.state != STATE_MENU) {
        render_map(&app->render, &app->game);
        render_player(&app->render, &app->game.player);
        render_enemies(&app->render, &app->game);
        render_health_bar(&app->render, &app->game.player);
        render_minimap(&app->render, &app->game);
        render_nuke_icons(&app->render, app->game.player.nukes_remaining);

        /* Present software buffer to texture */
        SDL_UpdateTexture(app->render.screen_texture, NULL,
                         app->render.screen_buffer,
                         SCREEN_WIDTH * sizeof(uint32_t));
        SDL_RenderCopy(app->renderer, app->render.screen_texture, NULL, NULL);

        /* Hardware particle rendering on top */
        render_particles_hw(&app->render, &app->game);

        /* Overlay screens */
        if (app->game.state == STATE_VICTORY) {
            render_victory_screen(&app->render);
        } else if (app->game.state == STATE_DEFEAT) {
            render_defeat_screen(&app->render);
        }
    }

    SDL_RenderPresent(app->renderer);

    /* Frame rate limiting (native only) */
#ifndef __EMSCRIPTEN__
    uint32_t frame_time = SDL_GetTicks() - current_tick;
    if (frame_time < 16) {
        SDL_Delay(16 - frame_time);
    }
#endif
}

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

static int init_sdl(App* app) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    Uint32 flags = SDL_WINDOW_SHOWN;
    if (app->fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }

    app->window = SDL_CreateWindow(
        "The Alan Parsons Project",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        flags
    );

    if (!app->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }

    app->renderer = SDL_CreateRenderer(app->window, -1,
                                        SDL_RENDERER_ACCELERATED |
                                        SDL_RENDERER_PRESENTVSYNC);
    if (!app->renderer) {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        return -1;
    }

    return 0;
}

static int init_app(App* app, int argc, char** argv) {
    memset(app, 0, sizeof(App));

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--fullscreen") == 0) {
            app->fullscreen = 1;
        } else if (strcmp(argv[i], "--captainplanet") == 0) {
            app->game.captain_planet = 1;
        }
    }

    /* Initialize SDL */
    if (init_sdl(app) < 0) {
        return -1;
    }

    /* Initialize render context */
    if (render_init(&app->render, app->renderer) < 0) {
        return -1;
    }

    if (render_load_assets(&app->render) < 0) {
        fprintf(stderr, "Warning: Some render assets failed to load\n");
    }

    /* Initialize audio */
    if (audio_init(&app->audio) < 0) {
        fprintf(stderr, "Warning: Audio init failed\n");
    } else {
        audio_load_assets(&app->audio);
    }

    /* Initialize game */
    game_init(&app->game);

    /* Load saved progress */
    /* platform_load_progress(app, &app->game); */

    app->running = 1;
    app->prev_tick = SDL_GetTicks();

    SDL_ShowCursor(SDL_DISABLE);

    return 0;
}

static void shutdown_app(App* app) {
    /* Save progress */
    /* platform_save_progress(app, &app->game); */

    audio_shutdown(&app->audio);
    render_shutdown(&app->render);

    if (app->renderer) SDL_DestroyRenderer(app->renderer);
    if (app->window) SDL_DestroyWindow(app->window);

    SDL_Quit();
}

/*============================================================================
 * MAIN ENTRY POINT
 *============================================================================*/

int main(int argc, char** argv) {
    App* app = &g_app;

    printf("The Alan Parsons Project - Clean Architecture Version\n");

    if (init_app(app, argc, argv) < 0) {
        fprintf(stderr, "Initialization failed\n");
        return 1;
    }

    /* Start first level for testing */
    game_start_level(&app->game, 0);

    /* Load map */
    render_load_map(&app->render, "./data/shire.bmp");

    /* Play level music */
    audio_play_level_music(&app->audio, 0);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(main_loop_iteration, app, 0, 1);
#else
    while (app->running) {
        main_loop_iteration(app);
    }
#endif

    shutdown_app(app);

    printf("Game exited cleanly\n");
    return 0;
}
