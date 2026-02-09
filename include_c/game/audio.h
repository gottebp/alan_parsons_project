/*
 * audio.h - Sound System Abstraction
 *
 * Clean audio interface that abstracts SDL_mixer.
 * Game code calls audio_play() - implementation handles the rest.
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <stdint.h>

/*============================================================================
 * SOUND IDENTIFIERS
 *
 * Named constants for all game sounds.
 * The game code never knows about file paths.
 *============================================================================*/

typedef enum {
    /* Sound effects */
    SOUND_WEAPON_FIRE,
    SOUND_ENGINE_THRUST,
    SOUND_HIT_PLAYER,
    SOUND_EXPLOSION_1,
    SOUND_EXPLOSION_2,
    SOUND_EXPLOSION_3,
    SOUND_EXPLOSION_4,
    SOUND_EXPLOSION_5,
    SOUND_EVIL_LAUGH,

    /* Music tracks */
    MUSIC_MENU,
    MUSIC_SHIRE,
    MUSIC_ARCHIPELAGO,
    MUSIC_DUNE,
    MUSIC_MIDKEMIA,
    MUSIC_OCEANIA,
    MUSIC_MORDOR,
    MUSIC_ENDING,

    SOUND_COUNT
} SoundId;

/*============================================================================
 * SOUND CHANNELS
 *
 * Dedicated channels for sounds that should be controlled individually.
 *============================================================================*/

typedef enum {
    CHANNEL_ANY = -1,      /* Let mixer choose */
    CHANNEL_WEAPON = 1,    /* Weapon fire loop */
    CHANNEL_ENGINE = 3,    /* Engine thrust loop */
} SoundChannel;

/*============================================================================
 * AUDIO CONTEXT
 *
 * Holds all audio resources and state.
 *============================================================================*/

struct Mix_Chunk;
struct Mix_Music;

typedef struct {
    /* Sound effects */
    struct Mix_Chunk* sounds[SOUND_COUNT];

    /* Current music track */
    struct Mix_Music* current_music;
    int current_music_id;

    /* Volume settings (0-128) */
    int music_volume;
    int sfx_volume;

    /* State tracking */
    int weapon_playing;
    int engine_playing;
    int initialized;
} AudioContext;

/*============================================================================
 * INITIALIZATION AND SHUTDOWN
 *============================================================================*/

/* Initialize audio system */
int audio_init(AudioContext* ctx);

/* Load all audio assets */
int audio_load_assets(AudioContext* ctx);

/* Shut down audio system */
void audio_shutdown(AudioContext* ctx);

/*============================================================================
 * PLAYBACK CONTROL
 *============================================================================*/

/* Play a sound effect once */
void audio_play(AudioContext* ctx, SoundId sound);

/* Play a sound effect on a specific channel (for looping sounds) */
void audio_play_channel(AudioContext* ctx, SoundId sound, SoundChannel channel);

/* Stop sound on a specific channel */
void audio_stop_channel(AudioContext* ctx, SoundChannel channel);

/* Play music track (with fade) */
void audio_play_music(AudioContext* ctx, SoundId music);

/* Stop music (with fade) */
void audio_stop_music(AudioContext* ctx, int fade_ms);

/* Check if music is playing */
int audio_music_playing(AudioContext* ctx);

/*============================================================================
 * GAME-SPECIFIC HELPERS
 *
 * Convenient functions for common game audio patterns.
 *============================================================================*/

/* Start or continue weapon fire sound */
void audio_start_weapon(AudioContext* ctx);

/* Stop weapon fire sound */
void audio_stop_weapon(AudioContext* ctx);

/* Start or continue engine thrust sound */
void audio_start_engine(AudioContext* ctx);

/* Stop engine thrust sound */
void audio_stop_engine(AudioContext* ctx);

/* Play random explosion sound */
void audio_play_explosion(AudioContext* ctx);

/* Play level music based on level index (0-5) */
void audio_play_level_music(AudioContext* ctx, int level_index);

/*============================================================================
 * VOLUME CONTROL
 *============================================================================*/

/* Set master music volume (0-128) */
void audio_set_music_volume(AudioContext* ctx, int volume);

/* Set master sound effects volume (0-128) */
void audio_set_sfx_volume(AudioContext* ctx, int volume);

#endif /* AUDIO_H */
