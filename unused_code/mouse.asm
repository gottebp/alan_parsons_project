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
	
        GLOBAL _InstallMouse, _RemoveMouse, _MOUSE_X, _MOUSE_Y, _MOUSE_LBUTTON

SECTION .bss

        mouse_seg          resw    1
        mouse_off          resw    1


SECTION .data
        _MOUSE_X           dw      0
        _MOUSE_Y           dw      0
        _MOUSE_LBUTTON     dw      0


;====== InstallKeyboard ===================================================
;;; _InstallKeyboard
;;;   INPUTS: None
;;;  OUTPUTS: None
;;;  PURPOSE: Installs a keyboard handler for detecting keypresses
_InstallMouse
        call _LibInit
        invoke  _LockArea, ds, dword _MOUSE_LBUTTON, dword 2
        invoke  _LockArea, cs, dword MouseCallback, dword MouseCallback_end-MouseCallback

        xor     eax, eax
        int     33h

        invoke  _Get_RMCB, dword mouse_seg, dword mouse_off, dword MouseCallback, dword 1
        cmp     eax, 0
        jnz     near .error

      	mov	dword [DPMI_EAX], 0Ch
	mov	dword [DPMI_ECX], 7Fh
	xor     edx, edx
        mov     dx, [mouse_off]
	mov	[DPMI_EDX], edx
      	mov     ax, [mouse_seg]
	mov	[DPMI_ES], ax
        mov     bx, 33h
        call    DPMI_Int

        mov word[_MOUSE_X], SCREEN_WIDTH / 2
        mov word[_MOUSE_Y], SCREEN_HEIGHT / 2        

.error

        call    _LibExit
	ret


;====== RemoveKeyboard ====================================================
;;;
;;; _RemoveKeyboard
;;;   INPUTS: None
;;;  OUTPUTS: None
;;;  PURPOSE: Uninstalls the keyboard handler
_RemoveMouse:
      	mov	dword [DPMI_EAX], 0Ch
	xor     edx, edx
	mov	[DPMI_ECX], edx
	mov	[DPMI_EDX], edx
	mov	[DPMI_ES], dx
        mov     bx, 33h
        call    DPMI_Int

        invoke  _Free_RMCB, word [mouse_seg], word [mouse_off]
	ret


;====== KeyboardISR =======================================================
;;;
;;; _KeyboardISR
;;;   INPUTS: NONE
;;;  OUTPUTS: _KEYBOARD array is updated with scancode entered
;;;  PURPOSE: Recieve keyboard input (Multiple keys too. None of this one key at a time crap)
proc MouseCallback
.DPMIRegsPtr	arg     4

        push    esi
        mov     esi, [ebp+.DPMIRegsPtr]

        mov     eax, [esi+DPMI_EDX_off]
        mov     [_MOUSE_Y], ax
        
        mov     eax, [esi+DPMI_ECX_off]
        mov     [_MOUSE_X], ax

        mov     eax, [esi+DPMI_EBX_off]
        mov     [_MOUSE_LBUTTON], ax

        pop     esi

	ret
endproc
MouseCallback_end


