/*
 * app.h - Application Context
 *
 * The App struct owns all platform-level concerns:
 * - Window and renderer lifecycle
 * - Audio system
 * - Frame timing
 * - Input polling
 * - The Game itself
 *
 * This separation means:
 * - Game logic is testable without SDL
 * - Platform code doesn't touch game internals
 * - Clear ownership of resources
 * - main.c becomes a simple orchestrator
 */

#ifndef APP_H
#define APP_H

#include "../game/game.h"
#include "../game/audio_bridge.h"
#include "../core/types.h"
#include <stdint.h>

/*============================================================================
 * AUDIO HANDLES
 *============================================================================*/

typedef struct {
    /* Sound effects */
    void* snd_engines;
    void* snd_weapon;
    void* snd_hit;
    void* snd_evil_laugh;
    void* snd_explosion[5];

    /* Music */
    void* music_menu;
    void* music_ending;
    void* music_level[6];
    void* music_current;

    /* State tracking for continuous sounds */
    int engines_playing;
    int weapon_cooldown;
} AudioAssets;

/*============================================================================
 * VISUAL ASSETS
 *============================================================================*/

typedef struct {
    uint32_t* title_screen;
    uint32_t* victory_screen;
    uint32_t* defeat_screen;
    uint32_t* story_clips;
    uint32_t nuke_icon[32 * 32];
} VisualAssets;

/*============================================================================
 * APP CONTEXT
 *============================================================================*/

typedef struct {
    /*------------------------------------------------------------------------
     * LIFECYCLE
     *------------------------------------------------------------------------*/
    int running;            /* 1 = app is running, 0 = should exit */
    int initialized;        /* 1 = init complete */
    int fullscreen;         /* Launch in fullscreen mode */
    int captain_planet;     /* Extra nukes mode (18 instead of 4) */

    /*------------------------------------------------------------------------
     * THE GAME
     *------------------------------------------------------------------------*/
    Game game;

    /*------------------------------------------------------------------------
     * TIMING
     *------------------------------------------------------------------------*/
    uint32_t prev_tick;     /* Previous frame's tick count */
    uint32_t curr_tick;     /* Current frame's tick count */
    float dt;               /* Delta time in seconds */
    uint32_t frame_count;   /* Total frames rendered */

    /*------------------------------------------------------------------------
     * INPUT
     *------------------------------------------------------------------------*/
    InputState input;       /* Current frame's input state */

    /*------------------------------------------------------------------------
     * ASSETS
     *------------------------------------------------------------------------*/
    AudioAssets audio;
    VisualAssets visuals;

    /*------------------------------------------------------------------------
     * AUDIO BRIDGE (for game sounds)
     *------------------------------------------------------------------------*/
    AudioBridgeState audio_state;

    /*------------------------------------------------------------------------
     * UI STATE
     *------------------------------------------------------------------------*/
    int ending_timer;       /* Delay before ending sequence */
    int in_ending;          /* 1 = ending sequence active */

} App;

/*============================================================================
 * APP LIFECYCLE
 *============================================================================*/

/* Initialize the application (SDL, audio, load resources) */
int app_init(App* app, int argc, char** argv);

/* Shutdown and free all resources */
void app_shutdown(App* app);

/* Check if app should continue running */
int app_running(const App* app);

/*============================================================================
 * FRAME OPERATIONS
 *============================================================================*/

/* Begin a frame - updates timing, polls input */
float app_frame_begin(App* app);

/* End a frame - presents, handles frame limiting */
void app_frame_end(App* app);

/* Poll input into the app's input state */
void app_poll_input(App* app);

/*============================================================================
 * GAME OPERATIONS
 *============================================================================*/

/* Load a level (level_id: 2-7 from menu, maps to 0-5 internally) */
void app_load_level(App* app, int level_id);

/* Request the menu to be shown */
void app_show_menu(App* app);

/* Handle menu result */
void app_handle_menu_result(App* app, int result);

/*============================================================================
 * SEQUENCES
 *============================================================================*/

/* Display title screen with intro */
void app_title_screen(App* app);

/* Run ending sequence after beating final level */
void app_ending_sequence(App* app);

/*============================================================================
 * AUDIO OPERATIONS
 *============================================================================*/

/* Update continuous audio (engines, etc) based on game state */
void app_update_audio(App* app);

/* Play level music */
void app_play_level_music(App* app, int level_index);

/* Play menu music */
void app_play_menu_music(App* app);

/* Stop all music */
void app_stop_music(App* app);

#endif /* APP_H */
