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
 * AUDIO CONTEXT - Set by audio_bridge_set_context()
 *============================================================================*/

static Mix_Chunk* ab_snd_hit = NULL;
static Mix_Chunk* ab_snd_explosion[5] = {NULL, NULL, NULL, NULL, NULL};
static Mix_Chunk* ab_snd_evil_laugh = NULL;
static Mix_Chunk* ab_snd_engines = NULL;
static Mix_Chunk* ab_snd_weapon = NULL;
static int ab_initialized = 0;

void audio_bridge_set_context(void* hit, void* evil_laugh, void* explosion[5], void* engines, void* weapon) {
    ab_snd_hit = (Mix_Chunk*)hit;
    ab_snd_evil_laugh = (Mix_Chunk*)evil_laugh;
    ab_snd_engines = (Mix_Chunk*)engines;
    ab_snd_weapon = (Mix_Chunk*)weapon;
    for (int i = 0; i < 5; i++) {
        ab_snd_explosion[i] = explosion ? (Mix_Chunk*)explosion[i] : NULL;
    }
    ab_initialized = 1;
}

/* Legacy fallback (for compatibility during transition) */
extern Mix_Chunk* snd_effect_hit;
extern Mix_Chunk* snd_effect_explosion[5];
extern Mix_Chunk* snd_effect_evil_laugh;

static Mix_Chunk* get_snd_hit(void) { return ab_initialized ? ab_snd_hit : snd_effect_hit; }
static Mix_Chunk* get_snd_evil_laugh(void) { return ab_initialized ? ab_snd_evil_laugh : snd_effect_evil_laugh; }
static Mix_Chunk* get_snd_explosion(int i) { return ab_initialized ? ab_snd_explosion[i] : snd_effect_explosion[i]; }
static Mix_Chunk* get_snd_engines(void) { return ab_snd_engines; }
static Mix_Chunk* get_snd_weapon(void) { return ab_snd_weapon; }

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
    state->current_music = NULL;

    /* Load engine/weapon sounds if context not set and not already loaded */
    if (!ab_initialized && !sounds_loaded) {
        ab_snd_engines = Mix_LoadWAV("./sound/engines.wav");
        ab_snd_weapon = Mix_LoadWAV("./sound/weapon.wav");

        if (ab_snd_weapon) {
            Mix_VolumeChunk(ab_snd_weapon, 80);
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
        Mix_Chunk* weapon = get_snd_weapon();
        if (!state->weapon_firing && weapon) {
            Mix_PlayChannelTimed(1, weapon, -1, -1);
            state->weapon_firing = 1;
        }
    } else {
        if (state->weapon_firing) {
            Mix_HaltChannel(1);
            state->weapon_firing = 0;
        }
    }

    /* Handle engine sound (keyboard up OR mobile stick thrust) */
    int thrusting = input->up || (input->mobile_active && input->stick_left_y < -0.01f);
    if (thrusting && game->player.health > 0) {
        Mix_Chunk* engines = get_snd_engines();
        if (!state->engine_thrusting && engines) {
            Mix_PlayChannelTimed(3, engines, -1, -1);
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
    Mix_Chunk* hit = get_snd_hit();
    if (hit) {
        Mix_PlayChannel(-1, hit, 0);
    }
}

void audio_bridge_enemy_destroyed(AudioBridgeState* state) {
    (void)state;

    /* Play random explosion sound */
    int which = rand() % 5;
    Mix_Chunk* explosion = get_snd_explosion(which);
    if (explosion) {
        Mix_PlayChannel(-1, explosion, 0);
    }
}

void audio_bridge_nuke_dropped(AudioBridgeState* state) {
    (void)state;

    /* Play the biggest explosion sound */
    Mix_Chunk* explosion = get_snd_explosion(4);
    if (explosion) {
        Mix_PlayChannel(-1, explosion, 0);
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
                Mix_HaltMusic();
                if (state->current_music) {
                    Mix_FreeMusic((Mix_Music*)state->current_music);
                }
                state->current_music = music;
                Mix_VolumeMusic(110);
                Mix_PlayMusic(music, 1);
                state->current_level_music = level_index;
                state->music_playing = 1;
            }
        }
    }
}

void audio_bridge_boss_spawned(AudioBridgeState* state) {
    (void)state;

    /* Play evil laugh when boss appears */
    Mix_Chunk* laugh = get_snd_evil_laugh();
    if (laugh) {
        Mix_PlayChannel(-1, laugh, 0);
    }
}

void audio_bridge_victory(AudioBridgeState* state) {
    (void)state;
    /* Victory doesn't have a specific sound in original */
}

void audio_bridge_defeat(AudioBridgeState* state) {
    (void)state;

    /* Play evil laugh on defeat */
    Mix_Chunk* laugh = get_snd_evil_laugh();
    if (laugh) {
        Mix_PlayChannel(-1, laugh, 0);
    }
}

/*============================================================================
 * MUSIC CONTROL
 *============================================================================*/

void audio_bridge_menu_music(AudioBridgeState* state) {
    if (!state) return;

    Mix_Music* music = Mix_LoadMUS("./sound/menu_theme.ogg");
    if (music) {
        Mix_HaltMusic();
        if (state->current_music) {
            Mix_FreeMusic((Mix_Music*)state->current_music);
        }
        state->current_music = music;
        Mix_VolumeMusic(110);
        Mix_PlayMusic(music, 1);
        state->current_level_music = -1;
        state->music_playing = 1;
    }
}

void audio_bridge_stop_music(AudioBridgeState* state) {
    if (!state) return;

    Mix_HaltMusic();
    if (state->current_music) {
        Mix_FreeMusic((Mix_Music*)state->current_music);
        state->current_music = NULL;
    }
    state->music_playing = 0;
}

void audio_bridge_fade_music(AudioBridgeState* state, int fade_ms) {
    if (!state) return;

    Mix_FadeOutMusic(fade_ms);
    state->music_playing = 0;
}
