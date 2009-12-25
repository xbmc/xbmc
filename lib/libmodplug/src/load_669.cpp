/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>,
 *          Adam Goode       <adam@evdebs.org> (endian and char fixes for PPC)
*/

////////////////////////////////////////////////////////////
// 669 Composer / UNIS 669 module loader
////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "sndfile.h"

//#pragma warning(disable:4244)

typedef struct tagFILEHEADER669
{
	WORD sig;				// 'if' or 'JN'
        signed char songmessage[108];	// Song Message
	BYTE samples;			// number of samples (1-64)
	BYTE patterns;			// number of patterns (1-128)
	BYTE restartpos;
	BYTE orders[128];
	BYTE tempolist[128];
	BYTE breaks[128];
} FILEHEADER669;


typedef struct tagSAMPLE669
{
	BYTE filename[13];
	BYTE length[4];	// when will somebody think about DWORD align ???
	BYTE loopstart[4];
	BYTE loopend[4];
} SAMPLE669;


BOOL CSoundFile::Read669(const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
	BOOL b669Ext;
	const FILEHEADER669 *pfh = (const FILEHEADER669 *)lpStream;
	const SAMPLE669 *psmp = (const SAMPLE669 *)(lpStream + 0x1F1);
	DWORD dwMemPos = 0;

	if ((!lpStream) || (dwMemLength < sizeof(FILEHEADER669))) return FALSE;
	if ((bswapLE16(pfh->sig) != 0x6669) && (bswapLE16(pfh->sig) != 0x4E4A)) return FALSE;
	b669Ext = (bswapLE16(pfh->sig) == 0x4E4A) ? TRUE : FALSE;
	if ((!pfh->samples) || (pfh->samples > 64) || (pfh->restartpos >= 128)
	 || (!pfh->patterns) || (pfh->patterns > 128)) return FALSE;
	DWORD dontfuckwithme = 0x1F1 + pfh->samples * sizeof(SAMPLE669) + pfh->patterns * 0x600;
	if (dontfuckwithme > dwMemLength) return FALSE;
	for (UINT ichk=0; ichk<pfh->samples; ichk++)
	{
		DWORD len = bswapLE32(*((DWORD *)(&psmp[ichk].length)));
		dontfuckwithme += len;
	}
	if (dontfuckwithme > dwMemLength) return FALSE;
	// That should be enough checking: this must be a 669 module.
	m_nType = MOD_TYPE_669;
	m_dwSongFlags |= SONG_LINEARSLIDES;
	m_nMinPeriod = 28 << 2;
	m_nMaxPeriod = 1712 << 3;
	m_nDefaultTempo = 125;
	m_nDefaultSpeed = 6;
	m_nChannels = 8;
	memcpy(m_szNames[0], pfh->songmessage, 16);
	m_nSamples = pfh->samples;
	for (UINT nins=1; nins<=m_nSamples; nins++, psmp++)
	{
		DWORD len = bswapLE32(*((DWORD *)(&psmp->length)));
		DWORD loopstart = bswapLE32(*((DWORD *)(&psmp->loopstart)));
		DWORD loopend = bswapLE32(*((DWORD *)(&psmp->loopend)));
		if (len > MAX_SAMPLE_LENGTH) len = MAX_SAMPLE_LENGTH;
		if ((loopend > len) && (!loopstart)) loopend = 0;
		if (loopend > len) loopend = len;
		if (loopstart + 4 >= loopend) loopstart = loopend = 0;
		Ins[nins].nLength = len;
		Ins[nins].nLoopStart = loopstart;
		Ins[nins].nLoopEnd = loopend;
		if (loopend) Ins[nins].uFlags |= CHN_LOOP;
		memcpy(m_szNames[nins], psmp->filename, 13);
		Ins[nins].nVolume = 256;
		Ins[nins].nGlobalVol = 64;
		Ins[nins].nPan = 128;
	}
	// Song Message
	m_lpszSongComments = new char[109];
	memcpy(m_lpszSongComments, pfh->songmessage, 108);
	m_lpszSongComments[108] = 0;
	// Reading Orders
	memcpy(Order, pfh->orders, 128);
	m_nRestartPos = pfh->restartpos;
	if (Order[m_nRestartPos] >= pfh->patterns) m_nRestartPos = 0;
	// Reading Pattern Break Locations
	for (UINT npan=0; npan<8; npan++)
	{
		ChnSettings[npan].nPan = (npan & 1) ? 0x30 : 0xD0;
		ChnSettings[npan].nVolume = 64;
	}
	// Reading Patterns
	dwMemPos = 0x1F1 + pfh->samples * 25;
	for (UINT npat=0; npat<pfh->patterns; npat++)
	{
		Patterns[npat] = AllocatePattern(64, m_nChannels);
		if (!Patterns[npat]) break;
		PatternSize[npat] = 64;
		MODCOMMAND *m = Patterns[npat];
		const BYTE *p = lpStream + dwMemPos;
		for (UINT row=0; row<64; row++)
		{
			MODCOMMAND *mspeed = m;
			if ((row == pfh->breaks[npat]) && (row != 63))
			{
				for (UINT i=0; i<8; i++)
				{
					m[i].command = CMD_PATTERNBREAK;
					m[i].param = 0;
				}
			}
			for (UINT n=0; n<8; n++, m++, p+=3)
			{
				UINT note = p[0] >> 2;
				UINT instr = ((p[0] & 0x03) << 4) | (p[1] >> 4);
				UINT vol = p[1] & 0x0F;
				if (p[0] < 0xFE)
				{
					m->note = note + 37;
					m->instr = instr + 1;
				}
				if (p[0] <= 0xFE)
				{
					m->volcmd = VOLCMD_VOLUME;
					m->vol = (vol << 2) + 2;
				}
				if (p[2] != 0xFF)
				{
					UINT command = p[2] >> 4;
					UINT param = p[2] & 0x0F;
					switch(command)
					{
					case 0x00:	command = CMD_PORTAMENTOUP; break;
					case 0x01:	command = CMD_PORTAMENTODOWN; break;
					case 0x02:	command = CMD_TONEPORTAMENTO; break;
					case 0x03:	command = CMD_MODCMDEX; param |= 0x50; break;
					case 0x04:	command = CMD_VIBRATO; param |= 0x40; break;
					case 0x05:	if (param) command = CMD_SPEED; else command = 0; param += 2; break;
					case 0x06:	if (param == 0) { command = CMD_PANNINGSLIDE; param = 0xFE; } else
								if (param == 1) { command = CMD_PANNINGSLIDE; param = 0xEF; } else
								command = 0;
								break;
					default:	command = 0;
					}
					if (command)
					{
						if (command == CMD_SPEED) mspeed = NULL;
						m->command = command;
						m->param = param;
					}
				}
			}
			if ((!row) && (mspeed))
			{
				for (UINT i=0; i<8; i++) if (!mspeed[i].command)
				{
					mspeed[i].command = CMD_SPEED;
					mspeed[i].param = pfh->tempolist[npat] + 2;
					break;
				}
			}
		}
		dwMemPos += 0x600;
	}
	// Reading Samples
	for (UINT n=1; n<=m_nSamples; n++)
	{
		UINT len = Ins[n].nLength;
		if (dwMemPos >= dwMemLength) break;
		if (len > 4) ReadSample(&Ins[n], RS_PCM8U, (LPSTR)(lpStream+dwMemPos), dwMemLength - dwMemPos);
		dwMemPos += len;
	}
	return TRUE;
}


