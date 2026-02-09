/*
 * ai_player.c - AI Controller Implementation
 *
 * The AI that plays the game. Watches the world, makes decisions,
 * generates inputs as if it were a human at the keyboard.
 */

#include "../../include_c/game/ai_player.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/*============================================================================
 * PREDEFINED CONFIGURATIONS
 *============================================================================*/

const AIConfig AI_CONFIG_AGGRESSIVE = {
    .preferred_distance = 200.0f,
    .fire_angle_threshold = 20.0f,
    .danger_radius = 300.0f,
    .dodge_health_threshold = 500,
    .approach_speed = 1.0f,
    .use_nukes = 1,
    .nuke_enemy_threshold = 8,
    .add_noise = 0,
    .noise_amount = 0.0f
};

const AIConfig AI_CONFIG_CAUTIOUS = {
    .preferred_distance = 400.0f,
    .fire_angle_threshold = 15.0f,
    .danger_radius = 400.0f,
    .dodge_health_threshold = 1000,
    .approach_speed = 0.5f,
    .use_nukes = 1,
    .nuke_enemy_threshold = 4,
    .add_noise = 0,
    .noise_amount = 0.0f
};

const AIConfig AI_CONFIG_BALANCED = {
    .preferred_distance = 350.0f,
    .fire_angle_threshold = 30.0f,
    .danger_radius = 400.0f,
    .dodge_health_threshold = 2000,
    .approach_speed = 0.6f,
    .use_nukes = 1,
    .nuke_enemy_threshold = 4,
    .add_noise = 1,
    .noise_amount = 0.05f
};

const AIConfig AI_CONFIG_TEST = {
    .preferred_distance = 250.0f,
    .fire_angle_threshold = 30.0f,
    .danger_radius = 300.0f,
    .dodge_health_threshold = 400,
    .approach_speed = 0.8f,
    .use_nukes = 1,
    .nuke_enemy_threshold = 5,
    .add_noise = 0,
    .noise_amount = 0.0f
};

/*============================================================================
 * AI LIFECYCLE
 *============================================================================*/

void ai_init(AIController* ai, const AIConfig* config) {
    if (config) {
        ai->config = *config;
    } else {
        ai->config = AI_CONFIG_BALANCED;
    }
    ai_reset(ai);
}

void ai_reset(AIController* ai) {
    memset(&ai->state, 0, sizeof(AIState));
}

/*============================================================================
 * HELPER FUNCTIONS
 *============================================================================*/

/* Calculate angle difference in range [-128, 127] */
static int8_t angle_diff(uint8_t from, uint8_t to) {
    return (int8_t)(to - from);
}

/* Find the nearest enemy to the player */
static const Enemy* find_nearest_enemy(const Game* game, Vec2 player_pos, float* out_dist) {
    const Enemy* nearest = NULL;
    float nearest_dist_sq = 1e30f;

    POOL_FOREACH_CONST(&game->enemies, e, Enemy, {
        Vec2 delta = vec2_toroidal_delta(player_pos, e->position, MAP_WIDTH, MAP_HEIGHT);
        float dist_sq = vec2_length_sq(delta);

        if (dist_sq < nearest_dist_sq) {
            nearest_dist_sq = dist_sq;
            nearest = e;
        }
    });

    if (out_dist && nearest) {
        *out_dist = sqrtf(nearest_dist_sq);
    }

    return nearest;
}

/* Count enemies within a radius */
static int count_enemies_in_radius(const Game* game, Vec2 center, float radius) {
    int count = 0;
    float radius_sq = radius * radius;

    POOL_FOREACH_CONST(&game->enemies, e, Enemy, {
        Vec2 delta = vec2_toroidal_delta(center, e->position, MAP_WIDTH, MAP_HEIGHT);
        if (vec2_length_sq(delta) < radius_sq) {
            count++;
        }
    });

    return count;
}

/* Count incoming enemy projectiles that threaten the player */
static int count_threats(const Game* game, Vec2 player_pos, float radius) {
    int count = 0;
    float radius_sq = radius * radius;

    POOL_FOREACH_CONST(&game->particles, p, Particle, {
        if (p->collision_layer != COLLISION_ENEMY_OWNED &&
            p->collision_layer != COLLISION_NEUTRAL) continue;

        Vec2 delta = vec2_toroidal_delta(player_pos, p->position, MAP_WIDTH, MAP_HEIGHT);
        if (vec2_length_sq(delta) < radius_sq) {
            /* Check if projectile is heading toward player */
            float dot = vec2_dot(delta, p->velocity);
            if (dot > 0) {  /* Moving toward us */
                count++;
            }
        }
    });

    return count;
}

/*============================================================================
 * AI DECISION MAKING
 *============================================================================*/

void ai_think(AIController* ai, const Game* game, InputState* output) {
    memset(output, 0, sizeof(InputState));

    AIState* state = &ai->state;
    const AIConfig* config = &ai->config;
    const Player* player = &game->player;
    Vec2 player_pos = player->position;

    state->frames_played++;

    /* Find target */
    state->target = find_nearest_enemy(game, player_pos, &state->target_dist);

    if (!state->target) {
        /* No enemies - just wait */
        return;
    }

    state->target_pos = state->target->position;

    /* Assess threats */
    state->nearby_enemies = count_enemies_in_radius(game, player_pos, config->danger_radius);
    state->incoming_projectiles = count_threats(game, player_pos, 150.0f);

    /* Calculate angle to target */
    Vec2 to_target = vec2_toroidal_delta(player_pos, state->target_pos, MAP_WIDTH, MAP_HEIGHT);
    uint8_t target_angle = vec2_to_angle(to_target);
    int8_t turn_needed = angle_diff(player->angle, target_angle);

    /* DECISION: Should we nuke? */
    if (config->use_nukes &&
        player->nukes_remaining > 0 &&
        player->nuke_cooldown == 0 &&
        state->nearby_enemies >= config->nuke_enemy_threshold) {
        output->nuke = 1;
        state->nukes_dropped++;
    }

    /* DECISION: Should we dodge? */
    if (state->is_dodging) {
        /* Continue current dodge */
        state->dodge_frames--;
        if (state->dodge_frames <= 0) {
            state->is_dodging = 0;
        } else {
            /* Strafe in dodge direction */
            if (state->dodge_direction > 0) {
                output->strafe_right = 1;
            } else {
                output->strafe_left = 1;
            }
            /* Thrust to escape */
            output->up = 1;
            /* Also turn toward target while dodging */
            if (turn_needed > 0) output->right = 1;
            else if (turn_needed < 0) output->left = 1;
            output->fire = 1;  /* Keep firing while dodging */
            state->shots_fired++;
            return;
        }
    }

    /* Check if we should start dodging - more aggressive dodging */
    if (state->incoming_projectiles >= 2 ||
        (player->health < config->dodge_health_threshold && state->incoming_projectiles >= 1) ||
        state->target_dist < 150.0f) {  /* Also dodge when too close */
        state->is_dodging = 1;
        /* Alternate dodge direction based on frame for unpredictability */
        state->dodge_direction = ((game->frame_count / 20) % 2 == 0) ? 1 : -1;
        state->dodge_frames = 20;  /* Shorter dodges for more responsiveness */
    }

    /* DECISION: Turning */
    /* Only apply turning if we need to turn more than a small deadzone */
    int abs_turn = (turn_needed < 0) ? -turn_needed : turn_needed;
    if (abs_turn > 5) {
        if (turn_needed > 0) {
            output->right = 1;
        } else {
            output->left = 1;
        }
    }

    /* DECISION: Thrust/approach */
    float desired_dist = config->preferred_distance;

    /* Prioritize bosses - get closer */
    if (state->target->size == ENEMY_SIZE_BOSS) {
        desired_dist *= 0.7f;
    }

    if (state->target_dist > desired_dist + 50.0f) {
        /* Too far - approach */
        output->up = 1;
    } else if (state->target_dist < desired_dist - 50.0f) {
        /* Too close - back off (or strafe) */
        if (state->target_dist < 100.0f) {
            output->down = 1;  /* Emergency reverse */
        } else {
            /* Circle strafe */
            if (game->frame_count % 120 < 60) {
                output->strafe_right = 1;
            } else {
                output->strafe_left = 1;
            }
        }
    } else {
        /* Good distance - maintain with slight adjustments */
        if (game->frame_count % 60 < 30) {
            output->up = 1;
        }
    }

    /* DECISION: Firing */
    float fire_threshold = config->fire_angle_threshold;

    /* More generous aim for bosses */
    if (state->target->size == ENEMY_SIZE_BOSS) {
        fire_threshold *= 1.5f;
    }

    if ((float)abs_turn < fire_threshold) {
        output->fire = 1;
        state->shots_fired++;
    }

    /* Add noise if configured */
    if (config->add_noise && config->noise_amount > 0) {
        /* Use game's random state for reproducibility */
        uint32_t r = game->frame_count * 1664525 + 1013904223;
        float noise = ((float)(r & 0xFF) / 255.0f - 0.5f) * config->noise_amount;

        /* Occasionally flip inputs */
        if (noise > 0.4f) {
            output->left = output->right;
            output->right = !output->left;
        }
    }
}

void ai_print_stats(const AIController* ai) {
    printf("AI Statistics:\n");
    printf("  Frames played:    %u\n", ai->state.frames_played);
    printf("  Shots fired:      %u\n", ai->state.shots_fired);
    printf("  Nukes dropped:    %u\n", ai->state.nukes_dropped);
    printf("  Enemies killed:   %u\n", ai->state.enemies_killed);
    printf("  Damage taken:     %u\n", ai->state.damage_taken);
}

/*============================================================================
 * SELF-PLAY ORCHESTRATION
 *============================================================================*/

int ai_play_level(Game* game, int level_idx, AIController* ai, int max_frames) {
    ai_reset(ai);
    game_start_level(game, level_idx);

    InputState input;
    int prev_enemy_count = enemy_pool_count(&game->enemies);
    int prev_health = game->player.health;

    for (int frame = 0; frame < max_frames; frame++) {
        if (game->state == STATE_VICTORY) {
            return 1;  /* Won! */
        }
        if (game->state == STATE_DEFEAT) {
            return 0;  /* Lost */
        }

        /* AI thinks */
        ai_think(ai, game, &input);

        /* Update game */
        game_update(game, &input, 0.016f);

        /* Fire weapons if AI wants to */
        if (input.fire) {
            player_fire_weapons(game);
        }

        /* Check for nuke usage */
        if (input.nuke && game->player.nuke_cooldown == 0) {
            player_fire_nuke(game);
        }

        /* Body collisions (ramming) */
        collisions_check_bodies(game);

        /* Track statistics */
        int curr_enemy_count = enemy_pool_count(&game->enemies);
        if (curr_enemy_count < prev_enemy_count) {
            ai->state.enemies_killed += (prev_enemy_count - curr_enemy_count);
        }
        prev_enemy_count = curr_enemy_count;

        if (game->player.health < prev_health) {
            ai->state.damage_taken += (prev_health - game->player.health);
        }
        prev_health = game->player.health;
    }

    /* Timeout - treat as loss */
    return 0;
}

int ai_play_campaign(Game* game, AIController* ai, int max_frames_per_level) {
    int levels_beaten = 0;

    for (int level = 0; level < (int)LEVEL_COUNT; level++) {
        printf("AI playing level %d (%s)...\n", level, LEVELS[level].name);

        if (ai_play_level(game, level, ai, max_frames_per_level)) {
            levels_beaten++;
            printf("  VICTORY! (frames: %u, kills: %u)\n",
                   ai->state.frames_played, ai->state.enemies_killed);
        } else {
            printf("  DEFEAT. (frames: %u, health: %d)\n",
                   ai->state.frames_played, game->player.health);
            break;  /* Campaign over */
        }
    }

    return levels_beaten;
}

SelfPlayResult ai_run_full_test(AIConfig config, int max_frames_per_level) {
    SelfPlayResult result = {0};

    Game game;
    AIController ai;

    game_init(&game);
    ai_init(&ai, &config);

    for (int level = 0; level < (int)LEVEL_COUNT; level++) {
        result.levels_attempted++;

        if (ai_play_level(&game, level, &ai, max_frames_per_level)) {
            result.levels_won++;
            result.total_frames += ai.state.frames_played;
            result.total_enemies_killed += ai.state.enemies_killed;
        } else {
            result.levels_lost++;
            result.total_frames += ai.state.frames_played;
            result.total_enemies_killed += ai.state.enemies_killed;
            break;
        }
    }

    result.final_weapons_level = game.weapons_level;

    return result;
}
