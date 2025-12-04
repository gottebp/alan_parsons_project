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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

/* Game state globals */
int GameState = 0;  /* Global - used by enemy.c for win/lose checking */
static int activate_menu = 0;
uint8_t game_turnout = PLAYER_NORMAL;  /* Exported for player module */
int game_level = 0;  /* Exported for menu module */
static int active_level = 0;
static int nukes_remaining = 0;
static int num_starting_nukes = NUM_NUKES;
static int nuke_wait_counter = 0;
static int victory_defeat_timer = 0;  /* Auto-advance after 7 seconds on victory/defeat */

/* Timing */
static uint32_t current_tick_count = 0;
static uint32_t prev_tick_count = 0;

/* Lookup tables */
float SIN_LOOK[256];
float COS_LOOK[256];

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
static uint32_t* victory_screen = NULL;
static uint32_t* defeat_screen = NULL;
static uint32_t* story_clips = NULL;
static uint32_t nuke_img[32 * 32];

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

/* Game ending state */
static int is_ending_running = 0;
static int ending_delay_timer = 0;

/* Forward declarations */
void initialize_game_data(void);
void destroy_game_data(void);
void run_game(void);
void process_input(void);
void load_level(int level_id);
void display_title_screen(void);
void init_trig_tables(void);
void render_minimap(void);
void run_ending_sequence(void);
void draw_minimap_object(int x, int y, uint32_t color1, uint32_t color2, int is_boss);
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
    activate_menu = 1;
    GameState = 0;

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
    InitPlayer();
    InitParticleEngine();
    LoadEnemyData();

    /* Set initial player position */
    fltPlayerX = (float)MapMidX;
    fltPlayerY = (float)MapMidY;
    intPlayerX = MapMidX;
    intPlayerY = MapMidY;
    intbPlayerAngle = PLAYER_START_ANGLE;
    fltPlayerSpeed = 0.0f;
    fltPlayerStrafeSpeed = 0.0f;
    intPlayerHealth = MAXPLAYERHEALTH;

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
    active_level = 0;
    nukes_remaining = num_starting_nukes;

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
    DestroyPlayer();
    DestroyMenu();
    DestroyEnemyData();

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

/* Game loop state - static for Emscripten persistence between iterations */
static int game_running = 1;
static int game_menu_result = 0;

/*
 * Single iteration of game loop - called by Emscripten or native loop
 */
void game_loop_iteration(void) {
    /* Early exit if not running */
    if (!game_running) {
        return;
    }
        /* Update input */
        UpdateInput();

        /* Check for victory/defeat auto-advance timer (7 seconds at 60 FPS = 420 frames) */
        if (game_turnout == PLAYER_WIN || game_turnout == PLAYER_DEAD) {
            /* Start timer if not already started */
            if (victory_defeat_timer == 0) {
                victory_defeat_timer = 420;  /* 7 seconds at 60 FPS */
            }

            /* Decrement timer */
            if (victory_defeat_timer > 0) {
                victory_defeat_timer--;

                /* Auto-activate menu after 7 seconds */
                if (victory_defeat_timer == 0) {
                    activate_menu = 1;
                }
            }
        }

        /* Check for escape key - activate menu (also advances victory/defeat screen) */
        if (KEYBOARD[SDL_SCANCODE_ESCAPE]) {
            activate_menu = 1;
        }

        /* Check if menu should be activated */
        if (activate_menu) {
            activate_menu = 0;
            victory_defeat_timer = 0;  /* Reset timer */

            game_menu_result = RunMenu(GameState);

            /* Check if user selected a level (2-7) */
            if (game_menu_result >= 2 && game_menu_result <= 7) {
                load_level(game_menu_result);
            }
        }

        /* Check if user quit from menu */
        if (game_menu_result == 0) {
            game_running = 0;
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
            game_running = 0;
#ifdef __EMSCRIPTEN__
            emscripten_cancel_main_loop();
#endif
            return;
        }

        /* Only run game logic if a level is loaded */
        if (GameState == 0) {
            /* No level loaded - wait for menu input */
            /* EMSCRIPTEN NOTE: SDL_Delay handled by emscripten_set_main_loop FPS parameter */
#ifndef __EMSCRIPTEN__
            SDL_Delay(16); /* ~60 FPS */
#endif
            return;
        }

        /* Decrement nuke wait counter - mirrors assembly line 357-360 */
        if (nuke_wait_counter > 0) {
            nuke_wait_counter--;
        }

        /* Process game input */
        process_input();

        /* Update game entities - mirrors assembly lines 362-366 */
        UpdatePlayer();
        UpdateEnemies();
        UpdateParticles(0);
        DetectPlayerCollisions();

        /* Check victory/defeat conditions */
        if (game_turnout == PLAYER_WIN) {
            /* Advance level if player beat their current max level */
            if (active_level >= game_level) {
                game_level++;
                intPlayerWeaponsLevel++;

                /* Save progress */
                /* EMSCRIPTEN NOTE: Save to localStorage for reliable browser persistence */
#ifdef __EMSCRIPTEN__
                EM_ASM({
                    localStorage.setItem('alan_parsons_level', $0);
                    localStorage.setItem('alan_parsons_weapons', $1);
                    console.log('Progress saved to localStorage - Level:', $0, 'Weapons:', $1);
                }, game_level, intPlayerWeaponsLevel);
#else
                /* Native build uses file-based saves */
                FILE* fp = fopen("level.dat", "wb");
                if (fp) {
                    fwrite(&game_level, sizeof(int), 1, fp);
                    fwrite(&intPlayerWeaponsLevel, sizeof(int), 1, fp);
                    fclose(fp);
                }
#endif
            }

            /* Restore health */
            intPlayerHealth = MAXPLAYERHEALTH;

            /* Check if we should run ending (after beating Mordor) - mirrors assembly lines 413-425 */
            if (active_level >= 5) {
                ending_delay_timer--;
                if (ending_delay_timer <= 0) {
                    /* Run ending sequence - mirrors assembly _RunEnding lines 1619-1697 */
                    run_ending_sequence();

                    /* Return to menu */
                    activate_menu = 1;
                    game_turnout = PLAYER_NORMAL;
                    ending_delay_timer = 300;  /* Reset for next time */
                }
            }
        }

        /* Render scene */
        sseMemset32(ScreenOff, 0x00000000, SCREEN_WIDTH * SCREEN_HEIGHT);

        RenderMap();
        RenderPlayer();
        RenderEnemies();
        RenderParticles(0);
        DrawPlayerHealth();
        render_minimap();

        /* Draw nuke icons if available */
        if (nukes_remaining > 0) {
            for (int i = 0; i < nukes_remaining; i++) {
                int x = SCREEN_WIDTH - 46 - (i * 34);
                int y = 18;
                AlphaBlit(x, y, nuke_img, 32, 32);
            }
        }

        /* Show victory screen if player won */
        if (game_turnout == PLAYER_WIN && victory_screen) {
            AlphaBlit(
                SCREEN_WIDTH / 2 - 460 / 2,
                SCREEN_HEIGHT / 2 - 345 / 2,
                victory_screen,
                460,
                345
            );
        }

        /* Show defeat screen if player died */
        if (game_turnout == PLAYER_DEAD && defeat_screen) {
            AlphaBlit(
                SCREEN_WIDTH / 2 - 460 / 2,
                SCREEN_HEIGHT / 2 - 345 / 2,
                defeat_screen,
                460,
                345
            );
        }

        /* Update screen */
        UpdateScreen();

        /* EMSCRIPTEN NOTE: Frame rate limiting handled by emscripten_set_main_loop FPS parameter
         * Native builds use manual timing with SDL_Delay and busy-wait for precision */
#ifndef __EMSCRIPTEN__
        /* Frame rate limiting - sleep to yield CPU to OS */
        current_tick_count = SDL_GetTicks();
        uint32_t frame_time = current_tick_count - prev_tick_count;
        uint32_t target_frame_time = 1000 / 60;  /* ~16.67ms for 60 FPS */

        if (frame_time < target_frame_time) {
            uint32_t delay = target_frame_time - frame_time;
            /* Sleep for most of the remaining time to be nice to OS scheduler */
            if (delay > 1) {
                SDL_Delay(delay - 1);  /* Sleep all but 1ms */
            }
            /* Final busy-wait for precision on the last millisecond */
            do {
                current_tick_count = SDL_GetTicks();
            } while ((current_tick_count - prev_tick_count) < target_frame_time);
        }
        prev_tick_count = current_tick_count;
#endif

        /* Check quit condition */
        if (QUIT_SIGNAL) {
            game_running = 0;
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
    game_running = 1;
    game_menu_result = 0;

    prev_tick_count = SDL_GetTicks();

#ifdef __EMSCRIPTEN__
    /* EMSCRIPTEN: Use emscripten_set_main_loop for browser-friendly event loop
     * - Yields control to browser between frames
     * - Allows browser to process events and rendering
     * - 0 = use requestAnimationFrame for optimal browser frame rate, 1 = simulate infinite loop */
    emscripten_set_main_loop(game_loop_iteration, 0, 1);
#else
    /* Native build: Traditional while loop */
    while (game_running) {
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
    game_running = 1;
    game_menu_result = 0;
    activate_menu = 0;  /* Show title screen first */
    GameState = 0;

    /* Show title screen with intro (starts music) */
    display_title_screen();

    /* Then go to menu */
    activate_menu = 1;

    /* Restart main game loop */
    emscripten_set_main_loop(game_loop_iteration, 0, 1);
}
#endif

/* External mobile control values - defined in input.c */
#ifdef __EMSCRIPTEN__
extern float mobile_tilt_steer;
extern float mobile_tilt_thrust;
extern int mobile_controls_active;
#endif

/*
 * Process player input
 */
void process_input(void) {
    if (game_turnout == PLAYER_DEAD) {
        return;
    }

    /* Rotation - check for mobile analog tilt first */
    intbPlayerTurnDir = 0;

#ifdef __EMSCRIPTEN__
    /* Mobile analog tilt steering - proportional rotation */
    if (mobile_controls_active && (mobile_tilt_steer > 0.15f || mobile_tilt_steer < -0.15f)) {
        /* Scale rotation by tilt amount: more tilt = faster rotation */
        /* Max rotation speed is PLAYER_ROTATE_SPEED * 1.5 for full tilt */
        float rotate_amount = mobile_tilt_steer * PLAYER_ROTATE_SPEED * 1.5f;
        intbPlayerAngle += (int8_t)rotate_amount;

        if (mobile_tilt_steer > 0.15f) {
            intbPlayerTurnDir = 1;
            FirePlayerRightThrusters();
        } else if (mobile_tilt_steer < -0.15f) {
            intbPlayerTurnDir = -1;
            FirePlayerLeftThrusters();
        }
    } else {
#endif
        /* Keyboard rotation (desktop) */
        if (KEYBOARD[SDL_SCANCODE_RIGHT]) {
            intbPlayerAngle += PLAYER_ROTATE_SPEED;
            intbPlayerTurnDir = 1;
            FirePlayerRightThrusters();
        }
        if (KEYBOARD[SDL_SCANCODE_LEFT]) {
            intbPlayerAngle -= PLAYER_ROTATE_SPEED;
            intbPlayerTurnDir = -1;
            FirePlayerLeftThrusters();
        }
#ifdef __EMSCRIPTEN__
    }
#endif

    /* Fire weapons with sound */
    if (!KEYBOARD[SDL_SCANCODE_X]) {
        if (snd_weapon_counter > 0) {
            snd_weapon_counter--;
            Mix_HaltChannel(1);
        }
    }
    if (KEYBOARD[SDL_SCANCODE_X]) {
        if (snd_weapon_counter == 0 && snd_effect_weapon) {
            snd_weapon_counter++;
            Mix_PlayChannelTimed(1, snd_effect_weapon, -1, -1);
        }
        FirePlayerWeapons();
    }

    /* Forward/backward movement with engine sound */
    int is_thrusting = 0;
    int is_braking = 0;

#ifdef __EMSCRIPTEN__
    /* Mobile analog tilt thrust - proportional acceleration */
    if (mobile_controls_active && (mobile_tilt_thrust > 0.2f || mobile_tilt_thrust < -0.2f)) {
        if (mobile_tilt_thrust > 0.2f) {
            /* Forward thrust - scale acceleration by tilt amount */
            is_thrusting = 1;
            float thrust_mult = mobile_tilt_thrust;  /* 0.2 to 1.0 */
            if (fltPlayerSpeed < fltPlayerMaxSpeed) {
                fltPlayerSpeed += fltPlayerAccel * thrust_mult;
            }
            FirePlayerMainThrusters();
        } else if (mobile_tilt_thrust < -0.2f) {
            /* Backward/brake - scale deceleration by tilt amount */
            is_braking = 1;
            float brake_mult = -mobile_tilt_thrust;  /* 0.2 to 1.0 */
            if (fltPlayerSpeed > fltPlayerMinSpeed) {
                fltPlayerSpeed -= fltPlayerAccel * brake_mult;
            }
        }
    }
#endif

    /* Keyboard/button thrust (fallback or in addition to tilt) */
    if (!is_thrusting && !KEYBOARD[SDL_SCANCODE_UP]) {
        if (snd_engines_counter > 0) {
            snd_engines_counter--;
            Mix_HaltChannel(3);
        }
    }
    if (KEYBOARD[SDL_SCANCODE_UP]) {
        is_thrusting = 1;
        if (snd_engines_counter == 0 && snd_effect_engines) {
            snd_engines_counter++;
            Mix_PlayChannelTimed(3, snd_effect_engines, -1, -1);
        }
        if (fltPlayerSpeed < fltPlayerMaxSpeed) {
            fltPlayerSpeed += fltPlayerAccel;
        }
        FirePlayerMainThrusters();
    }
    if (KEYBOARD[SDL_SCANCODE_DOWN]) {
        is_braking = 1;
        if (fltPlayerSpeed > fltPlayerMinSpeed) {
            fltPlayerSpeed -= fltPlayerAccel;
        }
    }

#ifdef __EMSCRIPTEN__
    /* Handle engine sound for mobile tilt thrust */
    if (is_thrusting && snd_engines_counter == 0 && snd_effect_engines) {
        snd_engines_counter++;
        Mix_PlayChannelTimed(3, snd_effect_engines, -1, -1);
    }
#endif

    /* Strafe */
    if (KEYBOARD[SDL_SCANCODE_C]) {
        if (fltPlayerStrafeSpeed < fltPlayerMaxStrafeSpeed) {
            fltPlayerStrafeSpeed += fltPlayerAccel;
        }
        intbPlayerTurnDir++;
    }
    if (KEYBOARD[SDL_SCANCODE_Z]) {
        if (fltPlayerStrafeSpeed > fltPlayerMinStrafeSpeed) {
            fltPlayerStrafeSpeed -= fltPlayerAccel;
        }
        intbPlayerTurnDir--;
    }

    /* Nuke launch - mirrors assembly lines 837-848 */
    if (KEYBOARD[SDL_SCANCODE_SPACE]) {
        if (nuke_wait_counter == 0 && nukes_remaining > 0) {
            nukes_remaining--;
            nuke_wait_counter = 25;
            DropNuke(fltPlayerX, fltPlayerY);
        }
    }
}

/*
 * Load a game level
 */
void load_level(int level_id) {
    printf("Loading level %d\n", level_id);

    /* Reset game state - mirrors assembly lines 866-872 */
    nukes_remaining = num_starting_nukes;
    game_turnout = PLAYER_NORMAL;
    victory_defeat_timer = 0;  /* Reset auto-advance timer */
    GameState = 1;
    intShakeMap = 0;

    /* Calculate active level and initialize enemies - mirrors assembly lines 874-877 */
    active_level = level_id - 2;
    InitializeEnemies(active_level);

    /* Fade out current music - callback will load level music - mirrors assembly line 879 */
    Mix_FadeOutMusic(200);

    /* Reset player - mirrors assembly lines 881-900 */
    fltPlayerX = (float)MapMidX;
    fltPlayerY = (float)MapMidY;
    intPlayerX = MapMidX;
    intPlayerY = MapMidY;
    intbPlayerAngle = PLAYER_START_ANGLE;
    fltPlayerSpeed = 0.0f;
    fltPlayerStrafeSpeed = 0.0f;
    intPlayerHealth = MAXPLAYERHEALTH;

    /* Reset particles - mirrors assembly line 902 */
    ResetParticleEngine();

    /* Load map based on level - mirrors assembly lines 904-959 */
    switch (level_id) {
        case MENU_LOAD_SHIRE:
            LoadMap("./data/shire.bmp");
            break;
        case MENU_LOAD_MORDOR:
            ending_delay_timer = 250;  /* Trigger ending after winning Mordor */
            LoadMap("./data/mordor.bmp");
            break;
        case MENU_LOAD_MIDKEMIA:
            LoadMap("./data/midkemia.bmp");
            break;
        case MENU_LOAD_ARCHIPELAGO:
            LoadMap("./data/archipelago.bmp");
            break;
        case MENU_LOAD_DUNE:
            LoadMap("./data/dune.bmp");
            break;
        case MENU_LOAD_OCEANIA:
            LoadMap("./data/oceania.bmp");
            break;
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

    is_ending_running = 1;

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
        is_ending_running = 0;
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
                is_ending_running = 0;
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

    is_ending_running = 0;
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
 * Draw an object on the minimap
 */
void draw_minimap_object(int x, int y, uint32_t color1, uint32_t color2, int is_boss) {
    /* Scale world coordinates to minimap coordinates */
    int map_x = (x / (MAP_WIDTH / MINIMAP_WIDTH)) + 16;
    int map_y = (y / (MAP_HEIGHT / MINIMAP_HEIGHT)) + 20;

    /* Calculate screen buffer offset */
    int offset = map_y * SCREEN_WIDTH + map_x;

    /* Draw center and immediate neighbors with color1 */
    ScreenOff[offset] = ComputeAlpha(color1, ScreenOff[offset]);
    ScreenOff[offset - 1] = ComputeAlpha(color1, ScreenOff[offset - 1]);
    ScreenOff[offset + 1] = ComputeAlpha(color1, ScreenOff[offset + 1]);
    ScreenOff[offset + SCREEN_WIDTH] = ComputeAlpha(color1, ScreenOff[offset + SCREEN_WIDTH]);
    ScreenOff[offset - SCREEN_WIDTH] = ComputeAlpha(color1, ScreenOff[offset - SCREEN_WIDTH]);

    /* Draw diagonal and extended pixels with color2 */
    ScreenOff[offset - SCREEN_WIDTH - 1] = ComputeAlpha(color2, ScreenOff[offset - SCREEN_WIDTH - 1]);
    ScreenOff[offset - SCREEN_WIDTH + 1] = ComputeAlpha(color2, ScreenOff[offset - SCREEN_WIDTH + 1]);
    ScreenOff[offset + SCREEN_WIDTH - 1] = ComputeAlpha(color2, ScreenOff[offset + SCREEN_WIDTH - 1]);
    ScreenOff[offset + SCREEN_WIDTH + 1] = ComputeAlpha(color2, ScreenOff[offset + SCREEN_WIDTH + 1]);

    ScreenOff[offset - 2] = ComputeAlpha(color2, ScreenOff[offset - 2]);
    ScreenOff[offset + 2] = ComputeAlpha(color2, ScreenOff[offset + 2]);
    ScreenOff[offset + SCREEN_WIDTH * 2] = ComputeAlpha(color2, ScreenOff[offset + SCREEN_WIDTH * 2]);
    ScreenOff[offset - SCREEN_WIDTH * 2] = ComputeAlpha(color2, ScreenOff[offset - SCREEN_WIDTH * 2]);

    /* Draw larger pattern for bosses */
    if (is_boss) {
        ScreenOff[offset - 3] = ComputeAlpha(color1, ScreenOff[offset - 3]);
        ScreenOff[offset + 3] = ComputeAlpha(color1, ScreenOff[offset + 3]);
        ScreenOff[offset - 4] = ComputeAlpha(color2, ScreenOff[offset - 4]);
        ScreenOff[offset + 4] = ComputeAlpha(color2, ScreenOff[offset + 4]);
        ScreenOff[offset + SCREEN_WIDTH * 3] = ComputeAlpha(color1, ScreenOff[offset + SCREEN_WIDTH * 3]);
        ScreenOff[offset - SCREEN_WIDTH * 3] = ComputeAlpha(color1, ScreenOff[offset - SCREEN_WIDTH * 3]);
        ScreenOff[offset + SCREEN_WIDTH * 4] = ComputeAlpha(color2, ScreenOff[offset + SCREEN_WIDTH * 4]);
        ScreenOff[offset - SCREEN_WIDTH * 4] = ComputeAlpha(color2, ScreenOff[offset - SCREEN_WIDTH * 4]);
    }
}

/*
 * Render the minimap
 */
void render_minimap(void) {
    if (game_turnout == PLAYER_DEAD) return;

    /* Draw semi-transparent dark background for minimap */
    uint32_t dark_overlay = 0x80000000;
    for (int row = 0; row < MINIMAP_HEIGHT; row++) {
        int y = 20 + row;
        for (int col = 0; col < MINIMAP_WIDTH; col++) {
            int x = 16 + col;
            int offset = y * SCREEN_WIDTH + x;
            ScreenOff[offset] = ComputeAlpha(dark_overlay, ScreenOff[offset]);
        }
    }

    /* Draw player on minimap (blue) */
    draw_minimap_object(intPlayerX, intPlayerY, 0x800000FF, 0xA0FFFFFF, 1);

    /* Draw all active enemies on minimap (red) */
    for (int i = 0; i < 100; i++) {
        if (Enemies[i].enemy_type == (uint32_t)-1) break;
        if (Enemies[i].enemy_active != 1) continue;

        /* Special case: ShimDog boss (type 3, size 1) */
        int is_boss = (Enemies[i].enemy_type == 3 && Enemies[i].enemy_size == 1) ? 1 : Enemies[i].enemy_size;

        draw_minimap_object(Enemies[i].enemy_x_int, Enemies[i].enemy_y_int, 0xA0FF0000, 0xA0FFFFFF, is_boss);
    }
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
    if (is_ending_running) {
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

    if (active_level >= 0 && active_level < 6) {
        snd_track = Mix_LoadMUS(music_files[active_level]);
        if (snd_track) {
            Mix_PlayMusic(snd_track, 1);
        }
    }
}
