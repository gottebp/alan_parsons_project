/*
 * ai_player.h - AI Controller for Self-Play
 *
 * An artificial player that seeks and destroys enemies.
 * Useful for testing game balance, stress testing, and verification.
 *
 * The AI plays by these principles:
 *   1. Find the nearest enemy
 *   2. Turn to face it
 *   3. Thrust toward it (but maintain combat distance)
 *   4. Fire when the enemy is in front
 *   5. Use nukes when surrounded
 *   6. Dodge when in danger
 */

#ifndef AI_PLAYER_H
#define AI_PLAYER_H

#include "game.h"

/*============================================================================
 * AI CONFIGURATION
 *============================================================================*/

typedef struct {
    /* Combat behavior */
    float preferred_distance;   /* Try to maintain this distance from target */
    float fire_angle_threshold; /* Only fire if target is within this angle */
    float danger_radius;        /* Radius within which to check for threats */
    int dodge_health_threshold; /* Start dodging below this health */

    /* Aggression */
    float approach_speed;       /* How aggressively to close in */
    int use_nukes;              /* Should the AI use nukes? */
    int nuke_enemy_threshold;   /* Use nuke when this many enemies nearby */

    /* Randomization */
    int add_noise;              /* Add slight randomness to inputs */
    float noise_amount;         /* How much noise (0.0 - 1.0) */
} AIConfig;

/* Default AI configurations for different play styles */
extern const AIConfig AI_CONFIG_AGGRESSIVE;
extern const AIConfig AI_CONFIG_CAUTIOUS;
extern const AIConfig AI_CONFIG_BALANCED;
extern const AIConfig AI_CONFIG_TEST;

/*============================================================================
 * AI STATE
 *============================================================================*/

typedef struct {
    /* Current target */
    const Enemy* target;
    Vec2 target_pos;
    float target_dist;

    /* Threat assessment */
    int nearby_enemies;
    int incoming_projectiles;

    /* Movement state */
    int is_dodging;
    int dodge_direction;        /* -1 left, +1 right */
    int dodge_frames;           /* Frames remaining in current dodge */

    /* Statistics (for analysis) */
    uint32_t frames_played;
    uint32_t shots_fired;
    uint32_t nukes_dropped;
    uint32_t enemies_killed;
    uint32_t damage_taken;
} AIState;

/*============================================================================
 * AI CONTROLLER
 *============================================================================*/

typedef struct {
    AIConfig config;
    AIState state;
} AIController;

/* Initialize AI controller with a configuration */
void ai_init(AIController* ai, const AIConfig* config);

/* Reset AI state for new level */
void ai_reset(AIController* ai);

/* Generate input for this frame based on game state */
void ai_think(AIController* ai, const Game* game, InputState* output);

/* Print AI statistics */
void ai_print_stats(const AIController* ai);

/*============================================================================
 * HIGHER-LEVEL SELF-PLAY
 *============================================================================*/

typedef struct {
    int levels_attempted;
    int levels_won;
    int levels_lost;
    uint32_t total_frames;
    uint32_t total_enemies_killed;
    int final_weapons_level;
} SelfPlayResult;

/* Run AI through a single level, returns 1 if won, 0 if lost */
int ai_play_level(Game* game, int level_idx, AIController* ai, int max_frames);

/* Run AI through all levels in sequence, returns number of levels beaten */
int ai_play_campaign(Game* game, AIController* ai, int max_frames_per_level);

/* Run AI with statistics gathering */
SelfPlayResult ai_run_full_test(AIConfig config, int max_frames_per_level);

#endif /* AI_PLAYER_H */
