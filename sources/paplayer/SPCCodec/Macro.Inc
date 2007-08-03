;อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ
;Description: TASM Like Macros
;Platform:    NASM
;Programmer:  Anti Resonance
;
;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Revision History:
;
; 2004.09.14
; - Changed parameter %defines to context local (makes the code look uglier, but keeps parameter
;   names from getting a global scope)
;
; 2004.08.07
; + Replaced RetZ and RetNZ with Retc
; + Updated PUBLIC, PROC, and LOCALS so the number of parameters is unlimited
; + USES now pushes EBP when procedure has no stack frame
; + Simplified ENDP by using a loop to pop used registers
;
;                                                          Copyright (C)2003-06 Alpha-II Productions
;อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ

;The following defines affect the way some macros work:
;   INTERNAL effects the way PUBLIC works (see PUBLIC header for more info)
;   STDCALL mangles labels and cleans up the stack on returns
;   CDECL mangles labels

%define	ST		ST0


;อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ
;Overloaded Instructions

;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Push
;
;Params:
;   List of values to be pushed onto the stack.  Values are pushed left to right.

%macro Push 2-*.nolist
	%rep %0
		Push	%1
		%rotate 1
	%endrep
%endmacro


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Pop
;
;Params:
;   List of values to be popped off the stack.  Values are popped left to right.

%macro Pop 2-*.nolist
	%rep %0
		Pop	%1
		%rotate 1
	%endrep
%endmacro


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Call Procedure
;
;If STDCALL is defined, the procedure is assumed to clean up the stack.
;
;Params:
;   Function name followed by stack parameters.  All arguments are assumed to be dwords.

%macro Call 2-*.nolist
	%rep %0-1
		%rotate -1
		Push	dword %1
	%endrep
	%rotate -1

	Call	%1

	%ifndef STDCALL
		Add	ESP,(%0*4)-4
	%endif
%endmacro


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Return from Procedure
;
;Ensures the stack frame will be cleaned up and any used registers restored
;
;Params (optional):
;   Value to return in EAX

%macro RetS 0-1.nolist								;Short return
	%if %0
		Mov	EAX,%1
	%endif

	%ifdef %$_params
		Jmp	short %$Done
	%elifdef %$_uses
		Jmp	short %$Done
	%else
		Ret
	%endif
%endmacro

%macro RetN 0-1.nolist								;Near return
	%if %0
		Mov	EAX,%1
	%endif

	%ifdef %$_params
		Jmp	%$Done
	%elifdef %$_uses
		Jmp	%$Done
	%else
		Ret
	%endif
%endmacro


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Conditional Return from Procedure
;
;Ensures the stack frame will be cleaned up and any used registers restored
;
;Params:
;   Condition code
;   Value to return in EAX (optional)

%macro Retc 1-2.nolist
	%if %0==2
		Mov	EAX,%2
	%endif
	J%+1	short %$Done
%endmacro


;อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ
;Procedure Macros

;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Publicly Declare a Label
;
;If INTERNAL is defined, the label is declared as GLOBAL, otherwise EXTERN.
;If STDCALL is defined, the label has an underscore prepended and an 'at' with the size of the
; argument stack appended.
;If CDECL is defined, the label has an underscore prepended.
;
;Params:
;   The variable or procedure name.  Procedures must be followed by a list of arguments.  If a
;   procedure has no arguments, specify 'NULL'.

%macro PUBLIC 1-*.nolist
	%ifdef	STDCALL
		%push PUBLIC

		%if %0==1
			%xdefine ARGSIZE
		%else
			%ifidni %2,NULL
				%xdefine ARGSIZE @0
			%else
				%assign %$size (%0*4)-4
				%xdefine ARGSIZE @%$size
			%endif
		%endif

		%ifdef	INTERNAL
			GLOBAL	_%1ARGSIZE
		%else
			EXTERN	_%1ARGSIZE
		%endif

		%xdefine	%1	_%1ARGSIZE

		%pop
	%elifdef	CDECL
		%ifdef	INTERNAL
			GLOBAL	_%1
		%else
			EXTERN	_%1
		%endif

		%xdefine	%1	_%1
	%else
		%ifdef	INTERNAL
			GLOBAL	%1
		%else
			EXTERN	%1
		%endif
	%endif
%endmacro


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Define Procedure
;
;Used to begin a procedure definition (body).  Defines a label, creates the initial stack frame, and
;generates offsets for any arguments.  Arguments are assumed to be 32-bit and passed right to left.
;
;Params:
;   Procedure name followed by a list of arguments passed on the stack

%macro PROC 1-*.nolist
	ALIGN	16												;Align entry point on a cache line boundary
	%1:													;Define procedure label

	%push PROC											;Save context

	%if %0>1												;Does this procedure have any arguments?
		Push	EBP										;Create a frame pointer
		Mov	EBP,ESP

		%assign %$_params (%0*4)-4					;Save the size of the frame

		%assign %$_offset 8							;Create pointers to each argument
		%rep %0-1
			%rotate 1
			%xdefine %$%1 EBP+%$_offset
			%assign %$_offset %$_offset+4
		%endrep
	%endif
%endmacro


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Define Local Variables
;
;Defines a list of local variables to be temporarily allocated on the stack
;
;By default, each variable name is allocated four bytes on the stack.  Specifying a number in the
;list will allocate that much memory for the next variable.  For example:
;
;   LOCALS float,8,double  -  Allocate four bytes for %$float and eight bytes for %$double
;
;Params:
;   Variable names

%macro LOCALS 1-*.nolist
	%ifndef %$_params									;If a stack frame hasn't been created, create one
		%assign	%$_params 0							;No arguments
		Push	EBP
		Mov	EBP,ESP
	%endif

	%assign %$_offset 0								;Create pointers to each of the locals
	%rep %0
		%ifnum %1
			%assign %$_offset %$_offset+%1-4
		%else
			%assign %$_offset %$_offset+4
			%xdefine %$%1 EBP-%$_offset
		%endif
		%rotate 1
	%endrep

	%assign %$_locals %$_offset					;Save the size of the local variables
	Add	ESP,-%$_locals								;Allocate space on the stack
%endmacro


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Registers Used by Procedure
;
;Pushes registers onto the stack, then restores them at the end of the procedure.  EAX is assumed
;to contain the return value.
;
;ALL saves all registers, except ESP and, if there's no stack frame, EBP.
;
;Params:
;   Registers used by the procedure (EBX, ECX, EDX, ESI, EDI, EBP, or ALL to save all.)

%macro USES 1-5.nolist
	%ifidni %1,ALL
		Push	ECX,EDX,EBX,ESI,EDI
		%assign %$_uses 5
		%assign %$_ur0 1
		%assign %$_ur1 2
		%assign %$_ur2 3
		%assign %$_ur3 6
		%assign %$_ur4 7
		%ifndef %$paramsize
			Push EBP
			%assign %$_uses 6
			%assign %$_ur5 5
		%endif
	%else
		%assign	%$_uses 0
		%rep	%0
			%ifidni %1,ECX								;Does parameter == "ECX"?
				%assign %$_ur%$_uses 1				;Define "$_ur?" as the register number
				Push	ECX								;Push ECX
			%elifidni %1,EDX
				%assign %$_ur%$_uses 2
				Push	EDX
			%elifidni %1,EBX
				%assign %$_ur%$_uses 3
				Push	EBX
			%elifidni %1,EBP
				%assign %$_ur%$_uses 5
				Push	EBP
			%elifidni %1,ESI
				%assign %$_ur%$_uses 6
				Push	ESI
			%elifidni %1,EDI
				%assign %$_ur%$_uses 7
				Push	EDI
			%else
				%error "USES only accepts EBX, ECX, EDX, ESI, EDI, EBP, or ALL"
			%endif

			%rotate 1
			%assign	%$_uses %$_uses+1
		%endrep
	%endif

%endmacro


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;End Procedure Definition
;
;Ends a procedure definition.  Restores any used registers, cleans up the stack frame, and pops the
;context stack.  Also pops any arguments off the stack if STDCALL is defined.

%macro ENDP 0-*.nolist
	%$Done:

	%ifdef %$_uses										;Pop used registers off the stack
		%rep %$_uses
			%assign %$_uses %$_uses-1
			DB	58h+%$_ur%$_uses						;Pop register off the stack
		%endrep
	%endif

	%ifdef %$_params									;Clean up stack frame
		Leave

		%if %$_params > 0
			%ifdef STDCALL
				Ret	%$_params
			%else
				Ret
			%endif
		%else
			Ret
		%endif
	%else
		Ret
	%endif

	%pop
%endmacro


;อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ
;Data Macros

;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Creating Floating Point Powers of 2

%macro Scale32 2.nolist							;Single
	%1	DD	(127+(%2)) << 23
%endmacro

%macro Scale64 2.nolist							;Double
	%1	DD	0,(1023+(%2)) << 20
%endmacro

%macro Scale80 2.nolist							;Extended
	%1	DD	0,80000000h
		DW	16383+(%2)
%endmacro
