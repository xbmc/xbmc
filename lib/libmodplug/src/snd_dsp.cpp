/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

#include "stdafx.h"
#include "sndfile.h"

#ifdef MODPLUG_FASTSOUNDLIB
#define MODPLUG_NO_REVERB
#endif


// Delayed Surround Filters
#ifndef MODPLUG_FASTSOUNDLIB
#define nDolbyHiFltAttn		6
#define nDolbyHiFltMask		3
#define DOLBYATTNROUNDUP	31
#else
#define nDolbyHiFltAttn		3
#define nDolbyHiFltMask		3
#define DOLBYATTNROUNDUP	3
#endif

// Bass Expansion
#define XBASS_DELAY			14	// 2.5 ms

// Buffer Sizes
#define XBASSBUFFERSIZE		64		// 2 ms at 50KHz
#define FILTERBUFFERSIZE	64		// 1.25 ms
#define SURROUNDBUFFERSIZE	((MAX_SAMPLE_RATE * 50) / 1000)
#define REVERBBUFFERSIZE	((MAX_SAMPLE_RATE * 200) / 1000)
#define REVERBBUFFERSIZE2	((REVERBBUFFERSIZE*13) / 17)
#define REVERBBUFFERSIZE3	((REVERBBUFFERSIZE*7) / 13)
#define REVERBBUFFERSIZE4	((REVERBBUFFERSIZE*7) / 19)


// DSP Effects: PUBLIC members
UINT CSoundFile::m_nXBassDepth = 6;
UINT CSoundFile::m_nXBassRange = XBASS_DELAY;
UINT CSoundFile::m_nReverbDepth = 1;
UINT CSoundFile::m_nReverbDelay = 100;
UINT CSoundFile::m_nProLogicDepth = 12;
UINT CSoundFile::m_nProLogicDelay = 20;

////////////////////////////////////////////////////////////////////
// DSP Effects internal state

// Bass Expansion: low-pass filter
static LONG nXBassSum = 0;
static LONG nXBassBufferPos = 0;
static LONG nXBassDlyPos = 0;
static LONG nXBassMask = 0;

// Noise Reduction: simple low-pass filter
static LONG nLeftNR = 0;
static LONG nRightNR = 0;

// Surround Encoding: 1 delay line + low-pass filter + high-pass filter
static LONG nSurroundSize = 0;
static LONG nSurroundPos = 0;
static LONG nDolbyDepth = 0;
static LONG nDolbyLoDlyPos = 0;
static LONG nDolbyLoFltPos = 0;
static LONG nDolbyLoFltSum = 0;
static LONG nDolbyHiFltPos = 0;
static LONG nDolbyHiFltSum = 0;

// Reverb: 4 delay lines + high-pass filter + low-pass filter
#ifndef MODPLUG_NO_REVERB
static LONG nReverbSize = 0;
static LONG nReverbBufferPos = 0;
static LONG nReverbSize2 = 0;
static LONG nReverbBufferPos2 = 0;
static LONG nReverbSize3 = 0;
static LONG nReverbBufferPos3 = 0;
static LONG nReverbSize4 = 0;
static LONG nReverbBufferPos4 = 0;
static LONG nReverbLoFltSum = 0;
static LONG nReverbLoFltPos = 0;
static LONG nReverbLoDlyPos = 0;
static LONG nFilterAttn = 0;
static LONG gRvbLowPass[8];
static LONG gRvbLPPos = 0;
static LONG gRvbLPSum = 0;
static LONG ReverbLoFilterBuffer[XBASSBUFFERSIZE];
static LONG ReverbLoFilterDelay[XBASSBUFFERSIZE];
static LONG ReverbBuffer[REVERBBUFFERSIZE];
static LONG ReverbBuffer2[REVERBBUFFERSIZE2];
static LONG ReverbBuffer3[REVERBBUFFERSIZE3];
static LONG ReverbBuffer4[REVERBBUFFERSIZE4];
#endif
static LONG XBassBuffer[XBASSBUFFERSIZE];
static LONG XBassDelay[XBASSBUFFERSIZE];
static LONG DolbyLoFilterBuffer[XBASSBUFFERSIZE];
static LONG DolbyLoFilterDelay[XBASSBUFFERSIZE];
static LONG DolbyHiFilterBuffer[FILTERBUFFERSIZE];
static LONG SurroundBuffer[SURROUNDBUFFERSIZE];

// Access the main temporary mix buffer directly: avoids an extra pointer
extern int MixSoundBuffer[MIXBUFFERSIZE*2];
//cextern int MixReverbBuffer[MIXBUFFERSIZE*2];
extern int MixReverbBuffer[MIXBUFFERSIZE*2];

static UINT GetMaskFromSize(UINT len)
//-----------------------------------
{
	UINT n = 2;
	while (n <= len) n <<= 1;
	return ((n >> 1) - 1);
}


void CSoundFile::InitializeDSP(BOOL bReset)
//-----------------------------------------
{
	if (!m_nReverbDelay) m_nReverbDelay = 100;
	if (!m_nXBassRange) m_nXBassRange = XBASS_DELAY;
	if (!m_nProLogicDelay) m_nProLogicDelay = 20;
	if (m_nXBassDepth > 8) m_nXBassDepth = 8;
	if (m_nXBassDepth < 2) m_nXBassDepth = 2;
	if (bReset)
	{
		// Noise Reduction
		nLeftNR = nRightNR = 0;
	}
	// Pro-Logic Surround
	nSurroundPos = nSurroundSize = 0;
	nDolbyLoFltPos = nDolbyLoFltSum = nDolbyLoDlyPos = 0;
	nDolbyHiFltPos = nDolbyHiFltSum = 0;
	if (gdwSoundSetup & SNDMIX_SURROUND)
	{
		memset(DolbyLoFilterBuffer, 0, sizeof(DolbyLoFilterBuffer));
		memset(DolbyHiFilterBuffer, 0, sizeof(DolbyHiFilterBuffer));
		memset(DolbyLoFilterDelay, 0, sizeof(DolbyLoFilterDelay));
		memset(SurroundBuffer, 0, sizeof(SurroundBuffer));
		nSurroundSize = (gdwMixingFreq * m_nProLogicDelay) / 1000;
		if (nSurroundSize > SURROUNDBUFFERSIZE) nSurroundSize = SURROUNDBUFFERSIZE;
		if (m_nProLogicDepth < 8) nDolbyDepth = (32 >> m_nProLogicDepth) + 32;
		else nDolbyDepth = (m_nProLogicDepth < 16) ? (8 + (m_nProLogicDepth - 8) * 7) : 64;
		nDolbyDepth >>= 2;
	}
	// Reverb Setup
#ifndef MODPLUG_NO_REVERB
	if (gdwSoundSetup & SNDMIX_REVERB)
	{
		UINT nrs = (gdwMixingFreq * m_nReverbDelay) / 1000;
		UINT nfa = m_nReverbDepth+1;
		if (nrs > REVERBBUFFERSIZE) nrs = REVERBBUFFERSIZE;
		if ((bReset) || (nrs != (UINT)nReverbSize) || (nfa != (UINT)nFilterAttn))
		{
			nFilterAttn = nfa;
			nReverbSize = nrs;
			nReverbBufferPos = nReverbBufferPos2 = nReverbBufferPos3 = nReverbBufferPos4 = 0;
			nReverbLoFltSum = nReverbLoFltPos = nReverbLoDlyPos = 0;
			gRvbLPSum = gRvbLPPos = 0;
			nReverbSize2 = (nReverbSize * 13) / 17;
			if (nReverbSize2 > REVERBBUFFERSIZE2) nReverbSize2 = REVERBBUFFERSIZE2;
			nReverbSize3 = (nReverbSize * 7) / 13;
			if (nReverbSize3 > REVERBBUFFERSIZE3) nReverbSize3 = REVERBBUFFERSIZE3;
			nReverbSize4 = (nReverbSize * 7) / 19;
			if (nReverbSize4 > REVERBBUFFERSIZE4) nReverbSize4 = REVERBBUFFERSIZE4;
			memset(ReverbLoFilterBuffer, 0, sizeof(ReverbLoFilterBuffer));
			memset(ReverbLoFilterDelay, 0, sizeof(ReverbLoFilterDelay));
			memset(ReverbBuffer, 0, sizeof(ReverbBuffer));
			memset(ReverbBuffer2, 0, sizeof(ReverbBuffer2));
			memset(ReverbBuffer3, 0, sizeof(ReverbBuffer3));
			memset(ReverbBuffer4, 0, sizeof(ReverbBuffer4));
			memset(gRvbLowPass, 0, sizeof(gRvbLowPass));
		}
	} else nReverbSize = 0;
#endif
	BOOL bResetBass = FALSE;
	// Bass Expansion Reset
	if (gdwSoundSetup & SNDMIX_MEGABASS)
	{
		UINT nXBassSamples = (gdwMixingFreq * m_nXBassRange) / 10000;
		if (nXBassSamples > XBASSBUFFERSIZE) nXBassSamples = XBASSBUFFERSIZE;
		UINT mask = GetMaskFromSize(nXBassSamples);
		if ((bReset) || (mask != (UINT)nXBassMask))
		{
			nXBassMask = mask;
			bResetBass = TRUE;
		}
	} else
	{
		nXBassMask = 0;
		bResetBass = TRUE;
	}
	if (bResetBass)
	{
		nXBassSum = nXBassBufferPos = nXBassDlyPos = 0;
		memset(XBassBuffer, 0, sizeof(XBassBuffer));
		memset(XBassDelay, 0, sizeof(XBassDelay));
	}
}


void CSoundFile::ProcessStereoDSP(int count)
//------------------------------------------
{
#ifndef MODPLUG_NO_REVERB
	// Reverb
	if (gdwSoundSetup & SNDMIX_REVERB)
	{
		int *pr = MixSoundBuffer, *pin = MixReverbBuffer, rvbcount = count;
		do
		{
			int echo = ReverbBuffer[nReverbBufferPos] + ReverbBuffer2[nReverbBufferPos2]
					+ ReverbBuffer3[nReverbBufferPos3] + ReverbBuffer4[nReverbBufferPos4];	// echo = reverb signal
			// Delay line and remove Low Frequencies			// v = original signal
			int echodly = ReverbLoFilterDelay[nReverbLoDlyPos];	// echodly = delayed signal
			ReverbLoFilterDelay[nReverbLoDlyPos] = echo >> 1;
			nReverbLoDlyPos++;
			nReverbLoDlyPos &= 0x1F;
			int n = nReverbLoFltPos;
			nReverbLoFltSum -= ReverbLoFilterBuffer[n];
			int tmp = echo / 128;
			ReverbLoFilterBuffer[n] = tmp;
			nReverbLoFltSum += tmp;
			echodly -= nReverbLoFltSum;
			nReverbLoFltPos = (n + 1) & 0x3F;
			// Reverb
			int v = (pin[0]+pin[1]) >> nFilterAttn;
			pr[0] += pin[0] + echodly;
			pr[1] += pin[1] + echodly;
			v += echodly >> 2;
			ReverbBuffer3[nReverbBufferPos3] = v;
			ReverbBuffer4[nReverbBufferPos4] = v;
			v += echodly >> 4;
			v >>= 1;
			gRvbLPSum -= gRvbLowPass[gRvbLPPos];
			gRvbLPSum += v;
			gRvbLowPass[gRvbLPPos] = v;
			gRvbLPPos++;
			gRvbLPPos &= 7;
			int vlp = gRvbLPSum >> 2;
			ReverbBuffer[nReverbBufferPos] = vlp;
			ReverbBuffer2[nReverbBufferPos2] = vlp;
			if (++nReverbBufferPos >= nReverbSize) nReverbBufferPos = 0;
			if (++nReverbBufferPos2 >= nReverbSize2) nReverbBufferPos2 = 0;
			if (++nReverbBufferPos3 >= nReverbSize3) nReverbBufferPos3 = 0;
			if (++nReverbBufferPos4 >= nReverbSize4) nReverbBufferPos4 = 0;
			pr += 2;
			pin += 2;
		} while (--rvbcount);
	}
#endif
	// Dolby Pro-Logic Surround
	if (gdwSoundSetup & SNDMIX_SURROUND)
	{
		int *pr = MixSoundBuffer, n = nDolbyLoFltPos;
		for (int r=count; r; r--)
		{
			int v = (pr[0]+pr[1]+DOLBYATTNROUNDUP) >> (nDolbyHiFltAttn+1);
#ifndef MODPLUG_FASTSOUNDLIB
			v *= (int)nDolbyDepth;
#endif
			// Low-Pass Filter
			nDolbyHiFltSum -= DolbyHiFilterBuffer[nDolbyHiFltPos];
			DolbyHiFilterBuffer[nDolbyHiFltPos] = v;
			nDolbyHiFltSum += v;
			v = nDolbyHiFltSum;
			nDolbyHiFltPos++;
			nDolbyHiFltPos &= nDolbyHiFltMask;
			// Surround
			int secho = SurroundBuffer[nSurroundPos];
			SurroundBuffer[nSurroundPos] = v;
			// Delay line and remove low frequencies
			v = DolbyLoFilterDelay[nDolbyLoDlyPos];		// v = delayed signal
			DolbyLoFilterDelay[nDolbyLoDlyPos] = secho;	// secho = signal
			nDolbyLoDlyPos++;
			nDolbyLoDlyPos &= 0x1F;
			nDolbyLoFltSum -= DolbyLoFilterBuffer[n];
			int tmp = secho / 64;
			DolbyLoFilterBuffer[n] = tmp;
			nDolbyLoFltSum += tmp;
			v -= nDolbyLoFltSum;
			n++;
			n &= 0x3F;
			// Add echo
			pr[0] += v;
			pr[1] -= v;
			if (++nSurroundPos >= nSurroundSize) nSurroundPos = 0;
			pr += 2;
		}
		nDolbyLoFltPos = n;
	}
	// Bass Expansion
	if (gdwSoundSetup & SNDMIX_MEGABASS)
	{
		int *px = MixSoundBuffer;
		int xba = m_nXBassDepth+1, xbamask = (1 << xba) - 1;
		int n = nXBassBufferPos;
		for (int x=count; x; x--)
		{
			nXBassSum -= XBassBuffer[n];
			int tmp0 = px[0] + px[1];
			int tmp = (tmp0 + ((tmp0 >> 31) & xbamask)) >> xba;
			XBassBuffer[n] = tmp;
			nXBassSum += tmp;
			int v = XBassDelay[nXBassDlyPos];
			XBassDelay[nXBassDlyPos] = px[0];
			px[0] = v + nXBassSum;
			v = XBassDelay[nXBassDlyPos+1];
			XBassDelay[nXBassDlyPos+1] = px[1];
			px[1] = v + nXBassSum;
			nXBassDlyPos = (nXBassDlyPos + 2) & nXBassMask;
			px += 2;
			n++;
			n &= nXBassMask;
		}
		nXBassBufferPos = n;
	}
	// Noise Reduction
	if (gdwSoundSetup & SNDMIX_NOISEREDUCTION)
	{
		int n1 = nLeftNR, n2 = nRightNR;
		int *pnr = MixSoundBuffer;
		for (int nr=count; nr; nr--)
		{
			int vnr = pnr[0] >> 1;
			pnr[0] = vnr + n1;
			n1 = vnr;
			vnr = pnr[1] >> 1;
			pnr[1] = vnr + n2;
			n2 = vnr;
			pnr += 2;
		}
		nLeftNR = n1;
		nRightNR = n2;
	}
}


void CSoundFile::ProcessMonoDSP(int count)
//----------------------------------------
{
#ifndef MODPLUG_NO_REVERB
	// Reverb
	if (gdwSoundSetup & SNDMIX_REVERB)
	{
		int *pr = MixSoundBuffer, rvbcount = count, *pin = MixReverbBuffer;
		do
		{
			int echo = ReverbBuffer[nReverbBufferPos] + ReverbBuffer2[nReverbBufferPos2]
					+ ReverbBuffer3[nReverbBufferPos3] + ReverbBuffer4[nReverbBufferPos4];	// echo = reverb signal
			// Delay line and remove Low Frequencies			// v = original signal
			int echodly = ReverbLoFilterDelay[nReverbLoDlyPos];	// echodly = delayed signal
			ReverbLoFilterDelay[nReverbLoDlyPos] = echo >> 1;
			nReverbLoDlyPos++;
			nReverbLoDlyPos &= 0x1F;
			int n = nReverbLoFltPos;
			nReverbLoFltSum -= ReverbLoFilterBuffer[n];
			int tmp = echo / 128;
			ReverbLoFilterBuffer[n] = tmp;
			nReverbLoFltSum += tmp;
			echodly -= nReverbLoFltSum;
			nReverbLoFltPos = (n + 1) & 0x3F;
			// Reverb
			int v = pin[0] >> (nFilterAttn-1);
			*pr++ += pin[0] + echodly;
			pin++;
			v += echodly >> 2;
			ReverbBuffer3[nReverbBufferPos3] = v;
			ReverbBuffer4[nReverbBufferPos4] = v;
			v += echodly >> 4;
			v >>= 1;
			gRvbLPSum -= gRvbLowPass[gRvbLPPos];
			gRvbLPSum += v;
			gRvbLowPass[gRvbLPPos] = v;
			gRvbLPPos++;
			gRvbLPPos &= 7;
			int vlp = gRvbLPSum >> 2;
			ReverbBuffer[nReverbBufferPos] = vlp;
			ReverbBuffer2[nReverbBufferPos2] = vlp;
			if (++nReverbBufferPos >= nReverbSize) nReverbBufferPos = 0;
			if (++nReverbBufferPos2 >= nReverbSize2) nReverbBufferPos2 = 0;
			if (++nReverbBufferPos3 >= nReverbSize3) nReverbBufferPos3 = 0;
			if (++nReverbBufferPos4 >= nReverbSize4) nReverbBufferPos4 = 0;
		} while (--rvbcount);
	}
#endif
	// Bass Expansion
	if (gdwSoundSetup & SNDMIX_MEGABASS)
	{
		int *px = MixSoundBuffer;
		int xba = m_nXBassDepth, xbamask = (1 << xba)-1;
		int n = nXBassBufferPos;
		for (int x=count; x; x--)
		{
			nXBassSum -= XBassBuffer[n];
			int tmp0 = *px;
			int tmp = (tmp0 + ((tmp0 >> 31) & xbamask)) >> xba;
			XBassBuffer[n] = tmp;
			nXBassSum += tmp;
			int v = XBassDelay[nXBassDlyPos];
			XBassDelay[nXBassDlyPos] = *px;
			*px++ = v + nXBassSum;
			nXBassDlyPos = (nXBassDlyPos + 2) & nXBassMask;
			n++;
			n &= nXBassMask;
		}
		nXBassBufferPos = n;
	}
	// Noise Reduction
	if (gdwSoundSetup & SNDMIX_NOISEREDUCTION)
	{
		int n = nLeftNR;
		int *pnr = MixSoundBuffer;
		for (int nr=count; nr; pnr++, nr--)
		{
			int vnr = *pnr >> 1;
			*pnr = vnr + n;
			n = vnr;
		}
		nLeftNR = n;
	}
}


/////////////////////////////////////////////////////////////////
// Clean DSP Effects interface

// [Reverb level 0(quiet)-100(loud)], [delay in ms, usually 40-200ms]
BOOL CSoundFile::SetReverbParameters(UINT nDepth, UINT nDelay)
//------------------------------------------------------------
{
	if (nDepth > 100) nDepth = 100;
	UINT gain = nDepth / 20;
	if (gain > 4) gain = 4;
	m_nReverbDepth = 4 - gain;
	if (nDelay < 40) nDelay = 40;
	if (nDelay > 250) nDelay = 250;
	m_nReverbDelay = nDelay;
	return TRUE;
}


// [XBass level 0(quiet)-100(loud)], [cutoff in Hz 20-100]
BOOL CSoundFile::SetXBassParameters(UINT nDepth, UINT nRange)
//-----------------------------------------------------------
{
	if (nDepth > 100) nDepth = 100;
	UINT gain = nDepth / 20;
	if (gain > 4) gain = 4;
	m_nXBassDepth = 8 - gain;	// filter attenuation 1/256 .. 1/16
	UINT range = nRange / 5;
	if (range > 5) range -= 5; else range = 0;
	if (nRange > 16) nRange = 16;
	m_nXBassRange = 21 - range;	// filter average on 0.5-1.6ms
	return TRUE;
}


// [Surround level 0(quiet)-100(heavy)] [delay in ms, usually 5-50ms]
BOOL CSoundFile::SetSurroundParameters(UINT nDepth, UINT nDelay)
//--------------------------------------------------------------
{
	UINT gain = (nDepth * 16) / 100;
	if (gain > 16) gain = 16;
	if (gain < 1) gain = 1;
	m_nProLogicDepth = gain;
	if (nDelay < 4) nDelay = 4;
	if (nDelay > 50) nDelay = 50;
	m_nProLogicDelay = nDelay;
	return TRUE;
}

BOOL CSoundFile::SetWaveConfigEx(BOOL bSurround,BOOL bNoOverSampling,BOOL bReverb,BOOL hqido,BOOL bMegaBass,BOOL bNR,BOOL bEQ)
//----------------------------------------------------------------------------------------------------------------------------
{
	DWORD d = gdwSoundSetup & ~(SNDMIX_SURROUND | SNDMIX_NORESAMPLING | SNDMIX_REVERB | SNDMIX_HQRESAMPLER | SNDMIX_MEGABASS | SNDMIX_NOISEREDUCTION | SNDMIX_EQ);
	if (bSurround) d |= SNDMIX_SURROUND;
	if (bNoOverSampling) d |= SNDMIX_NORESAMPLING;
	if (bReverb) d |= SNDMIX_REVERB;
	if (hqido) d |= SNDMIX_HQRESAMPLER;
	if (bMegaBass) d |= SNDMIX_MEGABASS;
	if (bNR) d |= SNDMIX_NOISEREDUCTION;
	if (bEQ) d |= SNDMIX_EQ;
	gdwSoundSetup = d;
	InitPlayer(FALSE);
	return TRUE;
}
