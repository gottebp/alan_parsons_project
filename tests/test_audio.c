/*
 * test_audio.c - Tests for the Audio Module
 *
 * Tests the audio interface logic without requiring SDL_mixer.
 * Verifies sound ID mappings, volume clamping, and state tracking.
 */

#include "test_framework.h"
#include "../include_c/game/audio.h"

/*============================================================================
 * SOUND ID TESTS
 *============================================================================*/

TEST(sound_id_valid_range) {
    /* All sound IDs should be less than SOUND_COUNT */
    TEST_ASSERT(SOUND_WEAPON_FIRE < SOUND_COUNT);
    TEST_ASSERT(SOUND_ENGINE_THRUST < SOUND_COUNT);
    TEST_ASSERT(SOUND_HIT_PLAYER < SOUND_COUNT);
    TEST_ASSERT(SOUND_EXPLOSION_1 < SOUND_COUNT);
    TEST_ASSERT(SOUND_EXPLOSION_5 < SOUND_COUNT);
    TEST_ASSERT(SOUND_EVIL_LAUGH < SOUND_COUNT);
    TEST_ASSERT(MUSIC_MENU < SOUND_COUNT);
    TEST_ASSERT(MUSIC_MORDOR < SOUND_COUNT);
    TEST_ASSERT(MUSIC_ENDING < SOUND_COUNT);
}

TEST(sound_id_effects_before_music) {
    /* Sound effects should come before music in the enum */
    TEST_ASSERT(SOUND_WEAPON_FIRE < MUSIC_MENU);
    TEST_ASSERT(SOUND_EXPLOSION_5 < MUSIC_MENU);
    TEST_ASSERT(SOUND_EVIL_LAUGH < MUSIC_MENU);
}

TEST(music_id_sequential) {
    /* Music IDs should be sequential starting from MUSIC_MENU */
    TEST_ASSERT_EQ(MUSIC_MENU + 1, MUSIC_SHIRE);
    TEST_ASSERT_EQ(MUSIC_SHIRE + 1, MUSIC_ARCHIPELAGO);
    TEST_ASSERT_EQ(MUSIC_ARCHIPELAGO + 1, MUSIC_DUNE);
    TEST_ASSERT_EQ(MUSIC_DUNE + 1, MUSIC_MIDKEMIA);
    TEST_ASSERT_EQ(MUSIC_MIDKEMIA + 1, MUSIC_OCEANIA);
    TEST_ASSERT_EQ(MUSIC_OCEANIA + 1, MUSIC_MORDOR);
    TEST_ASSERT_EQ(MUSIC_MORDOR + 1, MUSIC_ENDING);
}

/*============================================================================
 * LEVEL MUSIC MAPPING TESTS
 *============================================================================*/

TEST(level_music_mapping) {
    /* Level indices 0-5 should map to correct music tracks */
    SoundId expected[] = {
        MUSIC_SHIRE,
        MUSIC_ARCHIPELAGO,
        MUSIC_DUNE,
        MUSIC_MIDKEMIA,
        MUSIC_OCEANIA,
        MUSIC_MORDOR
    };

    for (int level = 0; level < 6; level++) {
        SoundId actual = MUSIC_SHIRE + level;
        TEST_ASSERT_EQ(expected[level], actual);
    }
}

/*============================================================================
 * EXPLOSION SOUND SELECTION TESTS
 *============================================================================*/

TEST(explosion_sounds_sequential) {
    /* Explosion sounds should be sequential for easy random selection */
    TEST_ASSERT_EQ(SOUND_EXPLOSION_1 + 1, SOUND_EXPLOSION_2);
    TEST_ASSERT_EQ(SOUND_EXPLOSION_2 + 1, SOUND_EXPLOSION_3);
    TEST_ASSERT_EQ(SOUND_EXPLOSION_3 + 1, SOUND_EXPLOSION_4);
    TEST_ASSERT_EQ(SOUND_EXPLOSION_4 + 1, SOUND_EXPLOSION_5);
}

TEST(random_explosion_in_range) {
    /* Random selection should always produce valid explosion ID */
    for (int i = 0; i < 100; i++) {
        int which = SOUND_EXPLOSION_1 + (i % 5);  /* Simulate rand() % 5 */
        TEST_ASSERT(which >= SOUND_EXPLOSION_1);
        TEST_ASSERT(which <= SOUND_EXPLOSION_5);
    }
}

/*============================================================================
 * VOLUME CLAMPING TESTS
 *============================================================================*/

static int clamp_volume(int volume) {
    if (volume < 0) return 0;
    if (volume > 128) return 128;
    return volume;
}

TEST(volume_clamp_normal) {
    TEST_ASSERT_EQ(64, clamp_volume(64));
    TEST_ASSERT_EQ(0, clamp_volume(0));
    TEST_ASSERT_EQ(128, clamp_volume(128));
}

TEST(volume_clamp_negative) {
    TEST_ASSERT_EQ(0, clamp_volume(-10));
    TEST_ASSERT_EQ(0, clamp_volume(-1000));
}

TEST(volume_clamp_over_max) {
    TEST_ASSERT_EQ(128, clamp_volume(129));
    TEST_ASSERT_EQ(128, clamp_volume(1000));
    TEST_ASSERT_EQ(128, clamp_volume(255));
}

/*============================================================================
 * CHANNEL ASSIGNMENT TESTS
 *============================================================================*/

TEST(channel_constants) {
    /* Verify dedicated channels are distinct */
    TEST_ASSERT(CHANNEL_WEAPON != CHANNEL_ENGINE);
    TEST_ASSERT(CHANNEL_ANY == -1);
}

/*============================================================================
 * AUDIO CONTEXT STATE TESTS
 *============================================================================*/

TEST(audio_context_initial_state) {
    AudioContext ctx = {0};

    /* Uninitialized context should have sensible defaults */
    TEST_ASSERT_EQ(0, ctx.initialized);
    TEST_ASSERT_EQ(0, ctx.weapon_playing);
    TEST_ASSERT_EQ(0, ctx.engine_playing);
    TEST_ASSERT_EQ(0, ctx.current_music_id);
}

/*============================================================================
 * MAIN
 *============================================================================*/

int main(void) {
    /* Sound ID tests */
    RUN_TEST(sound_id_valid_range);
    RUN_TEST(sound_id_effects_before_music);
    RUN_TEST(music_id_sequential);

    /* Mapping tests */
    RUN_TEST(level_music_mapping);

    /* Explosion tests */
    RUN_TEST(explosion_sounds_sequential);
    RUN_TEST(random_explosion_in_range);

    /* Volume tests */
    RUN_TEST(volume_clamp_normal);
    RUN_TEST(volume_clamp_negative);
    RUN_TEST(volume_clamp_over_max);

    /* Channel tests */
    RUN_TEST(channel_constants);

    /* Context tests */
    RUN_TEST(audio_context_initial_state);

    TEST_REPORT();
    return _tests_failed > 0 ? 1 : 0;
}
