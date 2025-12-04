/*
 * SSE-optimized memory operations
 * Rewritten from assembly to C
 *
 * This version mirrors the assembly implementation logic.
 * The assembly uses MMX/SSE instructions for performance,
 * while this C version provides the same algorithm for portability.
 */

#include "sse_mem.h"

/*
 * Copy memory - optimized for 32-bit (4-byte) aligned data
 * dest: destination pointer
 * src: source pointer
 * count: number of 32-bit values (not bytes) to copy
 */
void sseMemcpy32(void* dest, const void* src, unsigned long count) {
    /* Manual copy to mirror assembly implementation */
    uint32_t* d = (uint32_t*)dest;
    const uint32_t* s = (const uint32_t*)src;

    /* Process 16 dwords at a time (mirrors shr ecx, 4 and main loop) */
    while (count >= 16) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
        d[3] = s[3];
        d[4] = s[4];
        d[5] = s[5];
        d[6] = s[6];
        d[7] = s[7];
        d[8] = s[8];
        d[9] = s[9];
        d[10] = s[10];
        d[11] = s[11];
        d[12] = s[12];
        d[13] = s[13];
        d[14] = s[14];
        d[15] = s[15];

        d += 16;
        s += 16;
        count -= 16;
    }

    /* Handle tail - remaining dwords less than 16 (mirrors .COPYTAIL) */
    while (count > 0) {
        *d++ = *s++;
        count--;
    }
}

/*
 * Set memory - optimized for 32-bit (4-byte) aligned data
 * dest: destination pointer
 * value: 32-bit value to set
 * count: number of 32-bit values (not bytes) to set
 */
void sseMemset32(void* dest, uint32_t value, unsigned long count) {
    uint32_t* d = (uint32_t*)dest;

    /* Unrolled loop for better performance */
    while (count >= 16) {
        d[0] = value;
        d[1] = value;
        d[2] = value;
        d[3] = value;
        d[4] = value;
        d[5] = value;
        d[6] = value;
        d[7] = value;
        d[8] = value;
        d[9] = value;
        d[10] = value;
        d[11] = value;
        d[12] = value;
        d[13] = value;
        d[14] = value;
        d[15] = value;

        d += 16;
        count -= 16;
    }

    /* Handle remaining elements */
    while (count > 0) {
        *d++ = value;
        count--;
    }
}
