
%ifndef _MACROS_
%define _MACROS_

%macro CEXTERN 1
    EXTERN _%1
    %define %1 _%1
%endmacro

;Computes an alpha value for 1 pixel
;%{1} = alpha composition of %{2} and %{3}
%macro ComputeAlpha 3

        pxor mm1, mm1
        movd mm0, %{2}
        punpcklbw mm0, mm1
        movq mm2, mm0
        punpckhwd mm2, mm2
        punpckhdq mm2, mm2               ;mm2 now holds the alpha values
        pmullw mm0, mm2
        movq mm3, qword[_RoundingFactor]
        paddusw mm0, mm3
        psrlw mm0, 8                     ;divide by 256
        movd mm4, %{3}
        punpcklbw mm4, mm1
        paddusw mm0, mm4
        pmullw mm4, mm2
        paddusw mm4, mm3
        psrlw mm4, 8
        psubusw mm0, mm4
        packuswb mm0, mm0
        movd %{1}, mm0

%endmacro

;Computes an alpha value for 1 pixel
;%{1} = alpha composition of %{2} and %{3}
%macro QComputeAlpha 3

        pxor mm1, mm1
        movq mm0, %{2}
        punpcklbw mm0, mm1
        movq mm2, mm0
        punpckhwd mm2, mm2
        punpckhdq mm2, mm2               ;mm2 now holds the alpha values
        pmullw mm0, mm2
        movq mm3, qword[_RoundingFactor]
        paddusw mm0, mm3
        psrlw mm0, 8                     ;divide by 256
        movq mm4, %{3}
        punpcklbw mm4, mm1
        paddusw mm0, mm4
        pmullw mm4, mm2
        paddusw mm4, mm3
        psrlw mm4, 8
        psubusw mm0, mm4
        packuswb mm0, mm0
        movq %{1}, mm0

%endmacro

%macro invoke 1
        push ebp

        mov ebp, esp
        sub ebp, 4
        call %{1}

        pop ebp
%endmacro

%macro invoke 2
        push ebp

        push %{2}
        mov ebp, esp
        sub ebp, 4
        call %{1}
        add esp, 4

        pop ebp
%endmacro

%macro invoke 3
        push ebp

        push %{3}
        push %{2}
        mov ebp, esp
        sub ebp, 4
        call %{1}
        add esp, 8

        pop ebp
%endmacro

%macro invoke 4
        push ebp

        push %{4}
        push %{3}
        push %{2}
        mov ebp, esp
        sub ebp, 4
        call %{1}
        add esp, 12

        pop ebp
%endmacro

%macro invoke 5
        push ebp

        push %{5}
        push %{4}
        push %{3}
        push %{2}
        mov ebp, esp
        sub ebp, 4
        call %{1}
        add esp, 16

        pop ebp
%endmacro

%macro invoke 6
        push ebp

        push %{6}
        push %{5}
        push %{4}
        push %{3}
        push %{2}
        mov ebp, esp
        sub ebp, 4
        call %{1}
        add esp, 20

        pop ebp
%endmacro

%macro invoke 7
        push ebp

        push %{7}
        push %{6}
        push %{5}
        push %{4}
        push %{3}
        push %{2}
        mov ebp, esp
        sub ebp, 4
        call %{1}
        add esp, 24

        pop ebp
%endmacro

%macro invoke 8
        push ebp

        push %{8}
        push %{7}
        push %{6}
        push %{5}
        push %{4}
        push %{3}
        push %{2}
        mov ebp, esp
        sub ebp, 4
        call %{1}
        add esp, 28

        pop ebp
%endmacro

%macro invoke 9
        push ebp

        push %{9}
        push %{8}
        push %{7}
        push %{6}
        push %{5}
        push %{4}
        push %{3}
        push %{2}
        mov ebp, esp
        sub ebp, 4
        call %{1}
        add esp, 32

        pop ebp
%endmacro

%macro invoke 10
        push ebp

        push %{10}
        push %{9}
        push %{8}
        push %{7}
        push %{6}
        push %{5}
        push %{4}
        push %{3}
        push %{2}
        mov ebp, esp
        sub ebp, 4
        call %{1}
        add esp, 36

        pop ebp
%endmacro

%macro invoke 11
        push ebp

        push %{11}
        push %{10}
        push %{9}
        push %{8}
        push %{7}
        push %{6}
        push %{5}
        push %{4}
        push %{3}
        push %{2}
        mov ebp, esp
        sub ebp, 4
        call %{1}
        add esp, 40

        pop ebp
%endmacro

%macro invoke 12
        push ebp

        push %{12}
        push %{11}
        push %{10}
        push %{9}
        push %{8}
        push %{7}
        push %{6}
        push %{5}
        push %{4}
        push %{3}
        push %{2}
        mov ebp, esp
        sub ebp, 4
        call %{1}
        add esp, 44

        pop ebp
%endmacro

%endif


