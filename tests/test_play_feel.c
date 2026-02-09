/*
 * test_play_feel.c - Telemetry Harness for Feeling the Physics
 *
 * This test simulates actual gameplay with rich telemetry output
 * to let us "feel" how the physics work and tune them.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include_c/game/game.h"
#include "../include_c/game/ai_player.h"

/*============================================================================
 * TELEMETRY OUTPUT
 *============================================================================*/

typedef struct {
    /* Player state */
    float player_speed;
    float player_strafe;
    float player_angular_vel;
    int player_health;
    int player_angle;

    /* Combat */
    int enemies_alive;
    int boss_alive;
    int boss_health;
    float nearest_enemy_dist;
    int player_projectiles;
    int enemy_projectiles;

    /* Damage events */
    int player_damage_taken;
    int enemies_killed;

    /* Physics feel */
    float player_velocity_mag;
    int collisions_this_frame;
} FrameTelemetry;

static void print_telemetry_header(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                         GAMEPLAY TELEMETRY HARNESS                             ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

static void print_player_status(const Game* game, int frame) {
    const Player* p = &game->player;
    float vel_mag = vec2_length(vec2_add(
        vec2_mul(vec2_from_angle(p->angle), p->speed),
        vec2_mul(vec2_perp(vec2_from_angle(p->angle)), p->strafe_speed)
    ));

    printf("┌─ FRAME %5d ──────────────────────────────────────────────────────────────────┐\n", frame);
    printf("│ PLAYER: pos(%.0f, %.0f) angle=%3d  health=%5d/%5d                          │\n",
           p->position.x, p->position.y, (int)(uint8_t)p->angle, p->health, PLAYER_MAX_HEALTH);
    printf("│         speed=%.2f strafe=%.2f angular_vel=%.2f  vel_mag=%.2f              │\n",
           p->speed, p->strafe_speed, p->angular_vel, vel_mag);
}

static void print_combat_status(const Game* game) {
    int enemies_alive = 0;
    int boss_alive = 0;
    int boss_health = 0;
    float nearest_dist = 99999.0f;
    const Player* p = &game->player;

    POOL_FOREACH_CONST(&game->enemies, e, Enemy, {
        enemies_alive++;
        if (e->size == ENEMY_SIZE_BOSS) {
            boss_alive = 1;
            boss_health = e->health;
        }
        float dist = vec2_toroidal_distance(p->position, e->position, MAP_WIDTH, MAP_HEIGHT);
        if (dist < nearest_dist) nearest_dist = dist;
    });

    int player_shots = 0;
    int enemy_shots = 0;
    POOL_FOREACH_CONST(&game->particles, part, Particle, {
        if (part->collision_layer == COLLISION_PLAYER_OWNED) player_shots++;
        else if (part->collision_layer == COLLISION_ENEMY_OWNED) enemy_shots++;
    });

    printf("│ COMBAT: enemies=%3d  boss=%s (hp=%5d)  nearest=%.0f                      │\n",
           enemies_alive, boss_alive ? "YES" : "NO ", boss_health, nearest_dist);
    printf("│         player_projectiles=%4d  enemy_projectiles=%4d                        │\n",
           player_shots, enemy_shots);
}

static void print_physics_feel(const char* action) {
    printf("│ FEEL:   %-70s │\n", action);
}

static void print_frame_end(void) {
    printf("└────────────────────────────────────────────────────────────────────────────────┘\n");
}

/*============================================================================
 * MANUAL CONTROL SIMULATION
 *============================================================================*/

/* Simulate specific input sequences to feel the physics */
static void feel_acceleration(Game* game) {
    printf("\n=== FEELING: Acceleration from standstill ===\n");
    InputState input = {0};

    for (int i = 0; i < 60; i++) {
        input.up = 1;  /* Hold thrust */
        input.fire = 1;

        game_update(game, &input, 1.0f/60.0f);

        if (i % 10 == 0) {
            Player* p = &game->player;
            printf("  t=%2d: speed=%.2f (max=%.1f) angular_vel=%.2f\n",
                   i, p->speed, p->max_speed, p->angular_vel);
        }
    }
    printf("  Final speed: %.2f (should approach max=%.1f)\n\n",
           game->player.speed, game->player.max_speed);
}

static void feel_turning(Game* game) {
    printf("=== FEELING: Turning with angular momentum ===\n");
    InputState input = {0};

    /* Apply turn for 30 frames, then release */
    for (int i = 0; i < 90; i++) {
        input.right = (i < 30) ? 1 : 0;

        game_update(game, &input, 1.0f/60.0f);

        if (i % 10 == 0) {
            Player* p = &game->player;
            printf("  t=%2d: angle=%3d angular_vel=%.2f %s\n",
                   i, (int)(uint8_t)p->angle, p->angular_vel,
                   (i < 30) ? "(turning)" : "(coasting)");
        }
    }
    printf("  Feeling: %s\n\n",
           game->player.angular_vel < 0.1f ? "Ship comes to smooth stop" : "Ship still spinning!");
}

static void feel_strafe_dodge(Game* game) {
    printf("=== FEELING: Strafe dodge maneuver ===\n");
    InputState input = {0};

    float start_x = game->player.position.x;

    /* Quick strafe left then back */
    for (int i = 0; i < 60; i++) {
        if (i < 15) input.strafe_left = 1;
        else if (i < 30) { input.strafe_left = 0; input.strafe_right = 1; }
        else { input.strafe_right = 0; }

        game_update(game, &input, 1.0f/60.0f);

        if (i % 15 == 0) {
            Player* p = &game->player;
            float displacement = p->position.x - start_x;
            printf("  t=%2d: strafe_speed=%.2f displacement=%.1f\n",
                   i, p->strafe_speed, displacement);
        }
    }
    printf("\n");
}

static void feel_friction_stop(Game* game) {
    printf("=== FEELING: Friction stopping ===\n");
    InputState input = {0};

    /* Give initial velocity */
    game->player.speed = 10.0f;
    game->player.strafe_speed = 8.0f;
    game->player.angular_vel = 4.0f;

    for (int i = 0; i < 120; i++) {
        game_update(game, &input, 1.0f/60.0f);

        if (i % 20 == 0) {
            Player* p = &game->player;
            printf("  t=%3d: speed=%.2f strafe=%.2f angular=%.2f\n",
                   i, p->speed, p->strafe_speed, p->angular_vel);
        }
    }
    float residual = fabsf(game->player.speed) + fabsf(game->player.strafe_speed) + fabsf(game->player.angular_vel);
    printf("  Residual motion: %.4f %s\n\n", residual, residual < 0.01f ? "(stopped clean)" : "(still drifting)");
}

/*============================================================================
 * COMBAT FEEL TESTS
 *============================================================================*/

static void feel_enemy_swarm(Game* game) {
    printf("=== FEELING: Enemy swarm behavior ===\n");

    /* Spawn a cluster of enemies */
    game_start_level(game, 0);  /* Shire */

    /* Skip to first wave */
    InputState input = {0};
    input.fire = 1;

    int enemies_spawned = 0;
    for (int i = 0; i < 1000 && enemies_spawned == 0; i++) {
        game_update(game, &input, 1.0f/60.0f);
        POOL_FOREACH_CONST(&game->enemies, e, Enemy, { (void)e; enemies_spawned++; });
    }

    printf("  Enemies spawned: %d\n", enemies_spawned);

    /* Watch them swarm toward us */
    Vec2 prev_centroid = vec2_zero();
    for (int i = 0; i < 180; i++) {
        game_update(game, &input, 1.0f/60.0f);

        if (i % 30 == 0) {
            Vec2 centroid = vec2_zero();
            int count = 0;
            POOL_FOREACH_CONST(&game->enemies, e, Enemy, {
                centroid = vec2_add(centroid, e->position);
                count++;
            });
            if (count > 0) {
                centroid = vec2_div(centroid, (float)count);
                float dist_to_player = vec2_toroidal_distance(centroid, game->player.position, MAP_WIDTH, MAP_HEIGHT);
                float approach_speed = (i > 0) ? vec2_distance(prev_centroid, centroid) : 0;
                printf("  t=%3d: %d enemies, centroid dist=%.0f, approach=%.1f/frame\n",
                       i, count, dist_to_player, approach_speed / 30.0f);
                prev_centroid = centroid;
            }
        }
    }
    printf("\n");
}

static void feel_enemy_orbiting(Game* game) {
    printf("=== FEELING: Enemy orbiting behavior ===\n");

    game_start_level(game, 0);

    /* Spawn a single enemy right at inner radius */
    Enemy* e = enemy_spawn(game, ENEMY_STANDARD, ENEMY_SIZE_SMALL,
                           vec2_add(game->player.position, vec2(ENEMY_INNER_RADIUS + 50, 0)), 40.0f);

    if (!e) {
        printf("  Failed to spawn test enemy\n\n");
        return;
    }

    InputState input = {0};

    for (int i = 0; i < 300; i++) {
        game_update(game, &input, 1.0f/60.0f);

        if (i % 50 == 0 && e->active) {
            float dist = vec2_toroidal_distance(e->position, game->player.position, MAP_WIDTH, MAP_HEIGHT);
            float vel_mag = vec2_length(e->velocity);
            printf("  t=%3d: dist=%.0f (inner=%d) vel=%.2f angle=%d\n",
                   i, dist, ENEMY_INNER_RADIUS, vel_mag, (int)(uint8_t)e->angle);
        }
    }
    printf("  Expected: Enemy should orbit around inner radius, not crash into player\n\n");
}

static void feel_projectile_dodge(Game* game) {
    printf("=== FEELING: Dodging incoming projectiles ===\n");

    game_start_level(game, 0);

    /* Spawn an enemy that will shoot at us */
    Enemy* e = enemy_spawn(game, ENEMY_SCOUT, ENEMY_SIZE_SMALL,
                           vec2_add(game->player.position, vec2(300, 0)), 35.0f);
    e->angle = 128; /* Face player */

    InputState input = {0};
    int hit = 0;
    int initial_health = game->player.health;

    for (int i = 0; i < 300; i++) {
        /* Dodge pattern: strafe left/right */
        if ((i / 30) % 2 == 0) input.strafe_left = 1, input.strafe_right = 0;
        else input.strafe_left = 0, input.strafe_right = 1;

        int prev_health = game->player.health;
        game_update(game, &input, 1.0f/60.0f);

        if (game->player.health < prev_health) {
            hit++;
        }

        if (i % 60 == 0) {
            int proj_count = 0;
            POOL_FOREACH_CONST(&game->particles, p, Particle, {
                if (p->collision_layer == COLLISION_ENEMY_OWNED) proj_count++;
            });
            printf("  t=%3d: health=%d enemy_projectiles=%d\n",
                   i, game->player.health, proj_count);
        }
    }

    int damage = initial_health - game->player.health;
    printf("  Result: took %d damage from %d hits\n", damage, hit);
    printf("  Feel: %s\n\n", damage < 100 ? "Dodging feels effective!" : "Getting hit too much");
}

/*============================================================================
 * BOSS FIGHT FEEL
 *============================================================================*/

static void feel_boss_fight(Game* game) {
    printf("=== FEELING: Boss fight (AI controlled) ===\n");

    game_start_level(game, 0);

    /* Spawn a boss */
    const LevelDef* level = &LEVELS[0];
    Enemy* boss = enemy_spawn(game, level->boss_type, ENEMY_SIZE_BOSS,
                              vec2(level->boss_x, level->boss_y), level->boss_mass);

    if (!boss) {
        printf("  Failed to spawn boss\n\n");
        return;
    }

    int boss_initial_health = boss->health;
    int player_initial_health = game->player.health;

    printf("  Boss: type=%d health=%d mass=%.0f\n", boss->type, boss->health, boss->mass);

    /* Use the AI to fight */
    AIController ai;
    ai_init(&ai, &AI_CONFIG_AGGRESSIVE);

    int frame = 0;
    while (boss->active && frame < 3600) {  /* 60 seconds max */
        InputState input = {0};
        ai_think(&ai, game, &input);

        game_update(game, &input, 1.0f/60.0f);

        if (frame % 120 == 0) {
            float dist = vec2_toroidal_distance(boss->position, game->player.position, MAP_WIDTH, MAP_HEIGHT);
            int player_dmg = player_initial_health - game->player.health;
            int boss_dmg = boss_initial_health - boss->health;
            printf("  t=%4d: boss_hp=%5d (-%d) player_hp=%5d (-%d) dist=%.0f\n",
                   frame, boss->health, boss_dmg, game->player.health, player_dmg, dist);
        }
        frame++;
    }

    if (!boss->active) {
        printf("  BOSS DEFEATED in %d frames (%.1f seconds)!\n", frame, frame/60.0f);
    } else {
        printf("  Boss survived with %d hp\n", boss->health);
    }
    printf("  Player took %d damage\n\n", player_initial_health - game->player.health);
}

/*============================================================================
 * FULL COMBAT SIMULATION WITH TELEMETRY
 *============================================================================*/

static void play_with_telemetry(Game* game, int level_idx, int max_frames) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                    PLAYING: %-10s with full telemetry                    ║\n", LEVELS[level_idx].name);
    printf("╚════════════════════════════════════════════════════════════════════════════════╝\n\n");

    game_start_level(game, level_idx);

    AIController ai;
    ai_init(&ai, &AI_CONFIG_BALANCED);

    int peak_enemies = 0;
    int total_kills = 0;
    int waves_seen = 0;
    int boss_seen = 0;
    int last_wave = -1;

    for (int frame = 0; frame < max_frames && game->state == STATE_PLAYING; frame++) {
        InputState input = {0};
        ai_think(&ai, game, &input);

        int prev_enemies = 0;
        POOL_FOREACH_CONST(&game->enemies, e, Enemy, { (void)e; prev_enemies++; });

        game_update(game, &input, 1.0f/60.0f);

        int curr_enemies = 0;
        int boss_alive = 0;
        POOL_FOREACH_CONST(&game->enemies, e, Enemy, {
            curr_enemies++;
            if (e->size == ENEMY_SIZE_BOSS) boss_alive = 1;
        });

        if (curr_enemies > peak_enemies) peak_enemies = curr_enemies;
        if (prev_enemies > curr_enemies) total_kills += (prev_enemies - curr_enemies);
        if (game->waves.current_wave != last_wave) {
            waves_seen++;
            last_wave = game->waves.current_wave;
        }
        if (boss_alive && !boss_seen) {
            boss_seen = 1;
            printf("  >>> BOSS SPAWNED at frame %d <<<\n", frame);
        }

        /* Print status every 5 seconds */
        if (frame % 300 == 0) {
            Player* p = &game->player;
            float vel = vec2_length(vec2_add(
                vec2_mul(vec2_from_angle(p->angle), p->speed),
                vec2_mul(vec2_perp(vec2_from_angle(p->angle)), p->strafe_speed)
            ));

            int player_proj = 0, enemy_proj = 0;
            POOL_FOREACH_CONST(&game->particles, part, Particle, {
                if (part->collision_layer == COLLISION_PLAYER_OWNED) player_proj++;
                else if (part->collision_layer == COLLISION_ENEMY_OWNED) enemy_proj++;
            });

            printf("  [%5.1fs] HP:%5d Enemies:%3d Wave:%d Kills:%3d Vel:%.1f ProjP:%3d ProjE:%3d\n",
                   frame/60.0f, p->health, curr_enemies, game->waves.current_wave,
                   total_kills, vel, player_proj, enemy_proj);
        }
    }

    printf("\n  ─── RESULT ───\n");
    printf("  Outcome: %s\n", game->state == STATE_VICTORY ? "VICTORY!" :
                              game->state == STATE_DEFEAT ? "DEFEAT" : "TIMEOUT");
    printf("  Final health: %d\n", game->player.health);
    printf("  Waves completed: %d\n", waves_seen);
    printf("  Peak enemies: %d\n", peak_enemies);
    printf("  Total kills: %d\n", total_kills);
    printf("  Boss encountered: %s\n\n", boss_seen ? "Yes" : "No");
}

/*============================================================================
 * PHYSICS TUNING ANALYSIS
 *============================================================================*/

static void analyze_current_physics(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                         CURRENT PHYSICS PARAMETERS                              ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════════════╝\n\n");

    printf("  PLAYER MOVEMENT:\n");
    printf("    acceleration:     %.2f\n", PLAYER_ACCELERATION);
    printf("    friction:         %.2f\n", PLAYER_FRICTION);
    printf("    max_speed:        %.1f\n", PLAYER_MAX_SPEED);
    printf("    min_speed:        %.1f (reverse)\n", PLAYER_MIN_SPEED);
    printf("    max_strafe:       %.1f\n", PLAYER_MAX_STRAFE);
    printf("    strafe_friction:  %.2f\n", PLAYER_STRAFE_FRICTION);
    printf("\n");
    printf("  PLAYER ROTATION:\n");
    printf("    angular_accel:    %.2f\n", PLAYER_ANGULAR_ACCEL);
    printf("    angular_friction: %.2f\n", PLAYER_ANGULAR_FRICTION);
    printf("    max_angular_vel:  %.1f\n", PLAYER_MAX_ANGULAR_VEL);
    printf("\n");
    printf("  ENEMY MOVEMENT:\n");
    printf("    gravity_constant: %.1f\n", ENEMY_GRAVITY_CONSTANT);
    printf("    inner_radius:     %d (stop approaching)\n", ENEMY_INNER_RADIUS);
    printf("    outer_radius:     %d (AI activation)\n", ENEMY_OUTER_RADIUS);
    printf("    repulsion_radius: %.1f\n", ENEMY_REPULSION_RADIUS);
    printf("    repulsion_force:  %.1f\n", ENEMY_REPULSION_FORCE);
    printf("\n");
    printf("  ENEMY ANGULAR:\n");
    printf("    angular_accel:    %.2f\n", ENEMY_ANGULAR_ACCEL);
    printf("    angular_friction: %.2f\n", ENEMY_ANGULAR_FRICTION);
    printf("    max_turn_rate:    %d\n", ENEMY_MAX_TURN_RATE);
    printf("\n");

    /* Calculate derived values */
    float time_to_max_speed = PLAYER_MAX_SPEED / (PLAYER_ACCELERATION - PLAYER_FRICTION);
    float stopping_distance = (PLAYER_MAX_SPEED * PLAYER_MAX_SPEED) / (2 * PLAYER_FRICTION);
    float time_to_max_angular = PLAYER_MAX_ANGULAR_VEL / (PLAYER_ANGULAR_ACCEL - PLAYER_ANGULAR_FRICTION);

    printf("  DERIVED VALUES:\n");
    printf("    Time to max speed:    ~%.1f frames\n", time_to_max_speed);
    printf("    Stopping distance:    ~%.0f units\n", stopping_distance);
    printf("    Time to max rotation: ~%.1f frames\n", time_to_max_angular);
    printf("\n");
}

/*============================================================================
 * MAIN
 *============================================================================*/

int main(void) {
    Game game;
    game_init(&game);
    game_rand_seed(&game, 12345);

    print_telemetry_header();
    analyze_current_physics();

    /* Feel individual mechanics */
    printf("╔════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          FEELING THE PHYSICS                                   ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════════════╝\n\n");

    game_init(&game);
    game_start_level(&game, 0);
    feel_acceleration(&game);

    game_init(&game);
    game_start_level(&game, 0);
    feel_turning(&game);

    game_init(&game);
    game_start_level(&game, 0);
    feel_strafe_dodge(&game);

    game_init(&game);
    game_start_level(&game, 0);
    feel_friction_stop(&game);

    /* Feel combat */
    printf("╔════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          FEELING THE COMBAT                                    ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════════════╝\n\n");

    game_init(&game);
    feel_enemy_swarm(&game);

    game_init(&game);
    feel_enemy_orbiting(&game);

    game_init(&game);
    feel_projectile_dodge(&game);

    game_init(&game);
    feel_boss_fight(&game);

    /* Play campaign with weapon upgrades */
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                         FULL CAMPAIGN TELEMETRY                                ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════════════╝\n\n");

    game_init(&game);
    game_rand_seed(&game, 12345);

    int weapons = 0;
    for (int level = 0; level < 6; level++) {
        printf("  [Campaign] Starting level %d (%s) with weapons_level=%d\n\n",
               level, LEVELS[level].name, weapons);

        /* Set weapons level before starting level */
        game.weapons_level = weapons;
        game_start_level(&game, level);

        /* Run the AI controller for this level */
        AIController ai;
        ai_init(&ai, &AI_CONFIG_BALANCED);

        int frame = 0;
        while (game.state == STATE_PLAYING && frame < 18000) {
            InputState input = {0};
            ai_think(&ai, &game, &input);
            game_update(&game, &input, 1.0f/60.0f);
            frame++;

            if (frame % 600 == 0) {
                int enemies = 0;
                POOL_FOREACH_CONST(&game.enemies, e, Enemy, { (void)e; enemies++; });
                printf("  [%5.1fs] HP:%5d Enemies:%3d Wave:%d\n",
                       frame/60.0f, game.player.health, enemies, game.waves.current_wave);
            }
        }

        printf("\n  ─── %s RESULT ───\n", LEVELS[level].name);
        printf("  Outcome: %s\n", game.state == STATE_VICTORY ? "VICTORY!" : "DEFEAT");
        printf("  Final health: %d\n\n", game.player.health);

        /* If we won, upgrade weapons */
        if (game.state == STATE_VICTORY) {
            weapons++;
            printf("  >>> WEAPON UPGRADE to level %d! <<<\n\n", weapons);
        } else {
            printf("  >>> Campaign ended at level %d <<<\n\n", level);
            break;
        }
    }

    printf("╔════════════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                          TELEMETRY COMPLETE                                    ║\n");
    printf("╚════════════════════════════════════════════════════════════════════════════════╝\n");

    return 0;
}
