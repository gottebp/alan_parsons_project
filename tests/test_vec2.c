/*
 * test_vec2.c - Tests for the 2D vector library
 */

#include "test_framework.h"
#include "../include_c/math/vec2.h"

#define EPSILON 0.0001f

/*============================================================================
 * CONSTRUCTION TESTS
 *============================================================================*/

TEST(vec2_construction) {
    Vec2 v = vec2(3.0f, 4.0f);
    TEST_ASSERT_FLOAT_EQ(3.0f, v.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(4.0f, v.y, EPSILON);
}

TEST(vec2_zero) {
    Vec2 v = vec2_zero();
    TEST_ASSERT_FLOAT_EQ(0.0f, v.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(0.0f, v.y, EPSILON);
}

TEST(vec2_one) {
    Vec2 v = vec2_one();
    TEST_ASSERT_FLOAT_EQ(1.0f, v.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(1.0f, v.y, EPSILON);
}

/*============================================================================
 * ARITHMETIC TESTS
 *============================================================================*/

TEST(vec2_add) {
    Vec2 a = vec2(1.0f, 2.0f);
    Vec2 b = vec2(3.0f, 4.0f);
    Vec2 c = vec2_add(a, b);
    TEST_ASSERT_FLOAT_EQ(4.0f, c.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(6.0f, c.y, EPSILON);
}

TEST(vec2_sub) {
    Vec2 a = vec2(5.0f, 7.0f);
    Vec2 b = vec2(2.0f, 3.0f);
    Vec2 c = vec2_sub(a, b);
    TEST_ASSERT_FLOAT_EQ(3.0f, c.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(4.0f, c.y, EPSILON);
}

TEST(vec2_mul) {
    Vec2 v = vec2(3.0f, 4.0f);
    Vec2 r = vec2_mul(v, 2.0f);
    TEST_ASSERT_FLOAT_EQ(6.0f, r.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(8.0f, r.y, EPSILON);
}

TEST(vec2_div) {
    Vec2 v = vec2(6.0f, 8.0f);
    Vec2 r = vec2_div(v, 2.0f);
    TEST_ASSERT_FLOAT_EQ(3.0f, r.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(4.0f, r.y, EPSILON);
}

TEST(vec2_neg) {
    Vec2 v = vec2(3.0f, -4.0f);
    Vec2 r = vec2_neg(v);
    TEST_ASSERT_FLOAT_EQ(-3.0f, r.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(4.0f, r.y, EPSILON);
}

/*============================================================================
 * PRODUCT TESTS
 *============================================================================*/

TEST(vec2_dot) {
    Vec2 a = vec2(1.0f, 2.0f);
    Vec2 b = vec2(3.0f, 4.0f);
    float d = vec2_dot(a, b);
    TEST_ASSERT_FLOAT_EQ(11.0f, d, EPSILON);  /* 1*3 + 2*4 = 11 */
}

TEST(vec2_cross) {
    Vec2 a = vec2(1.0f, 0.0f);
    Vec2 b = vec2(0.0f, 1.0f);
    float c = vec2_cross(a, b);
    TEST_ASSERT_FLOAT_EQ(1.0f, c, EPSILON);  /* 1*1 - 0*0 = 1 */
}

/*============================================================================
 * LENGTH AND DISTANCE TESTS
 *============================================================================*/

TEST(vec2_length) {
    Vec2 v = vec2(3.0f, 4.0f);
    float len = vec2_length(v);
    TEST_ASSERT_FLOAT_EQ(5.0f, len, EPSILON);  /* 3-4-5 triangle */
}

TEST(vec2_length_sq) {
    Vec2 v = vec2(3.0f, 4.0f);
    float len_sq = vec2_length_sq(v);
    TEST_ASSERT_FLOAT_EQ(25.0f, len_sq, EPSILON);
}

TEST(vec2_distance) {
    Vec2 a = vec2(0.0f, 0.0f);
    Vec2 b = vec2(3.0f, 4.0f);
    float d = vec2_distance(a, b);
    TEST_ASSERT_FLOAT_EQ(5.0f, d, EPSILON);
}

/*============================================================================
 * NORMALIZATION TESTS
 *============================================================================*/

TEST(vec2_normalize) {
    Vec2 v = vec2(3.0f, 4.0f);
    Vec2 n = vec2_normalize(v);
    TEST_ASSERT_FLOAT_EQ(0.6f, n.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(0.8f, n.y, EPSILON);

    /* Length should be 1 */
    float len = vec2_length(n);
    TEST_ASSERT_FLOAT_EQ(1.0f, len, EPSILON);
}

TEST(vec2_normalize_zero) {
    Vec2 v = vec2(0.0f, 0.0f);
    Vec2 n = vec2_normalize(v);
    /* Should return zero vector, not crash */
    TEST_ASSERT_FLOAT_EQ(0.0f, n.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(0.0f, n.y, EPSILON);
}

TEST(vec2_clamp_length) {
    Vec2 v = vec2(6.0f, 8.0f);  /* Length = 10 */
    Vec2 c = vec2_clamp_length(v, 5.0f);
    float len = vec2_length(c);
    TEST_ASSERT_FLOAT_EQ(5.0f, len, EPSILON);
}

TEST(vec2_clamp_length_no_change) {
    Vec2 v = vec2(3.0f, 4.0f);  /* Length = 5 */
    Vec2 c = vec2_clamp_length(v, 10.0f);  /* Max is higher */
    TEST_ASSERT_FLOAT_EQ(3.0f, c.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(4.0f, c.y, EPSILON);
}

/*============================================================================
 * ANGLE TESTS (256-angle system)
 *============================================================================*/

TEST(vec2_from_angle_right) {
    Vec2 v = vec2_from_angle(0);  /* 0 = right */
    TEST_ASSERT_FLOAT_EQ(1.0f, v.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(0.0f, v.y, 0.01f);
}

TEST(vec2_from_angle_down) {
    Vec2 v = vec2_from_angle(64);  /* 64 = down (90 degrees) */
    TEST_ASSERT_FLOAT_EQ(0.0f, v.x, 0.01f);
    TEST_ASSERT_FLOAT_EQ(1.0f, v.y, 0.01f);
}

TEST(vec2_from_angle_left) {
    Vec2 v = vec2_from_angle(128);  /* 128 = left (180 degrees) */
    TEST_ASSERT_FLOAT_EQ(-1.0f, v.x, 0.01f);
    TEST_ASSERT_FLOAT_EQ(0.0f, v.y, 0.01f);
}

TEST(vec2_from_angle_up) {
    Vec2 v = vec2_from_angle(192);  /* 192 = up (270 degrees) */
    TEST_ASSERT_FLOAT_EQ(0.0f, v.x, 0.01f);
    TEST_ASSERT_FLOAT_EQ(-1.0f, v.y, 0.01f);
}

TEST(vec2_to_angle_right) {
    Vec2 v = vec2(1.0f, 0.0f);
    uint8_t a = vec2_to_angle(v);
    TEST_ASSERT_EQ(0, a);
}

TEST(vec2_to_angle_down) {
    Vec2 v = vec2(0.0f, 1.0f);
    uint8_t a = vec2_to_angle(v);
    TEST_ASSERT_EQ(64, a);
}

TEST(vec2_perp) {
    Vec2 v = vec2(1.0f, 0.0f);
    Vec2 p = vec2_perp(v);
    TEST_ASSERT_FLOAT_EQ(0.0f, p.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(1.0f, p.y, EPSILON);
}

/*============================================================================
 * TOROIDAL MAP TESTS (crucial for the game)
 *============================================================================*/

TEST(vec2_wrap_no_change) {
    Vec2 v = vec2(100.0f, 200.0f);
    Vec2 w = vec2_wrap(v, 3200.0f, 2400.0f);
    TEST_ASSERT_FLOAT_EQ(100.0f, w.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(200.0f, w.y, EPSILON);
}

TEST(vec2_wrap_positive) {
    Vec2 v = vec2(3300.0f, 2500.0f);  /* Over the edge */
    Vec2 w = vec2_wrap(v, 3200.0f, 2400.0f);
    TEST_ASSERT_FLOAT_EQ(100.0f, w.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(100.0f, w.y, EPSILON);
}

TEST(vec2_wrap_negative) {
    Vec2 v = vec2(-100.0f, -200.0f);  /* Under zero */
    Vec2 w = vec2_wrap(v, 3200.0f, 2400.0f);
    TEST_ASSERT_FLOAT_EQ(3100.0f, w.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(2200.0f, w.y, EPSILON);
}

TEST(vec2_toroidal_delta_direct) {
    /* Both on same side, should be direct path */
    Vec2 from = vec2(100.0f, 100.0f);
    Vec2 to = vec2(200.0f, 200.0f);
    Vec2 d = vec2_toroidal_delta(from, to, 3200.0f, 2400.0f);
    TEST_ASSERT_FLOAT_EQ(100.0f, d.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(100.0f, d.y, EPSILON);
}

TEST(vec2_toroidal_delta_wrap_x) {
    /* Closer to wrap around X */
    Vec2 from = vec2(3100.0f, 100.0f);
    Vec2 to = vec2(100.0f, 100.0f);
    Vec2 d = vec2_toroidal_delta(from, to, 3200.0f, 2400.0f);
    /* Should go +200, not -3000 */
    TEST_ASSERT_FLOAT_EQ(200.0f, d.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(0.0f, d.y, EPSILON);
}

TEST(vec2_toroidal_delta_wrap_y) {
    /* Closer to wrap around Y */
    Vec2 from = vec2(100.0f, 2300.0f);
    Vec2 to = vec2(100.0f, 100.0f);
    Vec2 d = vec2_toroidal_delta(from, to, 3200.0f, 2400.0f);
    /* Should go +200, not -2200 */
    TEST_ASSERT_FLOAT_EQ(0.0f, d.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(200.0f, d.y, EPSILON);
}

TEST(vec2_toroidal_distance) {
    Vec2 a = vec2(3100.0f, 100.0f);
    Vec2 b = vec2(100.0f, 100.0f);
    float d = vec2_toroidal_distance(a, b, 3200.0f, 2400.0f);
    TEST_ASSERT_FLOAT_EQ(200.0f, d, EPSILON);  /* Not 3000! */
}

/*============================================================================
 * INTEGER CONVERSION TESTS
 *============================================================================*/

TEST(vec2_to_int) {
    Vec2 v = vec2(3.7f, 4.2f);
    Vec2i i = vec2_to_int(v);
    TEST_ASSERT_EQ(4, i.x);  /* lrintf rounds to nearest */
    TEST_ASSERT_EQ(4, i.y);
}

TEST(vec2_from_int) {
    Vec2i i = {3, 4};
    Vec2 v = vec2_from_int(i);
    TEST_ASSERT_FLOAT_EQ(3.0f, v.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(4.0f, v.y, EPSILON);
}

/*============================================================================
 * LERP TESTS
 *============================================================================*/

TEST(vec2_lerp_start) {
    Vec2 a = vec2(0.0f, 0.0f);
    Vec2 b = vec2(10.0f, 10.0f);
    Vec2 r = vec2_lerp(a, b, 0.0f);
    TEST_ASSERT_FLOAT_EQ(0.0f, r.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(0.0f, r.y, EPSILON);
}

TEST(vec2_lerp_end) {
    Vec2 a = vec2(0.0f, 0.0f);
    Vec2 b = vec2(10.0f, 10.0f);
    Vec2 r = vec2_lerp(a, b, 1.0f);
    TEST_ASSERT_FLOAT_EQ(10.0f, r.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(10.0f, r.y, EPSILON);
}

TEST(vec2_lerp_middle) {
    Vec2 a = vec2(0.0f, 0.0f);
    Vec2 b = vec2(10.0f, 10.0f);
    Vec2 r = vec2_lerp(a, b, 0.5f);
    TEST_ASSERT_FLOAT_EQ(5.0f, r.x, EPSILON);
    TEST_ASSERT_FLOAT_EQ(5.0f, r.y, EPSILON);
}

/*============================================================================
 * MAIN
 *============================================================================*/

int main(void) {
    TEST_SUITE_BEGIN("Vec2 Tests");

    printf("\n  Construction:\n");
    RUN_TEST(vec2_construction);
    RUN_TEST(vec2_zero);
    RUN_TEST(vec2_one);

    printf("\n  Arithmetic:\n");
    RUN_TEST(vec2_add);
    RUN_TEST(vec2_sub);
    RUN_TEST(vec2_mul);
    RUN_TEST(vec2_div);
    RUN_TEST(vec2_neg);

    printf("\n  Products:\n");
    RUN_TEST(vec2_dot);
    RUN_TEST(vec2_cross);

    printf("\n  Length & Distance:\n");
    RUN_TEST(vec2_length);
    RUN_TEST(vec2_length_sq);
    RUN_TEST(vec2_distance);

    printf("\n  Normalization:\n");
    RUN_TEST(vec2_normalize);
    RUN_TEST(vec2_normalize_zero);
    RUN_TEST(vec2_clamp_length);
    RUN_TEST(vec2_clamp_length_no_change);

    printf("\n  Angles (256-system):\n");
    RUN_TEST(vec2_from_angle_right);
    RUN_TEST(vec2_from_angle_down);
    RUN_TEST(vec2_from_angle_left);
    RUN_TEST(vec2_from_angle_up);
    RUN_TEST(vec2_to_angle_right);
    RUN_TEST(vec2_to_angle_down);
    RUN_TEST(vec2_perp);

    printf("\n  Toroidal Map:\n");
    RUN_TEST(vec2_wrap_no_change);
    RUN_TEST(vec2_wrap_positive);
    RUN_TEST(vec2_wrap_negative);
    RUN_TEST(vec2_toroidal_delta_direct);
    RUN_TEST(vec2_toroidal_delta_wrap_x);
    RUN_TEST(vec2_toroidal_delta_wrap_y);
    RUN_TEST(vec2_toroidal_distance);

    printf("\n  Integer Conversion:\n");
    RUN_TEST(vec2_to_int);
    RUN_TEST(vec2_from_int);

    printf("\n  Interpolation:\n");
    RUN_TEST(vec2_lerp_start);
    RUN_TEST(vec2_lerp_end);
    RUN_TEST(vec2_lerp_middle);

    TEST_SUITE_END();
    TEST_REPORT();

    return TEST_EXIT_CODE();
}
