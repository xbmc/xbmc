/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

#include "stdafx.h"
#include "sndfile.h"

//#pragma warning(disable:4244)

#define ULT_16BIT   0x04
#define ULT_LOOP    0x08
#define ULT_BIDI    0x10

#pragma pack(1)

// Raw ULT header struct:
typedef struct tagULTHEADER
{
        char id[15];             // changed from CHAR
        char songtitle[32];      // changed from CHAR
	BYTE reserved;
} ULTHEADER;


// Raw ULT sampleinfo struct:
typedef struct tagULTSAMPLE
{
	CHAR samplename[32];
	CHAR dosname[12];
	LONG loopstart;
	LONG loopend;
	LONG sizestart;
	LONG sizeend;
	BYTE volume;
	BYTE flags;
	WORD finetune;
} ULTSAMPLE;

#pragma pack()


BOOL CSoundFile::ReadUlt(const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
	ULTHEADER *pmh = (ULTHEADER *)lpStream;
	ULTSAMPLE *pus;
	UINT nos, nop;
	DWORD dwMemPos = 0;

	// try to read module header
	if ((!lpStream) || (dwMemLength < 0x100)) return FALSE;
	if (strncmp(pmh->id,"MAS_UTrack_V00",14)) return FALSE;
	// Warning! Not supported ULT format, trying anyway
	// if ((pmh->id[14] < '1') || (pmh->id[14] > '4')) return FALSE;
	m_nType = MOD_TYPE_ULT;
	m_nDefaultSpeed = 6;
	m_nDefaultTempo = 125;
	memcpy(m_szNames[0], pmh->songtitle, 32);
	// read songtext
	dwMemPos = sizeof(ULTHEADER);
	if ((pmh->reserved) && (dwMemPos + pmh->reserved * 32 < dwMemLength))
	{
		UINT len = pmh->reserved * 32;
		m_lpszSongComments = new char[len + 1 + pmh->reserved];
		if (m_lpszSongComments)
		{
			for (UINT l=0; l<pmh->reserved; l++)
			{
				memcpy(m_lpszSongComments+l*33, lpStream+dwMemPos+l*32, 32);
				m_lpszSongComments[l*33+32] = 0x0D;
			}
			m_lpszSongComments[len] = 0;
		}
		dwMemPos += len;
	}
	if (dwMemPos >= dwMemLength) return TRUE;
	nos = lpStream[dwMemPos++];
	m_nSamples = nos;
	if (m_nSamples >= MAX_SAMPLES) m_nSamples = MAX_SAMPLES-1;
	UINT smpsize = 64;
	if (pmh->id[14] >= '4')	smpsize += 2;
	if (dwMemPos + nos*smpsize + 256 + 2 > dwMemLength) return TRUE;
	for (UINT ins=1; ins<=nos; ins++, dwMemPos+=smpsize) if (ins<=m_nSamples)
	{
		pus	= (ULTSAMPLE *)(lpStream+dwMemPos);
		MODINSTRUMENT *pins = &Ins[ins];
		memcpy(m_szNames[ins], pus->samplename, 32);
		memcpy(pins->name, pus->dosname, 12);
		pins->nLoopStart = pus->loopstart;
		pins->nLoopEnd = pus->loopend;
		pins->nLength = pus->sizeend - pus->sizestart;
		pins->nVolume = pus->volume;
		pins->nGlobalVol = 64;
		pins->nC4Speed = 8363;
		if (pmh->id[14] >= '4')
		{
			pins->nC4Speed = pus->finetune;
		}
		if (pus->flags & ULT_LOOP) pins->uFlags |= CHN_LOOP;
		if (pus->flags & ULT_BIDI) pins->uFlags |= CHN_PINGPONGLOOP;
		if (pus->flags & ULT_16BIT)
		{
			pins->uFlags |= CHN_16BIT;
			pins->nLoopStart >>= 1;
			pins->nLoopEnd >>= 1;
		}
	}
	memcpy(Order, lpStream+dwMemPos, 256);
	dwMemPos += 256;
	m_nChannels = lpStream[dwMemPos] + 1;
	nop = lpStream[dwMemPos+1] + 1;
	dwMemPos += 2;
	if (m_nChannels > 32) m_nChannels = 32;
	// Default channel settings
	for (UINT nSet=0; nSet<m_nChannels; nSet++)
	{
		ChnSettings[nSet].nVolume = 64;
		ChnSettings[nSet].nPan = (nSet & 1) ? 0x40 : 0xC0;
	}
	// read pan position table for v1.5 and higher
	if(pmh->id[14]>='3')
	{
		if (dwMemPos + m_nChannels > dwMemLength) return TRUE;
		for(UINT t=0; t<m_nChannels; t++)
		{
			ChnSettings[t].nPan = (lpStream[dwMemPos++] << 4) + 8;
			if (ChnSettings[t].nPan > 256) ChnSettings[t].nPan = 256;
		}
	}
	// Allocating Patterns
	for (UINT nAllocPat=0; nAllocPat<nop; nAllocPat++)
	{
		if (nAllocPat < MAX_PATTERNS)
		{
			PatternSize[nAllocPat] = 64;
			Patterns[nAllocPat] = AllocatePattern(64, m_nChannels);
		}
	}
	// Reading Patterns
	for (UINT nChn=0; nChn<m_nChannels; nChn++)
	{
		for (UINT nPat=0; nPat<nop; nPat++)
		{
			MODCOMMAND *pat = NULL;
			
			if (nPat < MAX_PATTERNS)
			{
				pat = Patterns[nPat];
				if (pat) pat += nChn;
			}
			UINT row = 0;
			while (row < 64)
			{
				if (dwMemPos + 6 > dwMemLength) return TRUE;
				UINT rep = 1;
				UINT note = lpStream[dwMemPos++];
				if (note == 0xFC)
				{
					rep = lpStream[dwMemPos];
					note = lpStream[dwMemPos+1];
					dwMemPos += 2;
				}
				UINT instr = lpStream[dwMemPos++];
				UINT eff = lpStream[dwMemPos++];
				UINT dat1 = lpStream[dwMemPos++];
				UINT dat2 = lpStream[dwMemPos++];
				UINT cmd1 = eff & 0x0F;
				UINT cmd2 = eff >> 4;
				if (cmd1 == 0x0C) dat1 >>= 2; else
				if (cmd1 == 0x0B) { cmd1 = dat1 = 0; }
				if (cmd2 == 0x0C) dat2 >>= 2; else
				if (cmd2 == 0x0B) { cmd2 = dat2 = 0; }
				while ((rep != 0) && (row < 64))
				{
					if (pat)
					{
						pat->instr = instr;
						if (note) pat->note = note + 36;
						if (cmd1 | dat1)
						{
							if (cmd1 == 0x0C)
							{
								pat->volcmd = VOLCMD_VOLUME;
								pat->vol = dat1;
							} else
							{
								pat->command = cmd1;
								pat->param = dat1;
								ConvertModCommand(pat);
							}
						}
						if (cmd2 == 0x0C)
						{
							pat->volcmd = VOLCMD_VOLUME;
							pat->vol = dat2;
						} else
						if ((cmd2 | dat2) && (!pat->command))
						{
							pat->command = cmd2;
							pat->param = dat2;
							ConvertModCommand(pat);
						}
						pat += m_nChannels;
					}
					row++;
					rep--;
				}
			}
		}
	}
	// Reading Instruments
	for (UINT smp=1; smp<=m_nSamples; smp++) if (Ins[smp].nLength)
	{
		if (dwMemPos >= dwMemLength) return TRUE;
		UINT flags = (Ins[smp].uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S;
		dwMemPos += ReadSample(&Ins[smp], flags, (LPSTR)(lpStream+dwMemPos), dwMemLength - dwMemPos);
	}
	return TRUE;
}

