/*
 * audio_bridge.h - Audio Bridge for New Architecture
 *
 * Functions that trigger sounds based on Game state and events.
 * Uses the legacy SDL_mixer infrastructure while reading from Game struct.
 */

#ifndef AUDIO_BRIDGE_H
#define AUDIO_BRIDGE_H

#include "game.h"

/*============================================================================
 * AUDIO EVENT TRACKING
 *
 * The game fires events that should trigger sounds. We track these
 * to avoid duplicate sound triggers and manage looping sounds.
 *============================================================================*/

typedef struct {
    /* Looping sound states */
    int weapon_firing;
    int engine_thrusting;

    /* One-shot tracking (debounce) */
    int last_explosion_frame;
    int last_hit_frame;

    /* Music state */
    int current_level_music;
    int music_playing;
} AudioBridgeState;

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

/* Set the audio context (call once during app init) */
void audio_bridge_set_context(void* hit, void* evil_laugh, void* explosion[5], void* engines, void* weapon);

/* Initialize audio bridge state */
void audio_bridge_init(AudioBridgeState* state);

/*============================================================================
 * PER-FRAME UPDATE
 *
 * Call this each frame to update audio based on game state.
 *============================================================================*/

/* Update all audio based on current game state and input */
void audio_bridge_update(AudioBridgeState* state, const Game* game, const InputState* input);

/*============================================================================
 * EVENT-BASED TRIGGERS
 *
 * Call these when specific events occur.
 *============================================================================*/

/* Player took damage */
void audio_bridge_player_hit(AudioBridgeState* state);

/* Enemy was destroyed */
void audio_bridge_enemy_destroyed(AudioBridgeState* state);

/* Nuke was dropped */
void audio_bridge_nuke_dropped(AudioBridgeState* state);

/* Boss appeared */
void audio_bridge_boss_spawned(AudioBridgeState* state);

/* Level started - play appropriate music */
void audio_bridge_level_start(AudioBridgeState* state, int level_index);

/* Victory achieved */
void audio_bridge_victory(AudioBridgeState* state);

/* Defeat occurred */
void audio_bridge_defeat(AudioBridgeState* state);

/*============================================================================
 * MUSIC CONTROL
 *============================================================================*/

/* Start menu music */
void audio_bridge_menu_music(AudioBridgeState* state);

/* Stop all music */
void audio_bridge_stop_music(AudioBridgeState* state);

/* Fade out music */
void audio_bridge_fade_music(AudioBridgeState* state, int fade_ms);

#endif /* AUDIO_BRIDGE_H */
