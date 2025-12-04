
%include "macros.inc"

;------------------------------------------------------------------------------
SDL_INIT_TIMER			EQU 				00000001h
SDL_INIT_AUDIO			EQU 				00000010h
SDL_INIT_VIDEO			EQU 				00000020h
SDL_INIT_CDROM			EQU 				00000100h
SDL_INIT_JOYSTICK		EQU	 			00000200h
SDL_INIT_NOPARACHUTE		EQU 				00100000h	
SDL_INIT_EVENTTHREAD		EQU 				01000000h	
SDL_INIT_EVERYTHING		EQU 				0000FFFFh
SDL_SWSURFACE			EQU				00000000h	
SDL_HWSURFACE		 	EQU				00000001h	
SDL_ASYNCBLIT		  	EQU				00000004h	
SDL_ANYFORMAT			EQU				10000000h	
SDL_HWPALETTE			EQU				20000000h	
SDL_DOUBLEBUF			EQU				40000000h	
SDL_FULLSCREEN			EQU				80000000h	
SDL_OPENGL      		EQU				00000002h
SDL_OPENGLBLIT			EQU				0000000Ah	
SDL_RESIZABLE			EQU				00000010h	
SDL_NOFRAME			EQU				00000020h	
SDL_HWACCEL			EQU				00000100h	
SDL_SRCCOLORKEY 		EQU				00001000h
SDL_RLEACCELOK			EQU				00002000h
SDL_RLEACCEL			EQU				00004000h	
SDL_SRCALPHA			EQU				00010000h	
SDL_PREALLOC			EQU				01000000h

AUDIO_U8                        EQU                             00000008h
AUDIO_S8                        EQU                             00008008h
AUDIO_U16LSB                    EQU                             00000010h
AUDIO_S16LSB                    EQU                             00008010h
AUDIO_U16MSB                    EQU                             00001010h
AUDIO_S16MSB	                EQU                             00009010h
AUDIO_U16                       EQU                             AUDIO_U16LSB
AUDIO_S16                       EQU                             AUDIO_S16LSB

AUDIO_BUFFER_SIZE               EQU                            4096
AUDIO_SAMPLE_RATE               EQU                             22050
AUDIO_NUM_CHANNELS              EQU                            8

MIX_NO_FADING                   EQU                             0
;------------------------------------------------------------------------------

EXTERN _InitGraphics
EXTERN _DestroyGraphics
EXTERN _LoadBMP
EXTERN _AlphaBlit

EXTERN _Screen
EXTERN _ScreenOff

EXTERN SDL_UpdateRect
EXTERN SDL_LockSurface
EXTERN SDL_UnlockSurface
EXTERN SDL_ShowCursor
EXTERN SDL_GetTicks
EXTERN SDL_Delay

EXTERN Mix_Init, SDL_GetError, Mix_OpenAudio, Mix_AllocateChannels, Mix_CloseAudio
EXTERN Mix_LoadWAV_RW, SDL_RWFromFile, Mix_VolumeChunk, Mix_LoadMUS, Mix_PlayMusic, Mix_PlayChannelTimed
EXTERN Mix_Playing, Mix_PlayingMusic, Mix_FreeChunk, Mix_FreeMusic, Mix_HaltChannel
EXTERN Mix_FadeOutChannel, Mix_FadeOutMusic, Mix_FadingMusic, Mix_HookMusicFinished
EXTERN Mix_LoadWAV, Mix_FadingChannel, Mix_FadeInChannelTimed, Mix_VolumeMusic
EXTERN Mix_HaltMusic, Mix_HaltChannel

