/*
 * The Alan Parsons Project
 * A particle-based space shooter game
 *
 * Originally written in x86 assembly by Benjamin Gottemoller
 * Converted to C
 *
 * Project: ECE291 Final Project
 * Website: http://www.particlefield.com
 * Date: 7/20/02, Ported to SDL: 8/12/02
 */

#include "defs.h"
#include "sdl_wrapper.h"
#include "input.h"
#include "rand.h"
#include "sse_mem.h"
#include "player.h"
#include "mapeng.h"
#include "ppe.h"
#include "enemy.h"
#include "ai.h"
#include "menu.h"
#include "game/bridge_opaque.h"  /* New Game struct bridge - for incremental migration */
#include "game/sprites.h"        /* Consolidated sprite loading */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/*============================================================================
 * APP CONTEXT
 * Platform-level concerns wrapped in a single struct.
 * Game logic lives in the Game struct; App owns the Game.
 *============================================================================*/
typedef struct {
    Game* game;             /* The game state */
    int running;            /* 1 = running, 0 = should exit */
    uint32_t prev_tick;     /* Previous frame tick */
    uint32_t curr_tick;     /* Current frame tick */
    int menu_result;        /* Result from last menu */

    /* Level state (still being migrated to Game) */
    int active_level;
    int nukes_remaining;
    int nuke_wait_counter;
    int victory_defeat_timer;
    int ending_delay_timer;
    int is_ending;
} App;

static App app = {0};

/* Game state globals - exported for legacy modules */
int level_is_loaded = 0;  /* 0 = no level (menu only), 1 = level loaded */
uint8_t game_turnout = PLAYER_NORMAL;  /* Exported for player module */
int game_level = 0;  /* Exported for menu module */
static int num_starting_nukes = NUM_NUKES;

/* Lookup tables */
float SIN_LOOK[256];
float COS_LOOK[256];

/* External variables from player.c and mapeng.c for bridge compatibility */
extern float fltPlayerAngularVel;
extern float fltCameraX, fltCameraY;
extern int intCameraX, intCameraY;

/* File paths for data files */
char* SmallParticleFile = "./data/small_flares.bmp";
char* LargeParticleFile = "./data/large_flares.bmp";
char* SmallEnemyFile = "./data/small_enemies.bmp";
char* LargeEnemyFile = "./data/large_enemies.bmp";

/* Constants - mirrors assembly main.asm lines 140-143 */
float fltDegToRad360 = 0.01745329252f;
float fltRadToDeg360 = 57.2957795131f;
float fltDegToRad256 = 0.024543692606f;
float fltRadToDeg256 = 40.7436654315f;

/* Map positions */
static int MapMidX = (MAP_WIDTH / 2) - (SCREEN_WIDTH / 2);
static int MapMidY = (MAP_HEIGHT / 2) - (SCREEN_HEIGHT / 2);

/* Image buffers */
static uint32_t* title_screen = NULL;
uint32_t* victory_screen = NULL;  /* Exported for render_bridge */
uint32_t* defeat_screen = NULL;   /* Exported for render_bridge */
static uint32_t* story_clips = NULL;
uint32_t nuke_img[32 * 32];       /* Exported for render_bridge */

/* Sound effects */
static Mix_Chunk* snd_effect_engines = NULL;
Mix_Chunk* snd_effect_evil_laugh = NULL;  /* Exported for enemy module */
Mix_Chunk* snd_effect_hit = NULL;  /* Used by player.c */
static Mix_Chunk* snd_effect_weapon = NULL;
Mix_Chunk* snd_effect_explosion[5] = {NULL, NULL, NULL, NULL, NULL};  /* Exported for ai.c */
static Mix_Music* snd_track = NULL;

/* Sound effect counters - used by player.c and menu.c */
int snd_engines_counter = 0;
int snd_weapon_counter = 0;

/* Game ending state now in App struct */

/* Forward declarations */
void initialize_game_data(void);
void destroy_game_data(void);
void run_game(void);
void load_level(int level_id);
void display_title_screen(void);
void init_trig_tables(void);
void run_ending_sequence(void);
void music_track_finished_callback(void);
#ifdef __EMSCRIPTEN__
void RestartGame(void);
#endif

/* ResetGameProgress removed - now handled in JavaScript for simplicity */

/*
 * Main entry point
 * Mirrors main.asm lines 208-294
 */
int main(int argc, char* argv[]) {
    int fullscreen = 0;

    /* Parse command line arguments - matches assembly logic exactly */
    if (argc > 3) {
        printf("\nUsage: app --fullscreen or app\n");
        return 1;
    }

    if (argc > 1) {
        /* Check first argument for --captainplanet */
        if (strcmp(argv[1], "--captainplanet") == 0) {
            num_starting_nukes = 18;

            /* If only captainplanet arg, skip to no commands */
            if (argc == 2) {
                goto no_commands;
            }
        }

        /* Check second argument for --captainplanet if argc == 3 */
        if (argc == 3) {
            if (strcmp(argv[2], "--captainplanet") == 0) {
                num_starting_nukes = 18;
            }
        }

        /* Check first argument for --fullscreen */
        if (strcmp(argv[1], "--fullscreen") == 0) {
            fullscreen = 1;
        } else {
            /* If nukes were set and argc == 3, check second arg for fullscreen */
            if (num_starting_nukes == 18 && argc == 3) {
                if (strcmp(argv[2], "--fullscreen") == 0) {
                    fullscreen = 1;
                } else {
                    printf("\nUsage: app --fullscreen or app\n");
                    return 1;
                }
            } else {
                printf("\nUsage: app --fullscreen or app\n");
                return 1;
            }
        }
    }

no_commands:

    /* Initialize graphics */
    if (InitGraphics(fullscreen) < 0) {
        fprintf(stderr, "Failed to initialize graphics\n");
        return 1;
    }

    /* Initialize map engine */
    InitMapEngine();

    /* Initialize game data */
    initialize_game_data();

#ifdef __EMSCRIPTEN__
    /* EMSCRIPTEN: Wait for user to click canvas before starting (enables audio) */
    printf("Waiting for user to click canvas to start game...\n");
    while (1) {
        int started = EM_ASM_INT({
            return window.gameStarted ? 1 : 0;
        });
        if (started) {
            printf("Game started by user click\n");
            break;
        }
        emscripten_sleep(100);  /* Check every 100ms */
    }
#endif

    /* Display title screen */
    display_title_screen();

    /* Run main game loop */
    run_game();

    /* Clean up */
    destroy_game_data();
    DestroyMapEngine();
    DestroyGraphics();

    return 0;
}

/*
 * Initialize game data
 */
void initialize_game_data(void) {
    /* Seed random number generator */
    SeedRand(0xAE33);
    Rand(); /* Consume one value like original */

    /* Initialize game state - mirrors assembly lines 471-481 */
    level_is_loaded = 0;

    /* Load saved game data - mirrors assembly lines 474-481 */
    /* EMSCRIPTEN NOTE: Uses localStorage for reliable browser persistence */
#ifdef __EMSCRIPTEN__
    /* Load from localStorage (synchronous and reliable) */
    game_level = EM_ASM_INT({
        var saved = localStorage.getItem('alan_parsons_level');
        if (saved !== null) {
            console.log('Loaded game level from localStorage:', saved);
            return parseInt(saved);
        }
        return 0;  /* Default if no save exists */
    });
    intPlayerWeaponsLevel = EM_ASM_INT({
        var saved = localStorage.getItem('alan_parsons_weapons');
        if (saved !== null) {
            console.log('Loaded weapons level from localStorage:', saved);
            return parseInt(saved);
        }
        return 0;  /* Default if no save exists */
    });

    /* Check if Captain Planet mode is enabled */
    int captain_planet_mode = EM_ASM_INT({
        var cpMode = localStorage.getItem('alan_parsons_captain_planet');
        if (cpMode === '1') {
            console.log('Captain Planet mode enabled - 18 nukes!');
            return 1;
        }
        return 0;
    });
    if (captain_planet_mode) {
        num_starting_nukes = 18;
    }
#else
    /* Native build uses file-based saves */
    FILE* fp = fopen("level.dat", "rb");
    if (fp) {
        fread(&game_level, sizeof(int), 1, fp);
        fread(&intPlayerWeaponsLevel, sizeof(int), 1, fp);
        fclose(fp);
    }
#endif

    /* Initialize subsystems */
    InitMenu();
    sprites_init();        /* Load all sprites (player, enemies) */
    InitPlayer();          /* Legacy - now no-op, kept for compatibility */
    InitParticleEngine();
    LoadEnemyData();       /* Legacy - now no-op, kept for compatibility */

    /* Set initial player position */
    fltPlayerX = (float)MapMidX;
    fltPlayerY = (float)MapMidY;
    intPlayerX = MapMidX;
    intPlayerY = MapMidY;
    intbPlayerAngle = PLAYER_START_ANGLE;
    fltPlayerSpeed = 0.0f;
    fltPlayerStrafeSpeed = 0.0f;
    fltPlayerAngularVel = 0.0f;
    intPlayerHealth = MAXPLAYERHEALTH;

    /* Initialize camera to player position */
    fltCameraX = fltPlayerX;
    fltCameraY = fltPlayerY;
    intCameraX = intPlayerX;
    intCameraY = intPlayerY;

    /* Initialize trig lookup tables */
    init_trig_tables();

    /* Initialize SDL_mixer for OGG support */
    if (Mix_Init(MIX_INIT_OGG) == 0) {
        fprintf(stderr, "Mix_Init failed: %s\n", Mix_GetError());
    }

    /* Load nuke image - mirrors assembly lines 490-513 */
    if (LoadBMP(nuke_img, "./data/nuke.bmp") == 0) {
        /* Set up alpha transparency for nuke image */
        for (int i = 0; i < 32 * 32; i++) {
            if (nuke_img[i] != 0xFF000000) {
                /* Non-black pixels get alpha 0xB8 */
                nuke_img[i] = (nuke_img[i] & 0x00FFFFFF) | 0xB8000000;
            } else {
                /* Black pixels need special blending via MakeAlphaFromRGB */
                MakeAlphaFromRGB(nuke_img[i]);  /* Result stored in global intPixel */
                nuke_img[i] = intPixel;
            }
        }
    }

    /* Allocate and load story clips (4 frames of 640x480) */
    story_clips = (uint32_t*)malloc(640 * 480 * 4 * 4);
    if (story_clips) {
        LoadBMP(story_clips, "./data/story_clips.bmp");
    }

    /* Allocate and load title screen */
    title_screen = (uint32_t*)malloc(480 * 480 * 4);
    if (title_screen) {
        LoadBMP(title_screen, "./data/title_screen.bmp");
    }

    /* Allocate and load victory screen */
    victory_screen = (uint32_t*)malloc(460 * 345 * 4);
    if (victory_screen) {
        LoadBMP(victory_screen, "./data/victory.bmp");
        /* Set alpha to 200 for all pixels */
        for (int i = 0; i < 460 * 345; i++) {
            victory_screen[i] = (victory_screen[i] & 0x00FFFFFF) | 0xC8000000;
        }
    }

    /* Allocate and load defeat screen */
    defeat_screen = (uint32_t*)malloc(460 * 345 * 4);
    if (defeat_screen) {
        LoadBMP(defeat_screen, "./data/defeat.bmp");
        /* Set alpha to 200 for all pixels */
        for (int i = 0; i < 460 * 345; i++) {
            defeat_screen[i] = (defeat_screen[i] & 0x00FFFFFF) | 0xC8000000;
        }
    }

    /* Load sound effects */
    snd_effect_engines = Mix_LoadWAV("./sound/engines.wav");
    snd_effect_evil_laugh = Mix_LoadWAV("./sound/evil_laugh.wav");
    snd_effect_hit = Mix_LoadWAV("./sound/hit.wav");
    snd_effect_weapon = Mix_LoadWAV("./sound/weapon.wav");

    if (snd_effect_hit) Mix_VolumeChunk(snd_effect_hit, 10);
    if (snd_effect_weapon) Mix_VolumeChunk(snd_effect_weapon, 80);

    /* Load explosion sound effects */
    snd_effect_explosion[0] = Mix_LoadWAV("./sound/explosion1.wav");
    snd_effect_explosion[1] = Mix_LoadWAV("./sound/explosion2.wav");
    snd_effect_explosion[2] = Mix_LoadWAV("./sound/explosion3.wav");
    snd_effect_explosion[3] = Mix_LoadWAV("./sound/explosion4.wav");
    snd_effect_explosion[4] = Mix_LoadWAV("./sound/explosion5.wav");

    for (int i = 0; i < 5; i++) {
        if (snd_effect_explosion[i]) {
            Mix_VolumeChunk(snd_effect_explosion[i], 128);
        }
    }

    /* Set up music callback */
    Mix_HookMusicFinished(music_track_finished_callback);

    /* Initialize remaining state */
    /* NOTE: game_level is NOT reset here - it was loaded from file above (or stayed at 0) */
    app.active_level = 0;
    app.nukes_remaining = num_starting_nukes;

    /* Create new Game struct for incremental migration */
    app.game = bridge_create_game();
    if (app.game) {
        bridge_seed_random(app.game, 0xAE33);
        bridge_request_menu(app.game);  /* Start with menu */
    }

    printf("Game initialized - game_level=%d, weapons=%d\n", game_level, intPlayerWeaponsLevel);
}

/*
 * Destroy game data
 */
void destroy_game_data(void) {
    /* Stop all audio */
    Mix_HaltMusic();
    Mix_HaltChannel(-1);

    /* Free allocated resources */
    DestroyParticleEngine();
    DestroyPlayer();       /* Legacy - now no-op */
    DestroyMenu();
    DestroyEnemyData();    /* Legacy - now no-op */
    sprites_destroy();     /* Free all sprites */

    /* Free image buffers */
    if (story_clips) free(story_clips);
    if (title_screen) free(title_screen);
    if (victory_screen) free(victory_screen);
    if (defeat_screen) free(defeat_screen);

    /* Free sound effects */
    if (snd_effect_engines) Mix_FreeChunk(snd_effect_engines);
    if (snd_effect_evil_laugh) Mix_FreeChunk(snd_effect_evil_laugh);
    if (snd_effect_hit) Mix_FreeChunk(snd_effect_hit);
    if (snd_effect_weapon) Mix_FreeChunk(snd_effect_weapon);

    for (int i = 0; i < 5; i++) {
        if (snd_effect_explosion[i]) {
            Mix_FreeChunk(snd_effect_explosion[i]);
        }
    }

    if (snd_track) Mix_FreeMusic(snd_track);

    /* Cleanup SDL_mixer */
    Mix_Quit();

    /* Destroy new Game struct */
    if (app.game) {
        bridge_destroy_game(app.game);
        app.game = NULL;
    }

    printf("Game data destroyed\n");
}

/*
 * Initialize trigonometric lookup tables
 */
void init_trig_tables(void) {
    for (int i = 0; i < 256; i++) {
        float angle = (float)i * fltDegToRad256;
        SIN_LOOK[i] = sinf(angle);
        COS_LOOK[i] = cosf(angle);
    }
}

/* Game loop state now in App struct */

/*
 * Single iteration of game loop - called by Emscripten or native loop
 */
void game_loop_iteration(void) {
    /* Early exit if not running */
    if (!app.running) {
        return;
    }

        /* Capture frame start time */
        app.curr_tick = SDL_GetTicks();

        /* Update input */
        UpdateInput();

        /* Check for victory/defeat auto-advance timer (7 seconds at 60 FPS = 420 frames) */
        if (game_turnout == PLAYER_WIN || game_turnout == PLAYER_DEAD) {
            /* Start timer if not already started */
            if (app.victory_defeat_timer == 0) {
                app.victory_defeat_timer = 420;  /* 7 seconds at 60 FPS */
            }

            /* Decrement timer */
            if (app.victory_defeat_timer > 0) {
                app.victory_defeat_timer--;

                /* Auto-activate menu after 7 seconds */
                if (app.victory_defeat_timer == 0 && app.game) {
                    bridge_request_menu(app.game);
                }
            }
        }

        /* Check for escape key - activate menu (also advances victory/defeat screen) */
        if (KEYBOARD[SDL_SCANCODE_ESCAPE] && app.game) {
            bridge_request_menu(app.game);
        }

        /* Check if menu should be activated */
        if (app.game && bridge_menu_requested(app.game)) {
            bridge_clear_menu_request(app.game);
            app.victory_defeat_timer = 0;  /* Reset timer */

            app.menu_result = RunMenu(level_is_loaded);
            bridge_set_menu_result(app.game, app.menu_result);

            /* Check if user selected a level (2-7) */
            if (app.menu_result >= 2 && app.menu_result <= 7) {
                load_level(app.menu_result);
            }
        }

        /* Check if user quit from menu */
        if (app.menu_result == 0) {
            app.running = 0;
#ifdef __EMSCRIPTEN__
            /* Stop all audio completely */
            Mix_HaltMusic();
            Mix_HaltChannel(-1);  /* Stop all sound channels */

            /* Free the music track to ensure it's really stopped */
            extern Mix_Music* snd_track;
            if (snd_track) {
                Mix_FreeMusic(snd_track);
                snd_track = NULL;
            }

            /* Cancel current game loop */
            emscripten_cancel_main_loop();

            /* Clear screen to black */
            sseMemset32(ScreenOff, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
            UpdateScreen();

            /* Reset page to initial state - show start overlay and cursor */
            EM_ASM(
                console.log('Game exited - audio stopped, showing start overlay');
                window.gameStarted = false;
                var overlay = document.getElementById('start-overlay');
                if (overlay) {
                    overlay.classList.remove('hidden');
                    /* Update text for restart */
                    var h2 = overlay.querySelector('h2');
                    if (h2) h2.textContent = '🎮 Click to Play Again';
                    var p = overlay.querySelector('p');
                    if (p) p.textContent = 'Click anywhere to restart';
                }
                var canvas = document.getElementById('canvas');
                if (canvas) {
                    canvas.classList.remove('game-active');  /* Show cursor again */
                    canvas.blur();  /* Remove focus so F12 works */
                }
                /* Hide status/progress container */
                var status = document.getElementById('status-container');
                if (status) status.classList.add('hidden');
            );

            /* Wait for restart (don't call display_title_screen yet to keep music off) */
            RestartGame();
#endif
            return;
        }

        /* Check for quit signal */
        if (QUIT_SIGNAL) {
            app.running = 0;
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#endif
            return;
        }

        /* Only run game logic if a level is loaded */
        if (level_is_loaded == 0) {
            /* No level loaded - wait for menu input */
            /* EMSCRIPTEN NOTE: SDL_Delay handled by emscripten_set_main_loop FPS parameter */
#ifndef __EMSCRIPTEN__
            SDL_Delay(16); /* ~60 FPS */
#endif
            return;
        }

        /* Decrement nuke wait counter - mirrors assembly line 357-360 */
        if (app.nuke_wait_counter > 0) {
            app.nuke_wait_counter--;
        }

        /*====================================================================
         * GAME UPDATE
         * Uses unified Game struct and game_update() for all logic.
         * No bridge sync needed - rendering reads directly from Game.
         *====================================================================*/
        if (app.game) {
            /* Calculate actual dt from frame time */
            float dt = (app.curr_tick > app.prev_tick)
                     ? (float)(app.curr_tick - app.prev_tick) / 1000.0f
                     : 1.0f / 60.0f;

            /* Clamp dt to prevent physics explosion on lag spikes */
            if (dt > 0.1f) dt = 0.1f;  /* Max 100ms = 10 FPS minimum */
            if (dt < 0.001f) dt = 1.0f / 60.0f;  /* Min 1ms */

            /* Update game with input from KEYBOARD globals */
            bridge_update_from_legacy_input(app.game, dt);

            /* Check outcome */
            int outcome = bridge_get_outcome(app.game);
            if (outcome == 1) {
                game_turnout = PLAYER_DEAD;
            } else if (outcome == 2) {
                game_turnout = PLAYER_WIN;
            }
        }

        /* Handle victory - advance level and save progress */
        if (game_turnout == PLAYER_WIN) {
            if (app.active_level >= game_level) {
                game_level++;
                bridge_increment_weapons_level(app.game);
                intPlayerWeaponsLevel = bridge_get_weapons_level(app.game);

                /* Save progress */
#ifdef __EMSCRIPTEN__
                EM_ASM({
                    localStorage.setItem('alan_parsons_level', $0);
                    localStorage.setItem('alan_parsons_weapons', $1);
                }, game_level, intPlayerWeaponsLevel);
#else
                FILE* fp = fopen("level.dat", "wb");
                if (fp) {
                    fwrite(&game_level, sizeof(int), 1, fp);
                    fwrite(&intPlayerWeaponsLevel, sizeof(int), 1, fp);
                    fclose(fp);
                }
#endif
            }

            /* Run ending sequence after beating Mordor */
            if (app.active_level >= 5) {
                app.ending_delay_timer--;
                if (app.ending_delay_timer <= 0) {
                    run_ending_sequence();
                    if (app.game) bridge_request_menu(app.game);
                    game_turnout = PLAYER_NORMAL;
                    app.ending_delay_timer = 300;
                }
            }
        }

        /* Render scene */
        sseMemset32(ScreenOff, 0x00000000, SCREEN_WIDTH * SCREEN_HEIGHT);

        /*====================================================================
         * RENDERING
         * All rendering reads directly from Game struct.
         *====================================================================*/
        if (app.game) {
            extern void render_frame(const Game* game);
            extern void render_present(const Game* game);
            render_frame(app.game);
            render_present(app.game);
        }

        /* EMSCRIPTEN NOTE: Frame rate limiting handled by emscripten_set_main_loop FPS parameter
         * Native builds use manual timing with SDL_Delay and busy-wait for precision */
#ifndef __EMSCRIPTEN__
        /* Frame rate limiting - sleep to yield CPU to OS */
        uint32_t frame_time = SDL_GetTicks() - app.curr_tick;
        uint32_t target_frame_time = 1000 / 60;  /* ~16.67ms for 60 FPS */

        if (frame_time < target_frame_time) {
            uint32_t delay = target_frame_time - frame_time;
            /* Sleep for most of the remaining time to be nice to OS scheduler */
            if (delay > 1) {
                SDL_Delay(delay - 1);  /* Sleep all but 1ms */
            }
            /* Final busy-wait for precision on the last millisecond */
            while ((SDL_GetTicks() - app.curr_tick) < target_frame_time) {
                /* Spin */
            }
        }
#endif
        /* Update prev_tick for next frame's dt calculation */
        app.prev_tick = app.curr_tick;

        /* Check quit condition */
        if (QUIT_SIGNAL) {
            app.running = 0;
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#endif
            return;
        }
}

/*
 * Main game loop wrapper
 * EMSCRIPTEN NOTE: For browser builds, uses emscripten_set_main_loop to yield control to browser
 * Native builds use traditional while loop
 */
void run_game(void) {
    /* Reset game loop state */
    app.running = 1;
    app.menu_result = 0;

    app.prev_tick = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
    /* EMSCRIPTEN: Use emscripten_set_main_loop for browser-friendly event loop
     * - Yields control to browser between frames
     * - Allows browser to process events and rendering
     * - 0 = use requestAnimationFrame for optimal browser frame rate, 1 = simulate infinite loop */
    emscripten_set_main_loop(game_loop_iteration, 0, 1);
#else
    /* Native build: Traditional while loop */
    while (app.running) {
        game_loop_iteration();
    }
#endif
}

#ifdef __EMSCRIPTEN__
/*
 * Restart game from beginning - called after exit
 * EMSCRIPTEN: Allows restarting without page reload
 * Skips title screen on restart to keep audio off until user is ready
 */
EMSCRIPTEN_KEEPALIVE
void RestartGame(void) {
    printf("Game exited - waiting for user click to restart...\n");

    /* Wait for user click (gameStarted flag will be set by canvas click) */
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
    app.running = 1;
    app.menu_result = 0;
    if (app.game) bridge_clear_menu_request(app.game);  /* Show title screen first */
    level_is_loaded = 0;

    /* Show title screen with intro (starts music) */
    display_title_screen();

    /* Then go to menu */
    if (app.game) bridge_request_menu(app.game);

    /* Restart main game loop */
    emscripten_set_main_loop(game_loop_iteration, 0, 1);
}
#endif

/*
 * Load a game level
 */
void load_level(int level_id) {
    printf("Loading level %d\n", level_id);

    /* Calculate level index (menu IDs are 2-7 for levels 0-5) */
    app.active_level = level_id - 2;

    /* Reset game state */
    game_turnout = PLAYER_NORMAL;
    app.victory_defeat_timer = 0;
    level_is_loaded = 1;

    /* Start level via Game struct - this sets up player, enemies, waves, particles */
    if (app.game) {
        /* Set captain planet mode if enabled */
        bridge_set_captain_planet(app.game, num_starting_nukes == 18 ? 1 : 0);
        bridge_set_weapons_level(app.game, intPlayerWeaponsLevel);
        bridge_start_level(app.game, app.active_level);
    }

    /* Fade out current music - callback will load level music */
    Mix_FadeOutMusic(200);

    /* Load map based on level */
    const char* map_files[] = {
        "./data/shire.bmp",       /* Level 0 */
        "./data/archipelago.bmp", /* Level 1 */
        "./data/dune.bmp",        /* Level 2 */
        "./data/midkemia.bmp",    /* Level 3 */
        "./data/oceania.bmp",     /* Level 4 */
        "./data/mordor.bmp"       /* Level 5 */
    };

    if (app.active_level >= 0 && app.active_level < 6) {
        LoadMap(map_files[app.active_level]);
    }

    /* Set ending timer for Mordor */
    if (app.active_level >= 5) {
        app.ending_delay_timer = 250;
    }
}

/*
 * Run ending sequence - mirrors assembly _RunEnding (lines 1619-1697)
 * EMSCRIPTEN NOTE: Now uses emscripten_sleep to yield properly
 */
void run_ending_sequence(void) {
    extern SDL_Renderer* screen_renderer;
    extern SDL_Texture* screen_texture;
    extern uint8_t KEYBOARD[320];

    app.is_ending = 1;

    /* Fade out current music - mirrors line 1623 */
    /* This will trigger music_track_finished_callback which loads ending_theme.ogg */
    Mix_FadeOutMusic(100);

    /* Wait a moment for fade to complete and ending theme to start */
#ifdef __EMSCRIPTEN__
    emscripten_sleep(200);
#else
    SDL_Delay(200);
#endif

    /* Auto-advance to ending clips - no wait here */
    /* Halt all audio channels - mirrors line 1648 */
    Mix_HaltChannel(-1);

    if (!story_clips) {
        app.is_ending = 0;
        return;
    }

    /* Clear keyboard state to prevent fade functions from waiting */
    FlushKeyboard();

    /* Show 3 story clips with fade transitions - mirrors lines 1650-1675 */
    /* NOTE: FlushKeyboard() before each fade ensures auto-advance without waiting */

    /* Clip 0: lines 1655-1659 */
    sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
    FlushKeyboard();  /* Ensure fade doesn't wait for keys */
    FadeToWhite(500, 2);  /* Fade: 5 seconds */

    sseMemset32(ScreenOff, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
    sseMemset32(ScreenTemp, 0, SCREEN_WIDTH * SCREEN_HEIGHT);

    AlphaBlit(SCREEN_WIDTH / 2 - 640 / 2, SCREEN_HEIGHT / 2 - 480 / 2,
              story_clips + (640 * 480 * 0), 640, 480);
    sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
    FlushKeyboard();  /* Ensure fade doesn't wait for keys */
    FadeFromWhite(300, 2);  /* Fade: 3 seconds */

    /* Clip 1: lines 1661-1667 */
    FlushKeyboard();  /* Ensure fade doesn't wait for keys */
    FadeToWhite(300, 2);  /* Fade: 3 seconds */

    AlphaBlit(SCREEN_WIDTH / 2 - 640 / 2, SCREEN_HEIGHT / 2 - 480 / 2,
              story_clips + (640 * 480 * 1), 640, 480);
    sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
    FlushKeyboard();  /* Ensure fade doesn't wait for keys */
    FadeFromWhite(300, 2);  /* Fade: 3 seconds */

    /* Clip 2: lines 1669-1675 */
    FlushKeyboard();  /* Ensure fade doesn't wait for keys */
    FadeToWhite(300, 2);  /* Fade: 3 seconds */

    AlphaBlit(SCREEN_WIDTH / 2 - 640 / 2, SCREEN_HEIGHT / 2 - 480 / 2,
              story_clips + (640 * 480 * 2), 640, 480);
    sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
    FlushKeyboard();  /* Ensure fade doesn't wait for keys */
    FadeFromWhite(300, 2);  /* Fade: 3 seconds */

    /* Wait for final key press - mirrors lines 1677-1692 */
    FlushKeyboard();
    SDL_Event event;  /* Declare event variable for quit detection */
    int waiting_to_finish = 1;
    while (waiting_to_finish) {
#ifdef __EMSCRIPTEN__
        emscripten_sleep(16);  /* Yield to browser */
#else
        SDL_Delay(16);
#endif

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                app.is_ending = 0;
                return;
            }
        }

        UpdateInput();

        /* Count keys pressed */
        int keys_pressed = 0;
        for (int i = 0; i < 128; i++) {
            if (KEYBOARD[i] == 1) {
                keys_pressed++;
            }
        }

        /* Exit when any key pressed (assembly line 1691-1692) */
        if (keys_pressed > 0) {
            waiting_to_finish = 0;
        }
    }

    /* Fade to black - mirrors line 1694 */
    FadeToBlack(120, 3);

    /* Mark game as beaten in localStorage to unlock Captain Planet mode */
#ifdef __EMSCRIPTEN__
    EM_ASM(
        localStorage.setItem('alan_parsons_beaten', '1');
        console.log('Game beaten! Captain Planet mode unlocked.');
    );
#endif

    app.is_ending = 0;
}

/*
 * Display title screen
 */
void display_title_screen(void) {
    printf("Title screen...\n");

    SDL_ShowCursor(SDL_DISABLE);

    /* Set menu running flag and start menu music */
    extern uint8_t IsMenuRunning;
    IsMenuRunning = 1;
    music_track_finished_callback();

    /* Display title screen if loaded - mirrors assembly lines 977-983 */
    /* EMSCRIPTEN NOTE: Fade functions now use emscripten_sleep to yield properly */
    extern uint32_t ScreenTemp[SCREEN_WIDTH * SCREEN_HEIGHT];
    if (title_screen) {
        sseMemset32(ScreenOff, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
        AlphaBlit(
            SCREEN_WIDTH / 2 - 480 / 2,
            SCREEN_HEIGHT / 2 - 480 / 2,
            title_screen,
            480,
            480
        );
        sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
        FadeFromBlack(180, 2);
        FadeToBlack(180, 3);
    }

    /* Show story clip if available - mirrors assembly lines 985-992 */
    if (story_clips) {
        /* Show 4th story clip (index 3) like assembly does */
        uint32_t* clip = story_clips + (640 * 480 * 3);
        sseMemset32(ScreenOff, 0, SCREEN_WIDTH * SCREEN_HEIGHT);
        AlphaBlit(
            SCREEN_WIDTH / 2 - 640 / 2,
            SCREEN_HEIGHT / 2 - 480 / 2,
            clip,
            640,
            480
        );
        sseMemcpy32(ScreenTemp, ScreenOff, SCREEN_WIDTH * SCREEN_HEIGHT);
        FadeFromBlack(180, 3);
        FadeToBlack(180, 3);
    }

    /* Wait for escape key or mouse button to be released - mirrors assembly lines 996-1000 */
    int done = 0;
    while (!done) {
        UpdateInput();
        /* Exit when neither key nor button is pressed (both released) */
        if (KEYBOARD[SDL_SCANCODE_ESCAPE] != 1 && MOUSE_LBUTTON != 1) {
            done = 1;
        }
#ifdef __EMSCRIPTEN__
        /* EMSCRIPTEN: Yield to browser */
        emscripten_sleep(16);
#endif
    }

    printf("Ready to start game\n");
}

/*
 * Music track finished callback - loads appropriate music for current game state
 */
void music_track_finished_callback(void) {
    /* Free previous track if it exists */
    if (snd_track) {
        Mix_FreeMusic(snd_track);
        snd_track = NULL;
    }

    /* Set music volume */
    Mix_VolumeMusic(110);

    /* Check if ending sequence is running */
    if (app.is_ending) {
        snd_track = Mix_LoadMUS("./sound/ending_theme.ogg");
        if (snd_track) {
            Mix_PlayMusic(snd_track, 1);
        }
        return;
    }

    /* Check if menu is running */
    extern uint8_t IsMenuRunning;
    if (IsMenuRunning) {
        snd_track = Mix_LoadMUS("./sound/menu_theme.ogg");
        if (snd_track) {
            Mix_PlayMusic(snd_track, 1);
        }
        return;
    }

    /* Load level-specific music */
    const char* music_files[] = {
        "./sound/sound_track1.ogg",  /* Shire */
        "./sound/sound_track2.ogg",  /* Archipelago */
        "./sound/sound_track3.ogg",  /* Dune */
        "./sound/sound_track4.ogg",  /* Midkemia */
        "./sound/sound_track5.ogg",  /* Oceania */
        "./sound/sound_track6.ogg"   /* Mordor */
    };

    if (app.active_level >= 0 && app.active_level < 6) {
        snd_track = Mix_LoadMUS(music_files[app.active_level]);
        if (snd_track) {
            Mix_PlayMusic(snd_track, 1);
        }
    }
}
