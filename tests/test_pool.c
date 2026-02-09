/*
 * test_pool.c - Tests for the entity pool system
 */

#include "test_framework.h"
#include "../include_c/core/pool.h"

/*============================================================================
 * TEST ENTITY TYPE
 *============================================================================*/

typedef struct {
    int active;
    int next_free;
    int value;
    float x, y;
} TestEntity;

/* Define a pool with capacity 10 */
DEFINE_POOL(test, TestEntity, 10)

/* Silence unused function warnings for test_pool_next */
__attribute__((unused)) static void _use_pool_next(void) {
    (void)test_pool_next;
}

/*============================================================================
 * BASIC ALLOCATION TESTS
 *============================================================================*/

TEST(pool_init) {
    testPool pool;
    test_pool_init(&pool);

    TEST_ASSERT_EQ(0, test_pool_count(&pool));
    TEST_ASSERT_EQ(10, test_pool_capacity(&pool));
}

TEST(pool_alloc_one) {
    testPool pool;
    test_pool_init(&pool);

    TestEntity* e = test_pool_alloc(&pool);
    TEST_ASSERT_NOT_NULL(e);
    TEST_ASSERT_EQ(1, e->active);
    TEST_ASSERT_EQ(1, test_pool_count(&pool));
}

TEST(pool_alloc_multiple) {
    testPool pool;
    test_pool_init(&pool);

    TestEntity* e1 = test_pool_alloc(&pool);
    TestEntity* e2 = test_pool_alloc(&pool);
    TestEntity* e3 = test_pool_alloc(&pool);

    TEST_ASSERT_NOT_NULL(e1);
    TEST_ASSERT_NOT_NULL(e2);
    TEST_ASSERT_NOT_NULL(e3);

    /* All should be different */
    TEST_ASSERT(e1 != e2);
    TEST_ASSERT(e2 != e3);
    TEST_ASSERT(e1 != e3);

    TEST_ASSERT_EQ(3, test_pool_count(&pool));
}

TEST(pool_alloc_until_full) {
    testPool pool;
    test_pool_init(&pool);

    /* Allocate all 10 */
    for (int i = 0; i < 10; i++) {
        TestEntity* e = test_pool_alloc(&pool);
        TEST_ASSERT_NOT_NULL(e);
        e->value = i;
    }

    TEST_ASSERT_EQ(10, test_pool_count(&pool));

    /* 11th should fail */
    TestEntity* overflow = test_pool_alloc(&pool);
    TEST_ASSERT_NULL(overflow);
}

/*============================================================================
 * FREE TESTS
 *============================================================================*/

TEST(pool_free_one) {
    testPool pool;
    test_pool_init(&pool);

    TestEntity* e = test_pool_alloc(&pool);
    TEST_ASSERT_EQ(1, test_pool_count(&pool));

    test_pool_free(&pool, e);
    TEST_ASSERT_EQ(0, test_pool_count(&pool));
    TEST_ASSERT_EQ(0, e->active);
}

TEST(pool_free_and_realloc) {
    testPool pool;
    test_pool_init(&pool);

    /* Allocate */
    TestEntity* e1 = test_pool_alloc(&pool);
    e1->value = 42;

    /* Free */
    test_pool_free(&pool, e1);

    /* Reallocate - should get the same slot */
    TestEntity* e2 = test_pool_alloc(&pool);
    TEST_ASSERT(e1 == e2);
    TEST_ASSERT_EQ(1, test_pool_count(&pool));
}

TEST(pool_free_middle) {
    testPool pool;
    test_pool_init(&pool);

    /* Allocate 5 */
    TestEntity* entities[5];
    for (int i = 0; i < 5; i++) {
        entities[i] = test_pool_alloc(&pool);
        entities[i]->value = i;
    }

    /* Free the middle one */
    test_pool_free(&pool, entities[2]);
    TEST_ASSERT_EQ(4, test_pool_count(&pool));

    /* Others should still be valid */
    TEST_ASSERT_EQ(1, entities[0]->active);
    TEST_ASSERT_EQ(1, entities[1]->active);
    TEST_ASSERT_EQ(0, entities[2]->active);  /* Freed */
    TEST_ASSERT_EQ(1, entities[3]->active);
    TEST_ASSERT_EQ(1, entities[4]->active);
}

TEST(pool_free_all_realloc) {
    testPool pool;
    test_pool_init(&pool);

    /* Fill pool */
    TestEntity* entities[10];
    for (int i = 0; i < 10; i++) {
        entities[i] = test_pool_alloc(&pool);
    }
    TEST_ASSERT_EQ(10, test_pool_count(&pool));

    /* Free all */
    for (int i = 0; i < 10; i++) {
        test_pool_free(&pool, entities[i]);
    }
    TEST_ASSERT_EQ(0, test_pool_count(&pool));

    /* Should be able to allocate again */
    for (int i = 0; i < 10; i++) {
        TestEntity* e = test_pool_alloc(&pool);
        TEST_ASSERT_NOT_NULL(e);
    }
    TEST_ASSERT_EQ(10, test_pool_count(&pool));
}

/*============================================================================
 * CLEAR TESTS
 *============================================================================*/

TEST(pool_clear) {
    testPool pool;
    test_pool_init(&pool);

    /* Allocate some */
    for (int i = 0; i < 5; i++) {
        test_pool_alloc(&pool);
    }
    TEST_ASSERT_EQ(5, test_pool_count(&pool));

    /* Clear */
    test_pool_clear(&pool);
    TEST_ASSERT_EQ(0, test_pool_count(&pool));

    /* Should be able to allocate all 10 again */
    for (int i = 0; i < 10; i++) {
        TestEntity* e = test_pool_alloc(&pool);
        TEST_ASSERT_NOT_NULL(e);
    }
}

/*============================================================================
 * ITERATION TESTS
 *============================================================================*/

TEST(pool_foreach_empty) {
    testPool pool;
    test_pool_init(&pool);

    int count = 0;
    POOL_FOREACH(&pool, e, TestEntity, {
        (void)e;  /* Silence unused variable warning */
        count++;
    });

    TEST_ASSERT_EQ(0, count);
}

TEST(pool_foreach_all) {
    testPool pool;
    test_pool_init(&pool);

    /* Allocate 5, set values */
    for (int i = 0; i < 5; i++) {
        TestEntity* e = test_pool_alloc(&pool);
        e->value = i;
    }

    int sum = 0;
    int count = 0;
    POOL_FOREACH(&pool, e, TestEntity, {
        sum += e->value;
        count++;
    });

    TEST_ASSERT_EQ(5, count);
    TEST_ASSERT_EQ(10, sum);  /* 0+1+2+3+4 = 10 */
}

TEST(pool_foreach_with_gaps) {
    testPool pool;
    test_pool_init(&pool);

    /* Allocate 5 */
    TestEntity* entities[5];
    for (int i = 0; i < 5; i++) {
        entities[i] = test_pool_alloc(&pool);
        entities[i]->value = i;
    }

    /* Free odd indices */
    test_pool_free(&pool, entities[1]);
    test_pool_free(&pool, entities[3]);

    int sum = 0;
    int count = 0;
    POOL_FOREACH(&pool, e, TestEntity, {
        sum += e->value;
        count++;
    });

    TEST_ASSERT_EQ(3, count);
    TEST_ASSERT_EQ(6, sum);  /* 0+2+4 = 6 */
}

/*============================================================================
 * EDGE CASE TESTS
 *============================================================================*/

TEST(pool_free_null) {
    testPool pool;
    test_pool_init(&pool);

    /* Should not crash */
    test_pool_free(&pool, NULL);
    TEST_ASSERT_EQ(0, test_pool_count(&pool));
}

TEST(pool_free_inactive) {
    testPool pool;
    test_pool_init(&pool);

    TestEntity* e = test_pool_alloc(&pool);
    test_pool_free(&pool, e);

    /* Should not crash or double-free */
    test_pool_free(&pool, e);
    TEST_ASSERT_EQ(0, test_pool_count(&pool));
}

TEST(pool_data_persistence) {
    testPool pool;
    test_pool_init(&pool);

    /* Allocate and set data */
    TestEntity* e = test_pool_alloc(&pool);
    e->value = 42;
    e->x = 3.14f;
    e->y = 2.71f;

    /* Data should persist */
    TEST_ASSERT_EQ(42, e->value);
    TEST_ASSERT_FLOAT_EQ(3.14f, e->x, 0.001f);
    TEST_ASSERT_FLOAT_EQ(2.71f, e->y, 0.001f);
}

/*============================================================================
 * MAIN
 *============================================================================*/

int main(void) {
    TEST_SUITE_BEGIN("Pool Tests");

    printf("\n  Basic Allocation:\n");
    RUN_TEST(pool_init);
    RUN_TEST(pool_alloc_one);
    RUN_TEST(pool_alloc_multiple);
    RUN_TEST(pool_alloc_until_full);

    printf("\n  Freeing:\n");
    RUN_TEST(pool_free_one);
    RUN_TEST(pool_free_and_realloc);
    RUN_TEST(pool_free_middle);
    RUN_TEST(pool_free_all_realloc);

    printf("\n  Clear:\n");
    RUN_TEST(pool_clear);

    printf("\n  Iteration:\n");
    RUN_TEST(pool_foreach_empty);
    RUN_TEST(pool_foreach_all);
    RUN_TEST(pool_foreach_with_gaps);

    printf("\n  Edge Cases:\n");
    RUN_TEST(pool_free_null);
    RUN_TEST(pool_free_inactive);
    RUN_TEST(pool_data_persistence);

    TEST_SUITE_END();
    TEST_REPORT();

    return TEST_EXIT_CODE();
}
