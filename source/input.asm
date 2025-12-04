
;============================================================
; Project:      "The Alan Parsons Project"
; Description:  Code for ece291 final project
; Author:       Benjamin Gottemoller
; Website:      http://www.particlefield.com
; Date:        7/20/02
;============================================================


%include "defs.inc"
%include "macros.inc"
%include "sse_mem.inc"


BITS    32

    EXTERN main
    EXTERN SDL_PumpEvents, SDL_GetMouseState, SDL_GetKeyState, SDL_PollEvent
    GLOBAL _UpdateInput
    GLOBAL _FlushKeyboard
    GLOBAL _KEYBOARD
    GLOBAL _MOUSE_X, _MOUSE_Y, _MOUSE_LBUTTON, _MOUSE_RBUTTON, _MOUSE_MBUTTON, _MOUSE_BUTTON
    GLOBAL _QUIT_SIGNAL

SECTION .bss

    _KBPtr                 resd      1
    _KEYBOARD                   resb      320

SECTION .data

    _SDLEvent                         db    20

    _MOUSE_X                     dw      0
    _MOUSE_Y                     dw      0
    _MOUSE_LBUTTON             dw      0
    _MOUSE_RBUTTON             dw     0
    _MOUSE_MBUTTON            dw     0
    _MOUSE_BUTTON             db     0

    ;_QUIT_SIGNAL                    db      0

SECTION .text


_UpdateInput:

    call SDL_PumpEvents

    push dword _MOUSE_Y
    push dword _MOUSE_X
    call SDL_GetMouseState
    add esp, 8

    mov byte[_MOUSE_BUTTON], al

    mov word[_MOUSE_LBUTTON], 0
    test byte[_MOUSE_BUTTON], 1
    jz .NotMB1
    mov word[_MOUSE_LBUTTON], 1
.NotMB1:

    mov word[_MOUSE_RBUTTON], 0
    test byte[_MOUSE_BUTTON], 4
    jz .NotMB2
    mov word[_MOUSE_RBUTTON], 1
.NotMB2:

    mov word[_MOUSE_MBUTTON], 0
    test byte[_MOUSE_BUTTON], 2
    jz .NotMB3
    mov word[_MOUSE_MBUTTON], 1
.NotMB3:

    push dword 0
    call SDL_GetKeyState
    add esp, 4
    mov dword[ _KBPtr], eax

    invoke _sseMemcpy32, dword _KEYBOARD, dword[_KBPtr], dword 320/4

    ;cmp byte[_SDLEvent], 12
    ;jne .Done
    ;mov byte[_QUIT_SIGNAL], 1

 .Done:

    ret


_FlushKeyboard:
    pusha

    push dword 0
    call SDL_GetKeyState
    add esp, 4
    mov dword[_KBPtr], eax
    invoke _sseMemset32, dword[_KBPtr], dword 0, dword 80

    popa
    ret

