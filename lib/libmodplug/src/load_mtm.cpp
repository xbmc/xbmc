/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

#include "stdafx.h"
#include "sndfile.h"

//#pragma warning(disable:4244)

//////////////////////////////////////////////////////////
// MTM file support (import only)

#pragma pack(1)


typedef struct tagMTMSAMPLE
{
        char samplename[22];      // changed from CHAR
	DWORD length;
	DWORD reppos;
	DWORD repend;
	CHAR finetune;
	BYTE volume;
	BYTE attribute;
} MTMSAMPLE;


typedef struct tagMTMHEADER
{
	char id[4];	    // MTM file marker + version // changed from CHAR
	char songname[20];  // ASCIIZ songname  // changed from CHAR
	WORD numtracks;	    // number of tracks saved
	BYTE lastpattern;   // last pattern number saved
	BYTE lastorder;	    // last order number to play (songlength-1)
	WORD commentsize;   // length of comment field
	BYTE numsamples;    // number of samples saved
	BYTE attribute;	    // attribute byte (unused)
	BYTE beatspertrack;
	BYTE numchannels;   // number of channels used
	BYTE panpos[32];    // voice pan positions
} MTMHEADER;


#pragma pack()


BOOL CSoundFile::ReadMTM(LPCBYTE lpStream, DWORD dwMemLength)
//-----------------------------------------------------------
{
	MTMHEADER *pmh = (MTMHEADER *)lpStream;
	DWORD dwMemPos = 66;

	if ((!lpStream) || (dwMemLength < 0x100)) return FALSE;
	if ((strncmp(pmh->id, "MTM", 3)) || (pmh->numchannels > 32)
	 || (pmh->numsamples >= MAX_SAMPLES) || (!pmh->numsamples)
	 || (!pmh->numtracks) || (!pmh->numchannels)
	 || (!pmh->lastpattern) || (pmh->lastpattern > MAX_PATTERNS)) 
		return FALSE;
	strncpy(m_szNames[0], pmh->songname, 20);
	m_szNames[0][20] = 0;
	if (dwMemPos + 37*pmh->numsamples + 128 + 192*pmh->numtracks
	 + 64 * (pmh->lastpattern+1) + pmh->commentsize >= dwMemLength) 
		return FALSE;
	m_nType = MOD_TYPE_MTM;
	m_nSamples = pmh->numsamples;
	m_nChannels = pmh->numchannels;
	// Reading instruments
	for	(UINT i=1; i<=m_nSamples; i++)
	{
		MTMSAMPLE *pms = (MTMSAMPLE *)(lpStream + dwMemPos);
		strncpy(m_szNames[i], pms->samplename, 22);
		m_szNames[i][22] = 0;
		Ins[i].nVolume = pms->volume << 2;
		Ins[i].nGlobalVol = 64;
		DWORD len = pms->length;
		if ((len > 4) && (len <= MAX_SAMPLE_LENGTH))
		{
			Ins[i].nLength = len;
			Ins[i].nLoopStart = pms->reppos;
			Ins[i].nLoopEnd = pms->repend;
			if (Ins[i].nLoopEnd > Ins[i].nLength) 
				Ins[i].nLoopEnd = Ins[i].nLength;
			if (Ins[i].nLoopStart + 4 >= Ins[i].nLoopEnd) 
				Ins[i].nLoopStart = Ins[i].nLoopEnd = 0;
			if (Ins[i].nLoopEnd) Ins[i].uFlags |= CHN_LOOP;
			Ins[i].nFineTune = MOD2XMFineTune(pms->finetune);
			if (pms->attribute & 0x01)
			{
				Ins[i].uFlags |= CHN_16BIT;
				Ins[i].nLength >>= 1;
				Ins[i].nLoopStart >>= 1;
				Ins[i].nLoopEnd >>= 1;
			}
			Ins[i].nPan = 128;
		}
		dwMemPos += 37;
	}
	// Setting Channel Pan Position
	for (UINT ich=0; ich<m_nChannels; ich++)
	{
		ChnSettings[ich].nPan = ((pmh->panpos[ich] & 0x0F) << 4) + 8;
		ChnSettings[ich].nVolume = 64;
	}
	// Reading pattern order
	memcpy(Order, lpStream + dwMemPos, pmh->lastorder+1);
	dwMemPos += 128;
	// Reading Patterns
	LPCBYTE pTracks = lpStream + dwMemPos;
	dwMemPos += 192 * pmh->numtracks;
	LPWORD pSeq = (LPWORD)(lpStream + dwMemPos);
	for (UINT pat=0; pat<=pmh->lastpattern; pat++)
	{
		PatternSize[pat] = 64;
		if ((Patterns[pat] = AllocatePattern(64, m_nChannels)) == NULL) break;
		for (UINT n=0; n<32; n++) if ((pSeq[n]) && (pSeq[n] <= pmh->numtracks) && (n < m_nChannels))
		{
			LPCBYTE p = pTracks + 192 * (pSeq[n]-1);
			MODCOMMAND *m = Patterns[pat] + n;
			for (UINT i=0; i<64; i++, m+=m_nChannels, p+=3)
			{
				if (p[0] & 0xFC) m->note = (p[0] >> 2) + 37;
				m->instr = ((p[0] & 0x03) << 4) | (p[1] >> 4);
				UINT cmd = p[1] & 0x0F;
				UINT param = p[2];
				if (cmd == 0x0A)
				{
					if (param & 0xF0) param &= 0xF0; else param &= 0x0F;
				}
				m->command = cmd;
				m->param = param;
				if ((cmd) || (param)) ConvertModCommand(m);
			}
		}
		pSeq += 32;
	}
	dwMemPos += 64*(pmh->lastpattern+1);
	if ((pmh->commentsize) && (dwMemPos + pmh->commentsize < dwMemLength))
	{
		UINT n = pmh->commentsize;
		m_lpszSongComments = new char[n+1];
		if (m_lpszSongComments)
		{
			memcpy(m_lpszSongComments, lpStream+dwMemPos, n);
			m_lpszSongComments[n] = 0;
			for (UINT i=0; i<n; i++)
			{
				if (!m_lpszSongComments[i])
				{
					m_lpszSongComments[i] = ((i+1) % 40) ? 0x20 : 0x0D;
				}
			}
		}
	}
	dwMemPos += pmh->commentsize;
	// Reading Samples
	for (UINT ismp=1; ismp<=m_nSamples; ismp++)
	{
		if (dwMemPos >= dwMemLength) break;
		dwMemPos += ReadSample(&Ins[ismp], (Ins[ismp].uFlags & CHN_16BIT) ? RS_PCM16U : RS_PCM8U,
								(LPSTR)(lpStream + dwMemPos), dwMemLength - dwMemPos);
	}
	m_nMinPeriod = 64;
	m_nMaxPeriod = 32767;
	return TRUE;
}

