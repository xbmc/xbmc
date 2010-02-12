/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

///////////////////////////////////////////////////
//
// AMF module loader
//
// There is 2 types of AMF files:
// - ASYLUM Music Format
// - Advanced Music Format(DSM)
//
///////////////////////////////////////////////////
#include "stdafx.h"
#include "sndfile.h"

//#define AMFLOG

//#pragma warning(disable:4244)

#pragma pack(1)

typedef struct _AMFFILEHEADER
{
	UCHAR szAMF[3];
	UCHAR version;
	CHAR title[32];
	UCHAR numsamples;
	UCHAR numorders;
	USHORT numtracks;
	UCHAR numchannels;
} AMFFILEHEADER;

typedef struct _AMFSAMPLE
{
	UCHAR type;
	CHAR  samplename[32];
	CHAR  filename[13];
	ULONG offset;
	ULONG length;
	USHORT c2spd;
	UCHAR volume;
} AMFSAMPLE;


#pragma pack()


#ifdef AMFLOG
extern void Log(LPCSTR, ...);
#endif

VOID AMF_Unpack(MODCOMMAND *pPat, const BYTE *pTrack, UINT nRows, UINT nChannels)
//-------------------------------------------------------------------------------
{
	UINT lastinstr = 0;
	UINT nTrkSize = bswapLE16(*(USHORT *)pTrack);
	nTrkSize += (UINT)pTrack[2] << 16;
	pTrack += 3;
	while (nTrkSize--)
	{
		UINT row = pTrack[0];
		UINT cmd = pTrack[1];
		UINT arg = pTrack[2];
		if (row >= nRows) break;
		MODCOMMAND *m = pPat + row * nChannels;
		if (cmd < 0x7F) // note+vol
		{
			m->note = cmd+1;
			if (!m->instr) m->instr = lastinstr;
			m->volcmd = VOLCMD_VOLUME;
			m->vol = arg;
		} else
		if (cmd == 0x7F) // duplicate row
		{
			signed char rdelta = (signed char)arg;
			int rowsrc = (int)row + (int)rdelta;
			if ((rowsrc >= 0) && (rowsrc < (int)nRows)) memcpy(m, &pPat[rowsrc*nChannels],sizeof(pPat[rowsrc*nChannels]));
		} else
		if (cmd == 0x80) // instrument
		{
			m->instr = arg+1;
			lastinstr = m->instr;
		} else
		if (cmd == 0x83) // volume
		{
			m->volcmd = VOLCMD_VOLUME;
			m->vol = arg;
		} else
		// effect
		{
			UINT command = cmd & 0x7F;
			UINT param = arg;
			switch(command)
			{
			// 0x01: Set Speed
			case 0x01:	command = CMD_SPEED; break;
			// 0x02: Volume Slide
			// 0x0A: Tone Porta + Vol Slide
			// 0x0B: Vibrato + Vol Slide
			case 0x02:	command = CMD_VOLUMESLIDE;
			case 0x0A:	if (command == 0x0A) command = CMD_TONEPORTAVOL;
			case 0x0B:	if (command == 0x0B) command = CMD_VIBRATOVOL;
						if (param & 0x80) param = (-(signed char)param)&0x0F;
						else param = (param&0x0F)<<4;
						break;
			// 0x04: Porta Up/Down
			case 0x04:	if (param & 0x80) { command = CMD_PORTAMENTOUP; param = (-(signed char)param)&0x7F; }
						else { command = CMD_PORTAMENTODOWN; } break;
			// 0x06: Tone Portamento
			case 0x06:	command = CMD_TONEPORTAMENTO; break;
			// 0x07: Tremor
			case 0x07:	command = CMD_TREMOR; break;
			// 0x08: Arpeggio
			case 0x08:	command = CMD_ARPEGGIO; break;
			// 0x09: Vibrato
			case 0x09:	command = CMD_VIBRATO; break;
			// 0x0C: Pattern Break
			case 0x0C:	command = CMD_PATTERNBREAK; break;
			// 0x0D: Position Jump
			case 0x0D:	command = CMD_POSITIONJUMP; break;
			// 0x0F: Retrig
			case 0x0F:	command = CMD_RETRIG; break;
			// 0x10: Offset
			case 0x10:	command = CMD_OFFSET; break;
			// 0x11: Fine Volume Slide
			case 0x11:	if (param) { command = CMD_VOLUMESLIDE;
							if (param & 0x80) param = 0xF0|((-(signed char)param)&0x0F);
							else param = 0x0F|((param&0x0F)<<4);
						} else command = 0; break;
			// 0x12: Fine Portamento
			// 0x16: Extra Fine Portamento
			case 0x12:
			case 0x16:	if (param) { int mask = (command == 0x16) ? 0xE0 : 0xF0;
							command = (param & 0x80) ? CMD_PORTAMENTOUP : CMD_PORTAMENTODOWN;
							if (param & 0x80) param = mask|((-(signed char)param)&0x0F);
							else param |= mask;
						} else command = 0; break;
			// 0x13: Note Delay
			case 0x13:	command = CMD_S3MCMDEX; param = 0xD0|(param & 0x0F); break;
			// 0x14: Note Cut
			case 0x14:	command = CMD_S3MCMDEX; param = 0xC0|(param & 0x0F); break;
			// 0x15: Set Tempo
			case 0x15:	command = CMD_TEMPO; break;
			// 0x17: Panning
			case 0x17:	param = (param+64)&0x7F;
						if (m->command) { if (!m->volcmd) { m->volcmd = VOLCMD_PANNING;  m->vol = param/2; } command = 0; }
						else { command = CMD_PANNING8; }
			// Unknown effects
			default:	command = param = 0;
			}
			if (command)
			{
				m->command = command;
				m->param = param;
			}
		}
		pTrack += 3;
	}
}



BOOL CSoundFile::ReadAMF(LPCBYTE lpStream, const DWORD dwMemLength)
//-----------------------------------------------------------
{
	const AMFFILEHEADER *pfh = (AMFFILEHEADER *)lpStream;
	DWORD dwMemPos;
	
	if ((!lpStream) || (dwMemLength < 2048)) return FALSE;
	if ((!strncmp((LPCTSTR)lpStream, "ASYLUM Music Format V1.0", 25)) && (dwMemLength > 4096))
	{
		UINT numorders, numpats, numsamples;

		dwMemPos = 32;
		numpats = lpStream[dwMemPos+3];
		numorders = lpStream[dwMemPos+4];
		numsamples = 64;
		dwMemPos += 6;
		if ((!numpats) || (numpats > MAX_PATTERNS) || (!numorders)
		 || (numpats*64*32 + 294 + 37*64 >= dwMemLength)) return FALSE;
		m_nType = MOD_TYPE_AMF0;
		m_nChannels = 8;
		m_nInstruments = 0;
		m_nSamples = 31;
		m_nDefaultTempo = 125;
		m_nDefaultSpeed = 6;
		for (UINT iOrd=0; iOrd<MAX_ORDERS; iOrd++)
		{
			Order[iOrd] = (iOrd < numorders) ? lpStream[dwMemPos+iOrd] : 0xFF;
		}
		dwMemPos = 294; // ???
		for (UINT iSmp=0; iSmp<numsamples; iSmp++)
		{
			MODINSTRUMENT *psmp = &Ins[iSmp+1];
			memcpy(m_szNames[iSmp+1], lpStream+dwMemPos, 22);
			psmp->nFineTune = MOD2XMFineTune(lpStream[dwMemPos+22]);
			psmp->nVolume = lpStream[dwMemPos+23];
			psmp->nGlobalVol = 64;
			if (psmp->nVolume > 0x40) psmp->nVolume = 0x40;
			psmp->nVolume <<= 2;
			psmp->nLength = bswapLE32(*((LPDWORD)(lpStream+dwMemPos+25)));
			psmp->nLoopStart = bswapLE32(*((LPDWORD)(lpStream+dwMemPos+29)));
			psmp->nLoopEnd = psmp->nLoopStart + bswapLE32(*((LPDWORD)(lpStream+dwMemPos+33)));
			if ((psmp->nLoopEnd > psmp->nLoopStart) && (psmp->nLoopEnd <= psmp->nLength))
			{
				psmp->uFlags = CHN_LOOP;
			} else
			{
				psmp->nLoopStart = psmp->nLoopEnd = 0;
			}
			if ((psmp->nLength) && (iSmp>31)) m_nSamples = iSmp+1;
			dwMemPos += 37;
		}
		for (UINT iPat=0; iPat<numpats; iPat++)
		{
			MODCOMMAND *p = AllocatePattern(64, m_nChannels);
			if (!p) break;
			Patterns[iPat] = p;
			PatternSize[iPat] = 64;
			const UCHAR *pin = lpStream + dwMemPos;
			for (UINT i=0; i<8*64; i++)
			{
				p->note = 0;

				if (pin[0])
				{
					p->note = pin[0] + 13;
				}
				p->instr = pin[1];
				p->command = pin[2];
				p->param = pin[3];
				if (p->command > 0x0F)
				{
				#ifdef AMFLOG
					Log("0x%02X.0x%02X ?", p->command, p->param);
				#endif
					p->command = 0;
				}
				ConvertModCommand(p);
				pin += 4;
				p++;
			}
			dwMemPos += 64*32;
		}
		// Read samples
		for (UINT iData=0; iData<m_nSamples; iData++)
		{
			MODINSTRUMENT *psmp = &Ins[iData+1];
			if (psmp->nLength)
			{
				if (dwMemPos > dwMemLength) return FALSE;
				dwMemPos += ReadSample(psmp, RS_PCM8S, (LPCSTR)(lpStream+dwMemPos), dwMemLength);
			}
		}
		return TRUE;
	}
	////////////////////////////
	// DSM/AMF
	USHORT *ptracks[MAX_PATTERNS];
	DWORD sampleseekpos[MAX_SAMPLES];

	if ((pfh->szAMF[0] != 'A') || (pfh->szAMF[1] != 'M') || (pfh->szAMF[2] != 'F')
	 || (pfh->version < 10) || (pfh->version > 14) || (!bswapLE16(pfh->numtracks))
	 || (!pfh->numorders) || (pfh->numorders > MAX_PATTERNS)
	 || (!pfh->numsamples) || (pfh->numsamples > MAX_SAMPLES)
	 || (pfh->numchannels < 4) || (pfh->numchannels > 32))
		return FALSE;
	memcpy(m_szNames[0], pfh->title, 32);
	dwMemPos = sizeof(AMFFILEHEADER);
	m_nType = MOD_TYPE_AMF;
	m_nChannels = pfh->numchannels;
	m_nSamples = pfh->numsamples;
	m_nInstruments = 0;
	// Setup Channel Pan Positions
	if (pfh->version >= 11)
	{
		signed char *panpos = (signed char *)(lpStream + dwMemPos);
		UINT nchannels = (pfh->version >= 13) ? 32 : 16;
		for (UINT i=0; i<nchannels; i++)
		{
			int pan = (panpos[i] + 64) * 2;
			if (pan < 0) pan = 0;
			if (pan > 256) { pan = 128; ChnSettings[i].dwFlags |= CHN_SURROUND; }
			ChnSettings[i].nPan = pan;
		}
		dwMemPos += nchannels;
	} else
	{
		for (UINT i=0; i<16; i++)
		{
			ChnSettings[i].nPan = (lpStream[dwMemPos+i] & 1) ? 0x30 : 0xD0;
		}
		dwMemPos += 16;
	}
	// Get Tempo/Speed
	m_nDefaultTempo = 125;
	m_nDefaultSpeed = 6;
	if (pfh->version >= 13)
	{
		if (lpStream[dwMemPos] >= 32) m_nDefaultTempo = lpStream[dwMemPos];
		if (lpStream[dwMemPos+1] <= 32) m_nDefaultSpeed = lpStream[dwMemPos+1];
		dwMemPos += 2;
	}
	// Setup sequence list
	for (UINT iOrd=0; iOrd<MAX_ORDERS; iOrd++)
	{
		Order[iOrd] = 0xFF;
		if (iOrd < pfh->numorders)
		{
			Order[iOrd] = iOrd;
			PatternSize[iOrd] = 64;
			if (pfh->version >= 14)
			{
				PatternSize[iOrd] = bswapLE16(*(USHORT *)(lpStream+dwMemPos));
				dwMemPos += 2;
			}
			ptracks[iOrd] = (USHORT *)(lpStream+dwMemPos);
			dwMemPos += m_nChannels * sizeof(USHORT);
		}
	}
	if (dwMemPos + m_nSamples * (sizeof(AMFSAMPLE)+8) > dwMemLength) return TRUE;
	// Read Samples
	UINT maxsampleseekpos = 0;
	for (UINT iIns=0; iIns<m_nSamples; iIns++)
	{
		MODINSTRUMENT *pins = &Ins[iIns+1];
		AMFSAMPLE *psh = (AMFSAMPLE *)(lpStream + dwMemPos);

		dwMemPos += sizeof(AMFSAMPLE);
		memcpy(m_szNames[iIns+1], psh->samplename, 32);
		memcpy(pins->name, psh->filename, 13);
		pins->nLength = bswapLE32(psh->length);
		pins->nC4Speed = bswapLE16(psh->c2spd);
		pins->nGlobalVol = 64;
		pins->nVolume = psh->volume * 4;
		if (pfh->version >= 11)
		{
			pins->nLoopStart = bswapLE32(*(DWORD *)(lpStream+dwMemPos));
			pins->nLoopEnd = bswapLE32(*(DWORD *)(lpStream+dwMemPos+4));
			dwMemPos += 8;
		} else
		{
			pins->nLoopStart = bswapLE16(*(WORD *)(lpStream+dwMemPos));
			pins->nLoopEnd = pins->nLength;
			dwMemPos += 2;
		}
		sampleseekpos[iIns] = 0;
		if ((psh->type) && (bswapLE32(psh->offset) < dwMemLength-1))
		{
			sampleseekpos[iIns] = bswapLE32(psh->offset);
			if (bswapLE32(psh->offset) > maxsampleseekpos) 
				maxsampleseekpos = bswapLE32(psh->offset);
			if ((pins->nLoopEnd > pins->nLoopStart + 2)
			 && (pins->nLoopEnd <= pins->nLength)) pins->uFlags |= CHN_LOOP;
		}
	}
	// Read Track Mapping Table
	USHORT *pTrackMap = (USHORT *)(lpStream+dwMemPos);
	UINT realtrackcnt = 0;
	dwMemPos += pfh->numtracks * sizeof(USHORT);
	for (UINT iTrkMap=0; iTrkMap<pfh->numtracks; iTrkMap++)
	{
		if (realtrackcnt < pTrackMap[iTrkMap]) realtrackcnt = pTrackMap[iTrkMap];
	}
	// Store tracks positions
	BYTE **pTrackData = new BYTE *[realtrackcnt];
	memset(pTrackData, 0, sizeof(pTrackData));
	for (UINT iTrack=0; iTrack<realtrackcnt; iTrack++) if (dwMemPos + 3 <= dwMemLength)
	{
		UINT nTrkSize = bswapLE16(*(USHORT *)(lpStream+dwMemPos));
		nTrkSize += (UINT)lpStream[dwMemPos+2] << 16;
		if (dwMemPos + nTrkSize * 3 + 3 <= dwMemLength)
		{
			pTrackData[iTrack] = (BYTE *)(lpStream + dwMemPos);
		}
		dwMemPos += nTrkSize * 3 + 3;
	}
	// Create the patterns from the list of tracks
	for (UINT iPat=0; iPat<pfh->numorders; iPat++)
	{
		MODCOMMAND *p = AllocatePattern(PatternSize[iPat], m_nChannels);
		if (!p) break;
		Patterns[iPat] = p;
		for (UINT iChn=0; iChn<m_nChannels; iChn++)
		{
			UINT nTrack = bswapLE16(ptracks[iPat][iChn]);
			if ((nTrack) && (nTrack <= pfh->numtracks))
			{
				UINT realtrk = bswapLE16(pTrackMap[nTrack-1]);
				if (realtrk)
				{
					realtrk--;
					if ((realtrk < realtrackcnt) && (pTrackData[realtrk]))
					{
						AMF_Unpack(p+iChn, pTrackData[realtrk], PatternSize[iPat], m_nChannels);
					}
				}
			}
		}
	}
	delete[] pTrackData;
	// Read Sample Data
	for (UINT iSeek=1; iSeek<=maxsampleseekpos; iSeek++)
	{
		if (dwMemPos >= dwMemLength) break;
		for (UINT iSmp=0; iSmp<m_nSamples; iSmp++) if (iSeek == sampleseekpos[iSmp])
		{
			MODINSTRUMENT *pins = &Ins[iSmp+1];
			dwMemPos += ReadSample(pins, RS_PCM8U, (LPCSTR)(lpStream+dwMemPos), dwMemLength-dwMemPos);
			break;
		}
	}
	return TRUE;
}

