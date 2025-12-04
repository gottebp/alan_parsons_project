#ifndef _SSE_MEM_H_
#define _SSE_MEM_H_

#include <stdint.h>

/*
 * SSE-optimized memory operations
 * These functions operate on 32-bit (4-byte) data
 */

void sseMemcpy32(void* dest, const void* src, unsigned long count);
void sseMemset32(void* dest, uint32_t value, unsigned long count);

#endif /* _SSE_MEM_H_ */
