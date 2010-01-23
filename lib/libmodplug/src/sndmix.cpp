/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

#include "libmodplug/stdafx.h"
#include "libmodplug/sndfile.h"
#include "tables.h"

#ifdef MODPLUG_TRACKER
#define ENABLE_STEREOVU
#endif

// Volume ramp length, in 1/10 ms
#define VOLUMERAMPLEN	146	// 1.46ms = 64 samples at 44.1kHz

// VU-Meter
#define VUMETER_DECAY		4

// SNDMIX: These are global flags for playback control
UINT CSoundFile::m_nStereoSeparation = 128;
LONG CSoundFile::m_nStreamVolume = 0x8000;
UINT CSoundFile::m_nMaxMixChannels = 32;
// Mixing Configuration (SetWaveConfig)
DWORD CSoundFile::gdwSysInfo = 0;
DWORD CSoundFile::gnChannels = 1;
DWORD CSoundFile::gdwSoundSetup = 0;
DWORD CSoundFile::gdwMixingFreq = 44100;
DWORD CSoundFile::gnBitsPerSample = 16;
// Mixing data initialized in
UINT CSoundFile::gnAGC = AGC_UNITY;
UINT CSoundFile::gnVolumeRampSamples = 64;
UINT CSoundFile::gnVUMeter = 0;
UINT CSoundFile::gnCPUUsage = 0;
LPSNDMIXHOOKPROC CSoundFile::gpSndMixHook = NULL;
PMIXPLUGINCREATEPROC CSoundFile::gpMixPluginCreateProc = NULL;
LONG gnDryROfsVol = 0;
LONG gnDryLOfsVol = 0;
LONG gnRvbROfsVol = 0;
LONG gnRvbLOfsVol = 0;
int gbInitPlugins = 0;

typedef DWORD (MPPASMCALL * LPCONVERTPROC)(LPVOID, int *, DWORD, LPLONG, LPLONG);

extern DWORD MPPASMCALL X86_Convert32To8(LPVOID lpBuffer, int *, DWORD nSamples, LPLONG, LPLONG);
extern DWORD MPPASMCALL X86_Convert32To16(LPVOID lpBuffer, int *, DWORD nSamples, LPLONG, LPLONG);
extern DWORD MPPASMCALL X86_Convert32To24(LPVOID lpBuffer, int *, DWORD nSamples, LPLONG, LPLONG);
extern DWORD MPPASMCALL X86_Convert32To32(LPVOID lpBuffer, int *, DWORD nSamples, LPLONG, LPLONG);
extern UINT MPPASMCALL X86_AGC(int *pBuffer, UINT nSamples, UINT nAGC);
extern VOID MPPASMCALL X86_Dither(int *pBuffer, UINT nSamples, UINT nBits);
extern VOID MPPASMCALL X86_InterleaveFrontRear(int *pFrontBuf, int *pRearBuf, DWORD nSamples);
extern VOID MPPASMCALL X86_StereoFill(int *pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs);
extern VOID MPPASMCALL X86_MonoFromStereo(int *pMixBuf, UINT nSamples);

extern int MixSoundBuffer[MIXBUFFERSIZE*4];
extern int MixRearBuffer[MIXBUFFERSIZE*2];
UINT gnReverbSend;


// Log tables for pre-amp
// We don't want the tracker to get too loud
const UINT PreAmpTable[16] =
{
	0x60, 0x60, 0x60, 0x70,	// 0-7
	0x80, 0x88, 0x90, 0x98,	// 8-15
	0xA0, 0xA4, 0xA8, 0xB0,	// 16-23
	0xB4, 0xB8, 0xBC, 0xC0,	// 24-31
};

const UINT PreAmpAGCTable[16] =
{
	0x60, 0x60, 0x60, 0x60,
	0x68, 0x70, 0x78, 0x80,
	0x84, 0x88, 0x8C, 0x90,
	0x94, 0x98, 0x9C, 0xA0,
};


// Return (a*b)/c - no divide error
int _muldiv(long a, long b, long c)
{
#ifdef MSC_VER
	int sign, result;
	_asm {
	mov eax, a
	mov ebx, b
	or eax, eax
	mov edx, eax
	jge aneg
	neg eax
aneg:
	xor edx, ebx
	or ebx, ebx
	mov ecx, c
	jge bneg
	neg ebx
bneg:
	xor edx, ecx
	or ecx, ecx
	mov sign, edx
	jge cneg
	neg ecx
cneg:
	mul ebx
	cmp edx, ecx
	jae diverr
	div ecx
	jmp ok
diverr:
	mov eax, 0x7fffffff
ok:
	mov edx, sign
	or edx, edx
	jge rneg
	neg eax
rneg:
	mov result, eax
	}
	return result;
#else
	return ((unsigned long long) a * (unsigned long long) b ) / c;
#endif
}


// Return (a*b+c/2)/c - no divide error
int _muldivr(long a, long b, long c)
{
#ifdef MSC_VER
	int sign, result;
	_asm {
	mov eax, a
	mov ebx, b
	or eax, eax
	mov edx, eax
	jge aneg
	neg eax
aneg:
	xor edx, ebx
	or ebx, ebx
	mov ecx, c
	jge bneg
	neg ebx
bneg:
	xor edx, ecx
	or ecx, ecx
	mov sign, edx
	jge cneg
	neg ecx
cneg:
	mul ebx
	mov ebx, ecx
	shr ebx, 1
	add eax, ebx
	adc edx, 0
	cmp edx, ecx
	jae diverr
	div ecx
	jmp ok
diverr:
	mov eax, 0x7fffffff
ok:
	mov edx, sign
	or edx, edx
	jge rneg
	neg eax
rneg:
	mov result, eax
	}
	return result;
#else
	return ((unsigned long long) a * (unsigned long long) b + (c >> 1)) / c;
#endif
}


BOOL CSoundFile::InitPlayer(BOOL bReset)
//--------------------------------------
{
	if (m_nMaxMixChannels > MAX_CHANNELS) m_nMaxMixChannels = MAX_CHANNELS;
	if (gdwMixingFreq < 4000) gdwMixingFreq = 4000;
	if (gdwMixingFreq > MAX_SAMPLE_RATE) gdwMixingFreq = MAX_SAMPLE_RATE;
	gnVolumeRampSamples = (gdwMixingFreq * VOLUMERAMPLEN) / 100000;
	if (gnVolumeRampSamples < 8) gnVolumeRampSamples = 8;
	gnDryROfsVol = gnDryLOfsVol = 0;
	gnRvbROfsVol = gnRvbLOfsVol = 0;
	if (bReset)
	{
		gnVUMeter = 0;
		gnCPUUsage = 0;
	}
	gbInitPlugins = (bReset) ? 3 : 1;
	InitializeDSP(bReset);
	return TRUE;
}


BOOL CSoundFile::FadeSong(UINT msec)
//----------------------------------
{
	LONG nsamples = _muldiv(msec, gdwMixingFreq, 1000);
	if (nsamples <= 0) return FALSE;
	if (nsamples > 0x100000) nsamples = 0x100000;
	m_nBufferCount = nsamples;
	LONG nRampLength = m_nBufferCount;
	// Ramp everything down
	for (UINT noff=0; noff < m_nMixChannels; noff++)
	{
		MODCHANNEL *pramp = &Chn[ChnMix[noff]];
		if (!pramp) continue;
		pramp->nNewLeftVol = pramp->nNewRightVol = 0;
		pramp->nRightRamp = (-pramp->nRightVol << VOLUMERAMPPRECISION) / nRampLength;
		pramp->nLeftRamp = (-pramp->nLeftVol << VOLUMERAMPPRECISION) / nRampLength;
		pramp->nRampRightVol = pramp->nRightVol << VOLUMERAMPPRECISION;
		pramp->nRampLeftVol = pramp->nLeftVol << VOLUMERAMPPRECISION;
		pramp->nRampLength = nRampLength;
		pramp->dwFlags |= CHN_VOLUMERAMP;
	}
	m_dwSongFlags |= SONG_FADINGSONG;
	return TRUE;
}


BOOL CSoundFile::GlobalFadeSong(UINT msec)
//----------------------------------------
{
	if (m_dwSongFlags & SONG_GLOBALFADE) return FALSE;
	m_nGlobalFadeMaxSamples = _muldiv(msec, gdwMixingFreq, 1000);
	m_nGlobalFadeSamples = m_nGlobalFadeMaxSamples;
	m_dwSongFlags |= SONG_GLOBALFADE;
	return TRUE;
}


UINT CSoundFile::Read(LPVOID lpDestBuffer, UINT cbBuffer)
//-------------------------------------------------------
{
	LPBYTE lpBuffer = (LPBYTE)lpDestBuffer;
	LPCONVERTPROC pCvt = X86_Convert32To8;
	UINT lRead, lMax, lSampleSize, lCount, lSampleCount, nStat=0;
	LONG nVUMeterMin = 0x7FFFFFFF, nVUMeterMax = -0x7FFFFFFF;
	UINT nMaxPlugins;

	{
		nMaxPlugins = MAX_MIXPLUGINS;
		while ((nMaxPlugins > 0) && (!m_MixPlugins[nMaxPlugins-1].pMixPlugin)) nMaxPlugins--;
	}
	m_nMixStat = 0;
	lSampleSize = gnChannels;
	if (gnBitsPerSample == 16) { lSampleSize *= 2; pCvt = X86_Convert32To16; }
#ifndef MODPLUG_FASTSOUNDLIB
	else if (gnBitsPerSample == 24) { lSampleSize *= 3; pCvt = X86_Convert32To24; }
	else if (gnBitsPerSample == 32) { lSampleSize *= 4; pCvt = X86_Convert32To32; }
#endif
	lMax = cbBuffer / lSampleSize;
	if ((!lMax) || (!lpBuffer) || (!m_nChannels)) return 0;
	lRead = lMax;
	if (m_dwSongFlags & SONG_ENDREACHED) goto MixDone;
	while (lRead > 0)
	{
		// Update Channel Data
		if (!m_nBufferCount)
		{
#ifndef MODPLUG_FASTSOUNDLIB
			if (m_dwSongFlags & SONG_FADINGSONG)
			{
				m_dwSongFlags |= SONG_ENDREACHED;
				m_nBufferCount = lRead;
			} else
#endif
			if (!ReadNote())
			{
#ifndef MODPLUG_FASTSOUNDLIB
				if (!FadeSong(FADESONGDELAY))
#endif
				{
					m_dwSongFlags |= SONG_ENDREACHED;
					if (lRead == lMax) goto MixDone;
					m_nBufferCount = lRead;
				}
			}
		}
		lCount = m_nBufferCount;
		if (lCount > MIXBUFFERSIZE) lCount = MIXBUFFERSIZE;
		if (lCount > lRead) lCount = lRead;
		if (!lCount) break;
		lSampleCount = lCount;
#ifndef MODPLUG_NO_REVERB
		gnReverbSend = 0;
#endif
		// Resetting sound buffer
		X86_StereoFill(MixSoundBuffer, lSampleCount, &gnDryROfsVol, &gnDryLOfsVol);
		if (gnChannels >= 2)
		{
			lSampleCount *= 2;
			m_nMixStat += CreateStereoMix(lCount);
			ProcessStereoDSP(lCount);
		} else
		{
			m_nMixStat += CreateStereoMix(lCount);
			if (nMaxPlugins) ProcessPlugins(lCount);
			ProcessStereoDSP(lCount);
			X86_MonoFromStereo(MixSoundBuffer, lCount);
		}
		nStat++;
#ifndef NO_AGC
		// Automatic Gain Control
		if (gdwSoundSetup & SNDMIX_AGC) ProcessAGC(lSampleCount);
#endif
		UINT lTotalSampleCount = lSampleCount;
#ifndef MODPLUG_FASTSOUNDLIB
		// Multichannel
		if (gnChannels > 2)
		{
			X86_InterleaveFrontRear(MixSoundBuffer, MixRearBuffer, lSampleCount);
			lTotalSampleCount *= 2;
		}
		// Hook Function
		if (gpSndMixHook)
		{
			gpSndMixHook(MixSoundBuffer, lTotalSampleCount, gnChannels);
		}
#endif
		// Perform clipping + VU-Meter
		lpBuffer += pCvt(lpBuffer, MixSoundBuffer, lTotalSampleCount, &nVUMeterMin, &nVUMeterMax);
		// Buffer ready
		lRead -= lCount;
		m_nBufferCount -= lCount;
	}
MixDone:
	if (lRead) memset(lpBuffer, (gnBitsPerSample == 8) ? 0x80 : 0, lRead * lSampleSize);
	// VU-Meter
	nVUMeterMin >>= (24-MIXING_ATTENUATION);
	nVUMeterMax >>= (24-MIXING_ATTENUATION);
	if (nVUMeterMax < nVUMeterMin) nVUMeterMax = nVUMeterMin;
	if ((gnVUMeter = (UINT)(nVUMeterMax - nVUMeterMin)) > 0xFF) gnVUMeter = 0xFF;
	if (nStat) { m_nMixStat += nStat-1; m_nMixStat /= nStat; }
	return lMax - lRead;
}



/////////////////////////////////////////////////////////////////////////////
// Handles navigation/effects

BOOL CSoundFile::ProcessRow()
//---------------------------
{
	if (++m_nTickCount >= m_nMusicSpeed * (m_nPatternDelay+1) + m_nFrameDelay)
	{
		m_nPatternDelay = 0;
		m_nFrameDelay = 0;
		m_nTickCount = 0;
		m_nRow = m_nNextRow;
		// Reset Pattern Loop Effect
		if (m_nCurrentPattern != m_nNextPattern) m_nCurrentPattern = m_nNextPattern;
		// Check if pattern is valid
		if (!(m_dwSongFlags & SONG_PATTERNLOOP))
		{
			m_nPattern = (m_nCurrentPattern < MAX_ORDERS) ? Order[m_nCurrentPattern] : 0xFF;
			if ((m_nPattern < MAX_PATTERNS) && (!Patterns[m_nPattern])) m_nPattern = 0xFE;
			while (m_nPattern >= MAX_PATTERNS)
			{
				// End of song ?
				if ((m_nPattern == 0xFF) || (m_nCurrentPattern >= MAX_ORDERS))
				{
					//if (!m_nRepeatCount)
						return FALSE;     //never repeat entire song
					if (!m_nRestartPos)
					{
						m_nMusicSpeed = m_nDefaultSpeed;
						m_nMusicTempo = m_nDefaultTempo;
						m_nGlobalVolume = m_nDefaultGlobalVolume;
						for (UINT i=0; i<MAX_CHANNELS; i++)
						{
							Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
							Chn[i].nFadeOutVol = 0;
							if (i < m_nChannels)
							{
								Chn[i].nGlobalVol = ChnSettings[i].nVolume;
								Chn[i].nVolume = ChnSettings[i].nVolume;
								Chn[i].nPan = ChnSettings[i].nPan;
								Chn[i].nPanSwing = Chn[i].nVolSwing = 0;
								Chn[i].nOldVolParam = 0;
								Chn[i].nOldOffset = 0;
								Chn[i].nOldHiOffset = 0;
								Chn[i].nPortamentoDest = 0;
								if (!Chn[i].nLength)
								{
									Chn[i].dwFlags = ChnSettings[i].dwFlags;
									Chn[i].nLoopStart = 0;
									Chn[i].nLoopEnd = 0;
									Chn[i].pHeader = NULL;
									Chn[i].pSample = NULL;
									Chn[i].pInstrument = NULL;
								}
							}
						}
					}
//					if (m_nRepeatCount > 0) m_nRepeatCount--;
					m_nCurrentPattern = m_nRestartPos;
					m_nRow = 0;
					if ((Order[m_nCurrentPattern] >= MAX_PATTERNS) || (!Patterns[Order[m_nCurrentPattern]])) return FALSE;
				} else
				{
					m_nCurrentPattern++;
				}
				m_nPattern = (m_nCurrentPattern < MAX_ORDERS) ? Order[m_nCurrentPattern] : 0xFF;
				if ((m_nPattern < MAX_PATTERNS) && (!Patterns[m_nPattern])) m_nPattern = 0xFE;
			}
			m_nNextPattern = m_nCurrentPattern;
		}
		// Weird stuff?
		if ((m_nPattern >= MAX_PATTERNS) || (!Patterns[m_nPattern])) return FALSE;
		// Should never happen
		if (m_nRow >= PatternSize[m_nPattern]) m_nRow = 0;
		m_nNextRow = m_nRow + 1;
		if (m_nNextRow >= PatternSize[m_nPattern])
		{
			if (!(m_dwSongFlags & SONG_PATTERNLOOP)) m_nNextPattern = m_nCurrentPattern + 1;
			m_nNextRow = 0;
		}
		// Reset channel values
		MODCHANNEL *pChn = Chn;
		MODCOMMAND *m = Patterns[m_nPattern] + m_nRow * m_nChannels;
		for (UINT nChn=0; nChn<m_nChannels; pChn++, nChn++, m++)
		{
			pChn->nRowNote = m->note;
			pChn->nRowInstr = m->instr;
			pChn->nRowVolCmd = m->volcmd;
			pChn->nRowVolume = m->vol;
			pChn->nRowCommand = m->command;
			pChn->nRowParam = m->param;

			pChn->nLeftVol = pChn->nNewLeftVol;
			pChn->nRightVol = pChn->nNewRightVol;
			pChn->dwFlags &= ~(CHN_PORTAMENTO | CHN_VIBRATO | CHN_TREMOLO | CHN_PANBRELLO);
			pChn->nCommand = 0;
		}
	}
	// Should we process tick0 effects?
	if (!m_nMusicSpeed) m_nMusicSpeed = 1;
	m_dwSongFlags |= SONG_FIRSTTICK;
	if (m_nTickCount)
	{
		m_dwSongFlags &= ~SONG_FIRSTTICK;
		if ((!(m_nType & MOD_TYPE_XM)) && (m_nTickCount < m_nMusicSpeed * (1 + m_nPatternDelay)))
		{
			if (!(m_nTickCount % m_nMusicSpeed)) m_dwSongFlags |= SONG_FIRSTTICK;
		}

	}
	// Update Effects
	return ProcessEffects();
}


////////////////////////////////////////////////////////////////////////////////////////////
// Handles envelopes & mixer setup

BOOL CSoundFile::ReadNote()
//-------------------------
{
	if (!ProcessRow()) return FALSE;
	////////////////////////////////////////////////////////////////////////////////////
	m_nTotalCount++;
	if (!m_nMusicTempo) return FALSE;
	m_nBufferCount = (gdwMixingFreq * 5 * m_nTempoFactor) / (m_nMusicTempo << 8);
	// Master Volume + Pre-Amplification / Attenuation setup
	DWORD nMasterVol;
	{
		int nchn32 = (m_nChannels < 32) ? m_nChannels : 31;
		if ((m_nType & MOD_TYPE_IT) && (m_nInstruments) && (nchn32 < 6)) nchn32 = 6;
		int realmastervol = m_nMasterVolume;
		if (realmastervol > 0x80)
		{
			realmastervol = 0x80 + ((realmastervol - 0x80) * (nchn32+4)) / 16;
		}
		UINT attenuation = (gdwSoundSetup & SNDMIX_AGC) ? PreAmpAGCTable[nchn32>>1] : PreAmpTable[nchn32>>1];
		DWORD mastervol = (realmastervol * (m_nSongPreAmp + 0x10)) >> 6;
		if (mastervol > 0x200) mastervol = 0x200;
		if ((m_dwSongFlags & SONG_GLOBALFADE) && (m_nGlobalFadeMaxSamples))
		{
			mastervol = _muldiv(mastervol, m_nGlobalFadeSamples, m_nGlobalFadeMaxSamples);
		}
		nMasterVol = (mastervol << 7) / attenuation;
		if (nMasterVol > 0x180) nMasterVol = 0x180;
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Update channels data
	m_nMixChannels = 0;
	MODCHANNEL *pChn = Chn;
	for (UINT nChn=0; nChn<MAX_CHANNELS; nChn++,pChn++)
	{
		if ((pChn->dwFlags & CHN_NOTEFADE) && (!(pChn->nFadeOutVol|pChn->nRightVol|pChn->nLeftVol)))
		{
			pChn->nLength = 0;
			pChn->nROfs = pChn->nLOfs = 0;
		}
		// Check for unused channel
		if ((pChn->dwFlags & CHN_MUTE) || ((nChn >= m_nChannels) && (!pChn->nLength)))
		{
			pChn->nVUMeter = 0;
#ifdef ENABLE_STEREOVU
			pChn->nLeftVU = pChn->nRightVU = 0;
#endif
			continue;
		}
		// Reset channel data
		pChn->nInc = 0;
		pChn->nRealVolume = 0;
		pChn->nRealPan = pChn->nPan + pChn->nPanSwing;
		if (pChn->nRealPan < 0) pChn->nRealPan = 0;
		if (pChn->nRealPan > 256) pChn->nRealPan = 256;
		pChn->nRampLength = 0;
		// Calc Frequency
		if ((pChn->nPeriod)	&& (pChn->nLength))
		{
			int vol = pChn->nVolume + pChn->nVolSwing;

			if (vol < 0) vol = 0;
			if (vol > 256) vol = 256;
			// Tremolo
			if (pChn->dwFlags & CHN_TREMOLO)
			{
				UINT trempos = pChn->nTremoloPos & 0x3F;
				if (vol > 0)
				{
					int tremattn = (m_nType & MOD_TYPE_XM) ? 5 : 6;
					switch (pChn->nTremoloType & 0x03)
					{
					case 1:
						vol += (ModRampDownTable[trempos] * (int)pChn->nTremoloDepth) >> tremattn;
						break;
					case 2:
						vol += (ModSquareTable[trempos] * (int)pChn->nTremoloDepth) >> tremattn;
						break;
					case 3:
						vol += (ModRandomTable[trempos] * (int)pChn->nTremoloDepth) >> tremattn;
						break;
					default:
						vol += (ModSinusTable[trempos] * (int)pChn->nTremoloDepth) >> tremattn;
					}
				}
				if ((m_nTickCount) || ((m_nType & (MOD_TYPE_STM|MOD_TYPE_S3M|MOD_TYPE_IT)) && (!(m_dwSongFlags & SONG_ITOLDEFFECTS))))
				{
					pChn->nTremoloPos = (trempos + pChn->nTremoloSpeed) & 0x3F;
				}
			}
			// Tremor
			if (pChn->nCommand == CMD_TREMOR)
			{
				UINT n = (pChn->nTremorParam >> 4) + (pChn->nTremorParam & 0x0F);
				UINT ontime = pChn->nTremorParam >> 4;
				if ((!(m_nType & MOD_TYPE_IT)) || (m_dwSongFlags & SONG_ITOLDEFFECTS)) { n += 2; ontime++; }
				UINT tremcount = (UINT)pChn->nTremorCount;
				if (tremcount >= n) tremcount = 0;
				if ((m_nTickCount) || (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT)))
				{
					if (tremcount >= ontime) vol = 0;
					pChn->nTremorCount = (BYTE)(tremcount + 1);
				}
				pChn->dwFlags |= CHN_FASTVOLRAMP;
			}
			// Clip volume
			if (vol < 0) vol = 0;
			if (vol > 0x100) vol = 0x100;
			vol <<= 6;
			// Process Envelopes
			if (pChn->pHeader)
			{
				INSTRUMENTHEADER *penv = pChn->pHeader;
				// Volume Envelope
				if ((pChn->dwFlags & CHN_VOLENV) && (penv->nVolEnv))
				{
					int envpos = pChn->nVolEnvPosition;
					UINT pt = penv->nVolEnv - 1;
					for (UINT i=0; i<(UINT)(penv->nVolEnv-1); i++)
					{
						if (envpos <= penv->VolPoints[i])
						{
							pt = i;
							break;
						}
					}
					int x2 = penv->VolPoints[pt];
					int x1, envvol;
					if (envpos >= x2)
					{
						envvol = penv->VolEnv[pt] << 2;
						x1 = x2;
					} else
					if (pt)
					{
						envvol = penv->VolEnv[pt-1] << 2;
						x1 = penv->VolPoints[pt-1];
					} else
					{
						envvol = 0;
						x1 = 0;
					}
					if (envpos > x2) envpos = x2;
					if ((x2 > x1) && (envpos > x1))
					{
						envvol += ((envpos - x1) * (((int)penv->VolEnv[pt]<<2) - envvol)) / (x2 - x1);
					}
					if (envvol < 0) envvol = 0;
					if (envvol > 256) envvol = 256;
					vol = (vol * envvol) >> 8;
				}
				// Panning Envelope
				if ((pChn->dwFlags & CHN_PANENV) && (penv->nPanEnv))
				{
					int envpos = pChn->nPanEnvPosition;
					UINT pt = penv->nPanEnv - 1;
					for (UINT i=0; i<(UINT)(penv->nPanEnv-1); i++)
					{
						if (envpos <= penv->PanPoints[i])
						{
							pt = i;
							break;
						}
					}
					int x2 = penv->PanPoints[pt], y2 = penv->PanEnv[pt];
					int x1, envpan;
					if (envpos >= x2)
					{
						envpan = y2;
						x1 = x2;
					} else
					if (pt)
					{
						envpan = penv->PanEnv[pt-1];
						x1 = penv->PanPoints[pt-1];
					} else
					{
						envpan = 128;
						x1 = 0;
					}
					if ((x2 > x1) && (envpos > x1))
					{
						envpan += ((envpos - x1) * (y2 - envpan)) / (x2 - x1);
					}
					if (envpan < 0) envpan = 0;
					if (envpan > 64) envpan = 64;
					int pan = pChn->nPan;
					if (pan >= 128)
					{
						pan += ((envpan - 32) * (256 - pan)) / 32;
					} else
					{
						pan += ((envpan - 32) * (pan)) / 32;
					}
					if (pan < 0) pan = 0;
					if (pan > 256) pan = 256;
					pChn->nRealPan = pan;
				}
				// FadeOut volume
				if (pChn->dwFlags & CHN_NOTEFADE)
				{
					UINT fadeout = penv->nFadeOut;
					if (fadeout)
					{
						pChn->nFadeOutVol -= fadeout << 1;
						if (pChn->nFadeOutVol <= 0) pChn->nFadeOutVol = 0;
						vol = (vol * pChn->nFadeOutVol) >> 16;
					} else
					if (!pChn->nFadeOutVol)
					{
						vol = 0;
					}
				}
				// Pitch/Pan separation
				if ((penv->nPPS) && (pChn->nRealPan) && (pChn->nNote))
				{
					int pandelta = (int)pChn->nRealPan + (int)((int)(pChn->nNote - penv->nPPC - 1) * (int)penv->nPPS) / (int)8;
					if (pandelta < 0) pandelta = 0;
					if (pandelta > 256) pandelta = 256;
					pChn->nRealPan = pandelta;
				}
			} else
			{
				// No Envelope: key off => note cut
				if (pChn->dwFlags & CHN_NOTEFADE) // 1.41-: CHN_KEYOFF|CHN_NOTEFADE
				{
					pChn->nFadeOutVol = 0;
					vol = 0;
				}
			}
			// vol is 14-bits
			if (vol)
			{
				// IMPORTANT: pChn->nRealVolume is 14 bits !!!
				// -> _muldiv( 14+8, 6+6, 18); => RealVolume: 14-bit result (22+12-20)
				pChn->nRealVolume = _muldiv(vol * m_nGlobalVolume, pChn->nGlobalVol * pChn->nInsVol, 1 << 20);
			}
			if (pChn->nPeriod < m_nMinPeriod) pChn->nPeriod = m_nMinPeriod;
			int period = pChn->nPeriod;
			if ((pChn->dwFlags & (CHN_GLISSANDO|CHN_PORTAMENTO)) ==	(CHN_GLISSANDO|CHN_PORTAMENTO))
			{
				period = GetPeriodFromNote(GetNoteFromPeriod(period), pChn->nFineTune, pChn->nC4Speed);
			}

			// Arpeggio ?
			if (pChn->nCommand == CMD_ARPEGGIO)
			{
				switch(m_nTickCount % 3)
				{
				case 1:	period = GetPeriodFromNote(pChn->nNote + (pChn->nArpeggio >> 4), pChn->nFineTune, pChn->nC4Speed); break;
				case 2:	period = GetPeriodFromNote(pChn->nNote + (pChn->nArpeggio & 0x0F), pChn->nFineTune, pChn->nC4Speed); break;
				}
			}

			if (m_dwSongFlags & SONG_AMIGALIMITS)
			{
				if (period < 113*4) period = 113*4;
				if (period > 856*4) period = 856*4;
			}

			// Pitch/Filter Envelope
			if ((pChn->pHeader) && (pChn->dwFlags & CHN_PITCHENV) && (pChn->pHeader->nPitchEnv))
			{
				INSTRUMENTHEADER *penv = pChn->pHeader;
				int envpos = pChn->nPitchEnvPosition;
				UINT pt = penv->nPitchEnv - 1;
				for (UINT i=0; i<(UINT)(penv->nPitchEnv-1); i++)
				{
					if (envpos <= penv->PitchPoints[i])
					{
						pt = i;
						break;
					}
				}
				int x2 = penv->PitchPoints[pt];
				int x1, envpitch;
				if (envpos >= x2)
				{
					envpitch = (((int)penv->PitchEnv[pt]) - 32) * 8;
					x1 = x2;
				} else
				if (pt)
				{
					envpitch = (((int)penv->PitchEnv[pt-1]) - 32) * 8;
					x1 = penv->PitchPoints[pt-1];
				} else
				{
					envpitch = 0;
					x1 = 0;
				}
				if (envpos > x2) envpos = x2;
				if ((x2 > x1) && (envpos > x1))
				{
					int envpitchdest = (((int)penv->PitchEnv[pt]) - 32) * 8;
					envpitch += ((envpos - x1) * (envpitchdest - envpitch)) / (x2 - x1);
				}
				if (envpitch < -256) envpitch = -256;
				if (envpitch > 256) envpitch = 256;
				// Filter Envelope: controls cutoff frequency
				if (penv->dwFlags & ENV_FILTER)
				{
#ifndef NO_FILTER
					SetupChannelFilter(pChn, (pChn->dwFlags & CHN_FILTER) ? FALSE : TRUE, envpitch);
#endif // NO_FILTER
				} else
				// Pitch Envelope
				{
					int l = envpitch;
					if (l < 0)
					{
						l = -l;
						if (l > 255) l = 255;
						period = _muldiv(period, LinearSlideUpTable[l], 0x10000);
					} else
					{
						if (l > 255) l = 255;
						period = _muldiv(period, LinearSlideDownTable[l], 0x10000);
					}
				}
			}

			// Vibrato
			if (pChn->dwFlags & CHN_VIBRATO)
			{
				UINT vibpos = pChn->nVibratoPos;
				LONG vdelta;
				switch (pChn->nVibratoType & 0x03)
				{
				case 1:
					vdelta = ModRampDownTable[vibpos];
					break;
				case 2:
					vdelta = ModSquareTable[vibpos];
					break;
				case 3:
					vdelta = ModRandomTable[vibpos];
					break;
				default:
					vdelta = ModSinusTable[vibpos];
				}
				UINT vdepth = ((m_nType != MOD_TYPE_IT) || (m_dwSongFlags & SONG_ITOLDEFFECTS)) ? 6 : 7;
				vdelta = (vdelta * (int)pChn->nVibratoDepth) >> vdepth;
				if ((m_dwSongFlags & SONG_LINEARSLIDES) && (m_nType & MOD_TYPE_IT))
				{
					LONG l = vdelta;
					if (l < 0)
					{
						l = -l;
						vdelta = _muldiv(period, LinearSlideDownTable[l >> 2], 0x10000) - period;
						if (l & 0x03) vdelta += _muldiv(period, FineLinearSlideDownTable[l & 0x03], 0x10000) - period;

					} else
					{
						vdelta = _muldiv(period, LinearSlideUpTable[l >> 2], 0x10000) - period;
						if (l & 0x03) vdelta += _muldiv(period, FineLinearSlideUpTable[l & 0x03], 0x10000) - period;

					}
				}
				period += vdelta;
				if ((m_nTickCount) || ((m_nType & MOD_TYPE_IT) && (!(m_dwSongFlags & SONG_ITOLDEFFECTS))))
				{
					pChn->nVibratoPos = (vibpos + pChn->nVibratoSpeed) & 0x3F;
				}
			}
			// Panbrello
			if (pChn->dwFlags & CHN_PANBRELLO)
			{
				UINT panpos = ((pChn->nPanbrelloPos+0x10) >> 2) & 0x3F;
				LONG pdelta;
				switch (pChn->nPanbrelloType & 0x03)
				{
				case 1:
					pdelta = ModRampDownTable[panpos];
					break;
				case 2:
					pdelta = ModSquareTable[panpos];
					break;
				case 3:
					pdelta = ModRandomTable[panpos];
					break;
				default:
					pdelta = ModSinusTable[panpos];
				}
				pChn->nPanbrelloPos += pChn->nPanbrelloSpeed;
				pdelta = ((pdelta * (int)pChn->nPanbrelloDepth) + 2) >> 3;
				pdelta += pChn->nRealPan;
				if (pdelta < 0) pdelta = 0;
				if (pdelta > 256) pdelta = 256;
				pChn->nRealPan = pdelta;
			}
			int nPeriodFrac = 0;
			// Instrument Auto-Vibrato
			if ((pChn->pInstrument) && (pChn->pInstrument->nVibDepth))
			{
				MODINSTRUMENT *pins = pChn->pInstrument;
				if (pins->nVibSweep == 0)
				{
					pChn->nAutoVibDepth = pins->nVibDepth << 8;
				} else
				{
					if (m_nType & MOD_TYPE_IT)
					{
						pChn->nAutoVibDepth += pins->nVibSweep << 3;
					} else
					if (!(pChn->dwFlags & CHN_KEYOFF))
					{
						pChn->nAutoVibDepth += (pins->nVibDepth << 8) /	pins->nVibSweep;
					}
					if ((pChn->nAutoVibDepth >> 8) > pins->nVibDepth)
						pChn->nAutoVibDepth = pins->nVibDepth << 8;
				}
				pChn->nAutoVibPos += pins->nVibRate;
				int val;
				switch(pins->nVibType)
				{
				case 4:	// Random
					val = ModRandomTable[pChn->nAutoVibPos & 0x3F];
					pChn->nAutoVibPos++;
					break;
				case 3:	// Ramp Down
					val = ((0x40 - (pChn->nAutoVibPos >> 1)) & 0x7F) - 0x40;
					break;
				case 2:	// Ramp Up
					val = ((0x40 + (pChn->nAutoVibPos >> 1)) & 0x7f) - 0x40;
					break;
				case 1:	// Square
					val = (pChn->nAutoVibPos & 128) ? +64 : -64;
					break;
				default:	// Sine
					val = ft2VibratoTable[pChn->nAutoVibPos & 255];
				}
				int n =	((val * pChn->nAutoVibDepth) >> 8);
				if (m_nType & MOD_TYPE_IT)
				{
					int df1, df2;
					if (n < 0)
					{
						n = -n;
						UINT n1 = n >> 8;
						df1 = LinearSlideUpTable[n1];
						df2 = LinearSlideUpTable[n1+1];
					} else
					{
						UINT n1 = n >> 8;
						df1 = LinearSlideDownTable[n1];
						df2 = LinearSlideDownTable[n1+1];
					}
					n >>= 2;
					period = _muldiv(period, df1 + ((df2-df1)*(n&0x3F)>>6), 256);
					nPeriodFrac = period & 0xFF;
					period >>= 8;
				} else
				{
					period += (n >> 6);
				}
			}
			// Final Period
			if (period <= m_nMinPeriod)
			{
				if (m_nType & MOD_TYPE_S3M) pChn->nLength = 0;
				period = m_nMinPeriod;
			}
			if (period > m_nMaxPeriod)
			{
				if ((m_nType & MOD_TYPE_IT) || (period >= 0x100000))
				{
					pChn->nFadeOutVol = 0;
					pChn->dwFlags |= CHN_NOTEFADE;
					pChn->nRealVolume = 0;
				}
				period = m_nMaxPeriod;
				nPeriodFrac = 0;
			}
			UINT freq = GetFreqFromPeriod(period, pChn->nC4Speed, nPeriodFrac);
			if ((m_nType & MOD_TYPE_IT) && (freq < 256))
			{
				pChn->nFadeOutVol = 0;
				pChn->dwFlags |= CHN_NOTEFADE;
				pChn->nRealVolume = 0;
			}
			UINT ninc = _muldiv(freq, 0x10000, gdwMixingFreq);
			if ((ninc >= 0xFFB0) && (ninc <= 0x10090)) ninc = 0x10000;
			if (m_nFreqFactor != 128) ninc = (ninc * m_nFreqFactor) >> 7;
			if (ninc > 0xFF0000) ninc = 0xFF0000;
			pChn->nInc = (ninc+1) & ~3;
		}

		// Increment envelope position
		if (pChn->pHeader)
		{
			INSTRUMENTHEADER *penv = pChn->pHeader;
			// Volume Envelope
			if (pChn->dwFlags & CHN_VOLENV)
			{
				// Increase position
				pChn->nVolEnvPosition++;
				// Volume Loop ?
				if (penv->dwFlags & ENV_VOLLOOP)
				{
					UINT volloopend = penv->VolPoints[penv->nVolLoopEnd];
					if (m_nType != MOD_TYPE_XM) volloopend++;
					if (pChn->nVolEnvPosition == volloopend)
					{
						pChn->nVolEnvPosition = penv->VolPoints[penv->nVolLoopStart];
						if ((penv->nVolLoopEnd == penv->nVolLoopStart) && (!penv->VolEnv[penv->nVolLoopStart])
						 && ((!(m_nType & MOD_TYPE_XM)) || (penv->nVolLoopEnd+1 == penv->nVolEnv)))
						{
							pChn->dwFlags |= CHN_NOTEFADE;
							pChn->nFadeOutVol = 0;
						}
					}
				}
				// Volume Sustain ?
				if ((penv->dwFlags & ENV_VOLSUSTAIN) && (!(pChn->dwFlags & CHN_KEYOFF)))
				{
					if (pChn->nVolEnvPosition == (UINT)penv->VolPoints[penv->nVolSustainEnd]+1)
						pChn->nVolEnvPosition = penv->VolPoints[penv->nVolSustainBegin];
				} else
				// End of Envelope ?
				if (pChn->nVolEnvPosition > penv->VolPoints[penv->nVolEnv - 1])
				{
					if ((m_nType & MOD_TYPE_IT) || (pChn->dwFlags & CHN_KEYOFF)) pChn->dwFlags |= CHN_NOTEFADE;
					pChn->nVolEnvPosition = penv->VolPoints[penv->nVolEnv - 1];
					if ((!penv->VolEnv[penv->nVolEnv-1]) && ((nChn >= m_nChannels) || (m_nType & MOD_TYPE_IT)))
					{
						pChn->dwFlags |= CHN_NOTEFADE;
						pChn->nFadeOutVol = 0;

						pChn->nRealVolume = 0;
					}
				}
			}
			// Panning Envelope
			if (pChn->dwFlags & CHN_PANENV)
			{
				pChn->nPanEnvPosition++;
				if (penv->dwFlags & ENV_PANLOOP)
				{
					UINT panloopend = penv->PanPoints[penv->nPanLoopEnd];
					if (m_nType != MOD_TYPE_XM) panloopend++;
					if (pChn->nPanEnvPosition == panloopend)
						pChn->nPanEnvPosition = penv->PanPoints[penv->nPanLoopStart];
				}
				// Panning Sustain ?
				if ((penv->dwFlags & ENV_PANSUSTAIN) && (pChn->nPanEnvPosition == (UINT)penv->PanPoints[penv->nPanSustainEnd]+1)
				 && (!(pChn->dwFlags & CHN_KEYOFF)))
				{
					// Panning sustained
					pChn->nPanEnvPosition = penv->PanPoints[penv->nPanSustainBegin];
				} else
				{
					if (pChn->nPanEnvPosition > penv->PanPoints[penv->nPanEnv - 1])
						pChn->nPanEnvPosition = penv->PanPoints[penv->nPanEnv - 1];
				}
			}
			// Pitch Envelope
			if (pChn->dwFlags & CHN_PITCHENV)
			{
				// Increase position
				pChn->nPitchEnvPosition++;
				// Pitch Loop ?
				if (penv->dwFlags & ENV_PITCHLOOP)
				{
					if (pChn->nPitchEnvPosition >= penv->PitchPoints[penv->nPitchLoopEnd])
						pChn->nPitchEnvPosition = penv->PitchPoints[penv->nPitchLoopStart];
				}
				// Pitch Sustain ?
				if ((penv->dwFlags & ENV_PITCHSUSTAIN) && (!(pChn->dwFlags & CHN_KEYOFF)))
				{
					if (pChn->nPitchEnvPosition == (UINT)penv->PitchPoints[penv->nPitchSustainEnd]+1)
						pChn->nPitchEnvPosition = penv->PitchPoints[penv->nPitchSustainBegin];
				} else
				{
					if (pChn->nPitchEnvPosition > penv->PitchPoints[penv->nPitchEnv - 1])
						pChn->nPitchEnvPosition = penv->PitchPoints[penv->nPitchEnv - 1];
				}
			}
		}
#ifdef MODPLUG_PLAYER
		// Limit CPU -> > 80% -> don't ramp
		if ((gnCPUUsage >= 80) && (!pChn->nRealVolume))
		{
			pChn->nLeftVol = pChn->nRightVol = 0;
		}
#endif // MODPLUG_PLAYER
		// Volume ramping
		pChn->dwFlags &= ~CHN_VOLUMERAMP;
		if ((pChn->nRealVolume) || (pChn->nLeftVol) || (pChn->nRightVol))
			pChn->dwFlags |= CHN_VOLUMERAMP;
#ifdef MODPLUG_PLAYER
		// Decrease VU-Meter
		if (pChn->nVUMeter > VUMETER_DECAY)	pChn->nVUMeter -= VUMETER_DECAY; else pChn->nVUMeter = 0;
#endif // MODPLUG_PLAYER
#ifdef ENABLE_STEREOVU
		if (pChn->nLeftVU > VUMETER_DECAY) pChn->nLeftVU -= VUMETER_DECAY; else pChn->nLeftVU = 0;
		if (pChn->nRightVU > VUMETER_DECAY) pChn->nRightVU -= VUMETER_DECAY; else pChn->nRightVU = 0;
#endif
		// Check for too big nInc
		if (((pChn->nInc >> 16) + 1) >= (LONG)(pChn->nLoopEnd - pChn->nLoopStart)) pChn->dwFlags &= ~CHN_LOOP;
		pChn->nNewRightVol = pChn->nNewLeftVol = 0;
		pChn->pCurrentSample = ((pChn->pSample) && (pChn->nLength) && (pChn->nInc)) ? pChn->pSample : NULL;
		if (pChn->pCurrentSample)
		{
			// Update VU-Meter (nRealVolume is 14-bit)
#ifdef MODPLUG_PLAYER
			UINT vutmp = pChn->nRealVolume >> (14 - 8);
			if (vutmp > 0xFF) vutmp = 0xFF;
			if (pChn->nVUMeter >= 0x100) pChn->nVUMeter = vutmp;
			vutmp >>= 1;
			if (pChn->nVUMeter < vutmp)	pChn->nVUMeter = vutmp;
#endif // MODPLUG_PLAYER
#ifdef ENABLE_STEREOVU
			UINT vul = (pChn->nRealVolume * pChn->nRealPan) >> 14;
			if (vul > 127) vul = 127;
			if (pChn->nLeftVU > 127) pChn->nLeftVU = (BYTE)vul;
			vul >>= 1;
			if (pChn->nLeftVU < vul) pChn->nLeftVU = (BYTE)vul;
			UINT vur = (pChn->nRealVolume * (256-pChn->nRealPan)) >> 14;
			if (vur > 127) vur = 127;
			if (pChn->nRightVU > 127) pChn->nRightVU = (BYTE)vur;
			vur >>= 1;
			if (pChn->nRightVU < vur) pChn->nRightVU = (BYTE)vur;
#endif
#ifdef MODPLUG_TRACKER
			UINT kChnMasterVol = (pChn->dwFlags & CHN_EXTRALOUD) ? 0x100 : nMasterVol;
#else
#define		kChnMasterVol	nMasterVol
#endif // MODPLUG_TRACKER
			// Adjusting volumes
			if (gnChannels >= 2)
			{
				int pan = ((int)pChn->nRealPan) - 128;
				pan *= (int)m_nStereoSeparation;
				pan /= 128;
				pan += 128;

				if (pan < 0) pan = 0;
				if (pan > 256) pan = 256;
#ifndef MODPLUG_FASTSOUNDLIB
				if (gdwSoundSetup & SNDMIX_REVERSESTEREO) pan = 256 - pan;
#endif
				LONG realvol = (pChn->nRealVolume * kChnMasterVol) >> (8-1);
				if (gdwSoundSetup & SNDMIX_SOFTPANNING)
				{
					if (pan < 128)
					{
						pChn->nNewLeftVol = (realvol * pan) >> 8;
						pChn->nNewRightVol = (realvol * 128) >> 8;
					} else
					{
						pChn->nNewLeftVol = (realvol * 128) >> 8;
						pChn->nNewRightVol = (realvol * (256 - pan)) >> 8;
					}
				} else
				{
					pChn->nNewLeftVol = (realvol * pan) >> 8;
					pChn->nNewRightVol = (realvol * (256 - pan)) >> 8;
				}
			} else
			{
				pChn->nNewRightVol = (pChn->nRealVolume * kChnMasterVol) >> 8;
				pChn->nNewLeftVol = pChn->nNewRightVol;
			}
			// Clipping volumes
			if (pChn->nNewRightVol > 0xFFFF) pChn->nNewRightVol = 0xFFFF;
			if (pChn->nNewLeftVol > 0xFFFF) pChn->nNewLeftVol = 0xFFFF;
			// Check IDO
			if (gdwSoundSetup & SNDMIX_NORESAMPLING)
			{
				pChn->dwFlags |= CHN_NOIDO;
			} else
			{
				pChn->dwFlags &= ~(CHN_NOIDO|CHN_HQSRC);
				if( pChn->nInc == 0x10000 )
				{	pChn->dwFlags |= CHN_NOIDO;
				}
				else
				{	if( ((gdwSoundSetup & SNDMIX_HQRESAMPLER) == 0) && ((gdwSoundSetup & SNDMIX_ULTRAHQSRCMODE) == 0) )
					{	if (pChn->nInc >= 0xFF00) pChn->dwFlags |= CHN_NOIDO;
					}
				}
			}
			pChn->nNewRightVol >>= MIXING_ATTENUATION;
			pChn->nNewLeftVol >>= MIXING_ATTENUATION;
			pChn->nRightRamp = pChn->nLeftRamp = 0;
			// Dolby Pro-Logic Surround
			if ((pChn->dwFlags & CHN_SURROUND) && (gnChannels <= 2)) pChn->nNewLeftVol = - pChn->nNewLeftVol;
			// Checking Ping-Pong Loops
			if (pChn->dwFlags & CHN_PINGPONGFLAG) pChn->nInc = -pChn->nInc;
			// Setting up volume ramp
			if ((pChn->dwFlags & CHN_VOLUMERAMP)
			 && ((pChn->nRightVol != pChn->nNewRightVol)
			  || (pChn->nLeftVol != pChn->nNewLeftVol)))
			{
				LONG nRampLength = gnVolumeRampSamples;
				LONG nRightDelta = ((pChn->nNewRightVol - pChn->nRightVol) << VOLUMERAMPPRECISION);
				LONG nLeftDelta = ((pChn->nNewLeftVol - pChn->nLeftVol) << VOLUMERAMPPRECISION);
#ifndef MODPLUG_FASTSOUNDLIB
				if ((gdwSoundSetup & SNDMIX_DIRECTTODISK)
				 || ((gdwSysInfo & (SYSMIX_ENABLEMMX|SYSMIX_FASTCPU))
				  && (gdwSoundSetup & SNDMIX_HQRESAMPLER) && (gnCPUUsage <= 20)))
				{
					if ((pChn->nRightVol|pChn->nLeftVol) && (pChn->nNewRightVol|pChn->nNewLeftVol) && (!(pChn->dwFlags & CHN_FASTVOLRAMP)))
					{
						nRampLength = m_nBufferCount;
						if (nRampLength > (1 << (VOLUMERAMPPRECISION-1))) nRampLength = (1 << (VOLUMERAMPPRECISION-1));
						if (nRampLength < (LONG)gnVolumeRampSamples) nRampLength = gnVolumeRampSamples;
					}
				}
#endif
				pChn->nRightRamp = nRightDelta / nRampLength;
				pChn->nLeftRamp = nLeftDelta / nRampLength;
				pChn->nRightVol = pChn->nNewRightVol - ((pChn->nRightRamp * nRampLength) >> VOLUMERAMPPRECISION);
				pChn->nLeftVol = pChn->nNewLeftVol - ((pChn->nLeftRamp * nRampLength) >> VOLUMERAMPPRECISION);
				if (pChn->nRightRamp|pChn->nLeftRamp)
				{
					pChn->nRampLength = nRampLength;
				} else
				{
					pChn->dwFlags &= ~CHN_VOLUMERAMP;
					pChn->nRightVol = pChn->nNewRightVol;
					pChn->nLeftVol = pChn->nNewLeftVol;
				}
			} else
			{
				pChn->dwFlags &= ~CHN_VOLUMERAMP;
				pChn->nRightVol = pChn->nNewRightVol;
				pChn->nLeftVol = pChn->nNewLeftVol;
			}
			pChn->nRampRightVol = pChn->nRightVol << VOLUMERAMPPRECISION;
			pChn->nRampLeftVol = pChn->nLeftVol << VOLUMERAMPPRECISION;
			// Adding the channel in the channel list
			ChnMix[m_nMixChannels++] = nChn;
			if (m_nMixChannels >= MAX_CHANNELS) break;
		} else
		{
#ifdef ENABLE_STEREOVU
			// Note change but no sample
			if (pChn->nLeftVU > 128) pChn->nLeftVU = 0;
			if (pChn->nRightVU > 128) pChn->nRightVU = 0;
#endif
			if (pChn->nVUMeter > 0xFF) pChn->nVUMeter = 0;
			pChn->nLeftVol = pChn->nRightVol = 0;
			pChn->nLength = 0;
		}
	}
	// Checking Max Mix Channels reached: ordering by volume
	if ((m_nMixChannels >= m_nMaxMixChannels) && (!(gdwSoundSetup & SNDMIX_DIRECTTODISK)))
	{
		for (UINT i=0; i<m_nMixChannels; i++)
		{
			UINT j=i;
			while ((j+1<m_nMixChannels) && (Chn[ChnMix[j]].nRealVolume < Chn[ChnMix[j+1]].nRealVolume))
			{
				UINT n = ChnMix[j];
				ChnMix[j] = ChnMix[j+1];
				ChnMix[j+1] = n;
				j++;
			}
		}
	}
	if (m_dwSongFlags & SONG_GLOBALFADE)
	{
		if (!m_nGlobalFadeSamples)
		{
			m_dwSongFlags |= SONG_ENDREACHED;
			return FALSE;
		}
		if (m_nGlobalFadeSamples > m_nBufferCount)
			m_nGlobalFadeSamples -= m_nBufferCount;
		else
			m_nGlobalFadeSamples = 0;
	}
	return TRUE;
}


