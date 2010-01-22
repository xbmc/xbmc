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

//////////////////////////////////////////////////////
// ScreamTracker S3M file support

typedef struct tagS3MSAMPLESTRUCT
{
	BYTE type;
	CHAR dosname[12];
	BYTE hmem;
	WORD memseg;
	DWORD length;
	DWORD loopbegin;
	DWORD loopend;
	BYTE vol;
	BYTE bReserved;
	BYTE pack;
	BYTE flags;
	DWORD finetune;
	DWORD dwReserved;
	WORD intgp;
	WORD int512;
	DWORD lastused;
	CHAR name[28];
	CHAR scrs[4];
} S3MSAMPLESTRUCT;


typedef struct tagS3MFILEHEADER
{
	CHAR name[28];
	BYTE b1A;
	BYTE type;
	WORD reserved1;
	WORD ordnum;
	WORD insnum;
	WORD patnum;
	WORD flags;
	WORD cwtv;
	WORD version;
	DWORD scrm;	// "SCRM" = 0x4D524353
	BYTE globalvol;
	BYTE speed;
	BYTE tempo;
	BYTE mastervol;
	BYTE ultraclicks;
	BYTE panning_present;
	BYTE reserved2[8];
	WORD special;
	BYTE channels[32];
} S3MFILEHEADER;


void CSoundFile::S3MConvert(MODCOMMAND *m, BOOL bIT) const
//--------------------------------------------------------
{
	UINT command = m->command;
	UINT param = m->param;
	switch (command + 0x40)
	{
	case 'A':	command = CMD_SPEED; break;
	case 'B':	command = CMD_POSITIONJUMP; break;
	case 'C':	command = CMD_PATTERNBREAK; if (!bIT) param = (param >> 4) * 10 + (param & 0x0F); break;
	case 'D':	command = CMD_VOLUMESLIDE; break;
	case 'E':	command = CMD_PORTAMENTODOWN; break;
	case 'F':	command = CMD_PORTAMENTOUP; break;
	case 'G':	command = CMD_TONEPORTAMENTO; break;
	case 'H':	command = CMD_VIBRATO; break;
	case 'I':	command = CMD_TREMOR; break;
	case 'J':	command = CMD_ARPEGGIO; break;
	case 'K':	command = CMD_VIBRATOVOL; break;
	case 'L':	command = CMD_TONEPORTAVOL; break;
	case 'M':	command = CMD_CHANNELVOLUME; break;
	case 'N':	command = CMD_CHANNELVOLSLIDE; break;
	case 'O':	command = CMD_OFFSET; break;
	case 'P':	command = CMD_PANNINGSLIDE; break;
	case 'Q':	command = CMD_RETRIG; break;
	case 'R':	command = CMD_TREMOLO; break;
	case 'S':	command = CMD_S3MCMDEX; break;
	case 'T':	command = CMD_TEMPO; break;
	case 'U':	command = CMD_FINEVIBRATO; break;
	case 'V':	command = CMD_GLOBALVOLUME; break;
	case 'W':	command = CMD_GLOBALVOLSLIDE; break;
	case 'X':	command = CMD_PANNING8; break;
	case 'Y':	command = CMD_PANBRELLO; break;
	case 'Z':	command = CMD_MIDI; break;
	default:	command = 0;
	}
	m->command = command;
	m->param = param;
}


void CSoundFile::S3MSaveConvert(UINT *pcmd, UINT *pprm, BOOL bIT) const
//---------------------------------------------------------------------
{
	UINT command = *pcmd;
	UINT param = *pprm;
	switch(command)
	{
	case CMD_SPEED:				command = 'A'; break;
	case CMD_POSITIONJUMP:		command = 'B'; break;
	case CMD_PATTERNBREAK:		command = 'C'; if (!bIT) param = ((param / 10) << 4) + (param % 10); break;
	case CMD_VOLUMESLIDE:		command = 'D'; break;
	case CMD_PORTAMENTODOWN:	command = 'E'; if ((param >= 0xE0) && (m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))) param = 0xDF; break;
	case CMD_PORTAMENTOUP:		command = 'F'; if ((param >= 0xE0) && (m_nType & (MOD_TYPE_MOD|MOD_TYPE_XM))) param = 0xDF; break;
	case CMD_TONEPORTAMENTO:	command = 'G'; break;
	case CMD_VIBRATO:			command = 'H'; break;
	case CMD_TREMOR:			command = 'I'; break;
	case CMD_ARPEGGIO:			command = 'J'; break;
	case CMD_VIBRATOVOL:		command = 'K'; break;
	case CMD_TONEPORTAVOL:		command = 'L'; break;
	case CMD_CHANNELVOLUME:		command = 'M'; break;
	case CMD_CHANNELVOLSLIDE:	command = 'N'; break;
	case CMD_OFFSET:			command = 'O'; break;
	case CMD_PANNINGSLIDE:		command = 'P'; break;
	case CMD_RETRIG:			command = 'Q'; break;
	case CMD_TREMOLO:			command = 'R'; break;
	case CMD_S3MCMDEX:			command = 'S'; break;
	case CMD_TEMPO:				command = 'T'; break;
	case CMD_FINEVIBRATO:		command = 'U'; break;
	case CMD_GLOBALVOLUME:		command = 'V'; break;
	case CMD_GLOBALVOLSLIDE:	command = 'W'; break;
	case CMD_PANNING8:			
		command = 'X';
		if ((bIT) && (m_nType != MOD_TYPE_IT) && (m_nType != MOD_TYPE_XM))
		{
			if (param == 0xA4) { command = 'S'; param = 0x91; }	else
			if (param <= 0x80) { param <<= 1; if (param > 255) param = 255; } else
			command = param = 0;
		} else
		if ((!bIT) && ((m_nType == MOD_TYPE_IT) || (m_nType == MOD_TYPE_XM)))
		{
			param >>= 1;
		}
		break;
	case CMD_PANBRELLO:			command = 'Y'; break;
	case CMD_MIDI:				command = 'Z'; break;
	case CMD_XFINEPORTAUPDOWN:
		if (param & 0x0F) switch(param & 0xF0)
		{
		case 0x10:	command = 'F'; param = (param & 0x0F) | 0xE0; break;
		case 0x20:	command = 'E'; param = (param & 0x0F) | 0xE0; break;
		case 0x90:	command = 'S'; break;
		default:	command = param = 0;
		} else command = param = 0;
		break;
	case CMD_MODCMDEX:
		command = 'S';
		switch(param & 0xF0)
		{
		case 0x00:	command = param = 0; break;
		case 0x10:	command = 'F'; param |= 0xF0; break;
		case 0x20:	command = 'E'; param |= 0xF0; break;
		case 0x30:	param = (param & 0x0F) | 0x10; break;
		case 0x40:	param = (param & 0x0F) | 0x30; break;
		case 0x50:	param = (param & 0x0F) | 0x20; break;
		case 0x60:	param = (param & 0x0F) | 0xB0; break;
		case 0x70:	param = (param & 0x0F) | 0x40; break;
		case 0x90:	command = 'Q'; param &= 0x0F; break;
		case 0xA0:	if (param & 0x0F) { command = 'D'; param = (param << 4) | 0x0F; } else command=param=0; break;
		case 0xB0:	if (param & 0x0F) { command = 'D'; param |= 0xF0; } else command=param=0; break;
		}
		break;
	default:	command = param = 0;
	}
	command &= ~0x40;
	*pcmd = command;
	*pprm = param;
}


BOOL CSoundFile::ReadS3M(const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
	UINT insnum,patnum,nins,npat;
	DWORD insfile[128];
	WORD ptr[256];
	BYTE s[1024];
	DWORD dwMemPos;
	BYTE insflags[128], inspack[128];
	S3MFILEHEADER psfh = *(S3MFILEHEADER *)lpStream;

	psfh.reserved1 = bswapLE16(psfh.reserved1);
	psfh.ordnum = bswapLE16(psfh.ordnum);
	psfh.insnum = bswapLE16(psfh.insnum);
	psfh.patnum = bswapLE16(psfh.patnum);
	psfh.flags = bswapLE16(psfh.flags);
	psfh.cwtv = bswapLE16(psfh.cwtv);
	psfh.version = bswapLE16(psfh.version);
	psfh.scrm = bswapLE32(psfh.scrm);
	psfh.special = bswapLE16(psfh.special);

	if ((!lpStream) || (dwMemLength <= sizeof(S3MFILEHEADER)+sizeof(S3MSAMPLESTRUCT)+64)) return FALSE;
	if (psfh.scrm != 0x4D524353) return FALSE;
	dwMemPos = 0x60;
	m_nType = MOD_TYPE_S3M;
	memset(m_szNames,0,sizeof(m_szNames));
	memcpy(m_szNames[0], psfh.name, 28);
	// Speed
	m_nDefaultSpeed = psfh.speed;
	if (m_nDefaultSpeed < 1) m_nDefaultSpeed = 6;
	if (m_nDefaultSpeed > 0x1F) m_nDefaultSpeed = 0x1F;
	// Tempo
	m_nDefaultTempo = psfh.tempo;
	if (m_nDefaultTempo < 40) m_nDefaultTempo = 40;
	if (m_nDefaultTempo > 240) m_nDefaultTempo = 240;
	// Global Volume
	m_nDefaultGlobalVolume = psfh.globalvol << 2;
	if ((!m_nDefaultGlobalVolume) || (m_nDefaultGlobalVolume > 256)) m_nDefaultGlobalVolume = 256;
	m_nSongPreAmp = psfh.mastervol & 0x7F;
	// Channels
	m_nChannels = 4;
	for (UINT ich=0; ich<32; ich++)
	{
		ChnSettings[ich].nPan = 128;
		ChnSettings[ich].nVolume = 64;

		ChnSettings[ich].dwFlags = CHN_MUTE;
		if (psfh.channels[ich] != 0xFF)
		{
			m_nChannels = ich+1;
			UINT b = psfh.channels[ich] & 0x0F;
			ChnSettings[ich].nPan = (b & 8) ? 0xC0 : 0x40;
			ChnSettings[ich].dwFlags = 0;
		}
	}
	if (m_nChannels < 4) m_nChannels = 4;
	if ((psfh.cwtv < 0x1320) || (psfh.flags & 0x40)) m_dwSongFlags |= SONG_FASTVOLSLIDES;
	// Reading pattern order
	UINT iord = psfh.ordnum;
	if (iord<1) iord = 1;
	if (iord > MAX_ORDERS) iord = MAX_ORDERS;
	if (iord)
	{
		memcpy(Order, lpStream+dwMemPos, iord);
		dwMemPos += iord;
	}
	if ((iord & 1) && (lpStream[dwMemPos] == 0xFF)) dwMemPos++;
	// Reading file pointers
	insnum = nins = psfh.insnum;
	if (insnum >= MAX_SAMPLES) insnum = MAX_SAMPLES-1;
	m_nSamples = insnum;
	patnum = npat = psfh.patnum;
	if (patnum > MAX_PATTERNS) patnum = MAX_PATTERNS;
	memset(ptr, 0, sizeof(ptr));
	if (nins+npat)
	{
		memcpy(ptr, lpStream+dwMemPos, 2*(nins+npat));
		dwMemPos += 2*(nins+npat);
		for (UINT j = 0; j < (nins+npat); ++j) {
		        ptr[j] = bswapLE16(ptr[j]);
		}
		if (psfh.panning_present == 252)
		{
			const BYTE *chnpan = lpStream+dwMemPos;
			for (UINT i=0; i<32; i++) if (chnpan[i] & 0x20)
			{
				ChnSettings[i].nPan = ((chnpan[i] & 0x0F) << 4) + 8;
			}
		}
	}
	if (!m_nChannels) return TRUE;
	// Reading instrument headers
	memset(insfile, 0, sizeof(insfile));
	for (UINT iSmp=1; iSmp<=insnum; iSmp++)
	{
		UINT nInd = ((DWORD)ptr[iSmp-1])*16;
		if ((!nInd) || (nInd + 0x50 > dwMemLength)) continue;
		memcpy(s, lpStream+nInd, 0x50);
		memcpy(Ins[iSmp].name, s+1, 12);
		insflags[iSmp-1] = s[0x1F];
		inspack[iSmp-1] = s[0x1E];
		s[0x4C] = 0;
		lstrcpy(m_szNames[iSmp], (LPCSTR)&s[0x30]);
		if ((s[0]==1) && (s[0x4E]=='R') && (s[0x4F]=='S'))
		{
			UINT j = bswapLE32(*((LPDWORD)(s+0x10)));
			if (j > MAX_SAMPLE_LENGTH) j = MAX_SAMPLE_LENGTH;
			if (j < 4) j = 0;
			Ins[iSmp].nLength = j;
			j = bswapLE32(*((LPDWORD)(s+0x14)));
			if (j >= Ins[iSmp].nLength) j = Ins[iSmp].nLength - 1;
			Ins[iSmp].nLoopStart = j;
			j = bswapLE32(*((LPDWORD)(s+0x18)));
			if (j > MAX_SAMPLE_LENGTH) j = MAX_SAMPLE_LENGTH;
			if (j < 4) j = 0;
			if (j > Ins[iSmp].nLength) j = Ins[iSmp].nLength;
			Ins[iSmp].nLoopEnd = j;
			j = s[0x1C];
			if (j > 64) j = 64;
			Ins[iSmp].nVolume = j << 2;
			Ins[iSmp].nGlobalVol = 64;
			if (s[0x1F]&1) Ins[iSmp].uFlags |= CHN_LOOP;
			j = bswapLE32(*((LPDWORD)(s+0x20)));
			if (!j) j = 8363;
			if (j < 1024) j = 1024;
			Ins[iSmp].nC4Speed = j;
			insfile[iSmp] = ((DWORD)bswapLE16(*((LPWORD)(s+0x0E)))) << 4;
			insfile[iSmp] += ((DWORD)(BYTE)s[0x0D]) << 20;
			if (insfile[iSmp] > dwMemLength) insfile[iSmp] &= 0xFFFF;
			if ((Ins[iSmp].nLoopStart >= Ins[iSmp].nLoopEnd) || (Ins[iSmp].nLoopEnd - Ins[iSmp].nLoopStart < 8))
				Ins[iSmp].nLoopStart = Ins[iSmp].nLoopEnd = 0;
			Ins[iSmp].nPan = 0x80;
		}
	}
	// Reading patterns
	for (UINT iPat=0; iPat<patnum; iPat++)
	{
		UINT nInd = ((DWORD)ptr[nins+iPat]) << 4;
		if (nInd + 0x40 > dwMemLength) continue;
		WORD len = bswapLE16(*((WORD *)(lpStream+nInd)));
		nInd += 2;
		PatternSize[iPat] = 64;
		if ((!len) || (nInd + len > dwMemLength - 6)
		 || ((Patterns[iPat] = AllocatePattern(64, m_nChannels)) == NULL)) continue;
		LPBYTE src = (LPBYTE)(lpStream+nInd);
		// Unpacking pattern
		MODCOMMAND *p = Patterns[iPat];
		UINT row = 0;
		UINT j = 0;
		while (j < len)
		{
			BYTE b = src[j++];
			if (!b)
			{
				if (++row >= 64) break;
			} else
			{
				UINT chn = b & 0x1F;
				if (chn < m_nChannels)
				{
					MODCOMMAND *m = &p[row*m_nChannels+chn];
					if (b & 0x20)
					{
						m->note = src[j++];
						if (m->note < 0xF0) m->note = (m->note & 0x0F) + 12*(m->note >> 4) + 13;
						else if (m->note == 0xFF) m->note = 0;
						m->instr = src[j++];
					}
					if (b & 0x40)
					{
						UINT vol = src[j++];
						if ((vol >= 128) && (vol <= 192))
						{
							vol -= 128;
							m->volcmd = VOLCMD_PANNING;
						} else
						{
							if (vol > 64) vol = 64;
							m->volcmd = VOLCMD_VOLUME;
						}
						m->vol = vol;
					}
					if (b & 0x80)
					{
						m->command = src[j++];
						m->param = src[j++];
						if (m->command) S3MConvert(m, FALSE);
					}
				} else
				{
					if (b & 0x20) j += 2;
					if (b & 0x40) j++;
					if (b & 0x80) j += 2;
				}
				if (j >= len) break;
			}
		}
	}
	// Reading samples
	for (UINT iRaw=1; iRaw<=insnum; iRaw++) if ((Ins[iRaw].nLength) && (insfile[iRaw]))
	{
		UINT flags = (psfh.version == 1) ? RS_PCM8S : RS_PCM8U;
		if (insflags[iRaw-1] & 4) flags += 5;
		if (insflags[iRaw-1] & 2) flags |= RSF_STEREO;
		if (inspack[iRaw-1] == 4) flags = RS_ADPCM4;
		dwMemPos = insfile[iRaw];
		dwMemPos += ReadSample(&Ins[iRaw], flags, (LPSTR)(lpStream + dwMemPos), dwMemLength - dwMemPos);
	}
	m_nMinPeriod = 64;
	m_nMaxPeriod = 32767;
	if (psfh.flags & 0x10) m_dwSongFlags |= SONG_AMIGALIMITS;
	return TRUE;
}


#ifndef MODPLUG_NO_FILESAVE

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

static BYTE S3MFiller[16] =
{
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};


BOOL CSoundFile::SaveS3M(LPCSTR lpszFileName, UINT nPacking)
//----------------------------------------------------------
{
	FILE *f;
	BYTE header[0x60];
	UINT nbo,nbi,nbp,i;
	WORD patptr[128];
	WORD insptr[128];
	BYTE buffer[5*1024];
	S3MSAMPLESTRUCT insex[128];

	if ((!m_nChannels) || (!lpszFileName)) return FALSE;
	if ((f = fopen(lpszFileName, "wb")) == NULL) return FALSE;
	// Writing S3M header
	memset(header, 0, sizeof(header));
	memset(insex, 0, sizeof(insex));
	memcpy(header, m_szNames[0], 0x1C);
	header[0x1B] = 0;
	header[0x1C] = 0x1A;
	header[0x1D] = 0x10;
	nbo = (GetNumPatterns() + 15) & 0xF0;
	if (!nbo) nbo = 16;
	header[0x20] = nbo & 0xFF;
	header[0x21] = nbo >> 8;
	nbi = m_nInstruments;
	if (!nbi) nbi = m_nSamples;
	if (nbi > 99) nbi = 99;
	header[0x22] = nbi & 0xFF;
	header[0x23] = nbi >> 8;
	nbp = 0;
	for (i=0; Patterns[i]; i++) { nbp = i+1; if (nbp >= MAX_PATTERNS) break; }
	for (i=0; i<MAX_ORDERS; i++) if ((Order[i] < MAX_PATTERNS) && (Order[i] >= nbp)) nbp = Order[i] + 1;
	header[0x24] = nbp & 0xFF;
	header[0x25] = nbp >> 8;
	if (m_dwSongFlags & SONG_FASTVOLSLIDES) header[0x26] |= 0x40;
	if ((m_nMaxPeriod < 20000) || (m_dwSongFlags & SONG_AMIGALIMITS)) header[0x26] |= 0x10;
	header[0x28] = 0x20;
	header[0x29] = 0x13;
	header[0x2A] = 0x02; // Version = 1 => Signed samples
	header[0x2B] = 0x00;
	header[0x2C] = 'S';
	header[0x2D] = 'C';
	header[0x2E] = 'R';
	header[0x2F] = 'M';
	header[0x30] = m_nDefaultGlobalVolume >> 2;
	header[0x31] = m_nDefaultSpeed;
	header[0x32] = m_nDefaultTempo;
	header[0x33] = ((m_nSongPreAmp < 0x20) ? 0x20 : m_nSongPreAmp) | 0x80;	// Stereo
	header[0x35] = 0xFC;
	for (i=0; i<32; i++)
	{
		if (i < m_nChannels)
		{
			UINT tmp = (i & 0x0F) >> 1;
			header[0x40+i] = (i & 0x10) | ((i & 1) ? 8+tmp : tmp);
		} else header[0x40+i] = 0xFF;
	}
	fwrite(header, 0x60, 1, f);
	fwrite(Order, nbo, 1, f);
	memset(patptr, 0, sizeof(patptr));
	memset(insptr, 0, sizeof(insptr));
	UINT ofs0 = 0x60 + nbo;
	UINT ofs1 = ((0x60 + nbo + nbi*2 + nbp*2 + 15) & 0xFFF0) + 0x20;
	UINT ofs = ofs1;

	for (i=0; i<nbi; i++) insptr[i] = (WORD)((ofs + i*0x50) / 16);
	for (i=0; i<nbp; i++) patptr[i] = (WORD)((ofs + nbi*0x50) / 16);
	fwrite(insptr, nbi, 2, f);
	fwrite(patptr, nbp, 2, f);
	if (header[0x35] == 0xFC)
	{
		BYTE chnpan[32];
		for (i=0; i<32; i++)
		{
			chnpan[i] = 0x20 | (ChnSettings[i].nPan >> 4);
		}
		fwrite(chnpan, 0x20, 1, f);
	}
	if ((nbi*2+nbp*2) & 0x0F)
	{
		fwrite(S3MFiller, 0x10 - ((nbi*2+nbp*2) & 0x0F), 1, f);
	}
	ofs1 = ftell(f);
	fwrite(insex, nbi, 0x50, f);
	// Packing patterns
	ofs += nbi*0x50;
	for (i=0; i<nbp; i++)
	{
		WORD len = 64;
		memset(buffer, 0, sizeof(buffer));
		patptr[i] = ofs / 16;
		if (Patterns[i])
		{
			len = 2;
			MODCOMMAND *p = Patterns[i];
			for (int row=0; row<64; row++) if (row < PatternSize[i])
			{
				for (UINT j=0; j<m_nChannels; j++)
				{
					UINT b = j;
					MODCOMMAND *m = &p[row*m_nChannels+j];
					UINT note = m->note;
					UINT volcmd = m->volcmd;
					UINT vol = m->vol;
					UINT command = m->command;
					UINT param = m->param;

					if ((note) || (m->instr)) b |= 0x20;
					if (!note) note = 0xFF; else
					if (note >= 0xFE) note = 0xFE; else
					if (note < 13) note = 0; else note -= 13;
					if (note < 0xFE) note = (note % 12) + ((note / 12) << 4);
					if (command == CMD_VOLUME)
					{
						command = 0;
						if (param > 64) param = 64;
						volcmd = VOLCMD_VOLUME;
						vol = param;
					}
					if (volcmd == VOLCMD_VOLUME) b |= 0x40; else
					if (volcmd == VOLCMD_PANNING) { vol |= 0x80; b |= 0x40; }
					if (command)
					{
						S3MSaveConvert(&command, &param, FALSE);
						if (command) b |= 0x80;
					}
					if (b & 0xE0)
					{
						buffer[len++] = b;
						if (b & 0x20)
						{
							buffer[len++] = note;
							buffer[len++] = m->instr;
						}
						if (b & 0x40)
						{
							buffer[len++] = vol;
						}
						if (b & 0x80)
						{
							buffer[len++] = command;
							buffer[len++] = param;
						}
						if (len > sizeof(buffer) - 20) break;
					}
				}
				buffer[len++] = 0;
				if (len > sizeof(buffer) - 20) break;
			}
		}
		buffer[0] = (len - 2) & 0xFF;
		buffer[1] = (len - 2) >> 8;
		len = (len+15) & (~0x0F);
		fwrite(buffer, len, 1, f);
		ofs += len;
	}
	// Writing samples
	for (i=1; i<=nbi; i++)
	{
		MODINSTRUMENT *pins = &Ins[i];
		if (m_nInstruments)
		{
			pins = Ins;
			if (Headers[i])
			{
				for (UINT j=0; j<128; j++)
				{
					UINT n = Headers[i]->Keyboard[j];
					if ((n) && (n < MAX_INSTRUMENTS))
					{
						pins = &Ins[n];
						break;
					}
				}
			}
		}
		memcpy(insex[i-1].dosname, pins->name, 12);
		memcpy(insex[i-1].name, m_szNames[i], 28);
		memcpy(insex[i-1].scrs, "SCRS", 4);
		insex[i-1].hmem = (BYTE)((DWORD)ofs >> 20);
		insex[i-1].memseg = (WORD)((DWORD)ofs >> 4);
		if (pins->pSample)
		{
			insex[i-1].type = 1;
			insex[i-1].length = pins->nLength;
			insex[i-1].loopbegin = pins->nLoopStart;
			insex[i-1].loopend = pins->nLoopEnd;
			insex[i-1].vol = pins->nVolume / 4;
			insex[i-1].flags = (pins->uFlags & CHN_LOOP) ? 1 : 0;
			if (pins->nC4Speed)
				insex[i-1].finetune = pins->nC4Speed;
			else
				insex[i-1].finetune = TransposeToFrequency(pins->RelativeTone, pins->nFineTune);
			UINT flags = RS_PCM8U;
#ifndef NO_PACKING
			if (nPacking)
			{
				if ((!(pins->uFlags & (CHN_16BIT|CHN_STEREO)))
				 && (CanPackSample((char *)pins->pSample, pins->nLength, nPacking)))
				{
					insex[i-1].pack = 4;
					flags = RS_ADPCM4;
				}
			} else
#endif // NO_PACKING
			{
				if (pins->uFlags & CHN_16BIT)
				{
					insex[i-1].flags |= 4;
					flags = RS_PCM16U;
				}
				if (pins->uFlags & CHN_STEREO)
				{
					insex[i-1].flags |= 2;
					flags = (pins->uFlags & CHN_16BIT) ? RS_STPCM16U : RS_STPCM8U;
				}
			}
			DWORD len = WriteSample(f, pins, flags);
			if (len & 0x0F)
			{
				fwrite(S3MFiller, 0x10 - (len & 0x0F), 1, f);
			}
			ofs += (len + 15) & (~0x0F);
		} else
		{
			insex[i-1].length = 0;
		}
	}
	// Updating parapointers
	fseek(f, ofs0, SEEK_SET);
	fwrite(insptr, nbi, 2, f);
	fwrite(patptr, nbp, 2, f);
	fseek(f, ofs1, SEEK_SET);
	fwrite(insex, 0x50, nbi, f);
	fclose(f);
	return TRUE;
}

#ifdef _MSC_VER
#pragma warning(default:4100)
#endif

#endif // MODPLUG_NO_FILESAVE

