;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Program:    SNES Digital Signal Processor Emulator
;Platform:   80386 / MMX
;Programmer: Anti Resonance
;
;"SNES" and "Super Nintendo Entertainment System" are trademarks of Nintendo Co., Limited and its
;subsidiary companies.
;
;This library is free software; you can redistribute it and/or modify it under the terms of the
;GNU Lesser General Public License as published by the Free Software Foundation; either version
;2.1 of the License, or (at your option) any later version.
;
;This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
;without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
;See the GNU Lesser General Public License for more details.
;
;You should have received a copy of the GNU Lesser General Public License along with this
;library; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
;Boston, MA  02111-1307  USA
;
;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Revision History:
;
;  2005.10.28 SNESAPU v3.0
;  + Added 8-point sinc interpolation
;  + Added per voice anti-aliasing
;  + Added quadraphonic output to the floating-point mixing routine
;  + Removed the branches in the volume ramping code in EmuDSPF
;  + Updated interpolation functions so PlaySrc could use them
;  + Added -6dB attenuation to PlaySrc, so single sounds aren't so loud compared to the relative
;    quietness of songs
;  - Volume ramping was disabled when stereo controls were turned off (not sure why)
;  - Fixed reverse stereo not working when stereo conrols were enabled, for real this time
;  + Changed debugging interface
;  + Removed option for generating Gaussian curve
;  + DSP register jump table gets initialized at run-time
;  - Fixed a bug in EmuDSP where muting the output buffer would crash
;  - Fixed a bug when resetting the DSP with the FLG register
;  + Made floating-point constants local variables in most functions
;  + Created macros to calculate relative displacments for some global variables
;  - Fixed DSPIn to store the new value in the registers after UpdateDSP is called.  Also moved
;    UpdateDSP to get called only if the register handler will be called.
;
;  2004.08.01 SNESAPU v2.1
;  + Improved monaural volume setting
;  + Removed BRR look-up table
;  + Moved all checks for SPC_NODSP from SPC700.Asm
;  - Envelope height in direct gain mode could be garbage
;  - Added a missing 16-bit clamp to the MMX Gaussian interpolation
;  - Added a 16-bit clamp to the MMX FIR filter to work around a distortion bug in DQK
;  - StartSrc wasn't wrapping source directory around 64k
;
;  2004.01.26 SNESAPU v2.0
;  + Added 24- and 32-bit sample support to MMX
;  + Hardware anomalies emulates the DSP at 32kHz then upsamples the output
;  + Replaced fake low-pass filter with a 32-tap FIR filter
;  + Writing to the DSP registers from the SPC700 automatically calls EmuDSP.  This should help keep
;    the DSP and SPC700 in better syncronization.
;  + Removed self modifying code
;  + Moved fade out from APU.Asm
;  - Output is inverted only when hardware anomalies are enabled
;  - Obtained a more accurate copy of the gaussian look-up table used in the SNES
;  - Gaussian interpolation correctly clears the LSB in MMX mixing mode
;  - When using 4-point interpolation, source playback begins with the fourth sample
;  - Removed 17-bit clamp from filter 1
;  - Envelopes are not based on a 33kHz clock (duh...)
;  - Envelopes now use a sample counter instead of a fractional clock
;  - FIR filter coefficients were getting applied in reverse order
;  - Filtered samples get shifted right before accumulation, and LSB is cleared after accum.
;  - Fixed PckWav (it was seriously broken)
;  - Added support for BRR_??? options to PckWav
;
;  2003.07.29 SNESAPU v1.01
;  - Voice's inactive flag wasn't getting set after a key off
;
;  2003.06.30
;  - Made some minor changes to RestoreDSP
;  - Fixed voices' pitch not getting changed in SetDSPOpt
;  - Fixed a bug in the floating-point DSP volume register handler when STEREO = 0
;
;  2003.06.18 SNESAPU v0.99c
;  - Fixed interger overflow in MMX FIR filter
;
;  2003.06.14 SNESAPU v0.99b
;  + Added support for old amplification values (<= 256)
;
;  2003.05.19 SNESAPU v0.99a
;  - Fixed a bug in the i386 FIR filter
;
;  2003.04.28 SNESAPU v0.99
;  + InitDSP will select 386 or MMX depending on CPU
;  + Added 16 and 24-bit sample output to EmuDSPF
;  + Changed SetDSPAmp to decibels
;  - Fixed several bugs in the envelope code so envelopes now work the same as the SNES (fixes a
;    lot of songs)
;  - Stopped caching loop points (fixes Castlevania IV, which likes to change sources after a
;    key on)
;  - Added clamping to the pitch modulation code (fixes CT)
;  - Fixed a bug in KON/KOF emulation
;  - Changed 16-bit mixing process to emulate the SNES
;  - Improved low-pass filtering and simulated RF interference
;  - Removed floating-point support from SetDSPAmp
;  - Fixed another volume bug in SetDSPOpt when switching between integer and float routines
;
;  2003.02.10 SNESAPU v0.98
;  + Converted to NASM
;  + Added SetDSPDbg, SaveDSP, RestoreDSP, SetDSPReg
;  + Added a loop to EmuDSP so the number of samples passed in can be any value
;  + Added floating-point support to SetDSPAmp
;  + Added micro ramping to the channel volumes in the floating-point routine
;  + Added 32-bit IEEE 754 output
;  - Added a 17-bit clamp to the UnpckWav (fixes Hiouden; CT and FF6 are iffy now)
;  - Added linear interpolation to FIR filter (fixes some songs that turned to noise)
;  - SetDSPOpt wasn't clearing EAX, which was causing dspDecmp to sometimes contain a bad value
;  - ENVX always gets updated, regardless of ADSR/GAIN flag (fixes Clue)
;
;  2001.10.02 SNESAPU v0.96
;  - freqTab was built from the wrong values (Hz instead of samples)
;  - rateTab now uses 32-bits of precision instead of 16
;  - Values in rateTab >= 1.0 are now set to 0.99 to reduce the ill effects of fast envelopes when
;    the sample rate is < 32kHz
;  - Reset all internal pitch values when PMON is written to (fixes Asterix)
;  - Clear bits in KOF if voice is inactive (improves EOS detection)
;  - Switched to a static lookup table generated by the SNES for Gaussian interpolation
;  - Damn near cracked the sample decompression
;  - Optimized code a bit in EmuDSPN and inadvertently fixed a bug that was causing some songs to
;    break after seeking
;  - EmuDSPN wasn't checking the loop counter after an envelope update was performed (seeking no
;    longer appears to hang)
;
;  2001.05.15 SNESAPU v0.95
;  + Added 4-point Gaussian interpolation
;  + EmuDSPN now decompresses samples, so seeking doesn't lose voices
;  + Added an option to FixSeek to disable resetting voices
;  + Reverse stereo swaps the left/right volumes instead of physically swapping samples
;  + More minor optimizations
;  - Changed the sample decompression routine.  Less songs should have problems.
;  - Echo delay was incorrect for most sample rates
;  - Did something to fix the noise generator not working in some songs
;
;  2001.03.02 SNESAPU v0.92
;  - Fixed the low-pass filter (I'm amazed it was sort of working at all)
;  - Added a hack to UnpckWav to subtract 3 from the range if > 12
;  - Forgot that the DSP needs to be limited to 64K as well (Secret of Evermore doesn't crash)
;  - Fixed a problem in the MMX dynamic code allocation
;  - FixSeek wasn't resetting the DSP X registers (seeking works in Der Langrisser)
;
;  2001.01.26 SNESAPU v0.90
;  + Added voice monitor and VMax2dB for visualization
;  + Added SetDSPPitch so people with SB/SBPro's and compatibles can hear the correct tone
;  + Added SetDSPStereo for headphone listeners
;  + Added SetDSPEFBCT to make the echo sound more natural
;  + Added UnpackWave so waveforms could be decompressed by external programs
;  + Added 32-bit floating point mixer for people with 24/32-bit sound cards
;  + Added a reverse stereo option
;  + Added GetProcInfo to detect 3DNow! and SSE capabilites
;  + Added EmuDSP which automatically calls the mixing routine set in SetDSPOpt
;  + Made a lot of minor optimizations to DSPIn and EmuDSP
;  + Made separate register handlers in DSPIn for mono and floating-point
;  - Changed SetDSPOpt to use individual parameters instead of a record
;  - SetDSPOpt performs range checking
;  - Fixed configuration implementation so options don't need to be reset for every song and they
;    can be changed between calls to EmuDSP
;  - Fixed mono related code (sounds better now)
;  - Echo filter was never getting turned on
;  - Implemented a better low-pass filter
;
;  2000.05.22 SNESAmp v1.2
;  + Added a low-pass filter
;  + Added output monitor
;  + Optimized the waveform code in EmuDSPN
;  + Added FixSeek
;  + SetDSPVol now uses a 16-bit value
;  - FixDSP was marking voices as keyed even if the pitch was 0
;  - Correctly implemented echo volume (should be perfect now)
;
;  2000.04.05 SNESAmp v1.0
;  - Voices with a pitch of 0 can't be keyed on
;  - ENVX becomes 0 when voice is keyed off
;  - ENVX is set to 0 when envelope is in GAIN mode
;  - Account for sample blocks with a range greater than 16-bits (ADPCM still not correct)
;
;  2000.03.17 SNESAmp v0.9
;                                                        Copyright (C)1999-2006 Alpha-II Productions
;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ

%define DSP_INC

;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Equates

;CPU types ----------------------------------
CPU_MMX		EQU	01h
CPU_3DNOW	EQU	02h
CPU_SIMD		EQU	04h

;Mixing routines (see source for additional notes)
MIX_NONE		EQU	0									;No mixing
MIX_INT		EQU	1									;Use integer instructions
MIX_FLOAT	EQU	3									;Use floating-point instructions

;Interpolation routines ---------------------
INT_NONE		EQU	0									;None
INT_LINEAR	EQU	1									;Linear
INT_CUBIC	EQU	2									;Cubic
INT_GAUSS	EQU	3									;4-point Gaussian
INT_SINC		EQU	4									;8-point Sinc

;DSP options --------------------------------
OPT_ANALOG	EQU	01h								;Simulate anomalies of the analog hardware
OPT_OLDSMP	EQU	02h								;Old sample decompression routine
OPT_SURND	EQU	04h								;"Surround" sound
OPT_REVERSE	EQU	08h								;Reverse stereo samples
OPT_NOECHO	EQU	10h								;Disable echo (must not be bit 0 or 5, see disEcho)
OPT_FILTER	EQU	20h								;Pass each voice through an anti-aliasing filter

;Automatic Amplification Reduction ----------
AAR_TOFF		EQU	0									;Disable AAR
AAR_TON		EQU	1									;Enable AAR
AAR_TINC		EQU	2									;Enable AAR with auto increase
AAR_TDYN		EQU	3									;Enable dynamic AAR (not currently supported)
AAR_TYPE		EQU	3									;Mask for type of AAR
AAR_INC		EQU	4									;Auto increase is enabled
AAR_ON		EQU	8									;AAR is enabled, amp level can be adjusted

;Voice muting -------------------------------
MUTE_GET		EQU	-1									;Do nothing (use to get mute state)
MUTE_OFF		EQU	0									;Unmute
MUTE_ON		EQU	1									;Mute
MUTE_SOLO	EQU	2									;Unmute voice and mute all others
MUTE_TOGGLE EQU	3									;Toggles current mute state

;PackSrc options ----------------------------
BRR_END		EQU	01h								;Set the end flag in the last block
BRR_LOOP		EQU	02h								;Set the loop flag in all blocks
BRR_LINEAR	EQU	04h								;Use linear compression for all blocks
BRR_NOINIT	EQU	10h								;Don't create an initial block of silence
BRR_CONT		EQU	80h								;Continue compression

;UnpackSrc options --------------------------
BRR_OLDSMP	EQU	80h								;Use old sample decompression routine

;Mixing flags -------------------------------
MFLG_MUTE	EQU	01h								;Voice is muted
MFLG_KOFF	EQU	04h								;Voice is in the process of keying off
MFLG_OFF		EQU	08h								;Voice is currently inactive

;Envelope mode masks ------------------------
E_ADJ			EQU	00001b							;Adjustment: Constant(1/64 or 1/256) / Exp.(x255/256)
E_DIR			EQU	00010b							;Direction: Decrease / Increase
E_DEST		EQU	00100b							;Destination: Default(0 or 1) / Other(x/8 or .75)
E_ADSR		EQU	01000b							;Envelope mode: Gain/ADSR
E_IDLE		EQU	80h								;Envelope speed is set to 0

;Envelope modes -----------------------------
E_DEC			EQU	00000b							;Linear decrease
E_EXP			EQU	00001b							;Exponential decrease
E_INC			EQU	00010b							;Linear increase
E_BENT		EQU	00110b							;Bent line increase
E_DIRECT		EQU	00111b							;Direct gain
E_REL			EQU	01000b							;Release mode
E_SUST		EQU	01001b							;Sustain mode
E_ATT			EQU	01010b							;Attack mode
E_DECAY		EQU	01101b							;Decay mode

;DSP debugging options ----------------------
DSP_HALT		EQU	80h								;Halt DSP emulation


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Structures

;DSP registers ------------------------------
STRUC DSPVoice
	volL	resb	1										;Volume Left
	volR	resb	1										;Volume Right
	pitch	resw	1										;Pitch (Rate/32000) [2.12]
	srcn	resb	1										;Sound source being played back
	adsr1	resb	1										;ADSR volume envelope (attack and decay rate)
	adsr2	resb	1										;   "  " (sustain level and rate)
	gain	resb	1										;Envelope gain rate (if not using ADSR)
	envx	resb	1										;Current envelope heigth [.7]
	outx	resb	1										;Current sample being output [-.7]
			resb	2
ENDSTRUC

STRUC DSPReg
			resb	12										;Settings for voice 0
	mvolL	resb	1										;Main Volume Left [-.7]
	efb	resb	1										;Echo Feedback [-.7]
			resb	1
	fc		resb	1										;Filter Coefficient 0
			resb	12
	mvolR	resb	1										;Main Volume Right [-.7]
			resb	15
	evolL	resb	1										;Echo Volume Left [-.7]
	pmon	resb	1										;Pitch Modulation on/off for each voice
			resb	14
	evolR	resb	1										;Echo Volume Right [-.7]
	non	resb	1										;Noise output on/off for each voice
			resb	14
	kon	resb	1										;Key On for each voice
	eon	resb	1										;Echo on/off for each voice
			resb	14
	kof	resb	1										;Key Off for each voice
	dir	resb	1										;Page containing source directory
			resb	14
	flg	resb	1										;DSP flags and noise frequency
	esa	resb	1										;Starting page used to store echo samples
			resb	14
	endx	resb	1										;End block of sound source has been decoded
	edl	resb	1										;Echo Delay in ms >> 4
			resb	2
ENDSTRUC

;Internal mixing settings -------------------
STRUC VoiceMix
	;Visualization --- 08
	vMax
	vMaxL	resd	1										;Maximum absolute sample output
	vMaxR	resd	1
	;Voice ----------- 04
	pDSPV	resd	1										;-> voice registers in DSP
	;Waveform -------- 06
	bCur	resd	1										;-> current block
	bHdr	resb	1										;Block Header for current block
	mFlg	resb	1										;Mixing flags
	;Envelope -------- 22
	eMode	resb	1										;[3-0] Current envelope mode
															;[6-4] ADSR mode to switch into from Gain
															;[7]   Envelope is not changing
	eRIdx	resb	1										;Index in rateTab
	eRate	resd	1										;Rate of envelope adjustment [16.16]
	eCnt	resd	1										;Sample counter [16.16]
	eVal	resd	1										;Current envelope height [.11]
	eAdj	resd	1										;Amount to adjust envelope height
	eDest	resd	1										;Envelope Destination [.11]
	;Samples --------- 56
	sP1	resw	1										;Last sample decompressed (prev1)
	sP2	resw	1										;Second to last sample (prev2)
	sIdx	resd	1										;-> current sample in sBuf
	sBufP	resw	8										;Last 8 samples from previous block
	sBuf	resw	16										;32 bytes for decompressed BRR block
	;Mixing ---------- 32
	mTgtL	resd	1										;Target volume (floating-point mixing only)
	mTgtR	resd	1										; "  "
	mChnL	resd	1										;Channel volume [-24.7]
	mChnR	resd	1										; "  "
	mRate	resd	1										;Pitch Rate after modulation [16.16]
	mDec	resd	1										;Pitch Decimal [.16] (used as delta for interpolation)
	mOut	resd	1										;Last sample output before chn vol (used for pitch mod)
			resd	1										;reserved
ENDSTRUC

;Saved state (see SaveDSP) ------------------
STRUC DSPState
	.pReg		resd	1									;[0.0] -> DSP registers (128 bytes)
	.pVoice	resd	1									;[0.4] -> Internal mixing settings (1k)
	.pEcho	resd	1									;[0.8] -> echo buffer (bytes = sample rate * 1.92)
	.amp		resd	1									;[0.C] Amplification (AAR is stored in upper 4 bits)
ENDSTRUC


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Public Variables

PUBLIC	dsp											;DSP registers
PUBLIC	mix											;Mixing structures for each voice


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Exported Functions

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Get Processor Information
;
;Detects if processor has MMX, 3DNow!, or SSE capabilities
;
;In:
;   nothing
;
;Out:
;   EAX = see CPU_??? equates
;
;Destroys:
;   nothing

PUBLIC GetProcInfo, NULL


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Initialize DSP
;
;Creates the lookup tables for interpolation, and sets the default mixing settings:
;
;   mixType = MIX_INT
;   numChn  = 2
;   bits    = 16
;   rate    = 32000
;   inter   = INT_GAUSS
;   opts    = 0
;
;Destroys:
;   EAX,ST0-7

PUBLIC InitDSP, NULL


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Reset DSP
;
;Resets the DSP registers, erases internal variables, resets the volume, and resets the halt flag
;
;Destroys:
;   EAX

PUBLIC ResetDSP, NULL


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Set DSP Options
;
;Recalculates tables, changes the output sample rate, and sets up the mixing routine
;
;Notes:
;   Range checking is performed on all parameters.  If a parameter does not match the required
;    range of values, the default value will be assumed.
;
;   -1 can be used for any paramater that should remain unchanged.
;
;   The OPT_ANALOG option only works when:
;    mix  =  MIX_INT
;    chn  =  2
;    bits >= 16
;    rate >= 32000
;
;   Four channel output forces MIX_FLOAT
;
; In:
;   mixType = Mixing routine (default MIX_INT)
;   numChn  = Number of channels (1, 2, or 4, default 2)
;   bits    = Sample size (mono 8 or 16; stereo 8, 16, 24, 32, or -32 [IEEE 754], default 16)
;   rate    = Sample rate (8000-192000, default 32000)
;   inter   = Interpolation type (default INT_GAUSS)
;   opts    = see OPT_??? equates
;
;Destroys:
;   EAX

PUBLIC SetDSPOpt, mixType:dword, numChn:dword, bits:dword, rate:dword, inter:dword, opts:dword


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Debug DSP
;
;Installs a callback that gets called each time a value is written to the DSP data register.
;The function is called with the C calling convention.
;
;Upon entrance to the function:
;   [ESP+4] = Current opcode (low word = PC)
;   [ESP+8] = YA
;
;Note:
;   DEBUG must be set to 1 in APU.Inc
;
;In:
;   pTrace-> Debug function (NULL turns off the debug call, -1 leaves the current callback)
;   opts   = DSP debugging options (-1 leaves the current options)
;
;Out:
;   Previously installed callback
;
;Destroys:
;   nothing

PUBLIC SetDSPDbg, pTrace:ptr, opts:dword


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Fix DSP After Loading SPC file
;
;Initializes the internal mixer variables
;
;Destroys:
;   EAX

PUBLIC FixDSPLoad, NULL


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Fix DSP After Seeking
;
;Puts all DSP voices in a key off state and erases echo region.  Call after using EmuDSPN.
;
;In:
;   reset = true, reset all voices
;           false, only erase memory
;
;Destroys:
;   EAX

PUBLIC FixDSPSeek, reset:byte


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Save DSP State
;
;Notes:
;   Pointers in the DSPState structure can be NULL if you don't want a copy of that data
;
;   The values in the Voice structures are modified as follows:
;     dspv points to the registers pointed to by DSPState.pDSP
;     bCur becomes an index into APU RAM (0-65535)
;     eRate is adjusted for 32kHz
;     eCnt is adjusted for 32kHz
;
;   Only the current samples being used for the echo delay are copied into pEchoBuf, not necessarily
;    all 240ms.
;
;In:
;   pState -> Saved state structure
;
;Destroys:
;   EAX

PUBLIC SaveDSP, pState:ptr


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Restore DSP State
;
;Notes:
;   Setting a pointer in DSPState to NULL will keep its corresponding internal data intact, except
;    for the echo.  If pEchoBuf is NULL, the internal echo buffer will be cleared.
;
;   The members of the Voice structures must contain the same values as they would during emulation,
;    except:
;     bCur must be an index into APU RAM (0-65535), not a physical pointer
;     eCnt must be the current sample count based on a 32kHz sample rate
;     dspv, bHdr, and eRate do not need to be set because they will be reset to their correct values
;    If the inactive flag in mFlg is set, then the remaining Voice members do not need to be set
;
;   By setting eRate to -1, RestoreDSP will reset the following members:
;     eRIdx, eRate, eCnt, eAdj, eDest
;     eMode and eVal will remain unchanged
;
;   By setting 'mRate' to -1, RestoreDSP will reset the following members:
;     mTgtL/R, mChnL/R, mRate, mDec, and mOrgP
;     mFlg and mOut will remain unchanged
;
;   Only the amount of echo delay specified by DSP.EDL is copied from pEchoBuf
;
;In:
;   pState -> Saved state structure
;
;Destroys:
;   EAX

PUBLIC RestoreDSP, pState:ptr


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Set Amplification Level
;
;Sets the amount to amplify the output during the final stage of mixing
;
;Note:
;   Calling this function disables AAR for the current song
;
;In:
;   amp = Amplification level [-15.16] (1.0 = SNES, negative values act as 0)
;
;Destroys:
;   EAX

PUBLIC SetDSPAmp, amp:dword


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Get Amplification Level
;
;Returns the current amplification level, which may have been changed due to AAR.
;
;Out:
;   EAX = Current amp level

PUBLIC GetDSPAmp


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Set Automatic Amplification Reduction
;
;Notes:
;   -1 can be passed for any parameter that should remain unchanged
;   Calling this function restarts AAR if it has been stopped due to amp reaching min or max or by
;    calling SetDSPAmp
;   This function has no effect if the emulator was not compiled with the VMETERM option set
;
;In:
;   type   = Type of AAR (see AAR_??? equates)
;   thresh = Threshold at which to decrease amplification
;   min    = Minimum amplification level to decrease to
;   max    = Maximum amplification level to increase to
;
;Destroys:
;   EAX

PUBLIC SetDSPAAR, type:byte, thresh:dword, min:dword, max:dword


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Set Song Length
;
;Sets the length of the song and fade
;
;Note:
;   To set a song with no length, pass -1 and 0 for the song and fade, respectively
;
;In:
;   song = Length of song (in 1/64000ths second)
;   fade = Length of fade (in 1/64000ths second)
;
;Out:
;   EAX = Total length

PUBLIC SetDSPLength, song:dword, fade:dword


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Set DSP Base Pitch
;
;Adjusts the pitch of the DSP
;
;In:
;   base = Base sample rate (32000 - Normal pitch, 32458 - Old SB cards, 32768 - Old ZSNES)
;
;Destroys:
;   EAX

PUBLIC SetDSPPitch, base:dword


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Set Voice Stereo Separation
;
;Sets the amount to adjust the panning position of each voice
;
;In:
;   sep = Separation [1.16]
;         1.0 - full separation (output is either left, center, or right)
;         0.5 - normal separation (output is unchanged)
;           0 - no separation (output is completely monaural)
;
;Destroys:
;   EAX,ST0-7

PUBLIC SetDSPStereo, sep:dword


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Set Echo Feedback Crosstalk
;
;Sets the amount of crosstalk between the left and right channel during echo feedback
;
;In:
;   leak = Crosstalk amount [-1.15]
;           1.0 - no crosstalk (SNES)
;             0 - full crosstalk (mono/center)
;          -1.0 - inverse crosstalk (L/R swapped)
;
;Destroys:
;   EAX

PUBLIC SetDSPEFBCT, leak:dword


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Set Voice Mute
;
;Prevents a voice's output from being mixed in with the main output
;
;In:
;   voice = Voice to mute (0-7, only bits 2-0 are used)
;   state = see MUTE_??? equates
;
;Out:
;   true, if voice is muted

PUBLIC SetDSPVoiceMute, voice:byte, state:byte


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;DSP Data Port
;
;Writes a value to a specified DSP register and alters the DSP accordingly.  If the register write
;affects the output generated by the DSP, this function returns true.
;
;Note:
;   SetDSPReg does not call the debugging callback
;
;In:
;   reg = DSP Address
;   val = DSP Data
;
;Out:
;   EAX = True, if the DSP state was affected

PUBLIC SetDSPReg, reg:byte, val:byte


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Emulate DSP
;
;Emulates the DSP of the SNES
;
;Notes:
;   If 'pBuf' is NULL, the routine MIX_NONE will be used
;   Range checking is performed on 'len'
;
;In:
;   pBuf-> Buffer to store output
;   len  = Length of buffer (in samples)
;
;Out:
;   EAX -> End of buffer
;
;Destroys:
;   ST0-ST7

PUBLIC EmuDSP, pBuf:ptr, len:dword


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Play Sound Source
;
;Plays a single sound source using the settings defined by SetDSPOpt.  All samples are output in
;integer form.  The output is attenuated -6dB.
;
;Admittedly, this function is poorly coded in that it does two different things depending on the
;value of the last parameter.
;
;Note:
;   If the source data is within APU RAM, the source pointer will wrap around within APU RAM
;
;In:
;   pSrc-> Sound source to play (if upper 16 bits are 0, lower 16 bits will index APU RAM)
;   loop = Block to loop on
;   rate = Logical sample rate to play back at (actual playback rate may be less)
;
;   pSrc-> Buffer to store decompressed and resampled data
;   loop = Number of samples to generate
;   rate = 0
;
;Out:
;   rate != 0, EAX = Sample rate sound will be played back at
;   rate == 0, EAX-> End of buffer

PUBLIC PlaySrc, pSrc:ptr, loop:dword, rate:dword


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Unpack Sound Source
;
;Decompresses a series of BRR blocks into 16-bit PCM samples
;
;Notes:
;   Regardless of the value in 'num', unpacking will stop when the end block is reached.
;   The previous samples are only needed if you're making multiple calls to UnpackWave.  If you're
;    decompressing the entire sound in one call, you can disregard them.
;   If the source data is within APU RAM, the source pointer will wrap around within APU RAM,
;    though no more than 7281 BRR blocks will be processed.
;
;In:
;   pSmp -> Location to store decompressed samples
;   pBlk -> Sound source to decompress (if upper 16 bits are 0, lower 16 bits will index APU RAM)
;   num   = Number of blocks to decompress (0 to decompress entire sound)
;   opts  = Options (only one for now)
;           BRR_OLDSMP = Traditional method of decompression
;   prev1-> First previous sample (can be NULL if not needed)
;   prev2-> Second previous sample
;
;Out:
;   EAX = Number of blocks decompressed

PUBLIC UnpackSrc, pSmp:ptr, pBlk:dword, num:dword, opts:dword, prev1:ptr, prev2:ptr


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Pack Sound Source
;
;Compresses a series of 16-bit PCM samples into BRR blocks.  Generally sounds are packed in one
;call.  However, if you want to implement some sort of real-time compression stream you can make
;multiple calls to PackSrc.  See the options description below for more information on continuing
;compression.
;
;THIS FUNCTION HAS NOT BEEN FINISHED AND DOES NOT PACK LOOPED SOUNDS
;
;Notes:
;   The previous samples are only needed if you're making multiple calls to PackSrc.  If you're
;    compressing the entire sound in one call, you can disregard them.
;   No checking is performed on the output pointer to make sure the data wraps around if it resides
;    within APU RAM.
;   The looping section must be at least 16 samples.
;
;In:
;   pBlk -> Location to store bit-rate reduced blocks
;   pSmp -> Samples to compress
;   pLen -> Number of samples to compress
;   pLoop-> Sample index of loop point.  Set this to NULL for one-shot sounds.  This parameter is
;           ignored if 'prev1' is not NULL.
;   opts  = Options.  The way the options behave is different depending on if 'prev1' is NULL.
;
;           If prev1 is NULL:
;           BRR_END    = This option has no effect.  The last block's end flag will always be set.
;           BRR_LOOP   = By default, if pLoop is not NULL the last block's loop flag will
;                        automatically be set.  This option causes the loop flag to be set in every
;                        block regardless of pLoop.
;           BRR_LINEAR = By default, the best compression filter will be determined for each block.
;                        This option forces linear compression (filter 0) to be used for all blocks.
;           BRR_NOINIT = By default, a block of silence is inserted at the beginning of the sound
;                        source.  This option skips the silence and encodes samples into the first
;                        block.  (The first block, whether it contains silence or samples, will
;                        always use linear compression.  This is necessary in order for the
;                        decompression filter to be properly initialized when the source is played.)
;
;           If prev1 is not NULL:
;           BRR_END    = Sets the end flag in the last block
;           BRR_LOOP   = This option causes the loop flag to be set in every block.
;           BRR_LINEAR = By default, the best compression filter will be determined for each block.
;                        This option forces linear compression (filter 0) to be used for all blocks.
;           BRR_NOINIT = By default, the first block is compressed with linear compression.  This
;                        option causes the first block to be compressed with whichever filter is the
;                        best.
;   prev1-> First previous sample.  NULL if the value of the previous samples is not needed.
;           (Generally the previous samples are only used if making multiple calls to PackSrc.)
;   prev2-> Second previous sample.  If this is NULL, 'prev1' is treated as NULL also.
;
;Out:
;   EAX  -> End of the data stored in pBlk, NULL if an error occurred.
;           Possible errors:
;              Length is 0
;              Loop point is greater than the number of samples
;              Loop length is less than 16
;   pLen -> Number of blocks in sound source
;   pLoop-> Starting block of loop

PUBLIC PackSrc, pBlk:ptr, pSmp:ptr, pLen:ptr, pLoop:ptr, opts:dword, prev1:ptr, prev2:ptr


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;VMax to Decible
;
;Generates decible values for the peak output of each voice and the main output, the purpose of
;which is to provide the user with some sort of PPM (Peak Program Meter) visualization.
;
;The decible values are stored in an array, where the first 8 pairs of values are used for the
;left/right output of each voice and the last pair for the main output.
;
;The values generated can be 32-bit floats or 16.16 fixed-point integers.  When floating-point
;numbers are chosen, the return values are in actual dB starting with -96 (which represents
;-infinity) on up to +0.  When integers are chosen, the return values are in fixed point notation
;[16.16] and are positive starting with 0 (which represents -infinity) on up to +96 (or 600000h,
;representing +0db).
;
;After calling VMax2dB, the vMax values for both voice and main outputs are set to 0.
;
;Notes:
;   The values for the main output will be higher than 0dB if clipping has occurred.
;
;In:
;   pList-> Array to store 18 32-bit numbers
;
;Destroys:
;   EAX

PUBLIC VMax2dBf, pList:ptr
PUBLIC VMax2dBi, pList:ptr