
;	Copyright (C) 1999 URURI

;	nasm�ѥޥ���
;	1999/08/21 ���
;	1999/10/10 ���Ĥ��ɲ�
;	1999/10/27 aout�б�
;	1999/11/07 pushf, popf ��NASM�ΥХ��б�
;	1999/12/02 for BCC ( Thanks to Miquel )

; for Windows Visual C++        -> define WIN32
;             Borland or cygwin ->        WIN32 and COFF
; for FreeBSD 2.x               ->        AOUT
; for TownsOS                   ->        __tos__
; otherwise                     ->   none

;̾����դ���

BITS 32

%ifdef YASM
	%define segment_code segment .text align=16 use32
	%define segment_data segment .data align=16 use32
	%define segment_bss  segment .bss align=16 use32
%elifdef WIN32
	%define segment_code segment .text align=16 class=CODE use32
	%define segment_data segment .data align=16 class=DATA use32
%ifdef __BORLANDC__
	%define segment_bss  segment .data align=16 class=DATA use32
%else
	%define segment_bss  segment .bss align=16 class=DATA use32
%endif
%elifdef AOUT
	%define _NAMING
	%define segment_code segment .text
	%define segment_data segment .data
	%define segment_bss  segment .bss
%else
%ifidn __OUTPUT_FORMAT__,elf
	section .note.GNU-stack progbits noalloc noexec nowrite align=1
%endif
	%define segment_code segment .text align=16 class=CODE use32
	%define segment_data segment .data align=16 class=DATA use32
	%define segment_bss  segment .bss align=16 class=DATA use32
%endif

%ifdef WIN32
	%define _NAMING
%endif

%ifdef __tos__
group CGROUP text
group DGROUP data
%endif

;ñ�����ư��������

%idefine float dword
%idefine fsize 4
%idefine fsizen(a) (fsize*(a))

;��ɷ��

%idefine wsize 2
%idefine wsizen(a) (wsize*(a))
%idefine dwsize 4
%idefine dwsizen(a) (dwsize*(a))

;REG

%define r0 eax
%define r1 ebx
%define r2 ecx
%define r3 edx
%define r4 esi
%define r5 edi
%define r6 ebp
%define r7 esp

;MMX,3DNow!,SSE

%define pmov	movq
%define pmovd	movd

%define pupldq	punpckldq
%define puphdq	punpckhdq
%define puplwd	punpcklwd
%define puphwd	punpckhwd

%define xm0 xmm0
%define xm1 xmm1
%define xm2 xmm2
%define xm3 xmm3
%define xm4 xmm4
%define xm5 xmm5
%define xm6 xmm6
%define xm7 xmm7

;�����åե��Ѥ�4�ʥޥ���

%define R4(a,b,c,d) (a*64+b*16+c*4+d)

;C�饤���ʴʰץޥ���

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
	%push	proc
	%ifdef _NAMING
		global _%1
	%else
		global %1
	%endif

	align 32
%1:
_%1:

	%assign %$STACK 0
	%assign %$STACKN 0
	%assign %$ARG 4
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
	%00	equ %$ARG
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

; bug of NASM-0.98
%define pushf db 0x66, 0x9C
%define popf  db 0x66, 0x9D

%define	ge16(n)		((((n) / 16)*0xFFFFFFFF) & 0xFFFFFFFF)
%define	ge15(n)		((((n) / 15)*0xFFFFFFFF) & 0xFFFFFFFF)
%define	ge14(n)		((((n) / 14)*0xFFFFFFFF) & 0xFFFFFFFF)
%define	ge13(n)		((((n) / 13)*0xFFFFFFFF) & 0xFFFFFFFF)
%define	ge12(n)		((((n) / 12)*0xFFFFFFFF) & 0xFFFFFFFF)
%define	ge11(n)		((((n) / 11)*0xFFFFFFFF) & 0xFFFFFFFF)
%define	ge10(n)		((((n) / 10)*0xFFFFFFFF) & 0xFFFFFFFF)
%define	ge9(n)		((((n) /  9)*0xFFFFFFFF) & 0xFFFFFFFF)
%define	ge8(n)		(ge9(n) | ((((n) /  8)*0xFFFFFFFF) & 0xFFFFFFFF))
%define	ge7(n)		(ge9(n) | ((((n) /  7)*0xFFFFFFFF) & 0xFFFFFFFF))
%define	ge6(n)		(ge9(n) | ((((n) /  6)*0xFFFFFFFF) & 0xFFFFFFFF))
%define	ge5(n)		(ge9(n) | ((((n) /  5)*0xFFFFFFFF) & 0xFFFFFFFF))
%define	ge4(n)		(ge5(n) | ((((n) /  4)*0xFFFFFFFF) & 0xFFFFFFFF))
%define	ge3(n)		(ge5(n) | ((((n) /  3)*0xFFFFFFFF) & 0xFFFFFFFF))
%define	ge2(n)		(ge3(n) | ((((n) /  2)*0xFFFFFFFF) & 0xFFFFFFFF))
%define	ge1(n)		(ge2(n) | ((((n) /  1)*0xFFFFFFFF) & 0xFFFFFFFF))

; macro to align for begining of loop
; %1   does not align if it LE bytes to next alignment 
;      4..16 
;      default is 12

%imacro	loopalignK6	0-1 12 
%%here:
	times (($$-%%here) & 15 & ge1(($$-%%here) & 15) & ~ge4(($$-%%here) & 15)) nop
	times (1                & ge4(($$-%%here) & 15) & ~ge%1(($$-%%here) & 15)) jmp short %%skip
	times (((($$-%%here) & 15)-2) & ge4(($$-%%here) & 15) & ~ge%1(($$-%%here) & 15)) nop
%%skip:
%endmacro

%imacro	loopalignK7	0-1 12 
%%here:
	times (1 & ge1(($$-%%here) & 15)  & ~ge2(($$-%%here) & 15)  & ~ge%1(($$-%%here) & 15)) nop
	times (1 & ge2(($$-%%here) & 15)  & ~ge3(($$-%%here) & 15)  & ~ge%1(($$-%%here) & 15)) DB 08Bh,0C0h
	times (1 & ge3(($$-%%here) & 15)  & ~ge4(($$-%%here) & 15)  & ~ge%1(($$-%%here) & 15)) DB 08Dh,004h,020h
	times (1 & ge4(($$-%%here) & 15)  & ~ge5(($$-%%here) & 15)  & ~ge%1(($$-%%here) & 15)) DB 08Dh,044h,020h,000h
	times (1 & ge5(($$-%%here) & 15)  & ~ge6(($$-%%here) & 15)  & ~ge%1(($$-%%here) & 15)) DB 08Dh,044h,020h,000h,090h
	times (1 & ge6(($$-%%here) & 15)  & ~ge7(($$-%%here) & 15)  & ~ge%1(($$-%%here) & 15)) DB 08Dh,080h,0,0,0,0
	times (1 & ge7(($$-%%here) & 15)  & ~ge8(($$-%%here) & 15)  & ~ge%1(($$-%%here) & 15)) DB 08Dh,004h,005h,0,0,0,0
	times (1 & ge8(($$-%%here) & 15)  & ~ge9(($$-%%here) & 15)  & ~ge%1(($$-%%here) & 15)) DB 08Dh,004h,005h,0,0,0,0,90h
	times (1 & ge9(($$-%%here) & 15)  & ~ge10(($$-%%here) & 15) & ~ge%1(($$-%%here) & 15)) DB 0EBh,007h,90h,90h,90h,90h,90h,90h,90h
	times (1 & ge10(($$-%%here) & 15) & ~ge11(($$-%%here) & 15) & ~ge%1(($$-%%here) & 15)) DB 0EBh,008h,90h,90h,90h,90h,90h,90h,90h,90h
	times (1 & ge11(($$-%%here) & 15) & ~ge12(($$-%%here) & 15) & ~ge%1(($$-%%here) & 15)) DB 0EBh,009h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	times (1 & ge12(($$-%%here) & 15) & ~ge13(($$-%%here) & 15) & ~ge%1(($$-%%here) & 15)) DB 0EBh,00Ah,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	times (1 & ge13(($$-%%here) & 15) & ~ge14(($$-%%here) & 15) & ~ge%1(($$-%%here) & 15)) DB 0EBh,00Bh,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	times (1 & ge14(($$-%%here) & 15) & ~ge15(($$-%%here) & 15) & ~ge%1(($$-%%here) & 15)) DB 0EBh,00Ch,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
	times (1 & ge15(($$-%%here) & 15) & ~ge16(($$-%%here) & 15) & ~ge%1(($$-%%here) & 15)) DB 0EBh,00Dh,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h,90h
%%skip:
%endmacro

%imacro	loopalign	0-1 12 
	loopalignK7 %1
%endmacro
%define PACK(x,y,z,w)	(x*64+y*16+z*4+w)

%ifidn __OUTPUT_FORMAT__,elf

%idefine PIC_BASE(A) _GLOBAL_OFFSET_TABLE_ + $$ - $ wrt ..gotpc
%idefine PIC_EBP_REL(A) ebp + A wrt ..gotoff
%macro PIC_OFFSETTABLE 0
extern  _GLOBAL_OFFSET_TABLE_
get_pc.bp:
	mov ebp, [esp]
	retn
%endmacro

%else

%define PIC_BASE(A) (0)
%define PIC_EBP_REL(A) (A)
%macro PIC_OFFSETTABLE 0
get_pc.bp:
	mov ebp, [esp]
	retn
%endmacro

%endif
