/*
 * weapons.c - Weapon Systems
 *
 * All firing logic for players and enemies.
 * Data-driven where possible, explicit where clarity matters.
 */

#include "../../include_c/game/game.h"

/* Enemy firing speeds (from original) */
static const float ENEMY_FIRE_SPEEDS[] = {
    12.6f, 14.5331f, 16.31f, 18.2f, 7.3112f
};

/* Player weapon speeds by level */
static const float PLAYER_SPEEDS[] = {
    9.2f, 10.32f, 12.88f, 14.742f, 15.8821f, 16.2332f, 17.1243f, 18.533f
};

/* Weapon timers for rate-limited shots */
static int weapon_timers[10] = {0};
static int8_t weapon_mod_angle = 0;
static int8_t wma_direction = 0;

/*============================================================================
 * PLAYER WEAPONS
 *============================================================================*/

void player_fire_weapons(Game* game) {
    Player* p = &game->player;
    if (p->health <= 0) return;

    Vec2 nose = p->nose;
    float base_speed = p->speed;

    /* Level 0: Base weapon - always fires */
    float speed0 = base_speed + PLAYER_SPEEDS[1];
    particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_SMALL, 10,
                   nose, p->angle, speed0, 30, 12);

    if (p->weapons_level < 1) return;

    /* Level 1: Spread shot */
    float speed1 = base_speed + PLAYER_SPEEDS[3];
    int spread = (int)game_rand_range(game, 10) - 5;
    particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_SMALL, 7,
                   nose, p->angle + spread, speed1, 30, 4);

    if (p->weapons_level < 2) return;

    /* Level 2: Side shots (every 4th frame) */
    if (weapon_timers[0] >= 4) weapon_timers[0] = 0;
    weapon_timers[0]++;

    if (weapon_timers[0] <= 1) {
        float speed2 = base_speed + PLAYER_SPEEDS[0];
        particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_LARGE, 6,
                       nose, p->angle + 20, speed2, 20, 1);
        particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_LARGE, 6,
                       nose, p->angle - 20, speed2, 20, 1);
    }

    if (p->weapons_level < 3) return;

    /* Level 3: Sweeping shots */
    float speed3 = base_speed + PLAYER_SPEEDS[0];

    if (wma_direction == 0) weapon_mod_angle++;
    if (wma_direction == 1) weapon_mod_angle--;

    if (weapon_mod_angle >= 16) wma_direction = 1;
    if (weapon_mod_angle <= -16) wma_direction = 0;

    particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_SMALL, 12,
                   nose, p->angle + weapon_mod_angle, speed3, 30, 4);
    particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_SMALL, 12,
                   nose, p->angle - weapon_mod_angle, speed3, 30, 4);

    if (p->weapons_level < 4) return;

    /* Level 4: Perpendicular shots (every 10th frame) */
    if (weapon_timers[4] >= 10) weapon_timers[4] = 0;
    weapon_timers[4]++;

    if (weapon_timers[4] <= 8) {
        particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_SMALL, 15,
                       nose, p->angle + 64, PLAYER_SPEEDS[0], 22, 3);
        particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_SMALL, 15,
                       nose, p->angle - 64, PLAYER_SPEEDS[0], 22, 3);
    }

    if (p->weapons_level < 5) return;

    /* Level 5: Shockwave (every 40th frame) */
    if (weapon_timers[8] >= 40) weapon_timers[8] = 0;
    weapon_timers[8]++;

    if (weapon_timers[8] <= 1) {
        for (int angle = 0; angle < 256; angle += 8) {
            particle_spawn(game, COLLISION_PLAYER_OWNED, PARTICLE_SIZE_SMALL, 6,
                           p->position, angle, PLAYER_SPEEDS[4], 10, 2);
        }
    }
}

void player_fire_nuke(Game* game) {
    Player* p = &game->player;
    if (p->health <= 0) return;
    if (p->nuke_cooldown > 0) return;
    if (p->nukes_remaining <= 0) return;

    p->nukes_remaining--;
    p->nuke_cooldown = PLAYER_NUKE_COOLDOWN;

    effect_nuke(game, p->position);
    game->audio_nuke_fired = 1;  /* Signal for audio */
}

/*============================================================================
 * PLAYER THRUSTER EFFECTS
 *============================================================================*/

void player_fire_main_thrusters(Game* game) {
    Player* p = &game->player;
    if (p->health <= 0) return;

    float thruster_vel = p->speed - THRUSTER_MAIN_SPEED;

    int spread1 = (int)game_rand_range(game, 10) - 5;
    particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_LARGE, 6,
                   p->tail, p->angle + spread1, thruster_vel, 16, 0);

    int spread2 = (int)game_rand_range(game, 10) - 5;
    particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_LARGE, 7,
                   p->tail, p->angle + spread2, thruster_vel, 21, 0);
}

void player_fire_left_thrusters(Game* game) {
    Player* p = &game->player;
    if (p->health <= 0) return;

    particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL, 11,
                   p->position, p->angle + 64, THRUSTER_STRAFE_SPEED, 2, 0);
}

void player_fire_right_thrusters(Game* game) {
    Player* p = &game->player;
    if (p->health <= 0) return;

    particle_spawn(game, COLLISION_NONE, PARTICLE_SIZE_SMALL, 11,
                   p->position, p->angle - 64, THRUSTER_STRAFE_SPEED, 2, 0);
}

/*============================================================================
 * ENEMY WEAPONS
 *============================================================================*/

void enemy_small_fire(Game* game, Enemy* e) {
    const SmallEnemyFireDef* def = enemy_fire_def(e->type);

    /*
     * ORIGINAL: All small enemies fired at ~50% per frame (rand & 0xFF >= 0x7F)
     * The type-differentiated rates were an invention that made enemies passive.
     * Use unified ENEMY_SMALL_FIRE_RATE constant.
     */
    if (game_rand_range(game, 100) >= ENEMY_SMALL_FIRE_RATE) {
        return;
    }

    uint8_t angle = e->angle;
    int flare_idx = (int)e->type + 4;
    float speed = ENEMY_FIRE_SPEEDS[0] * def->speed_mult;

    /* Fire center shot */
    Vec2 vel = vec2_add(e->velocity, vec2_mul(vec2_from_angle(angle), speed));
    particle_spawn_vec(game, COLLISION_ENEMY_OWNED, def->size,
                       flare_idx, e->position, vel, def->lifetime, def->damage);

    /* Fire spread shots if this enemy type uses them (hunters only) */
    if (def->spread_count > 1) {
        int side_damage = def->damage * 3 / 4;  /* Spread shots do 75% damage */
        int side_lifetime = def->lifetime - 10;

        /* Left shot */
        Vec2 vel_left = vec2_add(e->velocity,
            vec2_mul(vec2_from_angle(angle - def->spread_angle), speed));
        particle_spawn_vec(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_SMALL,
                           flare_idx, e->position, vel_left, side_lifetime, side_damage);

        /* Right shot */
        Vec2 vel_right = vec2_add(e->velocity,
            vec2_mul(vec2_from_angle(angle + def->spread_angle), speed));
        particle_spawn_vec(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_SMALL,
                           flare_idx, e->position, vel_right, side_lifetime, side_damage);
    }
}

void enemy_boss_fire(Game* game, Enemy* e) {
    uint8_t angle = e->angle;
    int fire_rate = boss_fire_rate(e->type);

    if (e->type == ENEMY_HUNTER) {
        /* SHIMDOG - special pattern */
        if (game_rand_range(game, 100) < (uint32_t)fire_rate) {
            /* 4 cardinal direction shots */
            particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 9,
                          e->position, angle, ENEMY_FIRE_SPEEDS[0], 30, 10);
            particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 9,
                          e->position, angle + 64, ENEMY_FIRE_SPEEDS[0], 30, 10);
            particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 9,
                          e->position, angle + 128, ENEMY_FIRE_SPEEDS[0], 30, 10);
            particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 9,
                          e->position, angle - 64, ENEMY_FIRE_SPEEDS[0], 30, 10);

            /* Rare shockwave */
            if (game_rand_range(game, 100) < (uint32_t)boss_shockwave_chance(e->type)) {
                for (int a = 0; a < 256; a++) {
                    particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 9,
                                  e->position, a, ENEMY_FIRE_SPEEDS[4], 100, 1);
                }
            }
        }
    } else {
        /* Regular bosses - progressive difficulty */
        if (game_rand_range(game, 100) < (uint32_t)fire_rate) {
            /* Main shot */
            particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 5,
                          e->position, angle, ENEMY_FIRE_SPEEDS[1], 30, 4);

            /* Type 0: 5-way spread */
            if (e->type == ENEMY_SCOUT) {
                for (int offset = 15; offset <= 30; offset += 15) {
                    particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 5,
                                  e->position, angle + offset, ENEMY_FIRE_SPEEDS[1], 30, 4);
                    particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 5,
                                  e->position, angle - offset, ENEMY_FIRE_SPEEDS[1], 30, 4);
                }
            }

            /* Type 1+: Side shots + fast center + burst */
            if (e->type >= ENEMY_STANDARD) {
                particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 7,
                              e->position, angle + 10, ENEMY_FIRE_SPEEDS[1], 30, 6);
                particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 7,
                              e->position, angle - 10, ENEMY_FIRE_SPEEDS[1], 30, 6);
                particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_SMALL, 8,
                              e->position, angle, ENEMY_FIRE_SPEEDS[3], 30, 10);

                /* Type 1 unique: burst pattern */
                if (e->type == ENEMY_STANDARD) {
                    particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 7,
                                  e->position, angle + 5, ENEMY_FIRE_SPEEDS[2], 35, 5);
                    particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 7,
                                  e->position, angle - 5, ENEMY_FIRE_SPEEDS[2], 35, 5);
                }
            }

            /* Type 2+: Extra shots + shockwave */
            if (e->type >= ENEMY_TANK) {
                particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 5,
                              e->position, angle + 20, ENEMY_FIRE_SPEEDS[1], 30, 6);
                particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_LARGE, 5,
                              e->position, angle - 20, ENEMY_FIRE_SPEEDS[1], 30, 6);

                /* Shockwave chance */
                if (game_rand_range(game, 100) < (uint32_t)boss_shockwave_chance(e->type)) {
                    for (int a = 0; a < 256; a++) {
                        particle_spawn(game, COLLISION_ENEMY_OWNED, PARTICLE_SIZE_SMALL, 8,
                                      e->position, a, ENEMY_FIRE_SPEEDS[4], 100, 1);
                    }
                }
            }
        }
    }
}
