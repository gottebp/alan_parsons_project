/*
 * pool.h - Entity Pool Management
 *
 * A simple, efficient pool allocator for game entities.
 * Uses a free list for O(1) allocation and deallocation.
 *
 * Instead of scanning the entire array for a free slot (O(n)),
 * we maintain a linked list of free slots embedded in the entities themselves.
 */

#ifndef POOL_H
#define POOL_H

#include <stddef.h>
#include <string.h>

/*
 * Any poolable entity must have these fields at the start of its struct:
 *   int active;
 *   int next_free;
 *
 * We use a macro to define pools for specific types, avoiding void* casts.
 */

/*
 * DEFINE_POOL(name, type, capacity)
 *
 * Generates:
 *   - typedef struct { ... } name##Pool;
 *   - void name##_pool_init(name##Pool* pool);
 *   - type* name##_pool_alloc(name##Pool* pool);
 *   - void name##_pool_free(name##Pool* pool, type* entity);
 *   - void name##_pool_clear(name##Pool* pool);
 *   - int name##_pool_count(name##Pool* pool);
 */

#define DEFINE_POOL(name, type, capacity)                                      \
                                                                               \
typedef struct {                                                               \
    type entities[capacity];                                                   \
    int first_free;    /* Head of free list, or -1 if full */                  \
    int count;         /* Number of active entities */                         \
} name##Pool;                                                                  \
                                                                               \
static inline void name##_pool_init(name##Pool* pool) {                        \
    pool->count = 0;                                                           \
    pool->first_free = 0;                                                      \
    memset(pool->entities, 0, sizeof(pool->entities));                         \
                                                                               \
    /* Build free list: each slot points to the next */                        \
    for (int i = 0; i < capacity - 1; i++) {                                   \
        pool->entities[i].active = 0;                                          \
        pool->entities[i].next_free = i + 1;                                   \
    }                                                                          \
    pool->entities[capacity - 1].active = 0;                                   \
    pool->entities[capacity - 1].next_free = -1; /* End of list */             \
}                                                                              \
                                                                               \
static inline type* name##_pool_alloc(name##Pool* pool) {                      \
    if (pool->first_free < 0) return NULL; /* Pool is full */                  \
                                                                               \
    int idx = pool->first_free;                                                \
    type* entity = &pool->entities[idx];                                       \
                                                                               \
    pool->first_free = entity->next_free;                                      \
    entity->active = 1;                                                        \
    entity->next_free = -1;                                                    \
    pool->count++;                                                             \
                                                                               \
    return entity;                                                             \
}                                                                              \
                                                                               \
static inline void name##_pool_free(name##Pool* pool, type* entity) {          \
    if (!entity || !entity->active) return;                                    \
                                                                               \
    int idx = (int)(entity - pool->entities);                                  \
    if (idx < 0 || idx >= capacity) return;                                    \
                                                                               \
    entity->active = 0;                                                        \
    entity->next_free = pool->first_free;                                      \
    pool->first_free = idx;                                                    \
    pool->count--;                                                             \
}                                                                              \
                                                                               \
static inline void name##_pool_clear(name##Pool* pool) {                       \
    name##_pool_init(pool);                                                    \
}                                                                              \
                                                                               \
static inline int name##_pool_count(name##Pool* pool) {                        \
    return pool->count;                                                        \
}                                                                              \
                                                                               \
static inline int name##_pool_capacity(name##Pool* pool) {                     \
    (void)pool;                                                                \
    return capacity;                                                           \
}                                                                              \
                                                                               \
/* Iterate over all active entities */                                         \
static inline type* name##_pool_first(name##Pool* pool) {                      \
    for (int i = 0; i < capacity; i++) {                                       \
        if (pool->entities[i].active) return &pool->entities[i];               \
    }                                                                          \
    return NULL;                                                               \
}                                                                              \
                                                                               \
static inline type* name##_pool_next(name##Pool* pool, type* current) {        \
    if (!current) return name##_pool_first(pool);                              \
    int idx = (int)(current - pool->entities);                                 \
    for (int i = idx + 1; i < capacity; i++) {                                 \
        if (pool->entities[i].active) return &pool->entities[i];               \
    }                                                                          \
    return NULL;                                                               \
}

/*
 * POOL_FOREACH(pool, name, type, body)
 *
 * Iterate over all active entities in a pool.
 * Usage:
 *   POOL_FOREACH(&enemy_pool, enemy, Enemy, {
 *       printf("Enemy at (%f, %f)\n", enemy->position.x, enemy->position.y);
 *   })
 */
#define POOL_FOREACH(pool_ptr, var_name, type, body)                           \
    do {                                                                       \
        for (int _i = 0; _i < (int)(sizeof((pool_ptr)->entities) /             \
                                    sizeof((pool_ptr)->entities[0])); _i++) {  \
            if ((pool_ptr)->entities[_i].active) {                             \
                type* var_name = &(pool_ptr)->entities[_i];                    \
                body                                                           \
            }                                                                  \
        }                                                                      \
    } while(0)

/*
 * POOL_FOREACH_INDEX(pool, idx, item, type, body)
 *
 * Like POOL_FOREACH but also provides the index.
 */
#define POOL_FOREACH_INDEX(pool_ptr, idx_name, var_name, type, body)           \
    do {                                                                       \
        for (int idx_name = 0; idx_name < (int)(sizeof((pool_ptr)->entities) / \
                                    sizeof((pool_ptr)->entities[0])); idx_name++) { \
            if ((pool_ptr)->entities[idx_name].active) {                       \
                type* var_name = &(pool_ptr)->entities[idx_name];              \
                body                                                           \
            }                                                                  \
        }                                                                      \
    } while(0)

/*
 * POOL_FOREACH_CONST(pool, name, type, body)
 *
 * Iterate over all active entities in a pool (read-only).
 * Use this when you only need to read from entities, not modify them.
 */
#define POOL_FOREACH_CONST(pool_ptr, var_name, type, body)                     \
    do {                                                                       \
        for (int _i = 0; _i < (int)(sizeof((pool_ptr)->entities) /             \
                                    sizeof((pool_ptr)->entities[0])); _i++) {  \
            if ((pool_ptr)->entities[_i].active) {                             \
                const type* var_name = &(pool_ptr)->entities[_i];              \
                body                                                           \
            }                                                                  \
        }                                                                      \
    } while(0)

#endif /* POOL_H */
