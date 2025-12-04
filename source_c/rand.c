/*
 * Mersenne Twister Random Number Generator
 * Rewritten in C from assembly version by Jun Mizutani
 * Based on original C code by Takuji Nishimura
 *
 * Copyright (C) 1997, 1999 Makoto Matsumoto and Takuji Nishimura.
 * This library is free software under GNU Library General Public License.
 */

#include "rand.h"

#define N 624
#define M 397

static uint32_t mt[N];
static uint32_t mag01[2] = {0x00000000, 0x9908b0df};
static uint32_t mti = N + 1;

/*
 * Initialize Mersenne Twister with seed
 */
void SeedRand(uint32_t seed) {
    uint32_t i;

    for (i = 0; i < N; i++) {
        uint32_t high = seed & 0xffff0000;
        seed = seed * 69069 + 1;
        uint32_t low = (seed & 0xffff0000) >> 16;
        mt[i] = high | low;
        seed = seed * 69069 + 1;
    }

    mti = N;
}

/*
 * Generate random number
 */
uint32_t Rand(void) {
    uint32_t y;
    uint32_t kk;

    if (mti >= N) {
        /* Generate N words at one time */
        for (kk = 0; kk < N - M; kk++) {
            y = (mt[kk] & 0x80000000) | (mt[kk + 1] & 0x7fffffff);
            mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1];
        }

        for (; kk < N - 1; kk++) {
            y = (mt[kk] & 0x80000000) | (mt[kk + 1] & 0x7fffffff);
            mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1];
        }

        y = (mt[N - 1] & 0x80000000) | (mt[0] & 0x7fffffff);
        mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1];

        mti = 0;
    }

    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680;
    y ^= (y << 15) & 0xefc60000;
    y ^= (y >> 18);

    return y;
}
