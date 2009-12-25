/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>,
 *          Adam Goode       <adam@evdebs.org> (endian and char fixes for PPC)
*/

#include "stdafx.h"
#include "sndfile.h"
#include "tables.h"

#ifdef _MSC_VER
//#pragma warning(disable:4244)
#endif

//////////////////////////////////////////////////////////
// ProTracker / NoiseTracker MOD/NST file support

void CSoundFile::ConvertModCommand(MODCOMMAND *m) const
//-----------------------------------------------------
{
	UINT command = m->command, param = m->param;

	switch(command)
	{
	case 0x00:	if (param) command = CMD_ARPEGGIO; break;
	case 0x01:	command = CMD_PORTAMENTOUP; break;
	case 0x02:	command = CMD_PORTAMENTODOWN; break;
	case 0x03:	command = CMD_TONEPORTAMENTO; break;
	case 0x04:	command = CMD_VIBRATO; break;
	case 0x05:	command = CMD_TONEPORTAVOL; if (param & 0xF0) param &= 0xF0; break;
	case 0x06:	command = CMD_VIBRATOVOL; if (param & 0xF0) param &= 0xF0; break;
	case 0x07:	command = CMD_TREMOLO; break;
	case 0x08:	command = CMD_PANNING8; break;
	case 0x09:	command = CMD_OFFSET; break;
	case 0x0A:	command = CMD_VOLUMESLIDE; if (param & 0xF0) param &= 0xF0; break;
	case 0x0B:	command = CMD_POSITIONJUMP; break;
	case 0x0C:	command = CMD_VOLUME; break;
	case 0x0D:	command = CMD_PATTERNBREAK; param = ((param >> 4) * 10) + (param & 0x0F); break;
	case 0x0E:	command = CMD_MODCMDEX; break;
	case 0x0F:	command = (param <= (UINT)((m_nType & (MOD_TYPE_XM|MOD_TYPE_MT2)) ? 0x1F : 0x20)) ? CMD_SPEED : CMD_TEMPO;
				if ((param == 0xFF) && (m_nSamples == 15)) command = 0; break;
	// Extension for XM extended effects
	case 'G' - 55:	command = CMD_GLOBALVOLUME; break;
	case 'H' - 55:	command = CMD_GLOBALVOLSLIDE; if (param & 0xF0) param &= 0xF0; break;
	case 'K' - 55:	command = CMD_KEYOFF; break;
	case 'L' - 55:	command = CMD_SETENVPOSITION; break;
	case 'M' - 55:	command = CMD_CHANNELVOLUME; break;
	case 'N' - 55:	command = CMD_CHANNELVOLSLIDE; break;
	case 'P' - 55:	command = CMD_PANNINGSLIDE; if (param & 0xF0) param &= 0xF0; break;
	case 'R' - 55:	command = CMD_RETRIG; break;
	case 'T' - 55:	command = CMD_TREMOR; break;
	case 'X' - 55:	command = CMD_XFINEPORTAUPDOWN;	break;
	case 'Y' - 55:	command = CMD_PANBRELLO; break;
	case 'Z' - 55:	command = CMD_MIDI;	break;
	default:	command = 0;
	}
	m->command = command;
	m->param = param;
}


WORD CSoundFile::ModSaveCommand(const MODCOMMAND *m, BOOL bXM) const
//------------------------------------------------------------------
{
	UINT command = m->command & 0x3F, param = m->param;

	switch(command)
	{
	case 0:						command = param = 0; break;
	case CMD_ARPEGGIO:			command = 0; break;
	case CMD_PORTAMENTOUP:
		if (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_STM))
		{
			if ((param & 0xF0) == 0xE0) { command=0x0E; param=((param & 0x0F) >> 2)|0x10; break; }
			else if ((param & 0xF0) == 0xF0) { command=0x0E; param &= 0x0F; param|=0x10; break; }
		}
		command = 0x01;
		break;
	case CMD_PORTAMENTODOWN:
		if (m_nType & (MOD_TYPE_S3M|MOD_TYPE_IT|MOD_TYPE_STM))
		{
			if ((param & 0xF0) == 0xE0) { command=0x0E; param=((param & 0x0F) >> 2)|0x20; break; }
			else if ((param & 0xF0) == 0xF0) { command=0x0E; param &= 0x0F; param|=0x20; break; }
		}
		command = 0x02;
		break;
	case CMD_TONEPORTAMENTO:	command = 0x03; break;
	case CMD_VIBRATO:			command = 0x04; break;
	case CMD_TONEPORTAVOL:		command = 0x05; break;
	case CMD_VIBRATOVOL:		command = 0x06; break;
	case CMD_TREMOLO:			command = 0x07; break;
	case CMD_PANNING8:			
		command = 0x08;
		if (bXM)
		{
			if ((m_nType != MOD_TYPE_IT) && (m_nType != MOD_TYPE_XM) && (param <= 0x80))
			{
				param <<= 1;
				if (param > 255) param = 255;
			}
		} else
		{
			if ((m_nType == MOD_TYPE_IT) || (m_nType == MOD_TYPE_XM)) param >>= 1;
		}
		break;
	case CMD_OFFSET:			command = 0x09; break;
	case CMD_VOLUMESLIDE:		command = 0x0A; break;
	case CMD_POSITIONJUMP:		command = 0x0B; break;
	case CMD_VOLUME:			command = 0x0C; break;
	case CMD_PATTERNBREAK:		command = 0x0D; param = ((param / 10) << 4) | (param % 10); break;
	case CMD_MODCMDEX:			command = 0x0E; break;
	case CMD_SPEED:				command = 0x0F; if (param > 0x20) param = 0x20; break;
	case CMD_TEMPO:				if (param > 0x20) { command = 0x0F; break; }
	case CMD_GLOBALVOLUME:		command = 'G' - 55; break;
	case CMD_GLOBALVOLSLIDE:	command = 'H' - 55; break;
	case CMD_KEYOFF:			command = 'K' - 55; break;
	case CMD_SETENVPOSITION:	command = 'L' - 55; break;
	case CMD_CHANNELVOLUME:		command = 'M' - 55; break;
	case CMD_CHANNELVOLSLIDE:	command = 'N' - 55; break;
	case CMD_PANNINGSLIDE:		command = 'P' - 55; break;
	case CMD_RETRIG:			command = 'R' - 55; break;
	case CMD_TREMOR:			command = 'T' - 55; break;
	case CMD_XFINEPORTAUPDOWN:	command = 'X' - 55; break;
	case CMD_PANBRELLO:			command = 'Y' - 55; break;
	case CMD_MIDI:				command = 'Z' - 55; break;
	case CMD_S3MCMDEX:
		switch(param & 0xF0)
		{
		case 0x10:	command = 0x0E; param = (param & 0x0F) | 0x30; break;
		case 0x20:	command = 0x0E; param = (param & 0x0F) | 0x50; break;
		case 0x30:	command = 0x0E; param = (param & 0x0F) | 0x40; break;
		case 0x40:	command = 0x0E; param = (param & 0x0F) | 0x70; break;
		case 0x90:	command = 'X' - 55; break;
		case 0xB0:	command = 0x0E; param = (param & 0x0F) | 0x60; break;
		case 0xA0:
		case 0x50:
		case 0x70:
		case 0x60:	command = param = 0; break;
		default:	command = 0x0E; break;
		}
		break;
	default:		command = param = 0;
	}
	return (WORD)((command << 8) | (param));
}


#pragma pack(1)

typedef struct _MODSAMPLE
{
	CHAR name[22];
	WORD length;
	BYTE finetune;
	BYTE volume;
	WORD loopstart;
	WORD looplen;
} MODSAMPLE, *PMODSAMPLE;

typedef struct _MODMAGIC
{
	BYTE nOrders;
	BYTE nRestartPos;
	BYTE Orders[128];
        char Magic[4];          // changed from CHAR
} MODMAGIC, *PMODMAGIC;

#pragma pack()

BOOL IsMagic(LPCSTR s1, LPCSTR s2)
{
	return ((*(DWORD *)s1) == (*(DWORD *)s2)) ? TRUE : FALSE;
}


BOOL CSoundFile::ReadMod(const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
        char s[1024];          // changed from CHAR
	DWORD dwMemPos, dwTotalSampleLen;
	PMODMAGIC pMagic;
	UINT nErr;

	if ((!lpStream) || (dwMemLength < 0x600)) return FALSE;
	dwMemPos = 20;
	m_nSamples = 31;
	m_nChannels = 4;
	pMagic = (PMODMAGIC)(lpStream+dwMemPos+sizeof(MODSAMPLE)*31);
	// Check Mod Magic
	memcpy(s, pMagic->Magic, 4);
	if ((IsMagic(s, "M.K.")) || (IsMagic(s, "M!K!"))
	 || (IsMagic(s, "M&K!")) || (IsMagic(s, "N.T."))) m_nChannels = 4; else
	if ((IsMagic(s, "CD81")) || (IsMagic(s, "OKTA"))) m_nChannels = 8; else
	if ((s[0]=='F') && (s[1]=='L') && (s[2]=='T') && (s[3]>='4') && (s[3]<='9')) m_nChannels = s[3] - '0'; else
	if ((s[0]>='2') && (s[0]<='9') && (s[1]=='C') && (s[2]=='H') && (s[3]=='N')) m_nChannels = s[0] - '0'; else
	if ((s[0]=='1') && (s[1]>='0') && (s[1]<='9') && (s[2]=='C') && (s[3]=='H')) m_nChannels = s[1] - '0' + 10; else
	if ((s[0]=='2') && (s[1]>='0') && (s[1]<='9') && (s[2]=='C') && (s[3]=='H')) m_nChannels = s[1] - '0' + 20; else
	if ((s[0]=='3') && (s[1]>='0') && (s[1]<='2') && (s[2]=='C') && (s[3]=='H')) m_nChannels = s[1] - '0' + 30; else
	if ((s[0]=='T') && (s[1]=='D') && (s[2]=='Z') && (s[3]>='4') && (s[3]<='9')) m_nChannels = s[3] - '0'; else
	if (IsMagic(s,"16CN")) m_nChannels = 16; else
	if (IsMagic(s,"32CN")) m_nChannels = 32; else m_nSamples = 15;
	// Load Samples
	nErr = 0;
	dwTotalSampleLen = 0;
	for	(UINT i=1; i<=m_nSamples; i++)
	{
		PMODSAMPLE pms = (PMODSAMPLE)(lpStream+dwMemPos);
		MODINSTRUMENT *psmp = &Ins[i];
		UINT loopstart, looplen;

		memcpy(m_szNames[i], pms->name, 22);
		m_szNames[i][22] = 0;
		psmp->uFlags = 0;
		psmp->nLength = bswapBE16(pms->length)*2;
		dwTotalSampleLen += psmp->nLength;
		psmp->nFineTune = MOD2XMFineTune(pms->finetune & 0x0F);
		psmp->nVolume = 4*pms->volume;
		if (psmp->nVolume > 256) { psmp->nVolume = 256; nErr++; }
		psmp->nGlobalVol = 64;
		psmp->nPan = 128;
		loopstart = bswapBE16(pms->loopstart)*2;
		looplen = bswapBE16(pms->looplen)*2;
		// Fix loops
		if ((looplen > 2) && (loopstart+looplen > psmp->nLength)
		 && (loopstart/2+looplen <= psmp->nLength))
		{
			loopstart /= 2;
		}
		psmp->nLoopStart = loopstart;
		psmp->nLoopEnd = loopstart + looplen;
		if (psmp->nLength < 4) psmp->nLength = 0;
		if (psmp->nLength)
		{
			UINT derr = 0;
			if (psmp->nLoopStart >= psmp->nLength) { psmp->nLoopStart = psmp->nLength-1; derr|=1; }
			if (psmp->nLoopEnd > psmp->nLength) { psmp->nLoopEnd = psmp->nLength; derr |= 1; }
			if (psmp->nLoopStart > psmp->nLoopEnd) derr |= 1;
			if ((psmp->nLoopStart > psmp->nLoopEnd) || (psmp->nLoopEnd <= 8)
			 || (psmp->nLoopEnd - psmp->nLoopStart <= 4))
			{
				psmp->nLoopStart = 0;
				psmp->nLoopEnd = 0;
			}
			if (psmp->nLoopEnd > psmp->nLoopStart)
			{
				psmp->uFlags |= CHN_LOOP;
			}
		}
		dwMemPos += sizeof(MODSAMPLE);
	}
	if ((m_nSamples == 15) && (dwTotalSampleLen > dwMemLength * 4)) return FALSE;
	pMagic = (PMODMAGIC)(lpStream+dwMemPos);
	dwMemPos += sizeof(MODMAGIC);
	if (m_nSamples == 15) dwMemPos -= 4;
	memset(Order, 0,sizeof(Order));
	memcpy(Order, pMagic->Orders, 128);

	UINT nbp, nbpbuggy, nbpbuggy2, norders;

	norders = pMagic->nOrders;
	if ((!norders) || (norders > 0x80))
	{
		norders = 0x80;
		while ((norders > 1) && (!Order[norders-1])) norders--;
	}
	nbpbuggy = 0;
	nbpbuggy2 = 0;
	nbp = 0;
	for (UINT iord=0; iord<128; iord++)
	{
		UINT i = Order[iord];
		if ((i < 0x80) && (nbp <= i))
		{
			nbp = i+1;
			if (iord<norders) nbpbuggy = nbp;
		}
		if (i >= nbpbuggy2) nbpbuggy2 = i+1;
	}
	for (UINT iend=norders; iend<MAX_ORDERS; iend++) Order[iend] = 0xFF;
	norders--;
	m_nRestartPos = pMagic->nRestartPos;
	if (m_nRestartPos >= 0x78) m_nRestartPos = 0;
	if (m_nRestartPos + 1 >= (UINT)norders) m_nRestartPos = 0;
	if (!nbp) return FALSE;
	DWORD dwWowTest = dwTotalSampleLen+dwMemPos;
	if ((IsMagic(pMagic->Magic, "M.K.")) && (dwWowTest + nbp*8*256 == dwMemLength)) m_nChannels = 8;
	if ((nbp != nbpbuggy) && (dwWowTest + nbp*m_nChannels*256 != dwMemLength))
	{
		if (dwWowTest + nbpbuggy*m_nChannels*256 == dwMemLength) nbp = nbpbuggy;
		else nErr += 8;
	} else
	if ((nbpbuggy2 > nbp) && (dwWowTest + nbpbuggy2*m_nChannels*256 == dwMemLength))
	{
		nbp = nbpbuggy2;
	}
	if ((dwWowTest < 0x600) || (dwWowTest > dwMemLength)) nErr += 8;
	if ((m_nSamples == 15) && (nErr >= 16)) return FALSE;
	// Default settings	
	m_nType = MOD_TYPE_MOD;
	m_nDefaultSpeed = 6;
	m_nDefaultTempo = 125;
	m_nMinPeriod = 14 << 2;
	m_nMaxPeriod = 3424 << 2;
	memcpy(m_szNames, lpStream, 20);
	// Setting channels pan
	for (UINT ich=0; ich<m_nChannels; ich++)
	{
		ChnSettings[ich].nVolume = 64;
		if (gdwSoundSetup & SNDMIX_MAXDEFAULTPAN)
			ChnSettings[ich].nPan = (((ich&3)==1) || ((ich&3)==2)) ? 256 : 0;
		else
			ChnSettings[ich].nPan = (((ich&3)==1) || ((ich&3)==2)) ? 0xC0 : 0x40;
	}
	// Reading channels
	for (UINT ipat=0; ipat<nbp; ipat++)
	{
		if (ipat < MAX_PATTERNS)
		{
			if ((Patterns[ipat] = AllocatePattern(64, m_nChannels)) == NULL) break;
			PatternSize[ipat] = 64;
			if (dwMemPos + m_nChannels*256 >= dwMemLength) break;
			MODCOMMAND *m = Patterns[ipat];
			LPCBYTE p = lpStream + dwMemPos;
			for (UINT j=m_nChannels*64; j; m++,p+=4,j--)
			{
				BYTE A0=p[0], A1=p[1], A2=p[2], A3=p[3];
				UINT n = ((((UINT)A0 & 0x0F) << 8) | (A1));
				if ((n) && (n != 0xFFF)) m->note = GetNoteFromPeriod(n << 2);
				m->instr = ((UINT)A2 >> 4) | (A0 & 0x10);
				m->command = A2 & 0x0F;
				m->param = A3;
				if ((m->command) || (m->param)) ConvertModCommand(m);
			}
		}
		dwMemPos += m_nChannels*256;
	}
	// Reading instruments
	DWORD dwErrCheck = 0;
	for (UINT ismp=1; ismp<=m_nSamples; ismp++) if (Ins[ismp].nLength)
	{
		LPSTR p = (LPSTR)(lpStream+dwMemPos);
		UINT flags = 0;
		if (dwMemPos + 5 >= dwMemLength) break;
		if (!strnicmp(p, "ADPCM", 5))
		{
			flags = 3;
			p += 5;
			dwMemPos += 5;
		}
		DWORD dwSize = ReadSample(&Ins[ismp], flags, p, dwMemLength - dwMemPos);
		if (dwSize)
		{
			dwMemPos += dwSize;
			dwErrCheck++;
		}
	}
#ifdef MODPLUG_TRACKER
	return TRUE;
#else
	return (dwErrCheck) ? TRUE : FALSE;
#endif
}


#ifndef MODPLUG_NO_FILESAVE

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

BOOL CSoundFile::SaveMod(LPCSTR lpszFileName, UINT nPacking)
//----------------------------------------------------------
{
	BYTE insmap[32];
	UINT inslen[32];
	BYTE bTab[32];
	BYTE ord[128];
	FILE *f;

	if ((!m_nChannels) || (!lpszFileName)) return FALSE;
	if ((f = fopen(lpszFileName, "wb")) == NULL) return FALSE;
	memset(ord, 0, sizeof(ord));
	memset(inslen, 0, sizeof(inslen));
	if (m_nInstruments)
	{
		memset(insmap, 0, sizeof(insmap));
		for (UINT i=1; i<32; i++) if (Headers[i])
		{
			for (UINT j=0; j<128; j++) if (Headers[i]->Keyboard[j])
			{
				insmap[i] = Headers[i]->Keyboard[j];
				break;
			}
		}
	} else
	{
		for (UINT i=0; i<32; i++) insmap[i] = (BYTE)i;
	}
	// Writing song name
	fwrite(m_szNames, 20, 1, f);
	// Writing instrument definition
	for (UINT iins=1; iins<=31; iins++)
	{
		MODINSTRUMENT *pins = &Ins[insmap[iins]];
		memcpy(bTab, m_szNames[iins],22);
		inslen[iins] = pins->nLength;
		if (inslen[iins] > 0x1fff0) inslen[iins] = 0x1fff0;
		bTab[22] = inslen[iins] >> 9;
		bTab[23] = inslen[iins] >> 1;
		if (pins->RelativeTone < 0) bTab[24] = 0x08; else
		if (pins->RelativeTone > 0) bTab[24] = 0x07; else
		bTab[24] = (BYTE)XM2MODFineTune(pins->nFineTune);
		bTab[25] = pins->nVolume >> 2;
		bTab[26] = pins->nLoopStart >> 9;
		bTab[27] = pins->nLoopStart >> 1;
		bTab[28] = (pins->nLoopEnd - pins->nLoopStart) >> 9;
		bTab[29] = (pins->nLoopEnd - pins->nLoopStart) >> 1;
		fwrite(bTab, 30, 1, f);
	}
	// Writing number of patterns
	UINT nbp=0, norders=128;
	for (UINT iord=0; iord<128; iord++)
	{
		if (Order[iord] == 0xFF)
		{
			norders = iord;
			break;
		}
		if ((Order[iord] < 0x80) && (nbp<=Order[iord])) nbp = Order[iord]+1;
	}
	bTab[0] = norders;
	bTab[1] = m_nRestartPos;
	fwrite(bTab, 2, 1, f);
	// Writing pattern list
	if (norders) memcpy(ord, Order, norders);
	fwrite(ord, 128, 1, f);
	// Writing signature
	if (m_nChannels == 4)
		lstrcpy((LPSTR)&bTab, "M.K.");
	else
		wsprintf((LPSTR)&bTab, "%luCHN", m_nChannels);
	fwrite(bTab, 4, 1, f);
	// Writing patterns
	for (UINT ipat=0; ipat<nbp; ipat++) if (Patterns[ipat])
	{
		BYTE s[64*4];
		MODCOMMAND *m = Patterns[ipat];
		for (UINT i=0; i<64; i++) if (i < PatternSize[ipat])
		{
			LPBYTE p=s;
			for (UINT c=0; c<m_nChannels; c++,p+=4,m++)
			{
				UINT param = ModSaveCommand(m, FALSE);
				UINT command = param >> 8;
				param &= 0xFF;
				if (command > 0x0F) command = param = 0;
				if ((m->vol >= 0x10) && (m->vol <= 0x50) && (!command) && (!param)) { command = 0x0C; param = m->vol - 0x10; }
				UINT period = m->note;
				if (period)
				{
					if (period < 37) period = 37;
					period -= 37;
					if (period >= 6*12) period = 6*12-1;
					period = ProTrackerPeriodTable[period];
				}
				UINT instr = (m->instr > 31) ? 0 : m->instr;
				p[0] = ((period >> 8) & 0x0F) | (instr & 0x10);
				p[1] = period & 0xFF;
				p[2] = ((instr & 0x0F) << 4) | (command & 0x0F);
				p[3] = param;
			}
			fwrite(s, m_nChannels, 4, f);
		} else
		{
			memset(s, 0, m_nChannels*4);
			fwrite(s, m_nChannels, 4, f);
		}
	}
	// Writing instruments
	for (UINT ismpd=1; ismpd<=31; ismpd++) if (inslen[ismpd])
	{
		MODINSTRUMENT *pins = &Ins[insmap[ismpd]];
		UINT flags = RS_PCM8S;
#ifndef NO_PACKING
		if (!(pins->uFlags & (CHN_16BIT|CHN_STEREO)))
		{
			if ((nPacking) && (CanPackSample((char *)pins->pSample, inslen[ismpd], nPacking)))
			{
				fwrite("ADPCM", 1, 5, f);
				flags = RS_ADPCM4;
			}
		}
#endif
		WriteSample(f, pins, flags, inslen[ismpd]);
	}
	fclose(f);
	return TRUE;
}

#ifdef _MSC_VER
#pragma warning(default:4100)
#endif

#endif // MODPLUG_NO_FILESAVE
