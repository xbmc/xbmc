;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Sony SPC700 Emulator
;                                                        Copyright (C)1999-2006 Alpha-II Productions
;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ

CPU	386
BITS	32


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Header files

%include	"../../Macro.Inc"
%include	"APU.Inc"
%include	"DSP.Inc"
%define INTERNAL
%include	"SPC700.Inc"

EXTERN SetDSPReg_A									;DSP.Asm - Used to write to the DSP registers

%if DSPINTEG
EXTERN UpdateDSP										;DSP.Asm - Used to keep DSP in sync with SPC700
%endif

%if PROFILE
GLOBAL profile											;Needed by APU.Asm and DSP.Asm
%endif


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Equates

;Clock cycles per operation -----------------
T64_CYC	EQU	384									;64kHz timer clock divisor

;Cleanup options ----------------------------
na			EQU	0										;Option is not applicable to this instruction
RD			EQU	010000b								;Instruction reads 8 bits from the direct page
RD16		EQU	010010b								;                  16 bits from the direct page
RBIT		EQU	010001b								;                  1 bit from an absolute address
RA			EQU	010101b								;                  8 bits from an absolute address
WD			EQU	100000b								;Instruction writes 8 bits to the direct page
WD16		EQU	100010b								;                   16 bits to the direct page
WBIT		EQU	100001b								;                   1 bit to an absolute address
WA			EQU	100101b								;                   8 bits to an absolute address
RW			EQU	110000b								;Instruction reads and writes 8 bits in the DP

;Registers ----------------------------------
%define	PC		SI										;Intel equivilants to SPC registers
%define	A		AL
%define	Y		AH
%define	YA		AX
%define	X		CH

;Pointers -----------------------------------
%define	OP		ESI									;Instruction operand
%define	DPI 	EBX									;Direct Page Index
%define	ABSL	EDX+EDI								;Absolute Location
%define	RAM	EDI									;64K RAM


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Macros

;All global variables are stored in the 128 bytes before APU RAM.  Since EDI always points to APU
;RAM during emulation, this allows global variables to be accessed with an 8-bit displacement
;relative to EDI instead of a full 32-bit pointer.  The end result is instructions that access
;global variables are three bytes smaller, and small is good.  (When compiling SNESAPU.DLL, this
;also reduces the size of the .reloc section.)

%define	_Var(v)		RAM - (GVar.sizeof - GVar. %+ v)	;Get global variable relative to EDI
%define	_VarR(r,v)	r - (GVar.sizeof - GVar. %+ v)	;Get global relative to any register
%define	_PSW(f)		PSWFlags. %+ f + RAM - (GVar.sizeof - GVar.regPSW)	;Get byte flag rel to EDI

%if PROFILE
%define	_Profile(t) Inc dword [EDX*4 + profile + Profile. %+ t]	;Increase profile counter
%define	_ProfStart	Call	StartAPUProfile,Profile.spcTSC
%define	_ProfEnd		Call	EndAPUProfile,Profile.spcTSC
%else
%define	_Profile(t)
%define	_ProfStart
%define	_ProfEnd
%endif

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Convert Flags
;   Convert flags between internal 8-byte and external 8-bit format

;Unpacks the bit flags in CL into bytes and stores them in regPSW
;
;In:	CL = PSW
;
;Out:	regPSW = Flags
;		BX indexes direct page selected by P
;		EDX -> regPSW
;     CL = Reflects state N and Z

%macro _UnpackPSW 0
	LEA	EDX,[7+_Var(regPSW)]
	Mov	BL,8
	%%Next:
		RoL	CL,1
		SetC	[EDX]
		Dec	EDX
	Dec	BL
	JNZ	%%Next
	Mov	BH,[_PSW(P)]
	And	CL,82h
	XOr	CL,2
%endmacro

;Pack the byte flags in regPSW into bits and store them in CL
;
;In:	regPSW
;
;Out:	CL  = Flags
;		EDX -> regPSW

%macro _PackPSW 0
	Test	CL,CL
	SetS	[_PSW(N)]									;Set N and Z byte flags
	SetZ	[_PSW(Z)]
	LEA	EDX,[7+_Var(regPSW)]
	Mov	CL,2
	%%Next:
		Or		CL,[EDX]
		Dec	EDX
		Add	CL,CL
	JNC	%%Next
	Or		CL,[EDX]
%endmacro


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Structures

;The flags are split up into eight bytes.  During emulation, N and Z are not set.  Instead, CL
;contains the result of the last instruction, and N and Z can be determined from it.  Between calls
;to EmuSPC, though, N and Z will be set accordingly.  All flags will contain a value of 1 or 0.

STRUC PSWFlags
	.C	resb	1											;Carry
	.Z	resb	1											;Zero
	.I	resb	1											;Interrupts Enabled (unused in the SNES)
	.H	resb	1											;Half-Carry
	.B	resb	1											;Software Break
	.P	resb	1											;Direct Page selector
	.V	resb	1											;Integer Overflow
	.N	resb	1											;Negative
ENDSTRUC

;APU RAM mapping

STRUC MemMap
	dp0			resb	0F0h							;Direct Page 0
		testReg	resb	1								;	Test register
		control	resb	1								;	Control register
		dspAddr	resb	1								;	DSP Address
		dspData	resb	1								;	DSP Data
		port0		resb	1								;	Port 0
		port1		resb	1								;	Port 1
		port2		resb	1								;	Port 2
		port3		resb	1								;	Port 3
					resb	1								;	unused
					resb	1								;	unused
		t0			resb	1								;	Timer 0
		t1			resb	1								;	Timer 1
		t2			resb	1								;	Timer 2
		c0			resb	1								;	Counter 0
		c1			resb	1								;	Counter 1
		c2			resb	1								;	Counter 2
	dp1			resb	100h							;Direct Page 1
					resb	0FD00h						;General Pages 2-254
	up				resb	0C0h							;Uppermost Page 255
		ipl		resb	040h							;	Program used to transfer memory
ENDSTRUC



;ßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßß
; Data

SECTION .rodata ALIGN=32

															;Offsets of each opcode handler
	opcOfs	DW	Opc00-Opc,Opc01-Opc,Opc02-Opc,Opc03-Opc,Opc04-Opc,Opc05-Opc,Opc06-Opc,Opc07-Opc
				DW	Opc08-Opc,Opc09-Opc,Opc0A-Opc,Opc0B-Opc,Opc0C-Opc,Opc0D-Opc,Opc0E-Opc,Opc0F-Opc
				DW	Opc10-Opc,Opc11-Opc,Opc12-Opc,Opc13-Opc,Opc14-Opc,Opc15-Opc,Opc16-Opc,Opc17-Opc
				DW	Opc18-Opc,Opc19-Opc,Opc1A-Opc,Opc1B-Opc,Opc1C-Opc,Opc1D-Opc,Opc1E-Opc,Opc1F-Opc
				DW	Opc20-Opc,Opc21-Opc,Opc22-Opc,Opc23-Opc,Opc24-Opc,Opc25-Opc,Opc26-Opc,Opc27-Opc
				DW	Opc28-Opc,Opc29-Opc,Opc2A-Opc,Opc2B-Opc,Opc2C-Opc,Opc2D-Opc,Opc2E-Opc,Opc2F-Opc
				DW	Opc30-Opc,Opc31-Opc,Opc32-Opc,Opc33-Opc,Opc34-Opc,Opc35-Opc,Opc36-Opc,Opc37-Opc
				DW	Opc38-Opc,Opc39-Opc,Opc3A-Opc,Opc3B-Opc,Opc3C-Opc,Opc3D-Opc,Opc3E-Opc,Opc3F-Opc
				DW	Opc40-Opc,Opc41-Opc,Opc42-Opc,Opc43-Opc,Opc44-Opc,Opc45-Opc,Opc46-Opc,Opc47-Opc
				DW	Opc48-Opc,Opc49-Opc,Opc4A-Opc,Opc4B-Opc,Opc4C-Opc,Opc4D-Opc,Opc4E-Opc,Opc4F-Opc
				DW	Opc50-Opc,Opc51-Opc,Opc52-Opc,Opc53-Opc,Opc54-Opc,Opc55-Opc,Opc56-Opc,Opc57-Opc
				DW	Opc58-Opc,Opc59-Opc,Opc5A-Opc,Opc5B-Opc,Opc5C-Opc,Opc5D-Opc,Opc5E-Opc,Opc5F-Opc
				DW	Opc60-Opc,Opc61-Opc,Opc62-Opc,Opc63-Opc,Opc64-Opc,Opc65-Opc,Opc66-Opc,Opc67-Opc
				DW	Opc68-Opc,Opc69-Opc,Opc6A-Opc,Opc6B-Opc,Opc6C-Opc,Opc6D-Opc,Opc6E-Opc,Opc6F-Opc
				DW	Opc70-Opc,Opc71-Opc,Opc72-Opc,Opc73-Opc,Opc74-Opc,Opc75-Opc,Opc76-Opc,Opc77-Opc
				DW	Opc78-Opc,Opc79-Opc,Opc7A-Opc,Opc7B-Opc,Opc7C-Opc,Opc7D-Opc,Opc7E-Opc,Opc7F-Opc
				DW	Opc80-Opc,Opc81-Opc,Opc82-Opc,Opc83-Opc,Opc84-Opc,Opc85-Opc,Opc86-Opc,Opc87-Opc
				DW	Opc88-Opc,Opc89-Opc,Opc8A-Opc,Opc8B-Opc,Opc8C-Opc,Opc8D-Opc,Opc8E-Opc,Opc8F-Opc
				DW	Opc90-Opc,Opc91-Opc,Opc92-Opc,Opc93-Opc,Opc94-Opc,Opc95-Opc,Opc96-Opc,Opc97-Opc
				DW	Opc98-Opc,Opc99-Opc,Opc9A-Opc,Opc9B-Opc,Opc9C-Opc,Opc9D-Opc,Opc9E-Opc,Opc9F-Opc
				DW	OpcA0-Opc,OpcA1-Opc,OpcA2-Opc,OpcA3-Opc,OpcA4-Opc,OpcA5-Opc,OpcA6-Opc,OpcA7-Opc
				DW	OpcA8-Opc,OpcA9-Opc,OpcAA-Opc,OpcAB-Opc,OpcAC-Opc,OpcAD-Opc,OpcAE-Opc,OpcAF-Opc
				DW	OpcB0-Opc,OpcB1-Opc,OpcB2-Opc,OpcB3-Opc,OpcB4-Opc,OpcB5-Opc,OpcB6-Opc,OpcB7-Opc
				DW	OpcB8-Opc,OpcB9-Opc,OpcBA-Opc,OpcBB-Opc,OpcBC-Opc,OpcBD-Opc,OpcBE-Opc,OpcBF-Opc
				DW	OpcC0-Opc,OpcC1-Opc,OpcC2-Opc,OpcC3-Opc,OpcC4-Opc,OpcC5-Opc,OpcC6-Opc,OpcC7-Opc
				DW	OpcC8-Opc,OpcC9-Opc,OpcCA-Opc,OpcCB-Opc,OpcCC-Opc,OpcCD-Opc,OpcCE-Opc,OpcCF-Opc
				DW	OpcD0-Opc,OpcD1-Opc,OpcD2-Opc,OpcD3-Opc,OpcD4-Opc,OpcD5-Opc,OpcD6-Opc,OpcD7-Opc
				DW	OpcD8-Opc,OpcD9-Opc,OpcDA-Opc,OpcDB-Opc,OpcDC-Opc,OpcDD-Opc,OpcDE-Opc,OpcDF-Opc
				DW	OpcE0-Opc,OpcE1-Opc,OpcE2-Opc,OpcE3-Opc,OpcE4-Opc,OpcE5-Opc,OpcE6-Opc,OpcE7-Opc
				DW	OpcE8-Opc,OpcE9-Opc,OpcEA-Opc,OpcEB-Opc,OpcEC-Opc,OpcED-Opc,OpcEE-Opc,OpcEF-Opc
				DW	OpcF0-Opc,OpcF1-Opc,OpcF2-Opc,OpcF3-Opc,OpcF4-Opc,OpcF5-Opc,OpcF6-Opc,OpcF7-Opc
				DW	OpcF8-Opc,OpcF9-Opc,OpcFA-Opc,OpcFB-Opc,OpcFC-Opc,OpcFD-Opc,OpcFE-Opc,OpcFF-Opc

															;Pointers to function register handlers
	funcRTab	DD	Func0r,	Func1r,	Func2r,	Func3r,	Func4r,	Func5r,	Func6r,	Func7r
				DD	Func8r,	Func9r,	FuncAr,	FuncBr,	FuncCr,	FuncDr,	FuncEr,	FuncFr

	funcWTab	DD	Func0w,	Func1w,	Func2w,	Func3w,	Func4w,	Func5w,	Func6w,	Func7w,
				DD	Func8w,	Func9w,	FuncAw,	FuncBw,	FuncCw,	FuncDw,	FuncEw,	FuncFw

															;The 64 bytes of ROM in the IPL region
	iplROM	DB 0CDh,0EFh							;FFC0		Mov	X,#EF
				DB	0BDh									;FFC2		Mov	SP,X			;SP = 01EFh
				DB	0E8h,000h							;FFC3		Mov	A,#00h
				DB	0C6h									;FFC5		Mov	(X),A			;Set all bytes in DP0 to 0
				DB	01Dh									;FFC6		Dec	X
				DB	0D0h,0FCh							;FFC7		BNE	FFC5
				DB	08Fh,0AAh,0F4h						;FFC9		Mov	0F4h,#0AAh	;Notify 65816 that we're
				DB	08Fh,0BBh,0F5h						;FFCC		Mov	0F5h,#0BBh	; ready for transfer
				DB	078h,0CCh,0F4h						;FFCF		Cmp	0F4h,#0CCh
				DB	0D0h,0FBh							;FFD2		BNE	FFCF			;Wait for 65816 to respond
				DB	02Fh,019h							;FFD4		Bra	FFEF
				DB	0EBh,0F4h							;FFD6		Mov	Y,0F4h
				DB	0D0h,0FCh							;FFD8		BNE	FFD6			;Wait for port0 == 0
				DB	07Eh,0F4h							;FFDA		Cmp	Y,0F4h
				DB	0D0h,00Bh							;FFDC		BNE	FFE9			;Wait for 65816 to send byte
				DB	0E4h,0F5h							;FFDE		Mov	A,0F5h		;Get byte
				DB 0CBh,0F4h							;FFE0		Mov	0F4h,Y		;Ack that we received byte
				DB	0D7h,000h							;FFE2		Mov	[00h]+Y,A	;Store byte in RAM
				DB	0FCh									;FFE4		Inc	Y				;Move to next byte
				DB	0D0h,0F3h							;FFE5		BNE	FFDA
				DB	0ABh,001h							;FFE7		Inc	01h			;Move to next page
				DB	010h,0EFh							;FFE9		BPl	FFDA
				DB	07Eh,0F4h							;FFEB		Cmp	Y,0F4h		;If (port0 - Y) > 1, get
				DB	010h,0EBh							;FFED		BPl	FFDA			; next address
				DB	0BAh,0F6h							;FFEF		MovW	YA,0F6h		;Get destination address
				DB	0DAh,000h							;FFF1		MovW	00h,YA
				DB	0BAh,0F4h							;FFF3		MovW	YA,0F4h
				DB	0C4h,0F4h							;FFF5		Mov	0F4h,A
				DB	0DDh									;FFF7		Mov	A,Y
				DB	05Dh									;FFF8		Mov	X,A
				DB	0D0h,0DBh							;FFF9		BNE	FFD6			;If X == 0, begin execution
				DB	01Fh,000h,000h						;FFFB		Jmp	[!0000h+X]
				DB	0C0h,0FFh							;FFFE		(Reset vector)



;ßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßß
; Variables

SECTION .bss ALIGN=64

	opcTab		resd	256							;Pointers to each opcode handler
	pAPURAM		resd	1								;Pointer to 64K SPC RAM

STRUC GVar
	.extraRAM	resb	64	;80						;Memory used for writes when ROM reading is enabled

	.regYA		resw	1	;C0
	.regPC		resw	1	;C2
	.regSP		resd	1	;C4						;Physical address of current Stack Pointer
	.regPSW		resb	8	;C8						;Flags in byte form
	.regX			resb	1	;D0						;Storage for registers between calls

	.dbgOpt		resb	1	;D1						;Debugging options
					resb	1

	.t8kHz		resb	1	;D3						;64kHz cycles left until 8kHz pulse
	.t64kHz		resd	1	;D4						;APU cycles left until 64kHz pulse
	.t64Cnt		resd	1	;D8						;64kHz counter (increased every 64kHz pulse)
	.t64Last		resd	1	;DC						;Value of 't64Cnt' at the last counter read
	.clkTotal	resd	1	;E0						;Total number of clock cycles left to execute
	.clkExec		resd	1	;E4						;Number of cycles needed for 64kHz timer to increase
	.clkLeft		resd	1	;E8						;Number of cycles left to execute until timer increase

	.pDebug		resd	1	;EC						;Pointer to tracing routine

	.inPortCp	resb	4	;F0						;In port copies [F4-F7h]
	.outPort		resb	4	;F4						;Out port values [2140-43h]
	.oldCtrl		resb	1	;F8						;Copy of [F1h]
					resb	1
	.timer
	.timer0		resb	1	;FA						;Timer value written to [FAh]
	.timer1		resb	1	;FB						;                       [FBh]
	.timer2		resb	1	;FC						;                       [FCh]
	.tStep
	.t0Step		resb	1	;FD						;Timer pulses left until next counter increase,
	.t1Step		resb	1	;FE						; aka up counters (even though I count down with them)
	.t2Step		resb	1	;FF						; Copied from [FA-FCh]
	.sizeof
ENDSTRUC

					resb	GVar.sizeof
	apuRAM		resb	20000h

%if PROFILE
	profile		resb	Profile.sizeof
%else
	profile
%endif



;ßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßß
; Code

SECTION .text ALIGN=16

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Initialize SPC700

PROC InitSPC
USES ECX,ESI,EDI

	Mov	EDI,apuRAM
	Add	EDI,0FFFFh
	Mov	DI,0
	Mov	[pAPURAM],EDI

	LEA	EAX,[EDI+1FFh]
	Mov	[_Var(regSP)],EAX

	Mov	AX,0F0h
	Mov	byte [1+EAX],80h							;IPL ROM reading enabled
	Mov	dword [0Ch+EAX],0F0F0F00h				;Counters set to 0Fh
	Mov	byte [_Var(timer0)],0FFh				;Timers set to 00h (ResetSPC will set the registers)
	Mov	byte [_Var(timer1)],0FFh
	Mov	byte [_Var(timer2)],0FFh

	Call	SetSPCDbg,0,0								;Disable debugging

	;Generate table of opcode pointers -------
	;The locations of the opcodes are stored as 16-bit offsets which are then converted to 32-bit
	;pointers by this code.  The only reason this is done is to make the compiled executable smaller.

	Mov	ESI,opcOfs
	Mov	EDI,opcTab
	XOr	ECX,ECX
	.Next:
		MovZX	EAX,word [ECX*2+ESI]
		Add	EAX,Opc
		Mov	[ECX*4+EDI],EAX
	Inc	CL
	JNZ	short .Next

%if PROFILE
	XOr	EAX,EAX
	Mov	EDI,profile
	Mov	ECX,Profile.sizeof/4
	Rep	StoSD
%endif

	Mov	EAX,[pAPURAM]

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Reset SPC700

PROC ResetSPC
USES ECX,EDX,ESI,EDI

	;Erase 64K SPC RAM -----------------------
	;Technically the RAM doesn't get erased on reset, but I do this for debugging purposes.
	Mov	EDI,[pAPURAM]
	Or		EAX,-1										;Fill RAM with STOPs
	Mov	ECX,0F0h / 4
	Rep	StoSD
	Add	EDI,10h
	Mov	ECX,0FF00h / 4
	Rep	StoSD

	;Reset Function Registers ----------------
	;<Verified by Blargg on 04.10.03>
	Mov	EDI,[pAPURAM]
	LEA	ESI,[EDI+0F0h]
	XOr	EAX,EAX
	Mov	[0+ESI],AL									;Test gets set to 00h
	And	byte [1+ESI],07h							;Timer status is preserved, other bits are reset
	Or		byte [1+ESI],80h
	Mov	[4+ESI],EAX									;Reset in-ports
	Mov	[_Var(outPort)],EAX						;Reset out-ports
	Mov	word [8+ESI],-1							;See above comment on erasing RAM
	Mov	AL,[_Var(timer0)]							;Copy timers to RAM for FixSPCLoad
	Inc	AL
	Mov	[0Ah+ESI],AL
	Mov	AL,[_Var(timer1)]
	Inc	AL
	Mov	[0Bh+ESI],AL
	Mov	AL,[_Var(timer2)]
	Inc	AL
	Mov	[0Ch+ESI],AL

	;Copy IPL ROM ----------------------------
	Mov	ESI,iplROM									;Copy to RAM
	Mov	DI,ipl
	Mov	CL,10h
	Rep	MovSD

	Mov	ESI,iplROM									;Copy to extra RAM
	Mov	EDI,[pAPURAM]
	LEA	EDI,[_Var(extraRAM)]
	Mov	CL,10h
	Rep	MovSD

	;Reset internal variables ----------------
	Mov	EDI,[pAPURAM]
	Mov	byte [_Var(t8kHz)],7						;Reset timer clocks
	Mov	dword [_Var(t64kHz)],T64_CYC-CPU_CYC
	Mov	dword [_Var(t64Cnt)],0
	Mov	dword [_Var(t64Last)],0

	Call	FixSPCLoad,0FFC0h,0,0,0,0,0

	And	byte [_Var(dbgOpt)],~SPC_HALT			;Reset the halt flag

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Debug SPC700

PROC SetSPCDbg, pTrace, opts
USES EDX,EDI

	Mov	EDI,[pAPURAM]

	Mov	AL,[%$opts]
	Cmp	AL,-1
	JE		short .NoOpts								;Leave options as they are
		And	AL,SPC_HALT|SPC_STOP
		Mov	[_Var(dbgOpt)],AL						;Save options
	.NoOpts:

	Mov	EDX,[%$pTrace]
	Mov	EAX,[_Var(pDebug)]
	Cmp	EDX,-1
	JE		short .NoFunc
		Mov	[_Var(pDebug)],EDX
		Jmp	short .Func
	.NoFunc:
		Mov	EDX,EAX
	.Func:

%if DEBUG
	;Should tracing be enabled? --------------
	And	byte [_Var(dbgOpt)],~SPC_TRACE		;Disable instruction tracing by default

	Test	EDX,EDX										;Make sure function pointer isn't null...
	SetZ	DL
	Test	byte [_Var(dbgOpt)],SPC_STOP			;...and tracing option isn't disabled
	SetNZ	DH
	Or		DL,DH
	Dec	DL
	And	DL,SPC_TRACE
	Or		[_Var(dbgOpt)],DL
%endif

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Fix SPC700 After Load

PROC FixSPCLoad, pc, a, y, x, psw, sp
USES ALL

	Mov	EDI,[pAPURAM]

	;Load registers --------------------------
	XOr	EAX,EAX
	Mov	AX,[%$pc]
	Mov	[_Var(regPC)],AX
	Mov	AL,[%$a]
	Mov	AH,[%$y]
	Mov	[_Var(regYA)],AX
	Mov	AL,[%$x]
	Mov	[_Var(regX)],AL
	Mov	AL,[%$sp]
	Mov	[_Var(regSP)],AL

	Mov	CL,[%$psw]
	_UnpackPSW

	XOr	EAX,EAX
	Mov	[RAM+testReg],AL

	Mov	AL,[RAM+t0]									;Initialize timer counters
	Mov	[RAM+t0],AH
	Dec	AL
	Mov	[_Var(timer0)],AL
	Mov	[_Var(t0Step)],AL

	Mov	AL,[RAM+t1]
	Mov	[RAM+t1],AH
	Dec	AL
	Mov	[_Var(timer1)],AL
	Mov	[_Var(t1Step)],AL

	Mov	AL,[RAM+t2]
	Mov	[RAM+t2],AH
	Dec	AL
	Mov	[_Var(timer2)],AL
	Mov	[_Var(t2Step)],AL

	Mov	EAX,[RAM+port0]							;Copy port values to InP
	Mov	[_Var(inPortCp)],EAX

	Mov	AL,[RAM+control]							;Copy control register for comparisons
	And	AL,87h
	Mov	[_Var(oldCtrl)],AL
	Mov	[RAM+control],AL

	ShL	EAX,29										;Set disabled counters to 0 and clear upper nibble of
	SAR	EAX,15										; enabled counters
	SAR	AX,7
	SAR	AL,7
	ShL	EAX,8
	And	EAX,0F0F0F00h
	And	[RAM+t2],EAX

	;Copy the correct extra RAM --------------
	Test	byte [_Var(oldCtrl)],80h
	Retc	NZ

	LEA	ESI,[RAM+ipl]
	LEA	EDI,[_Var(extraRAM)]
	Mov	ECX,10h
	Rep	MovSD

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Save SPC700's Current State of Operation

PROC SaveSPC, pState
USES ALL

	Mov	EBX,[%$pState]

	;RAM -------------------------------------
	Mov	EDI,[EBX+SPCState.pRAM]
	Test	EDI,EDI
	JZ		.NoRAM
		Mov	ESI,[pAPURAM]

		Test	byte [ESI+control],80h
		JNZ	short .ROM
			Mov	ECX,10000h/4						;If IPL writing is enabled, copy all 64k of RAM
			Rep	MovSD
			Jmp	short .NoXRAM

		.ROM:
			Mov	ECX,0FFC0h/4
			Rep	MovSD

			Mov	ESI,[pAPURAM]						;Copy extra RAM from saved buffer
			LEA	ESI,[_VarR(ESI,extraRAM)]
			Or		ECX,40h/4
			Rep	MovSD

		.NoXRAM:
		Mov	EDI,[pAPURAM]
		Mov	ESI,[EBX+SPCState.pRAM]				;Copy timer values
		Add	ESI,0F0h
		Mov	AL,[_Var(timer0)]
		Inc	AL
		Mov	[0Ah+ESI],AL
		Mov	AL,[_Var(timer1)]
		Inc	AL
		Mov	[0Bh+ESI],AL
		Mov	AL,[_Var(timer2)]
		Inc	AL
		Mov	[0Ch+ESI],AL
	.NoRAM:

	Mov	EDI,[pAPURAM]

	;Registers -------------------------------
	Mov	AX,[_Var(regPC)]
	Mov	[EBX+SPCState.pc],AX

	Mov	AX,[_Var(regYA)]
	Mov	[EBX+SPCState.a],AX

	Mov	AL,[_Var(regX)]
	Mov	[EBX+SPCState.x],AL

	Mov	AX,[_Var(regSP)]
	Mov	[EBX+SPCState.sp],AX

	LEA	ECX,[_Var(regPSW)]
	Mov	AL,80h
	.Flag:
		Mov	AH,[ECX]
		Inc	ECX
	ShR	EAX,1
	JNC	short .Flag
	Mov	[EBX+SPCState.psw],AL

	;Ports -----------------------------------
	Mov	EAX,[_Var(outPort)]
	Mov	[EBX+SPCState.outP],EAX

	;Timers ----------------------------------
	Mov	EAX,[_Var(tStep)]
	Mov	[EBX+SPCState.upCnt],EAX

	Mov	AL,[_Var(t8kHz)]
	Mov	[EBX+SPCState.t8kHz],AL

	Mov	EAX,[_Var(t64kHz)]
	Mov	[EBX+SPCState.t64kHz],EAX

	Mov	EAX,[_Var(t64Cnt)]
	Mov	[EBX+SPCState.t64Cnt],EAX

	;Clear reserved bytes --------------------
	XOr	EAX,EAX
	Mov	[EBX+SPCState._r1],EAX

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Restore SPC700's State of Operation

PROC RestoreSPC, pState
USES ALL

	Mov	EBX,[%$pState]
	XOr	EAX,EAX
	Mov	EDX,[pAPURAM]

	;RAM -------------------------------------
	Mov	ESI,[EBX+SPCState.pRAM]
	Test	ESI,ESI
	JZ		short .NoRAM
		Mov	EDI,EDX
		Mov	ECX,0FFC0h/4
		Rep	MovSD

		LEA	EDI,[_VarR(EDX,extraRAM)]
		Or		ECX,40h/4
		Rep	MovSD

		Mov	EDI,EDX

		Mov	AL,[EDI+control]
		And	AL,87h
		Mov	[_Var(oldCtrl)],AL

		Mov	EAX,[EDI+port0]
		Mov	[_Var(inPortCp)],EAX

		Mov	ESI,iplROM
		Test	AL,80h
		JNZ	short .ROM
			LEA	ESI,[_Var(extraRAM)]
		.ROM:

		Mov	DI,ipl
		Or		ECX,40h/4
		Rep	MovSD

		Mov	EDI,EDX
		LEA	ESI,[EDX+0F0h]
		Mov	AL,[0Ah+ESI]
		Mov	[0Ah+ESI],CL
		Dec	AL
		Mov	[_Var(timer0)],AL
		Mov	AL,[0Bh+ESI]
		Mov	[0Bh+ESI],CL
		Dec	AL
		Mov	[_Var(timer1)],AL
		Mov	AL,[0Ch+ESI]
		Mov	[0Ch+ESI],CL
		Dec	AL
		Mov	[_Var(timer2)],AL
	.NoRAM:

	Mov	EDI,EDX

	;Registers -------------------------------
	Mov	AX,[EBX+SPCState.pc]
	Mov	[_Var(regPC)],AX

	Mov	AX,[EBX+SPCState.a]
	Mov	[_Var(regYA)],AX

	Mov	AL,[EBX+SPCState.x]
	Mov	[_Var(regX)],AL

	Mov	AL,[EBX+SPCState.sp]
	Mov	[_Var(regSP)],AL

	Mov	AL,[EBX+SPCState.psw]
	LEA	ECX,[_Var(regPSW)]
	Mov	EDX,7
	.Flag:
		ShL	AL,1
		SetC	[EDX+ECX]
	Dec	EDX
	JNS	short .Flag

	;Ports -----------------------------------
	Mov	EAX,[EBX+SPCState.outP]
	Mov	[_Var(outPort)],EAX

	;Timers ----------------------------------
	Mov	EAX,[EBX+SPCState.upCnt]
	Mov	[_Var(t0Step)],AX
	ShR	EAX,16
	Mov	[_Var(t2Step)],AL

	Mov	AL,[EBX+SPCState.t8kHz]
	Mov	[_Var(t8kHz)],AL

	Mov	EAX,[EBX+SPCState.t64kHz]
	Mov	[_Var(t64kHz)],EAX

	Mov	EAX,[EBX+SPCState.t64Cnt]
	Mov	[_Var(t64Cnt)],EAX
	And	dword [_Var(t64Last)],0

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Write to APU RAM from the SPC700

PROC SetAPURAM, addr, val
USES EDX

	;Write value to memory -------------------
	Mov	EDX,[pAPURAM]
	Mov	DX,[%$addr]
	Mov	AL,[%$val]
	Mov	[EDX],AL

	;Check for special addresses -------------
	Cmp	DX,ipl										;IPL ROM region
	JAE	short .WROM

	Dec	DH												;Function registers
	Cmp	DX,-16
	Retc	B

	;Handle function register ----------------
	Push	EBX,EDI,EBP
	And	EDX,0Fh
	Mov	EDI,[pAPURAM]
	Mov	EBP,.Return
	Jmp	[EDX*4+funcWTab]
	.Return:
	Pop	EBP,EDI,EBX
	RetS

	;Handle IPL ROM --------------------------
	.WROM:
%if IPLW
	Mov	EAX,EDX
	Mov	AX,0F1h
	Test	byte [EAX],80h
	Retc	Z
%endif

	Push	ECX,EDI
	MovZX	ECX,DX
	Mov	EDI,[pAPURAM]
%if IPLW
	Mov	AL,[EDX]
	Mov	[ECX+_Var(extraRAM)-ipl],AL
	Mov	AL,[ECX+iplROM-ipl]
%else
	Mov	AL,[ECX+_Var(extraRAM)-ipl]
%endif
	Mov	[EDX],AL
	Pop	EDI,ECX
	RetS

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Write to APU Port

PROC SetAPUPort, port, val
USES ECX,EDI

	Mov	EDI,[pAPURAM]
	Mov	ECX,[%$port]
	Mov	AL,[%$val]

	;Write value to port ---------------------
	And	ECX,3
	Mov	[ECX+_Var(inPortCp)],AL
	Mov	[ECX+EDI+0F4h],AL

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Read from APU Port

PROC GetAPUPort, port

	Mov	EAX,[pAPURAM]
	Mov	AL,[%$port]
	And	AL,3
	Mov	AL,[_VarR(EAX,outPort)]

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Get SPC700 Time

PROC GetSPCTime

	Mov	EAX,[pAPURAM]
	Mov	EAX,[_VarR(EAX,t64Cnt)]

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Profile Timers

PROC StartAPUProfile, tsc

%if PROFILE
	Push	EAX,ECX
	Mov	ECX,profile
	Add	ECX,[%$tsc]
	DB		0Fh,31h										;RdTSC - Read Time-Stamp Counter
	Mov	[8+ECX],EAX
	Mov	[12+ECX],EDX
	Pop	ECX,EAX
%endif

ENDP


PROC EndAPUProfile, tsc

%if PROFILE
	Push	EAX,ECX
	Mov	ECX,profile
	Add	ECX,[%$tsc]
	DB		0Fh,31h
	Sub	EAX,[8+ECX]
	SbB	EDX,[12+ECX]
	Add	[ECX],EAX
	AdC	[4+ECX],EDX
	Pop	ECX,EAX
%endif

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Emulate SPC700

PROC EmuSPC, cyc
USES ALL

	_ProfStart

	Mov	EDI,[pAPURAM]
	XOr	EAX,EAX										;Return 0, if SPC700 is halted
	Mov	EDX,SPCFetch								;Default to normal instruction fetching

	Test	byte [_Var(dbgOpt)],SPC_HALT|SPC_TRACE
%if DEBUG
	JZ		short .NoHalt
	Test	byte [_Var(dbgOpt)],SPC_TRACE
;	Jmp	SPCDone
	Mov	EDX,SPCTrace								;Enable instruction tracing
	.NoHalt:
%else
	Retc	NZ
%endif

	Mov	EAX,[%$cyc]									;EAX = clock cycles to execute
	Push	EBP
	Mov	EBP,EDX										;EBP = Location to jump to after handling an opcode

	;Setup clock cycle execution -------------
	;clkLeft contains the number of clock cycles to emulate until the next 64kHz clock pulse or it's time to quit

	XOr	EDX,EDX
	Mov	[_Var(clkTotal)],EAX

	Sub	EAX,[_Var(t64kHz)]						;if (cyc > t64kHz) clkExec = t64kHz
	SetA	DL												;else clkExec = cyc
	Dec	EDX
	And	EAX,EDX
	Add	EAX,[_Var(t64kHz)]

	Mov	[_Var(clkExec)],EAX						;Save the number we want to execute this lap
	Mov	[_Var(clkLeft)],EAX

	;Load x86 registers ----------------------
	XOr	ECX,ECX
	Mov	ESI,EDI
	Mov	EBX,EDI
	Mov	SI,[_Var(regPC)]
	MovZX	EAX,word [_Var(regYA)]
	Mov	CH,[_Var(regX)]
	Mov	BH,[_PSW(P)]
	Mov	CL,[_PSW(N)]
	RoR	CL,1
	Or		CL,1
	XOr	CL,[_PSW(Z)]

%if DEBUG
	Jmp	EBP											;Jump into emulation routine
%else
	Jmp	SPCFetch
%endif

SPCHalt:
	XOr	EDX,EDX
	Mov	[_Var(clkTotal)],EDX

ALIGN 16
SPCExit:
	Test	CL,CL
	SetS	[_PSW(N)]
	SetZ	[_PSW(Z)]
	Mov	[_Var(regPC)],SI							;Save emulated registers
	Mov	[_Var(regYA)],AX
	Mov	[_Var(regX)],CH
	Pop	EBP

%if PROFILE
	_ProfEnd
%endif

	Mov	EAX,[_Var(clkTotal)]						;Return clock cycles left to emulate

SPCDone:

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Opcode Fetcher
;
;Fetches the next opcode and jumps to the appropriate handler.  Keeps track of clock cycles emulated
;and timers.

	;Fetching begins by first checking if more cycles need to be emulated before updating timer 2.
	;If so, then get the next opcode and emulate it.

ALIGN	16
SPCFetch:
	Cmp	dword [_Var(clkLeft)],0					;Have we executed all clock cycles?
	JL		short SPCTimers							;  Yes, Update timers

	MovZX	EDX,byte [ESI]								;Fetch instruction opcode
	_Profile(exec)
	Inc	PC												;Advance PC to first operand or next instruction
	Jmp	[EDX*4+opcTab]								;Jump to handler

	;clkExec contains the number of clock cycles we wanted to emulate this round.  By subtracting
	;clkLeft (which will be a number < 0) we get the actual number of cycles emulated, which will
	;be subtracted from the total number to be emulated (clkTotal) and used to update the timers.

SPCTimers:
	Mov	EDX,[_Var(clkExec)]						;EDX = Actual number of clock cycles emulated
	Sub	EDX,[_Var(clkLeft)]

	;Update timers ---------------------------
	Sub	[_Var(t64kHz)],EDX						;Subtract cycles from timer clock.  Has it rolled over?
	JNS	short .NoTInc								;  No, Don't update timers
		Mov	BL,[_Var(oldCtrl)]					;BL = Control register

		Add	dword [_Var(t64kHz)],T64_CYC		;Restore timer 2 clock
		Inc	dword [_Var(t64Cnt)]					;Increase 64kHz counter

		ShL	BL,6										;Copy timer enable bit into CF
		SbB	byte [_Var(t2Step)],0				;Decrease timer step if timer is enabled
		JNC	short .NoC2Inc							;If carry, counter needs to be increased
			Mov	BH,[_Var(timer2)]					;Restore the number of steps until counter increase
			Inc	byte [RAM+c2]						;Increase counter 2
			Mov	[_Var(t2Step)],BH
			And	byte [RAM+c2],0Fh					;Only lower 4-bits are operable
%if CNTBK
%if DSPINTEG
			Call	UpdateDSP
%else
			Mov	EBP,SPCExit							;Signal the emu to exit so the DSP can catch up
%endif
%endif
		.NoC2Inc:

		Dec	byte [_Var(t8kHz)]
		JNS	short .NoTInc
			Mov	byte [_Var(t8kHz)],7				;Reset clock pulse counter

			Add	BL,BL
			SbB	byte [_Var(t1Step)],0
			JNC	short .NoC1Inc
				Mov	BH,[_Var(timer1)]
				Inc	byte [RAM+c1]
				Mov	[_Var(t1Step)],BH
				And	byte [RAM+c1],0Fh
%if CNTBK
%if DSPINTEG
				Call	UpdateDSP
%else
				Mov	EBP,SPCExit
%endif
%endif
			.NoC1Inc:

			Add	BL,BL
			SbB	byte [_Var(t0Step)],0
			JNC	short .NoC0Inc
				Mov	BH,[_Var(timer0)]
				Inc	byte [RAM+c0]
				Mov	[_Var(t0Step)],BH
				And	byte [RAM+c0],0Fh
%if CNTBK
%if DSPINTEG
				Call	UpdateDSP
%else
				Mov	EBP,SPCExit
%endif
%endif
			.NoC0Inc:
	.NoTInc:

	;After the timers are updated, subtract the number of cycles emulated from the total number
	;passed in.  If we've emulated all clock cycles requested by the caller, then quit.  Otherwise
	;emulate until the next 64kHz clock pulse.

	Sub	[_Var(clkTotal)],EDX						;Have we executed all clock cycles?
	JLE	SPCExit										;  Yes, Quit

	Mov	BX,AX											;Calculate number of cycles to emulate
	Mov	EAX,[_Var(t64kHz)]						;clkExec = (t64kHz < clkTotal) ? t64kHz : clkTotal
	Sub	EAX,[_Var(clkTotal)]
	CDQ
	And	EAX,EDX
	Add	EAX,[_Var(clkTotal)]
	Mov	[_Var(clkLeft)],EAX
	Mov	[_Var(clkExec)],EAX
	Mov	AX,BX

	Mov	BH,[_PSW(P)]								;Restore EBX

	Jmp	EBP											;Return to fetcher

	;Tracing operates like fetching, except a user specified vector is called before executing the
	;instruction.

ALIGN	16
SPCTrace:
	Cmp	dword [_Var(clkLeft)],0					;Have we executed all clock cycles?
	JL		SPCTimers									;  Yes, Update timers

SPCBreak:
	;Call SPCTrace ---------------------------
	Mov	EBX,[_Var(timer)]
	_PackPSW
	MovZX	EDX,CL										;EDX = PSW
	ShR	ECX,8											;ECX = X
	Sub	EBX,[_Var(tStep)]

	Push	EBX											;Pass up counters
	Push	dword [_Var(regSP)]						; ""  SP
	Push	EDX											; ""  PSW
	Push	ECX											; ""  X
	Push	EAX											; ""  YA
	Push	ESI											; ""  PC

	Mov	EAX,T64_CYC-CPU_CYC						;Pass number of cycles left until 64kHz increase
	Sub	EAX,[_Var(t64kHz)]
	Add	EAX,[_Var(clkExec)]
	Sub	EAX,[_Var(clkLeft)]
	Mov	CL,CPU_CYC
	Div	CL
	Mov	[23+ESP],AL

	Call	[_Var(pDebug)]								;Call tracing routine

	;Update registers ------------------------
	Pop	EAX											;Pop PC
	Mov	SI,AX
	Pop	EAX											;Pop YA
	And	EAX,0FFFFh
	Pop	ECX											;Pop X
	MovZX	ECX,CL
	ShL	ECX,8
	Pop	EDX											;Pop PSW
	Mov	CL,DL
	_UnpackPSW
	Pop	EDX											;Pop SP
	Mov	[_Var(regSP)],DL
	Pop	EDX

	Test	byte [_Var(dbgOpt)],SPC_HALT
	JNZ	SPCHalt

	Mov	EBX,EDI
	Mov	BH,[_PSW(P)]

	MovZX	EDX,byte [ESI]
	Inc	PC												;Move PC to first operand or next instruction
	Jmp	[EDX*4+opcTab]								;Jump to handler


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Macros for Instruction Emulation

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Load pointers
;   Load DPI or ABSL (aliases for EBX) with the value needed by the instruction.

;dp - Load DPI with the 8-bit immediate value
%macro _dp 0
	Mov	BL,[OP]										;;BL-> Location in DP
%endmacro

;dp - Load DPI with the 2nd 8-bit immediate value
%macro _dp2 0
	Inc	PC
	Mov	BL,[OP]
%endmacro

;(X) - Load DPI with the value in X
%macro _X 0
	Mov	BL,X
%endmacro

;(Y) - Load DPI with the value in Y
%macro _Y 0
	Mov	BL,Y
%endmacro

;dp+X - Load DPI with the 8-bit immediate value + X
%macro _dpX 0
	_dp
	Add	BL,X
%endmacro

;dp+Y - Load DPI with the 8-bit immediate value + Y
%macro _dpY 0
	_dp
	Add	BL,Y
%endmacro

;!abs - Load ABSL with the 16-bit immediate value
%macro _abs 0
	Mov	DX,[OP]
%endmacro

;!abs+X - Load ABSL with the 16-bit immediate value + X
%macro _absX 0
	Mov	DL,X
	Add	DX,[OP]
%endmacro

;!abs+Y - Load ABSL with the 16-bit immediate value + Y
%macro _absY 0
	Mov	DL,Y
	Add	DX,[OP]
%endmacro

;[dp+X] - Load ABSL with the indexed 16-bit value at [dp+X]
%macro _idpX 0
	_dpX
	Mov	DX,[DPI]
%endmacro

;[dp]+Y - Load ABSL with the indexed 16-bit value at [dp] + Y
%macro _idpY 0
	_dp
	Mov	DL,Y
	Add	DX,[DPI]
%endmacro

;mem.bit - Load CF with bit indexed by the bit.mem operand
;   DX  -> byte in mem 0-1FFFh
;   CF   = bit in byte
%macro _mbit 0
	Mov	DX,[OP]
	RoL	DX,3
	BT		[RAM],EDX
%endmacro


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Stack Operations

;Pop byte off of stack
;   Val - r/m to pop
%macro _Pop 1
	Inc	byte [_Var(regSP)]						;;Increase SP
	Mov	EDX,[_Var(regSP)]							;;EDX -> Current stack position
	Mov	%1,[EDX]										;;Get value from stack
%endmacro

;Pop PC off of stack
%macro _PopPC 0
	Mov	EDX,[_Var(regSP)]
	Mov	PC,[1+EDX]
	Add	byte [_Var(regSP)],2
%endmacro

;Push byte onto stack
;   Val - r/m to push
%macro _Push 1
	Mov	EDX,[_Var(regSP)]
	Dec	byte [_Var(regSP)]						;;Decrease SP
	Mov	[EDX],%1										;;Put value in stack
%endmacro

;Push PC onto stack
%macro _PushPC 0-1
	Mov	EDX,[_Var(regSP)]
%if %0
%if %1 == 1
	LEA	BX,[ESI+1]
%else
	LEA	BX,[ESI+2]
%endif
	Mov	[EDX-1],BX
	Mov	BH,[_PSW(P)]
%else
	Mov	[EDX-1],PC
%endif
	Sub	byte [_Var(regSP)],2
%endmacro


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Get flags from CPU
;   The x86 CPU updates its flags with the execution of each instruction.  These macros update
;   regPSW with the x86 equivilants.

;Get Carry flag
%macro _GetC 0
	SetC	[_PSW(C)]
%endmacro

;Get !Carry flag
%macro _GetCs 0
	SetNC	[_PSW(C)]
%endmacro

;Get Negative and Zero flags
%macro _GetNZ 0
;	SetS	[_PSW(N)]
;	SetZ	[_PSW(Z)]
%endmacro

;Get Negative, Zero, and Carry flags
%macro _GetNZC 0
	_GetC
	_GetNZ
%endmacro

;Get Negative, Zero, and !Carry flags (for compare instructions)
%macro _GetNZCs 0
	_GetCs
	_GetNZ
%endmacro

;Get Negative, Overflow, and Zero flags
%macro _GetNVZ 0
	_GetNZ
	SetO	[_PSW(V)]
%endmacro

;Get Negative, Overflow, Half-Carry, Zero, and Carry flags (for addition instructions)
%macro _GetNVHZC 0
	_GetC
	_GetNVZ
%if HALFC
	Mov	CL,AH
	LAHF
	Xchg	CL,AH
	Test	CL,10h
	SetNZ	[_PSW(H)]
	And	CL,0C0h
	XOr	CL,40h
%endif
%endmacro

;Get Negative, Overflow, Half-Carry, Zero, and Not Carry flags (for subtraction instructions)
%macro _GetNVHZCs 0
	_GetCs
	_GetNVZ
%if HALFC
	Mov	CL,AH
	LAHF
	Xchg	CL,AH
	Test	CL,10h
	SetZ	[_PSW(H)]
	And	CL,0C0h
	XOr	CL,40h
%endif
%endmacro


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Memory checkers
;   Certain sections of memory can't be written to or read from, or trigger events when read or
;   modified.  This is the part that really slows emulation down, for every instruction involving
;   memory multiple checks have to be performed.

;Peform functions when specific memory locations have been written to

%macro _WrChk 1
%if %1 & 4
	Cmp	DX,ipl										;;Was memory in the IPL ROM region modified?
	JAE	short %%WROM								;;  Yes
%endif

	Dec	DH
	Cmp	DX,-16										;;Was a function register written to?
%if DEBUG
	JAE	short %%WReg								;;  Yes, Jump to handler
		Jmp	EBP										;;  No, Jump to next opcode
	%%WReg:
%else
	JB		SPCFetch
%endif
	And	EDX,0Fh										;;EDX->Function register handler
%if %1 & 2
	Push	EDX											;;Save low function register handler
	Mov	EBP,FuncNext
%endif
	Jmp	[EDX*4+funcWTab]

%if %1 & 4
	%%WROM:
%if IPLW
		Test	byte [_Var(oldCtrl)],80h			;;Is ROM reading enabled?
%if DEBUG
		JZ 	short %%WNext							;;  No, write was okay
%else
		JZ		SPCFetch
%endif
%endif
%if IPLW
			Mov	BL,[EDX+RAM]						;;Copy byte written to extra RAM
			Mov	[EDX+_Var(extraRAM)-ipl],BL
			Mov	BL,[EDX+iplROM-ipl]				;;Replace byte from ROM
%else
			Mov	BL,[EDX+_Var(extraRAM)-ipl]
%endif
			Mov	[EDX+RAM],BL
		%%WNext:
		Jmp	EBP
%endif
%endmacro

;Perform functions when specific memory locations have been read

%macro _RdChk 1
	Dec	DH

%if PROFILE
	Cmp	DX,-16									;;Check if registers F0 - FF were read
%elif DSPBK
	Cmp	DX,-13									;;Check if registers F3 - FF were read
%else
	Cmp	DX,-3										;;Check if registers FD - FF were read
%endif

%if %1 & 20h										;;If a write check happens afterwards, return here
	JB		short %%NotF
		And	EDX,0Fh
		Push	EBP
		Mov	EBP,%%Return
		Jmp	[EDX*4+funcRTab]
	%%Return:
		Pop	EBP
	%%NotF:
%elif DEBUG
	JAE	short %%ReadF
		Jmp	EBP
	%%ReadF:
	And	EDX,0Fh
	Jmp	[EDX*4+funcRTab]
%else
	JB		SPCFetch
	And	EDX,0Fh
	Jmp	[EDX*4+funcRTab]
%endif
%endmacro


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Clean up after executing an instruction
;   Updates flags to reflect instruction, checks memory read/writes, and decreases the clock counter
;   the number of cycles it takes for the instruction to execute on the SPC700.  If the opcode has
;   any operands, PC is moved past them.

;   %1 = Number of clock cycles instruction takes to execute
;   %2 = Number of bytes in expression
;   %3 = Check memory
;        01h - EDX/EBX is used for address
;        02h - 16/8-bit operation
;        04h - check for access to IPL region
;        10h - perform read checking
;        20h - perform write checking
;   %4 = Flags to be updated
;        NZ, NZC(s), NVZ, NVZC(s)

%macro _CleanUp 2-*.nolist
	%if %0 >= 4											;;Is flag paramater not blank?
		_Get%4											;;  Yes, Get modified flags
	%endif

	Sub	dword [_Var(clkLeft)],%1*CPU_CYC		;Subtract cycles instruction takes to execute

	%if %2 == 2
		Inc	PC
	%elif %2 == 3
		Add	PC,2
	%endif

	%if %0 < 3											;;Has a memory check been specified?
%if DEBUG
		Jmp	EBP										;;  No memory check, Grab next opcode
%else
		Jmp	SPCFetch
%endif
	%else
		%ifidn %3,na									;;  False alarm, no mem check needs to be performed
%if DEBUG
			Jmp	EBP
%else
			Jmp	SPCFetch
%endif
		%else
			%if %3 & 10h								;;Does a read check need to be performed?
				%if (%3 & 21h) == 0					;;If BX contains the address, copy it to DX
					Mov	EDX,EBX
				%endif
				%if %3 & 22h							;;If input is RW or RD16, expand macro in-line
					_RdChk %3
				%else
					Jmp	ReadDP
				%endif
			%endif

			%if %3 & 20h								;;Does a write check need to be performed?
				%if (%3 & 1) == 0						;;If BX contains the address, copy it to DX
					Mov	EDX,EBX
				%endif
				%if %3 & 2								;;If input is WD16, expand macro in-line
					_WrChk %3
				%elif %3 & 4
					Jmp	WriteAbs
				%else
					Jmp	WriteDP
				%endif
			%endif
		%endif
	%endif
%endmacro

ALIGN 16
ReadDP:
	_RdChk RD

ALIGN 16
WriteDP:
	_WrChk WD

ALIGN 16
WriteAbs:
	_WrChk WA


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Opcode Handlers
;
;Ins - Desc
; Flags
;Form
;
;Ins:   Mnemonic for instruction
;
;Desc:  Long name for instruction
;
;Flags: Results of flag after execution
;       ? - Unknown
;       * - Reflects result of instruction
;       0 - Clear
;       1 - Set
;
;Form:  Format implemented
;
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Upon entrance to an opcode handler, the registers will be:
;   AL  = SPC.A
;   AH  = SPC.Y
;   CH  = SPC.X
;   CL  = Result of previous instruction (Used to check for N and Z)
;   EDX = Current opcode
;   EBX-> Current direct page (BL is undefined)
;   SI  = SPC.PC+1
;   ESI-> First byte after opcode
;   EDI-> Base of APU RAM
;   EBP-> SPCFetch (or SPCTrace if debugging is enabled)
;
;   CL, EDX, and BL may be freely modified during execution.  Upon exit:
;   - CL must contain the result of the operation, unless the instruction doesn't modify N and Z,
;     in which case it must remain unmodified.
;   - If the instruction accesses an absolute memory location, DX must index that location.
;   - If the instruction accesses a direct page location, BX must index that location.
;   - If the instruction accesses DP for its source and destination operands, DX must index the
;     source location and BX the destination
;
;   All other registers must remain unchanged, or reflect the results of the operation.
;
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Opcodes:
;
;   00 NOP     10 BPL     20 CLRP    30 BMI     40 SETP    50 BVC     60 CLRC    70 BVS
;   01 TCALL   11 TCALL   21 TCALL   31 TCALL   41 TCALL   51 TCALL   61 TCALL   71 TCALL
;   02 SET1    12 CLR1    22 SET1    32 CLR1    42 SET1    52 CLR1    62 SET1    72 CLR1
;   03 BBS     13 BBC     23 BBS     33 BBC     43 BBS     53 BBC     63 BBS     73 BBC
;   04 OR      14 OR      24 AND     34 AND     44 EOR     54 EOR     64 CMP     74 CMP
;   05 OR      15 OR      25 AND     35 AND     45 EOR     55 EOR     65 CMP     75 CMP
;   06 OR      16 OR      26 AND     36 AND     46 EOR     56 EOR     66 CMP     76 CMP
;   07 OR      17 OR      27 AND     37 AND     47 EOR     57 EOR     67 CMP     77 CMP
;   08 OR      18 OR      28 AND     38 AND     48 EOR     58 EOR     68 CMP     78 CMP
;   09 OR      19 OR      29 AND     39 AND     49 EOR     59 EOR     69 CMP     79 CMP
;   0A OR1     1A DECW    2A OR1     3A INCW    4A AND1    5A CMPW    6A AND1    7A ADDW
;   0B ASL     1B ASL     2B ROL     3B ROL     4B LSR     5B LSR     6B ROR     7B ROR
;   0C ASL     1C ASL     2C ROL     3C ROL     4C LSR     5C LSR     6C ROR     7C ROR
;   0D PUSH    1D DEC     2D PUSH    3D INC     4D PUSH    5D MOV     6D PUSH    7D MOV
;   0E TSET1   1E CMP     2E CBNE    3E CMP     4E TCLR1   5E CMP     6E DBNZ    7E CMP
;   0F BRK     1F JMP     2F BRA     3F CALL    4F PCALL   5F JMP     6F RET     7F RETI
;
;   80 SETC    90 BCC     A0 EI      B0 BCS     C0 DI      D0 BNE     E0 CLRV    F0 BEQ
;   81 TCALL   91 TCALL   A1 TCALL   B1 TCALL   C1 TCALL   D1 TCALL   E1 TCALL   F1 TCALL
;   82 SET1    92 CLR1    A2 SET1    B2 CLR1    C2 SET1    D2 CLR1    E2 SET1    F2 CLR1
;   83 BBS     93 BBC     A3 BBS     B3 BBC     C3 BBS     D3 BBC     E3 BBS     F3 BBC
;   84 ADC     94 ADC     A4 SBC     B4 SBC     C4 MOV     D4 MOV     E4 MOV     F4 MOV
;   85 ADC     95 ADC     A5 SBC     B5 SBC     C5 MOV     D5 MOV     E5 MOV     F5 MOV
;   86 ADC     96 ADC     A6 SBC     B6 SBC     C6 MOV     D6 MOV     E6 MOV     F6 MOV
;   87 ADC     97 ADC     A7 SBC     B7 SBC     C7 MOV     D7 MOV     E7 MOV     F7 MOV
;   88 ADC     98 ADC     A8 SBC     B8 SBC     C8 CMP     D8 MOV     E8 MOV     F8 MOV
;   89 ADC     99 ADC     A9 SBC     B9 SBC     C9 MOV     D9 MOV     E9 MOV     F9 MOV
;   8A EOR1    9A SUBW    AA MOV1    BA MOVW    CA MOV1    DA MOVW    EA NOT1    FA MOV
;   8B DEC     9B DEC     AB INC     BB INC     CB MOV     DB MOV     EB MOV     FB MOV
;   8C DEC     9C DEC     AC INC     BC INC     CC MOV     DC DEC     EC MOV     FC INC
;   8D MOV     9D MOV     AD CMP     BD MOV     CD MOV     DD MOV     ED NOTC    FD MOV
;   8E POP     9E DIV     AE POP     BE DAA     CE POP     DE CBNE    EE POP     FE DBNZ
;   8F MOV     9F XCN     AF MOV     BF MOV     CF MUL     DF DAS     EF SLEEP   FF STOP

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;NOP - No Operation
; N V P B H I Z C
;

;NOp
ALIGN 16
Opc:														;This label marks the beginning of the opcode handlers
Opc00:
	_CleanUp	2,1										;Do nothing


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;ADC - Add with Carry
; N V P B H I Z C
; * *     *   * *

;AdC A,#imm
ALIGN 16
Opc88:
	Cmp	DH,[_PSW(C)]
	AdC	A,[OP]
	Mov	CL,A
	_CleanUp	2,2,na,NVHZC

;AdC A,dp
ALIGN 16
Opc84:
	_dp
	Cmp	DH,[_PSW(C)]
	AdC	A,[DPI]
	Mov	CL,A
	_CleanUp	3,2,RD,NVHZC

;AdC A,dp+X
ALIGN 16
Opc94:
	_dpX
	Cmp	DH,[_PSW(C)]
	AdC	A,[DPI]
	Mov	CL,A
	_CleanUp	4,2,RD,NVHZC

;AdC A,!abs
ALIGN 16
Opc85:
	_abs
	ShR	byte [_PSW(C)],1
	AdC	A,[ABSL]
	Mov	CL,A
	_CleanUp	4,3,RA,NVHZC

;AdC A,!abs+X
ALIGN 16
Opc95:
	_absX
	ShR	byte [_PSW(C)],1
	AdC	A,[ABSL]
	Mov	CL,A
	_CleanUp	5,3,RA,NVHZC

;AdC A,!abs+Y
ALIGN 16
Opc96:
	_absY
	ShR	byte [_PSW(C)],1
	AdC	A,[ABSL]
	Mov	CL,A
	_CleanUp	5,3,RA,NVHZC

;AdC A,(X)
ALIGN 16
Opc86:
	_X
	Cmp	DH,[_PSW(C)]
	AdC	A,[DPI]
	Mov	CL,A
	_CleanUp	3,1,RD,NVHZC

;AdC A,[dp+X]
ALIGN 16
Opc87:
	_idpX
	ShR	byte [_PSW(C)],1
	AdC	A,[ABSL]
	Mov	CL,A
	_CleanUp	6,2,RA,NVHZC

;AdC A,[dp]+Y
ALIGN 16
Opc97:
	_idpY
	ShR	byte [_PSW(C)],1
	AdC	A,[ABSL]
	Mov	CL,A
	_CleanUp	6,2,RA,NVHZC

;AdC dp,#imm
ALIGN 16
Opc98:
	Mov	CL,[OP]
	_dp2
	Cmp	DH,[_PSW(C)]
	AdC	CL,[DPI]
	Mov	[DPI],CL
	_CleanUp	5,2,WD,NVHZC

;AdC dp,dp
ALIGN 16
Opc89:
	_dp
	Mov	CL,[DPI]
	Mov	EDX,DPI
	_dp2
	ShR	byte [_PSW(C)],1
	AdC	CL,[DPI]
	Mov	[DPI],CL
	_CleanUp	6,2,RW,NVHZC

;AdC (X),(Y)
ALIGN 16
Opc99:
	_Y
	Mov	CL,[DPI]
	Mov	EDX,DPI
	_X
	ShR	byte [_PSW(C)],1
	AdC	CL,[DPI]
	Mov	[DPI],CL
	_CleanUp	5,1,RW,NVHZC


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;ADDW - Add Words
; N V P B H I Z C
; * *     *   * *

;AddW YA,dp
ALIGN 16
Opc7A:
	_dp
	Mov	DX,YA
	Add	DX,[DPI]
	SetNZ	CL
	Or		CL,DH
	Add	A,[DPI]
	AdC	Y,[1+DPI]									;Half carry is based on the addition made to Y
	_CleanUp	5,2,RD16,NVHZC


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;AND - Bit-wise Logical And
; N V P B H I Z C
; *           *

;And A,#imm
ALIGN 16
Opc28:
	And	A,[OP]
	Mov	CL,A
	_CleanUp	2,2,na,NZ

;And A,dp
ALIGN 16
Opc24:
	_dp
	And	A,[DPI]
	Mov	CL,A
	_CleanUp	3,2,RD,NZ

;And A,dp+X
ALIGN 16
Opc34:
	_dpX
	And	A,[DPI]
	Mov	CL,A
	_CleanUp	4,2,RD,NZ

;And A,!abs
ALIGN 16
Opc25:
	_abs
	And	A,[ABSL]
	Mov	CL,A
	_CleanUp	4,3,RA,NZ

;And A,!abs+X
ALIGN 16
Opc35:
	_absX
	And	A,[ABSL]
	Mov	CL,A
	_CleanUp	5,3,RA,NZ

;And A,!abs+Y
ALIGN 16
Opc36:
	_absY
	And	A,[ABSL]
	Mov	CL,A
	_CleanUp	5,3,RA,NZ

;And A,(X)
ALIGN 16
Opc26:
	_X
	And	A,[DPI]
	Mov	CL,A
	_CleanUp	3,1,RD,NZ

;And A,[dp+X]
ALIGN 16
Opc27:
	_idpX
	And	A,[ABSL]
	Mov	CL,A
	_CleanUp	6,2,RA,NZ

;And A,[dp]+Y
ALIGN 16
Opc37:
	_idpY
	And	A,[ABSL]
	Mov	CL,A
	_CleanUp	6,2,RA,NZ

;And dp,#imm
ALIGN 16
Opc38:
	Mov	CL,[OP]
	_dp2
	And	CL,[DPI]
	Mov	[DPI],CL
	_CleanUp	5,2,WD,NZ

;And dp,dp
ALIGN 16
Opc29:
	_dp
	Mov	CL,[DPI]
	Mov	EDX,DPI
	_dp2
	And	CL,[DPI]
	Mov	[DPI],CL
	_CleanUp	6,2,RW,NZ

;And (X),(Y)
ALIGN 16
Opc39:
	_Y
	Mov	CL,[DPI]
	Mov	EDX,DPI
	_X
	And	CL,[DPI]
	Mov	[DPI],CL
	_CleanUp	5,1,RW,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;AND1 - And Carry with Absolute Bit
; N V P B H I Z C
;               *

;And1 C,mem.bit
ALIGN 16
Opc4A:
	Test	byte [_PSW(C)],1							;Is carry zero?
	JZ 	short .NC									;  Yes, Result will be zero anyway so quit
		_mbit
		_GetC
		ShR	EDX,3
		_CleanUp	4,3,RBIT
	.NC:
	_CleanUp	4,3										;Should check memory read here, but meh...

;And1 C,/mem.bit
ALIGN 16
Opc6A:
	Test	byte [_PSW(C)],1
	JZ 	short .NCN
		_mbit
		_GetCs
		ShR	EDX,3
		_CleanUp	4,3,RBIT
	.NCN:
	_CleanUp	4,3


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;ASL - Arithmetic Shift Left
; N V P B H I Z C
; *           * *

;ASL A
ALIGN 16
Opc1C:
	Add	A,A
	Mov	CL,A
	_CleanUp	2,1,na,NZC

;ASL dp
ALIGN 16
Opc0B:
	_dp
	ShL	byte [DPI],1
	Mov	CL,[DPI]
	_CleanUp	4,2,WD,NZC

;ASL dp+X
ALIGN 16
Opc1B:
	_dpX
	ShL	byte [DPI],1
	Mov	CL,[DPI]
	_CleanUp	5,2,WD,NZC

;ASL !abs
ALIGN 16
Opc0C:
	_abs
	ShL	byte [ABSL],1
	Mov	CL,[ABSL]
	_CleanUp	5,3,WA,NZC


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;BBC - Branch If Bit Clear
; N V P B H I Z C
;

;BBC dp.?,rel
ALIGN 16
Opc13:
Opc33:
Opc53:
Opc73:
Opc93:
OpcB3:
OpcD3:
OpcF3:
	_dp
	ShR	EDX,5											;EDX = Bit to test
	Inc	PC												;Advance PC to displacment
	BT		[DPI],EDX									;Test requested bit, is it clear?
	JC		short .BCDone								;  No, Clean up
		_Profile(bbc)
		MovSX	EDX,byte [OP]							;EDX = Relative displacement
		Add	PC,DX										;Adjust PC
		Sub	dword [_Var(clkLeft)],2*CPU_CYC	;Subtract an additional 2 clock cycles
	.BCDone:
	Inc	PC
	_CleanUp	5,na,RD


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;BBS - Branch If Bit Set
; N V P B H I Z C
;

;BBS dp.?,rel
ALIGN 16
Opc03:
Opc23:
Opc43:
Opc63:
Opc83:
OpcA3:
OpcC3:
OpcE3:
	_dp
	ShR	EDX,5
	Inc	PC
	BT		[DPI],EDX
	JNC	short .BSDone
		_Profile(bbs)
		MovSX	EDX,byte [OP]
		Add	PC,DX
		Sub	dword [_Var(clkLeft)],2*CPU_CYC
	.BSDone:
	Inc	PC
	_CleanUp	5,na,RD


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Bcc - Conditional Branch
; N V P B H I Z C
;

	;-----------------------------------------
	;Branch on condition
	;   %1 - Flag to Test or condition of CL to test
	;   %2 - Condition of flag (0 or 1)

	%macro Bxx 1-2
		%if %0 == 2
			Test	byte [_PSW(%1)],1
		%if %2 == 1
			JZ 	short %%Done
		%else
			JNZ	short %%Done
		%endif
		%else
			Test	CL,CL
			J%-1	short %%Done
		%endif
%if PROFILE
			ShR	EDX,5
			_Profile(bxx)
%endif
			MovSX	EDX,byte [OP]
			Add	PC,DX
			_CleanUp	4,2
		%%Done:
		_CleanUp	2,2
	%endmacro

;	ShR	DL,5
;	And	DL,1
;	XOr	DL,[_PSW(%1)]
;	JNZ	short %%BcDone

;BCC rel - Branch if Carry Clear (JAE)
ALIGN 16
Opc90:
	Bxx	C,0

;BCS rel - Branch if Carry Set (JB)
ALIGN 16
OpcB0:
	Bxx	C,1

;BEQ rel - Branch if Equal (JE/JZ)
ALIGN 16
OpcF0:
	Bxx	Z

;BMI rel - Branch if Minus (JS)
ALIGN 16
Opc30:
	Bxx	S

;BNE rel - Branch if Not Equal (JNE/JNZ)
ALIGN 16
OpcD0:
	Bxx	NZ

;BPL rel - Branch if Plus (JNS)
ALIGN 16
Opc10:
	Bxx	NS

;BVC rel - Branch if Overflow Clear (JNO)
ALIGN 16
Opc50:
	Bxx	V,0

;BVS rel - Branch if Overflow Set (JO)
ALIGN 16
Opc70:
	Bxx	V,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;BRA - Branch (JMP Short)
; N V P B H I Z C
;

;BRA rel
ALIGN 16
Opc2F:
	MovSX	EDX,byte [OP]
	Add	PC,DX
	_CleanUp	4,2


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;BRK - Software Interrupt
; N V P B H I Z C
;       1   0

;Brk
ALIGN 16
Opc0F:
	_PushPC
	_PackPSW
	_Push	CL
	And	CL,82h										;_PackPSW destroyed the contents of CL
	XOr	CL,2											;Adjust CL to reflect the state of N and Z
	Mov	PC,[0FFDEh+RAM]
	Mov	byte [_PSW(B)],1
	Mov	byte [_PSW(I)],0
	_CleanUp	8,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;CALL - Call Procedure
; N V P B H I Z C
;

;Call !abs
ALIGN 16
Opc3F:
	_PushPC	2
	Mov		PC,[OP]
	_CleanUp	8,na


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;CBNE - Compare with A, Branch if Not Equal
; N V P B H I Z C
;

;CBNE dp,rel
ALIGN 16
Opc2E:
	_dp
	Inc	PC
	Cmp	A,[DPI]
	JE 	short .NCBdp
%if PROFILE
		Inc	dword [profile+Profile.cbne]
%endif
		MovSX	EDX,byte [OP]
		Add	PC,DX
		Sub	dword [_Var(clkLeft)],2*CPU_CYC
	.NCBdp:
	Inc	PC
	_CleanUp	5,na,RD

;CBNE dp+X,rel
ALIGN 16
OpcDE:
	_dpX
	Inc	PC
	Cmp	A,[DPI]
	JE 	short .NCBdpx
%if PROFILE
		Inc	dword [4+profile+Profile.cbne]
%endif
		MovSX	EDX,byte [OP]
		Add	PC,DX
		Sub	dword [_Var(clkLeft)],2*CPU_CYC
	.NCBdpx:
	Inc	PC
	_CleanUp	6,na,RD


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;CLR1 - Clear Bit
; N V P B H I Z C
;

;Clr1 dp.?
ALIGN 16
Opc12:
Opc32:
Opc52:
Opc72:
Opc92:
OpcB2:
OpcD2:
OpcF2:
	_dp
	ShR	EDX,5
	BTR	[DPI],EDX
	_CleanUp	4,2,WD



;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;CLRC - Clear Carry Flag
; N V P B H I Z C
;               0

;ClrC
ALIGN 16
Opc60:
	Mov	[_PSW(C)],DH
	_CleanUp	2,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;CLRP - Clear Direct Page Flag
; N V P B H I Z C
;     0

;ClrP
ALIGN 16
Opc20:
	Mov	BH,DH
	Mov	[_PSW(P)],DH
	_CleanUp	2,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;CLRV - Clear Overflow and Half-carry Flags
; N V P B H I Z C
;   0     0

;ClrV
ALIGN 16
OpcE0:
	Mov	[_PSW(V)],DH
	Mov	[_PSW(H)],DH
	_CleanUp	2,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;CMP - Compare Operands
; N V P B H I Z C
; *           * *

;Cmp A,#imm
ALIGN 16
Opc68:
	Mov	CL,A
	Sub	CL,[OP]
	_CleanUp	2,2,na,NZCs

;Cmp X,#imm
ALIGN 16
OpcC8:
	Mov	CL,X
	Sub	CL,[OP]
	_CleanUp	2,2,na,NZCs

;Cmp Y,#imm
ALIGN 16
OpcAD:
	Mov	CL,Y
	Sub	CL,[OP]
	_CleanUp	2,2,na,NZCs

;Cmp A,dp
ALIGN 16
Opc64:
	_dp
	Mov	CL,A
	Sub	CL,[DPI]
	_CleanUp	3,2,RD,NZCs

;Cmp X,dp
ALIGN 16
Opc3E:
	_dp
	Mov	CL,X
	Sub	CL,[DPI]
	_CleanUp	3,2,RD,NZCs

;Cmp Y,dp
ALIGN 16
Opc7E:
	_dp
	Mov	CL,Y
	Sub	CL,[DPI]
	_CleanUp	3,2,RD,NZCs

;Cmp A,dp+X
ALIGN 16
Opc74:
	_dpX
	Mov	CL,A
	Sub	CL,[DPI]
	_CleanUp	4,2,RD,NZCs

;Cmp A,!abs
ALIGN 16
Opc65:
	_abs
	Mov	CL,A
	Sub	CL,[ABSL]
	_CleanUp	4,3,RA,NZCs

;Cmp X,!abs
ALIGN 16
Opc1E:
	_abs
	Mov	CL,X
	Sub	CL,[ABSL]
	_CleanUp	4,3,RA,NZCs

;Cmp Y,!abs
ALIGN 16
Opc5E:
	_abs
	Mov	CL,Y
	Sub	CL,[ABSL]
	_CleanUp	4,3,RA,NZCs

;Cmp A,!abs+X
ALIGN 16
Opc75:
	_absX
	Mov	CL,A
	Sub	CL,[ABSL]
	_CleanUp	5,3,RA,NZCs

;Cmp A,!abs+Y
ALIGN 16
Opc76:
	_absY
	Mov	CL,A
	Sub	CL,[ABSL]
	_CleanUp	5,3,RA,NZCs

;Cmp A,(X)
ALIGN 16
Opc66:
	_X
	Mov	CL,A
	Sub	CL,[DPI]
	_CleanUp	3,1,RD,NZCs

;Cmp A,[dp+X]
ALIGN 16
Opc67:
	_idpX
	Mov	CL,A
	Sub	CL,[ABSL]
	_CleanUp	6,2,RA,NZCs

;Cmp A,[dp]+Y
ALIGN 16
Opc77:
	_idpY
	Mov	CL,A
	Sub	CL,[ABSL]
	_CleanUp	6,2,RA,NZCs

;Cmp dp,#imm
ALIGN 16
Opc78:
	Mov	DL,[OP]
	_dp2
	Mov	CL,[DPI]
	Sub	CL,DL
	_CleanUp	5,2,RD,NZCs

;Cmp dp,dp
ALIGN 16
Opc69:
	_dp
	Mov	DH,[DPI]
	Mov	DL,BL
	_dp2
	Mov	CL,[DPI]
	Sub	CL,DH
	_GetCs
	Mov	DH,BH
	_RdChk	WD											;These last intructions are oddities that require two
	_CleanUp	6,2,RD									; read checks

;Cmp (X),(Y)
ALIGN 16
Opc79:
	_Y
	Mov	DH,[DPI]
	Mov	DL,BL
	_X
	Mov	CL,[DPI]
	Sub	CL,DH
	Mov	DH,BH
	_GetCs
	_RdChk	WD
	_CleanUp	5,1,RD


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;CMPW - Compare Words
; N V P B H I Z C
; *           * *

;CmpW YA,dp
ALIGN 16
Opc5A:
	_dp
	Mov	DX,YA
	Sub	DX,[DPI]
	_GetCs
	SetNZ	CL
	Or		CL,DH
	_CleanUp	4,2,RD16


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;DAA - Decimal Adjust After Addition
; N V P B H I Z C
; *           * *

;DAA A
ALIGN 16
OpcDF:
	Mov	DL,AH											;Save AH (Y)
	Mov	AH,[_PSW(H)]								;Set AF and CF in AH
	ShL	AH,4
	Or 	AH,[_PSW(C)]
	SAHF													;Store AH into flags register
	Mov	AH,DL											;Restore AH
	DAA													;Execute DAA on AL (A)
	Mov	CL,A
	_CleanUp	3,1,na,NZC


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;DAS - Decimal Adjust After Subtraction
; N V P B H I Z C
; *           * *

;DAS A
ALIGN 16
OpcBE:
	Mov	DL,AH
	Mov	AH,[_PSW(H)]
	ShL	AH,4
	Or 	AH,[_PSW(C)]
	XOr	AH,11h										;Reverse flags for x86
	SAHF
	Mov	AH,DL
	DAS
	Mov	CL,A
	_CleanUp	3,1,na,NZCs


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;DBNZ - Decrease, Branch if not Zero
; N V P B H I Z C
;

;DBNZ Y,rel
ALIGN 16
OpcFE:
	Dec	Y
	JZ 	short .NoBranch
%if PROFILE
		Inc	dword [profile+Profile.dbnz]
%endif
		MovSX	EDX,byte [OP]
		Add	PC,DX
		Sub	dword [_Var(clkLeft)],2*CPU_CYC
	.NoBranch:
	_CleanUp	4,2

;DBNZ dp,rel
ALIGN 16
Opc6E:
	_dp
	Inc	PC
	Dec	byte [DPI]
	JZ 	short .NoBranch
%if PROFILE
		Inc	dword [4+profile+Profile.dbnz]
%endif
		MovSX	EDX,byte [OP]
		Add	PC,DX
		Sub	dword [_Var(clkLeft)],2*CPU_CYC
	.NoBranch:
	Inc	PC
	_CleanUp	5,na,WD


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;DEC - Decrease Byte by 1
; N V P B H I Z C
; *           *

;Dec A
ALIGN 16
Opc9C:
	Dec	A
	Mov	CL,A
	_CleanUp	2,1,na,NZ

;Dec X
ALIGN 16
Opc1D:
	Dec	X
	Mov	CL,X
	_CleanUp	2,1,na,NZ

;Dec Y
ALIGN 16
OpcDC:
	Dec	Y
	Mov	CL,Y
	_CleanUp	2,1,na,NZ

;Dec dp
ALIGN 16
Opc8B:
	_dp
	Dec	byte [DPI]
	Mov	CL,[DPI]
	_CleanUp	4,2,WD,NZ

;Dec dp+X
ALIGN 16
Opc9B:
	_dpX
	Dec	byte [DPI]
	Mov	CL,[DPI]
	_CleanUp	5,2,WD,NZ

;Dec !abs
ALIGN 16
Opc8C:
	_abs
	Dec	byte [ABSL]
	Mov	CL,[ABSL]
	_CleanUp	5,3,WA,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;DECW - Decrease Word by 1
; N V P B H I Z C
; *           *

;DecW dp
ALIGN 16
Opc1A:
	_dp
	Dec	word [DPI]
	SetNZ	CL
	Or		CL,[1+DPI]
	_CleanUp	6,2,WD16,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;DI - Disable Interrupts
; N V P B H I Z C
;           0

;DI
ALIGN 16
OpcC0:
	Mov	[_PSW(I)],DH
	_CleanUp	3,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;DIV - Divide
;<Divide by zero and normal divide verified by Blargg on 04.10.03>
;<Division overflow created by Anomie and verified by CaitSith on 05.08.04>
; N V P B H I Z C
; * *     *   *

;Div YA,X
ALIGN 16
Opc9E:
%if HALFC
	Mov	DL,Y
	Mov	BL,X
	And	DL,0Fh
	And	BL,0Fh
	Cmp	DL,BL
	SetAE	[_PSW(H)]
%endif

	Test	X,X											;Divide by 0?
	JZ 	short .Div0
	Cmp	Y,X											;Will division overflow?
	JAE	short .DivOF

		Div	X
		Mov	[_PSW(V)],DH
		Mov	CL,A
		_CleanUp	12,1,na,NZ

	.Div0:
		RoL	YA,8
		Not	A
		Mov	byte [_PSW(V)],1
		Mov	CL,A
		_CleanUp	12,1,na,NZ

	.DivOF:
		XOr	EDX,EDX								;Perform division manually to get the same wacky result
		Mov	CL,9									; as the SPC700

	.Next:
		SetNC	BL										;if (Y >= X) C ^= 1
		Cmp	Y,X
		AdC	BL,DL
		ShR	BL,1

		JNC	short .NoSub						;if (C) Y -= X
			Sub	Y,X
			StC
		.NoSub:

		RCL	YA,1

		Dec	CL
		JNZ	short .Next

		RCR	Y,1
		SetC	[_PSW(V)]
		Mov	CL,A
		_CleanUp	12,1,na,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;EI - Enable Interrupts
; N V P B H I Z C
;           1

;EI
ALIGN 16
OpcA0:
	Mov	byte [_PSW(I)],1
	_CleanUp	3,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;EOR - Bit-wise Logical Exclusive Or
; N V P B H I Z C
; *           *

;EOr A,#imm
ALIGN 16
Opc48:
	XOr	A,[OP]
	Mov	CL,A
	_CleanUp	2,2,na,NZ

;EOr A,dp
ALIGN 16
Opc44:
	_dp
	XOr	A,[DPI]
	Mov	CL,A
	_CleanUp	3,2,RD,NZ

;EOr A,dp+X
ALIGN 16
Opc54:
	_dpX
	XOr	A,[DPI]
	Mov	CL,A
	_CleanUp	4,2,RD,NZ

;EOr A,!abs
ALIGN 16
Opc45:
	_abs
	XOr	A,[ABSL]
	Mov	CL,A
	_CleanUp	4,3,RA,NZ

;EOr A,!abs+X
ALIGN 16
Opc55:
	_absX
	XOr	A,[ABSL]
	Mov	CL,A
	_CleanUp	5,3,RA,NZ

;EOr A,!abs+Y
ALIGN 16
Opc56:
	_absY
	XOr	A,[ABSL]
	Mov	CL,A
	_CleanUp	5,3,RA,NZ

;EOr A,(X)
ALIGN 16
Opc46:
	_X
	XOr	A,[DPI]
	Mov	CL,A
	_CleanUp	3,1,RD,NZ

;EOr A,[dp+X]
ALIGN 16
Opc47:
	_idpX
	XOr	A,[ABSL]
	Mov	CL,A
	_CleanUp	6,2,RA,NZ

;EOr A,[dp]+Y
ALIGN 16
Opc57:
	_idpY
	XOr	A,[ABSL]
	Mov	CL,A
	_CleanUp	6,2,RA,NZ

;EOr dp,#imm
ALIGN 16
Opc58:
	Mov	CL,[OP]
	_dp2
	XOr	CL,[DPI]
	Mov	[DPI],CL
	_CleanUp	5,2,WD,NZ

;EOr dp,dp
ALIGN 16
Opc49:
	_dp
	Mov	CL,[DPI]
	Mov	EDX,DPI
	_dp2
	XOr	CL,[DPI]
	Mov	[DPI],CL
	_CleanUp	6,2,RW,NZ

;EOr (X),(Y)
ALIGN 16
Opc59:
	_Y
	Mov	CL,[DPI]
	Mov	EDX,DPI
	_X
	XOr	CL,[DPI]
	Mov	[DPI],CL
	_CleanUp	5,1,RW,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;EOR1 - XOr Carry with Absolute Bit
; N V P B H I Z C
;               *

;EOr1 C,mem.bit
ALIGN 16
Opc8A:
	Mov	BL,DH
	_mbit
	RCL	BL,1
	XOr	[_PSW(C)],BL
	ShR	EDX,3
	_CleanUp	5,3,RBIT


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;INC - Increase Byte by 1
; N V P B H I Z C
; *           *

;Inc A
ALIGN 16
OpcBC:
	Inc	A
	Mov	CL,A
	_CleanUp	2,1,na,NZ

;Inc X
ALIGN 16
Opc3D:
	Inc	X
	Mov	CL,X
	_CleanUp	2,1,na,NZ

;Inc Y
ALIGN 16
OpcFC:
	Inc	Y
	Mov	CL,Y
	_CleanUp	2,1,na,NZ

;Inc dp
ALIGN 16
OpcAB:
	_dp
	Inc	byte [DPI]
	Mov	CL,[DPI]
	_CleanUp	4,2,WD,NZ

;Inc dp+X
ALIGN 16
OpcBB:
	_dpX
	Inc	byte [DPI]
	Mov	CL,[DPI]
	_CleanUp	5,2,WD,NZ

;Inc !abs
ALIGN 16
OpcAC:
	_abs
	Inc	byte [ABSL]
	Mov	CL,[ABSL]
	_CleanUp	5,3,WA,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;INCW - Increase Word by 1
; N V P B H I Z C
; *           *

;INCW dp
ALIGN 16
Opc3A:
	_dp
	Inc	word [DPI]
	SetNZ	CL
	Or		CL,[1+DPI]
	_CleanUp	6,2,WD16,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;JMP - Indirect Jump
; N V P B H I Z C
;

;Jmp !abs
ALIGN 16
Opc5F:
	Mov	PC,[OP]
	_CleanUp	3,na

;Jmp [!abs+X]
ALIGN 16
Opc1F:
	_absX
	Mov	PC,[ABSL]
	_CleanUp	6,na


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;LSR - Logical Shift Right
; N V P B H I Z C
; *           * *

;LSR A
ALIGN 16
Opc5C:
	ShR	A,1
	Mov	CL,A
	_CleanUp	2,1,na,NZC

;LSR dp
ALIGN 16
Opc4B:
	_dp
	ShR	byte [DPI],1
	Mov	CL,[DPI]
	_CleanUp	4,2,WD,NZC

;LSR dp+X
ALIGN 16
Opc5B:
	_dpX
	ShR	byte [DPI],1
	Mov	CL,[DPI]
	_CleanUp	5,2,WD,NZC

;LSR !abs
ALIGN 16
Opc4C:
	_abs
	ShR	byte [ABSL],1
	Mov	CL,[ABSL]
	_CleanUp	5,3,WA,NZC


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;MOV - Load Immediate
; N V P B H I Z C
; *           *

;Mov A,#imm
ALIGN 16
OpcE8:
	Mov	A,[OP]
	Mov	CL,A
	_CleanUp	2,2,na,NZ

;Mov X,#imm
ALIGN 16
OpcCD:
	Mov	X,[OP]
	Mov	CL,X
	_CleanUp	2,2,na,NZ

;Mov Y,#imm
ALIGN 16
Opc8D:
	Mov	Y,[OP]
	Mov	CL,Y
	_CleanUp	2,2,na,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;MOV - Copy Register
; N V P B H I Z C
; *           *

;Mov A,X
ALIGN 16
Opc7D:
	Mov	A,X
	Mov	CL,X
	_CleanUp	2,1,na,NZ

;Mov A,Y
ALIGN 16
OpcDD:
	Mov	A,Y
	Mov	CL,Y
	_CleanUp	2,1,na,NZ

;Mov X,A
ALIGN 16
Opc5D:
	Mov	X,A
	Mov	CL,A
	_CleanUp	2,1,na,NZ

;Mov Y,A
ALIGN 16
OpcFD:
	Mov	Y,A
	Mov	CL,A
	_CleanUp	2,1,na,NZ

;Mov X,SP
ALIGN 16
Opc9D:
	Mov	X,byte [_Var(regSP)]
	Mov	CL,X
	_CleanUp	2,1,na,NZ

;Mov SP,X (flags not affected)
ALIGN 16
OpcBD:
	Mov	byte [_Var(regSP)],X
	_CleanUp	2,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;MOV - Load Memory Into Register
; N V P B H I Z C
; *           *

;Mov A,dp
ALIGN 16
OpcE4:
	_dp
 	Mov	A,[DPI]
	Mov	CL,A
	_CleanUp	3,2,RD,NZ

;Mov X,dp
ALIGN 16
OpcF8:
	_dp
	Mov	X,[DPI]
	Mov	CL,X
	_CleanUp	3,2,RD,NZ

;Mov Y,dp
ALIGN 16
OpcEB:
	_dp
	Mov	Y,[DPI]
	Mov	CL,Y
	_CleanUp	3,2,RD,NZ

;Mov A,dp+X
ALIGN 16
OpcF4:
	_dpX
	Mov	A,[DPI]
	Mov	CL,A
	_CleanUp	4,2,RD,NZ

;Mov X,dp+Y
ALIGN 16
OpcF9:
	_dpY
	Mov	X,[DPI]
	Mov	CL,X
	_CleanUp	4,2,RD,NZ

;Mov Y,dp+X
ALIGN 16
OpcFB:
	_dpX
	Mov	Y,[DPI]
	Mov	CL,Y
	_CleanUp	4,2,RD,NZ

;Mov A,!abs
ALIGN 16
OpcE5:
	_abs
	Mov	A,[ABSL]
	Mov	CL,A
	_CleanUp	4,3,RA,NZ

;Mov X,!abs
ALIGN 16
OpcE9:
	_abs
	Mov	X,[ABSL]
	Mov	CL,X
	_CleanUp	4,3,RA,NZ

;Mov Y,!abs
ALIGN 16
OpcEC:
	_abs
	Mov	Y,[ABSL]
	Mov	CL,Y
	_CleanUp	4,3,RA,NZ

;Mov A,!abs+X
ALIGN 16
OpcF5:
	_absX
	Mov	A,[ABSL]
	Mov	CL,A
	_CleanUp	5,3,RA,NZ

;Mov A,!abs+Y
ALIGN 16
OpcF6:
	_absY
	Mov	A,[ABSL]
	Mov	CL,A
	_CleanUp	5,3,RA,NZ

;Mov A,(X)
ALIGN 16
OpcE6:
	_X
	Mov	A,[DPI]
	Mov	CL,A
	_CleanUp	3,1,RD,NZ

;Mov A,[dp+X]
ALIGN 16
OpcE7:
	_idpX
	Mov	A,[ABSL]
	Mov	CL,A
	_CleanUp	6,2,RA,NZ

;Mov A,[dp]+Y
ALIGN 16
OpcF7:
	_idpY
	Mov	A,[ABSL]
	Mov	CL,A
	_CleanUp	6,2,RA,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;MOV - Store Register in Memory
; N V P B H I Z C
;

;Mov dp,A
ALIGN 16
OpcC4:
	_dp
	Mov	[DPI],A
	_CleanUp	4,2,WD

;Mov dp,X
ALIGN 16
OpcD8:
	_dp
	Mov	[DPI],X
	_CleanUp	4,2,WD

;Mov dp,Y
ALIGN 16
OpcCB:
	_dp
	Mov	[DPI],Y
	_CleanUp	4,2,WD

;Mov dp+X,A
ALIGN 16
OpcD4:
	_dpX
	Mov	[DPI],A
	_CleanUp	5,2,WD

;Mov dp+Y,X
ALIGN 16
OpcD9:
	_dpY
	Mov	[DPI],X
	_CleanUp	5,2,WD

;Mov dp+X,Y
ALIGN 16
OpcDB:
	_dpX
	Mov	[DPI],Y
	_CleanUp	5,2,WD

;Mov !abs,A
ALIGN 16
OpcC5:
	_abs
	Mov	[ABSL],A
	_CleanUp	5,3,WA

;Mov !abs,X
ALIGN 16
OpcC9:
	_abs
	Mov	[ABSL],X
	_CleanUp	5,3,WA

;Mov !abs,Y
ALIGN 16
OpcCC:
	_abs
	Mov	[ABSL],Y
	_CleanUp	5,3,WA

;Mov !abs+X,A
ALIGN 16
OpcD5:
	_absX
	Mov	[ABSL],A
	_CleanUp	6,3,WA

;Mov !abs+Y,A
ALIGN 16
OpcD6:
	_absY
	Mov	[ABSL],A
	_CleanUp	6,3,WA

;Mov [dp+X],A
ALIGN 16
OpcC7:
	_idpX
	Mov	[ABSL],A
	_CleanUp	7,2,WA

;Mov [dp]+Y,A
ALIGN 16
OpcD7:
	_idpY
	Mov	[ABSL],A
	_CleanUp	7,2,WA

;Mov (X),A
ALIGN 16
OpcC6:
	_X
	Mov	[DPI],A
	_CleanUp	4,1,WD


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;MOV - Move Data to Memory
; N V P B H I Z C
;

;Mov dp,#imm
ALIGN 16
Opc8F:
	Mov	DL,[OP]
	_dp2
	Mov	[DPI],DL
	_CleanUp	5,2,WD

;Mov dp,dp
ALIGN 16
OpcFA:
	_dp
	Mov	DH,[DPI]
	Mov	DL,BL
	_dp2
	Mov	[DPI],DH
	Mov	DH,BH
	_CleanUp	5,2,RW


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;MOV - Load Byte with Auto Increase
; N V P B H I Z C
; *           *

;Mov A,(X)+
ALIGN 16
OpcBF:
	_X
	Inc	X
	Mov	A,[DPI]
	Mov	CL,A
	_CleanUp	4,1,RD,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;MOV - Store Byte with Auto Increase
; N V P B H I Z C
;

;Mov (X)+,A
ALIGN 16
OpcAF:
	_X
	Inc	X
	Mov	[DPI],A
	_CleanUp	4,1,WD


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;MOV1 - Load Absolute Bit Into Carry
; N V P B H I Z C
;               *

;Mov1 C,mem.bit
ALIGN 16
OpcAA:
	_mbit
	_GetC
	ShR	EDX,3
	_CleanUp	4,3,RBIT


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;MOV1 - Store Carry in Absolute Bit
; N V P B H I Z C
;

;Mov1 mem.bit,C
ALIGN 16
OpcCA:
	_abs
	Push	ECX
	Mov	CL,DH
	And	DH,1Fh
	ShR	CL,5
	Mov	BL,0FEh
	RoL	BL,CL
	And	[ABSL],BL
	Mov	BL,[_PSW(C)]
	ShL	BL,CL
	Or		[ABSL],BL
	Pop	ECX
	_CleanUp	6,3,WBIT


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;MOVW - Move Word
; N V P B H I Z C
; *           *

;MovW YA,dp
ALIGN 16
OpcBA:
	_dp
	Mov	YA,[DPI]
	Test	YA,YA
	SetNZ	CL
	Or		CL,Y
	_CleanUp	5,2,RD16,NZ

;MovW dp,YA (flags not affected)
ALIGN 16
OpcDA:
	_dp
	Mov	[DPI],YA
	_CleanUp	5,2,WD16


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;MUL - Multiply
; N V P B H I Z C
; *           *

;Mul YA
ALIGN 16
OpcCF:
	Mul	Y
	Test	YA,YA
	SetNZ	CL
	Or		CL,Y
	_CleanUp	9,1,na,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;NOT1 - Complement Absolute Bit
; N V P B H I Z C
;

;Not1 mem.bit
ALIGN 16
OpcEA:
	Mov	DX,[OP]
	RoL	DX,3
	BTC	[RAM],EDX
	ShR	EDX,3
	_CleanUp	5,3,WBIT


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;NOTC - Complement Carry Flag
; N V P B H I Z C
;               *

;NotC
ALIGN 16
OpcED:
	XOr	byte [_PSW(C)],1
	_CleanUp	3,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;OR - Bit-wise Logical Inclusive Or
; N V P B H I Z C
; *           *

;Or A,#imm
ALIGN 16
Opc08:
	Or 	A,[OP]
	Mov	CL,A
	_CleanUp	2,2,na,NZ

;Or A,dp
ALIGN 16
Opc04:
	_dp
	Or 	A,[DPI]
	Mov	CL,A
	_CleanUp	3,2,RD,NZ

;Or A,dp+X
ALIGN 16
Opc14:
	_dpX
	Or 	A,[DPI]
	Mov	CL,A
	_CleanUp	4,2,RD,NZ

;Or A,!abs
ALIGN 16
Opc05:
	_abs
	Or 	A,[ABSL]
	Mov	CL,A
	_CleanUp	4,3,RA,NZ

;Or A,!abs+X
ALIGN 16
Opc15:
	_absX
	Or 	A,[ABSL]
	Mov	CL,A
	_CleanUp	5,3,RA,NZ

;Or A,!abs+Y
ALIGN 16
Opc16:
	_absY
	Or 	A,[ABSL]
	Mov	CL,A
	_CleanUp	5,3,RA,NZ

;Or A,(X)
ALIGN 16
Opc06:
	_X
	Or 	A,[DPI]
	Mov	CL,A
	_CleanUp	3,1,RD,NZ

;Or A,[dp+X]
ALIGN 16
Opc07:
	_idpX
	Or 	A,[ABSL]
	Mov	CL,A
	_CleanUp	6,2,RA,NZ

;Or A,[dp]+Y
ALIGN 16
Opc17:
	_idpY
	Or 	A,[ABSL]
	Mov	CL,A
	_CleanUp	6,2,RA,NZ

;Or dp,#imm
ALIGN 16
Opc18:
	Mov	CL,[OP]
	_dp2
	Or		CL,[DPI]
	Mov	[DPI],CL
	_CleanUp	5,2,WD,NZ

;Or dp,dp
ALIGN 16
Opc09:
	_dp
	Mov	CL,[DPI]
	Mov	EDX,DPI
	_dp2
	Or		CL,[DPI]
	Mov	[DPI],CL
	_CleanUp	6,2,RW,NZ

;Or (X),(Y)
ALIGN 16
Opc19:
	_Y
	Mov	CL,[DPI]
	Mov	EDX,DPI
	_X
	Or		CL,[DPI]
	Mov	[DPI],CL
	_CleanUp	5,1,RW,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;OR1 - Or Carry with Absolute Bit
; N V P B H I Z C
;               *

;Or1 C,mem.bit
ALIGN 16
Opc0A:
	_mbit
	SetC	BL
	Or		[_PSW(C)],BL
	ShR	EDX,3
	_CleanUp	5,3,RBIT

;Or1 C,/mem.bit
ALIGN 16
Opc2A:
	_mbit
	SetNC	BL
	Or		[_PSW(C)],BL
	ShR	EDX,3
	_CleanUp	5,3,RBIT


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;PCALL - Call Procedure in Uppermost Page
; N V P B H I Z C
;

;PCall up
ALIGN 16
Opc4F:
	_PushPC	1
	Mov	DL,[OP]
	Mov	DH,0FFh
	Mov	ESI,EDX
	_CleanUp	6,na


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;POP - Pop Register Off Stack
; N V P B H I Z C
;

;Pop A
ALIGN 16
OpcAE:
	_Pop	A
	_CleanUp	4,1

;Pop X
ALIGN 16
OpcCE:
	_Pop	X
	_CleanUp	4,1

;Pop Y
ALIGN 16
OpcEE:
	_Pop	Y
	_CleanUp	4,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;POP - Pop Flags Off Stack
; N V P B H I Z C
; ? ? ? ? ? ? ? ?

;Pop PSW
ALIGN 16
Opc8E:
	_Pop	CL
	_UnpackPSW
	_CleanUp	4,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;PUSH - Push Register Onto Stack
; N V P B H I Z C
;

;Push A
ALIGN 16
Opc2D:
	_Push	A
	_CleanUp	4,1

;Push X
ALIGN 16
Opc4D:
	_Push	X
	_CleanUp	4,1

;Push Y
ALIGN 16
Opc6D:
	_Push	Y
	_CleanUp	4,1

;Push PSW
ALIGN 16
Opc0D:
	_PackPSW
	_Push	CL
	And	CL,82h
	XOr	CL,2
	_CleanUp	4,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;RET - Return From Call
; N V P B H I Z C
;

;Ret
ALIGN 16
Opc6F:
	_PopPC
	_CleanUp	5,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;RETI - Return From Interrupt
; N V P B H I Z C
; ? ? ? ? ? ? ? ?

;RetI
ALIGN 16
Opc7F:
	_Pop	CL
	_UnpackPSW
	_PopPC
	_CleanUp	6,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;ROL - Rotate 9-bits Left
; N V P B H I Z C
; *           * *

;RoL A
ALIGN 16
Opc3C:
	Cmp	DH,[_PSW(C)]
	RCL	A,1
	Mov	CL,A
	_CleanUp	2,1,na,NZC

;RoL dp
ALIGN 16
Opc2B:
	_dp
	Cmp	DH,[_PSW(C)]
	RCL	byte [DPI],1
	Mov	CL,[DPI]
	_CleanUp	4,2,WD,NZC

;RoL dp+X
ALIGN 16
Opc3B:
	_dpX
	Cmp	DH,[_PSW(C)]
	RCL	byte [DPI],1
	Mov	CL,[DPI]
	_CleanUp	5,2,WD,NZC

;RoL !abs
ALIGN 16
Opc2C:
	Cmp	DH,[_PSW(C)]
	_abs
	RCL	byte [ABSL],1
	Mov	CL,[ABSL]
	_CleanUp	5,3,WA,NZC


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;ROR - Rotate 9-bits Right
; N V P B H I Z C
; *           * *

;RoR A
ALIGN 16
Opc7C:
	Cmp	DH,[_PSW(C)]
	RCR	A,1
	Mov	CL,A
	_CleanUp	2,1,na,NZC

;RoR dp
ALIGN 16
Opc6B:
	_dp
	Cmp	DH,[_PSW(C)]
	RCR	byte [DPI],1
	Mov	CL,[DPI]
	_CleanUp	4,2,WD,NZC

;RoR dp+X
ALIGN 16
Opc7B:
	_dpX
	Cmp	DH,[_PSW(C)]
	RCR	byte [DPI],1
	Mov	CL,[DPI]
	_CleanUp	5,2,WD,NZC

;RoR !abs
ALIGN 16
Opc6C:
	Cmp	DH,[_PSW(C)]
	_abs
	RCR	byte [ABSL],1
	Mov	CL,[ABSL]
	_CleanUp	5,3,WA,NZC


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;SBC - Subtract with Carry
; N V P B H I Z C
; * *     *   * *

;SbC A,#imm
ALIGN 16
OpcA8:
	Cmp	byte [_PSW(C)],1
	SbB	A,[OP]
	Mov	CL,A
	_CleanUp	2,2,na,NVHZCs

;SbC A,dp
ALIGN 16
OpcA4:
	_dp
	Cmp	byte [_PSW(C)],1
	SbB	A,[DPI]
	Mov	CL,A
	_CleanUp	3,2,RD,NVHZCs

;SbC A,dp+X
ALIGN 16
OpcB4:
	_dpX
	Cmp	byte [_PSW(C)],1
	SbB	A,[DPI]
	Mov	CL,A
	_CleanUp	4,2,RD,NVHZCs

;SbC A,!abs
ALIGN 16
OpcA5:
	_abs
	Cmp	byte [_PSW(C)],1
	SbB	A,[ABSL]
	Mov	CL,A
	_CleanUp	4,3,RA,NVHZCs

;SbC A,!abs+X
ALIGN 16
OpcB5:
	_absX
	Cmp	byte [_PSW(C)],1
	SbB	A,[ABSL]
	Mov	CL,A
	_CleanUp	5,3,RA,NVHZCs

;SbC A,!abs+Y
ALIGN 16
OpcB6:
	_absY
	Cmp	byte [_PSW(C)],1
	SbB	A,[ABSL]
	Mov	CL,A
	_CleanUp	5,3,RA,NVHZCs

;SbC A,(X)
ALIGN 16
OpcA6:
	_X
	Cmp	byte [_PSW(C)],1
	SbB	A,[DPI]
	Mov	CL,A
	_CleanUp	3,1,RD,NVHZCs

;SbC A,[dp+X]
ALIGN 16
OpcA7:
	_idpX
	Cmp	byte [_PSW(C)],1
	SbB	A,[ABSL]
	Mov	CL,A
	_CleanUp	6,2,RA,NVHZCs

;SbC A,[dp]+Y
ALIGN 16
OpcB7:
	_idpY
	Cmp	byte [_PSW(C)],1
	SbB	A,[ABSL]
	Mov	CL,A
	_CleanUp	6,2,RA,NVHZCs

;SbC dp,#imm
ALIGN 16
OpcB8:
	Mov	CL,[OP]
	_dp2
	Cmp	byte [_PSW(C)],1
	SbB	[DPI],CL
	Mov	CL,[DPI]
	_CleanUp	5,2,WD,NVHZCs

;SbC dp,dp
ALIGN 16
OpcA9:
	_dp
	Mov	CL,[DPI]
	Mov	EDX,DPI
	_dp2
	Cmp	byte [_PSW(C)],1
	SbB	[DPI],CL
	Mov	CL,[DPI]
	_CleanUp	6,2,RW,NVHZCs

;SbC (X),(Y)
ALIGN 16
OpcB9:
	_Y
	Mov	CL,[DPI]
	Mov	EDX,DPI
	_X
	Cmp	byte [_PSW(C)],1
	SbB	[DPI],CL
	Mov	CL,[DPI]
	_CleanUp	5,1,RW,NVHZCs


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;SET1 - Set Bit
; N V P B H I Z C
;

;Set1 dp.?
ALIGN 16
Opc02:
Opc22:
Opc42:
Opc62:
Opc82:
OpcA2:
OpcC2:
OpcE2:
	_dp
	ShR	EDX,5
	BTS	[DPI],EDX
	_CleanUp	4,2,WD


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;SETC - Set Carry Flag
; N V P B H I Z C
;               1

;SetC
ALIGN 16
Opc80:
	Mov	byte [_PSW(C)],1
	_CleanUp	2,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;SETP - Set Direct Page Flag
; N V P B H I Z C
;     1     0

;SetP
ALIGN 16
Opc40:
	Mov	BH,1
	Mov	[_PSW(I)],DH
	Mov	[_PSW(P)],BH
	_CleanUp	2,1


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;SLEEP - Suspend the SPC700 and Timers
; N V P B H I Z C
;

;Sleep
ALIGN 16
OpcEF:
	;Fall through to STOP

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;STOP - Stop the SPC700 Clock
; N V P B H I Z C
;

;Stop
OpcFF:
	Or		byte [_Var(dbgOpt)],SPC_HALT			;Halt emulation
	Test	byte [_Var(dbgOpt)],SPC_STOP			;If the user only wants debug calls on errors and...
	SetZ	DL
	Dec	EDX
	Test	EDX,[_Var(pDebug)]						;...has specified a debugging vector then call vector
	JNZ	SPCBreak
	Jmp	SPCHalt										; otherwise, exit fetch loop


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;SUBW - Subtract Word
; N V P B H I Z C
; * *     *   * *

;SubW YA,dp
ALIGN 16
Opc9A:
	_dp
	Mov	DX,YA
	Sub	DX,[DPI]
	SetNZ	CL
	Or		CL,DH
	Sub	A,[DPI]
	SbB	Y,[1+DPI]									;Half-carry is based on the subtraction made from Y
	_CleanUp	5,2,RD16,NVHZCs


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;TCALL - Call Pointer in Vector Table
; N V P B H I Z C
;

	;-----------------------------------------
	;Table Call
	;   %1 - Table Index

	%macro TCall 1
		_PushPC
		Mov		PC,word [((15-%1)*2)+RAM+ipl]
		_CleanUp	8,na
	%endmacro

;	Mov	EBX,[_Var(regSP)]
;	ShR	EDX,3
;	Mov	[EBX-1],PC
;	XOr	EDX,0FFDEh
;	Sub	byte [_Var(regSP)],2
;	Mov	PC,[EDX+RAM]
;	Mov	BH,[_PSW(P)]

;TCall 0
ALIGN 16
Opc01:
	TCall	0

;TCall 1
ALIGN 16
Opc11:
	TCall	1

;TCall 2
ALIGN 16
Opc21:
	TCall	2

;TCall 3
ALIGN 16
Opc31:
	TCall	3

;TCall 4
ALIGN 16
Opc41:
	TCall	4

;TCall 5
ALIGN 16
Opc51:
	TCall	5

;TCall 6
ALIGN 16
Opc61:
	TCall	6

;TCall 7
ALIGN 16
Opc71:
	TCall	7

;TCall 8
ALIGN 16
Opc81:
	TCall	8

;TCall 9
ALIGN 16
Opc91:
	TCall	9

;TCall 10
ALIGN 16
OpcA1:
	TCall	10

;TCall 11
ALIGN 16
OpcB1:
	TCall	11

;TCall 12
ALIGN 16
OpcC1:
	TCall	12

;TCall 13
ALIGN 16
OpcD1:
	TCall	13

;TCall 14
ALIGN 16
OpcE1:
	TCall	14

;TCall 15
ALIGN 16
OpcF1:
	TCall	15


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;TCLR1 - Test and Clear Bits with A
; N V P B H I Z C
; *           *

;TClr1 !abs
ALIGN 16
Opc4E:
	_abs
	Mov	CL,[ABSL]
	And	CL,A
	XOr	[ABSL],CL
	_CleanUp	6,3,WA,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;TSET1 - Test and Set Bits with A
; N V P B H I Z C
; *           *

;TSet1 !abs
ALIGN 16
Opc0E:
	_abs
	Mov	CL,[ABSL]
	Or 	[ABSL],A
	And	CL,A
	_CleanUp	6,3,WA,NZ


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;XCN - Exchange Nybbles
; N V P B H I Z C
; *           *

;XCN A
ALIGN 16
Opc9F:
	RoR	A,4
	Mov	CL,A
	_CleanUp	5,1,na,NZ


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Function Register Handlers
;
;These are specific functions to handle data written to the function registers (F0h to FFh)
;Upon entrance to each handler EDX == the function register number (0 - F)

ALIGN 16
FuncNext:
;This is a bit of code to handle 16-bit writes to the F0h - FFh range.
;During a 16-bit write, EBP is loaded with the address here, and the address of the handler for the
;low byte is pushed onto the stack.  After handling the low byte, the emulator will jump here where
;EBP will be restored then control will be passed to the handler for the high byte.

%if DEBUG
	XOr	EDX,EDX
	Test	byte [_Var(dbgOpt)],SPC_TRACE
	SetNZ	DL
	Dec	EDX
	And	EDX,SPCFetch-SPCTrace
	LEA	EBP,[EDX+SPCTrace]
%else
	Mov	EBP,SPCFetch
%endif
	Pop	EDX
	_Profile(func16)
	Inc	EDX
	Cmp	EDX,10h
	JE		short .NoReg
	Jmp	[EDX*4+funcWTab]
.NoReg:
	Jmp	EBP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Test (Write)
;   Funky things happen when the test register is written to.  Apparently the timers only work if
;   bit 3 is set.  Setting bit 2 or clearing bit 1 halts execution.  Setting bits 7-4 slows down the
;   processor by varying amounts.
;   Reading from the register always returns 0 <verified by Blargg on 04.10.03>

ALIGN 16
Func0w:
	_Profile(funcw)
	Mov	DL,[RAM+testReg]
	And	DL,0F6h
	XOr	DL,2
	JNZ	OpcFF											;If bits 7-4,2-1 are modified, halt processor
	Mov	[RAM+testReg],DH							;Reset register to 0
	Jmp	EBP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Control (Write)
;   Checks control register for ROM reading enable, port reset, and timer enable.

ALIGN 16
Func1w:
	_Profile(funcw)

	Mov	BL,[RAM+control]

	;ROM access ------------------------------
	Mov	DL,[_Var(oldCtrl)]
	XOr	DL,BL											;Did ROM access change?
	JNS 	short .NoRA									;  No

		Push	ECX,ESI									;Save X and PC
		LEA	ESI,[_Var(extraRAM)]					;Setup registers to move data from Extra RAM to
		Mov	DI,ipl									; IPL region

		Test	BL,BL										;Is ROM readable?
		Mov	ECX,10h
		JNS	short .NoRR								;  No, Perform move as intended
			XChg	ESI,EDI								;Reverse original operation, move IPL to Extra RAM
			Rep	MovSD

			Mov	CL,10h								;Setup registers to move ROM program into IPL region
			LEA	EDI,[ESI-40h]
			Mov	ESI,iplROM
		.NoRR:

		Rep	MovSD										;Move iplROM or extraRAM to IPL ROM region

		Mov	EDI,[pAPURAM]
		Pop	ESI,ECX

	.NoRA:

	;Clear ports -----------------------------
	Test	BL,30h										;Was a clear ports command written?
	JZ 	short .NoCP									;  No
		Mov	EDX,EBX
		Not	EDX										;Reverse command bits
		ShL	EDX,26									;Create a mask based on bits 5 & 4
		SAR	EDX,15
		SAR	DX,15
		And	dword [RAM+port0],EDX				;Reset in-ports
		And	dword [_Var(inPortCp)],EDX
	.NoCP:

	;Reset/Enable timers ---------------------
	MovZX	EDX,byte [_Var(oldCtrl)]				;DL = Timers currently enabled
	And	BL,87h
	Mov	[_Var(oldCtrl)],BL						;Store new timers enabled
	Mov	[RAM+control],BL

	Not	DL
	And	DL,DL											;CL=1 if new timer=1 and old timer=0
	And	DL,7
	JZ 	short .NoTR									;  Quit if no timers are reset
		ShR	DL,1										;Do we need to reset timer 0?
		JNC 	short .NoRT0							;  No
			Mov	BL,[_Var(timer0)]					;Get timer register
			Mov	[RAM+c0],DH							;Reset counter
			Mov	[_Var(t0Step)],BL					;Reset timer step
		.NoRT0:

		ShR	DL,1
		JNC	short .NoRT1
			Mov	BL,[_Var(timer1)]
			Mov	[RAM+c1],DH
			Mov	[_Var(t1Step)],BL
		.NoRT1:

		ShR	DL,1
		JNC	short .NoRT2
			Mov	BL,[_Var(timer2)]
			Mov	[RAM+c2],DH
			Mov	[_Var(t2Step)],BL
		.NoRT2:

		;Branch free method (no longer works since timer values have to be decreased)
;		ShL	EDX,29									;EBX = abc00000000000000000000000000000
;		SAR	EDX,7										;EBX = aaaaaaaabc0000000000000000000000
;		ShR	EDX,8										;EBX = 00000000aaaaaaaabc00000000000000
;		SAR	DX,7										;EBX = 00000000aaaaaaaabbbbbbbbc0000000
;		SAR	DL,7										;EBX = 00000000aaaaaaaabbbbbbbbcccccccc
;		Not	EDX										;EBX = 11111111AAAAAAAABBBBBBBBCCCCCCCC
;
;		And	[t0Step],EDX							;Reset internal up counters
;		And	[RAM+c0],EDX							;Reset counters
;		Not	EDX
;		And	EDX,[RAM+t0]							;Get new timer values
;		Or 	[t0Step],EDX							;Update internal up counters
	.NoTR:
	Jmp	EBP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;DSP Data (Write)
;   Updates the DSP RAM with the value written to the data port, and calls the DSP emulator to
;   handle the new data.

ALIGN 16
Func3w:
	_Profile(funcw)
	Mov	BL,CL
	Push	EAX,EBX
	MovZX	EBX,byte [RAM+dspAddr]

%if PROFILE
	_ProfEnd
	Mov	EDX,EBX
	_Profile(dspw)
%endif

	Mov	AL,[RAM+dspData]
	Call	SetDSPReg_A

%if PROFILE
	MovSX	EDX,byte [RAM+dspAddr]
	Test	EDX,EDX
	JS		short .NoWrite
		Add	[EDX*4+profile+Profile.dspu],EAX
	.NoWrite:

	And	EDX,7Fh										;Update value at F3h.  If profiling is disabled, fall
	Mov	DL,[EDX+dsp]								; through and let Func2w handle it.
	Mov	[RAM+dspData],DL
%endif

	Pop	EBX,EAX
	Mov	CL,BL

%if PROFILE
	_ProfStart
	Jmp	EBP
%endif


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;DSP Address (Write)
;   Loads 0F3h with value indexed in DSP RAM.  In the SNES, reads from registers 80-FF mirror
;   registers 00-7F.

ALIGN 16
Func2w:
	_Profile(funcw)
	Mov	DL,[RAM+dspAddr]							;EDX = DSP register
	And	EDX,7Fh										;The MSB of the address is ignored when getting data
	Mov	DL,[EDX+dsp]								;Get byte from DSP RAM
	Mov	[RAM+dspData],DL							;Store byte in DSP data reg
	Jmp	EBP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Out Ports 0-3 (Write)
;   Moves data to out port memory, and replaces register with in port value

ALIGN 16
Func4w:
Func5w:
Func6w:
Func7w:
	_Profile(funcw)
	And	EDX,3
	Mov	BL,[EDX+RAM+port0]						;Copy byte to out port
	Mov	[EDX+_Var(outPort)],BL
	Mov	BL,[EDX+_Var(inPortCp)]					;Replace byte in RAM with in port
	Mov	[EDX+RAM+port0],BL
	Jmp	EBP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Unused (Write)
;   No special handling is associated with registers 0F8h and 0F9h

ALIGN 16
Func8w:
Func9w:
	_Profile(funcw)
	Jmp	EBP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Timers 0-2 (Write)
;   Reading the timer registers always returns 00 <verified by Blargg on 04.10.03>

ALIGN 16
FuncAw:
FuncBw:
FuncCw:
	_Profile(funcw)
	Mov	BL,0
	XChg	BL,[EDX+RAM+0F0h]
	Dec	BL
	Mov	[EDX+RAM-10h],BL
	Jmp	EBP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Counters 0-2 (Write)
;  Writing to a counter register clears it <verified by Blargg on 04.10.03>

ALIGN 16
FuncDw:
FuncEw:
FuncFw:
	_Profile(funcw)
	Mov	[EDX+RAM+0F0h],DH
	Jmp	EBP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;DSP Data (Read)
;  Break emulation of the SPC700 so the DSP can emulate, if the DSP is being read from.  Breaking
;  only happens on registers that get updated by the DSP (ENVX, OUTX, and ENDX).

ALIGN 16
Func3r:
%if PROFILE
	_Profile(funcr)
	Mov	DL,[RAM+dspAddr]
	And	EDX,7Fh
	_Profile(dspr)
%endif

%if DSPBK

%if PROFILE = 0
	Mov	DL,[RAM+dspAddr]
	And	DL,7Fh
%endif

	Cmp	DL,7Ch										;Is the register being read ENDX?
	JE		short .Break
	And	DL,0Fh
	Sub	DL,8											;ENVX or OUTX?
	Cmp	DL,1
	JA		short .NoBreak
	.Break:

%if DSPINTEG

%if PROFILE
	Inc	dword [4+profile+Profile.update]
	_ProfEnd
	Call	UpdateDSP
	_ProfStart
%else
	Push	EBP											;Push return address
	Jmp	UpdateDSP
%endif	;PROFILE

%else
	Push	EAX
	Mov	EBP,SPCExit
	Mov	EAX,[_Var(clkLeft)]
	Add	[_Var(clkExec)],EAX
	Sub	[_Var(clkLeft)],EAX
	Pop	EAX
%endif	;DSPINTEG

	.NoBreak:
%endif	;DSPBK

	Jmp	EBP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;  Reading from most registers doesn't do anything

ALIGN 16
Func0r:
Func1r:
Func2r:
Func4r:
Func5r:
Func6r:
Func7r:
Func8r:
Func9r:
FuncAr:
FuncBr:
FuncCr:
	_Profile(funcr)
	Jmp	EBP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Counters 0-2 (Read)
;  If the speed hack is enabled, this skips emulated cycles so the next counter to increase will
;  increase on the next 64kHz pulse.  For games that poll the counters waiting for an increase, this
;  guarantees the program will never have to poll the counters more than three consecutive times,
;  which increases emulation speed.  However, somewhere in this code is a flaw in my logic.  For
;  some songs the speed hack is buring cycles without the music advancing, and for today's >1GHz
;  processors the performance increase is negligible.

ALIGN	16
FuncDr:
FuncEr:
FuncFr:
	_Profile(funcr)
%if SPEED
	Or		DL,0F0h
	Test	byte [EDX+EDI],0Fh						;Is counter > 0?
	Mov	[EDX+EDI],DH								;Reset counter
	JNZ	.Quit											;  Yes, No need to speed up then

	Test	byte [_Var(oldCtrl)],7					;Are any timers enabled?
	JZ		.Quit

	Push	EAX,ECX

	;Some poorly coded games check the counters and keep a running total of the increases while
	;processing the song data.  If the counters are increased during each read, the counter
	;increases will accumulate quickly and the song will play at warp speed.
	;
	;To overcome this, if less than 64 CPU cycles go by between counter reads, we'll assume that
	;the program is polling the counters, otherwise we'll disable the speed hack.

	Mov	ECX,[_Var(t64Last)]
	Mov	EAX,[_Var(t64Cnt)]
	Add	ECX,(64 * CPU_CYC) / T64_CYC			;Add 64 CPU cycles to the last count
	Mov	[_Var(t64Last)],EAX
	Cmp	EAX,ECX
	JAE	.NoHack										;If at least 64 CPU cycles have passed, quit

	;Since we don't know which timers the program is using, the hack needs to advance the next
	;closest counter, regardless of which counter is actually being read.  So the first step is
	;to figure out how many 64kHz pulses are needed for each counter to increase, then choose the
	;smallest value.

	MovZX	EAX,byte [_Var(oldCtrl)]				;Determine number of 64kHz pulses until counter 0 or 1
	XOr	ECX,ECX										; increases, whichever is less.
	Test	AL,2
	SetNZ	CL
	And	AL,1
	Dec	CL
	Dec	AL												;If the timer is disabled, default to max time
	Or		CL,[_Var(t1Step)]							;ECX = (control & 2) ? t1Step : 0FFh
	Or		AL,[_Var(t0Step)]							;EAX = (control & 1) ? t0Step : 0FFh

	Sub	EAX,ECX										;if (EAX >= ECX) EAX = ECX
	CDQ
	And	EAX,EDX
	MovZX	EDX,byte [_Var(t8kHz)]
	Add	EAX,ECX
	LEA	EAX,[EAX*8+EDX]							;EAX = EAX * 8 + t8kHz

	Test	byte [_Var(oldCtrl)],4					;ECX = (control & 4) ? t2Step : 0FFh
	SetNZ	CL
	Dec	CL
	Or		CL,[_Var(t2Step)]

	Sub	EAX,ECX										;Does counter 0 or 1 have less time than counter 2?
	CDQ
	And	EAX,EDX
	Add	EAX,ECX
	JZ		.NoHack										;No pulses to increase, so quit

	;Once we've figured out the least number of cycles needed for a counter increase, we need to
	;make sure we have enough total cycles left to emulate.  Then make sure we have enough cycles
	;to increase the 64kHz timer.

	LEA	EAX,[EAX*3]									;Convert EAX to clock cycles
	Mov	ECX,[_Var(clkTotal)]
	ShL	EAX,7
	Sub	ECX,[_Var(clkExec)]
	Add	ECX,[_Var(clkLeft)]						;ECX = clock cycles left to emulate

	Sub	EAX,ECX										;if (EAX >= ECX) EAX = ECX
	CDQ
	And	EAX,EDX
	Add	EAX,ECX

	Cmp	EAX,T64_CYC									;Are there enough cycles for a 64kHz pulse?
	JB		short .NoHack								;  No, Quit since nothing will be modified

	;Now's the time to actually skip emulating clock cycles.  The number of pulses are added to
	;t64Cnt and the cycles added to clkTotal, as if they really were emulated.  If timer 2 is
	;enabled, the pulses are then subtracted from t2Step.  Next the number of pulses is divided by 8
	;to give us the number of 8kHz pulses to skip.  Since up to 7 64kHz pulses could occur before one
	;8kHz pulse, the remainder is subtracted from t8kHz.  If timers 1 and 0 are enabled, pulses are
	;subtracted from t1Step and t0Step respectively.

	XOr	EDX,EDX										;Convert clock cycles back into 64kHz pulses, in case
	Mov	ECX,T64_CYC									; the value changed.
	Div	ECX
	Add	[_Var(t64Cnt)],EAX						;Add pulses to 64kHz counter

	LEA	EDX,[EAX*3]									;clkTotal -= EAX * 384
	ShL	EDX,7
	Mov	CL,[_Var(oldCtrl)]
	Sub	[_Var(clkTotal)],EDX

	Test	CL,4											;if (control & 4) t2Step -= AL
	SetZ	DL
	Dec	DL
	And	DL,AL											;DL = (control & 4) ? AL : 0
	Sub	[_Var(t2Step)],DL

	ShL	EAX,5											;AH = EAX / 8  (number of 8kHz ticks to skip)
	Test	CL,2
	SetZ	DL
	And	CL,1
	ShR	AL,5											;AL = EAX % 8  (num 64kHz ticks left until 8kHz tick)
	XOr	CL,1
	Dec	DL
	Dec	CL

	Sub	[_Var(t8kHz)],AL							;t8kHz -= EAX % 8
	AdC	AH,0
	And	DL,AH											;DL = (control & 2) ? (EAX / 8) : 0
	And	CL,AH											;CL = (control & 2) ? (EAX / 8) : 0
	And	byte [_Var(t8kHz)],7
	Sub	[_Var(t1Step)],DL
	Sub	[_Var(t0Step)],CL

	.NoHack:
	Pop	ECX,EAX

	.Quit:
%else
	Mov	[EDX+RAM+0F0h],DH							;Reset counter
%endif
	Jmp	EBP