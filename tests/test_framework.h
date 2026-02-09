/*
 * test_framework.h - A minimal, elegant test framework
 *
 * No dependencies. Just C. Clear output.
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/*============================================================================
 * TEST TRACKING
 *============================================================================*/

static int _tests_run = 0;
static int _tests_passed = 0;
static int _tests_failed = 0;
static const char* _current_test = NULL;
static const char* _current_suite = NULL;

/*============================================================================
 * ASSERTION MACROS
 *============================================================================*/

#define TEST_ASSERT(condition) do { \
    if (!(condition)) { \
        printf("    FAIL: %s\n", #condition); \
        printf("           at %s:%d\n", __FILE__, __LINE__); \
        _tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_MSG(condition, msg) do { \
    if (!(condition)) { \
        printf("    FAIL: %s\n", msg); \
        printf("           %s\n", #condition); \
        printf("           at %s:%d\n", __FILE__, __LINE__); \
        _tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("    FAIL: expected %d, got %d\n", (int)(expected), (int)(actual)); \
        printf("           at %s:%d\n", __FILE__, __LINE__); \
        _tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_FLOAT_EQ(expected, actual, epsilon) do { \
    float _e = (expected); \
    float _a = (actual); \
    if (fabsf(_e - _a) > (epsilon)) { \
        printf("    FAIL: expected %.6f, got %.6f (diff: %.6f)\n", _e, _a, fabsf(_e - _a)); \
        printf("           at %s:%d\n", __FILE__, __LINE__); \
        _tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_STR_EQ(expected, actual) do { \
    if (strcmp((expected), (actual)) != 0) { \
        printf("    FAIL: expected \"%s\", got \"%s\"\n", (expected), (actual)); \
        printf("           at %s:%d\n", __FILE__, __LINE__); \
        _tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        printf("    FAIL: expected NULL, got %p\n", (void*)(ptr)); \
        printf("           at %s:%d\n", __FILE__, __LINE__); \
        _tests_failed++; \
        return; \
    } \
} while(0)

#define TEST_ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        printf("    FAIL: expected non-NULL, got NULL\n"); \
        printf("           at %s:%d\n", __FILE__, __LINE__); \
        _tests_failed++; \
        return; \
    } \
} while(0)

/*============================================================================
 * TEST DEFINITION MACROS
 *============================================================================*/

#define TEST_SUITE(name) \
    static void _suite_##name(void); \
    static void _register_##name(void) __attribute__((constructor)); \
    static void _register_##name(void) { _current_suite = #name; } \
    static void _suite_##name(void)

#define TEST(name) \
    static void _test_##name(void); \
    static void _run_test_##name(void) { \
        _current_test = #name; \
        _tests_run++; \
        int _before_fail = _tests_failed; \
        _test_##name(); \
        if (_tests_failed == _before_fail) { \
            _tests_passed++; \
            printf("    PASS: %s\n", #name); \
        } \
    } \
    static void _test_##name(void)

#define RUN_TEST(name) _run_test_##name()

/*============================================================================
 * TEST RUNNER
 *============================================================================*/

#define TEST_SUITE_BEGIN(name) do { \
    printf("\n=== %s ===\n", name); \
} while(0)

#define TEST_SUITE_END() do { \
    /* nothing needed */ \
} while(0)

#define TEST_REPORT() do { \
    printf("\n"); \
    printf("=====================================\n"); \
    printf("Tests run:    %d\n", _tests_run); \
    printf("Tests passed: %d\n", _tests_passed); \
    printf("Tests failed: %d\n", _tests_failed); \
    printf("=====================================\n"); \
    if (_tests_failed > 0) { \
        printf("SOME TESTS FAILED\n"); \
    } else { \
        printf("ALL TESTS PASSED\n"); \
    } \
} while(0)

#define TEST_EXIT_CODE() (_tests_failed > 0 ? 1 : 0)

#endif /* TEST_FRAMEWORK_H */
