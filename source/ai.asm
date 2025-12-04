;;;;;;;;;;;;;;;;;;;;;;;;
;;;;David Lytle;;;;;;;;;
;;;;Final project;;;;;;;
;;;;ai.asm;;;;;;;;;;;;;;
;;;copyright mooseinc;;
;;;;;;;;;;;;;;;;;;;;;;;;


%include "defs.inc"
%include "sdl.inc"
%include "player.inc"
%include "ppe.inc"
%include "rand.inc"
%include "mapeng.inc"
%include "sse_mem.inc"
%include "macros.inc"
%include "input.inc"
%include "enemy.inc"

    BITS 32

    GLOBAL _ENEMY_MOVE, _ShipExplode, _DropNuke

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

    EXTERN _SNDEffect_Engines, _SNDEffect_EvilLaugh, _SNDEffect_Hit
    EXTERN _SNDEffect_Weapon, _SNDEffect_ExplosionArray

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

SECTION .bss


    G_Mass                   resd   1
    Delta_X                  resd   1
    Delta_Y                  resd   1
    Delta_X1_Flt             resd   1
    Delta_X1_Int             resd   1
    Delta_Y1_Flt             resd   1
    Delta_Y1_Int             resd   1
    Delta_X2_Flt             resd   1
    Delta_X2_Int             resd   1
    Delta_Y2_Flt             resd   1
    Delta_Y2_Int             resd   1
    _Tan                     resd   1
    _Ftrans                  resd   1
    _RadiusFlt               resd   1
    _RadiusInt               resd   1
    _Acc                     resd   1


SECTION .data

    G                        dd 450.0 ;150.0

    _intTemp                 dd 0
    _intTemp2                dd 0
    _fltTemp                 dd 0
    _fltTemp2                dd 4.2
    _fltTemp3                dd 5.3
    _FudgeFactor             dd 2.6
    FIRERANGE                dd 83896
    FIRERANGE2               dd 634430
    _fltEnemyFiringRate      dd 12.6
                             dd  14.5331
                             dd  16.31
                             dd  18.2
                             dd  7.3112

    _fltPSpeed1              dd  4.2
    _fltPSpeed2              dd  5.33
    _fltPSpeed3              dd  6.112
    _fltPSpeed4              dd  7.5
    _fltPSpeed5              dd  8.82

    _PrintStr                db  10,'AI = %d',10,0

SECTION .text

;;;
;;; _ENEMY_MOVE
;;; Inputs: _fltPlayerMass, _Enemies array
;;;
;;; Constant: G
;;; Outputs:  an updated enemy array with new x, y speed and an angle vector pointing at the player
;;;
;;;   Notes: Angle in units of Bens....256 bens=360 degrees
;;;
_ENEMY_MOVE:

    pusha

    FINIT
    fld dword[_fltPlayerMass]
    fld dword[G]
    fmulp  ST1
    fstp   dword[G_Mass]                                  ; G*m1


    mov ecx, 0
.CheckAll:


    push ecx
    mov eax, ecx
    shl eax, 3
    shl ecx, 5                                    ;ecx gains 32 times enemy number
    add ecx, eax

    add ecx, _Enemies                             ; Add enemies offset to ecx
    mov eax, [ecx]                                ;enemy index
    cmp eax, -1
    je near .Finished                             ; if enemy index is -1, then no more enemies remain

    mov al, byte [ecx+ENEMY.EnemyActive]
    cmp al, -1                                    ; If not active, skip to next enemy
    je near .NoFire

    ;;; Beginning of SOMETHING
    mov   dword [_fltTemp], MAP_WIDTH
    fld   dword [_fltPlayerX]               ; ST0 = P
    fld   dword [ecx+ENEMY.EnemyXFloat]     ; ST0 = E       ST1 = P

    fsubp ST1                   ; ST0 = P - E
    fldz                        ; ST0 = 0        ST1 = Delta_X
    fadd  ST1                   ; ST0 = Delta_X  ST1 = Delta_X
    fst   dword [Delta_X1_Flt]  ; ST0 = Delta_X (Stored)  ST1 = Delta_X
    fistp dword [Delta_X1_Int]  ; Delta_X_Int (Stored)  ST0 = Delta_X
    fld   dword [_fltTemp]      ; ST0 = WIDTH   ST1 = Delta_X

    mov eax, dword [Delta_X1_Int]
    cmp eax, 0
    jge .DontCorrectForNegX

    faddp ST1                   ; ST0 = Delta_X + WIDTH
    jmp .ContinueAfterNegX
.DontCorrectForNegX:
    fsubp ST1                   ; ST0 = Delta_X - WIDTH
.ContinueAfterNegX:
    fst   dword [Delta_X2_Flt]  ; ST0 = Delta_X (Stored)
    fabs                        ;
    fistp dword [Delta_X2_Int]  ; Delta_X_Int (Stored)

; Now for the Y's!

    mov   dword [_fltTemp], MAP_HEIGHT
    fld   dword [_fltPlayerY]                            ;player Y position
    fld   dword [ecx+ENEMY.EnemyYFloat]                  ;enemy Y position

    fsubp ST1                   ; ST0 = P - E
    fldz                        ; ST0 = 0        ST1 = Delta_Y
    fadd  ST1                   ; ST0 = Delta_Y  ST1 = Delta_Y
    fst   dword[Delta_Y1_Flt]   ; ST0 = Delta_Y (Stored)  ST1 = Delta_Y
    fistp dword[Delta_Y1_Int]   ; ST0 = Delta_Y
    fld   dword[_fltTemp]       ; ST0 = HEIGHT   ST1 = Delta_Y

    mov eax, dword[Delta_Y1_Int]
    cmp eax, 0
    jge .DontCorrectForNegY

    faddp ST1                   ; ST0 = Delta_Y + HEIGHT
    jmp .ContinueAfterNegY
.DontCorrectForNegY:
    fsubp ST1                   ; ST0 = Delta_Y - HEIGHT
.ContinueAfterNegY:
    fst   dword[Delta_Y2_Flt]   ; ST0 = Delta_Y (Stored)
    fabs
    fistp dword[Delta_Y2_Int]   ; Delta_Y_Int (Stored)

    ; new code
.new_code_in_testing:


jmp .OppositeDirectionDone


.PlayerHorizontalTest:
    mov eax, dword [_intPlayerX]
    mov ebx, dword [_intPlayerY]
    cmp eax, dword [ecx+ENEMY.EnemyXInt]
    ja .PlayerRightOfEnemy
.PlayerLeftOfEnemy:
    ; |--> P          E -->|
    ; want Delta_X2 = (MAP_WIDTH + P - E)

    mov   dword [Delta_X2_Flt], MAP_WIDTH
    fld   dword [_fltPlayerX]                ; ST0 = PX
    fiadd dword [Delta_X2_Flt]               ; ST0 = WIDTH + PX
    fsub  dword [ecx+ENEMY.EnemyXFloat]      ; ST0 = WIDTH + PX - EX

    fst   dword[Delta_X2_Flt]                ; ST0 = Delta_X2
    fabs
    fistp dword[Delta_X2_Int]

    jmp .PlayerVerticalTest
.PlayerRightOfEnemy:
    ; |<-- E          P <--|
    ; want Delta_X2 = (P - MAP_WIDTH - E)

    mov   dword [Delta_X2_Flt], MAP_WIDTH
    fld   dword [_fltPlayerX]                ; ST0 = PX
    fisub dword [Delta_X2_Flt]               ; ST0 = PX - WIDTH
    fsub  dword [ecx+ENEMY.EnemyXFloat]      ; ST0 = PX - WIDTH - EX

    fst   dword[Delta_X2_Flt]                ; ST0 = Delta_X2
    fabs
    fistp dword[Delta_X2_Int]

.PlayerVerticalTest:

    cmp ebx, dword [ecx+ENEMY.EnemyYInt]
    ja .PlayerBelowEnemy
.PlayerAboveEnemy:
    ; TOP |--> P          E -->| BOTTOM
    ; want Delta_Y2 = (MAP_HEIGHT + P - E)

    mov   dword [Delta_Y2_Flt], MAP_HEIGHT
    fld   dword [_fltPlayerY]                ; ST0 = PY
    fild  dword [Delta_Y2_Flt]               ; ST0 = HEIGHT  ST1 = PY
    fld   dword [ecx+ENEMY.EnemyYFloat]      ; ST0 = EY  ST1 = HEIGHT  ST2 = PY
    fsubp ST1                                ; ST0 = HEIGHT - EY       ST1 = PY
    faddp ST1                                ; ST0 = HEIGHT - EY + PY

    fst   dword[Delta_Y2_Flt]
    fabs
    fistp dword[Delta_Y2_Int]

    jmp .OppositeDirectionDone
.PlayerBelowEnemy:
    ; TOP |<-- E          P <--| BOTTOM
    ; want Delta_Y2 = (P - MAP_HEIGHT - E)

    mov   dword [Delta_Y2_Flt], MAP_HEIGHT
    fld   dword [_fltPlayerY]                ; ST0 = PY
    fld   dword [ecx+ENEMY.EnemyYFloat]      ; ST0 = EY      ST1 = PY
    fild  dword [Delta_Y2_Flt]               ; ST0 = HEIGHT  ST1 = EY  ST2 = PY
    faddp ST1                                ; ST0 = HEIGHT + EY       ST1 = PY
    fsubp ST1                                ; ST0 = PY - (HEIGHT + EY)

    fst   dword[Delta_Y2_Flt]
    fabs
    fistp dword[Delta_Y2_Int]

.OppositeDirectionDone:

    mov eax, dword[Delta_X1_Int]
    mov ebx, dword[Delta_X2_Int]
    mov edx, dword[Delta_X1_Flt]
    cmp eax, ebx
    jb .DontSwapXtreme
.SwapXtreme:

    mov edx, dword[Delta_X2_Flt]
.DontSwapXtreme:
    mov dword[Delta_X], edx

    mov eax, dword[Delta_Y1_Int]
    mov ebx, dword[Delta_Y2_Int]
    mov edx, dword[Delta_Y1_Flt]
    cmp eax, ebx
    jb .DontSwapYtreme
.SwapYtreme:

    mov edx, dword[Delta_Y2_Flt]
.DontSwapYtreme:
    mov dword[Delta_Y], edx

    fld   dword [Delta_X]
    fldz
    fadd  ST1
    fmulp ST1                     ; ST0 = Delta_X ^2

    fld   dword [Delta_Y]
    fldz
    fadd  ST1
    fmulp ST1                    ; ST0 = Delta_Y ^2   ST1 = Delta_X ^2
    faddp ST1                            ; ST0 = R2^2
    fst   dword [_RadiusFlt]            ; STORED R2^2
    fistp dword [_RadiusInt]            ; popped

    cmp dword[Delta_X], 0
    jne .GoodTan

    ; this compare might be useless,
    ; fpatan takes atan of y/x, who cares if y=0 ?
    cmp dword[Delta_Y], 0
    je near .Done



    ; recent change... out
;
;      test dword[Delta_Y], 80000000h
;      jnz .NEG
;
;      mov byte [ecx+ENEMY.EnemyAngle], -64
;
;      jmp near .Done
;.NEG
;     mov byte [ecx+ENEMY.EnemyAngle], 64
;
;     jmp near .Done


.GoodTan:
;     FINIT
    fld dword[_fltRadToDeg256]
    fld dword[Delta_Y]
    fld dword[Delta_X]
    fpatan
    fmulp ST1
    fistp dword[_Ftrans]
    mov eax, dword[_Ftrans]
    ;neg eax

    ;mov byte[ecx+ENEMY.EnemyAngle], al

    ; NEW FEATURE
    ; limit rate at which enemy ships can turn.
    ; if you fly through the center of them,
    ; it will take them time to turn toward you.


    push ax
    push bx

    mov bx, 0
    and ax, 000FFh  ; angle pointing to us is in al
    mov bl, byte[ecx+ENEMY.EnemyAngle]

;     mov dl, byte MAX_ENEMY_TURNING_SPEED

    push cx
    mov cx, bx
    sub cx, ax

    cmp cl, byte MAX_ENEMY_TURNING_SPEED
    jg .DecreaseTurningAngle
    neg cl
    cmp cl, byte MAX_ENEMY_TURNING_SPEED
    jl .DontChangeTurningAngle

    add bl, byte MAX_ENEMY_TURNING_SPEED
    add bl, byte MAX_ENEMY_TURNING_SPEED

.DecreaseTurningAngle:

    sub bl, byte MAX_ENEMY_TURNING_SPEED

.DontChangeTurningAngle:

;.igiveup
    pop cx

    mov byte[ecx+ENEMY.EnemyAngle], bl

    pop bx
    pop ax

    ; END FEATURE


    ; ASSUME FPU IS GOOD (CAUSE I WROTE ALL CODE ABOVE!)


    fld dword[_RadiusFlt]           ; ST0 = R**2
    fldz                         ; ST0 = 0     ST1 = R**2
    fadd ST1                     ; ST0 = R**2  ST1 = R**2
    fld dword[G_Mass]            ; ST0 = G * player_mass  ST1 = R**2  ST2 = R**2
    fdivrp ST1                   ; ST0 = G * player_mass / R**2  ST1 = R**2
    fstp dword[_Acc]             ; STORE acceleration  ST0 = R**2

    fsqrt                        ; ST0 = R
    fldz                         ; ST0 = 0  ST1 = R
    fadd ST1                     ; ST0 = R  ST1 = R
;     fst dword[_Radius]           ; ST0 = R (STORED)
    fist dword [_RadiusInt]      ; STORE Radius  ST0 = R

    fld dword[Delta_X]           ; ST0 = DelX  ST1 = R  ST2 = R
    fdivrp ST1                   ; ST0 = DelX/R  ST1 = R
    fstp dword[Delta_X]          ; ST0 = R      ; STORED normalized X

    fld dword[Delta_Y]           ; ST0 = DelY  ST1 = R
    fdivrp ST1                   ; ST0 = DelY/R
    fst dword[Delta_Y]           ; ST0 = DelY/R STORED normalized Y

    fld dword[_Acc]              ; ST0 = Acc  ST1 = DelY/R
    fmulp ST1                    ; ST0 = Acc * DelY/R
    fstp dword[Delta_Y]          ; STORE Y acc

    fld dword[_Acc]              ; ST0 = Acc
    fld dword[Delta_X]           ; ST0 = DelX/R  ST1 = Acc
    fmulp st1                    ; ST0 = Acc * DelX/R
    fstp dword[Delta_X]          ; STORE X acc

    mov eax,[_RadiusInt]            ; get radius as Integer
    cmp eax, INNER_RADIUS_CUTOFF
    jl  near .Done

    fld dword[Delta_X]                        ; ST0 = X acc
    fld dword [ecx+ENEMY.EnemyXVelFloat]      ; ST0 = X vel   ST1 = X acc
    faddp ST1                                 ; ST0 = X acc + X vel
    fstp dword [Delta_X]                      ; STORE

    fld dword[Delta_Y]                        ; ST0 = Y acc
    fld dword [ecx+ENEMY.EnemyYVelFloat]      ; ST1 = Y vel   ST1 = Y acc
    faddp ST1                                 ; ST0 = Y acc + Y vel
    fstp dword [Delta_Y]                      ; STORE


    fld dword[Delta_X]
    fistp dword[_Ftrans]
    mov ebx, dword[_Ftrans]
    cmp ebx, MAX_ENEMY_SPEED
    jg .NoX

;     cmp eax, OUTER_RADIUS_CUTOFF
;     jg .Zero

    mov edx, dword[Delta_X]
    mov dword[ecx+ENEMY.EnemyXVelFloat], edx

.NoX:

    fld dword[Delta_Y]
    fistp dword[_Ftrans]
    mov ebx, dword[_Ftrans]
    cmp ebx, MAX_ENEMY_SPEED
    jg .Done

    mov edx, dword[Delta_Y]


    mov dword[ecx+ENEMY.EnemyYVelFloat], edx
    jmp .Done

.Zero:
    mov esi, [_FudgeFactor]
    mov dword[ecx+ENEMY.EnemyYVelFloat], esi
    mov dword[ecx+ENEMY.EnemyXVelFloat], esi

.Done:


    cmp eax, 400 ;300  ;Boss firing radius
    jg near .NoFire
    mov bl, byte[ecx+ENEMY.EnemySize]
    cmp bl, 1
    je near .BossWeapons


    ; FIRE ABOUT 1/2 THE TIME
    call _Rand
    and eax, 000000FFh
    cmp eax, 7Fh;[FIRERANGE]
    jb .NoFire

    xor eax, eax
    mov al,byte [ecx+ENEMY.EnemyAngle]
    shl eax, 2
    xor ebx, ebx


    ; NEED TO ADD SPEED TO PARTICLE !!!!
    fld   dword[ecx+ENEMY.EnemyXVelFloat]
    fld   dword[_COS_LOOK + eax]
    fmul  dword[_fltEnemyFiringRate]
    faddp ST1
    fstp  dword[_fltTemp]

    fld   dword[ecx+ENEMY.EnemyYVelFloat]
    fld   dword[_SIN_LOOK + eax]
    fmul  dword[_fltEnemyFiringRate]
    faddp ST1
    fstp  dword[_fltTemp2]

    mov edi,dword[ecx+ENEMY.EnemyType]
    add edi, 4
;     invoke _AddParticle, dword 2, dword LARGE_PARTICLE, dword edi, dword[ecx+ENEMY.EnemyXFloat], dword[ecx+ENEMY.EnemyYFloat], dword eax, dword[_fltEnemyFiringRate], dword 80, dword 10
    invoke _AddParticleByVector, dword 2, dword LARGE_PARTICLE, dword edi, dword[ecx+ENEMY.EnemyXFloat], dword[ecx+ENEMY.EnemyYFloat], dword[_fltTemp], dword[_fltTemp2], dword 80, dword 10


.NoFire:
    pop ecx
    inc ecx
    cmp ecx, 100
    jl near .CheckAll

    popa

    ret

.Finished:
    pop ecx

    popa

    ret




.BossWeapons:
    cmp dword[ecx+ENEMY.EnemyType], 3
    je near .SHIMDOG

    call _Rand
    xor edx, edx
    mov ebx, 100
    div ebx
    cmp edx,60
    jae near .NoFire

    xor ebx, ebx
    mov bl, byte[ecx+ENEMY.EnemyAngle]
    ;DetectCollisions, ImgSizeType, ImgIndex, X, Y, Angle, Speed, MaxLife, Damage
    push ecx
    invoke _AddParticle, dword 3, dword LARGE_PARTICLE, dword 5, dword[ecx+ENEMY.EnemyXFloat], dword[ecx+ENEMY.EnemyYFloat], dword ebx, dword[_fltEnemyFiringRate + 4], dword 30, dword 4
    pop ecx

    cmp dword[ecx+ENEMY.EnemyType], 1
    jb near .NoFire

    push ecx
    xor ebx, ebx
    mov bl, byte[ecx+ENEMY.EnemyAngle]
    add bl, 10
    invoke _AddParticle, dword 3, dword LARGE_PARTICLE, dword 7, dword[ecx+ENEMY.EnemyXFloat], dword[ecx+ENEMY.EnemyYFloat], dword ebx, dword[_fltEnemyFiringRate + 4], dword 30, dword 6
    pop ecx

    push ecx
    xor ebx, ebx
    mov bl, byte[ecx+ENEMY.EnemyAngle]
    sub bl, 10
    invoke _AddParticle, dword 3, dword LARGE_PARTICLE, dword 7, dword[ecx+ENEMY.EnemyXFloat], dword[ecx+ENEMY.EnemyYFloat], dword ebx, dword[_fltEnemyFiringRate + 4], dword 30, dword 6
    pop ecx


    push ecx
    xor ebx, ebx
    mov bl, byte[ecx+ENEMY.EnemyAngle]
    invoke _AddParticle, dword 3, dword SMALL_PARTICLE, dword 8, dword[ecx+ENEMY.EnemyXFloat], dword[ecx+ENEMY.EnemyYFloat], dword ebx, dword[_fltEnemyFiringRate + 12], dword 30, dword 10
    pop ecx

    cmp dword[ecx+ENEMY.EnemyType], 2
    jb near .NoFire


    push ecx
    call _Rand
    xor edx, edx
    mov ebx, 1000
    div ebx
    pop ecx
    cmp edx, 40
    ja .NoShockWeapon

    xor edx, edx
.ShockLoop:

    pusha
    invoke _AddParticle, dword 3, dword SMALL_PARTICLE, dword 8, dword[ecx+ENEMY.EnemyXFloat], dword[ecx+ENEMY.EnemyYFloat], dword edx, dword[_fltEnemyFiringRate + 16], dword 100, dword 1
    popa

    inc edx
    cmp edx, 256
    jb .ShockLoop


.NoShockWeapon:
    jmp .NoFire


.SHIMDOG: ;a special boss....he fires pot bellied piggies


    call _Rand
    xor edx, edx
    mov ebx, 100
    div ebx
    cmp edx,20
    jae near .NoFire


    push ecx
    xor ebx, ebx
    mov bl, byte[ecx+ENEMY.EnemyAngle]
    ;DetectCollisions, ImgSizeType, ImgIndex, X, Y, Angle, Speed, MaxLife, Damage
    invoke _AddParticle, dword 3, dword LARGE_PARTICLE, dword 9, dword[ecx+ENEMY.EnemyXFloat], dword[ecx+ENEMY.EnemyYFloat], dword ebx, dword[_fltEnemyFiringRate], dword 30, dword 10
    pop ecx

    push ecx
    xor ebx, ebx
    mov bl, byte[ecx+ENEMY.EnemyAngle]
    add bl, 64
    invoke _AddParticle, dword 3, dword LARGE_PARTICLE, dword 9, dword[ecx+ENEMY.EnemyXFloat], dword[ecx+ENEMY.EnemyYFloat], dword ebx, dword[_fltEnemyFiringRate], dword 30, dword 10
    pop ecx

    push ecx
    xor ebx, ebx
    mov bl, byte[ecx+ENEMY.EnemyAngle]
    add bl, 128
    invoke _AddParticle, dword 3, dword LARGE_PARTICLE, dword 9, dword[ecx+ENEMY.EnemyXFloat], dword[ecx+ENEMY.EnemyYFloat], dword ebx, dword[_fltEnemyFiringRate], dword 30, dword 10
    pop ecx

    push ecx
    xor ebx, ebx
    mov bl, byte[ecx+ENEMY.EnemyAngle]
    add bl, -64
    invoke _AddParticle, dword 3, dword LARGE_PARTICLE, dword 9, dword[ecx+ENEMY.EnemyXFloat], dword[ecx+ENEMY.EnemyYFloat], dword ebx, dword[_fltEnemyFiringRate], dword 30, dword 10
    pop ecx


    push ecx
    call _Rand
    xor edx, edx
    mov ebx, 1000
    div ebx
    pop ecx
    cmp edx, 60
    ja .NoShimDogShockWeapon

    xor edx, edx
.ShimDogShockLoop:

    pusha
    invoke _AddParticle, dword 3, dword LARGE_PARTICLE, dword 9, dword[ecx+ENEMY.EnemyXFloat], dword[ecx+ENEMY.EnemyYFloat], dword edx, dword[_fltEnemyFiringRate + 16], dword 100, dword 1
    popa

    inc edx
    cmp edx, 256
    jb .ShimDogShockLoop

.NoShimDogShockWeapon:
    jmp .NoFire

    ret



 ;==============================================================================================
;;; _DropNuke
;;; Inputs: fltx flty
;;; Outputs:  boom
;;;
_DropNuke:
.fltX             EQU    4
.fltY           EQU    8

    xor edx, edx
.ShockLoop:

    pusha
    invoke _AddParticle, dword 1, dword LARGE_PARTICLE, dword 5, dword[ebp+.fltX], dword[ebp+.fltY], dword edx, dword[_fltEnemyFiringRate + 16], dword 100, dword 20
    popa

    pusha
    add edx, 1
    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 15, dword[ebp+.fltX], dword[ebp+.fltY], dword edx, dword[_fltEnemyFiringRate + 8], dword 100, dword 20
    popa

    pusha
    add edx, 2
    invoke _AddParticle, dword 1, dword LARGE_PARTICLE, dword 5, dword[ebp+.fltX], dword[ebp+.fltY], dword edx, dword[_fltEnemyFiringRate + 4], dword 100, dword 20
    popa

    pusha
    add edx, 3
    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 15, dword[ebp+.fltX], dword[ebp+.fltY], dword edx, dword[_fltEnemyFiringRate], dword 100, dword 20
    popa


    add edx, 2
    cmp edx, 256
    jb near .ShockLoop

    ret


 ;==============================================================================================
;;; _ShipExplode
;;; Inputs: fltx flty .size .hit
;;; Outputs:  a burst of particles or smoke depending on which arguments it is called with
;;;
_ShipExplode:
.fltX             EQU    4
.fltY           EQU    8
.size           EQU     12        ;one if the ship was player or boss
.hit            EQU     16

    pusha             ;preserving all registers due to sound issue

    xor ecx, ecx

    mov eax, dword[ebp+.hit]
    cmp eax, 1
    je near .Hit

    mov eax, dword[ebp+.size]
    cmp eax, 1
    je near .BigExplosion


.SmallExplosion:

    push ecx

    call _Rand
    xor edx, edx
    mov ebx, 256
    div ebx
    xor ebx, ebx
    mov bl, dl

    invoke _AddParticle, dword 0, dword LARGE_PARTICLE, dword 3, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltPSpeed1], dword 10, dword 0

    call _Rand
    xor edx, edx
    mov ebx, 256
    div ebx
    xor ebx, ebx
    mov bl, dl

    invoke _AddParticle, dword 0, dword LARGE_PARTICLE, dword 7, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltPSpeed2], dword 20, dword 0

    call _Rand
    xor edx, edx
    mov ebx, 256
    div ebx
    xor ebx, ebx
    mov bl, dl

    invoke _AddParticle, dword 0, dword LARGE_PARTICLE, dword 8, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltPSpeed3], dword 30, dword 0

    pop ecx
    inc ecx

    cmp ecx, 20
    jb near .SmallExplosion

    pusha
    call _Rand
    xor edx, edx
    mov ebx, 5
    div ebx
    shl edx, 2
    invoke Mix_PlayChannelTimed, dword -1, dword[_SNDEffect_ExplosionArray + edx], dword 0, dword -1
    popa

    jmp .Done

.BigExplosion:

    push ecx

    call _Rand
    xor edx, edx
    mov ebx, 256
    div ebx
    xor ebx, ebx
    mov bl, dl

    ;DetectCollisions, ImgSizeType, ImgIndex, X, Y, Angle, Speed, MaxLife, Damage
    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 4, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltPSpeed1], dword 200, dword 100

    call _Rand
    xor edx, edx
    mov ebx, 256
    div ebx
    xor ebx, ebx
    mov bl, dl

    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 4, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltPSpeed3], dword 200, dword 100

    call _Rand
    xor edx, edx
    mov ebx, 256
    div ebx
    xor ebx, ebx
    mov bl, dl

    invoke _AddParticle, dword 1, dword LARGE_PARTICLE, dword 5, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltPSpeed2], dword 210, dword 100

    call _Rand
    xor edx, edx
    mov ebx, 256
    div ebx
    xor ebx, ebx
    mov bl, dl

    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 5, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltPSpeed1], dword 220, dword 100

    call _Rand
    xor edx, edx
    mov ebx, 256
    div ebx
    xor ebx, ebx
    mov bl, dl

    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 5, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltPSpeed3], dword 220, dword 100

    call _Rand
    xor edx, edx
    mov ebx, 256
    div ebx
    xor ebx, ebx
    mov bl, dl

    invoke _AddParticle, dword 1, dword SMALL_PARTICLE, dword 6, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltPSpeed4], dword 230, dword 100

    pop ecx
    inc ecx

    cmp ecx, 100
    jb near .BigExplosion

    pusha
    call _Rand
    xor edx, edx
    mov ebx, 5
    div ebx
    shl edx, 2
    invoke Mix_PlayChannelTimed, dword -1, dword[_SNDEffect_ExplosionArray + edx], dword 0, dword -1
    popa

    pusha
    call _Rand
    xor edx, edx
    mov ebx, 5
    div ebx
    shl edx, 2
    invoke Mix_PlayChannelTimed, dword -1, dword[_SNDEffect_ExplosionArray + edx], dword 0, dword -1
    popa

    pusha
    call _Rand
    xor edx, edx
    mov ebx, 5
    div ebx
    shl edx, 2
    invoke Mix_PlayChannelTimed, dword -1, dword[_SNDEffect_ExplosionArray + edx], dword 0, dword -1
    popa

    pusha
    call _Rand
    xor edx, edx
    mov ebx, 5
    div ebx
    shl edx, 2
    invoke Mix_PlayChannelTimed, dword -1, dword[_SNDEffect_ExplosionArray + edx], dword 0, dword -1
    popa

    jmp .Done

.Hit:

    call _Rand
    xor edx, edx
    mov ebx, 256
    div ebx
    xor ebx, ebx
    add bl, dl

    push ebx
    invoke _AddParticle, dword 0, dword SMALL_PARTICLE, dword 14, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltTemp3], dword 15, dword 0
    pop ebx

    add bl, 85

    push ebx
    invoke _AddParticle, dword 0, dword LARGE_PARTICLE, dword 2, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltTemp3], dword 15, dword 0
    pop ebx

    add bl, 85

    push ebx
    invoke _AddParticle, dword 0, dword LARGE_PARTICLE, dword 3, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltTemp3], dword 15, dword 0
    pop ebx


    cmp dword[ebp + .size], 10
    jne near .Done

    add bl, 23

    push ebx
    invoke _AddParticle, dword 0, dword SMALL_PARTICLE, dword 14, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltTemp3], dword 15, dword 0
    pop ebx

    add bl, 68

    push ebx
    invoke _AddParticle, dword 0, dword SMALL_PARTICLE, dword 14, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltTemp3], dword 15, dword 0
    pop ebx

    add bl, 92

    push ebx
    invoke _AddParticle, dword 0, dword SMALL_PARTICLE, dword 14, dword[ebp+.fltX], dword[ebp+.fltY], dword ebx, dword[_fltTemp3], dword 15, dword 0
    pop ebx


.Done:

    popa

    ret

