
;============================================================
; Project:      "The Alan Parsons Project"
; Description:  Code for ece291 final project
; Author:       Benjamin Gottemoller
; Website:      http://www.particlefield.com
; Date:        7/20/02
;============================================================

%include "defs.inc"
%include "sdl.inc"
%include "player.inc"
%include "ppe.inc"
%include "rand.inc"
%include "macros.inc"
%include "sse_mem.inc"
%include "input.inc"
%include "enemy.inc"
%include "ai.inc"

BITS 32


    EXTERN _InitializeGameData
    EXTERN _ProcessInput
    EXTERN _WaitForRetrace
    EXTERN _LoadLevel

    EXTERN _GraphicsMode
    EXTERN _ScreenOff
    EXTERN _KEYBOARD
    EXTERN _SIN_LOOK
    EXTERN _COS_LOOK
    EXTERN _RoundingFactor
    EXTERN _fltDegToRad360
    EXTERN _fltRadToDeg360
    EXTERN _fltDegToRad256
    EXTERN _fltRadToDeg256
    EXTERN _MapMidX
    EXTERN _MapMidY
    EXTERN _GameState
    EXTERN _ActivateMenu
    EXTERN _GameIsNetworked
    EXTERN _GameIsClient

    EXTERN _SNDEffect_Engines, _SNDEffect_EvilLaugh, _SNDEffect_Hit
    EXTERN _SNDEffect_Weapon, _SNDEffect_ExplosionArray

GLOBAL _MapPadTop, _MapOff, _MapPadBottom, _InitMapEngine, _LoadMap, _RenderMap, _ShakeMap, _intShakeMap, _DestroyMapEngine

SECTION .bss

_MapPadTop         resd    1
_MapOff            resd    1
_MapPadBottom      resd    1

_intShakeMap       resd    1

SECTION .data

;====== Temporary Variables Used For Anything Necessary ===================
_intTemp                             dd 0
_fltTemp                             dd 0

_intTop                              dd 0
_intBottom                           dd 0
_intLeft                             dd 0
_intRight                            dd 0

_memory_error                   db      10,'Memory allocation error occured in _InitMapEngine',10,0

;====== InitMapEngine ===================================================
;;; _InitMapEngine
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: Allocates and initializes dynamic memory used by the map engine
;;;  CALLS: _AllocMem, _sseMemset32
_InitMapEngine:

    mov dword[_intShakeMap], 0

    push dword (MAP_WIDTH*MAP_HEIGHT*4) + (MAP_WIDTH*2*4)
    call malloc
    add esp, 4

    cmp    eax, 0
    je near .memerror

    mov    dword[_MapPadTop], eax
    invoke  _sseMemset32, dword eax, dword 0, dword (MAP_WIDTH*MAP_HEIGHT + MAP_WIDTH*2)
    mov     eax, dword[_MapPadTop]
    add     eax, MAP_WIDTH * 4
    mov     dword[_MapOff], eax
    add     eax, MAP_WIDTH * MAP_HEIGHT * 4
    mov     dword[_MapPadBottom], eax

    ret

.memerror:
    invoke printf, dword _memory_error
    ret


;====== DestroyMapEngine ===================================================
;;; _DestroyMapEngine
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: Deallocates the map engine resources
_DestroyMapEngine:

    invoke free, dword[_MapPadTop]

    ret


;====== LoadMap ===================================================
;;; _LoadMap
;;;   INPUTS: map file name
;;;  OUTPUTS: Loaded map data
;;;  PURPOSE: Load levels in the game
_LoadMap:
.map_str    EQU    4

    push esi
    push edi

    invoke _LoadBMP, dword[_MapOff], dword[ebp + .map_str]

    invoke _sseMemcpy32, dword[_MapPadTop], dword [_MapOff], dword MAP_WIDTH
    mov eax, dword [_MapOff]
    add eax, MAP_WIDTH * (MAP_HEIGHT - 1) * 4
    invoke _sseMemcpy32, dword [_MapPadBottom], dword eax, dword MAP_WIDTH

    pop edi
    pop esi
    ret


;====== RenderMap ===================================================
;;; _RenderMap
;;;   INPUTS: integer x and y coordinates of the player on the map
;;;  OUTPUTS: The rendered map to [_ScreenOff]
;;;  PURPOSE: Draw the map
;;;    CALLS: _sseMemcpy32
_RenderMap:

    push esi
    push edi

    xor eax, eax
    xor ecx, ecx
    xor edx, edx

    cmp dword[_intShakeMap], 0
    je .DontShake

    call _Rand
    xor edx, edx
    mov ebx, SHAKE_FACTOR
    div ebx
    mov eax, edx
    sub eax, SHAKE_FACTOR / 2
    add eax, dword[_intPlayerX]
    mov dword[_intPlayerX], eax

    call _Rand
    xor edx, edx
    mov ebx, SHAKE_FACTOR
    div ebx
    mov eax, edx
    sub eax, SHAKE_FACTOR / 2
    add eax, dword[_intPlayerY]
    mov dword[_intPlayerY], eax

    cmp dword[_intPlayerX], MAP_WIDTH
    jb .DontClampPlayerX

    mov dword[_intPlayerX], 0

.DontClampPlayerX:
    cmp dword[_intPlayerY], MAP_HEIGHT
    jb .DontClampPlayerY

    mov dword[_intPlayerY], 0

.DontClampPlayerY:

    dec dword[_intShakeMap]

.DontShake:


    mov eax, dword[_intPlayerX]
    sub eax, SCREEN_WIDTH / 2
    shl eax, 2
    mov dword[_intTop], eax             ;Compute the offset of the top of the map at the players x position - (SCREEN_WIDTH / 2)

    xor edx, edx
    mov eax, MAP_HEIGHT - 1
    mov ebx, MAP_WIDTH
    mul ebx
    add eax, dword[_intPlayerX]
    sub eax, SCREEN_WIDTH / 2
    shl eax, 2
    mov dword[_intBottom], eax          ;Compute the offset of the bottom of the map at the players x position - (SCREEN_WIDTH / 2)

    xor edx, edx
    mov eax, dword[_intPlayerY]
    sub eax, SCREEN_HEIGHT / 2
    mov ebx, MAP_WIDTH
    mul ebx
    add eax, dword[_intPlayerX]
    sub eax, SCREEN_WIDTH / 2
    shl eax, 2

    xor ebx, ebx                        ;Compute the offset of the screen's upper left corner relative to the map
.RenderScanLine:

    cmp eax, dword[_intTop]
    jnl .DontWrapTop

    add eax, (MAP_WIDTH * MAP_HEIGHT) * 4

.DontWrapTop:

    cmp eax, dword[_intBottom]
    jng .DontWrapBottom

    sub eax, (MAP_WIDTH * MAP_HEIGHT) * 4

.DontWrapBottom:

    mov esi, dword[_ScreenOff]
    add esi, ebx
    mov edi, dword[_MapOff]
    add edi, eax

    push eax
    push ebx
    invoke _sseMemcpy32, dword esi, dword edi, dword SCREEN_WIDTH
    pop ebx
    pop eax

    add eax, MAP_WIDTH * 4
    add ebx, SCREEN_WIDTH * 4
    cmp ebx, (SCREEN_WIDTH * SCREEN_HEIGHT) * 4
    jb .RenderScanLine


    pop edi
    pop esi

    ret


;====== ShakeMap ===================================================
;;; _ShakeMap
;;;   INPUTS: Number of frames to shake the map for
;;;  OUTPUTS: none
;;;  PURPOSE: Create a nice earthquake effect
;;;  CALLS: none
_ShakeMap:
.num_frames    EQU    4

    mov eax, dword[ebp + .num_frames]
    mov dword[_intShakeMap], eax

    ret




