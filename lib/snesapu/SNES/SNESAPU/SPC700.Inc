;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Program:    Sony SPC700 Emulator
;Platform:   80386
;Programmer: Anti Resonance
;
;Thanks to Michael Abrash.  It was reading the Graphics Programming Black Book that gave me the
;idea and inspiration to write this in the first place.
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
; 2005.10.28 SNESAPU v3.0
; + Updated EmuSPC to use a jump table to execute opcodes :(
; + Removed InPortW
; + Added GetAPUPort
; + The memory address macros were getting the RAM pointer from pAPURAM instead of EDI
; + Macro _mbit now returns a bit instead of a memory location
; - IPL ROM wasn't getting copied to RAM on reset (don't know how I missed this one)
; - Fixed InitSPC and ResetSPC so function registers are properly preserved
; - Timer values are stored internally so reading FA-FC returns 0
; - Rewrote speed hack (now it's faster and doesn't update disabled timers)
; + All global variables are now stored relative to EDI
; + Common cleanup configurations are now jumped to instead of being expanded within the opcode
; + CL contains the result of the previous operation and is used to determine the values of N and Z.
;   Faster than setting the flags in memory.
; + Post read check for function registers now operates like the post write
; + Added profiling counters
;
; 2004.08.01 SNESAPU v2.1
; + Moved all checks for SPC_NODSP into DSP.Asm
;
; 2004.01.26 SNESAPU v2.0
; - Added code to call DSP emulator if 00F3 is read from
; - Updated opcode fetch code to work with new DSP syncronization
;
; 2003.06.30
; - Fixed a bug where the counter would increase every pulse if the timer was disabled and set to 0
;
; 2003.02.10 SNESAPU v0.98
; + Added SaveSPC, RestoreSPC, and SetAPURAM
; + Coded support for BRK (still not sure if it's correct)
; - Rewrote SetSPCDbg and the opcode fetcher to fix some potential bugs
; - Added code to CntHack to try and isolate small polling loops (fixes the tempo problems some
;   games were having)
; - Changed FixSPC, again, to copy extraRAM from APURAM[0FFC0h] if IPL reading is disabled in F1h
;   (fixes Tales of Phantasia)
; - Fixed a lame bug when clearing the in-ports with F1h (fixes Dragon Quest)
;
; 2001.04.12 SNESAPU v0.95
; + Added SPC_NODSP to the debugging options
; - Fixed some bugs with copying the IPL ROM and detecting reading ability
;
; 2001.01.29 SNESAmp v2.1
; - Changed internal clock to 1.024MHz (I haven't noticed any differences, yet)
;
; 2001.01.01 SNESAmp v2.0
; + Added an external counter to the 64kHz timer
; + Emulator is now based off of a 24.576MHz clock, so implementation of a 2.048 or 2.457MHz CPU
;   clock will be easier internally and transparent externally
; + Added speed hack to SLEEP
; + Added option flags to SPCDebug to allow greater control over the SPC700 state
; + Added breaking ability to STOP in non-debug builds
; - In ports don't get cleared on load, regardless of setting in F1
; - Speed hack is now implemented properly
; - PC is now operated on as a 16-bit quantity (Rudra no Hihou expects PC to roll over, which it
;   wasn't doing when instructions were adding to ESI instead of SI)
; - Changed coding style a bit to make code easier to read and added more comments
;
; 2000.05.04 SNESAmp v1.2
; - Control register is now set to 80h on reset
;
; 2000.04.04 SNESAmp v1.0
; + Rearranged code so static branch prediction would be more accurate
; + Clock cycles to emulate is passed on the stack to EmuSPC
; + SPC700 registers are passed on the stack to the external debug routine
; + Eliminated ORG directive and rewrote code to conform to MASM syntax
; - Fixed some implementation bugs with the speed hack
; - Fixed a bug in the 16-bit WrPost macro, again
;
; 2000.03.17 SNESAmp v0.9
; + Rewrote for compatibility in a 32-bit C environment
; + Added external functions to write to ports
; - SLEEP wasn't erasing the port status on the first time through
;
; 2000.02.22 SPC Tool v0.6
; - Fixed a bug in the 16-bit WrPost macro
;
; 2000.02.01 SPC Tool v0.51
; + Added a hack to eliminate polling the counters
; + Aligned many of the short jump destinations on paragraph boundaries
; + Rewrote the opcode header macro to compile into table form instead of packed form.
;   (The compiled code is four times bigger, but it's now easier to align, port, and debug.)
; + Store flags as dwords so accesses to the DP don't cause a partial register stall
; - Fixed PCALL, STOP, and SLEEP
;
; 2000.01.21 SPC Tool v0.5
; + Reprogrammed PSW handling for faster entry/exit
;
; 1999.12.27 SPC Tool v0.2
;                                                        Copyright (C)1999-2006 Alpha-II Productions
;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ

%define SPC700_INC

;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Equates

APU_CLK		EQU	24576000							;Number of clock cycles in one second

;SPC700 debugging options -------------------
SPC_HALT		EQU	80h								;Halt emulation
SPC_TRACE	EQU	40h								;Trace instruction execution (used internally)
SPC_STOP		EQU	01h								;Only break on STOP instruction


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Structures

STRUC SPCState
	.pRAM		resd	1									;[0.0] -> APU's RAM (64k)
	.pc		resw	1									;[0.4] Registers
	.a			resb	1
	.y			resb	1
	.x			resb	1
	.psw		resb	1
	.sp		resw	1
	.outP		resb	4									;[0.C] Out ports

	.upCnt	resb	3									;[1.0] Up counters for counter increase
	.t8kHz	resb	1									;[1.3] # of cycles left until timer increase
	.t64kHz	resd	1									;[1.4] # of cycles left until timer increase
	.t64Cnt	resd	1									;[1.8] # of 64kHz ticks since emulation began
	._r1		resd	1									;[1.C] reserved
ENDSTRUC

STRUC Profile
	.exec		resd	256								;Instruction executed
	.bxx		resd	8									;Branch taken on BPL,BMI,BVC,BVS,BCC,BCS,BNE,BEQ
	.bbc		resd	8									;Branch taken on BBC.bit
	.bbs		resd	8									;Branch taken on BBS.bit
	.cbne		resd	2									;Branch taken on CBNE dp,dp+X
	.dbnz		resd	2									;Branch taken on DBNZ Y,dp
	.funcr	resd	16									;Function register read
	.funcw	resd	16									;Function register write
	.func16	resd	16									;Function register write via MOVW, INCW, or DECW
	.dspr		resd	128								;DSP register read
	.dspw		resd	256								;DSP register write
	.dspu		resd	128								;DSP register write that affected the DSP state
	.update	resd	4
	.hostTSC	resq	2
	.apuTSC	resq	2									;Time spent in EmuAPU
	.spcTSC	resq	2									;Time spent in EmuSPC
	.dspTSC	resq	2									;Time spent in EmuDSP
	.sizeof
ENDSTRUC


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Public Variables

PUBLIC	pAPURAM										;Pointer to 64KB of APU RAM
PUBLIC	profile										;Profiling counters


;ÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ
;Exported Functions

;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Initialize SPC700
;
;This function is a remnant from the 16-bit assembly when dynamic code reallocation was used.
;Now it just initializes internal pointers.
;
;Out:
;   EAX -> 64KB APU RAM

PUBLIC InitSPC, NULL


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Reset SPC700
;
;Sets all user memory to FF, resets the SPC700 execution time, resets the halt flag, and copies ROM
;into the IPL region
;
;Destroys:
;   EAX

PUBLIC ResetSPC, NULL


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Debug SPC700
;
;Installs a callback that gets called before an instruction is executed.  The function is called
;with the C calling convention.  Non-pointer values are zero extended.
;
;Upon entrance to the function:
;   [ESP+4] -> Current opcode (low word = PC)
;   [ESP+8]  = YA
;   [ESP+12] = X
;   [ESP+16] = PSW
;   [ESP+20]-> Current stack (low word = SP)
;   [ESP+24] = Clock cycle down count
;              [0-1] 8kHz cycles left until counters 0 and 1 increase
;              [2]   64kHz cycles left until counter 2 increases
;              [3]   CPU cycles left until 64kHz clock pulse
;
;Note:
;   pTrace is always called when a STOP instruction is encountered, regardless of if the DEBUG
;    option is set.  DEBUG must be set to 1 in APU.Inc for pTrace to be called under other
;    circumstances.
;
;In:
;   pTrace-> Debug function (NULL turns off the debug call, -1 leaves the current callback)
;   opts   = SPC700 debugging options (-1 leaves the current options)
;
;Out:
;   Previously installed callback

PUBLIC SetSPCDbg, pTrace:ptr, opts:dword


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Fix SPC700 After Loading SPC File
;
;Loads timer steps with the values in the timer registers, resets the counters, sets up the in/out
;ports, and stores the registers.
;
;In:
;   SPC internal registers
;
;Destroys:
;   EAX

PUBLIC FixSPCLoad, pc:word, a:byte, y:byte, x:byte, psw:byte, sp:byte


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Save SPC700 State
;
;Note:
;   Pointers in the SPCState structure can be NULL
;
;In:
;   pState -> Saved state structure
;
;Destroys:
;   EAX

PUBLIC SaveSPC, pState:ptr


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Restore SPC700 State
;
;Note:
;   Setting a pointer in SPCState to NULL will keep its corresponding internal data intact.
;
;In:
;   pState -> Saved state structure
;
;Destroys:
;   EAX

PUBLIC RestoreSPC, pState:ptr


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Write to APU RAM via the SPC700
;
;Writes a value to APU RAM via the SPC700.  Use this instead of writing to RAM directly so any
;changes made by writing to function registers or the IPL region will be handled properly.
;
;In:
;   addr = Address to write to (only the lower 16-bits are used)
;   val  = Value to write
;
;Destroys:
;   EAX

PUBLIC SetAPURAM, addr:dword, val:byte


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Set APU Port
;
;Writes a value to APU RAM via the APU in ports.  This emulates writing to PPU registers 2140-2143h
;via the 65816.
;
;To change the APU out ports, write to function registers F4-F7h with SetAPURAM.
;
;In:
;   addr = Port on which to write (only the lower 2-bits are used)
;   val  = Value to write
;
;Destroys:
;   EAX

PUBLIC SetAPUPort, addr:dword, val:byte


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Get APU Port
;
;Reads the value stored in the APU out ports.  This emulates reading from PPU registers 2140-2143h
;via the 65816.
;
;In:
;   addr = Port to read value from (only the lower 2-bits are used)
;
;Out:
;   EAX = Value of port

PUBLIC GetAPUPort, addr:dword


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Get SPC700 Time
;
;Returns the number of 64kHz ticks that have gone by since the SPC700 was reset
;
;Out:
;   EAX = Number of 64kHz cycles that have elapsed

PUBLIC GetSPCTime, NULL


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Profile Timers
;
;In:
;   tsc -> Two qwords
;
;Destroys:
;   EDX

PUBLIC StartAPUProfile, tsc:ptr
PUBLIC EndAPUProfile, tsc:ptr


;ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ
;Emulate SPC700
;
;Emulates the SPC700 for the number of clock cycles specified or until a break condition is met,
;whichever happens first.  (see the compile options CNTBK and DSPBK in APU.Inc)
;
;In:
;   cyc = Number of 24.576MHz clock cycles to execute (signed, must be > 0)
;
;Out:
;   EAX = Clock cycles left to execute (negative if more cycles than specified were emulated)

PUBLIC EmuSPC, cyc:dword