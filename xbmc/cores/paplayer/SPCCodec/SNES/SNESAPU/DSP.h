/***************************************************************************************************
* Program:    Super Nintendo Entertainment System(tm) Digital Signal Processor Emulator            *
* Platform:   80386 / MMX                                                                          *
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
*  2005.xx.xx SNESAPU v3.0                                                                         *
*  + Added 8-point sinc interpolation                                                              *
*  + Added per voice anti-aliasing                                                                 *
*  + Added quadraphonic output to the floating-point mixing routine                                *
*  + Removed the branches in the volume ramping code in EmuDSPF                                    *
*  + Updated interpolation functions so PlaySrc could use them                                     *
*  + Added -6dB attenuation to PlaySrc, so single sounds aren't so loud compared to the relative   *
*    quietness of songs                                                                            *
*  - Volume ramping was disabled when stereo controls were turned off (not sure why)               *
*  - Fixed reverse stereo not working when stereo conrols were enabled, for real this time         *
*  - Correcly moved the placement of volatile in the DSPDebug typedef                              *
*  + Changed debugging interface                                                                   *
*  + Removed option for generating Gaussian curve                                                  *
*  + DSP register jump table gets initialized at run-time                                          *
*  - Fixed a bug in EmuDSP where muting the output buffer would crash                              *
*  - Fixed a bug when resetting the DSP with the FLG register                                      *
*  + Made floating-point constants local variables in most functions                               *
*  + Created macros to calculate relative displacments for some global variables                   *
*  - Fixed DSPIn to store the new value in the registers after UpdateDSP is called.  Also moved    *
*    UpdateDSP to get called only if the register handler will be called.                          *
*                                                                                                  *
*  2004.08.01 SNESAPU v2.1                                                                         *
*  + Improved monaural volume setting                                                              *
*  + Removed BRR look-up table                                                                     *
*  + Moved all checks for SPC_NODSP from SPC700.Asm                                                *
*  - Envelope height in direct gain mode could be garbage                                          *
*  - Added a missing 16-bit clamp to the MMX Gaussian interpolation                                *
*  - Added a 16-bit clamp to the MMX FIR filter to work around a distortion bug in DQK             *
*  - StartSrc wasn't wrapping source directory around 64k                                          *
*                                                                                                  *
*  2004.01.26 SNESAPU v2.0                                                                         *
*  + Added 24- and 32-bit sample support to MMX                                                    *
*  + Hardware anomalies emulates the DSP at 32kHz then upsamples the output                        *
*  + Replaced fake low-pass filter with a 32-tap FIR filter                                        *
*  + Writing to the DSP registers from the SPC700 automatically calls EmuDSP.  This should help    *
*    keep the DSP and SPC700 in better syncronization.                                             *
*  + Removed self modifying code                                                                   *
*  + Moved fade out from APU.Asm                                                                   *
*  - Output is inverted only when hardware anomalies are enabled                                   *
*  - Obtained a more accurate copy of the gaussian look-up table used in the SNES                  *
*  - Gaussian interpolation correctly clears the LSB in MMX mixing mode                            *
*  - When using 4-point interpolation, source playback begins with the fourth sample               *
*  - Removed 17-bit clamp from filter 1                                                            *
*  - Envelopes are not based on a 33kHz clock (duh...)                                             *
*  - Envelopes now use a sample counter instead of a fractional clock                              *
*  - FIR filter coefficients were getting applied in reverse order                                 *
*  - Filtered samples get shifted right before accumulation, and LSB is cleared after accum.       *
*  - Fixed PckWav (it was seriously broken)                                                        *
*  - Added support for BRR_??? options to PckWav                                                   *
*                                                                                                  *
*  2003.07.29 SNESAPU v1.01                                                                        *
*  - Voice's inactive flag wasn't getting set after a key off                                      *
*                                                                                                  *
*  2003.06.30                                                                                      *
*  - Made some minor changes to RestoreDSP                                                         *
*  - Fixed voices' pitch not getting changed in SetDSPOpt                                          *
*  - Fixed a bug in the floating-point DSP volume register handler when STEREO = 0                 *
*                                                                                                  *
*  2003.06.18 SNESAPU v0.99c                                                                       *
*  - Fixed interger overflow in MMX FIR filter                                                     *
*                                                                                                  *
*  2003.06.14 SNESAPU v0.99b                                                                       *
*  + Added support for old amplification values (<= 256)                                           *
*                                                                                                  *
*  2003.05.19 SNESAPU v0.99a                                                                       *
*  - Fixed a bug in the i386 FIR filter                                                            *
*                                                                                                  *
*  2003.04.28 SNESAPU v0.99                                                                        *
*  + InitDSP will select 386 or MMX depending on CPU                                               *
*  + Added 16 and 24-bit sample output to EmuDSPF                                                  *
*  + Changed SetDSPAmp to decibels                                                                 *
*  - Fixed several bugs in the envelope code so envelopes now work the same as the SNES (fixes a   *
*    lot of songs)                                                                                 *
*  - Stopped caching loop points (fixes Castlevania IV, which likes to change sources after a      *
*    key on)                                                                                       *
*  - Added clamping to the pitch modulation code (fixes CT)                                        *
*  - Fixed a bug in KON/KOF emulation                                                              *
*  - Changed 16-bit mixing process to emulate the SNES                                             *
*  - Improved low-pass filtering and simulated RF interference                                     *
*  - Removed floating-point support from SetDSPAmp                                                 *
*  - Fixed another volume bug in SetDSPOpt when switching between integer and float routines       *
*                                                                                                  *
*  2003.02.10 SNESAPU v0.98                                                                        *
*  + Converted to NASM                                                                             *
*  + Added SetDSPDbg, SaveDSP, RestoreDSP, and SetDSPReg                                           *
*  + Added a loop to EmuDSP so the number of samples passed in can be any value                    *
*  + Added floating-point support to SetDSPAmp                                                     *
*  + Added micro ramping to the channel volumes in the floating-point routine                      *
*  + Added 32-bit IEEE 754 output                                                                  *
*  - Added a 17-bit clamp to the UnpckWav (fixes Hiouden; CT and FF6 are iffy now)                 *
*  - Added linear interpolation to FIR filter (fixes some songs that turned to noise)              *
*  - SetDSPOpt wasn't clearing EAX, which was causing dspDecmp to sometimes contain a bad value    *
*  - ENVX always gets updated, regardless of ADSR/GAIN flag (fixes Clue)                           *
*                                                                                                  *
*  2001.10.02 SNESAPU v0.96                                                                        *
*  - freqTab was built from the wrong values (Hz instead of samples)                               *
*  - rateTab now uses 32-bits of precision instead of 16                                           *
*  - Values in rateTab >= 1.0 are now set to 0.99 to reduce the ill effects of fast envelopes when *
*    the sample rate is < 32kHz                                                                    *
*  - Reset all internal pitch values when PMON is written to (fixes Asterix)                       *
*  - Clear bits in KOF if voice is inactive (improves EOS detection)                               *
*  - Switched to a static lookup table generated by the SNES for Gaussian interpolation            *
*  - Damn near cracked the sample decompression                                                    *
*  - Optimized code a bit in EmuDSPN and inadvertently fixed a bug that was causing some songs to  *
*    break after seeking                                                                           *
*  - EmuDSPN wasn't checking the loop counter after an envelope update was performed (seeking      *
*    no longer appears to hang)                                                                    *
*                                                                                                  *
*  2001.05.15 SNESAPU v0.95                                                                        *
*  + Added 4-point Gaussian interpolation                                                          *
*  + EmuDSPN now decompresses samples, so seeking doesn't lose voices                              *
*  + Added an option to FixSeek to disable resetting voices                                        *
*  + Reverse stereo swaps the left/right volumes instead of physically swapping samples            *
*  + More minor optimizations                                                                      *
*  - Changed the sample decompression routine.  Less songs should have problems.                   *
*  - Echo delay was incorrect for most sample rates                                                *
*  - Did something to fix the noise generator not working in some songs                            *
*                                                                                                  *
*  2001.03.02 SNESAPU v0.92                                                                        *
*  - Fixed the low-pass filter (I'm amazed it was sort of working at all)                          *
*  - Added a hack to UnpckWav to subtract 3 from the range if > 12                                 *
*  - Forgot that the DSP needs to be limited to 64K as well (Secret of Evermore doesn't crash)     *
*  - Fixed a problem in the MMX dynamic code allocation                                            *
*  - FixSeek wasn't resetting the DSP X registers (seeking works in Der Langrisser)                *
*                                                                                                  *
*  2001.01.26 SNESAPU v0.90                                                                        *
*  + Added voice monitor and VMax2dB for visualization                                             *
*  + Added SetDSPPitch so people with SB/SBPro's and compatibles can hear the correct tone         *
*  + Added SetDSPStereo for headphone listeners                                                    *
*  + Added SetDSPEFBCT to make the echo sound more natural                                         *
*  + Added UnpackWave so waveforms could be decompressed by external programs                      *
*  + Added 32-bit floating point mixer for people with 24/32-bit sound cards                       *
*  + Added a reverse stereo option                                                                 *
*  + Added GetProcInfo to detect 3DNow! and SSE capabilites                                        *
*  + Added EmuDSP which automatically calls the mixing routine set in SetDSPOpt                    *
*  + Made a lot of minor optimizations to DSPIn and EmuDSP                                         *
*  + Made separate register handlers in DSPIn for mono and floating-point                          *
*  - Changed SetDSPOpt to use individual parameters instead of a record                            *
*  - SetDSPOpt performs range checking                                                             *
*  - Fixed configuration implementation so options don't need to be reset for every song and they  *
*    can be changed between calls to EmuDSP                                                        *
*  - Fixed mono related code (sounds better now)                                                   *
*  - Echo filter was never getting turned on                                                       *
*  - Implemented a better low-pass filter                                                          *
*                                                                                                  *
*  2000.05.22 SNESAmp v1.2                                                                         *
*  + Added a low-pass filter                                                                       *
*  + Added output monitor                                                                          *
*  + Optimized the waveform code in EmuDSPN                                                        *
*  + Added FixSeek                                                                                 *
*  + SetDSPVol now uses a 16-bit value                                                             *
*  - FixDSP was marking voices as keyed even if the pitch was 0                                    *
*  - Correctly implemented echo volume (should be perfect now)                                     *
*                                                                                                  *
*  2000.04.05 SNESAmp v1.0                                                                         *
*  - Voices with a pitch of 0 can't be keyed on                                                    *
*  - EnvH becomes 0 when voice is keyed off                                                        *
*  - EnvH is set to 0 when envelope is in gain mode                                                *
*  - Account for sample blocks with a range greater than 16-bits (ADPCM still not correct)         *
*                                                                                                  *
*  2000.03.17 SNESAmp v0.9                                                                         *
*                                                      Copyright (C)1999-2006 Alpha-II Productions *
***************************************************************************************************/

namespace A2
{
namespace SNES
{

	//**********************************************************************************************
	//Defines

	//CPU capability ---------------------------
	enum CPUCaps
	{
		CPU_MMX		= 1,						//Multi-Media eXtensions
		CPU_3DNOW	= 2,						//3DNow! extensions
		CPU_SSE		= 4							//Streaming SIMD Extensions
	};

	//Mixing routines (see DSP.Asm for details of each routine)
	enum Mixing
	{
		MIX_INVALID =	-1,
		MIX_NONE =		0,						//No mixing
		MIX_INT =		1,						//Use integer math
		MIX_FLOAT =		3						//Use floating-point math
	};

	//Interpolation routines -------------------
	enum DSPInter
	{
		INT_INVALID = -1,
		INT_NONE,								//None
		INT_LINEAR,								//Linear
		INT_CUBIC,								//Cubic
		INT_GAUSS,								//4-point Gaussian
		INT_SINC,								//8-point Sinc
		INT_TOTAL
	};

	//DSP options ------------------------------
	enum DSPOpts
	{
		OPT_ANALOG,								//Simulate anomalies of the analog hardware
		OPT_OLDSMP,								//Old sample decompression routine
		OPT_SURND,								//"Surround" sound
		OPT_REVERSE,							//Reverse stereo samples
		OPT_NOECHO,								//Disable echo
		OPT_FILTER,								//Pass each voice through an anti-aliasing filter
	};

	//Automatic Amplification Reduction --------
	enum AARType
	{
		AAR_OFF,								//Disable AAR
		AAR_ON,									//Enable AAR
		AAR_INC,								//Enable AAR with auto increase
		AAR_TOTAL
	};

	//Voice muting -----------------------------
	enum MuteState
	{
		MUTE_GET = -1,							//Do nothing (use to get mute state)
		MUTE_OFF,								//Unmute
		MUTE_ON,								//Mute
		MUTE_SOLO,								//Unmute voice and mute all others
		MUTE_TOGGLE								//Toggles current mute state
	};

	//PackSrc options --------------------------
	enum PackOpts
	{
		BRR_END,								//Set the end flag in the last block
		BRR_LOOP,								//Set the loop flag in all blocks
		BRR_LINEAR,								//Use linear compression for all blocks
		BRR_NOINIT = 4							//Don't create an initial block of silence
	};

	//UnpackSrc options ------------------------
	enum UnpackOpts
	{
		BRR_OLDSMP = 7							//Use old sample decompression routine
	};

	//Mixing flags -----------------------------
	static const u8		MFLG_MUTE	= 0x01;		//Voice is muted
	static const u8		MFLG_KOFF	= 0x04;		//Voice is in the process of keying off
	static const u8		MFLG_OFF	= 0x08;		//Voice is currently inactive

	//Envelope mode masks ----------------------
	static const u8		ENVM_ADJ	= 0x01;		//Adjustment: Const.(1/64 or 1/256) / Exp.(x255/256)
	static const u8		ENVM_DIR	= 0x02;		//Direction: Decrease / Increase
	static const u8		ENVM_DEST	= 0x04;		//Destination: Default(0 or 1) / Other(x/8 or .75)
	static const u8		ENVM_ADSR	= 0x08;		//Envelope mode: Gain/ADSR
	static const u8		ENVM_IDLE	= 0x80;		//Envelope is marked as idle, or not changing
	static const u8		ENVM_MODE	= 0x0F;		//Envelope mode is stored in lower four bits

	//Envelope modes ---------------------------
	enum EnvMode
	{
		ENV_DEC =		0,						//Linear decrease
		ENV_EXP =		1,						//Exponential decrease
		ENV_INC =		2,						//Linear increase
		ENV_BENT =		6,						//Bent line increase
		ENV_DIR =		7,						//Direct setting
		ENV_REL =		8,						//Release mode (key off)
		ENV_SUST =		9,						//Sustain mode
		ENV_ATTACK =	10,						//Attack mode
		ENV_DECAY =		13,						//Decay mode
	};

	//Flag register bits -----------------------
	static const u8		FLG_RES		= 0x80;		//Reset DSP
	static const u8		FLG_MUTE	= 0x40;		//Mute main output
	static const u8		FLG_ECEN	= 0x20;		//Disable echo region
	static const u8		FLG_NCK		= 0x1F;		//Mask for noise clock index

	//DSP debugging options --------------------

	// Halt DSP:
	//
	// This flag causes EmuDSP() to return silence.  Voices will not be updated and external writes
	// to DSP registers will be ignored.

	enum DSPDbgOpts
	{
		DSP_HALT = 7
	};


	//**********************************************************************************************
	//Structures

	//DSP registers ----------------------------
	struct Voice
	{
		s8	volL;								//Volume Left
		s8	volR;								//Volume Right
		u16	pitch;								//Pitch (rate/32000) (2.12)
		u8	srcn;								//Sound source being played back
		u8	adsr1;								//ADSR volume envelope (attack and decay rate)
		u8	adsr2;								//   "  " (sustain level and rate)
		u8	gain;								//Envelope gain (if not using ADSR)
		s8	envx;								//Current envelope height (.7)
		s8	outx;								//Current sample being output (-.7)
		s8	__r[6];
	};

	struct FIR
	{
		s8	__r[15];
		s8	c;									//Filter coefficient (-.7)
	};

	union Regs
	{
		Voice	voice[8];						//Voice registers

		struct									//Global registers
		{
			s8	__r00[12];
			s8	mvolL;							//Main Volume Left (-.7)
			s8	efb;							//Echo Feedback (-.7)
			s8	__r0E;
			s8	c0;								//FIR filter coefficent (-.7)

			s8	__r10[12];
			s8	mvolR;							//Main Volume Right (-.7)
			s8	__r1D;
			s8	__r1E;
			s8	c1;

			s8	__r20[12];
			s8	evolL;							//Echo Volume Left (-.7)
			u8	pmon;							//Pitch Modulation on/off for each voice
			s8	__r2E;
			s8	c2;

			s8	__r30[12];
			s8	evolR;							//Echo Volume Right (-.7)
			u8	non;							//Noise output on/off for each voice
			s8	__r3E;
			s8	c3;

			s8	__r40[12];
			u8	kon;							//Key On for each voice
			u8	eon;							//Echo on/off for each voice
			s8	__r4E;
			s8	c4;

			s8	__r50[12];
			u8	kof;							//Key Off for each voice
			u8	dir;							//Page containing source directory
			s8	__r5E;
			s8	c5;

			s8	__r60[12];
			u8	flg;							//DSP flags and noise frequency (see FLG_???)
			u8	esa;							//Starting page used to store echo samples
			s8	__r6E;
			s8	c6;

			s8	__r70[12];
			u8	endx;							//End block of sound source has been decoded
			u8	edl;							//Echo Delay in ms >> 4
			s8	__r7E;
			s8	c7;
		};

		FIR		fir[8];							//FIR filter

		u8		reg[128];
	};

	//Internal mixing data ---------------------
	struct MixData
	{
		//Visualization -- 08
		s32		vMaxL;							//Maximum absolute sample output
		s32		vMaxR;
		//Voice ---------- 04
		Voice*	pVoice;							//-> voice registers in DSP
		//Waveform ------- 06
		void*	bCur;							//-> current block
		u8		bHdr;							//Block Header for current block
		u8		mFlg;							//Mixing flags (see MFLG_???)
		//Envelope ------- 22
		u8		eMode;							//[3-0] Current mode (see ENV_???)
												//[6-4] ADSR mode to switch into from Gain
												//[7]   Envelope is idle
		u8		eRIdx;							//Index in rateTab (0-31)
		u32		eRate;							//Rate of envelope adjustment (16.16)
		u32		eCnt;							//Sample counter (16.16)
		u32		eVal;							//Current envelope height (.11)
		s32		eAdj;							//Amount to adjust envelope height
		u32		eDest;							//Envelope Destination (.11)
		//Samples -------- 56
		s16		sP1;							//Last sample decompressed (prev1)
		s16		sP2;							//Second to last sample (prev2)
		s16*	sIdx;							//-> current sample in sBuf
		s16		sBufP[8];						//Last 8 samples from prev block
		s16		sBuf[16];						//32 bytes for decompressed BRR block
		//Mixing --------- 32
		f32		mTgtL;							//Target volume (floating-point mixing only)
		f32		mTgtR;							// "  "
		s32		mChnL;							//Channel Volume (-24.7)
		s32		mChnR;							// "  "
		u32		mRate;							//Pitch Rate after modulation (16.16)
		u32		mDec;							//Pitch Decimal (.16) (used as delta for inter.)
		s32		mOut;							//Last sample output before chn vol (used for pmod)
		u32		__r;							//reserved
	};

	//Saved state ------------------------------
	//see SaveDSP for information about the values in the structure, and how they differ from the
	//internal values
	struct DSPState
	{
		Regs*		pReg;						//[0.0] -> DSP registers (128 bytes)
		MixData*	pVoice;						//[0.4] -> Internal mixing settings (1k)
		void*		pEcho;						//[0.8] -> echo buffer (bytes = sample rate * 1.92)
		u32			amp;						//[0.C] Amp level (AAR is stored in upper 4 bits)
	};


	//Maximum sample output --------------------
	struct MaxOutF
	{
		struct Voice
		{
			f32	left,right;			 			//Left/Right channel for each voice
		} voice[8];

		struct Main
		{
			f32	left,right;						//Left/Right channel for main output
		} main;
	};

	struct MaxOutI
	{
		struct Voice
		{
			s32	left,right;
		} voice[8];

		struct Main
		{
			s32	left,right;
		} main;
	};


	//**********************************************************************************************
	// DSP Debugging Routine
	//
	// A prototype for a function that gets called for debugging purposes.
	//
	// The paramaters passed in can be modified, and on return will be used to update the internal
	// registers.  Set bit-7 of 'reg' to prevent the DSP from handling the write.
	//
	// For calling a pointer to another debugging routine, use the following macro.
	//
	// In:
	//    reg -> Current register (LOBYTE = register index)
	//    val  = Value being written to register

	typedef void DSPDebug(u8* volatile reg, volatile u8 val);

#ifdef	__GNUC__
	#define	_CallDSPDebug(proc, reg, val) \
				asm(" \
					pushl	%1 \
					pushl	%0 \
					calll	%2 \
					popl	%0 \
					popl	%1 \
				" : "+m" (reg), "+m" (val) : "m" (proc));
#else
	#define	_CallDSPDebug(proc, reg, val) \
				_asm \
				{ \
					push	dword ptr [val]; \
					push	dword ptr [reg]; \
					call	dword ptr [proc]; \
					pop		dword ptr [reg]; \
					pop		dword ptr [val]; \
				}
#endif


#if !defined(SNESAPU_DLL) || defined(DONT_IMPORT_SNESAPU)

	////////////////////////////////////////////////////////////////////////////////////////////////
	// Private Declarations

	extern "C" Regs		dsp;					//DSP registers
	extern "C" MixData	mix[8];					//Mixing structures for each voice


	////////////////////////////////////////////////////////////////////////////////////////////////
	// External Functions
	//
	// NOTE:  Unless otherwise stated, no range checking is performed.  It is the caller's
	//        responsibilty to ensure paramaters passed are within a valid range.

extern "C" {

	//**********************************************************************************************
	// Get Processor Information
	//
	// Detects if processor has MMX, 3DNow!, or SSE capabilities
	//
	// Note:
	//    Values are or'd together.  If a procesor has 3DNow!, it will return 03h for MMX and 3DNow!.
	//
	// Out:
	//    see enum CPUCaps
	//
	// Destroys:
	//    nothing

	u32 GetProcInfo();


	//**********************************************************************************************
	// Initialize DSP
	//
	// Creates the lookup tables for interpolation, and sets the default mixing settings:
	//
	//    mixType = MIX_INT
	//    numChn  = 2
	//    bits    = 16
	//    rate    = 32000
	//    inter   = INT_GAUSS
	//    opts    = None
	//
	// Note:
	//    Callers should use InitAPU() instead
	//
	// Thread safe:
	//    No
	//
	// Destroys:
	//    EAX,ST0-7

	void InitDSP();


	//**********************************************************************************************
	// Reset DSP
	//
	// Resets the DSP registers, erases internal variables, and resets the volume
	//
	// Note:
	//    Callers should use ResetAPU() instead
	//
	// Thread safe:
	//    No
	//
	// Destroys:
	//    EAX

	void ResetDSP();


	//**********************************************************************************************
	// Set DSP Options
	//
	// Recalculates tables, changes the output sample rate, and sets up the mixing routine
	//
	// Notes:
	//    Range checking is performed on all parameters.  If a parameter does not match the required
	//     range of values, the default value will be assumed.
	//
	//    -1 (or Set::All) can be used for any paramater that should remain unchanged.
	//
	//    The OPT_ANALOG option only works when:
	//     mix  =  MIX_INT
	//     chn  =  2
	//     bits >= 16
	//     rate >= 32000
	//
	//    Four channel output forces MIX_FLOAT
	//
	//    Callers should use SetAPUOpt() instead
	//
	// Thread safe:
	//    No, except when only changing the interpolation type
	//
	// In:
	//    mix   = Mixing routine (see enum Mixing, default MIX_INT)
	//    chn   = Number of channels (1, 2, or 4, default 2)
	//    bits  = Sample size (mono 8 or 16; stereo 8, 16, 24, 32, or -32 [IEEE 754], default 16)
	//    rate  = Sample rate (8000-192000, default 32000)
	//    inter = Interpolation type (see enum Interpolation, default INT_GAUSS)
	//    opts  = see enum DSPOpts, Set::All() leaves the current options
	//
	// Destroys:
	//    EAX

	void SetDSPOpt(Mixing mix, u32 chn, u32 bits, u32 rate, DSPInter inter = INT_INVALID,
				   Set<DSPOpts> opts = ~Set<DSPOpts>());


	//**********************************************************************************************
	// Debug DSP
	//
	// Installs a callback that gets called each time a value is written to the DSP data register.
	//
	// Note:
	//    The build option SA_DEBUG must be enabled
	//
	// Thread safe:
	//    No, but it may be called from with in the debug callback
	//
	// In:
	//    pTrace-> Debugging callback (NULL can be passed to disable the callback, -1 leaves the
	//             current callback in place)
	//    opts   = DSP debugging flags (see enum DSPDbgOpts, Set.All() leaves the current flags)
	//
	// Out:
	//    Previously installed callback

	DSPDebug* SetDSPDbg(DSPDebug* pTrace, Set<DSPDbgOpts> opts = Set<DSPDbgOpts>());


	//**********************************************************************************************
	// Fix DSP After Loading SPC File
	//
	// Initializes the internal mixer variables
	//
	// Note:
	//    Callers should use FixAPULoad() instead
	//
	// Thread safe:
	//    No
	//
	// Destroys:
	//    EAX

	void FixDSPLoad();


	//**********************************************************************************************
	// Fix DSP After Seeking
	//
	// Puts all DSP voices in a key off state and erases echo region.  Call after using EmuDSPN.
	//
	// Thread safe:
	//    No
	//
	// In:
	//    reset = true, reset all voices
	//            false, only erase memory
	//
	// Destroys:
	//    EAX

	void FixDSPSeek(bool reset);


	//**********************************************************************************************
	// Save DSP State
	//
	// Notes:
	//    Pointers in the DSPState structure can be NULL if you don't want a copy of that data
	//
	//    The values in the Voice structures are modified as follows:
	//      dspv points to the registers pointed to by DSPState.pDSP
	//      bCur becomes an index into APU RAM (0-65535)
	//      eRate is adjusted for 32kHz
	//      eCnt is adjusted for 32kHz
	//
	//   Only the current samples being used for the echo delay are copied into pEchoBuf, not
	//    necessarily all 240ms.
	//
	//    Callers should use SaveAPU() instead
	//
	// Thread safe:
	//    No
	//
	// In:
	//    pState -> Saved state structure
	//
	// Destroys:
	//    EAX

	void SaveDSP(DSPState* state);


	//**********************************************************************************************
	// Restore DSP State
	//
	// Notes:
	//    Setting a pointer in DSPState to NULL will keep its corresponding internal data intact, except
	//     for the echo.  If pEchoBuf is NULL, the internal echo buffer will be cleared.
	//
	//    The members of the Voice structures must contain the same values as they would during
	//     emulation, except:
	//      bCur must be an index into APU RAM (0-65535), not a physical pointer
	//      eCnt must be the current sample count based on a 32kHz sample rate
	//      dspv, bHdr, and eRate do not need to be set because they will be reset to their correct vals
	//     If the inactive flag in mFlg is set, then the remaining Voice members do not need to be set
	//
	//    By setting eRate to -1, RestoreDSP will reset the following members:
	//      eRIdx, eRate, eCnt, eAdj, eDest
	//      eMode and eVal will remain unchanged
	//
	//    By setting 'mRate' to -1, RestoreDSP will reset the following members:
	//      mTgtL/R, mChnL/R, mRate, mDec, and mOrgP
	//      mFlg and mOut will remain unchanged
	//
	//    Only the amount of echo delay specified by DSP.EDL is copied from pEchoBuf
	//
	//    Callers should use RestoreAPU() instead
	//
	// Thread safe:
	//    No
	//
	// In:
	//    state -> Saved state structure
	//
	// Destroys:
	//    EAX

	void RestoreDSP(const DSPState& state);


	//**********************************************************************************************
	// Set Amplification Level
	//
	// Sets the amount to amplify the output during the final stage of mixing
	//
	// Note:
	//    Calling this function disables AAR for the current song
	//
	// Thread safe:
	//    No
	//
	// In:
	//    amp = Amplification level [-15.16] (1.0 = SNES, negative values act as 0)
	//
	// Destroys:
	//    EAX

	void SetDSPAmp(u32 amp);


	//**********************************************************************************************
	// Get Amplification Level
	//
	// Returns the current amplification level, which may have been changed due to AAR.
	//
	// Thread safe:
	//    Yes
	//
	// Out:
	//    Current amp level

	u32 GetDSPAmp();


	//**********************************************************************************************
	// Set Automatic Amplification Reduction
	//
	// Notes:
	//    -1 can be passed for any parameter that should remain unchanged
	//    Calling this function restarts AAR if it has been stopped due to amp reaching min or max or by
	//     calling SetDSPAmp()
	//
	// Thread safe:
	//    No
	//
	// In:
	//    type   = Type of AAR (see AAR_??? #defines)
	//    thresh = Threshold at which to decrease amplification
	//    min    = Minimum amplification level to decrease to
	//    max    = Maximum amplification level to increase to
	//
	// Destroys:
	//    EAX

	void SetDSPAAR(AARType type, u32 thresh, u32 min, u32 max);


	//**********************************************************************************************
	// Set Song Length
	//
	// Sets the length of the song and fade
	//
	// Note:
	//    To set a song with no length, pass -1 and 0 for the song and fade, respectively
	//
	// Thread safe:
	//    No
	//
	// In:
	//    song = Length of song (in 1/64000ths second)
	//    fade = Length of fade (in 1/64000ths second)
	//
	// Out:
	//    Total length

	u32 SetDSPLength(u32 song, u32 fade);


	//**********************************************************************************************
	// Set DSP Base Pitch
	//
	// Adjusts the pitch of the DSP
	//
	// Thread safe:
	//    No
	//
	// In:
	//    base = Base sample rate (32000 = Normal pitch, 32458 = Old SB cards, 32768 = Old ZSNES)
	//
	// Destroys:
	//    EAX

	void SetDSPPitch(u32 base);


	//**********************************************************************************************
	// Set Voice Stereo Separation
	//
	// Sets the amount to adjust the panning position of each voice
	//
	// Thread safe:
	//    No
	//
	// In:
	//   sep = Separation [1.16]
	//         1.0 - full separation (output is either left, center, or right)
	//         0.5 - normal separation (output is unchanged)
	//           0 - no separation (output is completely monaural)
	//
	// Destroys:
	//    EAX,ST(0-7)

	void SetDSPStereo(u32 sep);


	//**********************************************************************************************
	// Set Echo Feedback Crosstalk
	//
	// Sets the amount of crosstalk between the left and right channel during echo feedback
	//
	// Thread safe:
	//    No
	//
	// In:
	//   leak = Crosstalk amount [-1.15]
	//           1.0 - no crosstalk (SNES)
	//             0 - full crosstalk (mono/center)
	//          -1.0 - inverse crosstalk (L/R swapped)
	//
	// Destroys:
	//    EAX

	void SetDSPEFBCT(s32 leak);


	//**********************************************************************************************
	// Set Voice Mute
	//
	// Prevents a voice's output from being mixed in with the main output
	//
	// Thread safe:
	//    Yes
	//
	// In:
	//    voice = Voice to mute (0-7, only bits 2-0 are used)
	//    state = see enum MuteState
	//
	// Out:
	//    true, if voice is muted

	bool SetDSPVoiceMute(u32 voice, MuteState state);


	//**********************************************************************************************
	// DSP Data Port
	//
	// Writes a value to a specified DSP register and alters the DSP accordingly.  If the register
	// write affects the output generated by the DSP, this function returns true.
	//
	// Note:
	//    SetDSPReg() does not call the debugging callback
	//
	// Thread safe:
	//    No
	//
	// In:
	//    reg = DSP Address
	//    val = DSP Data
	//
	// Out:
	//    true, if the DSP state was affected

	bool SetDSPReg(u8 reg, u8 val);


	//**********************************************************************************************
	// Emulate DSP
	//
	// Emulates the DSP of the SNES
	//
	// Notes:
	//    If 'pBuf' is NULL, the routine MIX_NONE will be used
	//    Range checking is performed on 'len'
	//
	//    Callers should use EmuAPU() instead
	//
	// In:
	//    pBuf-> Buffer to store output
	//	  len  = Length of buffer (in samples, can be 0)
	//
	// Out:
	//    -> End of buffer
	//
	// Destroys:
	//    ST(0-7)

	void* EmuDSP(void* pBuf, s32 len);


	//**********************************************************************************************
	// Play Sound Source
	//
	// Plays a single sound source using the settings defined by SetDSPOpt.  All samples are output
	// in integer form.  The output is attenuated -6dB.
	//
	// Admittedly, this function is poorly coded in that it does two different things depending on
	// the value of the last parameter.
	//
	// Note:
	//    If the source data is within APU RAM, the source pointer will wrap around within APU RAM
	//
	// Thread safe:
	//    Yes, but can only be used to play one sound at a time
	//
	// In:
	//    pSrc-> Sound source to play (if upper 16 bits are 0, lower 16 bits will index APU RAM)
	//    loop = Block to loop on
	//    rate = Logical sample rate to play back at (actual playback rate may be less)
	//
	//    pSrc-> Buffer to store decompressed and resampled data
	//    loop = Number of samples to generate
	//    rate = 0
	//
	// Out:
	//    rate != 0, return value is sample rate sound will be played back at
	//    rate == 0, return value -> end of buffer

	void* PlaySrc(void* pSrc, u32 loop, u32 rate = 0);


	//**********************************************************************************************
	// Unpack Sound Source
	//
	// Decompresses a series of BRR blocks into 16-bit PCM samples
	//
	// Notes:
	//    Regardless of the value in 'num', unpacking will stop when the end block is reached.
	//    The previous samples are only needed if you're making multiple calls to UnpackWave.  If
	//     you're decompressing the entire sound in one call, you can disregard them.
	//    If the source data is within APU RAM, the source pointer will wrap around within APU RAM,
	//     though no more than 7281 BRR blocks will be processed.
	//
	// Thread safe:
	//    Yes, but can only be used to unpack one sound at a time
	//
	// In:
	//    pSmp -> Location to store decompressed samples
	//    pBlk -> Sound source to decompress (if upper 16 bits are 0, lower 16 bits will index APU RAM)
	//    num   = Number of blocks to decompress (0 to decompress entire sound)
	//    opts  = Options (only one for now)
	//            BRR_OLDSMP = Traditional method of decompression
	//    prev1-> First previous sample (can be NULL if not needed)
	//    prev2-> Second previous sample
	//
	// Out:
	//    Number of blocks decompressed

	u32 UnpackSrc(s16* pSmp, const void* pBlk, u32 num,
				  Set<UnpackOpts> opts = Set<UnpackOpts>(),
				  s32* prev1 = 0, s32* prev2 = 0);


	//**********************************************************************************************
	// Pack Sound Source
	//
	// Compresses a series of 16-bit PCM samples into BRR blocks.  Generally sounds are packed in
	// one call.  However, if you want to implement some sort of real-time compression stream you
	// can makemultiple calls to PackSrc.  See the options description below for more information on
	// continuing compression.
	//
	// THIS FUNCTION HAS NOT BEEN FINISHED AND DOES NOT PACK LOOPED SOUNDS
	//
	// Notes:
	//    The previous samples are only needed if you're making multiple calls to PackSrc.  If
	//     you're compressing the entire sound in one call, you can disregard them.
	//    No checking is performed on output pointer to make sure the data wraps around if it
	//     resides within APU RAM.
	//    The looping section must be at least 16 samples.
	//
	// Thread safe:
	//    Yes, but can only be used to pack one sound at a time
	//
	// In:
	//    pBlk -> Location to store bit-rate reduced blocks
	//    pSmp -> Samples to compress
	//    pLen -> Number of samples to compress
	//    pLoop-> Sample index of loop point.  Set this to NULL for one-shot sounds.  This parameter
	//            is ignored if 'prev1' is not NULL.
	//    opts  = Options.  The way the options behave is different depending on if 'prev1' is NULL.
	//
	//            If prev1 is NULL:
	//            BRR_END    = This option has no effect.  The last block's end flag will always be
	//                         set.
	//            BRR_LOOP   = By default, if pLoop is not NULL the last block's loop flag will
	//                         automatically be set.  This option causes the loop flag to be set in
	//                         every block regardless of pLoop.
	//            BRR_LINEAR = By default, the best compression filter will be determined for each
	//                         block.  This option forces linear compression (filter 0) to be used
	//                         for all blocks.
	//            BRR_NOINIT = By default, a block of silence is inserted at the beginning of the
	//                         sound source.  This option skips the silence and encodes samples into
	//                         the first block.  (The first block, whether it contains silence or
	//                         samples, will always use linear compression.  This is necessary in
	//                         order for the decompression filter to be properly initialized when
	//                         the source is played.)
	//
	//            If prev1 is not NULL:
	//            BRR_END    = Sets the end flag in the last block
	//            BRR_LOOP   = This option causes the loop flag to be set in every block.
	//            BRR_LINEAR = By default, the best compression filter will be determined for each
	//                         block.  This option forces linear compression (filter 0) to be used
	//                         for all blocks.
	//            BRR_NOINIT = By default, the first block is compressed with linear compression.
	//                         This option causes the first block to be compressed with whichever
	//                         filter is the best.
	//    prev1-> First previous sample.  NULL if the value of the previous samples is not needed.
	//            (Generally the previous samples are only used if making multiple calls to PackSrc.)
	//    prev2-> Second previous sample.  If this is NULL, 'prev1' is treated as NULL also.
	//
	// Out:
	//    -> End of the data stored in pBlk, NULL if an error occurred.
	//       Possible errors:
	//          Length is 0
	//          Loop point is greater than the number of samples
	//          Loop length is less than 16
	//    pLen -> Number of blocks in sound source
	//    pLoop-> Starting block of loop

	void* PackSrc(void* pBlk, const void* pSmp, u32* pLen, u32* pLoop,
				  Set<PackOpts> opts = Set<PackOpts>(),
				  s32* prev1 = 0, s32* prev2 = 0);


	//**********************************************************************************************
	// VMax to Decible
	//
	// Generates decible values for the peak output of each voice and the main output, the purpose
	// of which is to provide the user with some sort of PPM (Peak Program Meter) visualization.
	//
	// The values generated can be 32-bit floats or 16.16 fixed-point integers.  When floating-point
	// numbers are chosen, the return values are in actual dB starting with -96 (which represents
	// -infinity) on up to +0.  When integers are chosen, the return values are in fixed point
	// notation [16.16] and are positive starting with 0 (which represents -infinity) on up to +96
	// (or 600000h, representing +0db).
	//
	// After calling VMax2dB, the vMax values for both voice and main outputs are set to 0.
	//
	// Notes:
	//    The values for the main output will be higher than 0dB if clipping has occurred.
	//
	// In:
	//    p -> Structure to store 32-bit numbers
	//
	// Destroys:
	//    EAX

	void VMax2dBf(MaxOutF* p);
	void VMax2dBi(MaxOutI* p);

}	// extern "C"

#endif	//!SNESAPU_DLL

}	// namespace SNES
}	// namespace A2

