
;============================================================
; Project:      "The Alan Parsons Project"
; Description:  Code for ece291 final project
; Author:       Scott Moeller
; Website:
; Date:        7/20/02
;============================================================

%include "macros.inc"
%include "sdl.inc"
%include "rand.inc"
%include "ppe.inc"
%include "player.inc"
%include "ai.inc"
%include "defs.inc"
%include "mapeng.inc"
%include "sse_mem.inc"

BITS 32


    EXTERN _InitializeGameData
    EXTERN _ProcessInput
    EXTERN _WaitForRetrace

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
    EXTERN _Rand

    EXTERN _SNDEvil
    EXTERN _SNDExplodeEnemy
    EXTERN _GameTurnout

    EXTERN _SNDEffect_Engines, _SNDEffect_EvilLaugh, _SNDEffect_Hit
    EXTERN _SNDEffect_Weapon, _SNDEffect_ExplosionArray

GLOBAL _InitializeEnemies, _UpdateEnemies, _RenderEnemies, _LoadEnemyData, _DestroyEnemyData
GLOBAL _SpawnFrames, _Enemies

;;; I take care to make sure this structure uses data falling on four byte blocks, so access is faster (this array or 100 enemies is accessed very frequently)
STRUC ENEMY
    .EnemyType               resd    1
    .EnemyXFloat             resd    1
    .EnemyYFloat             resd    1
    .EnemyXVelFloat          resd    1
    .EnemyYVelFloat          resd    1
    .EnemyMass               resd    1
    .EnemyAngle              resb    1
    .EnemySize               resb    1
    .EnemyActive             resb    1
    .Padding                 resb    1
    .EnemyXInt               resd    1
    .EnemyYInt               resd    1
    .EnemyHealth             resd    1
ENDSTRUC

STRUC PARTICLE
    .IsActive                resb    1
    .DetectCollisions        resb    1
    .ImgSizeType             resd    1                    ;LARGE_PARTICLE or SMALL_PARTICLE
    .ImgOffset               resd    1
    .fltX                    resd    1
    .fltY                    resd    1
    .intX                    resd    1
    .intY                    resd    1
    .XV                      resd    1
    .YV                      resd    1
    .MaxLife                 resd    1
    .Age                     resd    1
    .Damage                  resd    1
ENDSTRUC

SECTION .bss
_SmallEnemyImageData resd 1
_BossImageData       resd 1

;;; I use doubles to store this data for faster access (4 byte boundaries are quicker to access)
_SpawnFrames:                 ; -1 frame time means boss, the enemy count following that will be total number of enemies(minus 1 - boss)
    resd    1             ; Set second frame on which enemies spawn
    resd    1             ; enemy count up to second frame
    resd    16            ; Remaining frame information

SECTION .data
_SmallEnemyImageDataAddr db './data/small_enemies.bmp',0
_BossImageDataAddr       db './data/large_enemies.bmp',0


_FrameNum            dd 0
_NextWaveNumber      dd 0       ; Index into _SpawnFrames to look for next spawn time

_EnemyMassLookup:
    dd      35.0          ; Mass of enemy zero
    dd      40.0          ; Mass of enemy one
    dd      60.0          ; Mass of enemy two
    dd      80.0          ; Mass of enemy three



_EnemiesInit:
    ;;; Sample Initialization values
    dd     -1          ; Enemy Index (for enemy images file)
    dd     -1.0        ; Enemy X Position
    dd     -1.0        ; Enemy Y Position
    dd     0.0         ; Float enemy X speed
    dd     0.0         ; Float enemy Y speed
    dd     -1.0        ; Float enemy mass
    db     0           ; byte Enemy angle
    db     0           ; Enemy image size (0 small, 1 lage - boss)
    db     -1          ; Enemy Active - set to one when frame number is passed (less compares in draw this way)
    db     0           ; Padding for 4 byte memory size multiple
    dd     -1          ; Enemy X int
    dd     -1          ; Enemy Y int
    dd     -1          ; Enemy Health

_Enemies:
    ;;; All enemy data as described by STRUCT ENEMY - 100 enemies of data max
    times 1000 dd -1

_MapData:
    ;;; MENU_LOAD_SHIRE Examle data: ID 0
    db     3          ; Number of waves of attack before main boss
    db     1;8          ; Number of enemies per wave
    db     0           ; Boss ID (for indexing into boss images)
    db     0           ; Padding to make 16 bytes
    dd     400.0       ; Boss X
    dd     400.0       ; Boss Y
    dd     300.0       ; Boss Mass

    ;;; MENU_LOAD_ARCHIPELAGO Examle data: ID 3
    db     4           ; Number of waves of attack before main boss
    db     10          ; Number of enemies per wave
    db     1           ; Boss ID (for indexing into boss images)
    db     0           ; Padding to make 16 bytes
    dd     400.0       ; Boss X
    dd     400.0       ; Boss Y
    dd     400.0       ; Boss Mass

    ;;; MENU_LOAD_DUNE Examle data: ID 4
    db     2           ; Number of waves of attack before main boss
    db     30          ; Number of enemies per wave
    db     1           ; Boss ID (for indexing into boss images)
    db     0           ; Padding to make 16 bytes
    dd     400.0       ; Boss X
    dd     400.0       ; Boss Y
    dd     450.0       ; Boss Mass

    ;;; MENU_LOAD_MIDKEMIA Examle data: ID 2
    db     7           ; Number of waves of attack before main boss
    db     10          ; Number of enemies per wave
    db     2           ; Boss ID (for indexing into boss images)
    db     0           ; Padding to make 16 bytes
    dd     400.0       ; Boss X
    dd     400.0       ; Boss Y
    dd     500.0       ; Boss Mass

    ;;; MENU_LOAD_OCEANIA Examle data: ID 5
    db     4           ; Number of waves of attack before main boss
    db     20          ; Number of enemies per wave
    db     2           ; Boss ID (for indexing into boss images)
    db     0           ; Padding to make 16 bytes
    dd     400.0       ; Boss X
    dd     400.0       ; Boss Y
    dd     650.0       ; Boss Mass

    ;;; MENU_LOAD_MORDOR Examle data: ID 1
    db     4           ; Number of waves of attack before main boss
    db     24          ; Number of enemies per wave
    db     3           ; Boss ID (for indexing into boss images)
    db     0           ; Padding to make 16 bytes
    dd     400.0       ; Boss X
    dd     400.0       ; Boss Y
    dd     800.0       ; Boss Mass



;====== Temporary Variables Used For Anything Necessary ===================
_intTemp                             dd 0
_fltTemp                             dd 0

_intTop                              dd 0
_intBottom                           dd 0
_intLeft                             dd 0
_intRight                            dd 0

_memory_error           db      10,'Memory allocation error occured in _LoadEnemyData',10,0

;====== LoadEnemyData =================================================
;;; _LoadEnemyData
;;;   INPUTS: NONE
;;;  OUTPUTS: load in small and large enemy files
;;;  PURPOSE: to load enemy images
_LoadEnemyData:

    push dword (SMALLENEMYMEMSIZE)
    call malloc
    mov [_SmallEnemyImageData], eax
    add esp, 4
    cmp eax, 0
    je near .memerror

    invoke _LoadBMP, dword[_SmallEnemyImageData], dword _SmallEnemyImageDataAddr

    mov esi, dword[_SmallEnemyImageData]
    xor eax, eax
    xor ecx, ecx
.SetupAlphaTransparency:

    mov ecx, dword[esi + eax]
    cmp ecx, 0FF00FFFFh
    jne .SkipPixel

    mov dword[esi + eax], 00000000h

.SkipPixel:

    add eax, 4
    cmp eax, SMALLENEMYMEMSIZE
    jb .SetupAlphaTransparency


    push dword (BOSSENEMYMEMSIZE)
    call malloc
    mov dword[_BossImageData], eax
    add esp, 4
    cmp eax, 0
    je near .memerror

    invoke _LoadBMP, dword[_BossImageData], dword _BossImageDataAddr

    mov esi, dword[_BossImageData]
    xor eax, eax
    xor ecx, ecx
.SetupAlphaTransparency2:

    mov ecx, dword[esi + eax]
    cmp ecx, 0FF00FFFFh
    jne .SkipPixel2

    mov dword[esi + eax], 00000000h

.SkipPixel2:

    add eax, 4
    cmp eax, BOSSENEMYMEMSIZE
    jb .SetupAlphaTransparency2

    ret

.memerror:
    invoke printf, dword _memory_error
    ret


;====== DestroyEnemyData =================================================
;;; _DestroyEnemyData
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: Loads enemy image data
_DestroyEnemyData:

    push dword[_SmallEnemyImageData]
    call free
    add esp, 4

    push dword[_BossImageData]
    call free
    add esp, 4

    ret


;====== InitializeEnemies =================================================
;;; _InitializeEnemies
;;;   INPUTS: .MapDescriptor, determines which map will be played
;;;  OUTPUTS: _Enemies array Initialized, varies upon map being loaded
;;;           _SpawnFrames Initializes, which determines when and how many enemies will be displayed
;;;           _FrameNum resets - counts current frame number for wave timing
;;;           _NextWaveNumber reset to zero
;;;  PURPOSE: Initializes data in _Enemies array to values appropriate for the map being played,
;;;           and initializes data in _SpawnFrames
;;;  CALLS:   _Rand
_InitializeEnemies:
    .MapDescriptor    EQU     4
    pusha

    pusha
    invoke _sseMemset32, dword _Enemies, dword 0, dword 1000
    popa

    xor eax, eax
.ClearEnemyDataLoop:

    mov dword[_Enemies + eax + ENEMY.EnemyType], -1
    mov byte[_Enemies + eax + ENEMY.EnemyActive], -1
    mov dword[_Enemies + eax + ENEMY.EnemyHealth], -1

    add eax, 40
    cmp eax, 40*100
    jb .ClearEnemyDataLoop

    mov dword [_FrameNum], 0              ; Reset current frame number
    mov dword [_NextWaveNumber], 0

    ;;; Calculate and set all random spawn wave times
    mov edi, _SpawnFrames
    mov esi, [ebp+.MapDescriptor]         ; esi gains map descriptor number
    shl esi, 4                            ; Multiply by sixteen
    mov cl, [_MapData+esi]
    mov ch, cl                            ; Preserve number of waves in ch
    xor edx, edx                          ; edx will store progressive spawn frames
.SpawnFrameLoop:
    call _Rand
    and eax, 00000FFFh                    ; 2^12 max value
    shr eax, RANDSPAWNDIVISOR             ; now eax 2^10 max value
    add eax, 800                          ; 800 frames + rand 1000
 ;     mov eax, 300
    add edx, eax
    mov dword [edi], edx
    add edi, 8
    dec cl
    jnz .SpawnFrameLoop
    ;;; DONE
    mov dword [edi], -1

    ;;; calculate and set all enemy counts for wave times
    mov edi, _SpawnFrames
    mov edx, 0                            ; edx will store progressive enemy count
    mov ebx, 0
    mov bl, [_MapData+esi+1]              ; Number of enemies per spawn
    mov cl, ch
.SpawnCountLoop:
    add edx, ebx                          ; Add number of enemies added per wave
    mov dword [edi+4], edx                ; Store cumulative number of enemies so far
    add edi, 8
    dec cl
    jnz .SpawnCountLoop
    ;;; DONE
    inc edx
    mov dword [edi+4], edx

    xor eax, eax
    mov al, [_MapData + esi + 1]
    mov bl, al                             ; move number of enemies per wave into bl
    mul ch
    mov ecx, eax                           ; ecx gains total number of bad-dudes to be generated

    push ebx                               ; Save number of enemies until after we initialize active setting to -1

    finit

    xor edi, edi
.GenerateEnemiesLoop:
    ;;; Generate x and y coordinates
    mov edx, 0
    call _Rand                             ; eax gains random num
    mov ebx, MAP_WIDTH
    div ebx                                ; edx contains remainder - X we want

    mov [_intTemp], edx
    fild dword [_intTemp]
    fst dword [_Enemies+edi+ENEMY.EnemyXFloat]             ; Store in X of Enemy the value of eax
    fistp dword [_Enemies+edi+ENEMY.EnemyXInt]
    ;;; X DONE
    mov edx, 0
    call _Rand                             ; eax gains random num
    mov ebx, MAP_HEIGHT
    div ebx                                ; edx contains remainder - X we want

    mov [_intTemp], edx
    fild dword [_intTemp]
    fst dword [_Enemies+edi+ENEMY.EnemyYFloat]             ; Store in Y of Enemy the value of eax
    fistp dword [_Enemies+edi+ENEMY.EnemyYInt]
    ;;; Y DONE

    ;;; Calculate Enemy number
    call _Rand
    and eax, 00000003h                     ; eax value from 0 to 4
    mov [_Enemies+edi], eax                ; Save enemy number
    ;;; DONE

    ;;; Store enemy Mass associated with that enemy number
    shr eax, 2                             ; 4 bytes per mass
    mov edx, [_EnemyMassLookup+eax]
    mov [_Enemies+edi+ENEMY.EnemyMass], edx
    fld dword [_EnemyMassLookup+eax]
    mov dword [_intTemp], MASS_HEALTH_MULTIPLIER
    fild dword [_intTemp]
    fmulp st1
    fistp dword [_Enemies+edi+ENEMY.EnemyHealth]
    ;;; DONE

    ;;; Store X and Y initial velocities
    mov eax, [_EnemiesInit+ENEMY.EnemyXVelFloat]
    mov [_Enemies+edi+ENEMY.EnemyXVelFloat], eax
    mov eax, [_EnemiesInit+ENEMY.EnemyYVelFloat]
    mov [_Enemies+edi+ENEMY.EnemyYVelFloat], eax

    ;;; Set enemy angle
    xor eax, eax
    mov al, byte[_EnemiesInit+ENEMY.EnemyAngle]
    mov byte[_Enemies+edi+ENEMY.EnemyAngle], al

    ;;; Set enemy inactive
    mov byte [_Enemies+edi+ENEMY.EnemyActive], -1

    ;;; Set enemy size to small
    mov byte [_Enemies+edi+ENEMY.EnemySize], 0

    add edi, 40                            ; Increment to next enemy location in memory
    dec ecx                                ; subtract one from num baddies to make
    jnz near .GenerateEnemiesLoop
    ;;; Done generating the little guys


    ;;; Store boss now
    ;;; Boss ID lookup
    xor eax, eax
    mov al, [_MapData + esi + 2]
    mov [_Enemies+edi+ENEMY.EnemyType], eax

    ;;; Store initial x and y velocities
    mov eax, [_EnemiesInit+ENEMY.EnemyXVelFloat]
    mov [_Enemies+edi+ENEMY.EnemyXVelFloat], eax

    mov eax, [_EnemiesInit+ENEMY.EnemyYVelFloat]
    mov [_Enemies+edi+ENEMY.EnemyYVelFloat], eax

    ;;; Boss X lookup
    mov eax, [_MapData + esi + 4]
    mov [_Enemies+edi+ENEMY.EnemyXFloat], eax
    mov dword [_Enemies+edi+ENEMY.EnemyXInt], 400
    ;;; X DONE

    ;;; Boss Y lookup
    mov eax, [_MapData + esi + 8]
    mov [_Enemies+edi+ENEMY.EnemyYFloat], eax
    mov dword [_Enemies+edi+ENEMY.EnemyYInt], 400
    ;;; Y DONE

    ;;; Boss Mass lookup
    mov eax, [_MapData + esi + 12]
    mov [_Enemies + edi + ENEMY.EnemyMass], eax
    fld dword [_Enemies+edi+ENEMY.EnemyMass]
    mov dword [_intTemp], MASS_HEALTH_MULTIPLIER
    fild dword [_intTemp]
    fmulp st1
    fistp dword [_Enemies+edi+ENEMY.EnemyHealth]
    ;;; DONE

    ;;; Set Boss size to 1
    mov byte [_Enemies + edi + ENEMY.EnemySize], 1
    ;;; Done

    ;;; Set Boss to inactive
    mov byte [_Enemies + edi + ENEMY.EnemyActive], -1

    ;;; Set (Boss+1)th enemy id to -1 (signals end of enemies)
    add edi, 40
    mov byte[_Enemies+edi+ENEMY.EnemyActive], -1

    ;;; Set first wave of enemies to active
    pop ebx                                ; restore enemy count per wave to bl
    mov edi, 0
.SetActive:
    mov byte [_Enemies+edi+ENEMY.EnemyActive], 1
    add edi, 40
    dec bl
    jnz .SetActive

    popa
    ret


;====== UpdateEnemies ===================================================
;;; _UpdateEnemies
;;;   INPUTS:
;;;  OUTPUTS: Updated position/data
;;;  PURPOSE:
_UpdateEnemies:
    pusha

    call _ENEMY_MOVE

    inc dword [_FrameNum]

    mov edi, 0
    mov esi, [_NextWaveNumber]
    shl esi, 3                            ; Multiply frame number by eight to get index into _SpawnFrames
    mov ecx, [_SpawnFrames+esi+4]         ; Store enemy number in ecx (will be less than 100, so cl is all that is needed)
    push dword [_SpawnFrames+esi]         ; Save for later the value of frame number to next
    mov ch, cl                            ; Save copy of enemy number into ch
    mov esi, 0
    ;;; edi is zero
    ;;; ch, cl have number of enemies
.UpdateLoop:
    ;;; If not active, skip it!
    cmp byte [_Enemies+esi+ENEMY.EnemyActive], -1
    je near .SkipUpdate

    invoke _EnemyCollisionDetection, dword esi
    cmp dword [_Enemies+esi+ENEMY.EnemyHealth], 0
    jge .NotDead
    mov eax, 0
    mov al, [_Enemies+esi+ENEMY.EnemySize]
    invoke _ShipExplode, dword [_Enemies+esi+ENEMY.EnemyXFloat], dword [_Enemies+esi+ENEMY.EnemyYFloat], dword eax, dword 0

    push eax
    ;mov al, 12
    ;call _SoundIsPlaying
    ;cmp eax, 1
    ;jne .NotPlayingYet
    ;invoke _StopSound, dword 12, dword SOUND_KILL
;.NotPlayingYet

    ;pusha
    ;invoke _AddSound, dword [_SNDExplodeEnemy], dword 35536, dword 00h, byte 12
    ;popa
    mov byte [_Enemies+esi+ENEMY.EnemyActive], -1

    cmp byte [_Enemies+esi+ENEMY.EnemySize], 1
    jne .NotBossDead

    cmp byte[_GameTurnout], PLAYER_DEAD
    je .NotBossDead

    mov byte [_GameTurnout], PLAYER_WIN
.NotBossDead:
    pop eax
    jmp .SkipUpdate
.NotDead:


    finit

    fld dword[_Enemies+esi+ENEMY.EnemyXFloat]
    fld dword[_Enemies+esi+ENEMY.EnemyXVelFloat]
    faddp st1                 ;st0 = speed * cos(angle)
    fst dword[_Enemies+esi+ENEMY.EnemyXFloat]
    fistp dword[_Enemies+esi+ENEMY.EnemyXInt]


    fld dword[_Enemies+esi+ENEMY.EnemyYFloat]
    fld dword[_Enemies+esi+ENEMY.EnemyYVelFloat]
    faddp st1                 ;st0 = speed * cos(angle)
    fst dword[_Enemies+esi+ENEMY.EnemyYFloat]
    fistp dword[_Enemies+esi+ENEMY.EnemyYInt]

    finit


    cmp dword[_Enemies+esi+ENEMY.EnemyXInt], MAP_WIDTH
    jl .DontWrapMaxX

    mov dword[_intTemp], MAP_WIDTH
    fld dword[_Enemies+esi+ENEMY.EnemyXFloat]
    fild dword[_intTemp]
    fsubp st1, st0
    fst dword[_Enemies+esi+ENEMY.EnemyXFloat]
    fistp dword[_Enemies+esi+ENEMY.EnemyXInt]

.DontWrapMaxX:

    cmp dword[_Enemies+esi+ENEMY.EnemyXInt], 0
    jg .DontWrapMinX

    mov dword[_intTemp], MAP_WIDTH
    fld dword[_Enemies+esi+ENEMY.EnemyXFloat]
    fild dword[_intTemp]
    faddp st1, st0
    fst dword[_Enemies+esi+ENEMY.EnemyXFloat]
    fistp dword[_Enemies+esi+ENEMY.EnemyXInt]

.DontWrapMinX:


    cmp dword[_Enemies+esi+ENEMY.EnemyYInt], MAP_HEIGHT
    jl .DontWrapMaxY

    mov dword[_intTemp], MAP_HEIGHT
    fld dword[_Enemies+esi+ENEMY.EnemyYFloat]
    fild dword[_intTemp]
    fsubp st1, st0
    fst dword[_Enemies+esi+ENEMY.EnemyYFloat]
    fistp dword[_Enemies+esi+ENEMY.EnemyYInt]

.DontWrapMaxY:

    cmp dword[_Enemies+esi+ENEMY.EnemyYInt], 0
    jg .SkipUpdate

    mov dword[_intTemp], MAP_HEIGHT
    fld dword[_Enemies+esi+ENEMY.EnemyYFloat]
    fild dword[_intTemp]
    faddp st1, st0
    fst dword[_Enemies+esi+ENEMY.EnemyYFloat]
    fistp dword[_Enemies+esi+ENEMY.EnemyYInt]

.SkipUpdate:
    add esi, 40
    dec cl
    jnz near .UpdateLoop

    ;;; When we get here, we have updated all of the first ch enemies, now check to see
    ;;;   if a new wave is in order, or if boss needs to be updated.

    pop eax                         ; Eax gains frame number of next wave
    cmp eax, -1                     ; -1 if boss is out
    je near .BossIsOut
    cmp eax, [_FrameNum]
    ja near .NotNewWave

    ;;; New wave, increment _NextFrameNumber
    inc dword [_NextWaveNumber]
    ;invoke _AddSound, dword [_SNDWeapon1], dword 31576, dword 00h, byte 4
    ;;; Check to see if new frame number describes boss is let out
    mov ebx, [_NextWaveNumber]
    shl ebx, 3                            ; Multiply frame number by eight to get index into _SpawnFrames
    mov eax, [_SpawnFrames + ebx]
    ;;; If eax is -1, then activate boss!
    cmp eax, -1
    je .ActivateBoss
    ;;; Wasn't boss wave, so activate correct number of bad dudes
    mov eax, [_SpawnFrames + ebx + 4]
    mov cl, al
    sub cl, ch                            ; ch gains number of bad guys per wave
.ActivateNewLoop:
    mov byte [_Enemies+esi+ENEMY.EnemyActive], 1         ; Activate enemy!
    add esi, 40
    dec cl
    jnz .ActivateNewLoop
    jmp .NotNewWave

.ActivateBoss:
    mov byte [_Enemies+esi+ENEMY.EnemyActive], 1         ; Activate boss!
    invoke Mix_PlayChannelTimed, dword -1, dword[_SNDEffect_EvilLaugh], dword 0, dword -1
    invoke _ShakeMap, dword 150
    jmp .NotNewWave
.BossIsOut:
    ;;; Could include firing code here for boss
.NotNewWave:
    popa
    ret


;====== RenderEnemies ===================================================
;;; _RenderEnemies
;;;   INPUTS: ScreenOff, Enemy Ships Data
;;;  OUTPUTS: Rendered Enemy
;;;  PURPOSE: Draws the enemies on the screen
_RenderEnemies:

    pusha

    finit

    mov esi, [_NextWaveNumber]
    shl esi, 3                            ; Multiply frame number by eight to get index into _SpawnFrames
    mov ecx, [_SpawnFrames+esi+4]         ; Store enemy number in ecx (will be less than 100, so cl is all that is needed)

    mov esi, 0                            ; esi indexes enemy number
.DrawEnemyLoop:
    cmp byte [_Enemies+esi+ENEMY.EnemyActive], -1
    je near .NotActive

    mov ebx, [_Enemies+esi+ENEMY.EnemyType]               ; ebx gains enemy number
    shl ebx, (4+ENEMYSHIFTSIZE)           ; multiply by 16 (frames per enemy) * bytes per frame

    mov eax, 0
    mov al, [_Enemies+esi+ENEMY.EnemyAngle]
    shr eax, 4                            ; eax gains degree/16 (angle frame to add)
    shl eax, ENEMYSHIFTSIZE               ; ENEMYSHIFTSIZE is number of bytes per frame (ie 16 for 128 by 128 by 4)

    add ebx, eax                          ; ebx now has total offset into enemy file

    xor eax, eax                          ; Will be X
    xor edx, edx                          ; Will be Y

    ;fld dword [_Enemies+esi+4]
    ;fistp dword [_intTemp]
    mov eax, dword [_Enemies+esi+ENEMY.EnemyXInt]
    sub eax, [_intPlayerX]
    add eax, SCREEN_WIDTH/2


    ;fld dword [_Enemies+esi+8]
    ;fistp dword [_intTemp]
    mov edx, dword [_Enemies+esi+ENEMY.EnemyYInt]
    sub edx, [_intPlayerY]
    add edx, SCREEN_HEIGHT/2


    ;;; Check to see if enemy is on screen but wrapped by map width

    cmp eax, (-MAP_WIDTH+SCREEN_WIDTH)
    jge .NotWrappingRight
    add eax, MAP_WIDTH
.NotWrappingRight:
    cmp eax, MAP_WIDTH
    jle .NotWrappingLeft
    sub eax, MAP_WIDTH
.NotWrappingLeft:

    cmp edx, (-MAP_HEIGHT+SCREEN_HEIGHT)
    jge .NotWrappingDown
    add edx, MAP_HEIGHT
.NotWrappingDown:
    cmp edx, MAP_HEIGHT
    jle .NotWrappingUp
    sub edx, MAP_HEIGHT
.NotWrappingUp:

    sub eax, 64             ; Half of height
    sub edx, 64

    cmp byte [_Enemies+esi+ENEMY.EnemySize], 1
    je .DrawBoss

    add ebx, dword[_SmallEnemyImageData]  ; Add memory base offset of loaded file
    invoke _AlphaBlit, dword eax, dword edx, dword ebx, dword 128, dword 128
    jmp .NotActive

.DrawBoss:
    ;;; Need to multiply offset by four
    shl ebx, 2
    add ebx, dword[_BossImageData]  ; Add memory base offset of loaded file

    sub eax, 64             ; Half of height
    sub edx, 64

    invoke _AlphaBlit, dword eax, dword edx, dword ebx, dword 256, dword 256
.NotActive:
    add esi, 40
    dec ecx
    jnz near .DrawEnemyLoop

    ;;; Still need to draw boss if he's up

    popa
    ret




;====== EnemyCollisionDetection ===================================================
;;; _EnemyCollisionDetection
;;;   INPUTS: .EnemyPtr, index to enemy data being analyzed (from _Enemies)
;;;  OUTPUTS: Subtracts from EnemyHealth each collision with player fire
;;;  PURPOSE: Draws sparks for hits, subtracts from enemy hitpoints
_EnemyCollisionDetection:
    .EnemyPtr       EQU   4
    pusha

    mov edi, [ebp+.EnemyPtr]

    cmp byte [_Enemies+edi+ENEMY.EnemySize], 1
    je .BossSizeInEdx
    mov edx, 50
    jmp .OffsetDone
.BossSizeInEdx:
    mov edx, 114
.OffsetDone:

    mov eax, [_Enemies+edi+ENEMY.EnemyXInt]
    sub eax, edx
    mov [_intLeft], eax
    add eax, edx
    add eax, edx
    mov [_intRight], eax

    mov eax, [_Enemies+edi+ENEMY.EnemyYInt]
    sub eax, edx
    mov [_intTop], eax
    add eax, edx
    add eax, edx
    mov [_intBottom], eax

    mov esi, dword[_ParticleDataOff]

    xor eax, eax
    xor ebx, ebx
    xor edx, edx
    xor ecx, ecx
.ParticleLoop:

    cmp byte[esi + eax + PARTICLE.IsActive], 1
    jne near .SkipParticle
    cmp byte[esi + eax + PARTICLE.DetectCollisions], 1
    jne near .SkipParticle

    mov ebx, dword[esi + eax + PARTICLE.intX]
    mov edx, dword[esi + eax + PARTICLE.intY]

    cmp ebx, dword[_intLeft]
    jl .SkipParticle
    cmp ebx, dword[_intRight]
    jg .SkipParticle
    cmp edx, dword[_intTop]
    jl .SkipParticle
    cmp edx, dword[_intBottom]
    jg .SkipParticle

;;; HandleCollision:

    pusha
    mov byte[esi + eax + PARTICLE.IsActive], 0
    mov ebx, dword[esi + eax + PARTICLE.Damage]
    sub dword[_Enemies+edi+ENEMY.EnemyHealth], ebx
    invoke _ShipExplode, dword[esi + eax + PARTICLE.fltX], dword[esi + eax + PARTICLE.fltY], dword 0, dword 1
    popa

    cmp dword[_Enemies+edi+ENEMY.EnemyHealth], 0
    jle .Done

.SkipParticle:

    add eax, PARTICLE_STRUCT_SIZE
    inc ecx
    cmp ecx, MAX_PARTICLES
    jb near .ParticleLoop

.Done:
    popa
    ret

