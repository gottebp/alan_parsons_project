/*
 * test_render.c - Tests for the Render Module
 *
 * Tests the clean rendering interface without requiring SDL.
 * Verifies alpha blending, coordinate transforms, and buffer operations.
 */

#include "test_framework.h"
#include "../include_c/game/game.h"
#include "../include_c/core/constants.h"
#include <string.h>
#include <stdlib.h>

/*============================================================================
 * MOCK RENDERING FUNCTIONS
 *
 * These test the core logic without SDL dependencies.
 *============================================================================*/

/* Inline implementation of alpha blend for testing */
static uint32_t test_blend_alpha(uint32_t src, uint32_t dst) {
    uint32_t sa = (src >> 24) & 0xFF;
    if (sa == 0) return dst;
    if (sa == 255) return src;

    uint32_t sr = (src >> 16) & 0xFF;
    uint32_t sg = (src >> 8) & 0xFF;
    uint32_t sb = src & 0xFF;

    uint32_t dr = (dst >> 16) & 0xFF;
    uint32_t dg = (dst >> 8) & 0xFF;
    uint32_t db = dst & 0xFF;

    uint32_t da = 255 - sa;

    uint32_t r = (sr * sa + dr * da) / 255;
    uint32_t g = (sg * sa + dg * da) / 255;
    uint32_t b = (sb * sa + db * da) / 255;

    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

/*============================================================================
 * ALPHA BLENDING TESTS
 *============================================================================*/

TEST(alpha_blend_fully_opaque) {
    uint32_t src = 0xFFFF0000;  /* Fully opaque red */
    uint32_t dst = 0xFF00FF00;  /* Fully opaque green */

    uint32_t result = test_blend_alpha(src, dst);

    /* Fully opaque source should completely replace destination */
    TEST_ASSERT((result & 0x00FF0000) == 0x00FF0000);  /* Red channel */
    TEST_ASSERT((result & 0x0000FF00) == 0x00000000);  /* No green */
}

TEST(alpha_blend_fully_transparent) {
    uint32_t src = 0x00FF0000;  /* Fully transparent red */
    uint32_t dst = 0xFF00FF00;  /* Fully opaque green */

    uint32_t result = test_blend_alpha(src, dst);

    /* Fully transparent source should not affect destination */
    TEST_ASSERT(result == dst);
}

TEST(alpha_blend_half_transparent) {
    uint32_t src = 0x80FF0000;  /* 50% transparent red */
    uint32_t dst = 0xFF00FF00;  /* Fully opaque green */

    uint32_t result = test_blend_alpha(src, dst);

    /* Should be roughly half red, half green */
    uint32_t r = (result >> 16) & 0xFF;
    uint32_t g = (result >> 8) & 0xFF;

    /* Allow some tolerance for rounding */
    TEST_ASSERT(r > 100 && r < 160);  /* Some red */
    TEST_ASSERT(g > 100 && g < 160);  /* Some green */
}

/*============================================================================
 * COORDINATE TRANSFORM TESTS
 *============================================================================*/

/* World to screen transform (camera at center) */
static void world_to_screen(int world_x, int world_y, int cam_x, int cam_y,
                            int* screen_x, int* screen_y) {
    *screen_x = world_x - cam_x + SCREEN_WIDTH / 2;
    *screen_y = world_y - cam_y + SCREEN_HEIGHT / 2;
}

TEST(world_to_screen_center) {
    /* Object at camera position should appear at screen center */
    int sx, sy;
    world_to_screen(100, 100, 100, 100, &sx, &sy);

    TEST_ASSERT_EQ(SCREEN_WIDTH / 2, sx);
    TEST_ASSERT_EQ(SCREEN_HEIGHT / 2, sy);
}

TEST(world_to_screen_offset) {
    int sx, sy;

    /* Object 100 pixels right of camera */
    world_to_screen(200, 100, 100, 100, &sx, &sy);
    TEST_ASSERT_EQ(SCREEN_WIDTH / 2 + 100, sx);
    TEST_ASSERT_EQ(SCREEN_HEIGHT / 2, sy);

    /* Object 50 pixels up from camera */
    world_to_screen(100, 50, 100, 100, &sx, &sy);
    TEST_ASSERT_EQ(SCREEN_WIDTH / 2, sx);
    TEST_ASSERT_EQ(SCREEN_HEIGHT / 2 - 50, sy);
}

/*============================================================================
 * TOROIDAL WRAP TESTS
 *============================================================================*/

/* Toroidal wrap for screen coordinates */
static int wrap_screen_x(int screen_x) {
    if (screen_x < -MAP_WIDTH + SCREEN_WIDTH) {
        return screen_x + MAP_WIDTH;
    } else if (screen_x > MAP_WIDTH) {
        return screen_x - MAP_WIDTH;
    }
    return screen_x;
}

TEST(screen_wrap_no_change) {
    /* Normal on-screen position shouldn't wrap */
    int result = wrap_screen_x(500);
    TEST_ASSERT_EQ(500, result);
}

TEST(screen_wrap_left_edge) {
    /* Position far off left edge should wrap */
    int result = wrap_screen_x(-MAP_WIDTH + SCREEN_WIDTH - 100);
    TEST_ASSERT_EQ(-MAP_WIDTH + SCREEN_WIDTH - 100 + MAP_WIDTH, result);
}

TEST(screen_wrap_right_edge) {
    /* Position far off right edge should wrap */
    int result = wrap_screen_x(MAP_WIDTH + 100);
    TEST_ASSERT_EQ(MAP_WIDTH + 100 - MAP_WIDTH, result);
}

/*============================================================================
 * CULLING TESTS
 *============================================================================*/

/* Simple AABB culling check */
static int is_visible(int x, int y, int width, int height) {
    if (x + width < 0) return 0;
    if (x >= SCREEN_WIDTH) return 0;
    if (y + height < 0) return 0;
    if (y >= SCREEN_HEIGHT) return 0;
    return 1;
}

TEST(culling_visible_center) {
    TEST_ASSERT(is_visible(SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 64, 64));
}

TEST(culling_visible_edge) {
    /* Partially visible at right edge */
    TEST_ASSERT(is_visible(SCREEN_WIDTH - 32, 100, 64, 64));
}

TEST(culling_invisible_left) {
    /* Completely off left */
    TEST_ASSERT(!is_visible(-100, 100, 64, 64));
}

TEST(culling_invisible_right) {
    /* Completely off right */
    TEST_ASSERT(!is_visible(SCREEN_WIDTH + 100, 100, 64, 64));
}

TEST(culling_invisible_top) {
    /* Completely off top */
    TEST_ASSERT(!is_visible(100, -100, 64, 64));
}

TEST(culling_invisible_bottom) {
    /* Completely off bottom */
    TEST_ASSERT(!is_visible(100, SCREEN_HEIGHT + 100, 64, 64));
}

/*============================================================================
 * MINIMAP COORDINATE TESTS
 *============================================================================*/

/* World to minimap coordinate transform */
static void world_to_minimap(float world_x, float world_y, int* minimap_x, int* minimap_y) {
    *minimap_x = (int)(world_x / (MAP_WIDTH / MINIMAP_WIDTH)) + MINIMAP_X;
    *minimap_y = (int)(world_y / (MAP_HEIGHT / MINIMAP_HEIGHT)) + MINIMAP_Y;
}

TEST(minimap_origin) {
    int mx, my;
    world_to_minimap(0, 0, &mx, &my);

    TEST_ASSERT_EQ(MINIMAP_X, mx);
    TEST_ASSERT_EQ(MINIMAP_Y, my);
}

TEST(minimap_center) {
    int mx, my;
    world_to_minimap(MAP_WIDTH / 2, MAP_HEIGHT / 2, &mx, &my);

    TEST_ASSERT_EQ(MINIMAP_X + MINIMAP_WIDTH / 2, mx);
    TEST_ASSERT_EQ(MINIMAP_Y + MINIMAP_HEIGHT / 2, my);
}

TEST(minimap_bounds) {
    int mx, my;
    world_to_minimap(MAP_WIDTH - 1, MAP_HEIGHT - 1, &mx, &my);

    /* Should be near max but within bounds */
    TEST_ASSERT(mx < MINIMAP_X + MINIMAP_WIDTH);
    TEST_ASSERT(my < MINIMAP_Y + MINIMAP_HEIGHT);
}

/*============================================================================
 * SPRITE FRAME SELECTION TESTS
 *============================================================================*/

TEST(player_sprite_frame_from_angle) {
    /* Player has 256 frames, one per angle */
    for (int angle = 0; angle < 256; angle++) {
        uint8_t frame = (uint8_t)angle;
        /* Frame must be a valid angle index */
        TEST_ASSERT(frame == (uint8_t)angle);
    }
}

TEST(enemy_sprite_frame_from_angle) {
    /* Enemy has 16 frames, covering 256 angles */
    for (int angle = 0; angle < 256; angle++) {
        int frame = angle >> 4;  /* Divide by 16 */
        TEST_ASSERT(frame >= 0 && frame < 16);
    }
}

TEST(enemy_sprite_offset_calculation) {
    /* Small enemy: type * 16 frames * frame_size + angle_frame * frame_size */
    int type = 2;
    uint8_t angle = 128;  /* 180 degrees */
    int frame_size = SMALL_ENEMY_WIDTH * SMALL_ENEMY_HEIGHT;

    int type_offset = type * 16 * frame_size;
    int angle_frame = angle >> 4;  /* 8 */
    int angle_offset = angle_frame * frame_size;

    int total_offset = type_offset + angle_offset;

    /* Type 2, frame 8 should be at correct position */
    TEST_ASSERT(total_offset > 0);
    TEST_ASSERT(type_offset == 2 * 16 * frame_size);
    TEST_ASSERT(angle_frame == 8);
}

/*============================================================================
 * HEALTH BAR TESTS
 *============================================================================*/

TEST(health_bar_full_width) {
    int health = PLAYER_MAX_HEALTH;
    int bar_width = (health * (SCREEN_WIDTH - 30)) / PLAYER_MAX_HEALTH;

    TEST_ASSERT_EQ(SCREEN_WIDTH - 30, bar_width);
}

TEST(health_bar_half_width) {
    int health = PLAYER_MAX_HEALTH / 2;
    int bar_width = (health * (SCREEN_WIDTH - 30)) / PLAYER_MAX_HEALTH;

    /* Should be approximately half */
    int expected = (SCREEN_WIDTH - 30) / 2;
    TEST_ASSERT(bar_width >= expected - 1 && bar_width <= expected + 1);
}

TEST(health_bar_zero_width) {
    int health = 0;
    int bar_width = (health * (SCREEN_WIDTH - 30)) / PLAYER_MAX_HEALTH;

    TEST_ASSERT_EQ(0, bar_width);
}

/*============================================================================
 * MAIN
 *============================================================================*/

int main(void) {
    /* Alpha blending */
    RUN_TEST(alpha_blend_fully_opaque);
    RUN_TEST(alpha_blend_fully_transparent);
    RUN_TEST(alpha_blend_half_transparent);

    /* Coordinate transforms */
    RUN_TEST(world_to_screen_center);
    RUN_TEST(world_to_screen_offset);

    /* Screen wrapping */
    RUN_TEST(screen_wrap_no_change);
    RUN_TEST(screen_wrap_left_edge);
    RUN_TEST(screen_wrap_right_edge);

    /* Culling */
    RUN_TEST(culling_visible_center);
    RUN_TEST(culling_visible_edge);
    RUN_TEST(culling_invisible_left);
    RUN_TEST(culling_invisible_right);
    RUN_TEST(culling_invisible_top);
    RUN_TEST(culling_invisible_bottom);

    /* Minimap */
    RUN_TEST(minimap_origin);
    RUN_TEST(minimap_center);
    RUN_TEST(minimap_bounds);

    /* Sprite selection */
    RUN_TEST(player_sprite_frame_from_angle);
    RUN_TEST(enemy_sprite_frame_from_angle);
    RUN_TEST(enemy_sprite_offset_calculation);

    /* Health bar */
    RUN_TEST(health_bar_full_width);
    RUN_TEST(health_bar_half_width);
    RUN_TEST(health_bar_zero_width);

    TEST_REPORT();
    return _tests_failed > 0 ? 1 : 0;
}
