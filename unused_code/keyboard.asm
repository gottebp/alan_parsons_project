
;============================================================
; Project:      "The Alan Parsons Project"
; Description:  Code for ece291 final project
; Author:       Benjamin Gottemoller
; Website:      http://www.particlefield.com
; Date:		7/20/02
;============================================================

%include "lib291.inc"
%include "defs.inc"
%include "ppe.inc"
%include "rand.inc"
%include "mapeng.inc"
%include "sse_mem.inc"
%include "macros.inc"

	BITS	32
	GLOBAL _main

        GLOBAL _InstallKeyboard, _RemoveKeyboard
        GLOBAL _kbINT, _kbPort, _kbIRQ, _KEYBOARD


SECTION .bss

        _kbINT		   resb	   1
        _kbIRQ		   resb	   1
        _kbPort		   resw	   1

        _KEYBOARD          resb    256


SECTION .data


;====== InstallKeyboard ===================================================
;;; _InstallKeyboard
;;;   INPUTS: None
;;;  OUTPUTS: None
;;;  PURPOSE: Installs a keyboard handler for detecting keypresses
;;;    CALLS: _LockArea, _Install_Int
_InstallKeyboard

	pusha

        xor esi, esi
.ClearKeyData:
        mov byte[_KEYBOARD + esi], 0
        inc esi
        cmp esi, 256
        jb .ClearKeyData

        invoke _LockArea, cs, dword _KeyboardISR, dword _KeyboardISR_end - _KeyboardISR
        invoke _LockArea, ds, dword _KEYBOARD, dword 256
	xor eax, eax
        mov al, byte[_kbINT]
        invoke _Install_Int, eax, dword _KeyboardISR
        popa

	ret


;====== RemoveKeyboard ====================================================
;;;
;;; _RemoveKeyboard
;;;   INPUTS: None
;;;  OUTPUTS: None
;;;  PURPOSE: Uninstalls the keyboard handler
;;;  CALLS: _Remove_Int
_RemoveKeyboard:

        pusha
        mov eax, dword[_kbINT]
        invoke _Remove_Int, eax
        popa
	ret


;====== KeyboardISR =======================================================
;;;
;;; _KeyboardISR
;;;   INPUTS: NONE
;;;  OUTPUTS: _KEYBOARD array is updated with scancode entered
;;;  PURPOSE: Recieve keyboard input (Multiple keys too. None of this one key at a time crap)
_KeyboardISR

        push    ds
        push    dx
        xor eax, eax

        mov     dx, word[_kbPort]
        in      al, dx

        test al, 80h
        jnz .KeyUp

.KeyDown:

        mov byte[_KEYBOARD + eax], 1
        jmp .Done

.KeyUp:
        and al, 7Fh
        mov byte[_KEYBOARD + eax], 0

.Done:
        mov     al, 20h
        out     20h, al

        pop     dx
        pop     ds
        mov     eax, 1

        ret
_KeyboardISR_end



