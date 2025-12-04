
;============================================================
; Project:      "The Alan Parsons Project"
; Description:  Code for ece291 final project
; Author:       Scott Moeller
; Website:      http://www.ews.uiuc.edu/~smoeller/
; Date:		7/20/02
;============================================================

%include "lib291.inc"
%include "defs.inc"

BITS 32              ; Tell NASM we're using 32-bit instructions by default.

     GLOBAL _AddSound
     GLOBAL _AddSoundTrack
     GLOBAL _SoundBufferUpdate
     GLOBAL _SOUNDISRTRIGGERED
     GLOBAL _SoundISR
     GLOBAL _LoadSound
     GLOBAL _SoundIsPlaying
     GLOBAL _ISR_Running
     GLOBAL _StopSound
     GLOBAL _StopSoundTrack
     GLOBAL _KillAllSound


SECTION .bss
;;; DMA Information storage
DMASel              resw	1
DMAAddr	            resd	1
DMAChan	            resb	1

SECTION .data
_SOUNDFOUND	    db	        0
_SOUNDISRTRIGGERED  dd          0
_ISR_Running        db          0
_DMAOffset          db          0

;SoundTrack information uses doubles for faster memory access times (four-byte blocks load faster, performance is key worry here)
_SoundTrack
                    dd -1       ; ID being played (base zero)

                    dd -1	; Base address of sound
		    dd -1	; Length of sound
		    dd -1	; Current pointer
		    dd -1	; Next id to play

		    times 36 dd -1	; Remaining 9 soundtrack files data (follow above framework)

;;; Channel sound information, 7 channels
_SoundInfo
                    dd -1       ; Location in memory of start point
                    dd -1	; Length in memory (bytes)
		    dd -1	; Current location of copy (into DMA)
		    db 0	; 7th bit is repeat bit
		    db 0	; Sound ID byte

		    times 42 dw -1	; Channels two through seven, following above framework
.end

;;; Function:  StopSound
;;; Purpose:   To stop a sound playing on a certain ID (Either immediately or after it finishes current loop)
;;; Inputs:
;;;	     .SoundID, byte ID of sound to stop
;;;          .HowStop, tells fn whether to truncate sound or simply turn off looping (SOUND_KILL or SOUND_END)
;;; Outputs: [_SoundInfo] data for that ID altered to stop sound output
;;; Calls:   NONE
proc _StopSound
     .SoundID        arg       4
     .HowStop        arg       4
     pusha

     mov al, [ebp+.SoundID]    ; Move sound ID to be found into al

     mov ebx, _SoundInfo       ; Starting address to seek from
     mov edi, 0
.checkloop
     cmp dword [ebx+edi+8], -1     ; If current position of sound is -1, it's not playing
     je .notsound
     cmp [ebx+edi+13], al          ; Compare sound ID
     jne .notsound
     ;;; was correct sound, is playing
     cmp byte [ebp+.HowStop], SOUND_KILL
     jne .soundend
     mov dword [ebx+edi+8], -1
     jmp .quit
.soundend
     and byte[ebx+edi+12], 07Fh
     jmp .quit

.notsound
     add edi, 14
     cmp edi, 14*7
     jb .checkloop

.quit
     popa
     ret
endproc


;;; Function:  KillAllSound
;;; Purpose:   To stop all sound output immediately.  I do this by clearing the buffer and setting all sounds to not play further.
;;; Inputs:    NONE
;;; Outputs:   Changes first dword of [_SoundTrack] to -1, ending soundtrack playback
;;;            Changes 'current location' dword of each sound channel to -1, ending play
;;; Calls:     NONE
_KillAllSound
     pusha

     call _StopSoundTrack

     mov ecx, 7
     mov edi, 0
.StopAllTracksLoop
     mov dword [_SoundInfo+edi+8], -1
     add edi, 14
     loop .StopAllTracksLoop

     ;;; Clear DMA memory block to zero
     mov ecx, DMABUFFSIZE
     shr ecx, 2
     mov es, [DMASel]
     mov edi, 0
.clearbuffer
     mov dword [es:edi], 00000000h
     add edi, 4
     dec ecx
     jnz .clearbuffer

     popa
     ret

;;; Function:  StopSoundTrack
;;; Purpose:   To remove all soundtracks from the playlist, stopping their playback
;;; Inputs:    NONE
;;; Outputs:   Changes first dword of [_SoundTrack] to -1, ending soundtrack playback
;;; Calls:     NONE
_StopSoundTrack
     mov dword [_SoundTrack], -1
     ret


;;; Function:  LoadSound
;;; Purpose:   To load a sound from a raw file (16 bit mono, 11,025 hz), storing pointer to it's allocated memory in .SoundPtr
;;; Inputs:
;;;            .SoundPtr, points to buffer
;;;            .SoundLen, length in bytes of sound in memory
;;;            .SoundNamePtr, pointer to string with sound name
;;; Outputs:   Stores in .SoundPtr buffer start location
;;; Calls:     _AllocMem, _OpenFile, _ReadFile, _Closefile
proc _LoadSound
       .SoundPtr       arg     4
       .SoundLen       arg     4
       .SoundNamePtr   arg     4

        pusha

	invoke	_AllocMem, dword [ebp+.SoundLen]
	mov ebx, [ebp+.SoundPtr]
	mov [ebx], eax
	invoke  _OpenFile, dword [ebp+.SoundNamePtr], word 0
	push eax
        mov ebx, [ebp+.SoundPtr]
	invoke	_ReadFile, dword eax, dword [ebx], dword [ebp+.SoundLen]
	pop	eax
	invoke	_CloseFile, dword eax

        popa

        ret
endproc

;;; Function:  SoundIsPlaying
;;; Purpose:   To see if a sound ID is playing, returning 0 in eax if not, 1 if so.
;;; Inputs:
;;;            al, Sound ID to be checked
;;; Outputs:   eax is 1 if it is playing, 0 if not
;;; Calls:     NONE
_SoundIsPlaying
     pusha

     mov ebx, _SoundInfo
     mov edi, 0
.checkloop
     cmp dword [ebx+edi+8], -1
     je .notsound
     cmp [ebx+edi+13], al      ; Compare sound ID
     jne .notsound
     ;;; was correct sound, is playing
     mov eax, 1      ; Sound found
     jmp .quit

.notsound
     add edi, 14
     cmp edi, 14*7
     jb .checkloop

     mov eax, 0      ; Sound not found
.quit

     popa
     ret
     
	
;;; Function:  AddSound
;;; Purpose:   To Add sound to first free channel found (noted by -1 value in 'currently playing' pointer).
;;;               if sound has not yet been initialized, do so.
;;; Inputs:    [_ISR_Running], so we know if a sound is already being played
;;;			.SoundPtr, points at beginning of sound in memory
;;;			.SoundLen, length in bytes of sound in memory
;;;			.SoundLoop, 0 if no loop, 80h if loop
;;;                     .SoundID, used to stop a sound, must be unique from all sounds that reuire stopping
;;; Outputs:   [_SoundInfo] Updated with new sound information
;;; Calls:     _DMA_Allocate_Mem, _DMA_Lock_Mem, _SB16_Init, _SB16_SetFormat, _SB16_SetMixers, _DMA_Start, _SB16_Start, _LibExit
proc _AddSound
	.SoundPtr	arg	4
	.SoundLen	arg	4
	.SoundInf	arg	4
	.SoundID        arg     4

        xor ecx, ecx
.FindSoundInfoSlot
	cmp word [_SoundInfo+ecx+8], -1
	je .OpenSlot
	add ecx, 14
	jmp .FindSoundInfoSlot
	
.OpenSlot
	mov edx, [ebp+.SoundPtr]
	mov [_SoundInfo+ecx], edx
	
	mov [_SoundInfo+ecx+8], edx			; Store pointer to where 2nd buffer copy should start in current copy location
	
	mov edx, [ebp+.SoundLen]
	mov [_SoundInfo+ecx+4], edx
	
	mov dl, [ebp+.SoundInf]
	mov [_SoundInfo+ecx+12], dl

        mov dl, [ebp+.SoundID]
        mov [_SoundInfo+ecx+13], dl

	push ecx

	cmp byte [_ISR_Running], 1
	je near .quit
	
	invoke	_DMA_Allocate_Mem, dword DMABUFFSIZE, dword DMASel, dword DMAAddr
	cmp	[DMASel], word 0
	je	near .error
	
	invoke	_DMA_Lock_Mem		; Lock the memory that was allocated
	
	;;; Clear DMA memory block to zero
	mov ecx, DMABUFFSIZE
	shr ecx, 2
	mov es, [DMASel]
	mov edi, 0
.clearbuffer
	mov dword [es:edi], 00000000h
	add edi, 4
	dec ecx
	jnz .clearbuffer
	;;; DONE
	
	;;; Check to see if sound is less than buffer size
	cmp dword [ebp+.SoundLen], DMABUFFSIZE		; If sound is larger than one buffer in length, don't set rest of buffer to zero
	ja near .overbuffer
	
		test byte [ebp+.SoundInf], 80h					; See if we were supposed to loop
		jnz near .looping
	;;; NOT LOOPING
	;;; Now copy to the dma memory location the values of sin8
		mov	ecx, [ebp+.SoundLen]	; Store Sound Length in ecx
		shr ecx, 3					; Divide sound length by four
		mov	edi, 0
		mov	esi, [ebp+.SoundPtr]
	.notloopingcopy
		movq mm0, [esi]
		psraw mm0, 3
		movq [es:edi], mm0
		add esi, 8
		add edi, 8
		dec ecx
		jnz .notloopingcopy
	;;; DONE
	;;; Set current location to -1
		emms
		pop ecx
		mov dword [_SoundInfo+ecx+8], -1
	;;; DONE
		jmp .initialize
		
	.looping
		mov esi, [ebp+.SoundPtr]
		mov edx, [ebp+.SoundLen]
		xor ecx, ecx					; Counts into source, to see if we are overstepping source
		xor edi, edi					; Destination starts at zero, can compare later to DMABUFFSIze that way
	.copylooping
		movq mm0, [esi]
		psraw mm0, 3
		movq  [es:edi], mm0
		add esi, 8
		add ecx, 8
		add edi, 8
		cmp edi, DMABUFFSIZE
		jae .donecopy
		cmp ecx, edx					; See if we are now over the source copy region
		jb .copylooping
		mov esi, [ebp+.SoundPtr]
		xor ecx, ecx
		jmp .copylooping
	.donecopy
		emms
		pop ecx
		mov dword [_SoundInfo+8+ecx], esi	; Copy end of looping location on buffer to memory so we can resume from there
		jmp .initialize
.overbuffer
		mov	ecx, DMABUFFSIZE/8	; Store Sound Length in ecx
		mov	edi, 0
		mov	esi, [ebp+.SoundPtr]	
	.copyover
		movq mm0, [esi]
		psraw mm0, 3
		movq [es:edi], mm0
		add esi, 8
		add edi, 8
		dec ecx
		jnz .copyover
	;;; DONE
		emms
		pop ecx
		mov [_SoundInfo+8+ecx], esi
		
.initialize
	
	;;; Initialize SB
	invoke	_SB16_Init, dword _SoundISR
	test	eax, eax
	jnz	near .error
	
	;;; Set DMA channel for 16 bit
	mov byte [DMAChan], 5
	
	;;; Set sb to 11 khz 16 bit sound - mono
	invoke	_SB16_SetFormat, dword 16, dword 11025, dword 0
	test	eax, eax
	jnz	near .error
	
	;;; Set mixer to correct values
	invoke	_SB16_SetMixers, word 07fh, word 07fh, word 07fh, word 07fh
	test	eax, eax
	jnz	near .error

	;;; DMA start
	movzx	eax, byte [DMAChan]
	invoke	_DMA_Start, eax, dword [DMAAddr], dword DMABUFFSIZE, dword 1, dword 1

	;;; Start SB16
	invoke	_SB16_Start, dword DMABUFFSIZE/4, dword 1, dword 1
	test	eax, eax
	jnz	near .error

	mov byte [_ISR_Running], 1
	
	mov byte [_DMAOffset], 0				; Offset into buffer for first interrupt (zero implies first half of buffer will be filled on first interrupt
.quit
        
	ret

.error
        
	mov	ax, 10
	call	_LibExit
	ret
endproc

;;; Function:  _SoundBufferUpdate
;;; Purpose:   If _SOUNDISRTRIGGERED is one, then new data needs to be copied to the buffer.  
;;;               This function should be put in any major loops through which sound needs to be played.
;;; Inputs:    [_SoundInfo], Contains sound data for seven channels of sound
;;;			[_SoundTrack], Contains sound data for up to ten sound clips composing the soundtrack
;;;			DMABUFFSIZE, size of the dma buffer declared
;;;			[DMASel], offset at which the dma buffer starts
;;;			[_DMAOffset], 0 if need to write to first half of buffer, 1 if second half
;;; Outputs:   Updates half buffer with streaming sound data, mixes the seven channels and soundtrack
;;; Calls:     _DMA_Stop, _SB16_Stop, _SB16_SetMixers, _SB16_Exit
_SoundBufferUpdate

        pusha

        cmp dword [_SOUNDISRTRIGGERED], 1
	jne near .notready
	
	mov byte [_SOUNDFOUND], 0		; Will tell me if there were any channels found
	
	;;; Set EDI according to call number (even or odd)
	xor edi, edi
	cmp byte [_DMAOffset], 1
	jne .noadd
	add edi, DMABUFFSIZE/2			; If in second half of buffer (int at zero), add 1/2 buffer size to edi
.noadd
	
	push edi						; Preserve starting point of copy to DMA buffer
	
	;;; Clear DMA memory block to zero
	mov ecx, DMABUFFSIZE/2
	shr ecx, 2
	mov es, [DMASel]
.clearbuffer
	mov dword [es:edi], 00000000h
	add edi, 4
	dec ecx
	jnz .clearbuffer
	;;; DONE
	
	pop edi							; Restore starting point of copy to DMA buffer
	
	mov ecx, -14
.checkSounds
	add ecx, 14
	cmp ecx, 14*3
	jae .donechecking
	cmp dword [_SoundInfo+ecx+8], -1
	jne .SoundFound
	jmp .checkSounds
.donechecking
	cmp dword [_SoundTrack], -1
	jne near .SoundTrack
	cmp byte [_SOUNDFOUND], 1
	je near .quit
	jmp .EndSound					; If no soundtrack and no sound found, quit
	
.SoundFound
	;;; Set EDI according to call number (even or odd)
	xor edi, edi
	cmp byte [_DMAOffset], 1
	jne .noadd3
	add edi, DMABUFFSIZE/2			; If in second half of buffer (int at zero), add 1/2 buffer size to edi
.noadd3

	mov byte [_SOUNDFOUND], 1
	mov es, [DMASel]
	mov ebx, ecx
	;;; Need to now find length of remaining sound to be played
		mov eax, [_SoundInfo+ecx]		; Eax gains base address of sound
		mov edx, [_SoundInfo+ecx+4]		; Edx gains length of sound
		mov esi, [_SoundInfo+ecx+8]		; Esi gains current pointer to buffer copy start location
		mov ebx, edx					; ebx will be number of bytes left to copy
		sub ebx, esi
		add ebx, eax
	;;; DONE
	
	cmp ebx, DMABUFFSIZE/2
	jae near .fillsbuff
		test byte [_SoundInfo+ecx+12], 80h
		jnz .repeating
		mov edx, ebx					; Edx is number of bytes left to copy
		mov eax, ebx
		;;; Copy over what sound file remains
		
		shr eax, 3						; 8 bytes per copy
	.copynorepeat
		movq mm0, [esi]
		movq mm1, [es:edi]
		psraw mm0, 3
		paddsw mm0, mm1
		movq [es:edi], mm0
		add esi, 8
		add edi, 8
		dec eax
		jnz .copynorepeat
		;;; DONE
		emms
		mov dword [_SoundInfo+8+ecx], -1	; Set sound file to done
		jmp .checkSounds

	.repeating
		add edx, eax
		mov ebx, edi
		add ebx, DMABUFFSIZE/2
	.copylooping
		movq mm0, [esi]
		movq mm1, [es:edi]
		psraw mm0, 3
		paddsw mm0, mm1
		movq [es:edi], mm0
		add esi, 8
		add edi, 8
		cmp edi, ebx
		jae .donecopy
		cmp esi, edx
		jb .copylooping
		mov esi, [_SoundInfo+ecx];eax
		jmp .copylooping
	.donecopy
		emms
		mov dword [_SoundInfo+8+ecx], esi	; Copy end of looping location on buffer to memory so we can resume from there
		jmp .checkSounds

.fillsbuff
	mov eax, DMABUFFSIZE/16				; Since sound fills buffer, set copy count to buffer size
.fillbuff
	movq mm0, [esi]
	movq mm1, [es:edi]
	psraw mm0, 3
	paddsw mm0, mm1
	movq [es:edi], mm0
	add edi, 8
	add esi, 8
	dec eax
	jnz .fillbuff
	
	emms
	mov [_SoundInfo+ecx+8], esi
	
	jmp .checkSounds

.SoundTrack
	mov es, [DMASel]
	;;; Set EDI according to call number (even or odd)
	xor edi, edi
	cmp byte [_DMAOffset], 1
	jne .noadd2
	add edi, DMABUFFSIZE/2			; If in second half of buffer (int at zero), add 1/2 buffer size to edi
.noadd2

	mov ebx, [_SoundTrack]				; ebx will be ptr offset into Soundtrack
	shl ebx, 4							; 16 bytes per id
	add ebx, 4							; add four bytes
	
	mov eax, [_SoundTrack+ebx]
	mov edx, [_SoundTrack+ebx+4]
	mov esi, [_SoundTrack+ebx+8]

	mov ecx, edx						; Calculate number of bytes left to copy
	sub ecx, esi
	add ecx, eax
	
	cmp ecx, DMABUFFSIZE/2
	jae .soundfillsbuff
	;;; Doesn't fill buffer
	shr ecx, 3
.copysoundnorepeat
	movq mm0, [esi]
	movq mm1, [es:edi]
	psraw mm0, 3
	paddsw mm0, mm1
	movq [es:edi], mm0
	
	add esi, 8
	add edi, 8
	dec ecx
	jnz .copysoundnorepeat
	;;; DONE
	emms
	mov eax, [_SoundTrack+ebx]
	mov [_SoundTrack+ebx+8], eax 
	mov eax, [_SoundTrack+ebx+12]
	mov [_SoundTrack], eax
	jmp .quit
	
.soundfillsbuff
	mov ecx, DMABUFFSIZE/16
.copysoundfills
	movq mm0, [esi]
	movq mm1, [es:edi]
	psraw mm0, 3
	paddsw mm0, mm1
	movq [es:edi], mm0
	
	add esi, 8
	add edi, 8
	dec ecx
	jnz .copysoundfills
	
	emms
	mov dword [_SoundTrack+ebx+8], esi
	jmp .quit

.EndSound
		;;; End autoinit
		;invoke	_SB16_Start, dword DMABUFFSIZE*2, dword 0, dword 1
		;test	eax, eax
		;jnz	near .error

		movzx	eax, byte [DMAChan]
		invoke	_DMA_Stop, eax
		; DMA_Stop doesn't report error

		invoke	_SB16_Stop
		; SB16_Stop doesn't report error

		;;; Set mixer volume back down to zero
		invoke	_SB16_SetMixers, word 0, word 0, word 0, word 0
		;test	eax, eax
		;jnz	near .error

		;;; De-initialize sb16
		invoke	_SB16_Exit
		;test	eax, eax
		;jnz	near .error
		
		mov byte [_ISR_Running], 0
.quit
	mov dword [_SOUNDISRTRIGGERED], 0

.notready

        popa
	ret



;;; Function:  AddSoundTrack
;;; Purpose:   To add a soundtrack to the _SoundTrack data at given index, such that
;;;               after it's completion the index of .NextSound will be played
;;; Inputs:    [_ISR_Running], so we know if a sound is already being played
;;;			.SoundPtr, points at beginning of sound in memory
;;;			.SoundLen, length in bytes of sound in memory
;;;			.NextSound, index of sound to play after this one completes
;;;			.ThisSound, index this sound is to be stored in within index
;;; Outputs:   [_SoundTrack] Updated with new sound information
;;; Calls:     _DMA_Lock_Mem, _SB16_Init, _SB16_SetFormat, _SB16_SetMixers, _DMA_Start, _SB16_Start, _LibExit
proc _AddSoundTrack
	.SoundPtr	arg	4
	.SoundLen	arg	4
	.NextSound	arg	4
	.ThisSound	arg     4
	pusha

 	cmp byte [_ISR_Running], 1
	je .noinit

	invoke	_DMA_Allocate_Mem, dword DMABUFFSIZE, dword DMASel, dword DMAAddr
	cmp	[DMASel], word 0
	je	near .error

	invoke	_DMA_Lock_Mem		; Lock the memory that was allocated

        ;;; Clear DMA memory block to zero
	mov ecx, DMABUFFSIZE
	shr ecx, 2
	mov es, [DMASel]
	mov edi, 0
.clearbuffer
	mov dword [es:edi], 00000000h
	add edi, 4
	dec ecx
	jnz .clearbuffer
	;;; DONE
.noinit

	mov edi, [ebp+.ThisSound]
	shl edi, 4
	add edi, 4
	
	cmp dword [_SoundTrack], -1
	jne .notfirstsoundtrack
	mov dword [_SoundTrack], 0
	mov edi, [_SoundTrack]
	add edi, 4	
.notfirstsoundtrack
	
	mov eax, [ebp+.SoundPtr]
	mov [_SoundTrack+edi], eax
	mov [_SoundTrack+edi+8], eax
	
	mov eax, [ebp+.SoundLen]
	mov [_SoundTrack+edi+4], eax
	
	mov eax, [ebp+.NextSound]
	mov [_SoundTrack+edi+12], eax

        cmp byte [_ISR_Running], 1
        je near .quit
        
.initialize

	;;; Initialize SB
	invoke	_SB16_Init, dword _SoundISR
	test	eax, eax
	jnz	near .error
	
	;;; Set DMA channel for 16 bit
	mov byte [DMAChan], 5
	
	;;; Set sb to 11 khz 16 bit sound - mono
	invoke	_SB16_SetFormat, dword 16, dword 11025, dword 0 
	test	eax, eax
	jnz	near .error
	
	;;; Set mixer to correct values
	invoke	_SB16_SetMixers, word 07fh, word 07fh, word 07fh, word 07fh
	test	eax, eax
	jnz	near .error

	;;; DMA start
	movzx	eax, byte [DMAChan]
	invoke	_DMA_Start, eax, dword [DMAAddr], dword DMABUFFSIZE, dword 1, dword 1

	;;; Start SB16
	invoke	_SB16_Start, dword DMABUFFSIZE/4, dword 1, dword 1
	test	eax, eax
	jnz	near .error

	mov byte [_ISR_Running], 1
	
	mov byte [_DMAOffset], 0				; Offset into buffer for first interrupt (zero implies first half of buffer will be filled on first interrupt
.quit
	popa
	ret

.error
        popa
	mov	ax, 10
	call	_LibExit
	ret
endproc






;;; Function:  _SoundISR
;;; Purpose:   In event that callback ISR is called (half buffer completed), toggle _DMAOffset 
;;;               (to determine which half of buffer to copy to), set _SOUNDISRTRIGGERED to 1 so _SoundBufferUpdate knows
;;; Inputs:		[_DMAOffset], will be toggled (1 to 0, or 0 to 1)
;;; Outputs:   [_SOUNDISRTRIGGERED], set to one, signaling to the main loop that new data needs to be copied
;;;            [_DMAOffset], toggled
;;; Calls:     NONE
_SoundISR
	xor byte [_DMAOffset], 1
	mov dword [_SOUNDISRTRIGGERED], 1
	ret


