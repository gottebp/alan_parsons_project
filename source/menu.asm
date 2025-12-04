;============================================================
; Project:      "The Alan Parsons Project"
; Description:  Code for ece291 final project
; Author:       David Lytle
; Website:
; Date:        7/20/02
;============================================================

%include "defs.inc"
%include "macros.inc"
%include "sdl.inc"
%include "input.inc"
%include "ppe.inc"
%include "sse_mem.inc"
%include "rand.inc"

BITS 32

    EXTERN _ScreenOff
    EXTERN _SIN_LOOK
    EXTERN _COS_LOOK
    EXTERN _RoundingFactor
    EXTERN _fltDegToRad360
    EXTERN _fltRadToDeg360
    EXTERN _fltDegToRad256
    EXTERN _fltRadToDeg256
    EXTERN _MapMidX
    EXTERN _MapMidY
    EXTERN _MenuBackgroundFile
    EXTERN _GameState
    EXTERN _ActivateMenu
    EXTERN _GameIsNetworked
    EXTERN _GameIsClient
    EXTERN _LoadLevel
    EXTERN _GameLevel

    EXTERN SDL_UpdateRect, SDL_LockSurface, SDL_UnlockSurface, SDL_ShowCursor
    EXTERN _ScreenTemp, _FadeFromBlack, _FadeToBlack, _FadeFromWhite, _FadeToWhite

    EXTERN _SNDEngines_Counter, _SNDWeapon_Counter

GLOBAL _IsMenuRunning
GLOBAL  _RunMenu, _InitMenu, _MenuResult, _DestroyMenu

SECTION .bss

_MenuBackOff           resd    1
_MouseCursorOff        resd    1

_MenuButtonMask        resd    1

SECTION .data

_fltPSpeed              dd      2.3421

_fltTemp                dd      0

_DarknessMask           times (143*122)         dd      0B0000000h
_RandomMask           times (143*122)         dd      0B0000000h

_MenuResult            dd      0
_MouseFile             db      './data/cursor.bmp',0

_IsMenuRunning          db      0

_memory_error           db      10,'Memory allocation error occured in _InitMenu',10,0

_PrintStr                     db        10,'Menu = %d',10,0


;====== InitMenu ===================================================
;;; _InitMenu
;;;   INPUTS:_MenuBackOff
;;;  OUTPUTS: The menu and the mouse
;;;  PURPOSE: to get the menu visible
_InitMenu:

    mov dword[_MenuResult], 0

    invoke malloc, dword (SCREEN_WIDTH * SCREEN_HEIGHT * 4)
    cmp    eax, 0
    je near .memerror
    mov    dword[_MenuBackOff], eax

    invoke malloc, dword (CURSOR_WIDTH * CURSOR_HEIGHT * 4)
    cmp    eax, 0
    je near .memerror
    mov    dword[_MouseCursorOff], eax

    invoke _LoadBMP, dword[_MenuBackOff], dword _MenuBackgroundFile
    invoke _LoadBMP, dword[_MouseCursorOff], dword _MouseFile

    mov esi, dword[_MouseCursorOff]
    xor eax, eax
    xor ecx, ecx
.SetupCursorAlphaTransparency:

    mov ecx, dword[esi + eax]
    cmp ecx, 0FF00FFFFh
    jne .SkipPixel

    mov dword[esi + eax], 00000000h

.SkipPixel:

    add eax, 4
    cmp eax, CURSOR_WIDTH * CURSOR_HEIGHT * 4
    jb .SetupCursorAlphaTransparency

    ret

.memerror:
    invoke printf, dword _memory_error
    ret


;====== RunMenu ===================================================
;;; _RunMenu
;;;   INPUTS: mouse inputs, keyboard inputs and a status variable
;;;  OUTPUTS: the map number that was clicked if one was clicked
;;;  PURPOSE: to get data to initialize correct maps and look pretty
_RunMenu:
.game_running    EQU    4

    cmp byte[_IsMenuRunning], 1
    je near .DontReinitMenu

    mov byte[_IsMenuRunning], 1
    invoke Mix_FadeOutMusic, dword 100
.DontReinitMenu:

    cmp dword[_SNDWeapon_Counter], 0
    je .DontKillFiringSound
    dec dword[_SNDWeapon_Counter]
    invoke Mix_HaltChannel, dword 1
.DontKillFiringSound:

    cmp dword[_SNDEngines_Counter], 0
    je .DontKillEngineSound
    dec dword[_SNDEngines_Counter]
    invoke Mix_HaltChannel, dword 3
.DontKillEngineSound:

    invoke _sseMemset32, dword[_ScreenOff], dword 0, dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _AlphaBlit, dword 0, dword 0, dword[_MenuBackOff], dword 640, dword 480
    invoke _sseMemcpy32, dword _ScreenTemp, dword[_ScreenOff], dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _FadeFromBlack, dword 80, dword 3

    invoke SDL_ShowCursor, dword 0
    mov dword[_MenuResult], 0

.WaitForEscKeyUp:
    call _UpdateInput
    cmp byte[_KEYBOARD + SDLK_ESCAPE], 1
    je .WaitForEscKeyUp

.MenuLoop:

    call _MakeRandomMask

    invoke _sseMemset32, dword[_ScreenOff], dword 0, dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _AlphaBlit, dword 0, dword 0, dword[_MenuBackOff], dword 640, dword 480

    call _UpdateInput

    cmp byte[_KEYBOARD + SDLK_ESCAPE], 1
    jne near .NotEsc

    mov eax, dword[ebp + .game_running]
    mov dword[_MenuResult], eax

    mov byte[_IsMenuRunning], 0

    invoke Mix_FadeOutMusic, dword 200

    jmp near .Done

.NotEsc:

    cmp word[_MOUSE_X], 13
    jb near .NotExitButton
    cmp word[_MOUSE_X], 94
    jg near .NotExitButton
    cmp word[_MOUSE_Y], 13
    jb near .NotExitButton
    cmp word[_MOUSE_Y], 115
    jg near .NotExitButton

    invoke _AlphaBlit, dword 8, dword 2, dword _RandomMask, dword 143, dword 122

    cmp word[_MOUSE_LBUTTON], 1
    jne near .NotExitButton

.WaitForMouseLButtonUp1:
    call _UpdateInput
    cmp word[_MOUSE_LBUTTON], 1
    je .WaitForMouseLButtonUp1

;        mov eax, dword[ebp + .game_running]
;    mov byte[_IsMenuRunning], 0
;        invoke Mix_FadeOutMusic, dword 200

    mov dword[_MenuResult], 0
    jmp near .Done

.NotExitButton:

    ;invoke _AlphaBlit, dword 163, dword 19-6, dword _DarknessMask, dword 143, dword 122

    cmp word[_MOUSE_X], 163
    jb .NotShireButton
    cmp word[_MOUSE_X], 306
    jg .NotShireButton
    cmp word[_MOUSE_Y], 19
    jb .NotShireButton
    cmp word[_MOUSE_Y], 155
    jg .NotShireButton

    invoke _AlphaBlit, dword 163, dword 19-6, dword _RandomMask, dword 143, dword 122

    cmp word[_MOUSE_LBUTTON], 1
    jne near .NotShireButton

.WaitForMouseLButtonUp2:
    call _UpdateInput
    cmp word[_MOUSE_LBUTTON], 1
    je .WaitForMouseLButtonUp2

    mov dword[_MenuResult], MENU_LOAD_SHIRE
    jmp near .Done

.NotShireButton:

    cmp dword[_GameLevel], 1
    jge .SkipDarknessMask3
    invoke _AlphaBlit, dword 483, dword 36-6, dword _DarknessMask, dword 143, dword 122
.SkipDarknessMask3:

    cmp word[_MOUSE_X], 483
    jb near .NotArchipelagoButton
    cmp word[_MOUSE_X], 626
    jg near .NotArchipelagoButton
    cmp word[_MOUSE_Y], 36
    jb near .NotArchipelagoButton
    cmp word[_MOUSE_Y], 153
    jg near .NotArchipelagoButton

    cmp dword[_GameLevel], 1
    jb near .NotArchipelagoButton

    invoke _AlphaBlit, dword 483, dword 36-6, dword _RandomMask, dword 143, dword 122

    cmp word[_MOUSE_LBUTTON], 1
    jne near .NotArchipelagoButton

.WaitForMouseLButtonUp5:
    call _UpdateInput
    cmp word[_MOUSE_LBUTTON], 1
    je .WaitForMouseLButtonUp5

    mov dword[_MenuResult], MENU_LOAD_ARCHIPELAGO
    jmp near .Done

.NotArchipelagoButton:


    cmp dword[_GameLevel], 2
    jge .SkipDarknessMask4
    invoke _AlphaBlit, dword 480, dword 192-6, dword _DarknessMask, dword 143, dword 122
.SkipDarknessMask4:

    cmp word[_MOUSE_X], 480
    jb near .NotDuneButton
    cmp word[_MOUSE_X], 623
    jg near .NotDuneButton
    cmp word[_MOUSE_Y], 192
    jb near .NotDuneButton
    cmp word[_MOUSE_Y], 309
    jg near .NotDuneButton

    cmp dword[_GameLevel], 2
    jb .NotDuneButton

    invoke _AlphaBlit, dword 480, dword 192-6, dword _RandomMask, dword 143, dword 122

    cmp word[_MOUSE_LBUTTON], 1
    jne near .NotDuneButton

.WaitForMouseLButtonUp6:
    call _UpdateInput
    cmp word[_MOUSE_LBUTTON], 1
    je .WaitForMouseLButtonUp6

    mov dword[_MenuResult], MENU_LOAD_DUNE
    jmp near .Done

.NotDuneButton:


    cmp dword[_GameLevel], 3
    jge .SkipDarknessMask2
    invoke _AlphaBlit, dword 20, dword 316-6, dword _DarknessMask, dword 143, dword 122
.SkipDarknessMask2:

    cmp word[_MOUSE_X], 20
    jb near .NotMidkemiaButton
    cmp word[_MOUSE_X], 163
    jg near .NotMidkemiaButton
    cmp word[_MOUSE_Y], 316
    jb near .NotMidkemiaButton
    cmp word[_MOUSE_Y], 433
    jg near .NotMidkemiaButton

    cmp dword[_GameLevel], 3
    jb .NotMidkemiaButton

    invoke _AlphaBlit, dword 20, dword 316-6, dword _RandomMask, dword 143, dword 122

    cmp word[_MOUSE_LBUTTON], 1
    jne near .NotMidkemiaButton

.WaitForMouseLButtonUp4:
    call _UpdateInput
    cmp word[_MOUSE_LBUTTON], 1
    je .WaitForMouseLButtonUp4

    mov dword[_MenuResult], MENU_LOAD_MIDKEMIA
    jmp near .Done

.NotMidkemiaButton:


    cmp dword[_GameLevel], 4
    jge .SkipDarknessMask5
    invoke _AlphaBlit, dword 454, dword 342-6, dword _DarknessMask, dword 143, dword 122
.SkipDarknessMask5:

    cmp word[_MOUSE_X], 454
    jb near .NotOceaniaButton
    cmp word[_MOUSE_X], 597
    jg near .NotOceaniaButton
    cmp word[_MOUSE_Y], 342
    jb near .NotOceaniaButton
    cmp word[_MOUSE_Y], 459
    jg near .NotOceaniaButton

    cmp dword[_GameLevel], 4
    jb .NotOceaniaButton

    invoke _AlphaBlit, dword 454, dword 342-6, dword _RandomMask, dword 143, dword 122

    cmp word[_MOUSE_LBUTTON], 1
    jne near .NotOceaniaButton

.WaitForMouseLButtonUp7:
    call _UpdateInput
    cmp word[_MOUSE_LBUTTON], 1
    je .WaitForMouseLButtonUp7

    mov dword[_MenuResult], MENU_LOAD_OCEANIA
    jmp near .Done

.NotOceaniaButton:


    cmp dword[_GameLevel], 5
    jge .SkipDarknessMask1
    invoke _AlphaBlit, dword 14, dword 157-6, dword _DarknessMask, dword 143, dword 122
.SkipDarknessMask1:

    cmp word[_MOUSE_X], 14
    jb near .NotMordorButton
    cmp word[_MOUSE_X], 157
    jg near .NotMordorButton
    cmp word[_MOUSE_Y], 157
    jb near .NotMordorButton
    cmp word[_MOUSE_Y], 274
    jg near .NotMordorButton

    cmp dword[_GameLevel], 5
    jb .NotMordorButton

    invoke _AlphaBlit, dword 14, dword 157-6, dword _RandomMask, dword 143, dword 122

    cmp word[_MOUSE_LBUTTON], 1
    jne near .NotMordorButton

.WaitForMouseLButtonUp3:
    call _UpdateInput
    cmp word[_MOUSE_LBUTTON], 1
    je .WaitForMouseLButtonUp3

    mov dword[_MenuResult], MENU_LOAD_MORDOR
    jmp near .Done

.NotMordorButton:


.DontCheckButtonBoundaries:

    xor eax, eax
    mov ax, word[_MOUSE_X]
    xor ebx, ebx
    mov bx, word[_MOUSE_Y]
    invoke _AlphaBlit, dword eax, dword ebx, dword[_MouseCursorOff], dword CURSOR_WIDTH, dword CURSOR_HEIGHT

    invoke SDL_UpdateRect, dword[_Screen], dword 0, dword 0, dword 0, dword 0

    jmp .MenuLoop

.Done:
    call _UpdateInput
    cmp byte[_KEYBOARD + SDLK_ESCAPE], 1
    je near .Done
.Exit:

    invoke _sseMemset32, dword[_ScreenOff], dword 0, dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _AlphaBlit, dword 0, dword 0, dword[_MenuBackOff], dword 640, dword 480
    invoke _sseMemcpy32, dword _ScreenTemp, dword[_ScreenOff], dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _FadeToBlack, dword 100, dword 3

    emms

    mov byte[_IsMenuRunning], 0
    mov eax, 0
    ret


;====== DestroyMenu ===================================================
;;; _DestroyMenu
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: Deallocates memory
_DestroyMenu:

    invoke free, dword[_MenuBackOff]
    invoke free, dword[_MouseCursorOff]

    ret


;====== MakeRandomMask ===================================================
;;; _MakeRandomMask
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: blah
_MakeRandomMask:
    pusha

    xor eax, eax
    xor ebx, ebx
    mov ecx, 3
    mov esi, dword _RandomMask
.MaskLoop:

    call _Rand
    xor edx, edx
    mov ebx, 100
    div ebx
    mov byte[esi + ecx], dl

    add ecx, 4
    cmp ecx, 143*122*4
    jb .MaskLoop

    popa
    ret
