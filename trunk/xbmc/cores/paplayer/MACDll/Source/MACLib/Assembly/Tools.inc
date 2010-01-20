;
; (C) Ururi 1999
;

BITS 32

%ifdef WIN32
        %define _NAMING
        %define segment_code segment .text align=32 class=CODE use32
        %define segment_data segment .data align=32 class=DATA use32
  %ifdef __BORLANDC__
        %define segment_bss  segment .data align=32 class=DATA use32
  %else
        %define segment_bss  segment .bss  align=32 class=DATA use32
  %endif

%elifdef AOUT
        %define _NAMING
        %define segment_code segment .text
        %define segment_data segment .data
        %define segment_bss  segment .bss

%else
        %define segment_code segment .text align=32 class=CODE use32
        %define segment_data segment .data align=32 class=DATA use32
        %define segment_bss  segment .bss  align=32 class=DATA use32
%endif

%define pmov    movq
%define pmovd   movd

%define pupldq  punpckldq
%define puphdq  punpckhdq
%define puplwd  punpcklwd
%define puphwd  punpckhwd

%imacro globaldef 1
        %ifdef _NAMING
                %define %1 _%1
        %endif
        global %1
%endmacro

%imacro externdef 1
        %ifdef _NAMING
                %define %1 _%1
        %endif
        extern %1
%endmacro

%imacro proc 1
        %push   proc
        global  _%1
        global  %1
_%1:
%1:
        %assign %$STACK  0
        %assign %$STACKN 0
        %assign %$ARG    4
%endmacro

%imacro endproc 0
        %ifnctx proc
                %error expected 'proc' before 'endproc'.
        %else
                %if %$STACK > 0
                        add esp, %$STACK
                %endif

                %if %$STACK <> (-%$STACKN)
                        %error STACKLEVEL mismatch check 'local', 'alloc', 'pushd', 'popd'
                %endif

                ret
                %pop
        %endif
%endmacro

%idefine sp(a) esp+%$STACK+a

%imacro arg 1
        %00     equ %$ARG
        %assign %$ARG %$ARG+%1
%endmacro

%imacro local 1
        %assign %$STACKN %$STACKN-%1
        %00 equ %$STACKN
%endmacro

%imacro alloc 0
        sub esp, (-%$STACKN)-%$STACK
        %assign %$STACK (-%$STACKN)
%endmacro

%imacro pushd 1-*
        %rep %0
                push %1
                %assign %$STACK %$STACK+4
        %rotate 1
        %endrep
%endmacro

%imacro popd 1-*
        %rep %0
        %rotate -1
                pop %1
                %assign %$STACK %$STACK-4
        %endrep
%endmacro

%macro  algn    1
        align   16
        %rep    (65536-%1) & 15
        nop
        %endrep
%endm
