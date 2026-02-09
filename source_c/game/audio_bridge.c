/*
 * audio_bridge.c - Audio Bridge Implementation
 *
 * Triggers sounds based on game events using legacy SDL_mixer.
 */

#include "../../include_c/game/audio_bridge.h"
#include "../../include_c/core/constants.h"
#include <SDL_mixer.h>
#include <stdlib.h>

/*============================================================================
 * EXTERNAL SOUND RESOURCES (from main.c)
 *============================================================================*/

extern Mix_Chunk* snd_effect_hit;
extern Mix_Chunk* snd_effect_explosion[5];
extern Mix_Chunk* snd_effect_evil_laugh;

/* Sound counters for looping control (from main.c) */
extern int snd_engines_counter;
extern int snd_weapon_counter;

/* Engine and weapon sounds need to be loaded */
static Mix_Chunk* snd_effect_engines = NULL;
static Mix_Chunk* snd_effect_weapon = NULL;
static int sounds_loaded = 0;

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

void audio_bridge_init(AudioBridgeState* state) {
    state->weapon_firing = 0;
    state->engine_thrusting = 0;
    state->last_explosion_frame = -100;
    state->last_hit_frame = -100;
    state->current_level_music = -1;
    state->music_playing = 0;

    /* Load sounds if not already loaded */
    if (!sounds_loaded) {
        snd_effect_engines = Mix_LoadWAV("./sound/engines.wav");
        snd_effect_weapon = Mix_LoadWAV("./sound/weapon.wav");

        if (snd_effect_weapon) {
            Mix_VolumeChunk(snd_effect_weapon, 80);
        }

        sounds_loaded = 1;
    }
}

/*============================================================================
 * PER-FRAME UPDATE
 *============================================================================*/

void audio_bridge_update(AudioBridgeState* state, const Game* game, const InputState* input) {
    if (!state || !game || !input) return;

    /* Only play weapon/engine sounds during gameplay */
    if (game->state != STATE_PLAYING) {
        /* Stop any playing sounds when not in gameplay */
        if (state->weapon_firing) {
            Mix_HaltChannel(1);
            state->weapon_firing = 0;
        }
        if (state->engine_thrusting) {
            Mix_HaltChannel(3);
            state->engine_thrusting = 0;
        }
        return;
    }

    /* Handle weapon sound */
    if (input->fire && game->player.health > 0) {
        if (!state->weapon_firing && snd_effect_weapon) {
            Mix_PlayChannelTimed(1, snd_effect_weapon, -1, -1);
            state->weapon_firing = 1;
        }
    } else {
        if (state->weapon_firing) {
            Mix_HaltChannel(1);
            state->weapon_firing = 0;
        }
    }

    /* Handle engine sound */
    if (input->up && game->player.health > 0) {
        if (!state->engine_thrusting && snd_effect_engines) {
            Mix_PlayChannelTimed(3, snd_effect_engines, -1, -1);
            state->engine_thrusting = 1;
        }
    } else {
        if (state->engine_thrusting) {
            Mix_HaltChannel(3);
            state->engine_thrusting = 0;
        }
    }

    /* Stop sounds if player dead */
    if (game->player.health <= 0) {
        if (state->weapon_firing) {
            Mix_HaltChannel(1);
            state->weapon_firing = 0;
        }
        if (state->engine_thrusting) {
            Mix_HaltChannel(3);
            state->engine_thrusting = 0;
        }
    }
}

/*============================================================================
 * EVENT-BASED TRIGGERS
 *============================================================================*/

void audio_bridge_player_hit(AudioBridgeState* state) {
    (void)state;  /* Could use for debouncing */
    if (snd_effect_hit) {
        Mix_PlayChannel(-1, snd_effect_hit, 0);
    }
}

void audio_bridge_enemy_destroyed(AudioBridgeState* state) {
    (void)state;

    /* Play random explosion sound */
    int which = rand() % 5;
    if (snd_effect_explosion[which]) {
        Mix_PlayChannel(-1, snd_effect_explosion[which], 0);
    }
}

void audio_bridge_nuke_dropped(AudioBridgeState* state) {
    (void)state;

    /* Play the biggest explosion sound */
    if (snd_effect_explosion[4]) {
        Mix_PlayChannel(-1, snd_effect_explosion[4], 0);
    }
}

void audio_bridge_level_start(AudioBridgeState* state, int level_index) {
    if (!state) return;

    /* Music file paths by level */
    const char* music_files[] = {
        "./sound/sound_track1.ogg",  /* Shire */
        "./sound/sound_track2.ogg",  /* Archipelago */
        "./sound/sound_track3.ogg",  /* Dune */
        "./sound/sound_track4.ogg",  /* Midkemia */
        "./sound/sound_track5.ogg",  /* Oceania */
        "./sound/sound_track6.ogg"   /* Mordor */
    };

    if (level_index >= 0 && level_index < 6) {
        /* Only change if different level */
        if (state->current_level_music != level_index) {
            Mix_Music* music = Mix_LoadMUS(music_files[level_index]);
            if (music) {
                Mix_VolumeMusic(110);
                Mix_PlayMusic(music, 1);
                state->current_level_music = level_index;
                state->music_playing = 1;
            }
        }
    }
}

void audio_bridge_victory(AudioBridgeState* state) {
    (void)state;
    /* Victory doesn't have a specific sound in original */
}

void audio_bridge_defeat(AudioBridgeState* state) {
    (void)state;

    /* Play evil laugh on defeat */
    if (snd_effect_evil_laugh) {
        Mix_PlayChannel(-1, snd_effect_evil_laugh, 0);
    }
}

/*============================================================================
 * MUSIC CONTROL
 *============================================================================*/

void audio_bridge_menu_music(AudioBridgeState* state) {
    if (!state) return;

    Mix_Music* music = Mix_LoadMUS("./sound/menu_theme.ogg");
    if (music) {
        Mix_VolumeMusic(110);
        Mix_PlayMusic(music, 1);
        state->current_level_music = -1;
        state->music_playing = 1;
    }
}

void audio_bridge_stop_music(AudioBridgeState* state) {
    if (!state) return;

    Mix_HaltMusic();
    state->music_playing = 0;
}

void audio_bridge_fade_music(AudioBridgeState* state, int fade_ms) {
    if (!state) return;

    Mix_FadeOutMusic(fade_ms);
    state->music_playing = 0;
}
