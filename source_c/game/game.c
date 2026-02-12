/*
 * game.c - The Game Core
 *
 * Implementation of the central game systems.
 * Clean, focused, with each function doing one thing well.
 */

#include "../../include_c/game/game.h"
#include <string.h>

/* Forward declarations for weapon functions (implemented in weapons.c) */
void enemy_boss_fire(Game* game, Enemy* e);
void enemy_small_fire(Game* game, Enemy* e);

/*============================================================================
 * RANDOM NUMBER GENERATOR
 *
 * Simple but good LCG (Linear Congruential Generator)
 * Same formula as the original Mersenne Twister was approximating
 *============================================================================*/

void game_rand_seed(Game* game, uint32_t seed) {
    game->rand_state = seed;
}

uint32_t game_rand(Game* game) {
    /* Numerical Recipes LCG parameters */
    game->rand_state = game->rand_state * 1664525 + 1013904223;
    return game->rand_state;
}

uint32_t game_rand_range(Game* game, uint32_t max) {
    if (max == 0) return 0;
    return game_rand(game) % max;
}

float game_rand_float(Game* game) {
    return (float)game_rand(game) / (float)0xFFFFFFFF;
}

/*============================================================================
 * GAME LIFECYCLE
 *============================================================================*/

void game_init(Game* game) {
    memset(game, 0, sizeof(Game));

    game->state = STATE_MENU;
    game->unlocked_level = 0;
    game->weapons_level = 0;
    game->captain_planet = 0;

    player_init(&game->player);
    enemy_pool_init(&game->enemies);
    particle_pool_init(&game->particles);

    game_rand_seed(game, 0xAE33);
    game_rand(game);  /* Consume one like original */
}

void game_reset_player(Game* game) {
    Player* p = &game->player;

    p->position = vec2(MAP_WIDTH / 2.0f, MAP_HEIGHT / 2.0f);
    p->velocity = vec2_zero();
    p->speed = 0.0f;
    p->strafe_speed = 0.0f;
    p->angle = (uint8_t)PLAYER_START_ANGLE;
    p->turn_direction = 0;
    p->health = PLAYER_MAX_HEALTH;

    int nuke_count = game->captain_planet ? 18 : PLAYER_NUKE_COUNT;
    p->nukes_remaining = nuke_count;
    p->nuke_cooldown = 0;

    /* Weapons level carries over from previous levels */
    p->weapons_level = game->weapons_level;
}

void game_start_level(Game* game, int level_index) {
    if (level_index < 0 || level_index >= (int)LEVEL_COUNT) return;

    game->current_level_idx = level_index;
    game->state = STATE_PLAYING;
    game->outcome_timer = 0;
    game->shake_timer = 0.0f;
    game->shake_offset_x = 0;
    game->shake_offset_y = 0;
    game->frame_count = 0;
    game->accumulator = 0.0f;

    game_reset_player(game);
    enemy_pool_clear(&game->enemies);
    particle_pool_clear(&game->particles);

    waves_init(game, level_index);
}

void game_check_outcome(Game* game) {
    /* Player dead? */
    if (game->player.health <= 0 && game->state == STATE_PLAYING) {
        player_explode(game);
        game->state = STATE_DEFEAT;
        game->outcome_timer = OUTCOME_DISPLAY_FRAMES;
    }

    /* Boss dead? Check if boss was spawned and is now inactive */
    if (game->waves.boss_spawned && game->waves.boss_index >= 0) {
        Enemy* boss = &game->enemies.entities[game->waves.boss_index];
        if (!boss->active && game->state == STATE_PLAYING) {
            game->state = STATE_VICTORY;
            game->outcome_timer = OUTCOME_DISPLAY_FRAMES;

            /* Level complete - advance progress if this was their frontier */
            if (game->current_level_idx >= game->unlocked_level) {
                game->unlocked_level = game->current_level_idx + 1;
                game->weapons_level++;
                game_save_progress(game);
            }
        }
    }
}

/*
 * game_tick - One fixed-rate logic step (always called at 1/TARGET_FPS)
 *
 * All frame-counted systems (weapon timers, cooldowns, invulnerability,
 * wave spawning, outcome timer, menu fade) work correctly because
 * dt is always exactly 1/TARGET_FPS, making scale = dt * TARGET_FPS = 1.0.
 */
static void game_tick(Game* game, const InputState* input, float dt) {
    game->frame_count++;

    /* Update based on state */
    switch (game->state) {
        case STATE_PLAYING:
            /* Update nuke cooldown */
            if (game->player.nuke_cooldown > 0) {
                game->player.nuke_cooldown--;
            }

            /* Update player */
            player_update(&game->player, input, dt);

            /* Handle firing inputs */
            if (input->fire) {
                player_fire_weapons(game);
            }
            if (input->nuke) {
                player_fire_nuke(game);
            }

            /* Thruster effects based on movement */
            if (game->player.speed > 1.0f) {
                player_fire_main_thrusters(game);
            }
            if (game->player.strafe_speed < -1.0f) {
                player_fire_left_thrusters(game);
            }
            if (game->player.strafe_speed > 1.0f) {
                player_fire_right_thrusters(game);
            }

            /* Wave spawning */
            waves_update(game);

            /* Update enemies */
            enemies_update(game, dt);

            /* Update particles */
            particles_update(game, dt);

            /* Collisions */
            collisions_check(game);

            /* Check outcome */
            game_check_outcome(game);

            /* Screen shake */
            if (game->shake_timer > 0.0f) {
                game->shake_timer -= dt;
                game->shake_offset_x = (int)(game_rand_range(game, SHAKE_INTENSITY)) - (SHAKE_INTENSITY / 2);
                game->shake_offset_y = (int)(game_rand_range(game, SHAKE_INTENSITY)) - (SHAKE_INTENSITY / 2);
            } else {
                game->shake_offset_x = 0;
                game->shake_offset_y = 0;
            }
            break;

        case STATE_VICTORY:
        case STATE_DEFEAT:
            /* Countdown outcome timer */
            if (game->outcome_timer > 0) {
                game->outcome_timer--;
                if (game->outcome_timer <= 0) {
                    game->menu_requested = 1;
                }
            }

            /* Keep player moving - can fly around during victory/defeat */
            player_update(&game->player, input, dt);

            /* Thruster effects based on movement */
            if (game->player.speed > 1.0f) {
                player_fire_main_thrusters(game);
            }
            if (game->player.strafe_speed < -1.0f) {
                player_fire_left_thrusters(game);
            }
            if (game->player.strafe_speed > 1.0f) {
                player_fire_right_thrusters(game);
            }

            /* Keep enemies moving (original ASM behavior) */
            enemies_update(game, dt);

            /* Continue updating particles */
            particles_update(game, dt);

            /* Screen shake */
            if (game->shake_timer > 0.0f) {
                game->shake_timer -= dt;
                game->shake_offset_x = (int)(game_rand_range(game, SHAKE_INTENSITY)) - (SHAKE_INTENSITY / 2);
                game->shake_offset_y = (int)(game_rand_range(game, SHAKE_INTENSITY)) - (SHAKE_INTENSITY / 2);
            } else {
                game->shake_offset_x = 0;
                game->shake_offset_y = 0;
            }
            break;

        case STATE_MENU:
            /* Non-blocking menu update */
            if (menu_update(game, input)) {
                /* Menu complete - result is in game->menu_result */
                /* Main loop will handle starting the level */
            }
            break;

        case STATE_ENDING:
            /* Handled by main loop */
            break;
    }
}

void game_update(Game* game, const InputState* input, float dt) {
    const float FIXED_DT = 1.0f / TARGET_FPS;

    game->accumulator += dt;

    /* Cap to prevent spiral of death on lag spikes */
    if (game->accumulator > 0.25f) {
        game->accumulator = 0.25f;
    }

    while (game->accumulator >= FIXED_DT) {
        game_tick(game, input, FIXED_DT);
        game->accumulator -= FIXED_DT;
    }
}

/*============================================================================
 * PLAYER
 *============================================================================*/

/* Apply friction to a velocity value, moving it toward zero */
/* Scale factor converts frame-based friction to time-based */
static float apply_friction(float value, float friction, float scale) {
    float decay = friction * scale;
    if (value > 0) {
        value -= decay;
        if (value < 0) value = 0;
    } else if (value < 0) {
        value += decay;
        if (value > 0) value = 0;
    }
    return value;
}

/* Handle player rotation input - direct angle control like the original */
static void player_handle_rotation(Player* p, const InputState* input, float scale) {
    p->turn_direction = 0;
    int turn_rate = (int)(PLAYER_ROTATE_SPEED * scale);
    if (turn_rate < 1) turn_rate = 1;

    if (input->mobile_active && (input->tilt_steer > 0.15f || input->tilt_steer < -0.15f)) {
        /* Mobile analog steering - direct angle change proportional to tilt */
        int tilt_turn = (int)(input->tilt_steer * turn_rate * 1.5f);
        p->angle += (int8_t)tilt_turn;
        p->turn_direction = (input->tilt_steer > 0) ? 1 : -1;
    } else {
        /* Keyboard rotation - direct angle change */
        if (input->right) {
            p->angle += (int8_t)turn_rate;
            p->turn_direction = 1;
        }
        if (input->left) {
            p->angle -= (int8_t)turn_rate;
            p->turn_direction = -1;
        }
    }
}

/* Handle player thrust input */
static void player_handle_thrust(Player* p, const InputState* input, float scale) {
    float accel = p->acceleration * scale;

    if (input->mobile_active && (input->tilt_thrust > 0.2f || input->tilt_thrust < -0.2f)) {
        /* Mobile analog thrust */
        float thrust_input = input->tilt_thrust;
        if (thrust_input > 0.2f && p->speed < p->max_speed) {
            p->speed += accel * thrust_input;
        } else if (thrust_input < -0.2f && p->speed > p->min_speed) {
            p->speed -= accel * (-thrust_input);
        }
    } else {
        /* Keyboard thrust */
        if (input->up && p->speed < p->max_speed) {
            p->speed += accel;
        }
        if (input->down && p->speed > p->min_speed) {
            p->speed -= accel;
        }
    }
}

/* Handle player strafe input */
static void player_handle_strafe(Player* p, const InputState* input, float scale) {
    float accel = p->acceleration * scale;

    if (input->strafe_right) {
        if (p->strafe_speed < p->max_strafe) {
            p->strafe_speed += accel;
        }
        /* Show banking sprite while strafe key held (if not turning) */
        if (p->turn_direction == 0) {
            p->turn_direction = 1;
        }
    }
    if (input->strafe_left) {
        if (p->strafe_speed > p->min_strafe) {
            p->strafe_speed -= accel;
        }
        /* Show banking sprite while strafe key held (if not turning) */
        if (p->turn_direction == 0) {
            p->turn_direction = -1;
        }
    }
}

void player_update(Player* p, const InputState* input, float dt) {
    /* Scale factor: at 60 FPS this equals 1.0, preserving original behavior */
    float scale = dt * TARGET_FPS;

    player_handle_rotation(p, input, scale);
    player_handle_thrust(p, input, scale);
    player_handle_strafe(p, input, scale);
    player_physics(p, dt);
}

void player_physics(Player* p, float dt) {
    /* Scale factor: at 60 FPS this equals 1.0, preserving original behavior */
    float scale = dt * TARGET_FPS;

    /* Direction vectors (angle is now set directly by rotation input) */
    Vec2 forward = vec2_from_angle(p->angle);
    Vec2 right = vec2_perp(forward);

    /* Update collision points (before position changes) */
    p->nose = vec2_add(p->position, vec2_mul(forward, PLAYER_NOSE_OFFSET));
    p->tail = vec2_sub(p->position, vec2_mul(forward, PLAYER_TAIL_OFFSET));

    /* Apply velocity - scale by dt for frame-rate independence */
    p->position = vec2_add(p->position, vec2_mul(forward, p->speed * scale));
    p->position = vec2_add(p->position, vec2_mul(right, p->strafe_speed * scale));

    /* Apply friction */
    p->speed = apply_friction(p->speed, p->friction, scale);
    p->strafe_speed = apply_friction(p->strafe_speed, p->strafe_friction, scale);

    /* Wrap to toroidal world */
    p->position = vec2_wrap(p->position, MAP_WIDTH, MAP_HEIGHT);
}

/*============================================================================
 * PARTICLES
 *============================================================================*/

Particle* particle_spawn(Game* game, int collision_layer, ParticleSize size,
                         int flare_index, Vec2 position, uint8_t angle,
                         float speed, int lifetime, int damage) {
    Particle* p = particle_pool_alloc(&game->particles);
    if (!p) return NULL;

    p->collision_layer = collision_layer;
    p->size = size;
    p->flare_index = flare_index;
    p->position = position;
    p->velocity = vec2_mul(vec2_from_angle(angle), speed);
    p->age = 0.0f;
    p->max_age = lifetime;
    p->damage = damage;

    return p;
}

Particle* particle_spawn_vec(Game* game, int collision_layer, ParticleSize size,
                             int flare_index, Vec2 position, Vec2 velocity,
                             int lifetime, int damage) {
    Particle* p = particle_pool_alloc(&game->particles);
    if (!p) return NULL;

    p->collision_layer = collision_layer;
    p->size = size;
    p->flare_index = flare_index;
    p->position = position;
    p->velocity = velocity;
    p->age = 0.0f;
    p->max_age = lifetime;
    p->damage = damage;

    return p;
}

void particles_update(Game* game, float dt) {
    /* Scale factor: at 60 FPS this equals 1.0 */
    float scale = dt * TARGET_FPS;

    POOL_FOREACH(&game->particles, p, Particle, {
        /* Age check - age is in frames, so we increment by scale */
        p->age += scale;
        if (p->age >= (float)p->max_age) {
            particle_pool_free(&game->particles, p);
            continue;
        }

        /* Update position - scale velocity by dt */
        p->position = vec2_add(p->position, vec2_mul(p->velocity, scale));

        /* Wrap position */
        p->position = vec2_wrap(p->position, MAP_WIDTH, MAP_HEIGHT);
    });
}

/*============================================================================
 * WAVE SYSTEM
 *============================================================================*/

void waves_init(Game* game, int level_index) {
    WaveSystem* ws = &game->waves;
    const LevelDef* level = &LEVELS[level_index];

    memset(ws, 0, sizeof(WaveSystem));
    ws->boss_index = -1;

    int enemy_idx = 0;
    int max_small = MAX_ENEMIES - 1;  /* Leave room for boss */

    /* Create all small enemies (inactive) */
    for (int wave = 0; wave < level->wave_count && enemy_idx < max_small; wave++) {
        ws->waves[wave].first_enemy_idx = enemy_idx;
        int wave_count = 0;

        for (int e = 0; e < level->enemies_per_wave && enemy_idx < max_small; e++) {
            /* Random position anywhere on map */
            Vec2 pos = vec2(
                (float)game_rand_range(game, MAP_WIDTH),
                (float)game_rand_range(game, MAP_HEIGHT)
            );

            /* Random type */
            EnemyType type = (EnemyType)(game_rand(game) & 0x03);
            float mass = enemy_mass_for_type(type);

            Enemy* enemy = enemy_spawn(game, type, ENEMY_SIZE_SMALL, pos, mass);
            if (enemy) {
                enemy->active = 0;  /* Not active until wave spawns */
                enemy_idx++;
                wave_count++;
            }
        }

        ws->waves[wave].enemy_count = wave_count;

        /* Calculate spawn time */
        uint32_t delay = SPAWN_BASE_DELAY + (game_rand_range(game, 0x0FFF) >> SPAWN_RANDOM_DIVISOR);
        if (wave == 0) {
            ws->waves[wave].frame_trigger = delay;
        } else {
            ws->waves[wave].frame_trigger = ws->waves[wave - 1].frame_trigger + delay;
        }

        ws->wave_count++;
    }

    /* Create boss (inactive) */
    Vec2 boss_pos = vec2(level->boss_x, level->boss_y);
    Enemy* boss = enemy_spawn(game, level->boss_type, ENEMY_SIZE_BOSS, boss_pos, level->boss_mass);
    if (boss) {
        boss->active = 0;
        ws->boss_index = (int)(boss - game->enemies.entities);
    }

    /* Activate first wave immediately */
    if (ws->wave_count > 0) {
        Wave* first = &ws->waves[0];
        for (int i = 0; i < first->enemy_count; i++) {
            game->enemies.entities[first->first_enemy_idx + i].active = 1;
        }
        ws->current_wave = 1;  /* Next wave to spawn */
    }

    ws->frame_counter = 0;
}

void waves_update(Game* game) {
    WaveSystem* ws = &game->waves;
    ws->frame_counter++;

    /* Check for next wave spawn */
    if (ws->current_wave < ws->wave_count) {
        Wave* next = &ws->waves[ws->current_wave];
        if ((int)ws->frame_counter >= next->frame_trigger) {
            /* Spawn this wave */
            for (int i = 0; i < next->enemy_count; i++) {
                game->enemies.entities[next->first_enemy_idx + i].active = 1;
            }
            ws->current_wave++;
        }
    }

    /* Check for boss spawn (after all waves have been triggered) */
    if (ws->current_wave >= ws->wave_count && !ws->boss_spawned && ws->boss_index >= 0) {
        waves_spawn_boss(game);
    }
}

void waves_spawn_boss(Game* game) {
    WaveSystem* ws = &game->waves;
    if (ws->boss_index < 0 || ws->boss_spawned) return;

    game->enemies.entities[ws->boss_index].active = 1;
    ws->boss_spawned = 1;

    /* Boss entrance effects */
    effect_screen_shake(game, 150);
    game->audio_boss_spawned = 1;
}

/*============================================================================
 * ENEMIES
 *============================================================================*/

#if !CLASSIC_AI_NO_REPULSION
/*
 * Calculate repulsion force from nearby enemies.
 * This keeps enemies from clumping together too tightly.
 * DISABLED by default - original ASM had no inter-enemy physics.
 */
static Vec2 enemy_calculate_repulsion(const Game* game, const Enemy* self) {
    Vec2 repulsion = vec2_zero();

    POOL_FOREACH_CONST(&game->enemies, other, Enemy, {
        if (other == self) continue;

        float dx = other->position.x - self->position.x;
        float dy = other->position.y - self->position.y;

        if (dx > MAP_WIDTH / 2) dx -= MAP_WIDTH;
        else if (dx < -MAP_WIDTH / 2) dx += MAP_WIDTH;
        if (dy > MAP_HEIGHT / 2) dy -= MAP_HEIGHT;
        else if (dy < -MAP_HEIGHT / 2) dy += MAP_HEIGHT;

        if (fabsf(dx) > ENEMY_REPULSION_RADIUS ||
            fabsf(dy) > ENEMY_REPULSION_RADIUS) continue;

        float dist_sq = dx * dx + dy * dy;
        float radius_sq = ENEMY_REPULSION_RADIUS * ENEMY_REPULSION_RADIUS;

        if (dist_sq < radius_sq && dist_sq > 1.0f) {
            float inv_dist = 1.0f / sqrtf(dist_sq);
            float force = ENEMY_REPULSION_FORCE / dist_sq;
            repulsion.x += dx * inv_dist * force;
            repulsion.y += dy * inv_dist * force;
        }
    });

    return repulsion;
}
#endif /* !CLASSIC_AI_NO_REPULSION */

/*
 * Apply velocity with per-component limiting.
 * Preserves the original game's feel where X and Y are clamped independently.
 */
static void enemy_apply_velocity(Enemy* e, Vec2 accel, int max_speed) {
    Vec2 new_vel = vec2_add(e->velocity, accel);

    if (fabsf(new_vel.x) <= max_speed) e->velocity.x = new_vel.x;
    if (fabsf(new_vel.y) <= max_speed) e->velocity.y = new_vel.y;
}

#if CLASSIC_AI_DIRECT_TURNING
/*
 * Turn enemy toward a target angle using direct adjustment.
 * ORIGINAL BEHAVIOR: Snap toward target, limited by MAX_TURN_RATE per frame.
 * This is snappier and doesn't cause vertigo like angular momentum.
 */
static void enemy_turn_toward(Enemy* e, uint8_t target_angle, float scale) {
    int8_t diff = (int8_t)(target_angle - e->angle);
    int turn_rate = (int)(ENEMY_MAX_TURN_RATE * scale);
    if (turn_rate < 1) turn_rate = 1;

    if (diff > turn_rate) {
        e->angle += (uint8_t)turn_rate;
    } else if (diff < -turn_rate) {
        e->angle -= (uint8_t)turn_rate;
    } else {
        e->angle = target_angle;
    }
}
#else
/*
 * Turn enemy toward a target angle using angular momentum.
 * DISABLED by default - causes vertigo.
 */
static void enemy_apply_angular_friction(Enemy* e, float scale) {
    float decay = ENEMY_ANGULAR_FRICTION * scale;
    if (e->angular_vel > 0) {
        e->angular_vel -= decay;
        if (e->angular_vel < 0) e->angular_vel = 0;
    } else if (e->angular_vel < 0) {
        e->angular_vel += decay;
        if (e->angular_vel > 0) e->angular_vel = 0;
    }
}

static void enemy_turn_toward(Enemy* e, uint8_t target_angle, float scale) {
    int8_t diff = (int8_t)(e->angle - target_angle);
    float max_angular_vel = (float)ENEMY_MAX_TURN_RATE;
    float accel = ENEMY_ANGULAR_ACCEL * scale;

    if (diff > 0) {
        e->angular_vel -= accel;
        if (e->angular_vel < -max_angular_vel)
            e->angular_vel = -max_angular_vel;
    } else if (diff < 0) {
        e->angular_vel += accel;
        if (e->angular_vel > max_angular_vel)
            e->angular_vel = max_angular_vel;
    }

    enemy_apply_angular_friction(e, scale);
    e->angle = (uint8_t)((int)e->angle + (int8_t)(e->angular_vel * scale));
}
#endif /* CLASSIC_AI_DIRECT_TURNING */

/*
 * Calculate gravitational acceleration toward player.
 * The gravitational constant varies by enemy type for small enemies.
 */
static Vec2 enemy_calculate_gravity(const Enemy* e, Vec2 to_player, float dist, float g_mass) {
    float gravity = g_mass / (dist * dist);
    if (e->size == ENEMY_SIZE_SMALL) {
        gravity *= enemy_gravity_for_type(e->type);
    }
    return vec2_mul(vec2_normalize(to_player), gravity);
}

/*
 * AI thinking: calculate desired acceleration based on player position.
 * Returns the acceleration vector the enemy wants to apply.
 */
static Vec2 enemy_ai_think(const Game* game, const Enemy* e, Vec2 to_player, float dist, float g_mass) {
    (void)game; /* May be unused if repulsion disabled */

#if CLASSIC_AI_NO_ORBIT
    /*
     * ORIGINAL BEHAVIOR: Inside inner radius, just stop accelerating.
     * Enemy coasts on current velocity, creating natural orbit from
     * their tangential approach velocity. No active orbit forces.
     */
    if (dist < ENEMY_INNER_RADIUS) {
        return vec2_zero();
    }
    return enemy_calculate_gravity(e, to_player, dist, g_mass);
#else
    /* New behavior: active orbit with tangential acceleration */
    Vec2 accel;

    if (dist < ENEMY_INNER_RADIUS) {
        Vec2 tangent = vec2_perp(vec2_normalize(to_player));
        int orbit_dir = ((uintptr_t)e / sizeof(Enemy)) % 2 == 0 ? 1 : -1;
        accel = vec2_mul(tangent, 0.5f * orbit_dir);

        if (dist < ENEMY_INNER_RADIUS * 0.5f) {
            Vec2 away = vec2_mul(vec2_normalize(to_player), -0.8f);
            accel = vec2_add(accel, away);
        }
    } else {
        accel = enemy_calculate_gravity(e, to_player, dist, g_mass);
    }

#if !CLASSIC_AI_NO_REPULSION
    accel = vec2_add(accel, enemy_calculate_repulsion(game, e));
#endif

    return accel;
#endif /* CLASSIC_AI_NO_ORBIT */
}

/*
 * AI firing decision: fire at player if within range.
 */
static void enemy_ai_fire(Game* game, Enemy* e, float dist) {
    int firing_range = (e->size == ENEMY_SIZE_BOSS) ? boss_firing_range(e->type) : 400;

    if (dist < firing_range) {
        if (e->size == ENEMY_SIZE_BOSS) {
            enemy_boss_fire(game, e);
        } else {
            enemy_small_fire(game, e);
        }
    }
}

/*
 * Physics: apply acceleration, update position, wrap at map edges.
 */
static void enemy_physics(Enemy* e, Vec2 accel, float scale) {
    /* Scale acceleration for time-independence */
    Vec2 scaled_accel = vec2_mul(accel, scale);

    /* Apply acceleration with speed limiting */
    int max_speed = (e->size == ENEMY_SIZE_SMALL)
                  ? enemy_max_speed_for_type(e->type)
                  : ENEMY_MAX_SPEED;
    enemy_apply_velocity(e, scaled_accel, max_speed);

    /* Update position - scale velocity by dt */
    e->position = vec2_add(e->position, vec2_mul(e->velocity, scale));
    e->position = vec2_wrap(e->position, MAP_WIDTH, MAP_HEIGHT);
}

Enemy* enemy_spawn(Game* game, EnemyType type, EnemySize size, Vec2 position, float mass) {
    Enemy* e = enemy_pool_alloc(&game->enemies);
    if (!e) return NULL;

    e->type = type;
    e->size = size;
    e->position = position;
    e->velocity = vec2_zero();
    e->angle = 0;
    e->angular_vel = 0.0f;
    e->mass = mass;
    e->health = (int)(mass * ENEMY_HEALTH_MULTIPLIER);

    return e;
}

void enemies_update(Game* game, float dt) {
    /* Scale factor: at 60 FPS this equals 1.0 */
    float scale = dt * TARGET_FPS;

    Player* player = &game->player;
    float g_mass = player->mass * ENEMY_GRAVITY_CONSTANT;

    POOL_FOREACH(&game->enemies, e, Enemy, {
        /* Calculate delta to player (toroidal) */
        Vec2 to_player = vec2_toroidal_delta(e->position, player->position, MAP_WIDTH, MAP_HEIGHT);
        float dist = vec2_length(to_player);
        if (dist < 1.0f) dist = 1.0f;

        /* AI: decide what to do */
        Vec2 accel = enemy_ai_think(game, e, to_player, dist, g_mass);
        enemy_turn_toward(e, vec2_to_angle(to_player), scale);
        enemy_ai_fire(game, e, dist);

        /* Physics: apply the decision */
        enemy_physics(e, accel, scale);
    });
}

/*============================================================================
 * COLLISION DETECTION
 *============================================================================*/

/*
 * Fast toroidal distance check (AABB approximation).
 * Returns dx, dy adjusted for wrap-around. Use for quick culling.
 */
static inline void toroidal_delta_fast(Vec2 from, Vec2 to, float* dx, float* dy) {
    *dx = to.x - from.x;
    *dy = to.y - from.y;

    if (*dx > MAP_WIDTH / 2) *dx -= MAP_WIDTH;
    else if (*dx < -MAP_WIDTH / 2) *dx += MAP_WIDTH;

    if (*dy > MAP_HEIGHT / 2) *dy -= MAP_HEIGHT;
    else if (*dy < -MAP_HEIGHT / 2) *dy += MAP_HEIGHT;
}

/*
 * Check if position is within AABB of a point.
 */
static inline int within_aabb(float dx, float dy, float size) {
    return fabsf(dx) < size && fabsf(dy) < size;
}

void collisions_check(Game* game) {
    Player* player = &game->player;

    POOL_FOREACH(&game->particles, p, Particle, {
        if (p->collision_layer == COLLISION_NONE) continue;

        int can_hit_player = (p->collision_layer == COLLISION_ENEMY_OWNED ||
                              p->collision_layer == COLLISION_NEUTRAL);
        int can_hit_enemy  = (p->collision_layer == COLLISION_PLAYER_OWNED ||
                              p->collision_layer == COLLISION_NEUTRAL);
        float dx;
        float dy;

        /* Check particle vs player */
        if (can_hit_player) {
            toroidal_delta_fast(player->position, p->position, &dx, &dy);
            if (within_aabb(dx, dy, PLAYER_COLLISION_RADIUS)) {
                if (collision_particle_player(p, player)) {
                    player_take_damage(game, p->damage, p->position);
                    particle_pool_free(&game->particles, p);
                    continue;
                }
            }
        }

        /* Check particle vs enemies */
        if (can_hit_enemy) {
            POOL_FOREACH(&game->enemies, e, Enemy, {
                float edx;
                float edy;
                toroidal_delta_fast(e->position, p->position, &edx, &edy);
                float coll_size = (e->size == ENEMY_SIZE_BOSS) ? BOSS_COLLISION : SMALL_ENEMY_COLLISION;

                if (within_aabb(edx, edy, coll_size)) {
                    if (collision_particle_enemy(p, e)) {
                        enemy_damage(game, e, p->damage, p->position);
                        particle_pool_free(&game->particles, p);
                        break;
                    }
                }
            });
        }
    });
}

int collision_particle_player(const Particle* p, const Player* player) {
    float half_size = (p->size == PARTICLE_SIZE_SMALL) ?
                      SMALL_PARTICLE_WIDTH / 2.0f : LARGE_PARTICLE_WIDTH / 2.0f;

    Vec2 pos = player->position;
    /* Check center, nose, and tail */
    Vec2 points[] = { pos, player->nose, player->tail };

    for (int i = 0; i < 3; i++) {
        Vec2i pt = vec2_to_int(points[i]);
        Vec2i pp = vec2_to_int(p->position);

        if (pt.x >= pp.x - half_size && pt.x <= pp.x + half_size &&
            pt.y >= pp.y - half_size && pt.y <= pp.y + half_size) {
            return 1;
        }
    }

    return 0;
}

int collision_particle_enemy(const Particle* p, const Enemy* e) {
    float collision_size = (e->size == ENEMY_SIZE_BOSS) ?
                           BOSS_COLLISION : SMALL_ENEMY_COLLISION;

    Vec2i eint = vec2_to_int(e->position);
    Vec2i pint = vec2_to_int(p->position);

    return (pint.x >= eint.x - collision_size &&
            pint.x <= eint.x + collision_size &&
            pint.y >= eint.y - collision_size &&
            pint.y <= eint.y + collision_size);
}

/*============================================================================
 * BODY COLLISION (RAMMING)
 *============================================================================*/

void collisions_check_bodies(Game* game) {
    Player* player = &game->player;

    /* Decrement invulnerability timers */
    if (player->invuln_frames > 0) {
        player->invuln_frames--;
    }

    POOL_FOREACH(&game->enemies, e, Enemy, {
        if (e->invuln_frames > 0) {
            e->invuln_frames--;
        }
    });

    /* Skip if player is invulnerable */
    if (player->invuln_frames > 0) return;

    Vec2 player_pos = player->position;

    /* Calculate player velocity vector */
    Vec2 forward = vec2_from_angle(player->angle);
    Vec2 right = vec2_perp(forward);
    Vec2 player_vel = vec2_add(
        vec2_mul(forward, player->speed),
        vec2_mul(right, player->strafe_speed)
    );

    POOL_FOREACH(&game->enemies, e, Enemy, {
        /* Skip invulnerable enemies */
        if (e->invuln_frames > 0) continue;

        /* Calculate distance (toroidal) */
        float dx = e->position.x - player_pos.x;
        float dy = e->position.y - player_pos.y;
        if (dx > MAP_WIDTH / 2) dx -= MAP_WIDTH;
        else if (dx < -MAP_WIDTH / 2) dx += MAP_WIDTH;
        if (dy > MAP_HEIGHT / 2) dy -= MAP_HEIGHT;
        else if (dy < -MAP_HEIGHT / 2) dy += MAP_HEIGHT;

        /* Combined collision radius */
        float enemy_radius = (e->size == ENEMY_SIZE_BOSS) ?
                             BOSS_COLLISION : SMALL_ENEMY_COLLISION;
        float collision_dist = PLAYER_BODY_RADIUS + enemy_radius;

        float dist_sq = dx * dx + dy * dy;
        if (dist_sq >= collision_dist * collision_dist) continue;

        /* COLLISION! Calculate relative velocity */
        Vec2 relative_vel = vec2_sub(player_vel, e->velocity);
        float rel_speed = vec2_length(relative_vel);

        /* Calculate damage based on closing speed */
        int base_damage = (int)(rel_speed * BODY_COLLISION_DAMAGE_SCALE);
        if (base_damage < BODY_COLLISION_MIN_DAMAGE) {
            base_damage = BODY_COLLISION_MIN_DAMAGE;
        }

        /* Player takes damage (reduced by mass ratio) */
        float mass_ratio = e->mass / (player->mass + e->mass);
        int player_damage = (int)(base_damage * mass_ratio);
        if (player_damage < BODY_COLLISION_MIN_DAMAGE) {
            player_damage = BODY_COLLISION_MIN_DAMAGE;
        }

        /* Enemy takes damage (proportional to player's share) */
        int enemy_damage_amt = (int)(base_damage * (1.0f - mass_ratio));
        if (enemy_damage_amt < BODY_COLLISION_MIN_DAMAGE) {
            enemy_damage_amt = BODY_COLLISION_MIN_DAMAGE;
        }

        /* Apply damage */
        player->health -= player_damage;
        e->health -= enemy_damage_amt;

        /* Calculate knockback direction (normalized separation) */
        float dist = sqrtf(dist_sq);
        if (dist < 1.0f) dist = 1.0f;
        Vec2 sep = vec2(dx / dist, dy / dist);  /* Points from player to enemy */

        /* Apply knockback (push apart) */
        float knockback = BODY_COLLISION_KNOCKBACK;

        /* Player knocked away from enemy */
        player->speed -= knockback * mass_ratio * (sep.x * forward.x + sep.y * forward.y);
        player->strafe_speed -= knockback * mass_ratio * (sep.x * right.x + sep.y * right.y);

        /* Enemy knocked away from player */
        e->velocity = vec2_add(e->velocity, vec2_mul(sep, knockback * (1.0f - mass_ratio)));

        /* Set invulnerability */
        player->invuln_frames = PLAYER_INVULN_FRAMES;
        e->invuln_frames = ENEMY_INVULN_FRAMES;

        /* Visual feedback */
        Vec2 impact_point = vec2_add(player_pos, vec2_mul(sep, PLAYER_BODY_RADIUS));
        effect_hit_sparks(game, impact_point, 1);  /* Large impact */
        effect_screen_shake(game, 30);

        /* Check for enemy death */
        if (e->health <= 0) {
            enemy_explode(game, e);
            enemy_pool_free(&game->enemies, e);
        }
    });
}

/*============================================================================
 * DAMAGE AND DEATH
 *============================================================================*/

void player_take_damage(Game* game, int damage, Vec2 impact_pos) {
    game->player.health -= damage;
    effect_hit_sparks(game, impact_pos, 1);  /* Large impact x2 */
    effect_hit_sparks(game, impact_pos, 1);
    game->audio_player_hit = 1;  /* Signal for audio */
}

void player_explode(Game* game) {
    effect_explosion_large(game, game->player.position);
    effect_explosion_small(game, game->player.position);
    effect_explosion_large(game, game->player.position);
    effect_explosion_small(game, game->player.position);
    effect_screen_shake(game, 150);
}

void enemy_damage(Game* game, Enemy* e, int damage, Vec2 impact_pos) {
    e->health -= damage;
    effect_hit_sparks(game, impact_pos, 0);

    if (e->health <= 0) {
        enemy_explode(game, e);
        enemy_pool_free(&game->enemies, e);
        game->audio_enemy_destroyed = 1;  /* Signal for audio */
    }
}

void enemy_explode(Game* game, Enemy* e) {
    if (e->size == ENEMY_SIZE_BOSS) {
        /* Massive explosion for boss death */
        effect_explosion_large(game, e->position);
        effect_explosion_large(game, e->position);
        effect_explosion_small(game, e->position);
        effect_explosion_small(game, e->position);
        effect_screen_shake(game, 180);
    } else {
        effect_explosion_small(game, e->position);
    }
}

/*============================================================================
 * EFFECTS
 *============================================================================*/

void effect_screen_shake(Game* game, int frames) {
    float seconds = (float)frames / TARGET_FPS;
    if (seconds > game->shake_timer) {
        game->shake_timer = seconds;
    }
}

void effect_explosion_small(Game* game, Vec2 position) {
    /* 20 loops, 3 particles each */
    for (int i = 0; i < 20; i++) {
        uint8_t angle1 = game_rand_range(game, 256);
        uint8_t angle2 = game_rand_range(game, 256);
        uint8_t angle3 = game_rand_range(game, 256);

        particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_LARGE, 3, position, angle1, 4.2f, 10, 0);
        particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_LARGE, 7, position, angle2, 5.33f, 20, 0);
        particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_LARGE, 8, position, angle3, 6.112f, 30, 0);
    }
}

void effect_explosion_large(Game* game, Vec2 position) {
    /* 100 loops, 6 particles each - visual only, no damage */
    for (int i = 0; i < 100; i++) {
        particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL, 4, position,
                      game_rand_range(game, 256), 4.2f, 200, 0);
        particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL, 4, position,
                      game_rand_range(game, 256), 6.112f, 200, 0);
        particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_LARGE, 5, position,
                      game_rand_range(game, 256), 5.33f, 210, 0);
        particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL, 5, position,
                      game_rand_range(game, 256), 4.2f, 220, 0);
        particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL, 5, position,
                      game_rand_range(game, 256), 6.112f, 220, 0);
        particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL, 6, position,
                      game_rand_range(game, 256), 7.5f, 230, 0);
    }
}

void effect_hit_sparks(Game* game, Vec2 position, int large) {
    uint8_t angle = game_rand_range(game, 256);
    float speed = 5.3f;

    /* Base sparks (always shown) */
    particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL, 14, position, angle, speed, 15, 0);
    particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_LARGE, 2, position, angle + 85, speed, 15, 0);
    particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_LARGE, 3, position, angle + 170, speed, 15, 0);

    /* Extra sparks for larger impacts */
    if (large) {
        particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL, 14, position, angle + 193, speed, 15, 0);
        particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL, 14, position, angle + 261, speed, 15, 0);
        particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL, 14, position, angle + 353, speed, 15, 0);
    }
}

void effect_nuke(Game* game, Vec2 position) {
    /*
     * Original ASM: Loop increments by 2 but spawns at offsets 0,1,2,3
     * This creates overlapping angles for a DENSE shockwave.
     * Speeds from _fltEnemyFiringRate array: 7.3112, 12.6, 14.5331, 16.31
     */
    for (int angle = 0; angle < 256; angle += 2) {
        /* Four particles per iteration with overlapping angles */
        particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_LARGE, 5, position,
                      angle, 7.3112f, 100, 20);

        particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_SMALL, 15, position,
                      angle + 1, 16.31f, 100, 20);

        particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_LARGE, 5, position,
                      angle + 2, 14.5331f, 100, 20);

        particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_SMALL, 15, position,
                      angle + 3, 12.6f, 100, 20);
    }
}

/*============================================================================
 * SAVE/LOAD (Platform-specific implementations elsewhere)
 *============================================================================*/

/* These are stubs - actual implementation depends on platform */
__attribute__((weak))
void game_save_progress(const Game* game) {
    (void)game;
}

__attribute__((weak))
void game_load_progress(Game* game) {
    (void)game;
}
