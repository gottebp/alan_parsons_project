
;============================================================
; Project:      "The Alan Parsons Project"
; Description:  Code for ece291 final project
; Author:       Benjamin Gottemoller
; Website:      http://www.particlefield.com
; Date:        7/20/02
;============================================================

%include "defs.inc"
%include "sdl.inc"
%include "ppe.inc"
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
    EXTERN _GameTurnout
    EXTERN _PlayerShipFileN
    EXTERN _PlayerShipFileL
    EXTERN _PlayerShipFileR

    EXTERN _SNDEffect_Engines, _SNDEffect_EvilLaugh, _SNDEffect_Hit
    EXTERN _SNDEffect_Weapon, _SNDEffect_ExplosionArray
    EXTERN _SNDEngines_Counter, _SNDWeapon_Counter

GLOBAL _UpdatePlayer, _RenderPlayer, _LoadRaw, _FirePlayerWeapons, _FirePlayerMainThrusters, _InitPlayer, _DestroyPlayer
GLOBAL _FirePlayerLeftThrusters, _FirePlayerRightThrusters, _DrawPlayerHealth, _DetectPlayerCollisions

GLOBAL _intPlayerX, _intPlayerY, _intbPlayerAngle, _intbPlayerTurnDir
GLOBAL _fltPlayerX, _fltPlayerY, _fltPlayerSpeed, _fltPlayerStrafeSpeed
GLOBAL _PlayerShipOff, _PlayerShipOffL, _PlayerShipOffR
GLOBAL _fltPlayerAccel, _fltPlayerFriction, _fltPlayerMaxSpeed, _fltPlayerMinSpeed, _fltPlayerMass
GLOBAL _fltPlayerStrafeFriction, _fltPlayerMaxStrafeSpeed, _fltPlayerMinStrafeSpeed
GLOBAL _fltPlayerNoseX, _fltPlayerNoseY, _fltPlayerTailX, _fltPlayerTailY
GLOBAL _intPlayerNoseX, _intPlayerNoseY, _intPlayerTailX, _intPlayerTailY, _intPlayerHealth
GLOBAL _intPlayerWeaponsLevel


STRUC PARTICLE
    .IsActive                resb    1
    .DetectCollisions        resb    1                    ;Collision ID (0=none)
    .ImgSizeType             resd    1                    ;LARGE_PARTICLE or SMALL_PARTICLE
    .ImgOffset               resd    1                    ;Location of the particle in the image data
    .fltX                    resd    1
    .fltY                    resd    1
    .intX                    resd    1
    .intY                    resd    1
    .XV                      resd    1                    ;x vector
    .YV                      resd    1                    ;y vector
    .MaxLife                 resd    1                    ;lifetime (in frames)
    .Age                     resd    1                    ;current age
    .Damage                  resd    1                    ;Damage points it does
ENDSTRUC



SECTION .bss

_PlayerShipOff                       resd    1
_PlayerShipOffL                      resd    1
_PlayerShipOffR                      resd    1

_intbPlayerTurnDir                   resb    1
_intPlayerX                          resd    1
_intPlayerY                          resd    1
_intbPlayerAngle                     resb    1
_fltPlayerX                          resd    1
_fltPlayerY                          resd    1
_fltPlayerSpeed                      resd    1
_fltPlayerStrafeSpeed                resd    1

SECTION .data

_fltPlayerAccel                      dd 1.07
_fltPlayerFriction                   dd 0.17
_fltPlayerMaxSpeed                   dd 16.0
_fltPlayerMinSpeed                   dd -12.0
_fltPlayerMass                       dd 100.0

_fltPlayerStrafeFriction             dd 0.20
_fltPlayerMaxStrafeSpeed             dd 12.0
_fltPlayerMinStrafeSpeed             dd -12.0

_fltPlayerNoseX                      dd 0
_fltPlayerNoseY                      dd 0
_fltPlayerTailX                      dd 0
_fltPlayerTailY                      dd 0

_intPlayerNoseX                      dd 0
_intPlayerNoseY                      dd 0
_intPlayerTailX                      dd 0
_intPlayerTailY                      dd 0

_intPlayerHealth                     dd 0

_intPlayerWeaponsLevel               dd 0

;====== Temporary Variables Used For Anything Necessary ===================
_intTemp                             dd 0
_fltTemp                             dd 0

_intTop                              dd 0
_intBottom                           dd 0
_intLeft                             dd 0
_intRight                            dd 0

_fltFiringSpeed                      dd 10.32
_fltThrusterSpeed                    dd 5.32
_fltThrusterStrafeSpeed              dd 13.32

_intbWeaponModAngle                  db 0
_intbWMADirection                    db 0

_RoundingFactor                      dd 00800080h, 00800080h

_fltSpeedArray                       dd 9.2
                                     dd 10.32
                                     dd 12.88
                                     dd 14.742
                                     dd 15.8821
                                     dd 16.2332
                                     dd 17.1243
                                     dd 18.533

_intWeaponTimerArray                 dd 0
                                     dd 0
                                     dd 0
                                     dd 0
                                     dd 0
                                     dd 0

_read_binary                         db 'rb',0
_file_ptr                            dd 0

;====== InitPlayer ===================================================
;;; _InitPlayer
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: Init player data/memory
_InitPlayer:

    push dword (PLAYER_IMG_FILE_WIDTH * PLAYER_IMG_FILE_HEIGHT * 4)
    call malloc
    mov dword[_PlayerShipOff], eax
    add esp, 4

    push dword (PLAYER_IMG_FILE_WIDTH * PLAYER_IMG_FILE_HEIGHT * 4)
    call malloc
    mov dword[_PlayerShipOffL], eax
    add esp, 4

    push dword (PLAYER_IMG_FILE_WIDTH * PLAYER_IMG_FILE_HEIGHT * 4)
    call malloc
    mov dword[_PlayerShipOffR], eax
    add esp, 4

    invoke _LoadRaw, dword _PlayerShipFileN, dword[_PlayerShipOff], dword (PLAYER_IMG_FILE_WIDTH * PLAYER_IMG_FILE_HEIGHT * 4)
    invoke _LoadRaw, dword _PlayerShipFileL, dword[_PlayerShipOffL], dword (PLAYER_IMG_FILE_WIDTH * PLAYER_IMG_FILE_HEIGHT * 4)
    invoke _LoadRaw, dword _PlayerShipFileR, dword[_PlayerShipOffR], dword (PLAYER_IMG_FILE_WIDTH * PLAYER_IMG_FILE_HEIGHT * 4)

    ret


;====== DestroyPlayer ===================================================
;;; _DestroyPlayer
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: Deallocate player data/memory
_DestroyPlayer:

    push dword[_PlayerShipOff]
    call free
    add esp, 4

    push dword[_PlayerShipOffL]
    call free
    add esp, 4

    push dword[_PlayerShipOffR]
    call free
    add esp, 4

    ret

;====== UpdatePlayer ===================================================
;;; _UpdatePlayer
;;;   INPUTS: Global player variables
;;;  OUTPUTS: Updated position/data
;;;  PURPOSE: Update player global variables
;;;    CALLS: _ShipExplode
_UpdatePlayer:

    finit

    cmp dword[_intPlayerHealth], 0
    jge near .NotDead

    cmp byte[_GameTurnout], PLAYER_DEAD
    je near .SkipExplosion
    invoke _ShipExplode, dword[_fltPlayerX], dword[_fltPlayerY], dword 1, dword 0
    invoke _ShipExplode, dword[_fltPlayerX], dword[_fltPlayerY], dword 0, dword 0
    invoke _ShipExplode, dword[_fltPlayerX], dword[_fltPlayerY], dword 1, dword 0
    invoke _ShipExplode, dword[_fltPlayerX], dword[_fltPlayerY], dword 0, dword 0
    invoke _ShakeMap, dword 150

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

.SkipExplosion:
    mov byte[_GameTurnout], PLAYER_DEAD
.NotDead:

    xor eax, eax
    mov al, byte[_intbPlayerAngle]
    shl eax, 2

    mov dword[_intTemp], 60
    fld dword[_fltPlayerX]
    fild dword[_intTemp]
    fld dword[_COS_LOOK + eax]
    fmulp st1
    faddp st1
    fst dword[_fltPlayerNoseX]
    fistp dword[_intPlayerNoseX]

    mov dword[_intTemp], 60
    fld dword[_fltPlayerX]
    fild dword[_intTemp]
    fld dword[_COS_LOOK + eax]
    fmulp st1
    fsubp st1
    fst dword[_fltPlayerTailX]
    fistp dword[_intPlayerTailX]

    mov dword[_intTemp], 60
    fld dword[_fltPlayerY]
    fild dword[_intTemp]
    fld dword[_SIN_LOOK + eax]
    fmulp st1
    faddp st1
    fst dword[_fltPlayerNoseY]
    fistp dword[_intPlayerNoseY]

    mov dword[_intTemp], 60
    fld dword[_fltPlayerY]
    fild dword[_intTemp]
    fld dword[_SIN_LOOK + eax]
    fmulp st1
    fsubp st1
    fst dword[_fltPlayerTailY]
    fistp dword[_intPlayerTailY]


    fld dword[_COS_LOOK + eax]
    fld dword[_fltPlayerSpeed]
    fmulp st1, st0                 ;st0 = speed * cos(angle)
    fld dword[_fltPlayerX]
    faddp st1, st0                 ;st0 = PlayerX + (speed * cos(angle))
    fst dword[_fltPlayerX]
    fistp dword[_intPlayerX]

    fld dword[_SIN_LOOK + eax]
    fld dword[_fltPlayerSpeed]
    fmulp st1, st0                 ;st0 = speed * sin(angle)
    fld dword[_fltPlayerY]
    faddp st1, st0                 ;st0 = PlayerY + (speed * sin(angle))
    fst dword[_fltPlayerY]
    fistp dword[_intPlayerY]


    shr eax, 2
    add al, 64
    shl eax, 2

    fld dword[_COS_LOOK + eax]
    fld dword[_fltPlayerStrafeSpeed]
    fmulp st1, st0                 ;st0 = speed * cos(angle + 64)
    fld dword[_fltPlayerX]
    faddp st1, st0                 ;st0 = PlayerX + (speed * cos(angle + 64))
    fst dword[_fltPlayerX]
    fistp dword[_intPlayerX]

    fld dword[_SIN_LOOK + eax]
    fld dword[_fltPlayerStrafeSpeed]
    fmulp st1, st0                 ;st0 = speed * sin(angle + 64)
    fld dword[_fltPlayerY]
    faddp st1, st0                 ;st0 = PlayerY + (speed * sin(angle + 64))
    fst dword[_fltPlayerY]
    fistp dword[_intPlayerY]


    fld dword[_fltPlayerFriction]
    fld dword[_fltPlayerSpeed]
    fist dword[_intTemp]
    cmp dword[_intTemp], 0
    jne .SpeedNotZero

    fldz
    fstp dword[_fltPlayerSpeed]

.SpeedNotZero:

    cmp dword[_intTemp], 0
    jge .SpeedNotLess

    fadd st0, st1
    fst dword[_fltPlayerSpeed]

.SpeedNotLess:
    cmp dword[_intTemp], 0
    jle .SpeedNotGreater

    fsub st0, st1
    fst dword[_fltPlayerSpeed]

.SpeedNotGreater:
    fdecstp
    fdecstp


    fld dword[_fltPlayerStrafeFriction]
    fld dword[_fltPlayerStrafeSpeed]
    fist dword[_intTemp]
    cmp dword[_intTemp], 0
    jne .StrafeSpeedNotZero

    fldz
    fstp dword[_fltPlayerStrafeSpeed]

.StrafeSpeedNotZero:

    cmp dword[_intTemp], 0
    jge .StrafeSpeedNotLess

    fadd st0, st1
    fst dword[_fltPlayerStrafeSpeed]

.StrafeSpeedNotLess:
    cmp dword[_intTemp], 0
    jle .StrafeSpeedNotGreater

    fsub st0, st1
    fst dword[_fltPlayerStrafeSpeed]

.StrafeSpeedNotGreater:
    fdecstp
    fdecstp
    finit


    cmp dword[_intPlayerX], MAP_WIDTH
    jl .DontWrapMaxX

    mov dword[_intTemp], MAP_WIDTH
    fld dword[_fltPlayerX]
    fild dword[_intTemp]
    fsubp st1, st0
    fst dword[_fltPlayerX]
    fistp dword[_intPlayerX]

.DontWrapMaxX:

    cmp dword[_intPlayerX], 0
    jg .DontWrapMinX

    mov dword[_intTemp], MAP_WIDTH
    fld dword[_fltPlayerX]
    fild dword[_intTemp]
    faddp st1, st0
    fst dword[_fltPlayerX]
    fistp dword[_intPlayerX]

.DontWrapMinX:


    cmp dword[_intPlayerY], MAP_HEIGHT
    jl .DontWrapMaxY

    mov dword[_intTemp], MAP_HEIGHT
    fld dword[_fltPlayerY]
    fild dword[_intTemp]
    fsubp st1, st0
    fst dword[_fltPlayerY]
    fistp dword[_intPlayerY]

.DontWrapMaxY:

    cmp dword[_intPlayerY], 0
    jg .DontWrapMinY

    mov dword[_intTemp], MAP_HEIGHT
    fld dword[_fltPlayerY]
    fild dword[_intTemp]
    faddp st1, st0
    fst dword[_fltPlayerY]
    fistp dword[_intPlayerY]

.DontWrapMinY:

.Done:
    ret


;====== RenderPlayer ===================================================
;;; _RenderPlayer
;;;   INPUTS: ScreenOff, PlayerShip image data, and global player variables
;;;  OUTPUTS: Rendered Player Image
;;;  PURPOSE: Draws the player on the screen
;;;    CALLS: macro: ComputeAlpha
_RenderPlayer:

    push esi
    push edi

    cmp byte[_GameTurnout], PLAYER_DEAD
    je near .Done


    mov esi, dword[_ScreenOff]
    mov edi, dword[_PlayerShipOff]

    cmp byte[_intbPlayerTurnDir], -1
    jnle .NotTurningLeft

    mov edi, dword[_PlayerShipOffL]

.NotTurningLeft:

    cmp byte[_intbPlayerTurnDir], 1
    jnge .NotTurningRight

    mov edi, dword[_PlayerShipOffR]

.NotTurningRight:

    xor eax, eax
    mov al, byte[_intbPlayerAngle]
    mov ebx, (PLAYER_WIDTH * PLAYER_HEIGHT * 4)
    mul ebx
    add edi, eax

    mov eax, ((((SCREEN_HEIGHT / 2) - (PLAYER_HEIGHT / 2)) * SCREEN_WIDTH) + (SCREEN_WIDTH / 2) - (PLAYER_WIDTH / 2)) * 4

    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
.RenderLoop:

    ComputeAlpha dword[esi + eax], dword[edi + ebx], dword[esi + eax]

    inc ecx
    cmp ecx, PLAYER_WIDTH
    jb .DontIncrementRow

    xor ecx, ecx
    add eax, (SCREEN_WIDTH - PLAYER_WIDTH) * 4

.DontIncrementRow:

    add ebx, 4
    add eax, 4
    cmp ebx, PLAYER_WIDTH * PLAYER_HEIGHT * 4
    jb .RenderLoop

    emms

.Done:
    pop edi
    pop esi
    ret


;====== FirePlayerWeapons ===================================================
;;; _FirePlayerWeapons
;;;   INPUTS: global player variables
;;;  OUTPUTS: none
;;;  PURPOSE: Fire weapons
;;;    CALLS: _AddParticle
_FirePlayerWeapons:

    finit
    xor eax, eax

    cmp byte[_GameTurnout], PLAYER_DEAD
    je near .Done

    fld dword[_fltPlayerSpeed]
    fld dword[_fltSpeedArray + 4 * 1]
    faddp st1
    fstp dword[_fltTemp]

    xor ebx, ebx
    mov bl, byte[_intbPlayerAngle]

    ;DetectCollisions, ImgSizeType, ImgIndex, X, Y, Angle, Speed, MaxLife, Damage
    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 10, dword[_fltPlayerNoseX], dword[_fltPlayerNoseY], dword ebx, dword[_fltTemp], dword 30, dword 12

    cmp dword[_intPlayerWeaponsLevel], 1
    jb near .Done

    fld dword[_fltPlayerSpeed]
    fld dword[_fltSpeedArray + 4 * 3]
    faddp st1
    fstp dword[_fltTemp]

    call _Rand
    xor edx, edx
    mov ebx, 10
    div ebx
    xor ebx, ebx
    mov bl, byte[_intbPlayerAngle]
    add bl, dl
    sub bl, 5
    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 7, dword[_fltPlayerNoseX], dword[_fltPlayerNoseY], dword ebx, dword[_fltTemp], dword 30, dword 4

    cmp dword[_intPlayerWeaponsLevel], 2
    jb near .Done


    cmp dword[_intWeaponTimerArray + 0], 4
    jb .DontResetTimer1
    mov dword[_intWeaponTimerArray + 0], 0
.DontResetTimer1:
    inc dword[_intWeaponTimerArray + 0]

    cmp dword[_intWeaponTimerArray + 0], 1
    ja near .SkipWeapon3

    fld dword[_fltPlayerSpeed]
    fld dword[_fltSpeedArray + 0]
    faddp st1
    fstp dword[_fltTemp]

    xor ebx, ebx
    mov bl, byte[_intbPlayerAngle]
    add bl, 20
    invoke _AddParticle, dword 1, dword LARGE_PARTICLE, dword 6, dword[_fltPlayerNoseX], dword[_fltPlayerNoseY], dword ebx, dword[_fltTemp], dword 20, dword 1

    xor ebx, ebx
    mov bl, byte[_intbPlayerAngle]
    sub bl, 20
    invoke _AddParticle, dword 1, dword LARGE_PARTICLE, dword 6, dword[_fltPlayerNoseX], dword[_fltPlayerNoseY], dword ebx, dword[_fltTemp], dword 20, dword 1
.SkipWeapon3:


    cmp dword[_intPlayerWeaponsLevel], 3
    jb near .Done

    fld dword[_fltPlayerSpeed]
    fld dword[_fltSpeedArray + 4 * 0]
    faddp st1
    fstp dword[_fltTemp]

    cmp byte[_intbWMADirection], 0
    jne .NotIncreasing
    add byte[_intbWeaponModAngle], 1
.NotIncreasing:

    cmp byte[_intbWMADirection], 1
    jne .NotDecreasing
    sub byte[_intbWeaponModAngle], 1
.NotDecreasing:

    cmp byte[_intbWeaponModAngle], 16
    jl .DontSetDecrement
    mov byte[_intbWMADirection], 1
.DontSetDecrement:

    cmp byte[_intbWeaponModAngle], -16
    jg .DontSetIncrement
    mov byte[_intbWMADirection], 0
.DontSetIncrement:

    xor ebx, ebx
    mov bl, byte[_intbWeaponModAngle]
    add bl, byte[_intbPlayerAngle]
    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 12, dword[_fltPlayerNoseX], dword[_fltPlayerNoseY], dword ebx, dword[_fltTemp], dword 30, dword 4

    xor ebx, ebx
    mov bl, byte[_intbWeaponModAngle]
    neg bl
    add bl, byte[_intbPlayerAngle]
    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 12, dword[_fltPlayerNoseX], dword[_fltPlayerNoseY], dword ebx, dword[_fltTemp], dword 30, dword 4


    cmp dword[_intPlayerWeaponsLevel], 4
    jb near .Done


    cmp dword[_intWeaponTimerArray + 4], 10
    jb .DontResetTimer2
    mov dword[_intWeaponTimerArray + 4], 0
.DontResetTimer2:
    inc dword[_intWeaponTimerArray + 4]

    cmp dword[_intWeaponTimerArray + 4], 8
    ja near .SkipWeapon4

    xor ebx, ebx
    mov bl, byte[_intbPlayerAngle]
    add bl, 64
    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 15, dword[_fltPlayerNoseX], dword[_fltPlayerNoseY], dword ebx, dword[_fltSpeedArray + 0], dword 22, dword 3

    xor ebx, ebx
    mov bl, byte[_intbPlayerAngle]
    sub bl, 64
    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 15, dword[_fltPlayerNoseX], dword[_fltPlayerNoseY], dword ebx, dword[_fltSpeedArray + 0], dword 22, dword 3
.SkipWeapon4:


    cmp dword[_intPlayerWeaponsLevel], 5
    jb near .Done

    cmp dword[_intWeaponTimerArray + 8], 40
    jb .DontResetTimer3
    mov dword[_intWeaponTimerArray + 8], 0
.DontResetTimer3:
    inc dword[_intWeaponTimerArray + 8]

    cmp dword[_intWeaponTimerArray + 8], 1
    ja near .SkipWeapon5


    xor edx, edx
.ShockLoop:
    pusha
    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 6, dword[_fltPlayerX], dword[_fltPlayerY], dword edx, dword[_fltSpeedArray + 4 * 4], dword 10, dword 2
    popa

    add edx, 8
    cmp edx, 256
    jb .ShockLoop


.SkipWeapon5:


.Done:

    ret


;====== FirePlayerMainThrusters ===================================================
;;; _FirePlayerThrusters
;;;   INPUTS: Global Player variables
;;;  OUTPUTS: none
;;;  PURPOSE: Fire engines
;;;    CALLS: _AddParticle
_FirePlayerMainThrusters:

    finit
    xor eax, eax

    cmp byte[_GameTurnout], PLAYER_DEAD
    je near .Done

    fld dword[_fltPlayerSpeed]
    fld dword[_fltThrusterSpeed]
    fsubp st1
    fstp dword[_fltTemp]

    call _Rand
    xor edx, edx
    mov ebx, 10
    div ebx
    xor ebx, ebx
    mov bl, byte[_intbPlayerAngle]
    add bl, dl
    sub bl, 5

    ;DetectCollisions, ImgSizeType, ImgIndex, X, Y, Angle, Speed, MaxLife, Damage
    invoke _AddParticle, dword 0, dword LARGE_PARTICLE, dword 6, dword[_fltPlayerTailX], dword[_fltPlayerTailY], dword ebx, dword[_fltTemp], dword 16, dword 0

    call _Rand
    xor edx, edx
    mov ebx, 10
    div ebx
    xor ebx, ebx
    mov bl, byte[_intbPlayerAngle]
    add bl, dl
    sub bl, 5

    invoke _AddParticle, dword 0, dword LARGE_PARTICLE, dword 7, dword[_fltPlayerTailX], dword[_fltPlayerTailY], dword ebx, dword[_fltTemp], dword 21, dword 0

.Done:
    ret


;====== FirePlayerLeftThrusters ===================================================
;;; _FirePlayerLeftThrusters
;;;   INPUTS: Global player variables
;;;  OUTPUTS: none
;;;  PURPOSE: Fire left thrusters
;;;    CALLS: _AddParticle
_FirePlayerLeftThrusters:

    finit
    xor eax, eax

    cmp byte[_GameTurnout], PLAYER_DEAD
    je near .Done

    xor ebx, ebx
    mov bl, byte[_intbPlayerAngle]
    add bl, 64

    ;DetectCollisions, ImgSizeType, ImgIndex, X, Y, Angle, Speed, MaxLife, Damage
    invoke _AddParticle, dword 0, dword SMALL_PARTICLE, dword 11, dword[_fltPlayerX], dword[_fltPlayerY], dword ebx, dword[_fltThrusterStrafeSpeed], dword 2, dword 0

.Done:
    ret



;====== FirePlayerRightThrusters ===================================================
;;; _FirePlayerRightThrusters
;;;   INPUTS: Global player variables
;;;  OUTPUTS: none
;;;  PURPOSE: Fire right thrusters
;;;    CALLS: _AddParticle
_FirePlayerRightThrusters:

    finit
    xor eax, eax

    cmp byte[_GameTurnout], PLAYER_DEAD
    je near .Done

    xor ebx, ebx
    mov bl, byte[_intbPlayerAngle]
    sub bl, 64

    ;DetectCollisions, ImgSizeType, ImgIndex, X, Y, Angle, Speed, MaxLife, Damage
    invoke _AddParticle, dword 0, dword SMALL_PARTICLE, dword 11, dword[_fltPlayerX], dword[_fltPlayerY], dword ebx, dword[_fltThrusterStrafeSpeed], dword 2, dword 0

.Done:
    ret



;====== LoadRaw ===================================================
;;; _LoadRaw
;;;   INPUTS: file name, buffer, size
;;;  OUTPUTS: Loaded object data
;;;  PURPOSE: Load objects in the game
;;;    CALLS: _OpenFile, _ReadFile, _CloseFile
_LoadRaw:
.file_str    EQU    4
.buffer         EQU    8
.size           EQU     12

    push esi
    push edi

    invoke fopen, dword[ebp + .file_str], dword _read_binary
    mov dword[_file_ptr], eax
    invoke fread, dword[ebp + .buffer], dword[ebp + .size], dword 1, dword[_file_ptr]
    invoke fclose, dword[_file_ptr]

    mov esi, dword[ebp + .buffer]
    xor eax, eax
    xor ecx, ecx
.SetupAlphaTransparency:

    mov ecx, dword[esi + eax]
    cmp ecx, 0FF00FFFFh
    jne .SkipPixel

    mov dword[esi + eax], 00000000h

.SkipPixel:

    add eax, 4
    cmp eax, dword[ebp + .size]
    jb .SetupAlphaTransparency

    pop edi
    pop esi
    ret



;====== DrawPlayerHealth ===================================================
;;; _DrawPlayerHealth
;;;   INPUTS: [_intPlayerHealth], health of player
;;;  OUTPUTS: Draws health bar on [_ScreenOff]
;;;  PURPOSE: Draw player health
;;;    CALLS: macro ComputeAlpha
_DrawPlayerHealth:

    pusha

    cmp byte[_GameTurnout], PLAYER_DEAD
    je near .Done

    mov edi, SCREEN_WIDTH*4*5+15*4    ; Start five lines down, SCREEN_WIDTH/4 pixels in
    mov esi, PLAYER_HEALTH_BAR_HEIGHT

    mov eax, dword [_intPlayerHealth]
    cmp ax, 0
    jle near .Done
    cmp ax, MAXPLAYERHEALTH
    ja near .Done


    mov ebx, SCREEN_WIDTH-30
    mul ebx                                   ; Health * Screen_width in edx:eax (eax is all that matters)
    mov ebx, MAXPLAYERHEALTH
    div ebx                                    ; now ax stores pixels into screen

    xor ecx, ecx
    mov cx, ax                                ; cx holds number of pixels to draw to
    mov ebx, ecx
    mov edx, SCREEN_WIDTH*4*5+15*4    ; remembers last row on screen
    ; eax, ecx, ebx, 280h each
.rowloop:
    add edi, [_ScreenOff]
.colloop:
    mov eax, 0800000FFh
    ComputeAlpha dword [edi], dword eax, dword [edi]
    add edi, 4
    dec cx
    jnz .colloop
    dec esi
    jz .Done
    lea edx, [edx + 4*SCREEN_WIDTH]
    mov edi, edx
    mov ecx, ebx
    jmp .rowloop

.Done:

    popa
    emms
    ret



;====== DetectPlayerCollisions ===================================================
;;; _DetectPlayerCollisions
;;;   INPUTS: Global player and particle variables
;;;  OUTPUTS: Updated player and particle data based on collisions
;;;  PURPOSE: Detects collisions and calculates damage
;;;    CALLS: _ShipExplode
_DetectPlayerCollisions:

    push esi
    push edi

    mov esi, dword[_ParticleDataOff]

    xor eax, eax
    xor ebx, ebx
    xor edx, edx
    xor ecx, ecx
.ParticleLoop:

    cmp byte[esi + eax + PARTICLE.IsActive], 0
    je near .SkipParticle
    cmp byte[esi + eax + PARTICLE.DetectCollisions], 0
    je near .SkipParticle
    cmp byte[esi + eax + PARTICLE.DetectCollisions], 1
    je near .SkipParticle

    cmp dword[esi + eax + PARTICLE.ImgSizeType], SMALL_PARTICLE
    jne near .NotSmallParticle

    mov ebx, dword[esi + eax + PARTICLE.intX]
    sub ebx, SMALL_PARTICLE_WIDTH / 2
    mov edx, dword[esi + eax + PARTICLE.intY]
    sub edx, SMALL_PARTICLE_HEIGHT / 2

    cmp dword[_intPlayerX], ebx
    jl .NotPlayerMiddle
    add ebx, SMALL_PARTICLE_WIDTH
    cmp dword[_intPlayerX], ebx
    jg .NotPlayerMiddle
    cmp dword[_intPlayerY], edx
    jl .NotPlayerMiddle
    add edx, SMALL_PARTICLE_HEIGHT
    cmp dword[_intPlayerY], edx
    jg .NotPlayerMiddle

    jmp .HandleCollision

.NotPlayerMiddle:


    mov ebx, dword[esi + eax + PARTICLE.intX]
    sub ebx, SMALL_PARTICLE_WIDTH / 2
    mov edx, dword[esi + eax + PARTICLE.intY]
    sub edx, SMALL_PARTICLE_HEIGHT / 2

    cmp dword[_intPlayerNoseX], ebx
    jl .NotPlayerNose
    add ebx, SMALL_PARTICLE_WIDTH
    cmp dword[_intPlayerNoseX], ebx
    jg .NotPlayerNose
    cmp dword[_intPlayerNoseY], edx
    jl .NotPlayerNose
    add edx, SMALL_PARTICLE_HEIGHT
    cmp dword[_intPlayerNoseY], edx
    jg .NotPlayerNose

    jmp .HandleCollision

.NotPlayerNose:

    mov ebx, dword[esi + eax + PARTICLE.intX]
    sub ebx, SMALL_PARTICLE_WIDTH / 2
    mov edx, dword[esi + eax + PARTICLE.intY]
    sub edx, SMALL_PARTICLE_HEIGHT / 2

    cmp dword[_intPlayerTailX], ebx
    jl .NotPlayerTail
    add ebx, SMALL_PARTICLE_WIDTH
    cmp dword[_intPlayerTailX], ebx
    jg .NotPlayerTail
    cmp dword[_intPlayerTailY], edx
    jl .NotPlayerTail
    add edx, SMALL_PARTICLE_HEIGHT
    cmp dword[_intPlayerTailY], edx
    jg .NotPlayerTail

    jmp .HandleCollision

.NotPlayerTail:

    jmp .SkipParticle

.NotSmallParticle:

    cmp dword[esi + eax + PARTICLE.ImgSizeType], LARGE_PARTICLE
    jne near .NotLargeParticle


    mov ebx, dword[esi + eax + PARTICLE.intX]
    sub ebx, LARGE_PARTICLE_WIDTH / 2
    mov edx, dword[esi + eax + PARTICLE.intY]
    sub edx, LARGE_PARTICLE_HEIGHT / 2

    cmp dword[_intPlayerX], ebx
    jl .NotPlayerMiddle2
    add ebx, LARGE_PARTICLE_WIDTH
    cmp dword[_intPlayerX], ebx
    jg .NotPlayerMiddle2
    cmp dword[_intPlayerY], edx
    jl .NotPlayerMiddle2
    add edx, LARGE_PARTICLE_HEIGHT
    cmp dword[_intPlayerY], edx
    jg .NotPlayerMiddle2

    jmp .HandleCollision

.NotPlayerMiddle2:


    mov ebx, dword[esi + eax + PARTICLE.intX]
    sub ebx, LARGE_PARTICLE_WIDTH / 2
    mov edx, dword[esi + eax + PARTICLE.intY]
    sub edx, LARGE_PARTICLE_HEIGHT / 2

    cmp dword[_intPlayerNoseX], ebx
    jl .NotPlayerNose2
    add ebx, LARGE_PARTICLE_WIDTH
    cmp dword[_intPlayerNoseX], ebx
    jg .NotPlayerNose2
    cmp dword[_intPlayerNoseY], edx
    jl .NotPlayerNose2
    add edx, LARGE_PARTICLE_HEIGHT
    cmp dword[_intPlayerNoseY], edx
    jg .NotPlayerNose2

    jmp .HandleCollision

.NotPlayerNose2:

    mov ebx, dword[esi + eax + PARTICLE.intX]
    sub ebx, LARGE_PARTICLE_WIDTH / 2
    mov edx, dword[esi + eax + PARTICLE.intY]
    sub edx, LARGE_PARTICLE_HEIGHT / 2

    cmp dword[_intPlayerTailX], ebx
    jl .NotPlayerTail2
    add ebx, LARGE_PARTICLE_WIDTH
    cmp dword[_intPlayerTailX], ebx
    jg .NotPlayerTail2
    cmp dword[_intPlayerTailY], edx
    jl .NotPlayerTail2
    add edx, LARGE_PARTICLE_HEIGHT
    cmp dword[_intPlayerTailY], edx
    jg .NotPlayerTail2

    jmp .HandleCollision

.NotPlayerTail2:

    jmp .SkipParticle

.NotLargeParticle:


.HandleCollision:

    mov byte[esi + eax + PARTICLE.IsActive], 0
    mov ebx, dword[esi + eax + PARTICLE.Damage]
    sub dword[_intPlayerHealth], ebx

    pusha
    invoke _ShipExplode, dword[esi + eax + PARTICLE.fltX], dword[esi + eax + PARTICLE.fltY], dword 10, dword 1
    invoke _ShipExplode, dword[esi + eax + PARTICLE.fltX], dword[esi + eax + PARTICLE.fltY], dword 10, dword 1
    invoke Mix_PlayChannelTimed, dword -1, dword[_SNDEffect_Hit], dword 0, dword -1
    popa

.SkipParticle:

    add eax, PARTICLE_STRUCT_SIZE
    inc ecx
    cmp ecx, MAX_PARTICLES
    jb near .ParticleLoop


    pop edi
    pop esi
    ret



