/*
 * menu_stub.c - Stub for menu functions in tests
 */

#include "../../include_c/game/game.h"

/* Stub menu functions - tests don't need actual menu rendering */

void menu_enter(Game* game) {
    game->state = STATE_MENU;
    game->menu.phase = MENU_PHASE_FADE_IN;
}

int menu_update(Game* game, const InputState* input) {
    (void)input;
    /* Immediately complete - tests can override menu_result if needed */
    game->menu.phase = MENU_PHASE_DONE;
    return 1;
}

void menu_render(const Game* game) {
    (void)game;
    /* No-op for tests */
}
