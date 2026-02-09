/*
 * vec2.h - 2D Vector Mathematics
 *
 * A simple, elegant 2D vector library. Inline functions for performance,
 * clear names for understanding. Uses the 256-angle system native to this game.
 */

#ifndef VEC2_H
#define VEC2_H

#include <math.h>
#include <stdint.h>

/*
 * The fundamental type: a point or direction in 2D space
 */
typedef struct {
    float x;
    float y;
} Vec2;

/*
 * Construction
 */

static inline Vec2 vec2(float x, float y) {
    return (Vec2){x, y};
}

static inline Vec2 vec2_zero(void) {
    return (Vec2){0.0f, 0.0f};
}

static inline Vec2 vec2_one(void) {
    return (Vec2){1.0f, 1.0f};
}

/*
 * Arithmetic
 */

static inline Vec2 vec2_add(Vec2 a, Vec2 b) {
    return (Vec2){a.x + b.x, a.y + b.y};
}

static inline Vec2 vec2_sub(Vec2 a, Vec2 b) {
    return (Vec2){a.x - b.x, a.y - b.y};
}

static inline Vec2 vec2_mul(Vec2 v, float s) {
    return (Vec2){v.x * s, v.y * s};
}

static inline Vec2 vec2_div(Vec2 v, float s) {
    float inv = 1.0f / s;
    return (Vec2){v.x * inv, v.y * inv};
}

static inline Vec2 vec2_neg(Vec2 v) {
    return (Vec2){-v.x, -v.y};
}

/*
 * Products
 */

static inline float vec2_dot(Vec2 a, Vec2 b) {
    return a.x * b.x + a.y * b.y;
}

/* 2D cross product returns scalar (z-component of 3D cross) */
static inline float vec2_cross(Vec2 a, Vec2 b) {
    return a.x * b.y - a.y * b.x;
}

/*
 * Length and Distance
 */

static inline float vec2_length_sq(Vec2 v) {
    return v.x * v.x + v.y * v.y;
}

static inline float vec2_length(Vec2 v) {
    return sqrtf(vec2_length_sq(v));
}

static inline float vec2_distance_sq(Vec2 a, Vec2 b) {
    return vec2_length_sq(vec2_sub(a, b));
}

static inline float vec2_distance(Vec2 a, Vec2 b) {
    return sqrtf(vec2_distance_sq(a, b));
}

/*
 * Normalization
 */

static inline Vec2 vec2_normalize(Vec2 v) {
    float len = vec2_length(v);
    if (len < 0.0001f) return vec2_zero();
    return vec2_div(v, len);
}

static inline Vec2 vec2_set_length(Vec2 v, float len) {
    return vec2_mul(vec2_normalize(v), len);
}

static inline Vec2 vec2_clamp_length(Vec2 v, float max_len) {
    float len_sq = vec2_length_sq(v);
    if (len_sq <= max_len * max_len) return v;
    return vec2_set_length(v, max_len);
}

static inline Vec2 vec2_limit(Vec2 v, float max_component) {
    Vec2 result = v;
    if (fabsf(result.x) > max_component) {
        result.x = (result.x > 0) ? max_component : -max_component;
    }
    if (fabsf(result.y) > max_component) {
        result.y = (result.y > 0) ? max_component : -max_component;
    }
    return result;
}

/*
 * Rotation and Angles
 *
 * This game uses a 256-angle system (0-255), not radians.
 * Full circle = 256 units, so each unit = 360/256 = 1.40625 degrees
 */

#define ANGLE_TO_RAD (0.024543692606f)  /* 2*PI / 256 */
#define RAD_TO_ANGLE (40.7436654315f)   /* 256 / (2*PI) */

/* Create a unit vector from a 256-angle
 * For performance, uses inline sin/cos - compiler will optimize */
static inline Vec2 vec2_from_angle(uint8_t angle) {
    float rad = (float)angle * ANGLE_TO_RAD;
    return (Vec2){cosf(rad), sinf(rad)};
}

/* Get 256-angle from vector */
static inline uint8_t vec2_to_angle(Vec2 v) {
    float rad = atan2f(v.y, v.x);
    return (uint8_t)lrintf(rad * RAD_TO_ANGLE);
}

/* Rotate vector by 256-angle */
static inline Vec2 vec2_rotate(Vec2 v, int8_t angle_delta) {
    float rad = (float)angle_delta * ANGLE_TO_RAD;
    float c = cosf(rad);
    float s = sinf(rad);
    return (Vec2){
        v.x * c - v.y * s,
        v.x * s + v.y * c
    };
}

/* Get perpendicular vector (90 degrees counter-clockwise) */
static inline Vec2 vec2_perp(Vec2 v) {
    return (Vec2){-v.y, v.x};
}

/*
 * Interpolation
 */

static inline Vec2 vec2_lerp(Vec2 a, Vec2 b, float t) {
    return (Vec2){
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t
    };
}

/*
 * Toroidal Map Operations
 *
 * The game world wraps around like a torus. These functions handle
 * that elegantly.
 */

static inline Vec2 vec2_wrap(Vec2 v, float width, float height) {
    Vec2 result = v;
    while (result.x >= width) result.x -= width;
    while (result.x < 0) result.x += width;
    while (result.y >= height) result.y -= height;
    while (result.y < 0) result.y += height;
    return result;
}

/* Find shortest delta on a toroidal surface */
static inline Vec2 vec2_toroidal_delta(Vec2 from, Vec2 to, float width, float height) {
    Vec2 delta = vec2_sub(to, from);

    /* Find shortest path in X */
    if (delta.x > width / 2.0f) delta.x -= width;
    else if (delta.x < -width / 2.0f) delta.x += width;

    /* Find shortest path in Y */
    if (delta.y > height / 2.0f) delta.y -= height;
    else if (delta.y < -height / 2.0f) delta.y += height;

    return delta;
}

static inline float vec2_toroidal_distance(Vec2 a, Vec2 b, float width, float height) {
    return vec2_length(vec2_toroidal_delta(a, b, width, height));
}

/*
 * Comparison
 */

static inline int vec2_equals(Vec2 a, Vec2 b) {
    return a.x == b.x && a.y == b.y;
}

static inline int vec2_near(Vec2 a, Vec2 b, float epsilon) {
    return vec2_distance_sq(a, b) < epsilon * epsilon;
}

/*
 * Integer conversion (matches original game's lrintf usage)
 */

typedef struct {
    int x;
    int y;
} Vec2i;

static inline Vec2i vec2_to_int(Vec2 v) {
    return (Vec2i){lrintf(v.x), lrintf(v.y)};
}

static inline Vec2 vec2_from_int(Vec2i v) {
    return (Vec2){(float)v.x, (float)v.y};
}

#endif /* VEC2_H */
