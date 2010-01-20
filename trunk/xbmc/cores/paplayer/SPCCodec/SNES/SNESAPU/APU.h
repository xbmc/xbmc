/***************************************************************************************************
* Program:    Super Nintendo Entertainment System(tm) Audio Processing Unit Emulator               *
* Platform:   Intel 80386 & MMX                                                                    *
* Programmer: Anti Resonance                                                                       *
*                                                                                                  *
* "SNES" and "Super Nintendo Entertainment System" are trademarks of Nintendo Co., Limited and its *
* subsidiary companies.                                                                            *
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
*  2005.10.28 SNESAPU 3.0
*  + Added NULL pointer support to SaveAPU and RestoreAPU                                          *
*  + Moved variables into the .bss                                                                 *
*  + The build options are now returned by InitAPU instead of being stored in a global variable    *
*  - Calculate the emulated sample rate at the top of EmuAPU so SetAPUSmpClk will be thread safe   *
*                                                                                                  *
*  2003.11.04 SNESAPU 2.0                                                                          *
*  + Updated EmuAPU to use SetEmuDSP when DSPINTEG is enabled                                      *
*  + Move fade out code into DSP.Asm                                                               *
*  + Removed amp setting from ResetAPU                                                             *
*                                                                                                  *
*  2003.07.12 SNESAPU 1.0a                                                                         *
*  - Fixed the call to SetDSPVol in SetFade that wasn't restoring the stack                        *
*                                                                                                  *
*  2003.06.20                                                                                      *
*  + Added LoadSPCFile                                                                             *
*  + Added ability to pass -1 to ResetAPU to keep current amp level                                *
*                                                                                                  *
*  2003.02.10                                                                                      *
*  + Rewrote in assembly                                                                           *
*  + Removed GetAPUData and memory allocation (they were specific to SNESAPU.DLL)                  *
*  - Sometimes EmuAPU would generate an extra sample                                               *
*                                                                                                  *
*  2002.06.20                                                                                      *
*  - Call SetFade after seeking                                                                    *
*                                                                                                  *
*  2001.04.12                                                                                      *
*  + Updated SeekAPU to incorporate new features in SPC700 and DSP emus                            *
*                                                                                                  *
*  2000.12.30 SNESAmp v2.0                                                                         *
*  + Added SetAPULength, SeekAPU, SaveAPU, RestoreAPU, HaltAPU                                     *
*  + General optimizations                                                                         *
*  - Simplified interface a bit                                                                    *
*                                                                                                  *
*  2000.04.04 SNESAmp v1.0                                                                         *
*                                                                                                  *
*  2000.03.17 SNESAmp v0.9                                                                         *
*                                                        Copyright (C)2003-06 Alpha-II Productions *
***************************************************************************************************/

namespace A2
{
namespace SNES
{

	//**********************************************************************************************
	// Compile Options

	//The build options correspond to the equates at the top of APU.Inc

	enum APUOpts
	{
		//SPC700 clock speed divisor (bits 7-0)
		SA_CLKDIV	= 0x00000FF,				//Mask for retieving the clock divisor

		//APU build options (bits 15-8) --------
		SA_DEBUG	= 0x0000100,				//Debugging ability enabled
		SA_DSPINTEG	= 0x0000200,				//DSP emulation is integrated with the SPC700

		//SPC700 build options (bits 23-16) ----
		SA_HALFC	= 0x0010000,				//Half-carry flag emulation enabled
		SA_IPLW		= 0x0080000,				//IPL ROM region writeable
		SA_CNTBK	= 0x0020000,				//Break SPC700/Update DSP if a counter is increased
		SA_DSPBK	= 0x0100000,				//Break SPC700/Update DSP if 0F3h is read from
		SA_SPEED	= 0x0040000,				//Hacks are enabled to skip cycles when a counter is
												// read or the processor is sleeping
		SA_PROFILE	= 0x0200000,				//Profiling counters enabled

		//DSP build options (bits 31-24) -------
		SA_MAINPPM	= 0x1000000,				//Peak program metering on main output (for AAR)
		SA_VOICEPPM	= 0x2000000,				//Peak program metering on voices (for visualization)
		SA_STEREO	= 0x4000000					//Stereo controls enabled (seperation and EFBCT)
	};


	//**********************************************************************************************
	// Structures

	//Profiling counters -----------------------
	struct Profile
	{
		struct Counter
		{
			u64		acc;						//Accumulated time spent executing
			u64		base;						//Temp value (holds the TSC upon entrance)
		};

		u32			exec[256];					//Number of times instruction has been executed

		struct Branch							//For branch instructions executed, the number of
		{										// times the branch was taken:
			u32		bxx[8];						//BPL, BMI, BVC, BVS, BCC, BCS, BNE, BEQ
			u32		bbc[8];						//BBC.bit
			u32		bbs[8];						//BBS.bit
			u32		cbne[2];					//CBNE dp, CBNE dp+X
			u32		dbnz[2];					//DBNZ Y, DBNZ dp
		} branch;

		struct Func								//Number of times a function register was accessed
		{
			u32		read[16];
			u32		write[16];
			u32		write16[16];				//16-bit write via MOVW, INCW, or DECW
		} func;

		struct DSP
		{
			u32		read[128];					//DSP register was read from
			u32		write[256];					//DSP register was written to

			struct Update						//Used when DSP integration is enabled
			{
				u32	reg[128];					//Register write that caused an update
				u32	num;						//Number of times DSP got updated during SPC700 emu.
				u32	read;						//Number of those times because of a register read
				u32	gen;						//Number of times the update generated output
				u32	_r;
			} update;
		} dsp;

		struct TSC								//Time-stamp counters
		{
			Counter	host;						//Place holder for user to store total execution time
			Counter	apu;						//Time spent in EmuAPU()
			Counter	spc700;						//Time spent in EmuSPC()
			Counter	dsp;						//Time spent in EmuDSP()
		} tsc;
	};


#if !defined(SNESAPU_DLL) || defined(DONT_IMPORT_SNESAPU)

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// Private Declarations

	extern "C" Profile	profile;				//Profiling counters


	////////////////////////////////////////////////////////////////////////////////////////////////
	// External Functions

extern "C" {

	//**********************************************************************************************
	// Initialize Audio Processing Unit
	//
	// Initializes internal tables, mixer settings, memory, and registers.
	// This only needs to be called once.
	//
	// see InitSPC() and InitDSP()
	//
	// Out:
	//    APU build options (see the SA_??? above, APU.Inc, and SNESAPU.h GetAPUData())

	u32 InitAPU();


	//**********************************************************************************************
	// Reset Audio Processor
	//
	// Clears all memory and sets registers to their default values
	//
	// Thread safe:
	//    No
	//
	// see ResetSPC() and ResetDSP()

	void ResetAPU();


	//**********************************************************************************************
	// Load SPC File
	//
	// Restores the APU state from an .SPC file.  You do not need to call ResetAPU() before loading
	// a file.
	//
	// Thread safe:
	//    No
	//
	// In:
	//    pFile -> 66048 byte SPC file

	void LoadSPCFile(const void* pFile);


	//**********************************************************************************************
	// Save Audio Processor State
	//
	// Creates a saved state of the audio processor
	//
	// see SaveSPC() and SaveDSP()
	//
	// Thread safe:
	//    No
	//
	// In:
	//    pSPC -> SPCState structure (can be NULL)
	//    pDSP -> DSPState structure (can be NULL)

	void SaveAPU(SPCState* pSPC, DSPState* pDSP);


	//**********************************************************************************************
	// Restore Audio Processor State
	//
	// Restores the audio processor from a saved state
	//
	// see RestoreSPC() and RestoreDSP()
	//
	// Thread safe:
	//    No
	//
	// In:
	//    spc -> SPCState structure (can be NULL)
	//    dsp -> DSPState structure (can be NULL)

	void RestoreAPU(const SPCState& spc, const DSPState& dsp);


	//**********************************************************************************************
	// Set Audio Processor Options
	//
	// Configures the sound processor emulator.  Range checking is performed on all parameters.
	//
	// Notes:  -1 can be passed for any parameter you want to remain unchanged
	//         see SetDSPOpt() in DSP.h for a more detailed explantion of the options
	//
	// Thread safe:
	//    No, except when only changing the interpolation type
	//
	// In:
	//    mix   = Mixing routine (default MIX_INT)
	//    chn   = Number of channels (1 or 2, default 2)
	//    bits  = Sample size (8, 16, 24, 32, or -32 [IEEE 754], default 16)
	//    rate  = Sample rate (8000-192000, default 32000)
	//    inter = Interpolation type (default INT_GAUSS)
	//    opts  = See enum DSPOpts

	void SetAPUOpt(Mixing mix, u32 chn, u32 bits, u32 rate, DSPInter inter = INT_INVALID,
				   Set<DSPOpts> opts = ~Set<DSPOpts>());


	//**********************************************************************************************
	// Set Audio Processor Sample Clock
	//
	// Calculates the ratio of emulated clock cycles to sample output.  Used to speed up or slow
	// down a song without affecting the pitch.  Range checking is performed.
	//
	// This function is safe to call while the APU is emulated in another thread
	//
	// Thread safe:
	//    Yes
	//
	// In:
	//    speed = Multiplier [16.16] (1/16x to 16x)

	void SetAPUSmpClk(u32 speed);


	//**********************************************************************************************
	// Emulate Audio Processing Unit
	//
	// Emulates the APU for a specified amount of time.  DSP output is placed in a buffer to be
	// handled by the main program.
	//
	// Calling EmuAPU() instead of EmuSPC() and EmuDSP() will ensure the two processors are kept in
	// sync with each other.
	//
	// If the cycle count is 0, the SPC700 will be emulated until the DSP has filled the output
	// buffer with the specified number of samples.
	//
	// If the sample count is 0, the DSP will be produce output for the same amount of time as the
	// SPC700 is emulated.
	//
	// If the cycle count and sample count aren't equal in terms of time, then one processor will be
	// halted at the end of its count while the other processor is continued.
	//
	// In:
	//    pBuf   -> Buffer to store output samples (can be NULL)
	//    cycles  = Number of clock cycles to emulate SPC700 for (APU_CLK = 1 second)
	//    samples = Number of samples for the DSP to generate
	//
	// Out:
	//    -> End of buffer

	void* EmuAPU(void* pBuf, u32 cycles, u32 samples);


	//**********************************************************************************************
	// Seek to Position
	//
	// Seeks forward in the song from the current position
	//
	// In:
	//    time = 1/64000ths of a second to seek forward (must be >= 0)
	//    fast = Use faster seeking method (may break some songs)

	void SeekAPU(u32 time, bool fast);


	//**********************************************************************************************
	// Shutdown Audio Processing Unit
	//
	// Currently does nothing

	void ShutAPU();

}	// extern "C"

#endif	//!SNESAPU_DLL

}	// namespace SNES
}	// namespace A2

