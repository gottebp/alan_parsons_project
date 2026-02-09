/*
 * audio_bridge_stub.c - Stub implementation for headless testing
 *
 * Provides no-op implementations of audio_bridge functions
 * so that bridge.c can link without SDL_mixer.
 */

#include "../../include_c/game/audio_bridge.h"

void audio_bridge_init(AudioBridgeState* state) {
    if (state) {
        state->weapon_firing = 0;
        state->engine_thrusting = 0;
    }
}

void audio_bridge_update(AudioBridgeState* state, const Game* game, const InputState* input) {
    (void)state;
    (void)game;
    (void)input;
}

void audio_bridge_level_start(AudioBridgeState* state, int level_index) {
    (void)state;
    (void)level_index;
}

void audio_bridge_enemy_destroyed(AudioBridgeState* state) {
    (void)state;
}

void audio_bridge_player_hit(AudioBridgeState* state) {
    (void)state;
}

void audio_bridge_defeat(AudioBridgeState* state) {
    (void)state;
}
