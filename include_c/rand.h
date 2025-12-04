#ifndef _RAND_H_
#define _RAND_H_

#include <stdint.h>

/* Mersenne Twister Random Number Generator */

void SeedRand(uint32_t seed);
uint32_t Rand(void);

#endif /* _RAND_H_ */
