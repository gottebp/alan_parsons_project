/*
 * Map Engine - handles map loading and rendering
 * Converted from x86 assembly
 */

#include "mapeng.h"
#include "defs.h"
#include "sdl_wrapper.h"
#include "sse_mem.h"
#include "rand.h"
#include "player.h"
#include <stdio.h>
#include <stdlib.h>

/* Map data */
uint32_t* MapOff = NULL;
uint32_t* MapPadTop = NULL;
uint32_t* MapPadBottom = NULL;
int intShakeMap = 0;

/*
 * Initialize map engine
 */
void InitMapEngine(void) {
    /* Allocate map memory with padding */
    size_t total_size = (MAP_WIDTH * MAP_HEIGHT * 4) + (MAP_WIDTH * 2 * 4);

    MapPadTop = (uint32_t*)malloc(total_size);
    if (!MapPadTop) {
        fprintf(stderr, "Memory allocation error in InitMapEngine\n");
        return;
    }

    /* Zero out the map */
    sseMemset32(MapPadTop, 0, MAP_WIDTH * MAP_HEIGHT + MAP_WIDTH * 2);

    /* Set up pointers */
    MapOff = MapPadTop + MAP_WIDTH;
    MapPadBottom = MapOff + (MAP_WIDTH * MAP_HEIGHT);

    intShakeMap = 0;
}

/*
 * Destroy map engine
 */
void DestroyMapEngine(void) {
    if (MapPadTop) {
        free(MapPadTop);
        MapPadTop = NULL;
        MapOff = NULL;
        MapPadBottom = NULL;
    }
}

/*
 * Load map from BMP file
 */
int LoadMap(const char* filename) {
    if (!MapOff) {
        fprintf(stderr, "Map engine not initialized\n");
        return -1;
    }

    /* Load BMP into map data */
    if (LoadBMP(MapOff, filename) < 0) {
        return -1;
    }

    /* Copy top row to padding */
    sseMemcpy32(MapPadTop, MapOff, MAP_WIDTH);

    /* Copy bottom row to padding */
    uint32_t* bottom_row = MapOff + (MAP_WIDTH * (MAP_HEIGHT - 1));
    sseMemcpy32(MapPadBottom, bottom_row, MAP_WIDTH);

    return 0;
}

/*
 * Render map to screen centered on player - mirrors assembly implementation
 */
void RenderMap(void) {
    extern uint32_t* ScreenOff;
    extern int intPlayerX, intPlayerY;

    /* Apply screen shake effect - mirrors assembly lines 154-188 */
    if (intShakeMap > 0) {
        /* Modify player position DIRECTLY for shake effect */
        int shake_x = (int)(Rand() % SHAKE_FACTOR) - (SHAKE_FACTOR / 2);
        int shake_y = (int)(Rand() % SHAKE_FACTOR) - (SHAKE_FACTOR / 2);

        intPlayerX += shake_x;
        intPlayerY += shake_y;

        /* Wrap player position if needed - mirrors assembly lines 175-185 */
        if (intPlayerX >= MAP_WIDTH) {
            intPlayerX = 0;
        }
        if (intPlayerY >= MAP_HEIGHT) {
            intPlayerY = 0;
        }

        intShakeMap--;
    }

    /* Calculate boundary offsets for toroidal wrapping - mirrors assembly lines 193-215 */
    int top_offset = (intPlayerX - (SCREEN_WIDTH / 2)) * 4;
    int bottom_offset = ((MAP_HEIGHT - 1) * MAP_WIDTH + intPlayerX - (SCREEN_WIDTH / 2)) * 4;

    /* Calculate starting map position - mirrors assembly line 207-215 */
    int map_offset = ((intPlayerY - (SCREEN_HEIGHT / 2)) * MAP_WIDTH + intPlayerX - (SCREEN_WIDTH / 2)) * 4;

    /* Render each scanline with toroidal wrapping - mirrors assembly lines 217-247 */
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        /* Map wrapping - mirrors assembly lines 219-230 */
        if (map_offset < top_offset) {
            map_offset += (MAP_WIDTH * MAP_HEIGHT) * 4;
        }
        if (map_offset > bottom_offset) {
            map_offset -= (MAP_WIDTH * MAP_HEIGHT) * 4;
        }

        /* Copy scanline from map to screen */
        uint32_t* src = (uint32_t*)((uint8_t*)MapOff + map_offset);
        uint32_t* dst = ScreenOff + (y * SCREEN_WIDTH);

        sseMemcpy32(dst, src, SCREEN_WIDTH);

        /* Move to next scanline */
        map_offset += MAP_WIDTH * 4;
    }
}

/*
 * Trigger screen shake effect
 */
void ShakeMap(int frames) {
    intShakeMap = frames;
}
