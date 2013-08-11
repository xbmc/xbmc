//**************************************************************************************************
// Type Redefinitions
#ifndef SPC_TYPES_H
#define SPC_TYPES_H

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

typedef void v0;

#ifdef	__cplusplus
#if defined __BORLANDC__
typedef bool b8;
#else
typedef unsigned char b8;
#endif
#else
typedef	char b8;
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#if defined _MSC_VER || defined __BORLANDC__
typedef unsigned __int64 u64;
#else
typedef	unsigned long long int u64;
#endif

typedef char s8;
typedef short s16;
typedef int s32;
#if defined _MSC_VER || defined __BORLANDC__
typedef __int64 s64;
#else
typedef	long long int s64;
#endif

typedef float f32;
typedef double f64;
typedef long double f80;

typedef struct DSPVoice
{
	s8	volL;									//Volume Left
	s8	volR;									//Volume Right
	u16	pitch;									//Pitch (rate/32000) (3.11)
	u8	srcn;									//Sound source being played back
	u8	adsr[2];								//Envelope rates for attack, decay, and sustain
	u8	gain;									//Envelope gain (if not using ADSR)
	s8	envx;									//Current envelope height (.7)
	s8	outx;									//Current sample being output (-.7)
	s8	__r[6];
} DSPVoice;

typedef struct SPCState
{
	void	*pRAM;								//[0.0] -> APU's RAM (64k)
	u32		_r1;								//[0.4] reserved
	u16		pc;									//[0.8] Registers
	u8		a,y,x;
	u8		psw;
	u16		sp;

	u32		t8kHz,t64kHz;						//[1.0] # of cycles left until timer increase
	u32		t64Cnt;								//[1.8] # of 64kHz ticks since emulation began
	u32		_r2;								//[1.C] reserved
	u8		upCnt[3];							//[2.0] Up counters for counter increase
	u8		portMod;							//[2.3] Flags for in ports that have been modified
	u8		outp[4];							//[2.4] Out ports
	u32		_r3[2];								//[2.8] reserved
} SPCState;

typedef struct DSPFIR
{
	s8	__r[15];
	s8	c;										//Filter coefficient
} DSPFIR;

typedef union DSPReg
{
	DSPVoice	voice[8];						//Voice registers

	struct										//Global registers
	{
		s8	__r00[12];
		s8	mvolL;								//Main Volume Left (-.7)
		s8	efb;								//Echo Feedback (-.7)
		s8	__r0E;
		s8	c0;									//FIR filter coefficent (-.7)

		s8	__r10[12];
		s8	mvolR;								//Main Volume Right (-.7)
		s8	__r1D;
		s8	__r1E;
		s8	c1;

		s8	__r20[12];
		s8	evolL;								//Echo Volume Left (-.7)
		u8	pmon;								//Pitch Modulation on/off for each voice
		s8	__r2E;
		s8	c2;

		s8	__r30[12];
		s8	evolR;								//Echo Volume Right (-.7)
		u8	non;								//Noise output on/off for each voice
		s8	__r3E;
		s8	c3;

		s8	__r40[12];
		u8	kon;								//Key On for each voice
		u8	eon;								//Echo on/off for each voice
		s8	__r4E;
		s8	c4;

		s8	__r50[12];
		u8	kof;								//Key Off for each voice (instantiates release mode)
		u8	dir;								//Page containing source directory (wave table offsets)
		s8	__r5E;
		s8	c5;

		s8	__r60[12];
		u8	flg;								//DSP flags and noise frequency
		u8	esa;								//Starting page used to store echo waveform
		s8	__r6E;
		s8	c6;

		s8	__r70[12];
		u8	endx;								//Waveform has ended
		u8	edl;								//Echo Delay in ms >> 4
		s8	__r7E;
		s8	c7;
	} u;

	DSPFIR	fir[8];								//FIR filter

	u8		reg[128];
} DSPReg;

typedef struct Voice
{
	//Voice -----------08
	DSPVoice	*pVoice;						//-> voice registers in DSP
	u32		_r;
	//Waveform --------06
	void	*bCur;								//-> current block
	u8		bHdr;								//Block Header for current block
	u8		mFlg;								//Mixing flags (see MixF)
	//Envelope --------22
	u8		eMode;								//[3-0] Current mode (see EnvM)
												//[6-4] ADSR mode to switch into from Gain
												//[7]   Envelope is idle
	u8		eRIdx;								//Index in RateTab (0-31)
	u32		eRate;								//Rate of envelope adjustment (16.16)
	u32		eCnt;								//Sample counter (16.16)
	u32		eVal;								//Current envelope value
	s32		eAdj;								//Amount to adjust envelope height
	u32		eDest;								//Envelope Destination
	//Visualization ---08
	s32		vMaxL;								//Maximum absolute sample output
	s32		vMaxR;
	//Samples ---------52
	s32		sP1;								//Last sample decompressed (prev1)
	s32		sP2;								//Second to last sample (prev2)
	s16		*sIdx;								//-> current sample in sBuf
	s16		sBufP[4];							//Last 4 samples from previous block (needed for inter.)
	s16		sBuf[16];							//32 bytes for decompressed sample blocks
	//Mixing ----------32
	float mTgtL;								//Target volume (floating-point routine only)
	float		mTgtR;								// "  "
	s32		mChnL;								//Channel Volume (-24.7)
	s32		mChnR;								// "  "
	u32		mRate;								//Pitch Rate after modulation (16.16)
	u32		mDec;								//Pitch Decimal (.16) (used as delta for interpolation)
	u32		mOrgP;								//Original pitch rate converted from the DSP (16.16)
	s32		mOut;								//Last sample output before chn vol (used for pitch mod)
} Voice;

typedef struct DSPState
{
	DSPReg	*pDSP;								//[0.0] -> DSP registers (128 bytes)
	Voice	*pVoice;							//[0.4] -> Internal mixing settings (1k)
	void	*pEcho;								//[0.8] -> echo buffer (bytes = sample rate * 1.92)
	u32		_r1;								//[0.C] reserved

	u32		vMMaxL,vMMaxR;						//[1.0] Maximum output so far
	u32		mAmp;								//[1.8] Amplification
	u8		vActive;							//[1.C] Flags for each active voice
	u8		_r2[3];								//[1.D] reserved
} DSPState;

typedef enum
{
  SPC_EMULATOR_UNKNOWN = 0,
  SPC_EMULATOR_ZSNES,
  SPC_EMULATOR_SNES9X
} SPC_EmulatorType;

typedef struct SPC_ID666
{
  char songname[33];
  char gametitle[33];
  char dumper[17];
  char comments[33];
  char author[33];
  int playtime;
  int fadetime;
  SPC_EmulatorType emulator;
} SPC_ID666;

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
		OPT_FILTER								//Pass each voice through an anti-aliasing filter
	};

	//Mixing routines (see DSP.Asm for details of each routine)
	enum Mixing
	{
		MIX_INVALID =	-1,
		MIX_NONE =		0,						//No mixing
		MIX_INT =		1,						//Use integer math
		MIX_FLOAT =		3						//Use floating-point math
	};

#endif

