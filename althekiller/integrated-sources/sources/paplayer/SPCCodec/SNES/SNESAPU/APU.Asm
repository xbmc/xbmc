;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Super Nintendo Entertainment System(tm) Audio Processing Unit Emulator
;                                                          Copyright (C)2003-06 Alpha-II Productions
;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ

CPU	386
BITS	32


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Header files

%include	"../../Macro.Inc"
%include	"SPC700.Inc"
%include	"DSP.Inc"
%define INTERNAL
%include	"APU.Inc"

%if DSPINTEG
EXTERN SetEmuDSP										;DSP.Asm - Used to set EmuDSP parameters
%endif



;ßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßß
; Data and Variables

SECTION .bss ALIGN=4

	cycLeft	resd	1									;Clock cycles left to emulate in EmuAPU loop
	smpDec	resd	1									;Unused clocks from cycle to sample conversion
	smpRate	resd	1									;Output sample rate (used by sound card and DSP)
	smpRAdj	resd	1									;Sample rate adjustment (16.16)



;ßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßßß
; Code

SECTION .text ALIGN=16

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Initialize Audio Processing Unit

PROC InitAPU

	Call	InitSPC
	Call	InitDSP

	Mov	dword [smpRate],32000
	Mov	dword [smpRAdj],10000h

	Call	SetAPUSmpClk,[smpRAdj]

	Mov	EAX, \
			CPU_CYC | \
			(DEBUG << 8) | (DSPINTEG << 9) | \
			(HALFC << 16) | (CNTBK << 17) | (SPEED << 18) | (IPLW << 19) | (DSPBK << 20) | (PROFILE << 21) | \
			(MMETER << 24) | (VMETER << 25) | (STEREO << 26)

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Reset Audio Processor

PROC ResetAPU

	Call	ResetSPC
	Call	ResetDSP

	And	dword [cycLeft],0
	And	dword [smpDec],0

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Load SPC File

PROC LoadSPCFile, pFile
USES ALL

	Call	ResetAPU

	Mov	ESI,[%$pFile]

	Add	ESI,100h										;memcpy(pAPURAM, &spc[0x100], 0x10000)
	Mov	EDI,[pAPURAM]
	Mov	ECX,10000h / 4
	Rep	MovSD

	Mov	EDI,dsp										;memcpy(&dsp, &spc[0x10100], 128)
	Or		ECX,128 / 4
	Rep	MovSD

	Add	ESI,40h										;memcpy(xram, &spc[0x101C0], 64)
	Mov	EDI,[pAPURAM]
	Add	EDI,-80h
	Or		ECX,40h / 4
	Rep	MovSD

	Mov	ESI,[%$pFile]
	MovZX	EAX,byte [2Bh+ESI]						;SP
	MovZX	ECX,byte [2Ah+ESI]						;PSW
	MovZX	EDX,byte [28h+ESI]						;X
	MovZX	EBX,byte [29h+ESI]						;Y
	MovZX	EDI,byte [27h+ESI]						;A
	MovZX	ESI,word [25h+ESI]						;PC
	Call	FixSPCLoad,ESI,EDI,EBX,EDX,ECX,EAX
	Call	FixDSPLoad

	Call	SetDSPAAR,-1,-1,-1,-1

ENDP

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Save Audio Processor State

PROC SaveAPU, pSPC, pDSP

	Mov	EAX,[%$pSPC]
	Test	EAX,EAX
	JZ		short .NoSPC
		Call	SaveSPC,EAX
	.NoSPC:

	Mov	EAX,[%$pDSP]
	Test	EAX,EAX
	JZ		short .NoDSP
		Call	SaveDSP,EAX
	.NoDSP:

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Restore Audio Processor State

PROC RestoreAPU, pSPC, pDSP

	Mov	EAX,[%$pSPC]
	Test	EAX,EAX
	JZ		short .NoSPC
		Call	RestoreSPC,EAX
	.NoSPC:

	Mov	EAX,[%$pDSP]
	Test	EAX,EAX
	JZ		short .NoDSP
		Call	RestoreDSP,EAX
	.NoDSP:

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Set Audio Processor Options

PROC SetAPUOpt, mixtype, chn, bits, rate, inter, opts

	Mov	EAX,[%$rate]								;Is a rate specified?
	Cmp	EAX,-1										;if (rate!=-1)
	JE		short .KeepRate

		Push	ECX,EDX
		Mov	EAX,[%$rate]

		Mov	ECX,8000									;if (rate < 8000) rate = 8000;
		Sub	EAX,ECX
		CDQ
		Not	EDX
		And	EAX,EDX
		Add	EAX,ECX

		Mov	ECX,192000								;if (rate > 192000) rate = 192000;
		Sub	EAX,ECX
		CDQ
		And	EAX,EDX
		Add	EAX,ECX

		Mov	[smpRate],EAX
		Pop	EDX,ECX

	.KeepRate:

	Pop	EBP
	Jmp	SetDSPOpt									;Set options in DSP emulator

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Set Audio Processor Sample Clock

PROC SetAPUSmpClk, speed
USES EDX,ECX

	Mov	EAX,[%$speed]

	Mov	ECX,1000h									;if (speed < 4096) speed = 4096;
	Sub	EAX,ECX
	CDQ
	Not	EDX
	And	EAX,EDX
	Add	EAX,ECX

	Mov	ECX,100000h									;if (speed > 1048576) speed = 1048576;
	Sub	EAX,ECX
	CDQ
	And	EAX,EDX
	Add	EAX,ECX

	Mov	[smpRAdj],EAX

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Emulate Audio Processing Unit

PROC EmuAPU, pBuf, cyc, smp
LOCALS rate,fcw
USES ECX,EDX,EBX,EDI

%if PROFILE
	Call	StartAPUProfile,Profile.apuTSC
%endif

	;Switch the FPU into single precision mode
	;According to the Intel docs, this is faster since the floating-point mixing routine only
	; requires single precision accuracy.  However, on my Pentium 4 the greatest performance
	; increase I've seen is 4 percent.

	FStCW	[%$fcw]
	FStCW	[2+%$fcw]
	And	word [%$fcw],~0300h						;Switch to single precision operation
	FLdCW	[%$fcw]

	Mov	EAX,[smpRate]								;rate = (smpRate << 16) / smpRAdj
	Mov	EDX,EAX
	ShL	EAX,16
	ShR	EDX,16
	Div	dword [smpRAdj]
	Mov	EBX,EAX

	Mov	ECX,APU_CLK

%if DSPINTEG
	;Fixup samples/cycles --------------------
	Cmp	dword [%$smp],0
	JZ		short .NoSamples

		Call	SetEmuDSP,[%$pBuf],[%$smp],EBX

	Mov	EAX,[%$cyc]
	Test	EAX,EAX
	JNZ	short .Cycles

		Mov	EAX,[%$smp]								;Calculate number of cycles based on samples
		Mul	ECX										;cycles = (APU_CLK * len) / smpREmu
		Div	EBX

	.Cycles:
		Add	[cycLeft],EAX
		JLE	short .NoCycles
		Jmp	short .Emulate

	.NoSamples:											;Calculate number of samples based on cycles
		Mov	EAX,[%$cyc]
		Add	[cycLeft],EAX
		Retc	LE

		Mov	EAX,[cycLeft]
		Mul	EBX										;samples = (smpREmu * len) / APU_CLK
		Div	ECX
		Call	SetEmuDSP,[%$pBuf],EAX,EBX

	.Emulate:

	;Emulate APU -----------------------------
	Call	EmuSPC,[cycLeft]
	Mov	[cycLeft],EAX

	.NoCycles:
	Call	SetEmuDSP,0,0,0							;Create any remaining samples

%else

	;If samples were passed, convert to clock cycles
	Mov	EAX,[%$cyc]
	Test	EAX,EAX
	JNZ	short .HaveCycles
		Mov	EAX,[%$smp]
		Mul	ECX										;cycles = (APU_CLK * len) / smpREmu
		Div	EBX
	.HaveCycles:
	Add	[cycLeft],EAX
	JLE	short .HaveSamples

	Cmp	dword [%$smp],0
	JNZ	short .HaveSamples
		Mov	EAX,[cycLeft]
		Mul	EBX
		Div	ECX
		Mov	[%$smp],EAX
	.HaveSamples:

	;Emulate APU -----------------------------
	Mov	[%$rate],EBX
	XOr	EBX,EBX										;Number of samples generated
	Mov	EDI,[%$pBuf]
	.Next:
		Mov	EAX,[cycLeft]
		Test	EAX,EAX
		JLE	short .Done

		;SPC700 -------------------------------
		Mov	EDX,EAX
 		Call	EmuSPC,EAX
 		Mov	[cycLeft],EAX

		;DSP ----------------------------------
		Sub	EDX,EAX									;Calculate number of samples to create
		Mov	EAX,EDX									;EAX = number of cycles emulated
		Mul	dword [%$rate]
		Add	EAX,[smpDec]
		AdC	EDX,0
		Div	ECX										;samples = (((cycles - cycLeft) * smpREmu) + smpDec) / APU_CLK
		Mov	[smpDec],EDX

		Add	EBX,EAX									;size += samples
		Cmp	EBX,[%$smp]								;Sometimes sample count will go over by one
		JBE	short .LenOK
			Add	EAX,[%$smp]
			Sub	EAX,EBX
			Mov	EBX,[%$smp]
		.LenOK:

		Call	EmuDSP,EDI,EAX							;pBuf = EmuDSP(pBuf,samples)
		Mov	EDI,EAX

	Jmp	short .Next

	.Done:

	;Make sure enough samples were created to fill buffer
	Sub	EBX,[%$smp]
	Retc	Z,EDI

	Neg	EBX
	Call	EmuDSP,EDI,EBX
%endif

	FLdCW	[2+%$fcw]									;Restore FPU precision to previous degree

%if PROFILE
	Call	EndAPUProfile,Profile.apuTSC
%endif

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Seek to Position

PROC SeekAPU, time, fast
USES ECX,EDX

	XOr	EDX,EDX
	Mov	EAX,[%$time]								;numSeconds = time / 64000
	Test	EAX,EAX
	Retc	Z
	Mov	ECX,64000
	Div	ECX
	Mov	ECX,EAX
	IMul	EDX,APU_CLK/64000							;EDX = (time % 64000) * (APU_CLK / 64000)

	Test	byte [%$fast],-1							;Fast mode completely bypasses the DSP emulation
	JZ		short .Slow

		Add	EDX,[cycLeft]
		Mov	EAX,EDX
		.EmuSPC:

			Add	EAX,APU_CLK							;Emulate SPC for 1 second
			.Next:
				Call	EmuSPC,EAX
			Test	EAX,EAX
			JG		short .Next

		Dec	ECX
		JG		short .EmuSPC
		Mov	[cycLeft],EAX

		Jmp	short .Done

	.Slow:
		Test	EDX,EDX
		JZ		short .EmuAPU

		Call	EmuAPU,0,EDX,0
		Test	ECX,ECX
		JZ		short .Done

		.EmuAPU:
			Call	EmuAPU,0,APU_CLK,0
		Dec	ECX
		JNZ	short .EmuAPU

	.Done:

	Call	FixDSPSeek,[%$fast]						;Fixup DSP after seeking

ENDP


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Shutdown Audio Processing Unit

PROC ShutAPU

ENDP