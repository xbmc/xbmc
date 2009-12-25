/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

//////////////////////////////////////////////
// AMS module loader                        //
//////////////////////////////////////////////
#include "stdafx.h"
#include "sndfile.h"

//#pragma warning(disable:4244)

#pragma pack(1)

typedef struct AMSFILEHEADER
{
	char szHeader[7];	// "Extreme"   // changed from CHAR
	BYTE verlo, verhi;	// 0x??,0x01
	BYTE chncfg;
	BYTE samples;
	WORD patterns;
	WORD orders;
	BYTE vmidi;
	WORD extra;
} AMSFILEHEADER;

typedef struct AMSSAMPLEHEADER
{
	DWORD length;
	DWORD loopstart;
	DWORD loopend;
	BYTE finetune_and_pan;
	WORD samplerate;	// C-2 = 8363
	BYTE volume;		// 0-127
	BYTE infobyte;
} AMSSAMPLEHEADER;


#pragma pack()



BOOL CSoundFile::ReadAMS(LPCBYTE lpStream, DWORD dwMemLength)
//-----------------------------------------------------------
{
	BYTE pkinf[MAX_SAMPLES];
	AMSFILEHEADER *pfh = (AMSFILEHEADER *)lpStream;
	DWORD dwMemPos;
	UINT tmp, tmp2;
	
	if ((!lpStream) || (dwMemLength < 1024)) return FALSE;
	if ((pfh->verhi != 0x01) || (strncmp(pfh->szHeader, "Extreme", 7))
	 || (!pfh->patterns) || (!pfh->orders) || (!pfh->samples) || (pfh->samples > MAX_SAMPLES)
	 || (pfh->patterns > MAX_PATTERNS) || (pfh->orders > MAX_ORDERS))
	{
		return ReadAMS2(lpStream, dwMemLength);
	}
	dwMemPos = sizeof(AMSFILEHEADER) + pfh->extra;
	if (dwMemPos + pfh->samples * sizeof(AMSSAMPLEHEADER) + 256 >= dwMemLength) return FALSE;
	m_nType = MOD_TYPE_AMS;
	m_nInstruments = 0;
	m_nChannels = (pfh->chncfg & 0x1F) + 1;
	m_nSamples = pfh->samples;
	for (UINT nSmp=1; nSmp<=m_nSamples; nSmp++, dwMemPos += sizeof(AMSSAMPLEHEADER))
	{
		AMSSAMPLEHEADER *psh = (AMSSAMPLEHEADER *)(lpStream + dwMemPos);
		MODINSTRUMENT *pins = &Ins[nSmp];
		pins->nLength = psh->length;
		pins->nLoopStart = psh->loopstart;
		pins->nLoopEnd = psh->loopend;
		pins->nGlobalVol = 64;
		pins->nVolume = psh->volume << 1;
		pins->nC4Speed = psh->samplerate;
		pins->nPan = (psh->finetune_and_pan & 0xF0);
		if (pins->nPan < 0x80) pins->nPan += 0x10;
		pins->nFineTune = MOD2XMFineTune(psh->finetune_and_pan & 0x0F);
		pins->uFlags = (psh->infobyte & 0x80) ? CHN_16BIT : 0;
		if ((pins->nLoopEnd <= pins->nLength) && (pins->nLoopStart+4 <= pins->nLoopEnd)) pins->uFlags |= CHN_LOOP;
		pkinf[nSmp] = psh->infobyte;
	}
	// Read Song Name
	tmp = lpStream[dwMemPos++];
	if (dwMemPos + tmp + 1 >= dwMemLength) return TRUE;
	tmp2 = (tmp < 32) ? tmp : 31;
	if (tmp2) memcpy(m_szNames[0], lpStream+dwMemPos, tmp2);
	m_szNames[0][tmp2] = 0;
	dwMemPos += tmp;
	// Read sample names
	for (UINT sNam=1; sNam<=m_nSamples; sNam++)
	{
		if (dwMemPos + 32 >= dwMemLength) return TRUE;
		tmp = lpStream[dwMemPos++];
		tmp2 = (tmp < 32) ? tmp : 31;
		if (tmp2) memcpy(m_szNames[sNam], lpStream+dwMemPos, tmp2);
		dwMemPos += tmp;
	}
	// Skip Channel names
	for (UINT cNam=0; cNam<m_nChannels; cNam++)
	{
		if (dwMemPos + 32 >= dwMemLength) return TRUE;
		tmp = lpStream[dwMemPos++];
		dwMemPos += tmp;
	}
	// Read Pattern Names
	m_lpszPatternNames = new char[pfh->patterns * 32];  // changed from CHAR
	if (!m_lpszPatternNames) return TRUE;
	m_nPatternNames = pfh->patterns;
	memset(m_lpszPatternNames, 0, m_nPatternNames * 32);
	for (UINT pNam=0; pNam < m_nPatternNames; pNam++)
	{
		if (dwMemPos + 32 >= dwMemLength) return TRUE;
		tmp = lpStream[dwMemPos++];
		tmp2 = (tmp < 32) ? tmp : 31;
		if (tmp2) memcpy(m_lpszPatternNames+pNam*32, lpStream+dwMemPos, tmp2);
		dwMemPos += tmp;
	}
	// Read Song Comments
	tmp = *((WORD *)(lpStream+dwMemPos));
	dwMemPos += 2;
	if (dwMemPos + tmp >= dwMemLength) return TRUE;
	if (tmp)
	{
		m_lpszSongComments = new char[tmp+1];  // changed from CHAR
		if (!m_lpszSongComments) return TRUE;
		memset(m_lpszSongComments, 0, tmp+1);
		memcpy(m_lpszSongComments, lpStream + dwMemPos, tmp);
		dwMemPos += tmp;
	}
	// Read Order List
	for (UINT iOrd=0; iOrd<pfh->orders; iOrd++, dwMemPos += 2)
	{
		UINT n = *((WORD *)(lpStream+dwMemPos));
		Order[iOrd] = (BYTE)n;
	}
	// Read Patterns
	for (UINT iPat=0; iPat<pfh->patterns; iPat++)
	{
		if (dwMemPos + 4 >= dwMemLength) return TRUE;
		UINT len = *((DWORD *)(lpStream + dwMemPos));
		dwMemPos += 4;
		if ((len >= dwMemLength) || (dwMemPos + len > dwMemLength)) return TRUE;
		PatternSize[iPat] = 64;
		MODCOMMAND *m = AllocatePattern(PatternSize[iPat], m_nChannels);
		if (!m) return TRUE;
		Patterns[iPat] = m;
		const BYTE *p = lpStream + dwMemPos;
		UINT row = 0, i = 0;
		while ((row < PatternSize[iPat]) && (i+2 < len))
		{
			BYTE b0 = p[i++];
			BYTE b1 = p[i++];
			BYTE b2 = 0;
			UINT ch = b0 & 0x3F;
			// Note+Instr
			if (!(b0 & 0x40))
			{
				b2 = p[i++];
				if (ch < m_nChannels)
				{
					if (b1 & 0x7F) m[ch].note = (b1 & 0x7F) + 25;
					m[ch].instr = b2;
				}
				if (b1 & 0x80)
				{
					b0 |= 0x40;
					b1 = p[i++];
				}
			}
			// Effect
			if (b0 & 0x40)
			{
			anothercommand:
				if (b1 & 0x40)
				{
					if (ch < m_nChannels)
					{
						m[ch].volcmd = VOLCMD_VOLUME;
						m[ch].vol = b1 & 0x3F;
					}
				} else
				{
					b2 = p[i++];
					if (ch < m_nChannels)
					{
						UINT cmd = b1 & 0x3F;
						if (cmd == 0x0C)
						{
							m[ch].volcmd = VOLCMD_VOLUME;
							m[ch].vol = b2 >> 1;
						} else
						if (cmd == 0x0E)
						{
							if (!m[ch].command)
							{
								UINT command = CMD_S3MCMDEX;
								UINT param = b2;
								switch(param & 0xF0)
								{
								case 0x00:	if (param & 0x08) { param &= 0x07; param |= 0x90; } else {command=param=0;} break;
								case 0x10:	command = CMD_PORTAMENTOUP; param |= 0xF0; break;
								case 0x20:	command = CMD_PORTAMENTODOWN; param |= 0xF0; break;
								case 0x30:	param = (param & 0x0F) | 0x10; break;
								case 0x40:	param = (param & 0x0F) | 0x30; break;
								case 0x50:	param = (param & 0x0F) | 0x20; break;
								case 0x60:	param = (param & 0x0F) | 0xB0; break;
								case 0x70:	param = (param & 0x0F) | 0x40; break;
								case 0x90:	command = CMD_RETRIG; param &= 0x0F; break;
								case 0xA0:	if (param & 0x0F) { command = CMD_VOLUMESLIDE; param = (param << 4) | 0x0F; } else command=param=0; break;
								case 0xB0:	if (param & 0x0F) { command = CMD_VOLUMESLIDE; param |= 0xF0; } else command=param=0; break;
								}
								m[ch].command = command;
								m[ch].param = param;
							}
						} else
						{
							m[ch].command = cmd;
							m[ch].param = b2;
							ConvertModCommand(&m[ch]);
						}
					}
				}
				if (b1 & 0x80)
				{
					b1 = p[i++];
					if (i <= len) goto anothercommand;
				}
			}
			if (b0 & 0x80)
			{
				row++;
				m += m_nChannels;
			}
		}
		dwMemPos += len;
	}
	// Read Samples
	for (UINT iSmp=1; iSmp<=m_nSamples; iSmp++) if (Ins[iSmp].nLength)
	{
		if (dwMemPos >= dwMemLength - 9) return TRUE;
		UINT flags = (Ins[iSmp].uFlags & CHN_16BIT) ? RS_AMS16 : RS_AMS8;
		dwMemPos += ReadSample(&Ins[iSmp], flags, (LPSTR)(lpStream+dwMemPos), dwMemLength-dwMemPos);
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////
// AMS 2.2 loader

#pragma pack(1)

typedef struct AMS2FILEHEADER
{
	DWORD dwHdr1;		// AMShdr
	WORD wHdr2;
	BYTE b1A;			// 0x1A
	BYTE titlelen;		// 30-bytes max
	CHAR szTitle[30];	// [titlelen]
} AMS2FILEHEADER;

typedef struct AMS2SONGHEADER
{
	WORD version;
	BYTE instruments;
	WORD patterns;
	WORD orders;
	WORD bpm;
	BYTE speed;
	BYTE channels;
	BYTE commands;
	BYTE rows;
	WORD flags;
} AMS2SONGHEADER;

typedef struct AMS2INSTRUMENT
{
	BYTE samples;
	BYTE notemap[NOTE_MAX];
} AMS2INSTRUMENT;

typedef struct AMS2ENVELOPE
{
	BYTE speed;
	BYTE sustain;
	BYTE loopbegin;
	BYTE loopend;
	BYTE points;
	BYTE info[3];
} AMS2ENVELOPE;

typedef struct AMS2SAMPLE
{
	DWORD length;
	DWORD loopstart;
	DWORD loopend;
	WORD frequency;
	BYTE finetune;
	WORD c4speed;
	CHAR transpose;
	BYTE volume;
	BYTE flags;
} AMS2SAMPLE;


#pragma pack()


BOOL CSoundFile::ReadAMS2(LPCBYTE lpStream, DWORD dwMemLength)
//------------------------------------------------------------
{
	AMS2FILEHEADER *pfh = (AMS2FILEHEADER *)lpStream;
	AMS2SONGHEADER *psh;
	DWORD dwMemPos;
	BYTE smpmap[16];
	BYTE packedsamples[MAX_SAMPLES];

	if ((pfh->dwHdr1 != 0x68534D41) || (pfh->wHdr2 != 0x7264)
	 || (pfh->b1A != 0x1A) || (pfh->titlelen > 30)) return FALSE;
	dwMemPos = pfh->titlelen + 8;
	psh = (AMS2SONGHEADER *)(lpStream + dwMemPos);
	if (((psh->version & 0xFF00) != 0x0200) || (!psh->instruments)
	 || (psh->instruments > MAX_INSTRUMENTS) || (!psh->patterns) || (!psh->orders)) return FALSE;
	dwMemPos += sizeof(AMS2SONGHEADER);
	if (pfh->titlelen)
	{
		memcpy(m_szNames, pfh->szTitle, pfh->titlelen);
		m_szNames[0][pfh->titlelen] = 0;
	}
	m_nType = MOD_TYPE_AMS;
	m_nChannels = 32;
	m_nDefaultTempo = psh->bpm >> 8;
	m_nDefaultSpeed = psh->speed;
	m_nInstruments = psh->instruments;
	m_nSamples = 0;
	if (psh->flags & 0x40) m_dwSongFlags |= SONG_LINEARSLIDES;
	for (UINT nIns=1; nIns<=m_nInstruments; nIns++)
	{
		UINT insnamelen = lpStream[dwMemPos];
		CHAR *pinsname = (CHAR *)(lpStream+dwMemPos+1);
		dwMemPos += insnamelen + 1;
		AMS2INSTRUMENT *pins = (AMS2INSTRUMENT *)(lpStream + dwMemPos);
		dwMemPos += sizeof(AMS2INSTRUMENT);
		if (dwMemPos + 1024 >= dwMemLength) return TRUE;
		AMS2ENVELOPE *volenv, *panenv, *pitchenv;
		volenv = (AMS2ENVELOPE *)(lpStream+dwMemPos);
		dwMemPos += 5 + volenv->points*3;
		panenv = (AMS2ENVELOPE *)(lpStream+dwMemPos);
		dwMemPos += 5 + panenv->points*3;
		pitchenv = (AMS2ENVELOPE *)(lpStream+dwMemPos);
		dwMemPos += 5 + pitchenv->points*3;
		INSTRUMENTHEADER *penv = new INSTRUMENTHEADER;
		if (!penv) return TRUE;
		memset(smpmap, 0, sizeof(smpmap));
		memset(penv, 0, sizeof(INSTRUMENTHEADER));
		for (UINT ismpmap=0; ismpmap<pins->samples; ismpmap++)
		{
			if ((ismpmap >= 16) || (m_nSamples+1 >= MAX_SAMPLES)) break;
			m_nSamples++;
			smpmap[ismpmap] = m_nSamples;
		}
		penv->nGlobalVol = 64;
		penv->nPan = 128;
		penv->nPPC = 60;
		Headers[nIns] = penv;
		if (insnamelen)
		{
			if (insnamelen > 31) insnamelen = 31;
			memcpy(penv->name, pinsname, insnamelen);
			penv->name[insnamelen] = 0;
		}
		for (UINT inotemap=0; inotemap<NOTE_MAX; inotemap++)
		{
			penv->NoteMap[inotemap] = inotemap+1;
			penv->Keyboard[inotemap] = smpmap[pins->notemap[inotemap] & 0x0F];
		}
		// Volume Envelope
		{
			UINT pos = 0;
			penv->nVolEnv = (volenv->points > 16) ? 16 : volenv->points;
			penv->nVolSustainBegin = penv->nVolSustainEnd = volenv->sustain;
			penv->nVolLoopStart = volenv->loopbegin;
			penv->nVolLoopEnd = volenv->loopend;
			for (UINT i=0; i<penv->nVolEnv; i++)
			{
				penv->VolEnv[i] = (BYTE)((volenv->info[i*3+2] & 0x7F) >> 1);
				pos += volenv->info[i*3] + ((volenv->info[i*3+1] & 1) << 8);
				penv->VolPoints[i] = (WORD)pos;
			}
		}
		penv->nFadeOut = (((lpStream[dwMemPos+2] & 0x0F) << 8) | (lpStream[dwMemPos+1])) << 3;
		UINT envflags = lpStream[dwMemPos+3];
		if (envflags & 0x01) penv->dwFlags |= ENV_VOLLOOP;
		if (envflags & 0x02) penv->dwFlags |= ENV_VOLSUSTAIN;
		if (envflags & 0x04) penv->dwFlags |= ENV_VOLUME;
		dwMemPos += 5;
		// Read Samples
		for (UINT ismp=0; ismp<pins->samples; ismp++)
		{
			MODINSTRUMENT *psmp = ((ismp < 16) && (smpmap[ismp])) ? &Ins[smpmap[ismp]] : NULL;
			UINT smpnamelen = lpStream[dwMemPos];
			if ((psmp) && (smpnamelen) && (smpnamelen <= 22))
			{
				memcpy(m_szNames[smpmap[ismp]], lpStream+dwMemPos+1, smpnamelen);
			}
			dwMemPos += smpnamelen + 1;
			if (psmp)
			{
				AMS2SAMPLE *pams = (AMS2SAMPLE *)(lpStream+dwMemPos);
				psmp->nGlobalVol = 64;
				psmp->nPan = 128;
				psmp->nLength = pams->length;
				psmp->nLoopStart = pams->loopstart;
				psmp->nLoopEnd = pams->loopend;
				psmp->nC4Speed = pams->c4speed;
				psmp->RelativeTone = pams->transpose;
				psmp->nVolume = pams->volume / 2;
				packedsamples[smpmap[ismp]] = pams->flags;
				if (pams->flags & 0x04) psmp->uFlags |= CHN_16BIT;
				if (pams->flags & 0x08) psmp->uFlags |= CHN_LOOP;
				if (pams->flags & 0x10) psmp->uFlags |= CHN_PINGPONGLOOP;
			}
			dwMemPos += sizeof(AMS2SAMPLE);
		}
	}
	if (dwMemPos + 256 >= dwMemLength) return TRUE;
	// Comments
	{
		UINT composernamelen = lpStream[dwMemPos];
		if (composernamelen)
		{
			m_lpszSongComments = new char[composernamelen+1]; // changed from CHAR
			if (m_lpszSongComments)
			{
				memcpy(m_lpszSongComments, lpStream+dwMemPos+1, composernamelen);
				m_lpszSongComments[composernamelen] = 0;
			}
		}
		dwMemPos += composernamelen + 1;
		// channel names
		for (UINT i=0; i<32; i++)
		{
			UINT chnnamlen = lpStream[dwMemPos];
			if ((chnnamlen) && (chnnamlen < MAX_CHANNELNAME))
			{
				memcpy(ChnSettings[i].szName, lpStream+dwMemPos+1, chnnamlen);
			}
			dwMemPos += chnnamlen + 1;
			if (dwMemPos + chnnamlen + 256 >= dwMemLength) return TRUE;
		}
		// packed comments (ignored)
		UINT songtextlen = *((LPDWORD)(lpStream+dwMemPos));
		dwMemPos += songtextlen;
		if (dwMemPos + 256 >= dwMemLength) return TRUE;
	}
	// Order List
	{
		for (UINT i=0; i<MAX_ORDERS; i++)
		{
			Order[i] = 0xFF;
			if (dwMemPos + 2 >= dwMemLength) return TRUE;
			if (i < psh->orders)
			{
				Order[i] = lpStream[dwMemPos];
				dwMemPos += 2;
			}
		}
	}
	// Pattern Data
	for (UINT ipat=0; ipat<psh->patterns; ipat++)
	{
		if (dwMemPos+8 >= dwMemLength) return TRUE;
		UINT packedlen = *((LPDWORD)(lpStream+dwMemPos));
		UINT numrows = 1 + (UINT)(lpStream[dwMemPos+4]);
		//UINT patchn = 1 + (UINT)(lpStream[dwMemPos+5] & 0x1F);
		//UINT patcmds = 1 + (UINT)(lpStream[dwMemPos+5] >> 5);
		UINT patnamlen = lpStream[dwMemPos+6];
		dwMemPos += 4;
		if ((ipat < MAX_PATTERNS) && (packedlen < dwMemLength-dwMemPos) && (numrows >= 8))
		{
			if ((patnamlen) && (patnamlen < MAX_PATTERNNAME))
			{
				char s[MAX_PATTERNNAME]; // changed from CHAR
				memcpy(s, lpStream+dwMemPos+3, patnamlen);
				s[patnamlen] = 0;
				SetPatternName(ipat, s);
			}
			PatternSize[ipat] = numrows;
			Patterns[ipat] = AllocatePattern(numrows, m_nChannels);
			if (!Patterns[ipat]) return TRUE;
			// Unpack Pattern Data
			LPCBYTE psrc = lpStream + dwMemPos;
			UINT pos = 3 + patnamlen;
			UINT row = 0;
			while ((pos < packedlen) && (row < numrows))
			{
				MODCOMMAND *m = Patterns[ipat] + row * m_nChannels;
				UINT byte1 = psrc[pos++];
				UINT ch = byte1 & 0x1F;
				// Read Note + Instr
				if (!(byte1 & 0x40))
				{
					UINT byte2 = psrc[pos++];
					UINT note = byte2 & 0x7F;
					if (note) m[ch].note = (note > 1) ? (note-1) : 0xFF;
					m[ch].instr = psrc[pos++];
					// Read Effect
					while (byte2 & 0x80)
					{
						byte2 = psrc[pos++];
						if (byte2 & 0x40)
						{
							m[ch].volcmd = VOLCMD_VOLUME;
							m[ch].vol = byte2 & 0x3F;
						} else
						{
							UINT command = byte2 & 0x3F;
							UINT param = psrc[pos++];
							if (command == 0x0C)
							{
								m[ch].volcmd = VOLCMD_VOLUME;
								m[ch].vol = param / 2;
							} else
							if (command < 0x10)
							{
								m[ch].command = command;
								m[ch].param = param;
								ConvertModCommand(&m[ch]);
							} else
							{
								// TODO: AMS effects
							}
						}
					}
				}
				if (byte1 & 0x80) row++;
			}
		}
		dwMemPos += packedlen;
	}
	// Read Samples
	for (UINT iSmp=1; iSmp<=m_nSamples; iSmp++) if (Ins[iSmp].nLength)
	{
		if (dwMemPos >= dwMemLength - 9) return TRUE;
		UINT flags;
		if (packedsamples[iSmp] & 0x03)
		{
			flags = (Ins[iSmp].uFlags & CHN_16BIT) ? RS_AMS16 : RS_AMS8;
		} else
		{
			flags = (Ins[iSmp].uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S;
		}
		dwMemPos += ReadSample(&Ins[iSmp], flags, (LPSTR)(lpStream+dwMemPos), dwMemLength-dwMemPos);
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////
// AMS Sample unpacking

void AMSUnpack(const char *psrc, UINT inputlen, char *pdest, UINT dmax, char packcharacter)
{
	UINT tmplen = dmax;
	signed char *amstmp = new signed char[tmplen];
	
	if (!amstmp) return;
	// Unpack Loop
	{
		signed char *p = amstmp;
		UINT i=0, j=0;
		while ((i < inputlen) && (j < tmplen))
		{
			signed char ch = psrc[i++];
			if (ch == packcharacter)
			{
				BYTE ch2 = psrc[i++];
				if (ch2)
				{
					ch = psrc[i++];
					while (ch2--)
					{
						p[j++] = ch;
						if (j >= tmplen) break;
					}
				} else p[j++] = packcharacter;
			} else p[j++] = ch;
		}
	}
	// Bit Unpack Loop
	{
		signed char *p = amstmp;
		UINT bitcount = 0x80, dh;
		UINT k=0;
		for (UINT i=0; i<dmax; i++)
		{
			BYTE al = *p++;
			dh = 0;
			for (UINT count=0; count<8; count++)
			{
				UINT bl = al & bitcount;
				bl = ((bl|(bl<<8)) >> ((dh+8-count) & 7)) & 0xFF;
				bitcount = ((bitcount|(bitcount<<8)) >> 1) & 0xFF;
				pdest[k++] |= bl;
				if (k >= dmax)
				{
					k = 0;
					dh++;
				}
			}
			bitcount = ((bitcount|(bitcount<<8)) >> dh) & 0xFF;
		}
	}
	// Delta Unpack
	{
		signed char old = 0;
		for (UINT i=0; i<dmax; i++)
		{
			int pos = ((LPBYTE)pdest)[i];
			if ((pos != 128) && (pos & 0x80)) pos = -(pos & 0x7F);
			old -= (signed char)pos;
			pdest[i] = old;
		}
	}
	delete amstmp;
}

