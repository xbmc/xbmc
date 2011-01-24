;อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ
;Program:    Super Nintendo Entertainment System(tm) Audio Processing Unit Emulator
;Platform:   Intel 80386 & MMX
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
;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
; Revision History:
;
; 2005.10.28 SNESAPU 3.0
; + Added NULL pointer support to SaveAPU and RestoreAPU
; + Moved variables into the .bss
; + The build options are now returned by InitAPU instead of being stored in a global variable
; - Calculate the emulated sample rate at the top of EmuAPU instead ofin SetAPUSmpClk
;
; 2003.11.04 SNESAPU 2.0
; + Updated EmuAPU to use SetEmuDSP when DSPINTEG is enabled
; + Move fade out code into DSP.Asm
; + Removed amp setting from ResetAPU
;
; 2003.07.12 SNESAPU 1.0a
; - Fixed the call to SetDSPVol in SetFade that wasn't restoring the stack
;
; 2003.06.20
; + Added LoadSPCFile
; + Added ability to pass -1 to ResetAPU to keep current amp level
;
; 2003.02.10
; + Rewrote in assembly
; + Removed GetAPUData and memory allocation (they were specific to SNESAPU.DLL)
; - Sometimes EmuAPU would generate an extra sample
;
; 2002.06.20
; - Call SetFade after seeking
;
; 2001.04.12
; + Updated SeekAPU to incorporate new features in SPC700 and DSP emus
;
; 2000.12.30 SNESAmp v2.0
; + Added SetAPULength, SeekAPU, SaveAPU, RestoreAPU, HaltAPU
; + General optimizations
; - Simplified interface a bit
;
; 2000.04.04 SNESAmp v1.0
;
; 2000.03.17 SNESAmp v0.9
;                                                          Copyright (C)2003-06 Alpha-II Productions
;อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ

%define APU_INC

;อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ
;Compile Options

CPU_CYC	EQU	24										;CPU clock divisor
															;10 = 2.457MHz
															;12 = 2.048MHz
															;24 = 1.024MHz

;Debugging ability:
;  This option enables code that allows hooks to be installed into the SPC700 opcode fetcher and
;  DSP register handler.  See SetSPCDbg and SetDSPDbg for more information.

DEBUG		EQU	1										;Enable debugging ability

;Integrate DSP emulation with SPC700:
;  The DSP will automatically be emulated, if necessary, each time a DSP register is written to.
;  With DSP integration enabled, the CNTBK and DSPBK options change from breaking SPC700 emulation
;  to updating the DSP.

DSPINTEG	EQU	1										;Enable integration of the DSP with the SPC700

PROFILE	EQU	0										;Enable instruction profiling

;SPC700 specific ----------------------------
HALFC		EQU	0										;Enable half-carry flag emulation for decimal adjusts
IPLW		EQU	1										;Enables the IPL Writable flag in register F1

CNTBK		EQU	0										;Break SPC700/Update DSP if a counter increases
DSPBK		EQU	1										;Break SPC700/Update DSP if register F3 is read from
															; (only if the reg is x8, x9, or 7C)

SPEED		EQU	0										;Enable a hack to skip cycles when a counter is read

;DSP specific -------------------------------
MMETER	EQU	1										;Main output PPM (for AAR and visualization)
VMETER	EQU	1										;Voice output PPM (for visualization)
STEREO	EQU	1										;Enable user stereo controls.
															; (see SetDSPStereo and SetDSPEFBCT)


;อออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออ
;Exported Functions

;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Initialize Audio Processing Unit
;
;Initializes internal tables, mixer settings, memory, and registers.
;This only needs to be called once.
;
;see InitSPC and InitDSP
;
;Out:
;   EAX = APU build options (see APU.Inc and SNESAPU.h GetAPUData() for more info)

PUBLIC InitAPU, NULL


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Reset Audio Processor
;
;Clears all memory and sets registers to their default values
;
;see ResetSPC and ResetDSP

PUBLIC ResetAPU, NULL


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Load SPC File
;
;Restores the APU state from an .SPC file.  You do not need to call ResetAPU before loading a file.
;
;In:
;   pFile -> 66048 byte SPC file

PUBLIC LoadSPCFile, pFile:dword


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Save Audio Processor State
;
;Creates a saved state of the audio processor
;
;see SaveSPC and SaveDSP
;
;In:
;   pSPC -> SPCState structure
;   pDSP -> DSPState structure

PUBLIC SaveAPU, pSPC:ptr, pDSP:ptr


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Restore Audio Processor State
;
;Restores the audio processor from a saved state
;
;see RestoreSPC and RestoreDSP
;
;In:
;   pSPC -> SPCState structure
;   pDSP -> DSPState structure

PUBLIC RestoreAPU, pSPC:ptr, pDSP:ptr


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Set Audio Processor Options
;
;Configures the sound processor emulator.  Range checking is performed on all parameters.
;
;Notes:  -1 can be passed for any parameter you want to remain unchanged
;        see SetDSPOpt in DSP.Inc for a more detailed description
;
;In:
;   mix   = Mixing routine (default MIX_INT)
;   chn   = Number of channels (1 or 2, default 2)
;   bits  = Sample size (8, 16, 24, 32, or -32 [IEEE 754], default 16)
;   rate  = Sample rate (8000-192000, default 32000)
;   inter = Interpolation type (default INT_GAUSS)
;   opts  = See 'DSP options' in the Equates section of DSP.Inc

PUBLIC SetAPUOpt, mix:dword, chn:dword, bits:dword, rate:dword, inter:dword, opts:dword


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Set Audio Processor Sample Clock
;
;Calculates the ratio of emulated clock cycles to sample output.  Used to speed up or slow down a
;song without affecting the pitch.  Range checking is performed.
;
;In:
;   speed = Multiplier [16.16] (1/16x to 16x)

PUBLIC SetAPUSmpClk, speed:dword


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Emulate Audio Processing Unit
;
;Emulates the APU for a specified amount of time.  DSP output is placed in a buffer to be handled
;by the main program.
;
;Calling EmuAPU instead of EmuSPC and EmuDSP will ensure the two processors are kept in sync with
;each other.
;
;If the cycle count is 0, the SPC700 will be emulated until the DSP has filled the output buffer
;with the specified number of samples.
;
;If the sample count is 0, the DSP will be produce output for the same amount of time as the SPC700
;is emulated.
;
;If the cycle count and sample count aren't equal in terms of time, then one processor will be
;halted at the end of its count while the other processor is continued.
;
;In:
;   pBuf-> Buffer to store output samples (can be NULL)
;   cyc  = Number of clock cycles to emulate SPC700 for (APU_CLK = 1 second)
;   smp  = Number of samples for the DSP to generate
;
;Out:
;   EAX -> End of buffer

PUBLIC EmuAPU, pBuf:ptr, cyc:dword, smp:dword


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Seek to Position
;
;Seeks forward in the song
;
;In:
;   time = 1/64000ths second to seek forward
;   fast = Use faster seeking method (may break some songs)

PUBLIC SeekAPU, time:dword, fast:bool


;ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ
;Shutdown Audio Processing Unit
;
;Currently does nothing

PUBLIC ShutAPU, NULL