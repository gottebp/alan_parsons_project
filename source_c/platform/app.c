/*
 * app.c - Application Implementation
 *
 * The complete platform layer: initialization, input, audio, timing.
 * This file consolidates what was scattered across main.c, bridge.c, and
 * various legacy modules into one coherent implementation.
 */

#include "../../include_c/platform/app.h"
#include "../../include_c/platform/platform.h"
#include "../../include_c/game/game.h"
#include "../../include_c/game/sprites.h"
#include "../../include_c/game/audio_bridge.h"
#include "../../include_c/core/constants.h"

#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/*============================================================================
 * EXTERNAL REFERENCES
 *============================================================================*/

/* Graphics - from sdl_wrapper.c */
extern int InitGraphics(int fullscreen);
extern void DestroyGraphics(void);
extern uint32_t* ScreenOff;
extern uint32_t ScreenTemp[];
extern SDL_Window* screen_window;
extern SDL_Renderer* screen_renderer;
extern SDL_Texture* screen_texture;
extern void UpdateScreen(void);
extern int LoadBMP(uint32_t* buffer, const char* filename);
extern void AlphaBlit(int x, int y, uint32_t* src, int src_width, int src_height);
extern void MakeAlphaFromRGB(uint32_t pixel);
extern uint32_t intPixel;

/* Memory operations - from sse_mem.c */
extern void sseMemset32(uint32_t* dst, uint32_t value, int count);
extern void sseMemcpy32(uint32_t* dst, const uint32_t* src, int count);

/* Map engine - from mapeng.c */
extern void InitMapEngine(void);
extern void DestroyMapEngine(void);
extern int LoadMap(const char* filename);

/* Input - from input.c */
extern uint8_t KEYBOARD[320];
extern uint16_t MOUSE_X, MOUSE_Y;
extern uint16_t MOUSE_LBUTTON, MOUSE_RBUTTON;
extern int QUIT_SIGNAL;
extern void UpdateInput(void);
extern void FlushKeyboard(void);

#ifdef __EMSCRIPTEN__
extern float mobile_stick_left_x;
extern float mobile_stick_left_y;
extern int mobile_target_angle;
extern int mobile_target_angle_active;
extern int mobile_controls_active;
#endif

/* Menu - from menu.c */
extern void InitMenu(void);
extern void DestroyMenu(void);
extern uint8_t IsMenuRunning;

/* Sprites - from sprites.c */
extern void sprites_init(void);
extern void sprites_destroy(void);

/* Particle engine - from ppe.c */
extern void InitParticleEngine(void);
extern void DestroyParticleEngine(void);

/* Rendering - from game/render.c */
extern void render_frame(const Game* game);
extern void render_present(const Game* game);

/* Trig lookup tables - initialized in app_init */
extern float SIN_LOOK[256];
extern float COS_LOOK[256];

/* Legacy player variables for bridge compatibility */
extern int intPlayerWeaponsLevel;

/* Screen temp buffer for fades */
extern uint32_t ScreenTemp[];

/* Fade functions */
extern void FadeFromBlack(int frames, int speed);
extern void FadeToBlack(int frames, int speed);
extern void FadeFromWhite(int frames, int speed);
extern void FadeToWhite(int frames, int speed);

/*============================================================================
 * CONSTANTS
 *============================================================================*/

static const float DEG_TO_RAD_256 = 0.024543692606f;

/*============================================================================
 * INTERNAL STATE
 *============================================================================*/

/* Global for legacy compatibility - some modules still check this */
int level_is_loaded = 0;
uint8_t game_turnout = 0;  /* 0=playing, 1=dead, 2=win */
int game_level = 0;        /* Unlocked level for menu */

/*============================================================================
 * GLOBAL SYMBOLS FOR LEGACY COMPATIBILITY
 * These were defined in main.c - modules still reference them
 *============================================================================*/

/* Trig lookup tables */
float SIN_LOOK[256];
float COS_LOOK[256];

/* Conversion constants */
float fltRadToDeg256 = 40.7436654315f;

/* Sound counter globals */
int snd_engines_counter = 0;
int snd_weapon_counter = 0;

/* Sound effect pointers for legacy modules */
Mix_Chunk* snd_effect_hit = NULL;
Mix_Chunk* snd_effect_evil_laugh = NULL;
Mix_Chunk* snd_effect_explosion[5] = {NULL, NULL, NULL, NULL, NULL};

/* Visual assets needed by render.c */
uint32_t* victory_screen = NULL;
uint32_t* defeat_screen = NULL;
uint32_t nuke_img[32 * 32];

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

static void init_trig_tables(void) {
    for (int i = 0; i < 256; i++) {
        float angle = (float)i * DEG_TO_RAD_256;
        SIN_LOOK[i] = sinf(angle);
        COS_LOOK[i] = cosf(angle);
    }
}

static int parse_args(App* app, int argc, char** argv) {
    app->fullscreen = 0;
    app->captain_planet = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--fullscreen") == 0) {
            app->fullscreen = 1;
        } else if (strcmp(argv[i], "--captainplanet") == 0) {
            app->captain_planet = 1;
        }
    }

    return 0;
}

static void load_visual_assets(App* app) {
    VisualAssets* v = &app->visuals;

    /* Title screen */
    v->title_screen = (uint32_t*)malloc(480 * 480 * 4);
    if (v->title_screen) {
        LoadBMP(v->title_screen, "./data/title_screen.bmp");
    }

    /* Victory screen - also set global for render.c */
    v->victory_screen = (uint32_t*)malloc(460 * 345 * 4);
    if (v->victory_screen) {
        LoadBMP(v->victory_screen, "./data/victory.bmp");
        for (int i = 0; i < 460 * 345; i++) {
            v->victory_screen[i] = (v->victory_screen[i] & 0x00FFFFFF) | 0xC8000000;
        }
    }
    victory_screen = v->victory_screen;

    /* Defeat screen - also set global for render.c */
    v->defeat_screen = (uint32_t*)malloc(460 * 345 * 4);
    if (v->defeat_screen) {
        LoadBMP(v->defeat_screen, "./data/defeat.bmp");
        for (int i = 0; i < 460 * 345; i++) {
            v->defeat_screen[i] = (v->defeat_screen[i] & 0x00FFFFFF) | 0xC8000000;
        }
    }
    defeat_screen = v->defeat_screen;

    /* Story clips */
    v->story_clips = (uint32_t*)malloc(640 * 480 * 4 * 4);
    if (v->story_clips) {
        LoadBMP(v->story_clips, "./data/story_clips.bmp");
    }

    /* Nuke icon - load into both local and global */
    if (LoadBMP(nuke_img, "./data/nuke.bmp") == 0) {
        for (int i = 0; i < 32 * 32; i++) {
            if (nuke_img[i] != 0xFF000000) {
                nuke_img[i] = (nuke_img[i] & 0x00FFFFFF) | 0xB8000000;
            } else {
                MakeAlphaFromRGB(nuke_img[i]);
                nuke_img[i] = intPixel;
            }
        }
    }
    memcpy(v->nuke_icon, nuke_img, sizeof(nuke_img));
}

static void load_audio_assets(App* app) {
    AudioAssets* a = &app->audio;

    /* Initialize mixer */
    if (Mix_Init(MIX_INIT_OGG) == 0) {
        fprintf(stderr, "Mix_Init failed: %s\n", Mix_GetError());
    }

    /* Sound effects */
    a->snd_engines = Mix_LoadWAV("./sound/engines.wav");
    a->snd_weapon = Mix_LoadWAV("./sound/weapon.wav");
    a->snd_hit = Mix_LoadWAV("./sound/hit.wav");
    a->snd_impact = Mix_LoadWAV("./sound/impact.wav");
    a->snd_evil_laugh = Mix_LoadWAV("./sound/evil_laugh.wav");

    if (a->snd_hit) Mix_VolumeChunk(a->snd_hit, 10);
    if (a->snd_weapon) Mix_VolumeChunk(a->snd_weapon, 80);

    /* Explosion sounds */
    for (int i = 0; i < 5; i++) {
        char path[64];
        snprintf(path, sizeof(path), "./sound/explosion%d.wav", i + 1);
        a->snd_explosion[i] = Mix_LoadWAV(path);
        if (a->snd_explosion[i]) {
            Mix_VolumeChunk(a->snd_explosion[i], 128);
        }
    }

    /* Set audio context for audio_bridge (replaces global pointers) */
    audio_bridge_set_context(a->snd_hit, a->snd_evil_laugh, a->snd_explosion,
                             a->snd_engines, a->snd_weapon);

    /* Legacy global pointers (kept for now, can be removed later) */
    snd_effect_hit = a->snd_hit;
    snd_effect_evil_laugh = a->snd_evil_laugh;
    for (int i = 0; i < 5; i++) {
        snd_effect_explosion[i] = a->snd_explosion[i];
    }

    /* Music - loaded on demand */
    a->music_menu = NULL;
    a->music_ending = NULL;
    for (int i = 0; i < 6; i++) {
        a->music_level[i] = NULL;
    }
    a->music_current = NULL;

    a->engines_playing = 0;
    a->weapon_cooldown = 0;
}

/*
 * Save progress - overrides weak stub in game.c
 * Called from game_check_outcome when player beats a frontier level
 */
void game_save_progress(const Game* game) {
    /* Sync global game_level from game struct */
    game_level = game->unlocked_level;

#ifdef __EMSCRIPTEN__
    EM_ASM({
        console.log('[SAVE] level=' + $0 + ' weapons=' + $1);
        localStorage.setItem('alan_parsons_level', $0);
        localStorage.setItem('alan_parsons_weapons', $1);
    }, game->unlocked_level, game->weapons_level);
#else
    {
        FILE* fp = fopen("level.dat", "wb");
        if (fp) {
            int level = game->unlocked_level;
            int weapons = game->weapons_level;
            fwrite(&level, sizeof(int), 1, fp);
            fwrite(&weapons, sizeof(int), 1, fp);
            fclose(fp);
        }
    }
#endif
}

static void load_progress(App* app) {
    (void)app;  /* Used in Emscripten build */
#ifdef __EMSCRIPTEN__
    game_level = EM_ASM_INT({
        var saved = localStorage.getItem('alan_parsons_level');
        var val = saved ? parseInt(saved) : 0;
        console.log('[LOAD] alan_parsons_level raw=' + saved + ' parsed=' + val);
        return val;
    });
    intPlayerWeaponsLevel = EM_ASM_INT({
        var saved = localStorage.getItem('alan_parsons_weapons');
        var val = saved ? parseInt(saved) : 0;
        console.log('[LOAD] alan_parsons_weapons raw=' + saved + ' parsed=' + val);
        return val;
    });
    int cp_mode = EM_ASM_INT({
        return localStorage.getItem('alan_parsons_captain_planet') === '1' ? 1 : 0;
    });
    if (cp_mode) app->captain_planet = 1;
#else
    FILE* fp = fopen("level.dat", "rb");
    if (fp) {
        fread(&game_level, sizeof(int), 1, fp);
        fread(&intPlayerWeaponsLevel, sizeof(int), 1, fp);
        fclose(fp);
    }
#endif
}

int app_init(App* app, int argc, char** argv) {
    memset(app, 0, sizeof(App));

    /* Parse command line */
    parse_args(app, argc, argv);

    /* Initialize graphics */
    if (InitGraphics(app->fullscreen) < 0) {
        fprintf(stderr, "Failed to initialize graphics\n");
        return -1;
    }

    /* Capture rendering resources into RenderContext
     * (globals still work for backward compatibility) */
    app->render.screen = ScreenOff;
    app->render.temp = ScreenTemp;
    app->render.window = screen_window;
    app->render.renderer = screen_renderer;
    app->render.texture = screen_texture;
    app->render.width = SCREEN_WIDTH;
    app->render.height = SCREEN_HEIGHT;

    /* Initialize render module with context */
    extern void render_set_context(uint32_t* screen, uint32_t* temp, void* renderer, void* texture, int width, int height);
    render_set_context(app->render.screen, app->render.temp, app->render.renderer, app->render.texture, app->render.width, app->render.height);

    /* Initialize menu module with context */
    menu_set_render_context(app->render.screen);

    /* Initialize map engine */
    InitMapEngine();

    /* Initialize trig tables */
    init_trig_tables();

    /* Initialize subsystems */
    InitMenu();
    sprites_init();
    InitParticleEngine();

    /* Load assets */
    load_visual_assets(app);
    load_audio_assets(app);
    load_progress(app);

    /* Initialize game */
    game_init(&app->game);
    app->game.unlocked_level = game_level;
    app->game.weapons_level = intPlayerWeaponsLevel;
    if (app->captain_planet) {
        app->game.captain_planet = 1;
    }

    /* Initialize audio bridge for game sounds */
    audio_bridge_init(&app->audio_state);

    /* Initial state */
    app->running = 1;
    app->initialized = 1;
    app->prev_tick = SDL_GetTicks();

    printf("App initialized - level=%d, weapons=%d\n", game_level, intPlayerWeaponsLevel);

    return 0;
}

/*============================================================================
 * SHUTDOWN
 *============================================================================*/

void app_shutdown(App* app) {
    if (!app->initialized) return;

    /* Stop audio */
    Mix_HaltMusic();
    Mix_HaltChannel(-1);

    /* Free visual assets */
    VisualAssets* v = &app->visuals;
    if (v->title_screen) free(v->title_screen);
    if (v->victory_screen) free(v->victory_screen);
    if (v->defeat_screen) free(v->defeat_screen);
    if (v->story_clips) free(v->story_clips);

    /* Free audio assets */
    AudioAssets* a = &app->audio;
    if (a->snd_engines) Mix_FreeChunk(a->snd_engines);
    if (a->snd_weapon) Mix_FreeChunk(a->snd_weapon);
    if (a->snd_hit) Mix_FreeChunk(a->snd_hit);
    if (a->snd_evil_laugh) Mix_FreeChunk(a->snd_evil_laugh);
    for (int i = 0; i < 5; i++) {
        if (a->snd_explosion[i]) Mix_FreeChunk(a->snd_explosion[i]);
    }
    if (a->music_menu) Mix_FreeMusic(a->music_menu);
    if (a->music_ending) Mix_FreeMusic(a->music_ending);
    for (int i = 0; i < 6; i++) {
        if (a->music_level[i]) Mix_FreeMusic(a->music_level[i]);
    }
    Mix_Quit();

    /* Shutdown subsystems */
    DestroyParticleEngine();
    sprites_destroy();
    DestroyMenu();
    DestroyMapEngine();
    DestroyGraphics();

    app->initialized = 0;
    printf("App shutdown complete\n");
}

/*============================================================================
 * FRAME OPERATIONS
 *============================================================================*/

int app_running(const App* app) {
    return app->running && !QUIT_SIGNAL;
}

float app_frame_begin(App* app) {
    app->curr_tick = SDL_GetTicks();

    /* Calculate delta time */
    float dt = (app->curr_tick > app->prev_tick)
             ? (float)(app->curr_tick - app->prev_tick) / 1000.0f
             : 1.0f / 60.0f;

    /* Clamp to prevent physics explosion on lag spikes */
    if (dt > 0.1f) dt = 0.1f;
    if (dt < 0.001f) dt = 1.0f / 60.0f;

    app->dt = dt;
    return dt;
}

void app_frame_end(App* app) {
    app->frame_count++;
    app->prev_tick = app->curr_tick;

#ifndef __EMSCRIPTEN__
    /* Frame rate limiting for native builds */
    uint32_t frame_time = SDL_GetTicks() - app->curr_tick;
    uint32_t target_time = 1000 / 60;

    if (frame_time < target_time) {
        uint32_t delay = target_time - frame_time;
        if (delay > 1) SDL_Delay(delay - 1);
        while ((SDL_GetTicks() - app->curr_tick) < target_time) {
            /* Spin for precision */
        }
    }
#endif
}

/*============================================================================
 * RENDER OPERATIONS
 *============================================================================*/

void app_clear_screen(App* app, uint32_t color) {
    sseMemset32(app->render.screen, color, app->render.width * app->render.height);
}

void app_present(App* app) {
    (void)app;  /* TODO: Use app->render when UpdateScreen takes context */
    UpdateScreen();
}

/*============================================================================
 * INPUT
 *============================================================================*/

void app_poll_input(App* app) {
    UpdateInput();

    InputState* in = &app->input;
    memset(in, 0, sizeof(InputState));

    /* Keyboard */
    in->up = KEYBOARD[SDL_SCANCODE_UP] || KEYBOARD[SDL_SCANCODE_W];
    in->down = KEYBOARD[SDL_SCANCODE_DOWN] || KEYBOARD[SDL_SCANCODE_S];
    in->left = KEYBOARD[SDL_SCANCODE_LEFT] || KEYBOARD[SDL_SCANCODE_A];
    in->right = KEYBOARD[SDL_SCANCODE_RIGHT] || KEYBOARD[SDL_SCANCODE_D];
    in->fire = KEYBOARD[SDL_SCANCODE_X] || MOUSE_LBUTTON;
    in->nuke = KEYBOARD[SDL_SCANCODE_SPACE] || MOUSE_RBUTTON;
    in->strafe_left = KEYBOARD[SDL_SCANCODE_Z];
    in->strafe_right = KEYBOARD[SDL_SCANCODE_C];
    in->escape = KEYBOARD[SDL_SCANCODE_ESCAPE];

    /* Mouse */
    in->mouse_x = MOUSE_X;
    in->mouse_y = MOUSE_Y;
    in->mouse_left = MOUSE_LBUTTON;
    in->mouse_right = MOUSE_RBUTTON;

#ifdef __EMSCRIPTEN__
    /* Mobile twin-stick controls */
    in->stick_left_x = mobile_stick_left_x;
    in->stick_left_y = mobile_stick_left_y;
    in->target_angle = mobile_target_angle;
    in->target_angle_active = mobile_target_angle_active;
    in->mobile_active = mobile_controls_active;
#endif
}

/*============================================================================
 * LEVEL OPERATIONS
 *============================================================================*/

void app_load_level(App* app, int level_id) {
    /* Menu IDs are 2-7, map to internal 0-5 */
    int level_index = level_id - 2;
    if (level_index < 0 || level_index > 5) return;

    /* Legacy globals for compatibility */
    level_is_loaded = 1;
    game_turnout = 0;

    /* Configure and start level (sets current_level_idx and outcome_timer) */
    app->game.captain_planet = app->captain_planet;
    game_start_level(&app->game, level_index);

    /* Load map */
    const char* map_files[] = {
        "./data/shire.bmp",
        "./data/archipelago.bmp",
        "./data/dune.bmp",
        "./data/midkemia.bmp",
        "./data/oceania.bmp",
        "./data/mordor.bmp"
    };
    LoadMap(map_files[level_index]);

    /* Start level music */
    app_play_level_music(app, level_index);

    /* Setup ending timer for Mordor */
    if (level_index >= 5) {
        app->ending_timer = 250;
    }

}

void app_show_menu(App* app) {
    /* Quick fade out from current screen (skip in main loop - menu has its own fades) */
    if (!app->in_main_loop) {
        sseMemcpy32(ScreenTemp, app->render.screen, SCREEN_WIDTH * SCREEN_HEIGHT);
        FadeToBlack(30, 4);
    }

    /* Transition to non-blocking menu state */
    menu_enter(&app->game);

    /* Start menu music (if not already playing) */
    if (app->audio.music_current != app->audio.music_menu) {
        app_play_menu_music(app);
    }
}

void app_handle_menu_result(App* app, int result) {
    if (result == 0) {
        /* Quit */
        app->running = 0;
    } else if (result >= 2 && result <= 7) {
        if (!app->in_main_loop) {
            /* Fade to black before loading level */
            sseMemcpy32(ScreenTemp, app->render.screen, SCREEN_WIDTH * SCREEN_HEIGHT);
            FadeToBlack(100, 3);
        }

        /* Load level */
        app_load_level(app, result);

        if (!app->in_main_loop) {
            /* Render first frame and fade from black into it */
            sseMemset32(app->render.screen, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
            render_frame(&app->game);
            sseMemcpy32(ScreenTemp, app->render.screen, SCREEN_WIDTH * SCREEN_HEIGHT);
            FadeFromBlack(80, 3);
        }
        /* In main loop: skip rendering here - main loop handles it next frame */
    }
    /* result == 1 means resume, nothing to do */
}

/*============================================================================
 * AUDIO
 *============================================================================*/

void app_play_level_music(App* app, int level_index) {
    AudioAssets* a = &app->audio;

    /* Stop current music immediately (avoid potential ASYNCIFY issues with fade) */
    Mix_HaltMusic();

    /* Load if needed */
    if (!a->music_level[level_index]) {
        const char* music_files[] = {
            "./sound/sound_track1.ogg",
            "./sound/sound_track2.ogg",
            "./sound/sound_track3.ogg",
            "./sound/sound_track4.ogg",
            "./sound/sound_track5.ogg",
            "./sound/sound_track6.ogg"
        };
        a->music_level[level_index] = Mix_LoadMUS(music_files[level_index]);
    }

    /* Play */
    if (a->music_level[level_index]) {
        Mix_VolumeMusic(110);
        Mix_PlayMusic(a->music_level[level_index], -1);
        a->music_current = a->music_level[level_index];
    }
}

void app_play_menu_music(App* app) {
    AudioAssets* a = &app->audio;

    if (!a->music_menu) {
        a->music_menu = Mix_LoadMUS("./sound/menu_theme.ogg");
    }

    if (a->music_menu) {
        Mix_VolumeMusic(110);
        Mix_PlayMusic(a->music_menu, -1);
        a->music_current = a->music_menu;
    }
}

void app_stop_music(App* app) {
    Mix_HaltMusic();
    app->audio.music_current = NULL;
}

void app_update_audio(App* app) {
    /* Update continuous sounds (engines, weapons) based on input and game state */
    audio_bridge_update(&app->audio_state, &app->game, &app->input);

    /* Process game audio events */
    if (app->game.audio_body_collision) {
        app->game.audio_body_collision = 0;
    }
    if (app->game.audio_player_hit) {
        audio_bridge_player_hit(&app->audio_state);
        app->game.audio_player_hit = 0;
    }

    if (app->game.audio_enemy_destroyed) {
        audio_bridge_enemy_destroyed(&app->audio_state);
        app->game.audio_enemy_destroyed = 0;
    }

    if (app->game.audio_nuke_fired) {
        audio_bridge_nuke_dropped(&app->audio_state);
        app->game.audio_nuke_fired = 0;
    }

    if (app->game.audio_boss_spawned) {
        audio_bridge_boss_spawned(&app->audio_state);
        app->game.audio_boss_spawned = 0;
    }
}

/*============================================================================
 * SEQUENCES
 *============================================================================*/

void app_title_screen(App* app) {
    SDL_ShowCursor(SDL_DISABLE);

    /* Start menu music */
    IsMenuRunning = 1;
    app_play_menu_music(app);

    /* Display title screen with fades */
    if (app->visuals.title_screen) {
        sseMemset32(app->render.screen, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
        AlphaBlit(
            SCREEN_WIDTH / 2 - 480 / 2,
            SCREEN_HEIGHT / 2 - 480 / 2,
            app->visuals.title_screen,
            480, 480
        );
        sseMemcpy32(ScreenTemp, app->render.screen, SCREEN_WIDTH * SCREEN_HEIGHT);
        FadeFromBlack(180, 2);
        FadeToBlack(180, 3);
    }

    /* Show story clip (4th clip) */
    if (app->visuals.story_clips) {
        uint32_t* clip = app->visuals.story_clips + (640 * 480 * 3);
        sseMemset32(app->render.screen, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
        AlphaBlit(
            SCREEN_WIDTH / 2 - 640 / 2,
            SCREEN_HEIGHT / 2 - 480 / 2,
            clip, 640, 480
        );
        sseMemcpy32(ScreenTemp, app->render.screen, SCREEN_WIDTH * SCREEN_HEIGHT);
        FadeFromBlack(180, 3);
        FadeToBlack(180, 3);
    }

    /* Wait for key release */
    int done = 0;
    while (!done) {
        UpdateInput();
        if (!KEYBOARD[SDL_SCANCODE_ESCAPE] && !MOUSE_LBUTTON) {
            done = 1;
        }
#ifdef __EMSCRIPTEN__
        emscripten_sleep(16);
#endif
    }

    printf("Title screen complete\n");
}

void app_ending_sequence(App* app) {
    app->in_ending = 1;

    /* Fade out level music */
    Mix_FadeOutMusic(100);

#ifdef __EMSCRIPTEN__
    emscripten_sleep(200);
#else
    SDL_Delay(200);
#endif

    /* Load and play ending music */
    AudioAssets* a = &app->audio;
    if (!a->music_ending) {
        a->music_ending = Mix_LoadMUS("./sound/ending_theme.ogg");
    }
    if (a->music_ending) {
        Mix_VolumeMusic(110);
        Mix_PlayMusic(a->music_ending, 1);
    }

    Mix_HaltChannel(-1);

    if (!app->visuals.story_clips) {
        app->in_ending = 0;
        return;
    }

    /* Show story clips with fades */
    FlushKeyboard();

    /* Clip 0 */
    sseMemcpy32(ScreenTemp, app->render.screen, SCREEN_WIDTH * SCREEN_HEIGHT);
    FlushKeyboard();
    FadeToWhite(500, 2);

    sseMemset32(app->render.screen, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
    sseMemset32(ScreenTemp, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
    AlphaBlit(SCREEN_WIDTH / 2 - 320, SCREEN_HEIGHT / 2 - 240,
              app->visuals.story_clips, 640, 480);
    sseMemcpy32(ScreenTemp, app->render.screen, SCREEN_WIDTH * SCREEN_HEIGHT);
    FlushKeyboard();
    FadeFromWhite(300, 2);

    /* Clip 1 */
    FlushKeyboard();
    FadeToWhite(300, 2);
    AlphaBlit(SCREEN_WIDTH / 2 - 320, SCREEN_HEIGHT / 2 - 240,
              app->visuals.story_clips + (640 * 480), 640, 480);
    sseMemcpy32(ScreenTemp, app->render.screen, SCREEN_WIDTH * SCREEN_HEIGHT);
    FlushKeyboard();
    FadeFromWhite(300, 2);

    /* Clip 2 */
    FlushKeyboard();
    FadeToWhite(300, 2);
    AlphaBlit(SCREEN_WIDTH / 2 - 320, SCREEN_HEIGHT / 2 - 240,
              app->visuals.story_clips + (640 * 480 * 2), 640, 480);
    sseMemcpy32(ScreenTemp, app->render.screen, SCREEN_WIDTH * SCREEN_HEIGHT);
    FlushKeyboard();
    FadeFromWhite(300, 2);

    /* Wait for key press, touch/click, or timeout (15s) */
    FlushKeyboard();
    SDL_Event event;
    int waiting = 1;
    int wait_frames = 0;
    while (waiting) {
#ifdef __EMSCRIPTEN__
        emscripten_sleep(16);
#else
        SDL_Delay(16);
#endif
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                app->in_ending = 0;
                return;
            }
        }
        UpdateInput();
        /* Check keyboard */
        for (int i = 0; i < 128; i++) {
            if (KEYBOARD[i]) {
                waiting = 0;
                break;
            }
        }
        /* Check mouse/touch (handle_touch_cursor sets MOUSE_LBUTTON) */
        if (MOUSE_LBUTTON) {
            waiting = 0;
        }
        /* Safety timeout: 900 frames (~15s) prevents permanent hang */
        if (++wait_frames >= 900) {
            waiting = 0;
        }
    }

    FadeToBlack(120, 3);

    /* Mark game as beaten */
#ifdef __EMSCRIPTEN__
    EM_ASM(
        localStorage.setItem('alan_parsons_beaten', '1');
        console.log('Game beaten! Captain Planet mode unlocked.');
    );
#endif

    app->in_ending = 0;
}

/*============================================================================
 * EMSCRIPTEN RESTART SUPPORT
 *============================================================================*/

#ifdef __EMSCRIPTEN__
/* Global app pointer for RestartGame - set by main_pure.c */
static App* g_restart_app = NULL;

void app_set_restart_pointer(App* app) {
    g_restart_app = app;
}

EMSCRIPTEN_KEEPALIVE
void RestartGame(void) {
    if (!g_restart_app) return;
    App* app = g_restart_app;

    printf("Game exited - waiting for user click to restart...\n");

    EM_ASM(
        window.gameStarted = false;
    );

    while (1) {
        int started = EM_ASM_INT({
            return window.gameStarted ? 1 : 0;
        });
        if (started) {
            printf("Restarting game by user click\n");
            break;
        }
        emscripten_sleep(100);
    }

    /* Reset game state */
    app->running = 1;
    level_is_loaded = 0;

    /* Show title screen with intro (starts music) */
    app_title_screen(app);

    /* Then go to menu */
    app_show_menu(app);
}
#endif
