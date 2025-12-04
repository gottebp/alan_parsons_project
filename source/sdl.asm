;============================================================
; Project:              "The Alan Parsons Project"
; Description:          Code for ece291 final project
; Author:               Benjamin Gottemoller
; Website:              http://www.particlefield.com
; Date:                    7/20/02
; Ported to SDL:    8/12/02
;============================================================

BITS    32

%include "defs.inc"
%include "macros.inc"
%include "sse_mem.inc"
%include "input.inc"

;-----------------------------------------------------------------------------------------------------------------------
SDL_INIT_TIMER            EQU                 00000001h
SDL_INIT_AUDIO            EQU                 00000010h
SDL_INIT_VIDEO            EQU                 00000020h
SDL_INIT_CDROM            EQU                 00000100h
SDL_INIT_JOYSTICK        EQU                 00000200h
SDL_INIT_NOPARACHUTE        EQU                 00100000h
SDL_INIT_EVENTTHREAD        EQU                 01000000h
SDL_INIT_EVERYTHING        EQU                 0000FFFFh
SDL_SWSURFACE            EQU                00000000h
SDL_HWSURFACE             EQU                00000001h
SDL_ASYNCBLIT              EQU                00000004h
SDL_ANYFORMAT            EQU                10000000h
SDL_HWPALETTE            EQU                20000000h
SDL_DOUBLEBUF            EQU                40000000h
SDL_FULLSCREEN            EQU                80000000h
SDL_OPENGL              EQU                00000002h
SDL_OPENGLBLIT            EQU                0000000Ah
SDL_RESIZABLE            EQU                00000010h
SDL_NOFRAME            EQU                00000020h
SDL_HWACCEL            EQU                00000100h
SDL_SRCCOLORKEY         EQU                00001000h
SDL_RLEACCELOK            EQU                00002000h
SDL_RLEACCEL            EQU                00004000h
SDL_SRCALPHA            EQU                00010000h
SDL_PREALLOC            EQU                01000000h

AUDIO_U8                        EQU                             00000008h
AUDIO_S8                        EQU                             00008008h
AUDIO_U16LSB                    EQU                             00000010h
AUDIO_S16LSB                    EQU                             00008010h
AUDIO_U16MSB                    EQU                             00001010h
AUDIO_S16MSB                    EQU                             00009010h
AUDIO_U16                       EQU                             AUDIO_U16LSB
AUDIO_S16                       EQU                             AUDIO_S16LSB

AUDIO_BUFFER_SIZE               EQU                             4096
AUDIO_SAMPLE_RATE               EQU                             22050
AUDIO_NUM_CHANNELS              EQU                             16
;-----------------------------------------------------------------------------------------------------------------------

GLOBAL _InitGraphics, _DestroyGraphics, _LoadBMP, _AlphaBlit, Mix_LoadWAV
GLOBAL _Screen, _ScreenOff
EXTERN SDL_Init, SDL_Quit, SDL_SetVideoMode, SDL_WM_SetCaption, SDL_RWFromFile, SDL_LoadBMP_RW
EXTERN Mix_OpenAudio, Mix_AllocateChannels, Mix_CloseAudio
EXTERN Mix_LoadWAV_RW, Mix_VolumeChunk, Mix_LoadMUS, Mix_PlayMusic, Mix_PlayChannel
EXTERN Mix_Playing, Mix_PlayingMusic, Mix_FreeChunk, Mix_FreeMusic, SDL_FreeSurface
EXTERN Mix_HaltMusic, Mix_HaltChannel

SECTION .bss

_Screen            resd    1
_ScreenOff        resd    1

_TempSurface           resd  1

_TempSurfacePtr         resd     1

SECTION .data

_ProjectName        db        'The Alan Parsons Project',0
_UsageMessage        db        'Usage: LinAPP --fullscreen or LinAPP',10,0
_FullScreenCommand    db        '--fullscreen',0
_FileLoadFailed          db     10,'Failed To Load %s',10,0
_LoadBMP_Debug           db     '[DEBUG] _LoadBMP: ESP=%08X EBP=%08X [EBP+4]=%08X [EBP+8]=%08X',10,0

_rb                                   db        'rb',0

_RoundingFactor                      dd 00800080h, 00800080h

_intTop                              dd 0
_intBottom                           dd 0
_intLeft                             dd 0
_intRight                            dd 0

_intABSrcWidth                       dd 0
_intABSrcHeight                      dd 0
_intABSrcSize                        dd 0

SECTION .text

;====== InitGraphics ===================================================
;### _InitGraphics
;### INPUTS: 0=Windowed mode, 1=Fullscreen
;### OUTPUTS:    Who Cares
;===================================================================
_InitGraphics:
    push ebp
    mov ebp, esp
    add ebp, 4

;;;;I don't need to allocate my own back buffer as long as the one SDL allocates has an lpitch the same as SCREEN_WIDTH
;;;;I'm assuming this will always be the case since I'm initializing SDL to place the back buffer in system memory
;    push dword (SCREEN_WIDTH * SCREEN_HEIGHT * (SCREEN_BPP / 8))
;    call malloc
;    mov dword[_ScreenOff], eax
;    add esp, 4

    push dword SDL_INIT_VIDEO | SDL_INIT_AUDIO

    call SDL_Init
    add esp, 4

    cmp dword[ebp + 4], 0
    je .NotFullScreen

    push dword SDL_SWSURFACE | SDL_FULLSCREEN
    jmp .NotWindowed

.NotFullScreen:

    push dword SDL_SWSURFACE

.NotWindowed:

    ;Screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_SWSURFACE | SDL_FULLSCREEN);
    push dword SCREEN_BPP
    push dword SCREEN_HEIGHT
    push dword SCREEN_WIDTH
    call SDL_SetVideoMode
    mov dword[_Screen], eax
    add esp, 16

    mov eax, dword[eax + 20]
mov dword[_ScreenOff], eax

    push dword 0
    push dword _ProjectName
    call SDL_WM_SetCaption
    add esp, 8

    invoke Mix_OpenAudio, dword AUDIO_SAMPLE_RATE, dword AUDIO_S16LSB, dword 2, dword AUDIO_BUFFER_SIZE
    invoke Mix_AllocateChannels, dword AUDIO_NUM_CHANNELS

    pop ebp
    ret

;====== DestroyGraphics ================================================
;### _DestroyGraphics
;### INPUTS: none
;### OUTPUTS: none
;===================================================================
_DestroyGraphics:
    push ebp
    mov ebp, esp
    add ebp, 4

;    push dword[_ScreenOff]
;    call free
;    add esp, 4

    call Mix_CloseAudio
    call SDL_Quit

    pop ebp
    ret

;====== LoadBMP ================================================
;### _LoadBMP
;### INPUTS: dword pointer to buffer that will recieve image data, dword file string ptr
;### OUTPUTS: none
;===================================================================
_LoadBMP:
    pusha

    ; DEBUG: Log ebp and parameter values
    pusha
    push dword[ebp + 8]
    push dword[ebp + 4]
    push ebp
    push esp
    invoke printf, dword _LoadBMP_Debug
    add esp, 16
    popa

    invoke SDL_RWFromFile, dword[ebp + 8], dword _rb
    invoke SDL_LoadBMP_RW, dword eax, dword 1

    mov dword[_TempSurfacePtr], eax

    cmp eax, 0
    jne .NoError
    pusha
    invoke printf, dword _FileLoadFailed, dword[ebp + 8]
    popa
.NoError:

    push eax
    mov ebx, dword[eax + 12]
    mov eax, dword[eax + 8]
    mul ebx
    mov ecx, eax
    shl ecx, 2
    pop eax

    mov esi, dword[ebp + 4]
    mov edi, dword[eax + 20]
    xor eax, eax
    xor ebx, ebx
.CopyData:

    mov dl, byte[edi + ebx]
    mov byte[esi + eax], dl
    inc eax
    inc ebx

    mov dl, byte[edi + ebx]
    mov byte[esi + eax], dl
    inc eax
    inc ebx

    mov dl, byte[edi + ebx]
    mov byte[esi + eax], dl
    inc eax
    inc ebx

    mov byte[esi + eax], 0FFh
    inc eax

    cmp eax, ecx
    jb .CopyData

    invoke SDL_FreeSurface, dword[_TempSurfacePtr]

    popa
    ret


;====== AlphaBlit ===================================================
;### _AlphaBlit
;### INPUTS: .x, .y, .src_off, .src_width, .src_height
;### OUTPUTS: Alpha composed image is drawn on the screen
_AlphaBlit:
.x                      EQU    4;4
.y                      EQU    8;4+.x
.src_off                EQU    12;4+.x+.y
.src_width              EQU    16;4+.x+.y+.src_off
.src_height             EQU    20;4+.x+.y+.src_off+.src_width

    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi

    mov eax, dword[ebp + .src_width]
    mov ebx, dword[ebp + .src_height]
    mov dword[_intABSrcWidth], eax
    mov dword[_intABSrcHeight], ebx

    mov eax, dword[ebp + .x]
    cmp eax, 0
    jge .DontAdjustLeft

    neg eax
    cmp eax, dword[ebp + .src_width]
    jge near .Done

    mov dword[ebp + .x], 0
    sub dword[ebp + .src_width], eax
    shl eax, 2
    add dword[ebp + .src_off], eax

.DontAdjustLeft:


    mov eax, dword[ebp + .y]
    cmp eax, 0
    jge .DontAdjustTop

    neg eax
    cmp eax, dword[ebp + .src_height]
    jge near .Done

    mov dword[ebp + .y], 0
    sub dword[ebp + .src_height], eax

    mov ebx, eax
    mov eax, dword[_intABSrcWidth]
    mul ebx
    shl eax, 2
    add dword[ebp + .src_off], eax

.DontAdjustTop:


    mov eax, dword[ebp + .x]
    add eax, dword[_intABSrcWidth]
    cmp eax, SCREEN_WIDTH
    jl .DontAdjustRight

    sub eax, SCREEN_WIDTH
    cmp eax, dword[ebp + .src_width]
    jge near .Done

    sub dword[ebp + .src_width], eax

.DontAdjustRight:


    mov eax, dword[ebp + .y]
    add eax, dword[_intABSrcHeight]
    cmp eax, SCREEN_HEIGHT
    jl .DontAdjustBottom

    sub eax, SCREEN_HEIGHT
    cmp eax, dword[ebp + .src_height]
    jge near .Done

    sub dword[ebp + .src_height], eax

.DontAdjustBottom:


    mov eax, dword[ebp + .src_height]
    mov ebx, dword[_intABSrcWidth]
    mul ebx
    shl eax, 2
    mov dword[_intABSrcSize], eax

    mov eax, dword[ebp + .y]
    mov ebx, SCREEN_WIDTH
    mul ebx
    add eax, dword[ebp + .x]
    shl eax, 2

    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
    mov edi, dword[ebp + .src_off]
    mov esi, dword[_ScreenOff]

.DrawImage:

    ComputeAlpha dword[esi + eax], dword[edi + ebx], dword[esi + eax]

    inc ecx
    cmp ecx, dword[ebp + .src_width]
    jb .RowNotDone

    mov ecx, SCREEN_WIDTH
    sub ecx, dword[ebp + .src_width]
    shl ecx, 2
    add eax, ecx

    mov ecx, dword[_intABSrcWidth]
    shl ecx, 2
    add ebx, ecx
    mov ecx, dword[ebp + .src_width]
    shl ecx, 2
    sub ebx, ecx

    xor ecx, ecx
.RowNotDone:

    add ebx, 4
    add eax, 4
    cmp ebx, dword[_intABSrcSize]
    jb near .DrawImage

.Done:

    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax

    ret


;====== Mix_LoadWAV ================================================
;### Mix_LoadWAV
;### INPUTS: dword file string ptr
;### OUTPUTS: eax = dword pointer to Mix_Chunk
;===================================================================
Mix_LoadWAV:
    push ebx
    push ecx
    push edx
    push esi
    push edi

    invoke SDL_RWFromFile, dword[ebp + 4], dword _rb
    invoke Mix_LoadWAV_RW, dword eax, dword 1

    cmp eax, 0
    jne .NoError
    pusha
    invoke printf, dword _FileLoadFailed, dword[ebp + 4]
    popa
.NoError:

    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    ret




;====== TestLoadBMPWrapper ===================================================
; Wrapper for testing _LoadBMP from JavaScript
; Takes dest and filename pointers from [ebp+4] and [ebp+8]
;=============================================================================
GLOBAL _TestLoadBMPWrapper
_TestLoadBMPWrapper:
    push ebp
    mov ebp, esp
    
    ; Call _LoadBMP using invoke
    invoke _LoadBMP, dword[ebp + 8], dword[ebp + 12]
    
    pop ebp
    ret
