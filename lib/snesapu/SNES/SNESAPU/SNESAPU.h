/***************************************************************************************************
* Program:    Super Nintendo Entertainment System(tm) Audio Processing Unit Emulator DLL           *
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
*  2005.10.28 SNESAmp v3.3                                                                         *
*  + Changed C interface to C++                                                                    *
*  + Removed all sample packing code                                                               *
*  + Made GetAPUData() better                                                                      *
*                                                                                                  *
*  2004.xx.xx SNESAmp v3.2                                                                         *
*  - Added a build check for SSE2 processors in EightToSixteen()                                   *
*  - Fixed the memcpy() in BitRateReduce() when the loop point was < 16                            *
*                                                                                                  *
*  2004.01.26                                                                                      *
*  + Added BitRateReduce, a better function for creating BBR blocks from raw samples               *
*                                                                                                  *
*  2003.02.10 SNESAmp v2.99                                                                        *
*  + Added version information to the DLL                                                          *
*  + Moved GetAPUData to SNESAPU.cpp, since it's only needed for SNESAPU.DLL                       *
*  + Moved memory allocation into SNESAPU.cpp, since memory allocation is system, and possibly     *
*    application, specific                                                                         *
*                                                                                                  *
*  2001.01.01 SNESAmp v2.0                                                                         *
*  + Made some changes to the internal interface with the DSP                                      *
*                                                                                                  *
*  2000.12.14 Super Jukebox v3.0                                                                   *
*                                                      Copyright (C)2001-2006 Alpha-II Productions *
***************************************************************************************************/

#ifdef __SNESAPU_H__
#error SNESAPU.h may only be #included once per file
#endif
#define __SNESAPU_H__

#define	SNESAPU_DLL	0x03000000					//Don't include local function/variable definitions

#include	"SPC700.h"
#include	"DSP.h"
#include	"APU.h"

namespace A2
{
namespace SNES
{

	//**********************************************************************************************
	// Type Definitions

	//See GetAPUData() for explanations of each
	enum DataType
	{
		DATA_OPTIONS = -1,
		DATA_RAM,
		DATA_DSP,
		DATA_MIX,
		DATA_PROFILE
	};

#ifndef DONT_IMPORT_SNESAPU

	//Function pointers to SNESAPU
	struct Functions
	{
		void*		(__stdcall *GetAPUData)(DataType type);

		void		(__stdcall *ResetAPU)();
		void		(__stdcall *LoadSPCFile)(const void* pSPC);
		void		(__stdcall *SaveAPU)(SPCState* pSPC, DSPState* pDSP);
		void		(__stdcall *RestoreAPU)(const SPCState& spc, const DSPState& dsp);
		void		(__stdcall *SetAPUOpt)(Mixing, u32 chn, u32 bits, u32 rate,
										   DSPInter inter = INT_INVALID,
										   Set<DSPOpts> opts = ~Set<DSPOpts>());
		void		(__stdcall *SetAPUSmpClk)(u32 speed);
		void*		(__stdcall *EmuAPU)(void* pBuf, u32 cycles, u32 samples);
		void		(__stdcall *SeekAPU)(u32 time, bool fast);

		SPCDebug*	(__stdcall *SetSPCDbg)(SPCDebug* pTrace,
										   Set<SPCDbgOpts> opts = Set<SPCDbgOpts>());
		void		(__stdcall *SetAPURAM)(u32 addr, u8 val);
		void		(__stdcall *SetAPUPort)(u32 port, u8 val);
		u8			(__stdcall *GetAPUPort)(u32 port);
		u32			(__stdcall *GetSPCTime)();
		s32			(__stdcall *EmuSPC)(u32 cyc);

		u32			(__stdcall *GetProcInfo)();
		DSPDebug*	(__stdcall *SetDSPDbg)(DSPDebug* pTrace,
										   Set<DSPDbgOpts> opts = Set<DSPDbgOpts>());
		void		(__stdcall *SetDSPAmp)(u32 level);
		u32			(__stdcall *GetDSPAmp)();
		void		(__stdcall *SetDSPAAR)(AARType type, u32 thresh, u32 min, u32 max);
		u32			(__stdcall *SetDSPLength)(u32 song, u32 fade);
		void		(__stdcall *SetDSPPitch)(u32 base);
		void		(__stdcall *SetDSPStereo)(u32 sep);
		void		(__stdcall *SetDSPEFBCT)(s32 leak);
		bool		(__stdcall *SetDSPVoiceMute)(u32 voice, MuteState state);
		bool		(__stdcall *SetDSPReg)(u8 reg, u8 val);
		void*		(__stdcall *EmuDSP)(void* pBuf, s32 size);
		void*		(__stdcall *PlaySrc)(void* pSrc, u32 loop, u32 rate);
		u32			(__stdcall *UnpackSrc)(s16* pBuf, const void* pSrc, u32 num,
										   Set<UnpackOpts> opts = Set<UnpackOpts>(),
										   s32* prev1 = 0, s32* prev2 = 0);
		void*		(__stdcall *PackSrc)(void* pBlk, const void* pSmp, u32* pLen, u32* pLoop,
										 Set<PackOpts> opts = Set<PackOpts>(),
										 s32* prev1 = 0, s32* prev2 = 0);
		void		(__stdcall *VMax2dBf)(MaxOutF* p);
		void		(__stdcall *VMax2dBi)(MaxOutI* p);
	};


	//**********************************************************************************************
	//External Functions - See other header files for exported function descriptions

	#ifndef import
	#define	import	__declspec(dllimport)
	#endif

extern "C" {

	//**********************************************************************************************
	// Get Pointer to APU Internal Data
	//
	// Returns pointers to internal data structures.  Access is provided solely for debugging
	// purposes.  All data should be considered read-only.
	//
	// DATA_OPTIONS
	//    Returns the build options specified in APU.Inc (the return value is a u32, not a void*)
	//    bits 23-0  Options (see #define SA_??? in APU.h)
	//    bits 31-24 SPC700 clock divisor (divide APU_CLK by the clock divisor to get the emulated
	//               clock speed of the SPC700)
	//
	// DATA_RAM
	//    Returns a pointer to the 64KB of APU RAM - apuRAM[65536]
	//
	// DATA_DSP
	//    Returns a pointer to the 128 bytes of DSP registers - DSPReg (DSP.h)
	//
	// DATA_MIX
	//    Returns a pointer to an array of 8 MixData structures - Voice[8] (DSP.h)
	//
	// DATA_PROFILE
	//    Returns a pointer to a profiling structure - APUProf (SPC700.h)
	//    If profiling isn't enabled (see #define SA_PROFILE, APU.h) the pointer returned will be
	//    NULL.
	//
	// In:
	//    type = Pointer to retrieve (see DataType)
	//
	// Out:
	//    Pointer to requested data

	import	void*			__stdcall GetAPUData(DataType type);


	//**********************************************************************************************
	// APU Functions

	import	void		__stdcall ResetAPU();
	import	void		__stdcall LoadSPCFile(const void* pSPC);
	import	void		__stdcall SaveAPU(SPCState* pSPC, DSPState* pDSP);
	import	void		__stdcall RestoreAPU(const SPCState& spc, const DSPState& dsp);
	import	void		__stdcall SetAPUOpt(Mixing mix, u32 chn, u32 bits, u32 rate,
											DSPInter inter = INT_INVALID,
											Set<DSPOpts> opts = ~Set<DSPOpts>());
	import	void		__stdcall SetAPUSmpClk(u32 speed);
	import	void*		__stdcall EmuAPU(void* pBuf, u32 cycles, u32 samples);
	import	void		__stdcall SeekAPU(u32 time, bool fast);


	//**********************************************************************************************
	// SPC700 Functions

	import	SPCDebug*	__stdcall SetSPCDbg(SPCDebug* pTrace,
											Set<SPCDbgOpts> opts = Set<SPCDbgOpts>());
	import	void		__stdcall SetAPURAM(u32 addr, u8 val);
	import	void		__stdcall SetAPUPort(u32 port, u8 val);
	import	u8			__stdcall GetAPUPort(u32 port);
	import	u32			__stdcall GetSPCTime();
	import	s32			__stdcall EmuSPC(u32 cyc);


	//**********************************************************************************************
	// DSP Functions

	import	u32			__stdcall GetProcInfo();
	import	DSPDebug*	__stdcall SetDSPDbg(DSPDebug* pTrace,
											Set<DSPDbgOpts> opts = Set<DSPDbgOpts>());
	import	void		__stdcall SetDSPAmp(u32 level);
	import	u32			__stdcall GetDSPAmp();
	import	void		__stdcall SetDSPAAR(AARType type, u32 thresh, u32 min, u32 max);
	import	u32			__stdcall SetDSPLength(u32 song, u32 fade);
	import	void		__stdcall SetDSPPitch(u32 base);
	import	void		__stdcall SetDSPStereo(u32 sep);
	import	void		__stdcall SetDSPEFBCT(s32 leak);
	import	bool		__stdcall SetDSPVoiceMute(u32 voice, MuteState state);
	import	bool		__stdcall SetDSPReg(u8 reg, u8 val);
	import	void*		__stdcall EmuDSP(void* pBuf, s32 size);
	import	void*		__stdcall PlaySrc(void* pSrc, u32 loop, u32 rate = 0);
	import	u32			__stdcall UnpackSrc(s16* pBuf, const void* pSrc, u32 num,
											Set<UnpackOpts> opts = Set<UnpackOpts>(),
											s32* prev1 = 0, s32* prev2 = 0);
	import	void*		__stdcall PackSrc(void* pBlk, const void* pSmp, u32* pLen, u32* pLoop,
										  Set<PackOpts> opts = Set<PackOpts>(),
										  s32* prev1 = 0, s32* prev2 = 0);
	import	void		__stdcall VMax2dBf(MaxOutF* p);
	import	void		__stdcall VMax2dBi(MaxOutI* p);

}	// extern "C"

#endif	//DONT_IMPORT_SNESAPU

}	// namespace SNES
}	// namespace A2

