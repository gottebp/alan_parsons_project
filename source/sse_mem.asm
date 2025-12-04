;---------------------------------------------------------------------
;   Functions: _sseMemcpy32, _sseMemset32
;   File : sse_mem.inc
;   Written By: Andrew Gottemoller
;---------------------------------------------------------------------

; This library is free software; you can redistribute it and/or
; modify it under the terms of the GNU Library General Public
; License as published by the Free Software Foundation; either
; version 2 of the License, or (at your option) any later
; version.
; This library is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
; See the GNU Library General Public License for more details.

GLOBAL _sseMemcpy32, _sseMemset32

SECTION .text

;void sseMemcpy32 (void* dest, void* src, unsigned long count);
_sseMemcpy32:
    mov edi, [ebp+4]        ;U - load dest
    mov ecx, [ebp+12]        ;V - load dword count
    mov esi, [ebp+8]        ;U - load source

    mov edx, ecx                ;V - copy dword count
    shr ecx, 4            ;U - find 16 count sets
    jz .COPYTAILCALC        ;V - jump to tail if none

.COPY:
    movq mm0, [esi]                ;U -load 8bytes from source
    movq mm1, [esi+8]        ;V
    movq mm2, [esi+16]            ;U
    movq mm3, [esi+24]            ;V
    movq mm4, [esi+32]            ;U
    movq mm5, [esi+40]            ;V
    movq mm6, [esi+48]            ;U
    movq mm7, [esi+56]            ;V

    movntq [edi], mm0        ;U - write 8bytes to destination
    movntq [edi+8], mm1            ;V
    movntq [edi+16], mm2            ;U
    movntq [edi+24], mm3            ;V
    movntq [edi+32], mm4            ;U
    movntq [edi+40], mm5            ;V
    movntq [edi+48], mm6            ;U
    movntq [edi+56], mm7            ;V

    add esi, 64            ;U - add 64bytes to source pointer
    add edi, 64            ;V - add 64bytes to destination pointer

    dec ecx                    ;U - one less 16 count set
    jnz .COPY            ;V - jump if another remains

.COPYTAILCALC:
    and edx, 0000000Fh            ;U - get less than 16 count sets
    jz .COPYDONE            ;V - if none, finish

.COPYTAIL:
    movd mm0, [esi]                ;U - load 4bytes from source
    add esi, 4            ;V - add 4bytes to source
    movd [edi], mm0                ;U - save 4bytes to destination
    add edi, 4            ;V - add 4bytes to destination

    dec edx                    ;U - one less dword
    jnz .COPYTAIL            ;V - if remaining, loop again

.COPYDONE:
    emms                ;we used mmx here, must do this
    ret                ;all done


;void sseMemset32 (void* dest, unssigned long src, unsigned long dwords);
_sseMemset32:
    mov edi, [ebp+4]        ;U - load dest
    mov ecx, [ebp+12]        ;V - load dword count

    movd mm0, [ebp+8]        ;U - load src
    punpckldq mm0, mm0        ;V - extend it to 64bit

    mov edx, ecx            ;U - copy count
    shr ecx, 4            ;V - put ecx into 16count sets
    jz .COPYTAILCALC        ;N - if zero copy end

.COPY:
    movntq [edi], mm0        ;U - copy src
    movntq [edi+8], mm0        ;V
    movntq [edi+16], mm0        ;U
    movntq [edi+24], mm0        ;V
    movntq [edi+32], mm0        ;U
    movntq [edi+40], mm0        ;V
    movntq [edi+48], mm0        ;U
    movntq [edi+56], mm0        ;V

    add edi, 64            ;U - add 64 bytes to destination

    dec ecx                ;V - one less set
    jnz .COPY            ;N - if one remains loop again

.COPYTAILCALC:
    and edx, 0000000Fh        ;U - calc lower set
    jz .COPYDONE            ;V - if none quit

.COPYTAIL:
    movd [edi], mm0            ;U - save src
    add edi, 4            ;V - next 4 bytes

    dec edx                ;U - one less set
    jnz .COPYTAIL            ;V - if another loop

.COPYDONE:
    emms                ;mmx used, fix it
    ret                ;exit



