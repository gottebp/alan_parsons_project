
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
%include "rand.inc"
%include "mapeng.inc"
%include "sse_mem.inc"
%include "macros.inc"
%include "enemy.inc"
%include "ai.inc"


BITS 32

    EXTERN _InitializeGameData
    EXTERN _ProcessInput
    EXTERN _WaitForRetrace
    EXTERN _LoadLevel

    EXTERN _GraphicsMode
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
    EXTERN _GameState
    EXTERN _ActivateMenu
    EXTERN _GameIsNetworked
    EXTERN _GameIsClient
    EXTERN _SmallParticleFile
    EXTERN _LargeParticleFile

    EXTERN _SNDEffect_Engines, _SNDEffect_EvilLaugh, _SNDEffect_Hit
    EXTERN _SNDEffect_Weapon, _SNDEffect_ExplosionArray

GLOBAL _InitParticleEngine, _DestroyParticleEngine, _ResetParticleEngine, _AddParticle, _AddParticleByVector, _UpdateParticles, _RenderParticles
GLOBAL _MakeAlphaFromRGB, _intPixel, _ParticleDataOff

STRUC PARTICLE
    .IsActive                       resb    1
    .DetectCollisions          resb    1                    ;Collision ID (0=none)
    .ImgSizeType               resd    1                    ;LARGE_PARTICLE or SMALL_PARTICLE
    .ImgOffset                   resd    1                    ;Location of the particle in the image data
    .fltX                             resd    1
    .fltY                             resd    1
    .intX                            resd    1
    .intY                            resd    1
    .XV                              resd    1                    ;x vector
    .YV                              resd    1                    ;y vector
    .MaxLife                      resd    1                    ;lifetime (in frames)
    .Age                            resd    1                    ;current age
    .Damage                     resd    1                    ;Damage points it does
ENDSTRUC


SECTION .bss


_NumParticles             resd    1
_LastParticle             resd    1
_ParticleDataOff          resd    1
_SmallParticleFlaresOff   resd    1
_LargeParticleFlaresOff   resd    1

SECTION .data

;====== Temporary Variables Used For Anything Necessary ===================
_intTemp                             dd 0
_fltTemp                             dd 0

_intTop                              dd 0
_intBottom                           dd 0
_intLeft                             dd 0
_intRight                            dd 0

_intABSrcWidth                       dd 0
_intABSrcHeight                      dd 0
_intABSrcSize                        dd 0

_intPixel                            dd 0
_intbRed                             db 0
_intbGreen                           db 0
_intbBlue                            db 0

_memory_error           db      10,'Memory allocation error occured in _InitParticleEngine',10,0


;====== InitParticleEngine ===================================================
;;; _InitParticleEngine
;;;   INPUTS: Global particle data
;;;  OUTPUTS: Initialized data
;;;  PURPOSE: Allocates and initializes dynamic memory used by the particle engine
;;;    CALLS: _AllocMem, _LoadPNG, _MakeAlphaFromRGB, _ResetParticleEngine
_InitParticleEngine:

    push esi
    push edi

    mov dword[_NumParticles], 0
    mov dword[_LastParticle], 0

    push dword (SMALL_FLARE_FILE_WIDTH*SMALL_FLARE_FILE_HEIGHT*4)
    call malloc
    mov dword[_SmallParticleFlaresOff], eax
    add esp, 4
    cmp eax, 0
    je near .memerror

    push dword (LARGE_FLARE_FILE_WIDTH*LARGE_FLARE_FILE_HEIGHT*4)
    call malloc
    mov dword[_LargeParticleFlaresOff], eax
    add esp, 4
    cmp eax, 0
    je near .memerror

    push dword (PARTICLE_STRUCT_SIZE*MAX_PARTICLES)
    call malloc
    mov dword[_ParticleDataOff], eax
    add esp, 4
    cmp eax, 0
    je near .memerror


    invoke _LoadBMP, dword[_SmallParticleFlaresOff], dword _SmallParticleFile
    mov esi, dword[_SmallParticleFlaresOff]
    xor eax, eax
    xor ecx, ecx
.SetupSPAlphaTransparency:

    invoke _MakeAlphaFromRGB, dword[esi + eax]
    mov ecx, dword[_intPixel]
    mov dword[esi + eax], ecx

    add eax, 4
    cmp eax, SMALL_FLARE_FILE_WIDTH * SMALL_FLARE_FILE_HEIGHT * 4
    jb .SetupSPAlphaTransparency


    invoke _LoadBMP, dword[_LargeParticleFlaresOff], dword _LargeParticleFile
    mov esi, dword[_LargeParticleFlaresOff]
    xor eax, eax
    xor ecx, ecx
.SetupLPAlphaTransparency:

    invoke _MakeAlphaFromRGB, dword[esi + eax]
    mov ecx, dword[_intPixel]
    mov dword[esi + eax], ecx

    add eax, 4
    cmp eax, LARGE_FLARE_FILE_WIDTH * LARGE_FLARE_FILE_HEIGHT * 4
    jb .SetupLPAlphaTransparency

    call _ResetParticleEngine

    pop edi
    pop esi
    ret

.memerror:

    invoke printf, dword _memory_error
    pop edi
    pop esi
    ret


;====== DestroyParticleEngine ===================================================
;;; _DestroyParticleEngine
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: Deallocates memory used by the ppe
_DestroyParticleEngine:
    push esi
    push edi

    invoke free, dword[_SmallParticleFlaresOff]
    invoke free, dword[_LargeParticleFlaresOff]
    invoke free, dword[_ParticleDataOff]

    pop edi
    pop esi
    ret


;====== ResetParticleEngine ===================================================
;;; _ResetParticleEngine
;;;   INPUTS: Global particle data
;;;  OUTPUTS: Cleared particle data
;;;  PURPOSE: Resets all the particles
;;;    CALLS: none
_ResetParticleEngine:

    mov ebx, dword[_ParticleDataOff]
    xor eax, eax
.ClearParticles:
    mov byte[ebx + eax], 0
    inc eax
    cmp eax, PARTICLE_STRUCT_SIZE*MAX_PARTICLES
    jb .ClearParticles

    ret


;====== AddParticle ===================================================
;;; _AddParticle
;;;   INPUTS: DetectCollisions, ImgSizeType, ImgIndex, X, Y, Angle, Speed, MaxLife, Damage
;;;  OUTPUTS: none
;;;  PURPOSE: Creates a particle in the particle array
;;;    CALLS: _app_net_ParticleEvent
_AddParticle:
.detect_collisions    EQU       4
.img_size_type        EQU       8
.img_index            EQU       12
.x                    EQU       16
.y                    EQU       20
.angle                EQU       24
.speed                EQU       28
.max_life             EQU       32
.damage               EQU       36

    push esi
    push edi

    finit

    mov esi, dword[_ParticleDataOff]

    mov eax, dword[_LastParticle]
    mov ebx, PARTICLE_STRUCT_SIZE
    mul ebx
    xor ebx, ebx
    mov ecx, dword[_LastParticle]
.LocateFreeEndParticle:

    cmp byte[esi + eax + PARTICLE.IsActive], 0
    jne near .EndParticleNotOpen

    xor edx, edx
    mov byte[esi + eax + PARTICLE.IsActive], 1
    mov edx, dword[ebp + .detect_collisions]
    mov byte[esi + eax + PARTICLE.DetectCollisions], dl
    mov edx, dword[ebp + .img_size_type]
    mov dword[esi + eax + PARTICLE.ImgSizeType], edx

    push eax
    push ebx
    mov ebx, eax
    xor eax, eax
    cmp dword[ebp + .img_size_type], SMALL_PARTICLE
    jne .NotSmallEndParticle

    mov eax, dword[ebp + .img_index]
    mov edx, SMALL_PARTICLE_WIDTH * SMALL_PARTICLE_HEIGHT
    mul edx
    shl eax, 2
.NotSmallEndParticle:

    cmp dword[ebp + .img_size_type], LARGE_PARTICLE
    jne .NotLargeEndParticle

    mov eax, dword[ebp + .img_index]
    mov edx, LARGE_PARTICLE_WIDTH * LARGE_PARTICLE_HEIGHT
    mul edx
    shl eax, 2
.NotLargeEndParticle:

    mov dword[esi + ebx + PARTICLE.ImgOffset], eax
    pop ebx
    pop eax

    mov edx, dword[ebp + .x]
    mov dword[esi + eax + PARTICLE.fltX], edx
    mov dword[_fltTemp], edx
    fld dword[_fltTemp]
    fistp dword[esi + eax + PARTICLE.intX]
    mov edx, dword[ebp + .y]
    mov dword[esi + eax + PARTICLE.fltY], edx
    mov dword[_fltTemp], edx
    fld dword[_fltTemp]
    fistp dword[esi + eax + PARTICLE.intY]

    xor edx, edx
    mov edx, dword[ebp + .angle]
    shl edx, 2

    fld dword[_COS_LOOK + edx]
    fld dword[ebp + .speed]
    fmulp st1, st0
    fstp dword[esi + eax + PARTICLE.XV]

    fld dword[_SIN_LOOK + edx]
    fld dword[ebp + .speed]
    fmulp st1, st0
    fstp dword[esi + eax + PARTICLE.YV]

    mov edx, dword[ebp + .max_life]
    mov dword[esi + eax + PARTICLE.MaxLife], edx
    mov dword[esi + eax + PARTICLE.Age], 0
    mov edx, dword[ebp + .damage]
    mov dword[esi + eax + PARTICLE.Damage], edx

    inc dword[_NumParticles]
    mov dword[_LastParticle], ecx
    jmp near .Done

.EndParticleNotOpen:

    add eax, PARTICLE_STRUCT_SIZE
    inc ecx
    cmp ecx, MAX_PARTICLES
    jb near .LocateFreeEndParticle


    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
.LocateFreeBeginningParticle:


    cmp byte[esi + eax + PARTICLE.IsActive], 0
    jne near .BeginningParticleNotOpen

    xor edx, edx
    mov byte[esi + eax + PARTICLE.IsActive], 1
    mov edx, dword[ebp + .detect_collisions]
    mov byte[esi + eax + PARTICLE.DetectCollisions], dl
    mov edx, dword[ebp + .img_size_type]
    mov dword[esi + eax + PARTICLE.ImgSizeType], edx


    push eax
    push ebx
    mov ebx, eax
    xor eax, eax
    cmp dword[ebp + .img_size_type], SMALL_PARTICLE
    jne .NotSmallBeginningParticle

    mov eax, dword[ebp + .img_index]
    mov edx, SMALL_PARTICLE_WIDTH * SMALL_PARTICLE_HEIGHT
    mul edx
    shl eax, 2
.NotSmallBeginningParticle:

    cmp dword[ebp + .img_size_type], LARGE_PARTICLE
    jne .NotLargeBeginningParticle

    mov eax, dword[ebp + .img_index]
    mov edx, LARGE_PARTICLE_WIDTH * LARGE_PARTICLE_HEIGHT
    mul edx
    shl eax, 2
.NotLargeBeginningParticle:

    mov dword[esi + ebx + PARTICLE.ImgOffset], eax
    pop ebx
    pop eax


    mov edx, dword[ebp + .x]
    mov dword[esi + eax + PARTICLE.fltX], edx
    mov dword[_fltTemp], edx
    fld dword[_fltTemp]
    fistp dword[esi + eax + PARTICLE.intX]
    mov edx, dword[ebp + .y]
    mov dword[esi + eax + PARTICLE.fltY], edx
    mov dword[_fltTemp], edx
    fld dword[_fltTemp]
    fistp dword[esi + eax + PARTICLE.intY]

    xor edx, edx
    mov edx, dword[ebp + .angle]
    shl edx, 2

    fld dword[_COS_LOOK + edx]
    fld dword[ebp + .speed]
    fmulp st1, st0
    fstp dword[esi + eax + PARTICLE.XV]

    fld dword[_SIN_LOOK + edx]
    fld dword[ebp + .speed]
    fmulp st1, st0
    fstp dword[esi + eax + PARTICLE.YV]

    mov edx, dword[ebp + .max_life]
    mov dword[esi + eax + PARTICLE.MaxLife], edx
    mov dword[esi + eax + PARTICLE.Age], 0
    mov edx, dword[ebp + .damage]
    mov dword[esi + eax + PARTICLE.Damage], edx


    inc dword[_NumParticles]
    mov dword[_LastParticle], ecx
    jmp near .Done

.BeginningParticleNotOpen:

    add eax, PARTICLE_STRUCT_SIZE
    inc ecx
    cmp ecx, dword[_LastParticle]
    jb near .LocateFreeBeginningParticle

.Done:
    pop edi
    pop esi
    ret


;====== AddParticleByVector ===============================================
;;; _AddParticleByVector
;;;   INPUTS: DetectCollisions, ImgSizeType, ImgIndex, X, Y, XV, YV, MaxLife, Damage
;;;  OUTPUTS: none
;;;  PURPOSE: Creates a particle in the particle array
;;;    CALLS: _app_net_ParticleEvent
_AddParticleByVector:
.detect_collisions    EQU       4
.img_size_type        EQU       8
.img_index            EQU       12
.x                    EQU       16
.y                    EQU       20
.xv                   EQU       24
.yv                   EQU       28
.max_life             EQU       32
.damage               EQU       36

    push esi
    push edi

    finit

    mov esi, dword[_ParticleDataOff]

    mov eax, dword[_LastParticle]
    mov ebx, PARTICLE_STRUCT_SIZE
    mul ebx
    xor ebx, ebx
    mov ecx, dword[_LastParticle]
.LocateFreeEndParticle:

    cmp byte[esi + eax + PARTICLE.IsActive], 0
    jne near .EndParticleNotOpen

    xor edx, edx
    mov byte[esi + eax + PARTICLE.IsActive], 1
    mov edx, dword[ebp + .detect_collisions]
    mov byte[esi + eax + PARTICLE.DetectCollisions], dl
    mov edx, dword[ebp + .img_size_type]
    mov dword[esi + eax + PARTICLE.ImgSizeType], edx

    push eax
    push ebx
    mov ebx, eax
    xor eax, eax
    cmp dword[ebp + .img_size_type], SMALL_PARTICLE
    jne .NotSmallEndParticle

    mov eax, dword[ebp + .img_index]
    mov edx, SMALL_PARTICLE_WIDTH * SMALL_PARTICLE_HEIGHT
    mul edx
    shl eax, 2
.NotSmallEndParticle:

    cmp dword[ebp + .img_size_type], LARGE_PARTICLE
    jne .NotLargeEndParticle

    mov eax, dword[ebp + .img_index]
    mov edx, LARGE_PARTICLE_WIDTH * LARGE_PARTICLE_HEIGHT
    mul edx
    shl eax, 2
.NotLargeEndParticle:

    mov dword[esi + ebx + PARTICLE.ImgOffset], eax
    pop ebx
    pop eax

    mov edx, dword[ebp + .x]
    mov dword[esi + eax + PARTICLE.fltX], edx
    mov dword[_fltTemp], edx
    fld dword[_fltTemp]
    fistp dword[esi + eax + PARTICLE.intX]
    mov edx, dword[ebp + .y]
    mov dword[esi + eax + PARTICLE.fltY], edx
    mov dword[_fltTemp], edx
    fld dword[_fltTemp]
    fistp dword[esi + eax + PARTICLE.intY]

    push ebx
    mov edx, dword[ebp + .xv]
    mov ebx, dword[ebp + .yv]
    mov dword[esi + eax + PARTICLE.XV], edx
    mov dword[esi + eax + PARTICLE.YV], ebx
    pop ebx

    mov edx, dword[ebp + .max_life]
    mov dword[esi + eax + PARTICLE.MaxLife], edx
    mov dword[esi + eax + PARTICLE.Age], 0
    mov edx, dword[ebp + .damage]
    mov dword[esi + eax + PARTICLE.Damage], edx

    inc dword[_NumParticles]
    mov dword[_LastParticle], ecx
    jmp near .Done

.EndParticleNotOpen:

    add eax, PARTICLE_STRUCT_SIZE
    inc ecx
    cmp ecx, MAX_PARTICLES
    jb near .LocateFreeEndParticle


    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
.LocateFreeBeginningParticle:


    cmp byte[esi + eax + PARTICLE.IsActive], 0
    jne near .BeginningParticleNotOpen

    xor edx, edx
    mov byte[esi + eax + PARTICLE.IsActive], 1
    mov edx, dword[ebp + .detect_collisions]
    mov byte[esi + eax + PARTICLE.DetectCollisions], dl
    mov edx, dword[ebp + .img_size_type]
    mov dword[esi + eax + PARTICLE.ImgSizeType], edx


    push eax
    push ebx
    mov ebx, eax
    xor eax, eax
    cmp dword[ebp + .img_size_type], SMALL_PARTICLE
    jne .NotSmallBeginningParticle

    mov eax, dword[ebp + .img_index]
    mov edx, SMALL_PARTICLE_WIDTH * SMALL_PARTICLE_HEIGHT
    mul edx
    shl eax, 2
.NotSmallBeginningParticle:

    cmp dword[ebp + .img_size_type], LARGE_PARTICLE
    jne .NotLargeBeginningParticle

    mov eax, dword[ebp + .img_index]
    mov edx, LARGE_PARTICLE_WIDTH * LARGE_PARTICLE_HEIGHT
    mul edx
    shl eax, 2
.NotLargeBeginningParticle:

    mov dword[esi + ebx + PARTICLE.ImgOffset], eax
    pop ebx
    pop eax


    mov edx, dword[ebp + .x]
    mov dword[esi + eax + PARTICLE.fltX], edx
    mov dword[_fltTemp], edx
    fld dword[_fltTemp]
    fistp dword[esi + eax + PARTICLE.intX]
    mov edx, dword[ebp + .y]
    mov dword[esi + eax + PARTICLE.fltY], edx
    mov dword[_fltTemp], edx
    fld dword[_fltTemp]
    fistp dword[esi + eax + PARTICLE.intY]

    push ebx
    mov edx, dword[ebp + .xv]
    mov ebx, dword[ebp + .yv]
    mov dword[esi + eax + PARTICLE.XV], edx
    mov dword[esi + eax + PARTICLE.YV], ebx
    pop ebx

    mov edx, dword[ebp + .max_life]
    mov dword[esi + eax + PARTICLE.MaxLife], edx
    mov dword[esi + eax + PARTICLE.Age], 0
    mov edx, dword[ebp + .damage]
    mov dword[esi + eax + PARTICLE.Damage], edx


    inc dword[_NumParticles]
    mov dword[_LastParticle], ecx
    jmp near .Done

.BeginningParticleNotOpen:

    add eax, PARTICLE_STRUCT_SIZE
    inc ecx
    cmp ecx, dword[_LastParticle]
    jb near .LocateFreeBeginningParticle

.Done:
    pop edi
    pop esi
    ret


;====== UpdateParticles ===================================================
;;; _UpdateParticles
;;;   INPUTS: Global particle data
;;;  OUTPUTS: Updated global particle data
;;;  PURPOSE: Updates all the particles
;;;    CALLS: none
_UpdateParticles:
.Range     EQU     4

    push esi
    push edi
    finit

    mov esi, dword[_ParticleDataOff]

    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
.UpdateParticleLoop:

    mov ebx, dword[ebp + .Range]
    cmp byte[esi + eax + PARTICLE.DetectCollisions], bl
    jb near .SkipParticle

    cmp byte[esi + eax + PARTICLE.IsActive], 0
    je near .SkipParticle

    mov ebx, dword[esi + eax + PARTICLE.MaxLife]
    cmp dword[esi + eax + PARTICLE.Age], ebx
    jb .DontKillParticle

    mov dword[esi + eax + PARTICLE.IsActive], 0

.DontKillParticle:

    fld dword[esi + eax + PARTICLE.fltX]
    fld dword[esi + eax + PARTICLE.XV]
    faddp st1
    fist dword[esi + eax + PARTICLE.intX]
    fstp dword[esi + eax + PARTICLE.fltX]

    fld dword[esi + eax + PARTICLE.fltY]
    fld dword[esi + eax + PARTICLE.YV]
    faddp st1
    fist dword[esi + eax + PARTICLE.intY]
    fstp dword[esi + eax + PARTICLE.fltY]


    cmp dword[esi + eax + PARTICLE.intX], MAP_WIDTH
    jl .DontWrapMaxX

    mov dword[_intTemp], MAP_WIDTH
    fld dword[esi + eax + PARTICLE.fltX]
    fild dword[_intTemp]
    fsubp st1, st0
    fst dword[esi + eax + PARTICLE.fltX]
    fistp dword[esi + eax + PARTICLE.intX]

.DontWrapMaxX:

    cmp dword[esi + eax + PARTICLE.intX], 0
    jg .DontWrapMinX

    mov dword[_intTemp], MAP_WIDTH
    fld dword[esi + eax + PARTICLE.fltX]
    fild dword[_intTemp]
    faddp st1, st0
    fst dword[esi + eax + PARTICLE.fltX]
    fistp dword[esi + eax + PARTICLE.intX]

.DontWrapMinX:


    cmp dword[esi + eax + PARTICLE.intY], MAP_HEIGHT
    jl .DontWrapMaxY

    mov dword[_intTemp], MAP_HEIGHT
    fld dword[esi + eax + PARTICLE.fltY]
    fild dword[_intTemp]
    fsubp st1, st0
    fst dword[esi + eax + PARTICLE.fltY]
    fistp dword[esi + eax + PARTICLE.intY]

.DontWrapMaxY:

    cmp dword[esi + eax + PARTICLE.intY], 0
    jg .DontWrapMinY

    mov dword[_intTemp], MAP_HEIGHT
    fld dword[esi + eax + PARTICLE.fltY]
    fild dword[_intTemp]
    faddp st1, st0
    fst dword[esi + eax + PARTICLE.fltY]
    fistp dword[esi + eax + PARTICLE.intY]

.DontWrapMinY:


    inc dword[esi + eax + PARTICLE.Age]

.SkipParticle:

    add eax, PARTICLE_STRUCT_SIZE
    inc ecx
    cmp ecx, MAX_PARTICLES
    jb near .UpdateParticleLoop

    pop edi
    pop esi
    ret


;====== RenderParticles ===================================================
;;; _RenderParticles
;;;   INPUTS: Global particle data
;;;  OUTPUTS: none
;;;  PURPOSE: Renders all the particles on the screen
;;;    CALLS: _AlphaBlit
_RenderParticles:
.Range  EQU     4

    push esi
    push edi

    mov esi, dword[_ParticleDataOff]

    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
.RenderParticleLoop:

    mov ebx, dword[ebp + .Range]
    cmp byte[esi + eax + PARTICLE.DetectCollisions], bl
    jb near .SkipParticle

    cmp byte[esi + eax + PARTICLE.IsActive], 0
    je near .SkipParticle

    mov ebx, dword[esi + eax + PARTICLE.intX]
    sub ebx, dword[_intPlayerX]
    add ebx, SCREEN_WIDTH / 2

    mov edx, dword[esi + eax + PARTICLE.intY]
    sub edx, dword[_intPlayerY]
    add edx, SCREEN_HEIGHT / 2


    cmp dword[esi + eax + PARTICLE.ImgSizeType], SMALL_PARTICLE
    jne .NotSmallParticle

    sub ebx, SMALL_PARTICLE_WIDTH / 2
    sub edx, SMALL_PARTICLE_HEIGHT / 2

    mov edi, dword[_SmallParticleFlaresOff]
    add edi, dword[esi + eax + PARTICLE.ImgOffset]

    cmp ebx, (-MAP_WIDTH+SCREEN_WIDTH)
    jge .NotWrappingRight
    add ebx, MAP_WIDTH
.NotWrappingRight:
    cmp ebx, MAP_WIDTH
    jle .NotWrappingLeft
    sub ebx, MAP_WIDTH
.NotWrappingLeft:

    cmp edx, (-MAP_HEIGHT+SCREEN_HEIGHT)
    jge .NotWrappingDown
    add edx, MAP_HEIGHT
.NotWrappingDown:
    cmp edx, MAP_HEIGHT
    jle .NotWrappingUp
    sub edx, MAP_HEIGHT
.NotWrappingUp:

    invoke _AlphaBlit, dword ebx, dword edx, dword edi, dword SMALL_PARTICLE_WIDTH, dword SMALL_PARTICLE_HEIGHT


.NotSmallParticle:

    cmp dword[esi + eax + PARTICLE.ImgSizeType], LARGE_PARTICLE
    jne .NotLargeParticle

    sub ebx, LARGE_PARTICLE_WIDTH / 2
    sub edx, LARGE_PARTICLE_HEIGHT / 2

    mov edi, dword[_LargeParticleFlaresOff]
    add edi, dword[esi + eax + PARTICLE.ImgOffset]

    cmp ebx, (-MAP_WIDTH+SCREEN_WIDTH)
    jge .NotWrappingRight2
    add ebx, MAP_WIDTH
.NotWrappingRight2:
    cmp ebx, MAP_WIDTH
    jle .NotWrappingLeft2
    sub ebx, MAP_WIDTH
.NotWrappingLeft2:

    cmp edx, (-MAP_HEIGHT+SCREEN_HEIGHT)
    jge .NotWrappingDown2
    add edx, MAP_HEIGHT
.NotWrappingDown2:
    cmp edx, MAP_HEIGHT
    jle .NotWrappingUp2
    sub edx, MAP_HEIGHT
.NotWrappingUp2:

    invoke _AlphaBlit, dword ebx, dword edx, dword edi, dword LARGE_PARTICLE_WIDTH, dword LARGE_PARTICLE_HEIGHT

.NotLargeParticle:


.SkipParticle:

    add eax, PARTICLE_STRUCT_SIZE
    inc ecx
    cmp ecx, MAX_PARTICLES
    jb near .RenderParticleLoop

    emms

    pop edi
    pop esi
    ret


;====== MakeAlphaFromRGB ===================================================
;;; _MakeAlphaFromRGB
;;;   INPUTS: pixel in argb format
;;;  OUTPUTS: dword[_intPixel] = new pixel
;;;  PURPOSE: compute nice alpha values for particles
;;;    CALLS: none
_MakeAlphaFromRGB:
.src_pix        EQU    4
    pusha

    mov eax, dword[ebp + .src_pix]

    mov ebx, eax
    and ebx, 00FF0000h
    shr ebx, 16
    mov byte[_intbRed], bl

    mov ebx, eax
    and ebx, 0000FF00h
    shr ebx, 8
    mov byte[_intbGreen], bl

    mov ebx, eax
    and ebx, 000000FFh
    mov byte[_intbBlue], bl

    xor eax, eax
    xor ebx, ebx
    mov bl, byte[_intbRed]
    add eax, ebx
    mov bl, byte[_intbGreen]
    add eax, ebx
    mov bl, byte[_intbBlue]
    add eax, ebx

    xor edx, edx
    mov ebx, 3
    div ebx

    mov ebx, dword[ebp + .src_pix]
    and ebx, 00FFFFFFh

    cmp al, 4
    jb .DontSetAlpha

    shl eax, 24
    or  ebx, eax

.DontSetAlpha:

    mov dword[_intPixel], ebx

    popa
    ret


