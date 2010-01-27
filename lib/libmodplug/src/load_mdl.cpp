/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

//////////////////////////////////////////////
// DigiTracker (MDL) module loader          //
//////////////////////////////////////////////
#include "stdafx.h"
#include "sndfile.h"

//#pragma warning(disable:4244)

typedef struct MDLSONGHEADER
{
	DWORD id;	// "DMDL" = 0x4C444D44
	BYTE version;
} MDLSONGHEADER;


typedef struct MDLINFOBLOCK
{
	CHAR songname[32];
	CHAR composer[20];
	WORD norders;
	WORD repeatpos;
	BYTE globalvol;
	BYTE speed;
	BYTE tempo;
	BYTE channelinfo[32];
	BYTE seq[256];
} MDLINFOBLOCK;


typedef struct MDLPATTERNDATA
{
	BYTE channels;
	BYTE lastrow;	// nrows = lastrow+1
	CHAR name[16];
	WORD data[1];
} MDLPATTERNDATA;


void ConvertMDLCommand(MODCOMMAND *m, UINT eff, UINT data)
//--------------------------------------------------------
{
	UINT command = 0, param = data;
	switch(eff)
	{
	case 0x01:	command = CMD_PORTAMENTOUP; break;
	case 0x02:	command = CMD_PORTAMENTODOWN; break;
	case 0x03:	command = CMD_TONEPORTAMENTO; break;
	case 0x04:	command = CMD_VIBRATO; break;
	case 0x05:	command = CMD_ARPEGGIO; break;
	case 0x07:	command = (param < 0x20) ? CMD_SPEED : CMD_TEMPO; break;
	case 0x08:	command = CMD_PANNING8; param <<= 1; break;
	case 0x0B:	command = CMD_POSITIONJUMP; break;
	case 0x0C:	command = CMD_GLOBALVOLUME; break;
	case 0x0D:	command = CMD_PATTERNBREAK; param = (data & 0x0F) + (data>>4)*10; break;
	case 0x0E:
		command = CMD_S3MCMDEX;
		switch(data & 0xF0)
		{
		case 0x00:	command = 0; break; // What is E0x in MDL (there is a bunch) ?
		case 0x10:	if (param & 0x0F) { param |= 0xF0; command = CMD_PANNINGSLIDE; } else command = 0; break;
		case 0x20:	if (param & 0x0F) { param = (param << 4) | 0x0F; command = CMD_PANNINGSLIDE; } else command = 0; break;
		case 0x30:	param = (data & 0x0F) | 0x10; break; // glissando
		case 0x40:	param = (data & 0x0F) | 0x30; break; // vibrato waveform
		case 0x60:	param = (data & 0x0F) | 0xB0; break;
		case 0x70:	param = (data & 0x0F) | 0x40; break; // tremolo waveform
		case 0x90:	command = CMD_RETRIG; param &= 0x0F; break;
		case 0xA0:	param = (data & 0x0F) << 4; command = CMD_GLOBALVOLSLIDE; break;
		case 0xB0:	param = data & 0x0F; command = CMD_GLOBALVOLSLIDE; break;
		case 0xF0:	param = ((data >> 8) & 0x0F) | 0xA0; break;
		}
		break;
	case 0x0F:	command = CMD_SPEED; break;
	case 0x10:	if ((param & 0xF0) != 0xE0) { command = CMD_VOLUMESLIDE; if ((param & 0xF0) == 0xF0) param = ((param << 4) | 0x0F); else param >>= 2; } break;
	case 0x20:	if ((param & 0xF0) != 0xE0) { command = CMD_VOLUMESLIDE; if ((param & 0xF0) != 0xF0) param >>= 2; } break;
	case 0x30:	command = CMD_RETRIG; break;
	case 0x40:	command = CMD_TREMOLO; break;
	case 0x50:	command = CMD_TREMOR; break;
	case 0xEF:	if (param > 0xFF) param = 0xFF; command = CMD_OFFSET; break;
	}
	if (command)
	{
		m->command = command;
		m->param = param;
	}
}


void UnpackMDLTrack(MODCOMMAND *pat, UINT nChannels, UINT nRows, UINT nTrack, const BYTE *lpTracks)
//-------------------------------------------------------------------------------------------------
{
	MODCOMMAND cmd, *m = pat;
	UINT len = *((WORD *)lpTracks);
	UINT pos = 0, row = 0, i;
	lpTracks += 2;
	for (UINT ntrk=1; ntrk<nTrack; ntrk++)
	{
		lpTracks += len;
		len = *((WORD *)lpTracks);
		lpTracks += 2;
	}
	cmd.note = cmd.instr = 0;
	cmd.volcmd = cmd.vol = 0;
	cmd.command = cmd.param = 0;
	while ((row < nRows) && (pos < len))
	{
		UINT xx;
		BYTE b = lpTracks[pos++];
		xx = b >> 2;
		switch(b & 0x03)
		{
		case 0x01:
			for (i=0; i<=xx; i++)
			{
				if (row) *m = *(m-nChannels);
				m += nChannels;
				row++;
				if (row >= nRows) break;
			}
			break;

		case 0x02:
			if (xx < row) *m = pat[nChannels*xx];
			m += nChannels;
			row++;
			break;

		case 0x03:
			{
				cmd.note = (xx & 0x01) ? lpTracks[pos++] : 0;
				cmd.instr = (xx & 0x02) ? lpTracks[pos++] : 0;
				cmd.volcmd = cmd.vol = 0;
				cmd.command = cmd.param = 0;
				if ((cmd.note < NOTE_MAX-12) && (cmd.note)) cmd.note += 12;
				UINT volume = (xx & 0x04) ? lpTracks[pos++] : 0;
				UINT commands = (xx & 0x08) ? lpTracks[pos++] : 0;
				UINT command1 = commands & 0x0F;
				UINT command2 = commands & 0xF0;
				UINT param1 = (xx & 0x10) ? lpTracks[pos++] : 0;
				UINT param2 = (xx & 0x20) ? lpTracks[pos++] : 0;
				if ((command1 == 0x0E) && ((param1 & 0xF0) == 0xF0) && (!command2))
				{
					param1 = ((param1 & 0x0F) << 8) | param2;
					command1 = 0xEF;
					command2 = param2 = 0;
				}
				if (volume)
				{
					cmd.volcmd = VOLCMD_VOLUME;
					cmd.vol = (volume+1) >> 2;
				}
				ConvertMDLCommand(&cmd, command1, param1);
				if ((cmd.command != CMD_SPEED)
				 && (cmd.command != CMD_TEMPO)
				 && (cmd.command != CMD_PATTERNBREAK))
					ConvertMDLCommand(&cmd, command2, param2);
				*m = cmd;
				m += nChannels;
				row++;
			}
			break;

		// Empty Slots
		default:
			row += xx+1;
			m += (xx+1)*nChannels;
			if (row >= nRows) break;
		}
	}
}



BOOL CSoundFile::ReadMDL(const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
	DWORD dwMemPos, dwPos, blocklen, dwTrackPos;
	const MDLSONGHEADER *pmsh = (const MDLSONGHEADER *)lpStream;
	MDLINFOBLOCK *pmib;
	MDLPATTERNDATA *pmpd;
	UINT i,j, norders = 0, npatterns = 0, ntracks = 0;
	UINT ninstruments = 0, nsamples = 0;
	WORD block;
	WORD patterntracks[MAX_PATTERNS*32];
	BYTE smpinfo[MAX_SAMPLES];
	BYTE insvolenv[MAX_INSTRUMENTS];
	BYTE inspanenv[MAX_INSTRUMENTS];
	LPCBYTE pvolenv, ppanenv, ppitchenv;
	UINT nvolenv, npanenv, npitchenv;

	if ((!lpStream) || (dwMemLength < 1024)) return FALSE;
	if ((pmsh->id != 0x4C444D44) || ((pmsh->version & 0xF0) > 0x10)) return FALSE;
	memset(patterntracks, 0, sizeof(patterntracks));
	memset(smpinfo, 0, sizeof(smpinfo));
	memset(insvolenv, 0, sizeof(insvolenv));
	memset(inspanenv, 0, sizeof(inspanenv));
	dwMemPos = 5;
	dwTrackPos = 0;
	pvolenv = ppanenv = ppitchenv = NULL;
	nvolenv = npanenv = npitchenv = 0;
	m_nSamples = m_nInstruments = 0;
	while (dwMemPos+6 < dwMemLength)
	{
		block = *((WORD *)(lpStream+dwMemPos));
		blocklen = *((DWORD *)(lpStream+dwMemPos+2));
		dwMemPos += 6;
		if (dwMemPos + blocklen > dwMemLength)
		{
			if (dwMemPos == 11) return FALSE;
			break;
		}
		switch(block)
		{
		// IN: infoblock
		case 0x4E49:
			pmib = (MDLINFOBLOCK *)(lpStream+dwMemPos);
			memcpy(m_szNames[0], pmib->songname, 32);
			norders = pmib->norders;
			if (norders > MAX_ORDERS) norders = MAX_ORDERS;
			m_nRestartPos = pmib->repeatpos;
			m_nDefaultGlobalVolume = pmib->globalvol;
			m_nDefaultTempo = pmib->tempo;
			m_nDefaultSpeed = pmib->speed;
			m_nChannels = 4;
			for (i=0; i<32; i++)
			{
				ChnSettings[i].nVolume = 64;
				ChnSettings[i].nPan = (pmib->channelinfo[i] & 0x7F) << 1;
				if (pmib->channelinfo[i] & 0x80)
					ChnSettings[i].dwFlags |= CHN_MUTE;
				else
					m_nChannels = i+1;
			}
			for (j=0; j<norders; j++) Order[j] = pmib->seq[j];
			break;
		// ME: song message
		case 0x454D:
			if (blocklen)
			{
				if (m_lpszSongComments) delete [] m_lpszSongComments;
				m_lpszSongComments = new char[blocklen];
				if (m_lpszSongComments)
				{
					memcpy(m_lpszSongComments, lpStream+dwMemPos, blocklen);
					m_lpszSongComments[blocklen-1] = 0;
				}
			}
			break;
		// PA: Pattern Data
		case 0x4150:
			npatterns = lpStream[dwMemPos];
			if (npatterns > MAX_PATTERNS) npatterns = MAX_PATTERNS;
			dwPos = dwMemPos + 1;
			for (i=0; i<npatterns; i++)
			{
				if (dwPos+18 >= dwMemLength) break;
				pmpd = (MDLPATTERNDATA *)(lpStream + dwPos);
				if (pmpd->channels > 32) break;
				PatternSize[i] = pmpd->lastrow+1;
				if (m_nChannels < pmpd->channels) m_nChannels = pmpd->channels;
				dwPos += 18 + 2*pmpd->channels;
				for (j=0; j<pmpd->channels; j++)
				{
					patterntracks[i*32+j] = pmpd->data[j];
				}
			}
			break;
		// TR: Track Data
		case 0x5254:
			if (dwTrackPos) break;
			ntracks = *((WORD *)(lpStream+dwMemPos));
			dwTrackPos = dwMemPos+2;
			break;
		// II: Instruments
		case 0x4949:
			ninstruments = lpStream[dwMemPos];
			dwPos = dwMemPos+1;
			for (i=0; i<ninstruments; i++)
			{
				UINT nins = lpStream[dwPos];
				if ((nins >= MAX_INSTRUMENTS) || (!nins)) break;
				if (m_nInstruments < nins) m_nInstruments = nins;
				if (!Headers[nins])
				{
					UINT note = 12;
					if ((Headers[nins] = new INSTRUMENTHEADER) == NULL) break;
					INSTRUMENTHEADER *penv = Headers[nins];
					memset(penv, 0, sizeof(INSTRUMENTHEADER));
					memcpy(penv->name, lpStream+dwPos+2, 32);
					penv->nGlobalVol = 64;
					penv->nPPC = 5*12;
					for (j=0; j<lpStream[dwPos+1]; j++)
					{
						const BYTE *ps = lpStream+dwPos+34+14*j;
						while ((note < (UINT)(ps[1]+12)) && (note < NOTE_MAX))
						{
							penv->NoteMap[note] = note+1;
							if (ps[0] < MAX_SAMPLES)
							{
								int ismp = ps[0];
								penv->Keyboard[note] = ps[0];
								Ins[ismp].nVolume = ps[2];
								Ins[ismp].nPan = ps[4] << 1;
								Ins[ismp].nVibType = ps[11];
								Ins[ismp].nVibSweep = ps[10];
								Ins[ismp].nVibDepth = ps[9];
								Ins[ismp].nVibRate = ps[8];
							}
							penv->nFadeOut = (ps[7] << 8) | ps[6];
							if (penv->nFadeOut == 0xFFFF) penv->nFadeOut = 0;
							note++;
						}
						// Use volume envelope ?
						if (ps[3] & 0x80)
						{
							penv->dwFlags |= ENV_VOLUME;
							insvolenv[nins] = (ps[3] & 0x3F) + 1;
						}
						// Use panning envelope ?
						if (ps[5] & 0x80)
						{
							penv->dwFlags |= ENV_PANNING;
							inspanenv[nins] = (ps[5] & 0x3F) + 1;
						}
					}
				}
				dwPos += 34 + 14*lpStream[dwPos+1];
			}
			for (j=1; j<=m_nInstruments; j++) if (!Headers[j])
			{
				Headers[j] = new INSTRUMENTHEADER;
				if (Headers[j]) memset(Headers[j], 0, sizeof(INSTRUMENTHEADER));
			}
			break;
		// VE: Volume Envelope
		case 0x4556:
			if ((nvolenv = lpStream[dwMemPos]) == 0) break;
			if (dwMemPos + nvolenv*32 + 1 <= dwMemLength) pvolenv = lpStream + dwMemPos + 1;
			break;
		// PE: Panning Envelope
		case 0x4550:
			if ((npanenv = lpStream[dwMemPos]) == 0) break;
			if (dwMemPos + npanenv*32 + 1 <= dwMemLength) ppanenv = lpStream + dwMemPos + 1;
			break;
		// FE: Pitch Envelope
		case 0x4546:
			if ((npitchenv = lpStream[dwMemPos]) == 0) break;
			if (dwMemPos + npitchenv*32 + 1 <= dwMemLength) ppitchenv = lpStream + dwMemPos + 1;
			break;
		// IS: Sample Infoblock
		case 0x5349:
			nsamples = lpStream[dwMemPos];
			dwPos = dwMemPos+1;
			for (i=0; i<nsamples; i++, dwPos += 59)
			{
				UINT nins = lpStream[dwPos];
				if ((nins >= MAX_SAMPLES) || (!nins)) continue;
				if (m_nSamples < nins) m_nSamples = nins;
				MODINSTRUMENT *pins = &Ins[nins];
				memcpy(m_szNames[nins], lpStream+dwPos+1, 32);
				memcpy(pins->name, lpStream+dwPos+33, 8);
				pins->nC4Speed = *((DWORD *)(lpStream+dwPos+41));
				pins->nLength = *((DWORD *)(lpStream+dwPos+45));
				pins->nLoopStart = *((DWORD *)(lpStream+dwPos+49));
				pins->nLoopEnd = pins->nLoopStart + *((DWORD *)(lpStream+dwPos+53));
				if (pins->nLoopEnd > pins->nLoopStart) pins->uFlags |= CHN_LOOP;
				pins->nGlobalVol = 64;
				if (lpStream[dwPos+58] & 0x01)
				{
					pins->uFlags |= CHN_16BIT;
					pins->nLength >>= 1;
					pins->nLoopStart >>= 1;
					pins->nLoopEnd >>= 1;
				}
				if (lpStream[dwPos+58] & 0x02) pins->uFlags |= CHN_PINGPONGLOOP;
				smpinfo[nins] = (lpStream[dwPos+58] >> 2) & 3;
			}
			break;
		// SA: Sample Data
		case 0x4153:
			dwPos = dwMemPos;
			for (i=1; i<=m_nSamples; i++) if ((Ins[i].nLength) && (!Ins[i].pSample) && (smpinfo[i] != 3) && (dwPos < dwMemLength))
			{
				MODINSTRUMENT *pins = &Ins[i];
				UINT flags = (pins->uFlags & CHN_16BIT) ? RS_PCM16S : RS_PCM8S;
				if (!smpinfo[i])
				{
					dwPos += ReadSample(pins, flags, (LPSTR)(lpStream+dwPos), dwMemLength - dwPos);
				} else
				{
					DWORD dwLen = *((DWORD *)(lpStream+dwPos));
					dwPos += 4;
					if ((dwPos+dwLen <= dwMemLength) && (dwLen > 4))
					{
						flags = (pins->uFlags & CHN_16BIT) ? RS_MDL16 : RS_MDL8;
						ReadSample(pins, flags, (LPSTR)(lpStream+dwPos), dwLen);
					}
					dwPos += dwLen;
				}
			}
			break;
		}
		dwMemPos += blocklen;
	}
	// Unpack Patterns
	if ((dwTrackPos) && (npatterns) && (m_nChannels) && (ntracks))
	{
		for (UINT ipat=0; ipat<npatterns; ipat++)
		{
			if ((Patterns[ipat] = AllocatePattern(PatternSize[ipat], m_nChannels)) == NULL) break;
			for (UINT chn=0; chn<m_nChannels; chn++) if ((patterntracks[ipat*32+chn]) && (patterntracks[ipat*32+chn] <= ntracks))
			{
				MODCOMMAND *m = Patterns[ipat] + chn;
				UnpackMDLTrack(m, m_nChannels, PatternSize[ipat], patterntracks[ipat*32+chn], lpStream+dwTrackPos);
			}
		}
	}
	// Set up envelopes
	for (UINT iIns=1; iIns<=m_nInstruments; iIns++) if (Headers[iIns])
	{
		INSTRUMENTHEADER *penv = Headers[iIns];
		// Setup volume envelope
		if ((nvolenv) && (pvolenv) && (insvolenv[iIns]))
		{
			LPCBYTE pve = pvolenv;
			for (UINT nve=0; nve<nvolenv; nve++, pve+=33) if (pve[0]+1 == insvolenv[iIns])
			{
				WORD vtick = 1;
				penv->nVolEnv = 15;
				for (UINT iv=0; iv<15; iv++)
				{
					if (iv) vtick += pve[iv*2+1];
					penv->VolPoints[iv] = vtick;
					penv->VolEnv[iv] = pve[iv*2+2];
					if (!pve[iv*2+1])
					{
						penv->nVolEnv = iv+1;
						break;
					}
				}
				penv->nVolSustainBegin = penv->nVolSustainEnd = pve[31] & 0x0F;
				if (pve[31] & 0x10) penv->dwFlags |= ENV_VOLSUSTAIN;
				if (pve[31] & 0x20) penv->dwFlags |= ENV_VOLLOOP;
				penv->nVolLoopStart = pve[32] & 0x0F;
				penv->nVolLoopEnd = pve[32] >> 4;
			}
		}
		// Setup panning envelope
		if ((npanenv) && (ppanenv) && (inspanenv[iIns]))
		{
			LPCBYTE ppe = ppanenv;
			for (UINT npe=0; npe<npanenv; npe++, ppe+=33) if (ppe[0]+1 == inspanenv[iIns])
			{
				WORD vtick = 1;
				penv->nPanEnv = 15;
				for (UINT iv=0; iv<15; iv++)
				{
					if (iv) vtick += ppe[iv*2+1];
					penv->PanPoints[iv] = vtick;
					penv->PanEnv[iv] = ppe[iv*2+2];
					if (!ppe[iv*2+1])
					{
						penv->nPanEnv = iv+1;
						break;
					}
				}
				if (ppe[31] & 0x10) penv->dwFlags |= ENV_PANSUSTAIN;
				if (ppe[31] & 0x20) penv->dwFlags |= ENV_PANLOOP;
				penv->nPanLoopStart = ppe[32] & 0x0F;
				penv->nPanLoopEnd = ppe[32] >> 4;
			}
		}
	}
	m_dwSongFlags |= SONG_LINEARSLIDES;
	m_nType = MOD_TYPE_MDL;
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////
// MDL Sample Unpacking

// MDL Huffman ReadBits compression
WORD MDLReadBits(DWORD &bitbuf, UINT &bitnum, LPBYTE &ibuf, CHAR n)
//-----------------------------------------------------------------
{
	WORD v = (WORD)(bitbuf & ((1 << n) - 1) );
	bitbuf >>= n;
	bitnum -= n;
	if (bitnum <= 24)
	{
		bitbuf |= (((DWORD)(*ibuf++)) << bitnum);
		bitnum += 8;
	}
	return v;
}


