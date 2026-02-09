/*
 * audio.c - Sound System Implementation
 *
 * SDL_mixer implementation of the audio interface.
 * All SDL_mixer calls are contained here.
 */

#include "../../include_c/game/audio.h"
#include <SDL_mixer.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*============================================================================
 * ASSET PATHS
 *============================================================================*/

static const char* SOUND_PATHS[] = {
    /* Sound effects */
    [SOUND_WEAPON_FIRE]  = "./sound/weapon.wav",
    [SOUND_ENGINE_THRUST] = "./sound/engines.wav",
    [SOUND_HIT_PLAYER]   = "./sound/hit.wav",
    [SOUND_EXPLOSION_1]  = "./sound/explosion1.wav",
    [SOUND_EXPLOSION_2]  = "./sound/explosion2.wav",
    [SOUND_EXPLOSION_3]  = "./sound/explosion3.wav",
    [SOUND_EXPLOSION_4]  = "./sound/explosion4.wav",
    [SOUND_EXPLOSION_5]  = "./sound/explosion5.wav",
    [SOUND_EVIL_LAUGH]   = "./sound/evil_laugh.wav",
};

static const char* MUSIC_PATHS[] = {
    [MUSIC_MENU - MUSIC_MENU]        = "./sound/menu_theme.ogg",
    [MUSIC_SHIRE - MUSIC_MENU]       = "./sound/sound_track1.ogg",
    [MUSIC_ARCHIPELAGO - MUSIC_MENU] = "./sound/sound_track2.ogg",
    [MUSIC_DUNE - MUSIC_MENU]        = "./sound/sound_track3.ogg",
    [MUSIC_MIDKEMIA - MUSIC_MENU]    = "./sound/sound_track4.ogg",
    [MUSIC_OCEANIA - MUSIC_MENU]     = "./sound/sound_track5.ogg",
    [MUSIC_MORDOR - MUSIC_MENU]      = "./sound/sound_track6.ogg",
    [MUSIC_ENDING - MUSIC_MENU]      = "./sound/ending_theme.ogg",
};

/* Default volumes matching original game */
#define DEFAULT_MUSIC_VOLUME  110
#define DEFAULT_SFX_VOLUME    128
#define VOLUME_HIT            10
#define VOLUME_WEAPON         80
#define VOLUME_EXPLOSION      128

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

int audio_init(AudioContext* ctx) {
    memset(ctx, 0, sizeof(AudioContext));

    /* Initialize SDL_mixer for OGG support */
    if (Mix_Init(MIX_INIT_OGG) == 0) {
        fprintf(stderr, "Mix_Init failed: %s\n", Mix_GetError());
        return -1;
    }

    /* Open audio device */
    if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) < 0) {
        fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
        return -1;
    }

    /* Allocate mixing channels */
    Mix_AllocateChannels(16);

    ctx->music_volume = DEFAULT_MUSIC_VOLUME;
    ctx->sfx_volume = DEFAULT_SFX_VOLUME;
    ctx->current_music_id = -1;
    ctx->initialized = 1;

    return 0;
}

int audio_load_assets(AudioContext* ctx) {
    if (!ctx->initialized) return -1;

    /* Load sound effects */
    for (int i = 0; i < SOUND_EVIL_LAUGH + 1; i++) {
        if (SOUND_PATHS[i]) {
            ctx->sounds[i] = Mix_LoadWAV(SOUND_PATHS[i]);
            if (!ctx->sounds[i]) {
                fprintf(stderr, "Warning: Could not load %s: %s\n",
                        SOUND_PATHS[i], Mix_GetError());
            }
        }
    }

    /* Set individual volumes */
    if (ctx->sounds[SOUND_HIT_PLAYER]) {
        Mix_VolumeChunk(ctx->sounds[SOUND_HIT_PLAYER], VOLUME_HIT);
    }
    if (ctx->sounds[SOUND_WEAPON_FIRE]) {
        Mix_VolumeChunk(ctx->sounds[SOUND_WEAPON_FIRE], VOLUME_WEAPON);
    }
    for (int i = SOUND_EXPLOSION_1; i <= SOUND_EXPLOSION_5; i++) {
        if (ctx->sounds[i]) {
            Mix_VolumeChunk(ctx->sounds[i], VOLUME_EXPLOSION);
        }
    }

    return 0;
}

void audio_shutdown(AudioContext* ctx) {
    if (!ctx->initialized) return;

    /* Stop all audio */
    Mix_HaltMusic();
    Mix_HaltChannel(-1);

    /* Free sound effects */
    for (int i = 0; i < SOUND_COUNT; i++) {
        if (ctx->sounds[i]) {
            Mix_FreeChunk(ctx->sounds[i]);
            ctx->sounds[i] = NULL;
        }
    }

    /* Free music */
    if (ctx->current_music) {
        Mix_FreeMusic(ctx->current_music);
        ctx->current_music = NULL;
    }

    Mix_CloseAudio();
    Mix_Quit();

    ctx->initialized = 0;
}

/*============================================================================
 * PLAYBACK CONTROL
 *============================================================================*/

void audio_play(AudioContext* ctx, SoundId sound) {
    if (!ctx->initialized) return;
    if (sound < 0 || sound >= SOUND_COUNT) return;
    if (!ctx->sounds[sound]) return;

    Mix_PlayChannel(-1, ctx->sounds[sound], 0);
}

void audio_play_channel(AudioContext* ctx, SoundId sound, SoundChannel channel) {
    if (!ctx->initialized) return;
    if (sound < 0 || sound >= SOUND_COUNT) return;
    if (!ctx->sounds[sound]) return;

    /* -1 for infinite loop on dedicated channel */
    Mix_PlayChannelTimed(channel, ctx->sounds[sound], -1, -1);
}

void audio_stop_channel(AudioContext* ctx, SoundChannel channel) {
    if (!ctx->initialized) return;
    Mix_HaltChannel(channel);
}

void audio_play_music(AudioContext* ctx, SoundId music) {
    if (!ctx->initialized) return;
    if (music < MUSIC_MENU || music > MUSIC_ENDING) return;

    /* Don't restart if same track */
    if (ctx->current_music_id == music && Mix_PlayingMusic()) {
        return;
    }

    /* Free previous track */
    if (ctx->current_music) {
        Mix_FreeMusic(ctx->current_music);
        ctx->current_music = NULL;
    }

    /* Load new track */
    int index = music - MUSIC_MENU;
    const char* path = MUSIC_PATHS[index];

    ctx->current_music = Mix_LoadMUS(path);
    if (ctx->current_music) {
        Mix_VolumeMusic(ctx->music_volume);
        Mix_PlayMusic(ctx->current_music, 1);
        ctx->current_music_id = music;
    } else {
        fprintf(stderr, "Warning: Could not load music %s: %s\n",
                path, Mix_GetError());
        ctx->current_music_id = -1;
    }
}

void audio_stop_music(AudioContext* ctx, int fade_ms) {
    if (!ctx->initialized) return;

    if (fade_ms > 0) {
        Mix_FadeOutMusic(fade_ms);
    } else {
        Mix_HaltMusic();
    }
}

int audio_music_playing(AudioContext* ctx) {
    if (!ctx->initialized) return 0;
    return Mix_PlayingMusic();
}

/*============================================================================
 * GAME-SPECIFIC HELPERS
 *============================================================================*/

void audio_start_weapon(AudioContext* ctx) {
    if (!ctx->initialized) return;

    if (!ctx->weapon_playing) {
        audio_play_channel(ctx, SOUND_WEAPON_FIRE, CHANNEL_WEAPON);
        ctx->weapon_playing = 1;
    }
}

void audio_stop_weapon(AudioContext* ctx) {
    if (!ctx->initialized) return;

    if (ctx->weapon_playing) {
        audio_stop_channel(ctx, CHANNEL_WEAPON);
        ctx->weapon_playing = 0;
    }
}

void audio_start_engine(AudioContext* ctx) {
    if (!ctx->initialized) return;

    if (!ctx->engine_playing) {
        audio_play_channel(ctx, SOUND_ENGINE_THRUST, CHANNEL_ENGINE);
        ctx->engine_playing = 1;
    }
}

void audio_stop_engine(AudioContext* ctx) {
    if (!ctx->initialized) return;

    if (ctx->engine_playing) {
        audio_stop_channel(ctx, CHANNEL_ENGINE);
        ctx->engine_playing = 0;
    }
}

void audio_play_explosion(AudioContext* ctx) {
    if (!ctx->initialized) return;

    /* Pick random explosion (1-5) */
    int which = SOUND_EXPLOSION_1 + (rand() % 5);
    audio_play(ctx, which);
}

void audio_play_level_music(AudioContext* ctx, int level_index) {
    if (!ctx->initialized) return;

    /* Level indices: 0=Shire, 1=Archipelago, 2=Dune, 3=Midkemia, 4=Oceania, 5=Mordor */
    SoundId music_ids[] = {
        MUSIC_SHIRE,
        MUSIC_ARCHIPELAGO,
        MUSIC_DUNE,
        MUSIC_MIDKEMIA,
        MUSIC_OCEANIA,
        MUSIC_MORDOR
    };

    if (level_index >= 0 && level_index < 6) {
        audio_play_music(ctx, music_ids[level_index]);
    }
}

/*============================================================================
 * VOLUME CONTROL
 *============================================================================*/

void audio_set_music_volume(AudioContext* ctx, int volume) {
    if (!ctx->initialized) return;

    ctx->music_volume = (volume < 0) ? 0 : (volume > 128) ? 128 : volume;
    Mix_VolumeMusic(ctx->music_volume);
}

void audio_set_sfx_volume(AudioContext* ctx, int volume) {
    if (!ctx->initialized) return;

    ctx->sfx_volume = (volume < 0) ? 0 : (volume > 128) ? 128 : volume;
    /* Note: Individual chunk volumes set at load time */
}
