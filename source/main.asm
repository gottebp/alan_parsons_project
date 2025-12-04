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
%include "sdl.inc"
%include "input.inc"
%include "menu.inc"
%include "mapeng.inc"
%include "player.inc"
%include "ppe.inc"
%include "enemy.inc"
%include "ai.inc"
%include "sse_mem.inc"
%include "rand.inc"

    GLOBAL main

    GLOBAL _ScreenTemp, _FadeFromBlack, _FadeToBlack, _FadeFromWhite, _FadeToWhite
    GLOBAL _InitializeGameData, _ProcessInput, _WaitForRetrace, _LoadLevel

    GLOBAL _GraphicsMode
    GLOBAL _ScreenOff
    GLOBAL _SIN_LOOK, _COS_LOOK
    GLOBAL _fltDegToRad360, _fltRadToDeg360, _fltDegToRad256, _fltRadToDeg256
    GLOBAL _MapMidX, _MapMidY
    GLOBAL _MenuBackgroundFile
    GLOBAL _GameState, _ActivateMenu
    GLOBAL _GameIsNetworked, _GameIsClient
    GLOBAL _SmallParticleFile, _LargeParticleFile, _GameTurnout, _GameLevel

    GLOBAL _PlayerShipFileN
    GLOBAL _PlayerShipFileL
    GLOBAL _PlayerShipFileR

    GLOBAL _SNDEffect_Engines, _SNDEffect_EvilLaugh, _SNDEffect_Hit
    GLOBAL _SNDEffect_Weapon, _SNDEffect_ExplosionArray

    GLOBAL _SNDEngines_Counter, _SNDWeapon_Counter

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

_GraphicsMode                   resw    1

_TitleScreenOff                 resd    1

_ScreenTemp                     resd      SCREEN_WIDTH * SCREEN_HEIGHT

_VictoryScreenOff               resd    1
_DefeatScreenOff                resd    1

_SIN_LOOK                       resd    256
_COS_LOOK                       resd    256


_SNDTrack                       resd    1

_SNDEffect_Engines              resd    1
_SNDEffect_EvilLaugh            resd    1
_SNDEffect_Hit                  resd    1
_SNDEffect_Weapon               resd    1
_SNDEffect_ExplosionArray       resd    5

_NukeImg                        resd    32 * 32

_StoryClipsOff                  resd    1

SECTION .data

;====== SECTION 5: Declare variables for main procedure ===================
_Map_FileShire                       db './data/shire.bmp',0
_Map_FileMordor                      db './data/mordor.bmp',0
_Map_FileMidkemia                    db './data/midkemia.bmp',0
_Map_FileArchipelago                 db './data/archipelago.bmp',0
_Map_FileDune                        db './data/dune.bmp',0
_Map_FileOceania                     db './data/oceania.bmp',0

_PlayerShipFileN                     db './data/player_norm.raw',0
_PlayerShipFileL                     db './data/player_left.raw',0
_PlayerShipFileR                     db './data/player_right.raw',0

_StoryClipsFile                      db './data/story_clips.bmp',0
_TitleScreenFile                     db './data/title_screen.bmp',0
_MenuBackgroundFile                  db './data/main_menu.bmp',0
_VictoryFile                         db './data/victory.bmp',0
_DefeatFile                          db './data/defeat.bmp',0

_SNDTrack1File                       db './sound/sound_track1.ogg',0
_SNDTrack2File                       db './sound/sound_track2.ogg',0
_SNDTrack3File                       db './sound/sound_track3.ogg',0
_SNDTrack4File                       db './sound/sound_track4.ogg',0
_SNDTrack5File                       db './sound/sound_track5.ogg',0
_SNDTrack6File                       db './sound/sound_track6.ogg',0
_SNDTrackMenuFile                    db './sound/menu_theme.ogg',0
_SNDTrackEndingFile                  db './sound/ending_theme.ogg',0

_SNDEffectEnginesFile                db './sound/engines.wav',0
_SNDEffectEvilLaughFile              db './sound/evil_laugh.wav',0
_SNDEffectHitFile                    db './sound/hit.wav',0
_SNDEffectWeaponFile                 db './sound/weapon.wav',0
_SNDEffectExplosion1File             db './sound/explosion1.wav',0
_SNDEffectExplosion2File             db './sound/explosion2.wav',0
_SNDEffectExplosion3File             db './sound/explosion3.wav',0
_SNDEffectExplosion4File             db './sound/explosion4.wav',0
_SNDEffectExplosion5File             db './sound/explosion5.wav',0

_MixInitFailed                       db     10,'Mix_Init: %s',10,0

_SNDEngines_Counter                  dd      0
_SNDWeapon_Counter                   dd      0

_SmallParticleFile                   db './data/small_flares.bmp',0
_LargeParticleFile                   db './data/large_flares.bmp',0

_fltDegToRad360                      dd 0.01745329252
_fltRadToDeg360                      dd 57.2957795131

_fltDegToRad256                      dd 0.024543692606
_fltRadToDeg256                      dd 40.7436654315

_MapMidX                             dd (MAP_WIDTH / 2) - (SCREEN_WIDTH / 2)
_MapMidY                             dd (MAP_HEIGHT / 2) - (SCREEN_HEIGHT / 2)

_ActivateMenu                        dd 0
_GameState                           dd 0
_GameTurnout                         db PLAYER_NORMAL

_GameLevel                           dd 0
_ActiveLevel                         dd 0
_IsEndingRunning                     dd 0

_GameIsNetworked                     dd 0
_GameIsClient                        dd 0

_RoundingFactor                      dd 00800080h, 00800080h

_NukeWaitCounter                     dd 0
_NukesRemaining                      dd 0
_NumStartingNukes                    dd NUM_NUKES

_NukeImgFile                         db './data/nuke.bmp',0

_EndingDelayTimer                    dd 0

_CurrentTickCount                    dd 0
_PrevTickCount                       dd 0

;====== Temporary Variables Used For Anything Necessary ===================
_intTemp                             dd 0
_fltTemp                             dd 0

_intTop                              dd 0
_intBottom                           dd 0
_intLeft                             dd 0
_intRight                            dd 0

_fltMidX                             dd 1280.0
_fltMidY                             dd 960.0

_fltParticleSpeed                    dd 6.34

_ProjectName                         db 'The Alan Parsons Project',0
_UsageMessage                        db    'Usage: app --fullscreen or app',10,0
_FullScreenCommand                   db    '--fullscreen',0

_LotsOfNukesCommand                  db      '--captainplanet',0

_SaveGameFile                        db 'level.dat',0
_file_ptr                            dd 0
_wb                                  db 'wb',0
_rb                                  db 'rb',0

_NumKeysPressed                      dd      0

_PrintStr                            db        10,'Main = %d',10,0

SECTION .text

;====== main ===================================================
;### main
;### INPUTS: Does mainly stuff
;### OUTPUTS:   Who Cares
;==============================================================
main:
    push ebp
    mov ebp, esp
    add ebp, 4

    push ebx

;-----------Configure Grapics-------------------------------------------------------------------
    cmp dword[ebp + 4], 3   ;2
    jbe .ValidArgc

    push dword _UsageMessage
    call printf
    add esp, 4
    jmp .Done

.ValidArgc:

    cmp dword[ebp + 4], 1
    jbe near .NoCommands

    push dword _LotsOfNukesCommand
    mov eax, dword[ebp + 8]
    push dword [eax + 4]
    call strcmp
    add esp, 8
    cmp eax, 0
    jne .DontAdjustNukes1
    mov dword[_NumStartingNukes], 18

    cmp dword[ebp + 4], 2
    je near .NoCommands

.DontAdjustNukes1:

    cmp dword[ebp + 4], 3
    jne .DontAdjustNukes2
    push dword _LotsOfNukesCommand
    mov eax, dword[ebp + 8]
    push dword [eax + 8]
    call strcmp
    add esp, 8
    cmp eax, 0
    jne .DontAdjustNukes2
    mov dword[_NumStartingNukes], 18
.DontAdjustNukes2:


    push dword _FullScreenCommand
    mov eax, dword[ebp + 8]
    push dword [eax + 4]
    call strcmp
    add esp, 8
    cmp eax, 0
    je .ValidCommand

    cmp dword[_NumStartingNukes], 18
    jne .DontReadSecondArg

    cmp dword[ebp + 4], 3
    jne .DontReadSecondArg
    push dword _FullScreenCommand
    mov eax, dword[ebp + 8]
    push dword [eax + 8]
    call strcmp
    add esp, 8
    cmp eax, 0
    je .ValidCommand

.DontReadSecondArg:

    push dword _UsageMessage
    call printf
    add esp, 4
    jmp .Done

.ValidCommand:
    push dword 1
    jmp .DoGraphicsSetup
   
.NoCommands:
   
.NotFullScreen:

    push dword 0

.DoGraphicsSetup:

    call _InitGraphics
    add esp, 4
;------------------------------------------------------------------------------

    call _InitMapEngine
    call _InitializeGameData

    call _DisplayTitleScreen

    call _RunGame
    call _DestroyGameData
    call _DestroyMapEngine

    call _DestroyGraphics

.Done:
    pop ebx
    pop ebp

ret

;====== RunGame ===================================================
;### _RunGame
;### INPUTS: runs the game
;### OUTPUTS:   Who Cares
;==============================================================
_RunGame:
    push    edi
    xor ebx, ebx

.GameLoop:
    
    call _UpdateInput

    cmp byte[_KEYBOARD + SDLK_ESCAPE], 1
    jne .NotEsc
    mov dword[_ActivateMenu], 1

.NotEsc:

    cmp dword[_ActivateMenu], 1
    jne near .DontRunMenu
    mov dword[_ActivateMenu], 0

    invoke _RunMenu, dword[_GameState]
;        mov dword[_MenuResult], 0

    cmp dword[_MenuResult], 2
    jb .NotMap
    cmp dword[_MenuResult], 7
    ja .NotMap

    invoke _LoadLevel, dword[_MenuResult]

.NotMap:

.DontRunMenu:

    cmp dword[_MenuResult], 0
    je near .QuitGame

    cmp dword[_NukeWaitCounter], 0
    je .NukeReady
    dec dword[_NukeWaitCounter]
.NukeReady:
    
    call _ProcessInput
    call _UpdatePlayer
    call _UpdateEnemies
    invoke _UpdateParticles, dword 0
    call _DetectPlayerCollisions
    call _RenderMap
    call _RenderPlayer
    call _RenderEnemies
    invoke _RenderParticles, dword 0
    call _DrawPlayerHealth
    call _RenderMiniMap

    cmp dword[_NukesRemaining], 0
    je .SkipDrawingNukeIcons
    pusha
    xor eax, eax
    mov ebx, 46;34
.DrawNukesLoop:
    mov ecx, SCREEN_WIDTH
    sub ecx, ebx
    pusha
    invoke _AlphaBlit, dword ecx, dword 18, dword _NukeImg, dword 32, dword 32
    popa
    add ebx, 34
    inc eax
    cmp eax, dword[_NukesRemaining]
    jb .DrawNukesLoop
    popa
.SkipDrawingNukeIcons:

.CheckCompletionStats:
    cmp byte[_GameTurnout], PLAYER_WIN
    jne near .NotVictory

    mov eax, dword[_GameLevel]
    cmp dword[_ActiveLevel], eax
    jb near .DontAdvanceLevels

    inc dword[_GameLevel]
    inc dword[_intPlayerWeaponsLevel]

    invoke fopen, dword _SaveGameFile, dword _wb
    mov dword[_file_ptr], eax
    invoke fwrite, dword _GameLevel, dword 4, dword 1, dword[_file_ptr]
    invoke fwrite, dword _intPlayerWeaponsLevel, dword 4, dword 1, dword[_file_ptr]
    invoke fclose, dword[_file_ptr]

.DontAdvanceLevels:

    mov dword[_intPlayerHealth], MAXPLAYERHEALTH

    cmp dword[_ActiveLevel], 5
    jae .RunEnding

    invoke _AlphaBlit, dword SCREEN_WIDTH/2 - 460/2, dword SCREEN_HEIGHT/2 - 345/2, dword[_VictoryScreenOff], dword 460, dword 345
    jmp .NotVictory

.RunEnding:
    
    dec dword[_EndingDelayTimer]
    cmp dword[_EndingDelayTimer], 0
    jg .NotVictory

    call _RunEnding

.NotVictory:

    cmp byte[_GameTurnout], PLAYER_DEAD
    jne .NotDead

    invoke _AlphaBlit, dword SCREEN_WIDTH/2 - 460/2, dword SCREEN_HEIGHT/2 - 345/2, dword[_DefeatScreenOff], dword 460, dword 345
.NotDead:

;        invoke SDL_LockSurface, dword[_Screen]
;        mov eax, dword[_Screen]
;        invoke _sseMemcpy32, dword[eax + 20], dword[_ScreenOff], dword SCREEN_WIDTH * SCREEN_HEIGHT
;        invoke SDL_UnlockSurface, dword[_Screen]

    invoke SDL_UpdateRect, dword[_Screen], dword 0, dword 0, dword 0, dword 0

.LimitFrameRate:
    call SDL_GetTicks
    mov [_CurrentTickCount], eax
    sub eax, [_PrevTickCount]
    cmp eax, 1000/60
    jl .LimitFrameRate
    mov eax, [_CurrentTickCount]
    mov [_PrevTickCount], eax
    
    jmp    .GameLoop

.QuitGame:

    pop edi
    ret


;====== InitializeGameData ===================================================
;;; _InitializeGameData
;;;   INPUTS: Global data from everything
;;;  OUTPUTS: Initialized data
;;;  PURPOSE: Initializes global data for use in the gaming loop
_InitializeGameData:
    pusha

    mov eax, 0AE33h
    call _SeedRand
    call _Rand

    mov dword[_ActivateMenu], 1
    mov dword[_GameState], 0

    invoke fopen, dword _SaveGameFile, dword _rb
    cmp eax, 0
    je .FileOpenFailed
    mov dword[_file_ptr], eax
    invoke fread, dword _GameLevel, dword 4, dword 1, dword[_file_ptr]
    invoke fread, dword _intPlayerWeaponsLevel, dword 4, dword 1, dword[_file_ptr]
    invoke fclose, dword[_file_ptr]
.FileOpenFailed:

    mov dword[_GameIsNetworked], 0
    mov dword[_GameIsClient], 0

    call _InitMenu
    call _InitPlayer

    pusha
    invoke _LoadBMP, dword _NukeImg, dword _NukeImgFile
    mov eax, 0
.PrepareNukeLoop:

    cmp dword[_NukeImg + eax], 0FF000000h
    je .DontManuallyDepleteAlpha

    mov byte[_NukeImg + eax + 3], 0B8h

    jmp .DontBlend
.DontManuallyDepleteAlpha:

    pusha
    invoke _MakeAlphaFromRGB, dword[_NukeImg + eax]
    popa
    mov ebx, dword[_intPixel]
    mov dword[_NukeImg + eax], ebx

.DontBlend:

    add eax, 4
    cmp eax, 32*32*4
    jb .PrepareNukeLoop
    popa

    push dword (640 * 480 * 4 * 4)
    call malloc
    mov dword[_StoryClipsOff], eax
    add esp, 4
    invoke _LoadBMP, dword[_StoryClipsOff], dword _StoryClipsFile

    push dword (480 * 480 * 4)
    call malloc
    mov dword[_TitleScreenOff], eax
    add esp, 4

    push dword (460 * 345 * 4)
    call malloc
    mov dword[_VictoryScreenOff], eax
    add esp, 4

    push dword (460 * 345 * 4)
    call malloc
    mov dword[_DefeatScreenOff], eax
    add esp, 4

    invoke _LoadBMP, dword[_TitleScreenOff], dword _TitleScreenFile

    invoke _LoadBMP, dword[_VictoryScreenOff], dword _VictoryFile
    mov eax, 3
    mov esi, dword[_VictoryScreenOff]
.SetupVictoryFileAlpha:
    mov byte[esi + eax], 200
    add eax, 4
    cmp eax, 460 * 345 * 4
    jb .SetupVictoryFileAlpha

    invoke _LoadBMP, dword[_DefeatScreenOff], dword _DefeatFile
    mov eax, 3
    mov esi, dword[_DefeatScreenOff]
.SetupDefeatFileAlpha:
    mov byte[esi + eax], 200
    add eax, 4
    cmp eax, 460 * 345 * 4
    jb .SetupDefeatFileAlpha

    call _InitParticleEngine
    call _LoadEnemyData

    finit
    fild dword[_MapMidX]
    fild dword[_MapMidY]
    fstp dword[_fltPlayerY]
    fstp dword[_fltPlayerX]

    mov eax, dword[_MapMidX]
    mov dword[_intPlayerX], eax
    mov eax, dword[_MapMidY]
    mov dword[_intPlayerY], eax

    mov byte[_intbPlayerAngle], PLAYER_START_ANGLE

    fldz
    fstp dword[_fltPlayerSpeed]

    fldz
    fstp dword[_fltPlayerStrafeSpeed]

    mov dword[_intTemp], 0
.TrigLookupLoop:

    mov eax, dword[_intTemp]
    shl eax, 2

    fild dword[_intTemp]
    fld dword[_fltDegToRad256]
    fmulp st1, st0
    fsin
    fstp dword[_SIN_LOOK + eax]

    fild dword[_intTemp]
    fld dword[_fltDegToRad256]
    fmulp st1, st0
    fcos
    fstp dword[_COS_LOOK + eax]

    inc dword[_intTemp]
    cmp dword[_intTemp], 256
    jb .TrigLookupLoop

    ; Enable OGG
    invoke Mix_Init, dword 0x00000008
    
    invoke Mix_LoadWAV, dword _SNDEffectEnginesFile
    mov dword[_SNDEffect_Engines], eax

    invoke Mix_LoadWAV, dword _SNDEffectEvilLaughFile
    mov dword[_SNDEffect_EvilLaugh], eax

    invoke Mix_LoadWAV, dword _SNDEffectHitFile
    mov dword[_SNDEffect_Hit], eax
    invoke Mix_VolumeChunk, dword[_SNDEffect_Hit + 0], dword 10
    
    invoke Mix_LoadWAV, dword _SNDEffectWeaponFile
    mov dword[_SNDEffect_Weapon], eax
    invoke Mix_VolumeChunk, dword[_SNDEffect_Weapon], dword 80

    invoke Mix_LoadWAV, dword _SNDEffectExplosion1File
    mov dword[_SNDEffect_ExplosionArray + 0], eax
    invoke Mix_VolumeChunk, dword[_SNDEffect_ExplosionArray + 0], dword 128
    
    invoke Mix_LoadWAV, dword _SNDEffectExplosion2File
    mov dword[_SNDEffect_ExplosionArray + 4], eax
    invoke Mix_VolumeChunk, dword[_SNDEffect_ExplosionArray + 4], dword 128
    
    invoke Mix_LoadWAV, dword _SNDEffectExplosion3File
    mov dword[_SNDEffect_ExplosionArray + 8], eax
    invoke Mix_VolumeChunk, dword[_SNDEffect_ExplosionArray + 8], dword 128
    
    invoke Mix_LoadWAV, dword _SNDEffectExplosion4File
    mov dword[_SNDEffect_ExplosionArray + 12], eax
    invoke Mix_VolumeChunk, dword[_SNDEffect_ExplosionArray + 12], dword 128
    
    invoke Mix_LoadWAV, dword _SNDEffectExplosion5File
    mov dword[_SNDEffect_ExplosionArray + 16], eax
    invoke Mix_VolumeChunk, dword[_SNDEffect_ExplosionArray + 16], dword 128

    invoke Mix_HookMusicFinished, dword _MusicTrackFinishedCallback

    popa
    ret


;====== DestroyGameData ===================================================
;;; _DestroyGameData
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: Deallocates a ton of game data
_DestroyGameData:
    pusha

.WaitForMusicFadeIn:
    invoke Mix_FadingMusic
    cmp eax, MIX_NO_FADING
    jne .WaitForMusicFadeIn

    ;invoke Mix_FadeOutMusic, dword 500
    invoke Mix_HaltMusic
    invoke Mix_HaltChannel, dword -1

    call _DestroyMenu
    call _DestroyPlayer
    call _DestroyParticleEngine
    call _DestroyEnemyData

    push dword[_StoryClipsOff]
    call free
    add esp, 4

    push dword[_TitleScreenOff]
    call free
    add esp, 4

    push dword[_VictoryScreenOff]
    call free
    add esp, 4

    push dword[_DefeatScreenOff]
    call free
    add esp, 4

;        cmp dword[_SNDTrack], 0
;        je .SkipDeallocationStep
;        invoke Mix_FreeMusic, dword[_SNDTrack]
;.SkipDeallocationStep:

    invoke Mix_FreeChunk, dword[_SNDEffect_Engines]
    invoke Mix_FreeChunk, dword[_SNDEffect_EvilLaugh]
    invoke Mix_FreeChunk, dword[_SNDEffect_Hit]
    invoke Mix_FreeChunk, dword[_SNDEffect_Weapon]
    invoke Mix_FreeChunk, dword[_SNDEffect_ExplosionArray + 0]
    invoke Mix_FreeChunk, dword[_SNDEffect_ExplosionArray + 4]
    invoke Mix_FreeChunk, dword[_SNDEffect_ExplosionArray + 8]
    invoke Mix_FreeChunk, dword[_SNDEffect_ExplosionArray + 12]
    invoke Mix_FreeChunk, dword[_SNDEffect_ExplosionArray + 16]

    popa
    ret



;====== ProcessInput ===================================================
;;; _ProcessInput
;;;   INPUTS: [_KEYBOARD] data
;;;  OUTPUTS: Updated position/data
;;;  PURPOSE: Update the game based on user input
_ProcessInput:

    pusha

    cmp byte[_GameTurnout], PLAYER_DEAD
    je near .Done

    mov byte[_intbPlayerTurnDir], 0

    cmp byte[_KEYBOARD + SDLK_x], 1
    je near .DontKillFiringSound
    cmp dword[_SNDWeapon_Counter], 0
    je .DontKillFiringSound
    dec dword[_SNDWeapon_Counter]
    invoke Mix_HaltChannel, dword 1
.DontKillFiringSound:
    cmp byte[_KEYBOARD + SDLK_x], 1
    jne near .NotX
    cmp dword[_SNDWeapon_Counter], 0
    jne .DontPlayWeaponSound
    inc dword[_SNDWeapon_Counter]
    invoke Mix_PlayChannelTimed, dword 1, dword[_SNDEffect_Weapon], dword -1, dword -1
.DontPlayWeaponSound:
    call _FirePlayerWeapons
.NotX:

    cmp byte[_KEYBOARD + SDLK_RIGHT], 1
    jnz .NotRight

    add byte[_intbPlayerAngle], PLAYER_ROTATE_SPEED
    add byte[_intbPlayerTurnDir], 1

    call _FirePlayerRightThrusters

.NotRight:
    cmp byte[_KEYBOARD + SDLK_LEFT], 1
    jnz .NotLeft

    sub byte[_intbPlayerAngle], PLAYER_ROTATE_SPEED
    sub byte[_intbPlayerTurnDir], 1

    call _FirePlayerLeftThrusters

.NotLeft:

    finit

    cmp byte[_KEYBOARD + SDLK_UP], 1
    je near .DontKillEngineSound
    cmp dword[_SNDEngines_Counter], 0
    je .DontKillEngineSound
    dec dword[_SNDEngines_Counter]
    invoke Mix_HaltChannel, dword 3
.DontKillEngineSound:
    cmp byte[_KEYBOARD + SDLK_UP], 1
    jnz .NotUp
    cmp dword[_SNDEngines_Counter], 0
    jne .DontPlayEngineSound
    inc dword[_SNDEngines_Counter]
    invoke Mix_PlayChannelTimed, dword 3, dword[_SNDEffect_Engines], dword -1, dword -1
.DontPlayEngineSound:

    fld dword[_fltPlayerMaxSpeed]
    fld dword[_fltPlayerSpeed]
    fcomip st1                          ;cmp _fltPlayerSpeed, _fltPlayerMaxSpeed
    fdecstp                             ;if less, C=1, else 0
    jnc .NotUp

    fld dword[_fltPlayerSpeed]
    fld dword[_fltPlayerAccel]
    faddp st1, st0
    fstp dword[_fltPlayerSpeed]

    call _FirePlayerMainThrusters

.NotUp:

    finit
    cmp byte[_KEYBOARD + SDLK_DOWN], 1
    jnz .NotDown

    fld dword[_fltPlayerMinSpeed]
    fld dword[_fltPlayerSpeed]
    fcomip st1
    fdecstp
    jc .NotDown

    fld dword[_fltPlayerSpeed]
    fld dword[_fltPlayerAccel]
    fsubp st1, st0
    fstp dword[_fltPlayerSpeed]

.NotDown:

    cmp byte[_KEYBOARD + SDLK_c], 1
    jnz .NotC

    add byte[_intbPlayerTurnDir], 1

    fld dword[_fltPlayerMaxStrafeSpeed]
    fld dword[_fltPlayerStrafeSpeed]
    fcomip st1
    fdecstp
    jnc .NotC

    fld dword[_fltPlayerStrafeSpeed]
    fld dword[_fltPlayerAccel]
    faddp st1, st0
    fstp dword[_fltPlayerStrafeSpeed]

.NotC:

    cmp byte[_KEYBOARD + SDLK_z], 1
    jnz .NotZ

    sub byte[_intbPlayerTurnDir], 1

    fld dword[_fltPlayerMinStrafeSpeed]
    fld dword[_fltPlayerStrafeSpeed]
    fcomip st1
    fdecstp
    jc .NotZ

    fld dword[_fltPlayerStrafeSpeed]
    fld dword[_fltPlayerAccel]
    fsubp st1, st0
    fstp dword[_fltPlayerStrafeSpeed]

.NotZ:


    cmp byte[_KEYBOARD + SDLK_SPACE], 1
    jnz .NotSPACE
    cmp dword[_NukeWaitCounter], 0
    ja .NotSPACE
    cmp dword[_NukesRemaining], 0
    jle .NotSPACE
    dec dword[_NukesRemaining]

    mov dword[_NukeWaitCounter], 25
    invoke _DropNuke, dword[_fltPlayerX], dword[_fltPlayerY]

.NotSPACE:


.Done:

    popa
    ret


;====== LoadLevel ===================================================
;;; _LoadLevel
;;;   INPUTS: Level ID
;;;  OUTPUTS: Loaded Level
;;;  PURPOSE: Load levels in the game
_LoadLevel:
.level_id    EQU    4
    pusha

    mov eax, dword[_NumStartingNukes]
    mov dword[_NukesRemaining], eax

    mov byte [_GameTurnout], PLAYER_NORMAL

    mov dword[_GameState], 1
    mov dword[_intShakeMap], 0

    mov eax, dword[ebp + .level_id]
    sub eax, 2
    mov dword[_ActiveLevel], eax
    invoke _InitializeEnemies, dword eax

    invoke Mix_FadeOutMusic, dword 200

    finit
    fild dword[_MapMidX]
    fild dword[_MapMidY]
    fstp dword[_fltPlayerY]
    fstp dword[_fltPlayerX]

    mov eax, dword[_MapMidX]
    mov dword[_intPlayerX], eax
    mov eax, dword[_MapMidY]
    mov dword[_intPlayerY], eax

    mov byte[_intbPlayerAngle], PLAYER_START_ANGLE

    fldz
    fstp dword[_fltPlayerSpeed]

    fldz
    fstp dword[_fltPlayerStrafeSpeed]

    mov dword[_intPlayerHealth], MAXPLAYERHEALTH

    call _ResetParticleEngine

    cmp dword[ebp + .level_id], MENU_LOAD_SHIRE
    jne near .DontLoadShire

    invoke  _LoadMap, dword _Map_FileShire

    jmp .Done

.DontLoadShire:

    cmp dword[ebp + .level_id], MENU_LOAD_MORDOR
    jne near .DontLoadMordor

    mov dword[_EndingDelayTimer], 250
    invoke _LoadMap, dword _Map_FileMordor

    jmp .Done

.DontLoadMordor:

    cmp dword[ebp + .level_id], MENU_LOAD_MIDKEMIA
    jne near .DontLoadMidkemia

    invoke  _LoadMap, dword _Map_FileMidkemia

    jmp .Done

.DontLoadMidkemia:

    cmp dword[ebp + .level_id], MENU_LOAD_ARCHIPELAGO
    jne near .DontLoadArchipelago

    invoke  _LoadMap, dword _Map_FileArchipelago

    jmp .Done

.DontLoadArchipelago:

    cmp dword[ebp + .level_id], MENU_LOAD_DUNE
    jne near .DontLoadDune

    invoke  _LoadMap, dword _Map_FileDune

    jmp .Done

.DontLoadDune:

    cmp dword[ebp + .level_id], MENU_LOAD_OCEANIA
    jne near .DontLoadOceania

    invoke  _LoadMap, dword _Map_FileOceania

    jmp .Done

.DontLoadOceania:

.Done:
    popa
    ret


;====== DisplayTitleScreen ===================================================
;;; _DisplayTitleScreen
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: Display title
_DisplayTitleScreen:
    pusha

    invoke SDL_ShowCursor, dword 0

    mov byte[_IsMenuRunning], 1
    call _MusicTrackFinishedCallback

    pusha
    invoke _sseMemset32, dword[_ScreenOff], dword 0, dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _AlphaBlit, dword SCREEN_WIDTH/2 - 480/2, dword SCREEN_HEIGHT/2 - 480/2, dword[_TitleScreenOff], dword 480, dword 480
    invoke _sseMemcpy32, dword _ScreenTemp, dword[_ScreenOff], dword SCREEN_WIDTH * SCREEN_HEIGHT
    popa
    invoke _FadeFromBlack, dword 180, dword 2
    invoke _FadeToBlack, dword 180, dword 3

    mov eax, dword[_StoryClipsOff]
    add eax, 640 * 480 * 4 * 3
    pusha
    invoke _AlphaBlit, dword SCREEN_WIDTH/2 - 640/2, dword SCREEN_HEIGHT/2 - 480/2, dword eax, dword 640, dword 480
    invoke _sseMemcpy32, dword _ScreenTemp, dword[_ScreenOff], dword SCREEN_WIDTH * SCREEN_HEIGHT
    popa
    invoke _FadeFromBlack, dword 180, dword 3
    invoke _FadeToBlack, dword 180, dword 3


.Done:
    call _UpdateInput
    cmp byte[_KEYBOARD + SDLK_ESCAPE], 1
    je .Done
    cmp word[_MOUSE_LBUTTON], 1
    je near .Done

    popa
    ret

;====== _MusicTrackFinishedCallback ===================================================
;;; _MusicTrackFinishedCallback
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: music stuff
_MusicTrackFinishedCallback:
    pusha

    cmp dword[_SNDTrack], 0
    je .SkipDeallocationStep
    invoke Mix_FreeMusic, dword[_SNDTrack]
.SkipDeallocationStep:

    invoke Mix_VolumeMusic, dword 110

    cmp dword[_IsEndingRunning], 0
    je .NotGameEnding

    invoke Mix_LoadMUS, dword _SNDTrackEndingFile
    mov dword[_SNDTrack], eax
    invoke Mix_PlayMusic, dword[_SNDTrack], dword 1
    jmp .Done

.NotGameEnding:

    cmp byte[_IsMenuRunning], 0
    je near .MenuNotRunning

    invoke Mix_LoadMUS, dword _SNDTrackMenuFile
    mov dword[_SNDTrack], eax
    invoke Mix_PlayMusic, dword[_SNDTrack], dword 1
    jmp .Done

.MenuNotRunning:

    cmp dword[_ActiveLevel], 0
    jne .NotShire

    invoke Mix_LoadMUS, dword _SNDTrack1File
    mov dword[_SNDTrack], eax
    invoke Mix_PlayMusic, dword[_SNDTrack], dword 1
    jmp .Done

.NotShire:

    cmp dword[_ActiveLevel], 1
    jne .NotArchipelago

    invoke Mix_LoadMUS, dword _SNDTrack2File
    mov dword[_SNDTrack], eax
    invoke Mix_PlayMusic, dword[_SNDTrack], dword 1
    jmp .Done

.NotArchipelago:

    cmp dword[_ActiveLevel], 2
    jne .NotDune

    invoke Mix_LoadMUS, dword _SNDTrack3File
    mov dword[_SNDTrack], eax
    invoke Mix_PlayMusic, dword[_SNDTrack], dword 1
    jmp .Done

.NotDune:

    cmp dword[_ActiveLevel], 3
    jne .NotMidkemia

    invoke Mix_LoadMUS, dword _SNDTrack4File
    mov dword[_SNDTrack], eax
    invoke Mix_PlayMusic, dword[_SNDTrack], dword 1
    jmp .Done

.NotMidkemia:

    cmp dword[_ActiveLevel], 4
    jne .NotOceania

    invoke Mix_LoadMUS, dword _SNDTrack5File
    mov dword[_SNDTrack], eax
    invoke Mix_PlayMusic, dword[_SNDTrack], dword 1
    jmp .Done

.NotOceania:

    cmp dword[_ActiveLevel], 5
    jne .NotMordor

    invoke Mix_LoadMUS, dword _SNDTrack6File
    mov dword[_SNDTrack], eax
    invoke Mix_PlayMusic, dword[_SNDTrack], dword 1
    jmp .Done

.NotMordor:

.Done:
    popa
    ret


;====== _RenderMiniMap ===================================================
;;; _RenderMiniMap
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: minimap
_RenderMiniMap:
    pusha

    cmp byte[_GameTurnout], PLAYER_DEAD
    je near .Done

    mov esi, dword[_ScreenOff]
    mov eax, 0

mov dword[_intTemp], 080000000h

    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx
.RenderLoop:

    ComputeAlpha dword[esi + eax + (SCREEN_WIDTH * 4 * 20) + (4 * 16)], dword[_intTemp], dword[esi + eax + (SCREEN_WIDTH * 4 * 20) + (4 * 16)]

    inc ecx
    cmp ecx, MINIMAP_WIDTH
    jb .DontIncrementRow

    xor ecx, ecx
    add eax, (SCREEN_WIDTH - MINIMAP_WIDTH) * 4

.DontIncrementRow:

    add ebx, 4
    add eax, 4
    cmp ebx, MINIMAP_WIDTH * MINIMAP_HEIGHT * 4
    jb .RenderLoop

    pusha
    invoke _DrawMiniMapObject, dword[_intPlayerX], dword[_intPlayerY], dword 0800000FFh, dword 0A0FFFFFFh, dword 1
    popa

    xor eax, eax
    xor ebx, ebx
.DrawEnemyObjectsLoop:

    cmp byte [_Enemies + eax + ENEMY.EnemyActive], -1
    je near .NextEnemy
    cmp byte [_Enemies + eax + ENEMY.EnemyActive], 0
    je near .NextEnemy

    cmp dword[_Enemies + eax + ENEMY.EnemyType], 3
    jne near .NotShimDog
    cmp byte[_Enemies + eax + ENEMY.EnemySize], 1
    jne near .NotShimDog

    pusha
    invoke _DrawMiniMapObject, dword[_Enemies + eax + ENEMY.EnemyXInt], dword[_Enemies + eax + ENEMY.EnemyYInt], dword 0A0FF0000h, dword 0A0FFFFFFh, dword 1
    popa

    jmp near .NextEnemy
.NotShimDog:

    xor ebx, ebx
    mov bl, byte[_Enemies + eax + ENEMY.EnemySize]

    pusha
    invoke _DrawMiniMapObject, dword[_Enemies + eax + ENEMY.EnemyXInt], dword[_Enemies + eax + ENEMY.EnemyYInt], dword 0A0FF0000h, dword 0A0FFFFFFh, dword ebx
    popa

.NextEnemy:
    add eax, 40
    cmp eax, 40*100
    jb near .DrawEnemyObjectsLoop

    emms
.Done:

    popa
    ret


;====== _DrawMiniMapObject ===================================================
;;; _DrawMiniMapObject
;;;   INPUTS: stuff
;;;  OUTPUTS: none
;;;  PURPOSE: minimap objects
_DrawMiniMapObject:
.x              EQU     4
.y              EQU     8
.color1         EQU     12
.color2         EQU     16
.is_boss        EQU     20

    mov esi, dword[_ScreenOff]

    mov eax, dword[ebp + .y]
    mov ebx, MAP_HEIGHT / MINIMAP_HEIGHT
    div ebx
    add eax, 20
    mov ebx, SCREEN_WIDTH
    mul ebx

    mov ecx, eax
    mov eax, dword[ebp + .x]
    mov ebx, MAP_WIDTH / MINIMAP_WIDTH
    div ebx
    add eax, 16
    add eax, ecx
    shl eax, 2

    ComputeAlpha dword[esi + eax], dword[ebp + .color1], dword[esi + eax]
    ComputeAlpha dword[esi + eax - 4], dword[ebp + .color1], dword[esi + eax - 4]
    ComputeAlpha dword[esi + eax + 4], dword[ebp + .color1], dword[esi + eax + 4]
    ComputeAlpha dword[esi + eax + SCREEN_WIDTH * 4], dword[ebp + .color1], dword[esi + eax + SCREEN_WIDTH * 4]
    ComputeAlpha dword[esi + eax - SCREEN_WIDTH * 4], dword[ebp + .color1], dword[esi + eax - SCREEN_WIDTH * 4]

    ComputeAlpha dword[esi + eax - SCREEN_WIDTH * 4 - 4], dword[ebp + .color2], dword[esi + eax - SCREEN_WIDTH * 4 - 4]
    ComputeAlpha dword[esi + eax - SCREEN_WIDTH * 4 + 4], dword[ebp + .color2], dword[esi + eax - SCREEN_WIDTH * 4 + 4]
    ComputeAlpha dword[esi + eax + SCREEN_WIDTH * 4 - 4], dword[ebp + .color2], dword[esi + eax + SCREEN_WIDTH * 4 - 4]
    ComputeAlpha dword[esi + eax + SCREEN_WIDTH * 4 + 4], dword[ebp + .color2], dword[esi + eax + SCREEN_WIDTH * 4 + 4]

    ComputeAlpha dword[esi + eax - 8], dword[ebp + .color2], dword[esi + eax - 8]
    ComputeAlpha dword[esi + eax + 8], dword[ebp + .color2], dword[esi + eax + 8]
    ComputeAlpha dword[esi + eax + SCREEN_WIDTH * 4 * 2], dword[ebp + .color2], dword[esi + eax + SCREEN_WIDTH * 4 * 2]
    ComputeAlpha dword[esi + eax - SCREEN_WIDTH * 4 * 2], dword[ebp + .color2], dword[esi + eax - SCREEN_WIDTH * 4 * 2]

    cmp dword[ebp + .is_boss], 0
    je near .Done

    ComputeAlpha dword[esi + eax - 12], dword[ebp + .color1], dword[esi + eax - 12]
    ComputeAlpha dword[esi + eax + 12], dword[ebp + .color1], dword[esi + eax + 12]
    ComputeAlpha dword[esi + eax - 16], dword[ebp + .color2], dword[esi + eax - 16]
    ComputeAlpha dword[esi + eax + 16], dword[ebp + .color2], dword[esi + eax + 16]
    ComputeAlpha dword[esi + eax + SCREEN_WIDTH * 4 * 3], dword[ebp + .color1], dword[esi + eax + SCREEN_WIDTH * 4 * 3]
    ComputeAlpha dword[esi + eax - SCREEN_WIDTH * 4 * 3], dword[ebp + .color1], dword[esi + eax - SCREEN_WIDTH * 4 * 3]
    ComputeAlpha dword[esi + eax + SCREEN_WIDTH * 4 * 4], dword[ebp + .color2], dword[esi + eax + SCREEN_WIDTH * 4 * 4]
    ComputeAlpha dword[esi + eax - SCREEN_WIDTH * 4 * 4], dword[ebp + .color2], dword[esi + eax - SCREEN_WIDTH * 4 * 4]

.Done:

    ret



;====== FadeScreen ===================================================
;;; _FadeScreen
;;;   INPUTS: color
;;;  OUTPUTS: none
;;;  PURPOSE: gfx
_FadeScreen:
.color          EQU     4

    pusha

    mov esi, dword[_ScreenOff]

    xor eax, eax
.ScreenLoop:

    ComputeAlpha dword[esi + eax], dword[ebp + .color], dword[esi + eax]

    add eax, 4
    cmp eax, SCREEN_WIDTH * SCREEN_HEIGHT * 4
    jb .ScreenLoop

    popa

    ret


;====== SaturatedAlphaAdd ===================================================
;;; _SaturatedAlphaAdd
;;;   INPUTS: pixel, number
;;;  OUTPUTS: none
;;;  PURPOSE: gfx
_SaturatedAlphaAdd:
.pixel          EQU     4
.num            EQU     8

    pusha

    mov edx, dword[ebp + .pixel]

    xor eax, eax
    mov al, byte[edx + 3]
    add eax, dword[ebp + .num]

    mov byte[edx + 3], al

    cmp eax, 0FFh
    jbe .NotSaturated
    mov byte[edx + 3], 0FFh
.NotSaturated:

    popa
    ret


;====== SaturatedAlphaSub ===================================================
;;; _SaturatedAlphaSub
;;;   INPUTS: pixel, number
;;;  OUTPUTS: none
;;;  PURPOSE: gfx
_SaturatedAlphaSub:
.pixel          EQU     4
.num            EQU     8

    pusha

    mov edx, dword[ebp + .pixel]

    xor eax, eax
    mov al, byte[edx + 3]
    sub eax, dword[ebp + .num]

    mov byte[edx + 3], al

    cmp eax, 0
    jg .NotSaturated
    mov byte[edx + 3], 00h
.NotSaturated:

    popa
    ret
    
;====== FadeFromWhite ===================================================
;;; _FadeFromWhite
;;;   INPUTS: num frames
;;;  OUTPUTS: none
;;;  PURPOSE: gfx
_FadeFromWhite:
.frame_count    EQU     4
.speed  EQU     8

.Wait:
    call _FlushKeyboard
    call _UpdateInput
    cmp byte[_KEYBOARD + SDLK_ESCAPE], 1
    je .SkipWait

    xor eax, eax
    xor ebx, ebx
.CountKeys1:
    xor edx, edx
    mov dl, byte[_KEYBOARD + eax]
    add ebx, edx
    inc eax
    cmp eax, 128
    jb .CountKeys1
    mov dword[_NumKeysPressed], ebx
    cmp dword[_NumKeysPressed], 0
    ja near .Wait
    cmp word[_MOUSE_LBUTTON], 1
    je near .Wait

.SkipWait:

    mov dword[_intTemp], 0FFFFFFFFh
    xor eax, eax
.DisplayLoop:

    pusha
    call _FlushKeyboard
    call _UpdateInput
    xor eax, eax
    xor ebx, ebx
.CountKeys2:
    xor edx, edx
    mov dl, byte[_KEYBOARD + eax]
    add ebx, edx
    inc eax
    cmp eax, 128
    jb .CountKeys2
    mov dword[_NumKeysPressed], ebx
    popa
    cmp word[_MOUSE_LBUTTON], 1
    je near .Done
    cmp dword[_NumKeysPressed], 0
    ja near .Done

    pusha
    invoke _sseMemcpy32, dword[_ScreenOff], dword _ScreenTemp, dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _FadeScreen, dword[_intTemp]
    invoke _SaturatedAlphaSub, dword _intTemp, dword[ebp + .speed]
    invoke SDL_UpdateRect, dword[_Screen], dword 0, dword 0, dword 0, dword 0
    invoke SDL_Delay, dword 10
    popa

    inc eax
    cmp eax, dword[ebp + .frame_count]
    jb near .DisplayLoop

.Done:
    ret


;====== FadeToWhite ===================================================
;;; _FadeFromWhite
;;;   INPUTS: num frames
;;;  OUTPUTS: none
;;;  PURPOSE: gfx
_FadeToWhite:
.frame_count    EQU     4
.speed  EQU     8

.Wait:
    call _FlushKeyboard
    call _UpdateInput
    cmp byte[_KEYBOARD + SDLK_ESCAPE], 1
    je .SkipWait

    xor eax, eax
    xor ebx, ebx
.CountKeys1:
    xor edx, edx
    mov dl, byte[_KEYBOARD + eax]
    add ebx, edx
    inc eax
    cmp eax, 128
    jb .CountKeys1
    mov dword[_NumKeysPressed], ebx
    cmp dword[_NumKeysPressed], 0
    ja near .Wait
    cmp word[_MOUSE_LBUTTON], 1
    je near .Wait


.SkipWait:

    mov dword[_intTemp], 000FFFFFFh
    xor eax, eax
.DisplayLoop:

    pusha
    call _FlushKeyboard
    call _UpdateInput
    xor eax, eax
    xor ebx, ebx
.CountKeys2:
    xor edx, edx
    mov dl, byte[_KEYBOARD + eax]
    add ebx, edx
    inc eax
    cmp eax, 128
    jb .CountKeys2
    mov dword[_NumKeysPressed], ebx
    popa
    cmp word[_MOUSE_LBUTTON], 1
    je near .Done
    cmp dword[_NumKeysPressed], 0
    ja near .Done

    pusha
    invoke _sseMemcpy32, dword[_ScreenOff], dword _ScreenTemp, dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _FadeScreen, dword[_intTemp]
    invoke _SaturatedAlphaAdd, dword _intTemp, dword[ebp + .speed]
    invoke SDL_UpdateRect, dword[_Screen], dword 0, dword 0, dword 0, dword 0
    invoke SDL_Delay, dword 10
    popa

    inc eax
    cmp eax, dword[ebp + .frame_count]
    jb near .DisplayLoop

.Done:
    ret


;====== FadeFromBlack ===================================================
;;; _FadeFromBlack
;;;   INPUTS: num frames
;;;  OUTPUTS: none
;;;  PURPOSE: gfx
_FadeFromBlack:
.frame_count    EQU     4
.speed  EQU     8

.Wait:
    call _FlushKeyboard
    call _UpdateInput
    cmp byte[_KEYBOARD + SDLK_ESCAPE], 1
    je .SkipWait

    xor eax, eax
    xor ebx, ebx
.CountKeys1:
    xor edx, edx
    mov dl, byte[_KEYBOARD + eax]
    add ebx, edx
    inc eax
    cmp eax, 128
    jb .CountKeys1
    mov dword[_NumKeysPressed], ebx
    cmp dword[_NumKeysPressed], 0
    ja near .Wait
    cmp word[_MOUSE_LBUTTON], 1
    je near .Wait

.SkipWait:

    mov dword[_intTemp], 0FF000000h
    xor eax, eax
.DisplayLoop:

    pusha
    call _FlushKeyboard
    call _UpdateInput
    xor eax, eax
    xor ebx, ebx
.CountKeys2:
    xor edx, edx
    mov dl, byte[_KEYBOARD + eax]
    add ebx, edx
    inc eax
    cmp eax, 128
    jb .CountKeys2
    mov dword[_NumKeysPressed], ebx
    popa
    cmp word[_MOUSE_LBUTTON], 1
    je near .Done
    cmp dword[_NumKeysPressed], 0
    ja near .Done

    pusha
    invoke _sseMemcpy32, dword[_ScreenOff], dword _ScreenTemp, dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _FadeScreen, dword[_intTemp]
    invoke _SaturatedAlphaSub, dword _intTemp, dword[ebp + .speed]
    invoke SDL_UpdateRect, dword[_Screen], dword 0, dword 0, dword 0, dword 0
    invoke SDL_Delay, dword 10
    popa

    inc eax
    cmp eax, dword[ebp + .frame_count]
    jb near .DisplayLoop

.Done:
    ret

;====== FadeToBlack ===================================================
;;; _FadeFromBlack
;;;   INPUTS: num frames
;;;  OUTPUTS: none
;;;  PURPOSE: gfx
_FadeToBlack:
.frame_count    EQU     4
.speed  EQU     8

.Wait:
    call _FlushKeyboard
    call _UpdateInput
    cmp byte[_KEYBOARD + SDLK_ESCAPE], 1
    je .SkipWait

    xor eax, eax
    xor ebx, ebx
.CountKeys1:
    xor edx, edx
    mov dl, byte[_KEYBOARD + eax]
    add ebx, edx
    inc eax
    cmp eax, 128
    jb .CountKeys1
    mov dword[_NumKeysPressed], ebx
    cmp dword[_NumKeysPressed], 0
    ja near .Wait
    cmp word[_MOUSE_LBUTTON], 1
    je near .Wait

.SkipWait:

    mov dword[_intTemp], 000000000h
    xor eax, eax
.DisplayLoop:

    pusha
    call _FlushKeyboard
    call _UpdateInput
    xor eax, eax
    xor ebx, ebx
.CountKeys2:
    xor edx, edx
    mov dl, byte[_KEYBOARD + eax]
    add ebx, edx
    inc eax
    cmp eax, 128
    jb .CountKeys2
    mov dword[_NumKeysPressed], ebx
    popa
    cmp word[_MOUSE_LBUTTON], 1
    je near .Done
    cmp dword[_NumKeysPressed], 0
    ja near .Done

    pusha
    invoke _sseMemcpy32, dword[_ScreenOff], dword _ScreenTemp, dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _FadeScreen, dword[_intTemp]
    invoke _SaturatedAlphaAdd, dword _intTemp, dword[ebp + .speed]
    invoke SDL_UpdateRect, dword[_Screen], dword 0, dword 0, dword 0, dword 0
    invoke SDL_Delay, dword 10
    popa

    inc eax
    cmp eax, dword[ebp + .frame_count]
    jb near .DisplayLoop

.Done:
    ret


;====== RunEnding ===================================================
;;; _RunEnding
;;;   INPUTS: none
;;;  OUTPUTS: none
;;;  PURPOSE: need an ending
_RunEnding:

    mov dword[_IsEndingRunning], 1
    mov dword[_ActivateMenu], 1
    invoke Mix_FadeOutMusic, dword 100

.Wait1:
    call _FlushKeyboard
    call _UpdateInput
    cmp byte[_KEYBOARD + SDLK_ESCAPE], 1
    je .SkipWait1

    xor eax, eax
    xor ebx, ebx
.CountKeys1:
    xor edx, edx
    mov dl, byte[_KEYBOARD + eax]
    add ebx, edx
    inc eax
    cmp eax, 128
    jb .CountKeys1
    mov dword[_NumKeysPressed], ebx
    cmp dword[_NumKeysPressed], 0
    ja near .Wait
    cmp word[_MOUSE_LBUTTON], 1
    je near .Wait1

.SkipWait1:

    invoke Mix_HaltChannel, dword -1
    invoke _sseMemcpy32, dword _ScreenTemp, dword[_ScreenOff], dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _FadeToWhite, dword 800, dword 1

    invoke _sseMemset32, dword[_ScreenOff], dword 0, dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _sseMemset32, dword _ScreenTemp, dword 0, dword SCREEN_WIDTH * SCREEN_HEIGHT

    mov eax, dword[_StoryClipsOff]
    ;add eax, 640 * 480 * 4 * 1
    invoke _AlphaBlit, dword SCREEN_WIDTH/2 - 640/2, dword SCREEN_HEIGHT/2 - 480/2, dword eax, dword 640, dword 480
    invoke _sseMemcpy32, dword _ScreenTemp, dword[_ScreenOff], dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _FadeFromWhite, dword 800, dword 1

    invoke _FadeToWhite, dword 800, dword 1

    mov eax, dword[_StoryClipsOff]
    add eax, 640 * 480 * 4 * 1
    invoke _AlphaBlit, dword SCREEN_WIDTH/2 - 640/2, dword SCREEN_HEIGHT/2 - 480/2, dword eax, dword 640, dword 480
    invoke _sseMemcpy32, dword _ScreenTemp, dword[_ScreenOff], dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _FadeFromWhite, dword 800, dword 1

    invoke _FadeToWhite, dword 800, dword 1

    mov eax, dword[_StoryClipsOff]
    add eax, 640 * 480 * 4 * 2
    invoke _AlphaBlit, dword SCREEN_WIDTH/2 - 640/2, dword SCREEN_HEIGHT/2 - 480/2, dword eax, dword 640, dword 480
    invoke _sseMemcpy32, dword _ScreenTemp, dword[_ScreenOff], dword SCREEN_WIDTH * SCREEN_HEIGHT
    invoke _FadeFromWhite, dword 800, dword 1

.Wait:
    call _FlushKeyboard
    call _UpdateInput

    mov eax, 0
    mov ebx, 0
.CountKeys:
    mov edx, 0
    mov dl, byte[_KEYBOARD + eax]
    add ebx, edx
    inc eax
    cmp eax, 128
    jb .CountKeys

    cmp ebx, 0
    je .Wait

    invoke _FadeToBlack, dword 800, dword 2

    mov dword[_IsEndingRunning], 0
    ret
