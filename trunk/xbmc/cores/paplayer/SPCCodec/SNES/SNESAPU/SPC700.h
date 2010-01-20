/***************************************************************************************************
* Program:    Sony SPC700 Emulator                                                                 *
* Platform:   80386                                                                                *
* Programmer: Anti Resonance                                                                       *
*                                                                                                  *
* Thanks to Michael Abrash.  It was reading the Graphics Programming Black Book that gave me the   *
* idea and inspiration to write this in the first place.                                           *
*                                                                                                  *
* This library is free software; you can redistribute it and/or modify it under the terms of the   *
* GNU Lesser General Public License as published by the Free Software Foundation; either version   *
* 2.1 of the License, or (at your option) any later version.                                       *
*                                                                                                  *
* This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;        *
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.        *
* See the GNU Lesser General Public License for more details.                                      *
*                                                                                                  *
* You should have received a copy of the GNU Lesser General Public License along with this         *
* library; if not, write to the Free Software Foundation, Inc., 59 Temple Place, Suite 330,        *
* Boston, MA  02111-1307  USA                                                                      *
*                                                                                                  *
* ------------------------------------------------------------------------------------------------ *
* Revision History:                                                                                *
*                                                                                                  *
*  2005.xx.xx SNESAPU v3.0                                                                         *
*  + Updated EmuSPC to use a jump table to execute opcodes :(                                      *
*  + Removed InPortW                                                                               *
*  + Added GetAPUPort                                                                              *
*  + The memory address macros were getting the RAM pointer from pAPURAM instead of EDI            *
*  + Macro _mbit now returns a bit instead of a memory location                                    *
*  - Correcly moved the placement of volatile in the SPCDebug typedef                              *
*  - IPL ROM wasn't getting copied to RAM on reset (don't know how I missed this one)              *
*  - Fixed InitSPC and ResetSPC so function registers are properly preserved                       *
*  - Timer values are stored internally so reading FA-FC returns 0                                 *
*  - Rewrote speed hack (now it's faster and doesn't update disabled timers)                       *
*  + All global variables are now stored relative to EDI                                           *
*  + Common cleanup configurations are now jumped to instead of being expanded within the opcode   *
*  + CL contains the result of the previous operation and is used to determine the values of       *
*    N and Z.  Faster than setting the flags in memory.                                            *
*  + Post read check for function registers now operates like the post write                       *
*  + Added profiling counters                                                                      *
*                                                                                                  *
*  2004.08.01 SNESAPU v2.1                                                                         *
*  + Moved all checks for SPC_NODSP into DSP.Asm                                                   *
*                                                                                                  *
*  2004.01.26 SNESAPU v2.0                                                                         *
*  - Added code to call DSP emulator if 00F3 is read from                                          *
*  - Updated opcode fetch code to work with new DSP syncronization                                 *
*                                                                                                  *
*  2003.06.30                                                                                      *
*  - Fixed a bug where the counter would increase every pulse if the timer was disabled and set    *
*    to 0                                                                                          *
*                                                                                                  *
*  2003.02.10 SNESAPU v0.98                                                                        *
*  + Added SaveSPC, RestoreSPC, and SetAPURAM                                                      *
*  + Coded support for BRK (still not sure if it's correct)                                        *
*  - Rewrote SetSPCDbg and the opcode fetcher to fix some potential bugs                           *
*  - Added code to CntHack to try and isolate small polling loops (fixes the tempo problems some   *
*    games were having)                                                                            *
*  - Changed FixSPC, again, to copy extraRAM from APURAM[0FFC0h] if IPL reading is disabled in F1h *
*    (fixes Tales of Phantasia)                                                                    *
*  - Fixed a lame bug when clearing the in-ports with F1h (fixes Dragon Quest)                     *
*                                                                                                  *
*  2001.04.12 SNESAPU v0.95                                                                        *
*  + Added SPC_NODSP to the debugging options                                                      *
*  - Fixed some bugs with copying the IPL ROM and detecting reading ability                        *
*                                                                                                  *
*  2001.01.29 SNESAmp v2.1                                                                         *
*  - Changed internal clock to 1.024MHz (I haven't noticed any differences, yet)                   *
*                                                                                                  *
*  2001.01.01 SNESAmp v2.0                                                                         *
*  + Added an external counter to the 64kHz timer                                                  *
*  + Emulator is now based off of a 24.576MHz clock, so implementation of a 2.048 or 2.457MHz CPU  *
*    clock will be easier internally and transparent externally                                    *
*  + Added speed hack to SLEEP                                                                     *
*  + Added option flags to SPCDebug to allow greater control over the SPC700 state                 *
*  + Added breaking ability to STOP in non-debug builds                                            *
*  - In ports don't get cleared on load, regardless of setting in F1                               *
*  - Speed hack is now implemented properly                                                        *
*  - PC is now operated on as a 16-bit quantity (Rudra no Hihou expects PC to roll over, which it  *
*    wasn't doing when instructions were adding to ESI instead of SI)                              *
*  - Changed coding style a bit to make code easier to read and added more comments                *
*                                                                                                  *
*  2000.05.04 SNESAmp v1.2                                                                         *
*  - Control register is now set to 80h on reset                                                   *
*                                                                                                  *
*  2000.04.04 SNESAmp v1.0                                                                         *
*  + Rearranged code so static branch prediction would be more accurate                            *
*  + Clock cycles to emulate is passed on the stack to EmuSPC                                      *
*  + SPC700 registers are passed on the stack to the external debug routine                        *
*  + Eliminated ORG directive and rewrote code to conform to MASM syntax                           *
*  - Fixed some implementation bugs with the speed hack                                            *
*  - Fixed a bug in the 16-bit WrPost macro, again                                                 *
*                                                                                                  *
*  2000.03.17 SNESAmp v0.9                                                                         *
*  + Rewrote for compatibility in a 32-bit C environment                                           *
*  + Added external functions to write to ports                                                    *
*  - SLEEP wasn't erasing the port status on the first time through                                *
*                                                                                                  *
*  2000.02.22 SPC Tool v0.6                                                                        *
*  - Fixed a bug in the 16-bit WrPost macro                                                        *
*                                                                                                  *
*  2000.02.01 SPC Tool v0.51                                                                       *
*  + Added a hack to eliminate polling the counters                                                *
*  + Aligned many of the short jump destinations on paragraph boundaries                           *
*  + Rewrote the opcode header macro to compile into table form instead of packed form.            *
*    (The compiled code is four times bigger, but it's now easier to align, port, and debug.)      *
*  + Store flags as dwords so accesses to the DP don't cause a partial register stall              *
*  - Fixed PCALL, STOP, and SLEEP                                                                  *
*                                                                                                  *
*  2000.01.21 SPC Tool v0.5                                                                        *
*  + Reprogrammed PSW handling for faster entry/exit                                               *
*                                                                                                  *
*  1999.12.27 SPC Tool v0.2                                                                        *
*                                                      Copyright (C)1999-2006 Alpha-II Productions *
***************************************************************************************************/

namespace A2
{
namespace SNES
{

	//**********************************************************************************************
	// Defines

	static const s32	APU_CLK		= 24576000;	//Number of clock cycles in one second

	//SPC700 debugging options -----------------

	enum SPCDbgOpts
	{
		// Halt SPC700:
		//
		// This flag causes EmuSPC() to return that cycles were emulated, but nothing will have
		// happened.  When set from within the debugging callback, EmuSPC() will update the internal
		// registers then return before executing the next instruction.
		//
		// Executing a STOP or SLEEP instruction automatically sets this option.

		SPC_HALT = 7,

		// Only on STOP:
		//
		// By default, the debugging callback will be called before each instruction is executed.
		// If you only want the callback to get called when a STOP or SLEEP instruction is
		// encountered, specify this flag.

		SPC_STOP = 0
	};


	//**********************************************************************************************
	// Structures

	//Saved state ------------------------------
	struct PSW
	{
		u8		c:1;							//Carry
		u8		z:1;							//Zero
		u8		i:1;							//Interrupts Enabled (not used in the SNES)
		u8		h:1;							//Half-Carry (auxiliary)
		u8		b:1;							//Software Break
		u8		p:1;							//Direct Page Selector
		u8		v:1;							//Overflow
		u8		n:1;							//Negative (sign)
	};

	struct SPCState
	{
		void*	pRAM;							//[0.0] -> APU's RAM (64k)
		u16		pc;								//[0.4] Registers
		u8		a,y,x;
		PSW		psw;
		u16		sp;
		u8		outp[4];						//[0.C] Out ports

		u8		upCnt[3];						//[1.0] Up counters for counter increase
		u8		t8kHz;							//[1.3] # of cycles left until 8kHz timer increase
		u32		t64kHz;							//[1.4] # of cycles left until 64kHz timer increase
		u32		t64Cnt;							//[1.8] # of 64kHz ticks since emulation began
		u32		_r1;							//[1.C]
	};


	//**********************************************************************************************
	// SPC700 Debugging Routine
	//
	// A prototype for a function that gets called for debugging purposes
	//
	// The paramaters passed in can be modified, and on return will be used to update the internal
	// registers (except 'cnt').  Non-pointer values are zero extended.
	//
	// Note:
	//    When modifying PC or SP, only the lower word or byte needs to be modified, respectively;
	//     the upper bytes will be ignored.
	//
	// In:
	//    pc -> Current opcode (LOWORD = PC)
	//    ya  = YA
	//    x   = X
	//    psw = PSW
	//    sp -> Current stack byte (LOBYTE = SP)
	//    cnt = Clock cycle counters (four counters, one in each byte)
	//          [0-1] 8kHz up counters for timers 0 and 1
	//          [2]   64kHz up counter for timer 2
	//          [3]   CPU cycles left until next 64kHz clock pulse

	typedef void SPCDebug(u8* volatile pc, volatile u16 ya, volatile u8 x, volatile PSW psw,
						  u8* volatile sp, volatile u32 cnt);


#if !defined(SNESAPU_DLL) || defined(DONT_IMPORT_SNESAPU)

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Private Declarations

	extern "C" u8*	pAPURAM;					//Pointer to 64KB of APU RAM


	////////////////////////////////////////////////////////////////////////////////////////////////
	// External Functions

extern "C" {

	//**********************************************************************************************
	// Initialize SPC700
	//
	// This function is a remnant from the 16-bit assembly when dynamic code reallocation was used.
	// Now it just initializes internal pointers.
	//
	// Note:
	//    You should call InitAPU() instead
	//
	// Thread safe:
	//    No
	//
	// out:
	//    -> 64KB APU RAM

	u8* InitSPC();


	//**********************************************************************************************
	// Reset SPC700
	//
	// Sets all user memory to FF, resets the SPC700 execution time, resets the halt flag, and
	// copies ROM into the IPL region
	//
	// Note:
	//    You should call ResetAPU() instead
	//
	// Thread safe:
	//    No
	//
	// Destroys:
	//    EAX

	void ResetSPC();


	//**********************************************************************************************
	// Set SPC700 Debugging Routine
	//
	// Installs a callback that gets called before an instruction is executed, provided SPC700.Asm
	// was compiled with the SA_DEBUG option set.
	//
	// This function is not safe to call during emulation from another thread, but it may be called
	// from within the pDebug function.
	//
	// Note:
	//    pDebug is always called when a STOP or SLEEP instruction is encountered, regardless of if
	//     the SA_DEBUG option is set.
	//
	// Thread safe:
	//    No
	//
	// In:
	//    pDebug-> Debugging callback (NULL can be passed to disable the callback, -1 leaves the
	//             current callback in place)
	//    opts   = SPC700 debugging flags (see SPCDbgOpts, Set::All() leaves the current flags)
	//
	// Out:
	//    Previously installed callback

	SPCDebug* SetSPCDbg(SPCDebug* pDebug, Set<SPCDbgOpts> opts = Set<SPCDbgOpts>());


	//**********************************************************************************************
	// Fix SPC700 After Loading SPC File
	//
	// Loads timer steps with the values in the timer registers, resets the counters, sets up the
	// in/out ports, and stores the registers.
	//
	// Note:
	//    You should call FixAPU() instead
	//
	// Thread safe:
	//    No
	//
	// In:
	//    SPC internal registers
	//
	// Destroys:
	//    EAX

	void FixSPCLoad(u16 pc, u8 a, u8 y, u8 x, u8 psw, u8 sp);


	//**********************************************************************************************
	// Save SPC700 State
	//
	// Note:
	//    Pointers in the SPCState structure can be NULL
	//
	// Thread safe:
	//    No
	//
	// In:
	//    pState -> Saved state structure
	//
	// Destroys:
	//    EAX

	void SaveSPC(SPCState* pState);


	//**********************************************************************************************
	// Restore SPC700 State
	//
	// Note:
	//    Setting a pointer in SPCState to NULL will keep its corresponding internal data intact.
	//
	// Thread safe:
	//    No
	//
	// In:
	//    state -> Saved state structure
	//
	// Destroys:
	//    EAX

	void RestoreSPC(const SPCState& state);


	//**********************************************************************************************
	// Write to APU RAM via the SPC700
	//
	// Writes a value to APU RAM via the SPC700.  Use this instead of writing to RAM directly so any
	// changes made by writing to function registers or the IPL region will be handled properly.
	//
	// Thread safe:
	//    No
	//
	// In:
	//    addr = Address to write to (only the lower 16-bits are used)
	//    val  = Value to write
	//
	// Destroys:
	//    EAX

	void SetAPURAM(u32 addr, u8 val);


	//**********************************************************************************************
	// Set APU Port
	//
	// Writes a value to APU RAM via the APU in ports.  This emulates writing to PPU registers
	// 2140-2143h via the 65816.
	//
	// To change the APU out ports, write to function registers F4-F7h with SetAPURAM().
	//
	// Thread safe:
	//    No
	//
	// In:
	//    port = Port on which to write (only the lower 2-bits are used)
	//    val  = Value to write
	//
	// Destroys:
	//    EAX

	void SetAPUPort(u32 addr, u8 val);


	//**********************************************************************************************
	// Get APU Port
	//
	// Reads the value stored in the APU out ports.  This emulates reading from PPU registers 2140-2143h
	// via the 65816.
	//
	// Thread safe:
	//    Yes
	//
	// In:
	//    addr = Port to read value from (only the lower 2-bits are used)

	u8 GetAPUPort(u32 addr);


	//**********************************************************************************************
	// Get SPC700 Time
	//
	// Returns the number of 64kHz ticks that have gone by since the SPC700 was reset
	//
	// Thread safe:
	//    Yes
	//
	// Out:
	//    Number of 64kHz cycles that have elapsed

	u32 GetSPCTime();


	//**********************************************************************************************
	// Emulate SPC700
	//
	// Emulates the SPC700 for the number of clock cycles specified or until a break condition is
	// met, whichever happens first.  (see compile options SA_CNTBK and SA_DSPBK)
	//
	// Notes:
	//    You should call EmuAPU() instead
	//    Passing values <= 0 will cause undeterminable results
	//
	// In:
	//    cyc = Number of 24.576MHz clock cycles to execute (must be > 0)
	//
	// Out:
	//    Clock cycles left to execute (negative if more cycles than specified were emulated)

	s32 EmuSPC(s32 cyc);

}	// extern "C"

#endif	//!SNESAPU_DLL

}	// namespace SNES
}	// namespace A2

