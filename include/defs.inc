%ifndef _DEFS_H_
%define _DEFS_H_

%include "macros.inc"

    EXTERN printf
    EXTERN malloc
    EXTERN free
    EXTERN strcmp
    EXTERN fopen
    EXTERN fclose
    EXTERN fread
    EXTERN fwrite
    
;====== SECTION 1: Define constants =======================================

        SCREEN_WIDTH            EQU 1280 ;640
        SCREEN_HEIGHT           EQU 800 ;480
        SCREEN_BPP              EQU 32

        MAP_WIDTH               EQU 3200
        MAP_HEIGHT              EQU 2400

        MINIMAP_WIDTH           EQU 128
        MINIMAP_HEIGHT          EQU 96

        PLAYER_WIDTH            EQU 128
        PLAYER_HEIGHT           EQU 128

        MAXPLAYERHEALTH             EQU 10000
        PLAYER_HEALTH_BAR_HEIGHT    EQU 10

        PLAYER_ROTATE_SPEED     EQU 4
        PLAYER_START_ANGLE      EQU -64

        PLAYER_IMG_FILE_WIDTH   EQU PLAYER_WIDTH
        PLAYER_IMG_FILE_HEIGHT  EQU PLAYER_HEIGHT*256

        SHAKE_FACTOR            EQU 40

        MAX_PARTICLES           EQU 10000
        SMALL_PARTICLE_WIDTH    EQU 32
        SMALL_PARTICLE_HEIGHT   EQU 32
        LARGE_PARTICLE_WIDTH    EQU 64
        LARGE_PARTICLE_HEIGHT   EQU 64
        PARTICLE_STRUCT_SIZE    EQU 46
        SMALL_PARTICLE          EQU 0
        LARGE_PARTICLE          EQU 1

        NUM_SMALL_FLARE_TYPES   EQU 16
        NUM_LARGE_FLARE_TYPES   EQU 10

        SMALL_FLARE_FILE_WIDTH  EQU 32
        SMALL_FLARE_FILE_HEIGHT EQU 512
        LARGE_FLARE_FILE_WIDTH  EQU 64
        LARGE_FLARE_FILE_HEIGHT EQU 640

        PLAYER_NORMAL           EQU 0
        PLAYER_DEAD             EQU 1
        PLAYER_WIN              EQU 2

        NUM_NUKES               EQU 4
        
;=========================================================================
; Enemy Defs
        RANDSPAWNDIVISOR        EQU 2
        MASS_HEALTH_MULTIPLIER  EQU 20
        ENEMYSHIFTSIZE          EQU 16
        SMALLENEMYMEMSIZE       EQU 128*128*4*16*4
        BOSSENEMYMEMSIZE        EQU 256*256*4*16*4

        INNER_RADIUS_CUTOFF     EQU 300;300
        OUTER_RADIUS_CUTOFF     EQU 2500;1200

        MAX_ENEMY_TURNING_SPEED EQU 4
	MAX_ENEMY_SPEED         EQU 4

;==========================================================================
; Sound Defs

	DMABUFFSIZE             EQU 2000	; Buffer Size
        SOUND_KILL              EQU 1           ; Stop sound this buffer
        SOUND_END               EQU 0           ; Stop sound at end of repeat
        SOUND_REPEAT            EQU 80h         ; Repeat Sound
        SOUND_NOREPEAT          EQU 0h          ; Don't repeat


;==========================================================================
; Menu Defs

        MENU_LOAD_SHIRE         EQU 2
        MENU_LOAD_MORDOR        EQU 7
        MENU_LOAD_MIDKEMIA      EQU 5
        MENU_LOAD_ARCHIPELAGO   EQU 3
        MENU_LOAD_DUNE          EQU 4
        MENU_LOAD_OCEANIA       EQU 6


        CURSOR_WIDTH            EQU 64
        CURSOR_HEIGHT           EQU 64

;====== SECTION 2: Declare external routines ==============================


        _LoadLevel_arglen           EQU 4
        _AlphaBlit_arglen           EQU 20
        _MakeAlphaFromRGB_arglen    EQU 4
        _EnemyCollisionDetection_arglen        EQU      4

%endif




