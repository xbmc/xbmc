/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>,
 *          Adam Goode       <adam@evdebs.org> (Endian and char fixes for PPC)
 *          Marco Trillo     <toad@arsystel.com> (Endian fixes for SaveIT, XM->IT Sample Converter)
 *
*/

#include "stdafx.h"
#include "sndfile.h"
#include "it_defs.h"

#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

BYTE autovibit2xm[8] =
{ 0, 3, 1, 4, 2, 0, 0, 0 };

BYTE autovibxm2it[8] =
{ 0, 2, 4, 1, 3, 0, 0, 0 };

//////////////////////////////////////////////////////////
// Impulse Tracker IT file support

// for conversion of XM samples
extern WORD XMPeriodTable[96+8];
extern UINT XMLinearTable[768];

static inline UINT ConvertVolParam(UINT value)
//--------------------------------------------
{
	return (value > 9)  ? 9 : value;
}


BOOL CSoundFile::ITInstrToMPT(const void *p, INSTRUMENTHEADER *penv, UINT trkvers)
//--------------------------------------------------------------------------------
{
	if (trkvers < 0x0200)
	{
		const ITOLDINSTRUMENT *pis = (const ITOLDINSTRUMENT *)p;
		memcpy(penv->name, pis->name, 26);
		memcpy(penv->filename, pis->filename, 12);
		penv->nFadeOut = bswapLE16(pis->fadeout) << 6;
		penv->nGlobalVol = 64;
		for (UINT j=0; j<NOTE_MAX; j++)
		{
			UINT note = pis->keyboard[j*2];
			UINT ins = pis->keyboard[j*2+1];
			if (ins < MAX_SAMPLES) penv->Keyboard[j] = ins;
			if (note < 128) penv->NoteMap[j] = note+1;
			else if (note >= 0xFE) penv->NoteMap[j] = note;
		}
		if (pis->flags & 0x01) penv->dwFlags |= ENV_VOLUME;
		if (pis->flags & 0x02) penv->dwFlags |= ENV_VOLLOOP;
		if (pis->flags & 0x04) penv->dwFlags |= ENV_VOLSUSTAIN;
		penv->nVolLoopStart = pis->vls;
		penv->nVolLoopEnd = pis->vle;
		penv->nVolSustainBegin = pis->sls;
		penv->nVolSustainEnd = pis->sle;
		penv->nVolEnv = 25;
		for (UINT ev=0; ev<25; ev++)
		{
			if ((penv->VolPoints[ev] = pis->nodes[ev*2]) == 0xFF)
			{
				penv->nVolEnv = ev;
				break;
			}
			penv->VolEnv[ev] = pis->nodes[ev*2+1];
		}
		penv->nNNA = pis->nna;
		penv->nDCT = pis->dnc;
		penv->nPan = 0x80;
	} else
	{
		const ITINSTRUMENT *pis = (const ITINSTRUMENT *)p;
		memcpy(penv->name, pis->name, 26);
		memcpy(penv->filename, pis->filename, 12);
		penv->nMidiProgram = pis->mpr;
		penv->nMidiChannel = pis->mch;
		penv->wMidiBank = bswapLE16(pis->mbank);
		penv->nFadeOut = bswapLE16(pis->fadeout) << 5;
		penv->nGlobalVol = pis->gbv >> 1;
		if (penv->nGlobalVol > 64) penv->nGlobalVol = 64;
		for (UINT j=0; j<NOTE_MAX; j++)
		{
			UINT note = pis->keyboard[j*2];
			UINT ins = pis->keyboard[j*2+1];
			if (ins < MAX_SAMPLES) penv->Keyboard[j] = ins;
			if (note < 128) penv->NoteMap[j] = note+1;
			else if (note >= 0xFE) penv->NoteMap[j] = note;
		}
		// Volume Envelope
		if (pis->volenv.flags & 1) penv->dwFlags |= ENV_VOLUME;
		if (pis->volenv.flags & 2) penv->dwFlags |= ENV_VOLLOOP;
		if (pis->volenv.flags & 4) penv->dwFlags |= ENV_VOLSUSTAIN;
		if (pis->volenv.flags & 8) penv->dwFlags |= ENV_VOLCARRY;
		penv->nVolEnv = pis->volenv.num;
		if (penv->nVolEnv > 25) penv->nVolEnv = 25;

		penv->nVolLoopStart = pis->volenv.lpb;
		penv->nVolLoopEnd = pis->volenv.lpe;
		penv->nVolSustainBegin = pis->volenv.slb;
		penv->nVolSustainEnd = pis->volenv.sle;
		// Panning Envelope
		if (pis->panenv.flags & 1) penv->dwFlags |= ENV_PANNING;
		if (pis->panenv.flags & 2) penv->dwFlags |= ENV_PANLOOP;
		if (pis->panenv.flags & 4) penv->dwFlags |= ENV_PANSUSTAIN;
		if (pis->panenv.flags & 8) penv->dwFlags |= ENV_PANCARRY;
		penv->nPanEnv = pis->panenv.num;
		if (penv->nPanEnv > 25) penv->nPanEnv = 25;
		penv->nPanLoopStart = pis->panenv.lpb;
		penv->nPanLoopEnd = pis->panenv.lpe;
		penv->nPanSustainBegin = pis->panenv.slb;
		penv->nPanSustainEnd = pis->panenv.sle;
		// Pitch Envelope
		if (pis->pitchenv.flags & 1) penv->dwFlags |= ENV_PITCH;
		if (pis->pitchenv.flags & 2) penv->dwFlags |= ENV_PITCHLOOP;
		if (pis->pitchenv.flags & 4) penv->dwFlags |= ENV_PITCHSUSTAIN;
		if (pis->pitchenv.flags & 8) penv->dwFlags |= ENV_PITCHCARRY;
		if (pis->pitchenv.flags & 0x80) penv->dwFlags |= ENV_FILTER;
		penv->nPitchEnv = pis->pitchenv.num;
		if (penv->nPitchEnv > 25) penv->nPitchEnv = 25;
		penv->nPitchLoopStart = pis->pitchenv.lpb;
		penv->nPitchLoopEnd = pis->pitchenv.lpe;
		penv->nPitchSustainBegin = pis->pitchenv.slb;
		penv->nPitchSustainEnd = pis->pitchenv.sle;
		// Envelopes Data
		for (UINT ev=0; ev<25; ev++)
		{
			penv->VolEnv[ev] = pis->volenv.data[ev*3];
			penv->VolPoints[ev] = (pis->volenv.data[ev*3+2] << 8) | (pis->volenv.data[ev*3+1]);
			penv->PanEnv[ev] = pis->panenv.data[ev*3] + 32;
			penv->PanPoints[ev] = (pis->panenv.data[ev*3+2] << 8) | (pis->panenv.data[ev*3+1]);
			penv->PitchEnv[ev] = pis->pitchenv.data[ev*3] + 32;
			penv->PitchPoints[ev] = (pis->pitchenv.data[ev*3+2] << 8) | (pis->pitchenv.data[ev*3+1]);
		}
		penv->nNNA = pis->nna;
		penv->nDCT = pis->dct;
		penv->nDNA = pis->dca;
		penv->nPPS = pis->pps;
		penv->nPPC = pis->ppc;
		penv->nIFC = pis->ifc;
		penv->nIFR = pis->ifr;
		penv->nVolSwing = pis->rv;
		penv->nPanSwing = pis->rp;
		penv->nPan = (pis->dfp & 0x7F) << 2;
		if (penv->nPan > 256) penv->nPan = 128;
		if (pis->dfp < 0x80) penv->dwFlags |= ENV_SETPANNING;
	}
	if ((penv->nVolLoopStart >= 25) || (penv->nVolLoopEnd >= 25)) penv->dwFlags &= ~ENV_VOLLOOP;
	if ((penv->nVolSustainBegin >= 25) || (penv->nVolSustainEnd >= 25)) penv->dwFlags &= ~ENV_VOLSUSTAIN;
	return TRUE;
}


BOOL CSoundFile::ReadIT(const BYTE *lpStream, DWORD dwMemLength)
//--------------------------------------------------------------
{
	ITFILEHEADER pifh = *(ITFILEHEADER *)lpStream;
	DWORD dwMemPos = sizeof(ITFILEHEADER);
	DWORD inspos[MAX_INSTRUMENTS];
	DWORD smppos[MAX_SAMPLES];
	DWORD patpos[MAX_PATTERNS];
	BYTE chnmask[64], channels_used[64];
	MODCOMMAND lastvalue[64];

	pifh.id = bswapLE32(pifh.id);
	pifh.reserved1 = bswapLE16(pifh.reserved1);
	pifh.ordnum = bswapLE16(pifh.ordnum);
	pifh.insnum = bswapLE16(pifh.insnum);
	pifh.smpnum = bswapLE16(pifh.smpnum);
	pifh.patnum = bswapLE16(pifh.patnum);
	pifh.cwtv = bswapLE16(pifh.cwtv);
	pifh.cmwt = bswapLE16(pifh.cmwt);
	pifh.flags = bswapLE16(pifh.flags);
	pifh.special = bswapLE16(pifh.special);
	pifh.msglength = bswapLE16(pifh.msglength);
	pifh.msgoffset = bswapLE32(pifh.msgoffset);
	pifh.reserved2 = bswapLE32(pifh.reserved2);

	if ((!lpStream) || (dwMemLength < 0x100)) return FALSE;
	if ((pifh.id != 0x4D504D49) || (pifh.insnum >= MAX_INSTRUMENTS)
	 || (!pifh.smpnum) || (pifh.smpnum >= MAX_INSTRUMENTS) || (!pifh.ordnum)) return FALSE;
	if (dwMemPos + pifh.ordnum + pifh.insnum*4
	 + pifh.smpnum*4 + pifh.patnum*4 > dwMemLength) return FALSE;
	m_nType = MOD_TYPE_IT;
	if (pifh.flags & 0x08) m_dwSongFlags |= SONG_LINEARSLIDES;
	if (pifh.flags & 0x10) m_dwSongFlags |= SONG_ITOLDEFFECTS;
	if (pifh.flags & 0x20) m_dwSongFlags |= SONG_ITCOMPATMODE;
	if (pifh.flags & 0x80) m_dwSongFlags |= SONG_EMBEDMIDICFG;
	if (pifh.flags & 0x1000) m_dwSongFlags |= SONG_EXFILTERRANGE;
	memcpy(m_szNames[0], pifh.songname, 26);
	m_szNames[0][26] = 0;
	// Global Volume
	if (pifh.globalvol)
	{
		m_nDefaultGlobalVolume = pifh.globalvol << 1;
		if (!m_nDefaultGlobalVolume) m_nDefaultGlobalVolume = 256;
		if (m_nDefaultGlobalVolume > 256) m_nDefaultGlobalVolume = 256;
	}
	if (pifh.speed) m_nDefaultSpeed = pifh.speed;
	if (pifh.tempo) m_nDefaultTempo = pifh.tempo;
	m_nSongPreAmp = pifh.mv & 0x7F;
	// Reading Channels Pan Positions
	for (int ipan=0; ipan<64; ipan++) if (pifh.chnpan[ipan] != 0xFF)
	{
		ChnSettings[ipan].nVolume = pifh.chnvol[ipan];
		ChnSettings[ipan].nPan = 128;
		if (pifh.chnpan[ipan] & 0x80) ChnSettings[ipan].dwFlags |= CHN_MUTE;
		UINT n = pifh.chnpan[ipan] & 0x7F;
		if (n <= 64) ChnSettings[ipan].nPan = n << 2;
		if (n == 100) ChnSettings[ipan].dwFlags |= CHN_SURROUND;
	}
	if (m_nChannels < 4) m_nChannels = 4;
	// Reading Song Message
	if ((pifh.special & 0x01) && (pifh.msglength) && (pifh.msgoffset + pifh.msglength < dwMemLength))
	{
		m_lpszSongComments = new char[pifh.msglength+1];
		if (m_lpszSongComments)
		{
			memcpy(m_lpszSongComments, lpStream+pifh.msgoffset, pifh.msglength);
			m_lpszSongComments[pifh.msglength] = 0;
		}
	}
	// Reading orders
	UINT nordsize = pifh.ordnum;
	if (nordsize > MAX_ORDERS) nordsize = MAX_ORDERS;
	memcpy(Order, lpStream+dwMemPos, nordsize);
	dwMemPos += pifh.ordnum;
	// Reading Instrument Offsets
	memset(inspos, 0, sizeof(inspos));
	UINT inspossize = pifh.insnum;
	if (inspossize > MAX_INSTRUMENTS) inspossize = MAX_INSTRUMENTS;
	inspossize <<= 2;
	memcpy(inspos, lpStream+dwMemPos, inspossize);
	for (UINT j=0; j < (inspossize>>2); j++)
	{
	       inspos[j] = bswapLE32(inspos[j]);
	}
	dwMemPos += pifh.insnum * 4;
	// Reading Samples Offsets
	memset(smppos, 0, sizeof(smppos));
	UINT smppossize = pifh.smpnum;
	if (smppossize > MAX_SAMPLES) smppossize = MAX_SAMPLES;
	smppossize <<= 2;
	memcpy(smppos, lpStream+dwMemPos, smppossize);
	for (UINT j=0; j < (smppossize>>2); j++)
	{
	       smppos[j] = bswapLE32(smppos[j]);
	}
	dwMemPos += pifh.smpnum * 4;
	// Reading Patterns Offsets
	memset(patpos, 0, sizeof(patpos));
	UINT patpossize = pifh.patnum;
	if (patpossize > MAX_PATTERNS) patpossize = MAX_PATTERNS;
	patpossize <<= 2;
	memcpy(patpos, lpStream+dwMemPos, patpossize);
	for (UINT j=0; j < (patpossize>>2); j++)
	{
	       patpos[j] = bswapLE32(patpos[j]);
	}
	dwMemPos += pifh.patnum * 4;
	// Reading IT Extra Info
	if (dwMemPos + 2 < dwMemLength)
	{
		UINT nflt = bswapLE16(*((WORD *)(lpStream + dwMemPos)));
		dwMemPos += 2;
		if (dwMemPos + nflt * 8 < dwMemLength) dwMemPos += nflt * 8;
	}
	// Reading Midi Output & Macros
	if (m_dwSongFlags & SONG_EMBEDMIDICFG)
	{
		if (dwMemPos + sizeof(MODMIDICFG) < dwMemLength)
		{
			memcpy(&m_MidiCfg, lpStream+dwMemPos, sizeof(MODMIDICFG));
			dwMemPos += sizeof(MODMIDICFG);
		}
	}
	// Read pattern names: "PNAM"
	if ((dwMemPos + 8 < dwMemLength) && (bswapLE32(*((DWORD *)(lpStream+dwMemPos))) == 0x4d414e50))
	{
		UINT len = bswapLE32(*((DWORD *)(lpStream+dwMemPos+4)));
		dwMemPos += 8;
		if ((dwMemPos + len <= dwMemLength) && (len <= MAX_PATTERNS*MAX_PATTERNNAME) && (len >= MAX_PATTERNNAME))
		{
			m_lpszPatternNames = new char[len];
			if (m_lpszPatternNames)
			{
				m_nPatternNames = len / MAX_PATTERNNAME;
				memcpy(m_lpszPatternNames, lpStream+dwMemPos, len);
			}
			dwMemPos += len;
		}
	}
	// 4-channels minimum
	m_nChannels = 4;
	// Read channel names: "CNAM"
	if ((dwMemPos + 8 < dwMemLength) && (bswapLE32(*((DWORD *)(lpStream+dwMemPos))) == 0x4d414e43))
	{
		UINT len = bswapLE32(*((DWORD *)(lpStream+dwMemPos+4)));
		dwMemPos += 8;
		if ((dwMemPos + len <= dwMemLength) && (len <= 64*MAX_CHANNELNAME))
		{
			UINT n = len / MAX_CHANNELNAME;
			if (n > m_nChannels) m_nChannels = n;
			for (UINT i=0; i<n; i++)
			{
				memcpy(ChnSettings[i].szName, (lpStream+dwMemPos+i*MAX_CHANNELNAME), MAX_CHANNELNAME);
				ChnSettings[i].szName[MAX_CHANNELNAME-1] = 0;
			}
			dwMemPos += len;
		}
	}
	// Read mix plugins information
	if (dwMemPos + 8 < dwMemLength)
	{
		dwMemPos += LoadMixPlugins(lpStream+dwMemPos, dwMemLength-dwMemPos);
	}
	// Checking for unused channels
	UINT npatterns = pifh.patnum;
	if (npatterns > MAX_PATTERNS) npatterns = MAX_PATTERNS;
	for (UINT patchk=0; patchk<npatterns; patchk++)
	{
		memset(chnmask, 0, sizeof(chnmask));
		if ((!patpos[patchk]) || ((DWORD)patpos[patchk] + 4 >= dwMemLength)) continue;
		UINT len = bswapLE16(*((WORD *)(lpStream+patpos[patchk])));
		UINT rows = bswapLE16(*((WORD *)(lpStream+patpos[patchk]+2)));
		if ((rows < 4) || (rows > 256)) continue;
		if (patpos[patchk]+8+len > dwMemLength) continue;
		UINT i = 0;
		const BYTE *p = lpStream+patpos[patchk]+8;
		UINT nrow = 0;
		while (nrow<rows)
		{
			if (i >= len) break;
			BYTE b = p[i++];
			if (!b)
			{
				nrow++;
				continue;
			}
			UINT ch = b & 0x7F;
			if (ch) ch = (ch - 1) & 0x3F;
			if (b & 0x80)
			{
				if (i >= len) break;
				chnmask[ch] = p[i++];
			}
			// Channel used
			if (chnmask[ch] & 0x0F)
			{
				if ((ch >= m_nChannels) && (ch < 64)) m_nChannels = ch+1;
			}
			// Note
			if (chnmask[ch] & 1) i++;
			// Instrument
			if (chnmask[ch] & 2) i++;
			// Volume
			if (chnmask[ch] & 4) i++;
			// Effect
			if (chnmask[ch] & 8) i += 2;
			if (i >= len) break;
		}
	}
	// Reading Instruments
	m_nInstruments = 0;
	if (pifh.flags & 0x04) m_nInstruments = pifh.insnum;
	if (m_nInstruments >= MAX_INSTRUMENTS) m_nInstruments = MAX_INSTRUMENTS-1;
	for (UINT nins=0; nins<m_nInstruments; nins++)
	{
		if ((inspos[nins] > 0) && (inspos[nins] < dwMemLength - sizeof(ITOLDINSTRUMENT)))
		{
			INSTRUMENTHEADER *penv = new INSTRUMENTHEADER;
			if (!penv) continue;
			Headers[nins+1] = penv;
			memset(penv, 0, sizeof(INSTRUMENTHEADER));
			ITInstrToMPT(lpStream + inspos[nins], penv, pifh.cmwt);
		}
	}
	// Reading Samples
	m_nSamples = pifh.smpnum;
	if (m_nSamples >= MAX_SAMPLES) m_nSamples = MAX_SAMPLES-1;
	for (UINT nsmp=0; nsmp<pifh.smpnum; nsmp++) if ((smppos[nsmp]) && (smppos[nsmp] + sizeof(ITSAMPLESTRUCT) <= dwMemLength))
	{
		ITSAMPLESTRUCT pis = *(ITSAMPLESTRUCT *)(lpStream+smppos[nsmp]);
		pis.id = bswapLE32(pis.id);
		pis.length = bswapLE32(pis.length);
		pis.loopbegin = bswapLE32(pis.loopbegin);
		pis.loopend = bswapLE32(pis.loopend);
		pis.C5Speed = bswapLE32(pis.C5Speed);
		pis.susloopbegin = bswapLE32(pis.susloopbegin);
		pis.susloopend = bswapLE32(pis.susloopend);
		pis.samplepointer = bswapLE32(pis.samplepointer);

		if (pis.id == 0x53504D49)
		{
			MODINSTRUMENT *pins = &Ins[nsmp+1];
			memcpy(pins->name, pis.filename, 12);
			pins->uFlags = 0;
			pins->nLength = 0;
			pins->nLoopStart = pis.loopbegin;
			pins->nLoopEnd = pis.loopend;
			pins->nSustainStart = pis.susloopbegin;
			pins->nSustainEnd = pis.susloopend;
			pins->nC4Speed = pis.C5Speed;
			if (!pins->nC4Speed) pins->nC4Speed = 8363;
			if (pis.C5Speed < 256) pins->nC4Speed = 256;
			pins->nVolume = pis.vol << 2;
			if (pins->nVolume > 256) pins->nVolume = 256;
			pins->nGlobalVol = pis.gvl;
			if (pins->nGlobalVol > 64) pins->nGlobalVol = 64;
			if (pis.flags & 0x10) pins->uFlags |= CHN_LOOP;
			if (pis.flags & 0x20) pins->uFlags |= CHN_SUSTAINLOOP;
			if (pis.flags & 0x40) pins->uFlags |= CHN_PINGPONGLOOP;
			if (pis.flags & 0x80) pins->uFlags |= CHN_PINGPONGSUSTAIN;
			pins->nPan = (pis.dfp & 0x7F) << 2;
			if (pins->nPan > 256) pins->nPan = 256;
			if (pis.dfp & 0x80) pins->uFlags |= CHN_PANNING;
			pins->nVibType = autovibit2xm[pis.vit & 7];
			pins->nVibRate = pis.vis;
			pins->nVibDepth = pis.vid & 0x7F;
			pins->nVibSweep = (pis.vir + 3) / 4;
			if ((pis.samplepointer) && (pis.samplepointer < dwMemLength) && (pis.length))
			{
				pins->nLength = pis.length;
				if (pins->nLength > MAX_SAMPLE_LENGTH) pins->nLength = MAX_SAMPLE_LENGTH;
				UINT flags = (pis.cvt & 1) ? RS_PCM8S : RS_PCM8U;
				if (pis.flags & 2)
				{
					flags += 5;
					if (pis.flags & 4) flags |= RSF_STEREO;
					pins->uFlags |= CHN_16BIT;
					// IT 2.14 16-bit packed sample ?
					if (pis.flags & 8) flags = ((pifh.cmwt >= 0x215) && (pis.cvt & 4)) ? RS_IT21516 : RS_IT21416;
				} else
				{
					if (pis.flags & 4) flags |= RSF_STEREO;
					if (pis.cvt == 0xFF) flags = RS_ADPCM4; else
					// IT 2.14 8-bit packed sample ?
					if (pis.flags & 8)	flags =	((pifh.cmwt >= 0x215) && (pis.cvt & 4)) ? RS_IT2158 : RS_IT2148;
				}
				ReadSample(&Ins[nsmp+1], flags, (LPSTR)(lpStream+pis.samplepointer), dwMemLength - pis.samplepointer);
			}
		}
		memcpy(m_szNames[nsmp+1], pis.name, 26);
	}
	// Reading Patterns
	for (UINT npat=0; npat<npatterns; npat++)
	{
		if ((!patpos[npat]) || ((DWORD)patpos[npat] + 4 >= dwMemLength))
		{
			PatternSize[npat] = 64;
			Patterns[npat] = AllocatePattern(64, m_nChannels);
			continue;
		}

		UINT len = bswapLE16(*((WORD *)(lpStream+patpos[npat])));
		UINT rows = bswapLE16(*((WORD *)(lpStream+patpos[npat]+2)));
		if ((rows < 4) || (rows > 256)) continue;
		if (patpos[npat]+8+len > dwMemLength) continue;
		PatternSize[npat] = rows;
		if ((Patterns[npat] = AllocatePattern(rows, m_nChannels)) == NULL) continue;
		memset(lastvalue, 0, sizeof(lastvalue));
		memset(chnmask, 0, sizeof(chnmask));
		MODCOMMAND *m = Patterns[npat];
		UINT i = 0;
		const BYTE *p = lpStream+patpos[npat]+8;
		UINT nrow = 0;
		while (nrow<rows)
		{
			if (i >= len) break;
			BYTE b = p[i++];
			if (!b)
			{
				nrow++;
				m+=m_nChannels;
				continue;
			}
			UINT ch = b & 0x7F;
			if (ch) ch = (ch - 1) & 0x3F;
			if (b & 0x80)
			{
				if (i >= len) break;
				chnmask[ch] = p[i++];
			}
			if ((chnmask[ch] & 0x10) && (ch < m_nChannels))
			{
				m[ch].note = lastvalue[ch].note;
			}
			if ((chnmask[ch] & 0x20) && (ch < m_nChannels))
			{
				m[ch].instr = lastvalue[ch].instr;
			}
			if ((chnmask[ch] & 0x40) && (ch < m_nChannels))
			{
				m[ch].volcmd = lastvalue[ch].volcmd;
				m[ch].vol = lastvalue[ch].vol;
			}
			if ((chnmask[ch] & 0x80) && (ch < m_nChannels))
			{
				m[ch].command = lastvalue[ch].command;
				m[ch].param = lastvalue[ch].param;
			}
			if (chnmask[ch] & 1)	// Note
			{
				if (i >= len) break;
				UINT note = p[i++];
				if (ch < m_nChannels)
				{
					if (note < 0x80) note++;
					m[ch].note = note;
					lastvalue[ch].note = note;
					channels_used[ch] = TRUE;
				}
			}
			if (chnmask[ch] & 2)
			{
				if (i >= len) break;
				UINT instr = p[i++];
				if (ch < m_nChannels)
				{
					m[ch].instr = instr;
					lastvalue[ch].instr = instr;
				}
			}
			if (chnmask[ch] & 4)
			{
				if (i >= len) break;
				UINT vol = p[i++];
				if (ch < m_nChannels)
				{
					// 0-64: Set Volume
					if (vol <= 64) { m[ch].volcmd = VOLCMD_VOLUME; m[ch].vol = vol; } else
					// 128-192: Set Panning
					if ((vol >= 128) && (vol <= 192)) { m[ch].volcmd = VOLCMD_PANNING; m[ch].vol = vol - 128; } else
					// 65-74: Fine Volume Up
					if (vol < 75) { m[ch].volcmd = VOLCMD_FINEVOLUP; m[ch].vol = vol - 65; } else
					// 75-84: Fine Volume Down
					if (vol < 85) { m[ch].volcmd = VOLCMD_FINEVOLDOWN; m[ch].vol = vol - 75; } else
					// 85-94: Volume Slide Up
					if (vol < 95) { m[ch].volcmd = VOLCMD_VOLSLIDEUP; m[ch].vol = vol - 85; } else
					// 95-104: Volume Slide Down
					if (vol < 105) { m[ch].volcmd = VOLCMD_VOLSLIDEDOWN; m[ch].vol = vol - 95; } else
					// 105-114: Pitch Slide Up
					if (vol < 115) { m[ch].volcmd = VOLCMD_PORTADOWN; m[ch].vol = vol - 105; } else
					// 115-124: Pitch Slide Down
					if (vol < 125) { m[ch].volcmd = VOLCMD_PORTAUP; m[ch].vol = vol - 115; } else
					// 193-202: Portamento To
					if ((vol >= 193) && (vol <= 202)) { m[ch].volcmd = VOLCMD_TONEPORTAMENTO; m[ch].vol = vol - 193; } else
					// 203-212: Vibrato
					if ((vol >= 203) && (vol <= 212)) { m[ch].volcmd = VOLCMD_VIBRATOSPEED; m[ch].vol = vol - 203; }
					lastvalue[ch].volcmd = m[ch].volcmd;
					lastvalue[ch].vol = m[ch].vol;
				}
			}
			// Reading command/param
			if (chnmask[ch] & 8)
			{
				if (i > len - 2) break;
				UINT cmd = p[i++];
				UINT param = p[i++];
				if (ch < m_nChannels)
				{
					if (cmd)
					{
						m[ch].command = cmd;
						m[ch].param = param;
						S3MConvert(&m[ch], TRUE);
						lastvalue[ch].command = m[ch].command;
						lastvalue[ch].param = m[ch].param;
					}
				}
			}
		}
	}
	for (UINT ncu=0; ncu<MAX_BASECHANNELS; ncu++)
	{
		if (ncu>=m_nChannels)
		{
			ChnSettings[ncu].nVolume = 64;
			ChnSettings[ncu].dwFlags &= ~CHN_MUTE;
		}
	}
	m_nMinPeriod = 8;
	m_nMaxPeriod = 0xF000;
	return TRUE;
}


#ifndef MODPLUG_NO_FILESAVE
//#define SAVEITTIMESTAMP
#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

BOOL CSoundFile::SaveIT(LPCSTR lpszFileName, UINT nPacking)
//---------------------------------------------------------
{
	DWORD dwPatNamLen, dwChnNamLen;
	ITFILEHEADER header, writeheader;
	ITINSTRUMENT iti, writeiti;
	ITSAMPLESTRUCT itss;
	BYTE smpcount[MAX_SAMPLES];
	DWORD inspos[MAX_INSTRUMENTS];
	DWORD patpos[MAX_PATTERNS];
	DWORD smppos[MAX_SAMPLES];
	DWORD dwPos = 0, dwHdrPos = 0, dwExtra = 2;
	WORD patinfo[4];
	BYTE chnmask[64];
	BYTE buf[512];
	MODCOMMAND lastvalue[64];
	FILE *f;


	if ((!lpszFileName) || ((f = fopen(lpszFileName, "wb")) == NULL)) return FALSE;
	memset(inspos, 0, sizeof(inspos));
	memset(patpos, 0, sizeof(patpos));
	memset(smppos, 0, sizeof(smppos));
	// Writing Header
	memset(&header, 0, sizeof(header));
	dwPatNamLen = 0;
	dwChnNamLen = 0;
	header.id = 0x4D504D49; // IMPM
	lstrcpyn((char *)header.songname, m_szNames[0], 27);
	header.reserved1 = 0x1004;
	header.ordnum = 0;
	while ((header.ordnum < MAX_ORDERS) && (Order[header.ordnum] < 0xFF)) header.ordnum++;
	if (header.ordnum < MAX_ORDERS) Order[header.ordnum++] = 0xFF;
	header.insnum = m_nInstruments;
	header.smpnum = m_nSamples;
	header.patnum = MAX_PATTERNS;
	while ((header.patnum > 0) && (!Patterns[header.patnum-1])) header.patnum--;
	header.cwtv = 0x217;
	header.cmwt = 0x200;
	header.flags = 0x0001;
	header.special = 0x0006;
	if (m_nInstruments) header.flags |= 0x04;
	if (m_dwSongFlags & SONG_LINEARSLIDES) header.flags |= 0x08;
	if (m_dwSongFlags & SONG_ITOLDEFFECTS) header.flags |= 0x10;
	if (m_dwSongFlags & SONG_ITCOMPATMODE) header.flags |= 0x20;
	if (m_dwSongFlags & SONG_EXFILTERRANGE) header.flags |= 0x1000;
	header.globalvol = m_nDefaultGlobalVolume >> 1;
	header.mv = m_nSongPreAmp;
	// clip song pre-amp values (between 0x20 and 0x7f)
	if (header.mv < 0x20) header.mv = 0x20;
	if (header.mv > 0x7F) header.mv = 0x7F;
	header.speed = m_nDefaultSpeed;
	header.tempo = m_nDefaultTempo;
	header.sep = m_nStereoSeparation;
	dwHdrPos = sizeof(header) + header.ordnum;
	// Channel Pan and Volume
	memset(header.chnpan, 0xFF, 64);
	memset(header.chnvol, 64, 64);
	for (UINT ich=0; ich<m_nChannels; ich++)
	{
		header.chnpan[ich] = ChnSettings[ich].nPan >> 2;
		if (ChnSettings[ich].dwFlags & CHN_SURROUND) header.chnpan[ich] = 100;
		header.chnvol[ich] = ChnSettings[ich].nVolume;
		if (ChnSettings[ich].dwFlags & CHN_MUTE) header.chnpan[ich] |= 0x80;
		if (ChnSettings[ich].szName[0])
		{
			dwChnNamLen = (ich+1) * MAX_CHANNELNAME;
		}
	}
	if (dwChnNamLen) dwExtra += dwChnNamLen + 8;
#ifdef SAVEITTIMESTAMP
	dwExtra += 8; // Time Stamp
#endif
	if (m_dwSongFlags & SONG_EMBEDMIDICFG)
	{
		header.flags |= 0x80;
		header.special |= 0x08;
		dwExtra += sizeof(MODMIDICFG);
	}
	// Pattern Names
	if ((m_nPatternNames) && (m_lpszPatternNames))
	{
		dwPatNamLen = m_nPatternNames * MAX_PATTERNNAME;
		while ((dwPatNamLen >= MAX_PATTERNNAME) && (!m_lpszPatternNames[dwPatNamLen-MAX_PATTERNNAME])) dwPatNamLen -= MAX_PATTERNNAME;
		if (dwPatNamLen < MAX_PATTERNNAME) dwPatNamLen = 0;
		if (dwPatNamLen) dwExtra += dwPatNamLen + 8;
	}
	// Mix Plugins
	dwExtra += SaveMixPlugins(NULL, TRUE);
	// Comments
	if (m_lpszSongComments)
	{
		header.special |= 1;
		header.msglength = strlen(m_lpszSongComments)+1;
		header.msgoffset = dwHdrPos + dwExtra + header.insnum*4 + header.patnum*4 + header.smpnum*4;
	}
	// Write file header
	memcpy(&writeheader, &header, sizeof(header));

	// Byteswap header information
	writeheader.id = bswapLE32(writeheader.id);
	writeheader.reserved1 = bswapLE16(writeheader.reserved1);
	writeheader.ordnum = bswapLE16(writeheader.ordnum);
	writeheader.insnum = bswapLE16(writeheader.insnum);
	writeheader.smpnum = bswapLE16(writeheader.smpnum);
	writeheader.patnum = bswapLE16(writeheader.patnum);
	writeheader.cwtv = bswapLE16(writeheader.cwtv);
	writeheader.cmwt = bswapLE16(writeheader.cmwt);
	writeheader.flags = bswapLE16(writeheader.flags);
	writeheader.special = bswapLE16(writeheader.special);
	writeheader.msglength = bswapLE16(writeheader.msglength);
	writeheader.msgoffset = bswapLE32(writeheader.msgoffset);
	writeheader.reserved2 = bswapLE32(writeheader.reserved2);

	fwrite(&writeheader, 1, sizeof(writeheader), f);

	fwrite(Order, 1, header.ordnum, f);
	if (header.insnum) fwrite(inspos, 4, header.insnum, f);
	if (header.smpnum) fwrite(smppos, 4, header.smpnum, f);
	if (header.patnum) fwrite(patpos, 4, header.patnum, f);
	// Writing editor history information
	{
#ifdef SAVEITTIMESTAMP
		SYSTEMTIME systime;
		FILETIME filetime;
		WORD timestamp[4];
		WORD nInfoEx = 1;
		memset(timestamp, 0, sizeof(timestamp));
		fwrite(&nInfoEx, 1, 2, f);
		GetSystemTime(&systime);
		SystemTimeToFileTime(&systime, &filetime);
		FileTimeToDosDateTime(&filetime, &timestamp[0], &timestamp[1]);
		fwrite(timestamp, 1, 8, f);
#else
		WORD nInfoEx = 0;
		fwrite(&nInfoEx, 1, 2, f);
#endif
	}
	// Writing midi cfg
	if (header.flags & 0x80)
	{
		fwrite(&m_MidiCfg, 1, sizeof(MODMIDICFG), f);
	}
	// Writing pattern names
	if (dwPatNamLen)
	{
		DWORD d = bswapLE32(0x4d414e50);
		UINT len= bswapLE32(dwPatNamLen);
		fwrite(&d, 1, 4, f);
		fwrite(&len, 1, 4, f);
		fwrite(m_lpszPatternNames, 1, dwPatNamLen, f);
	}
	// Writing channel Names
	if (dwChnNamLen)
	{
		DWORD d = bswapLE32(0x4d414e43);
		UINT len= bswapLE32(dwChnNamLen);
		fwrite(&d, 1, 4, f);
		fwrite(&len, 1, 4, f);
		UINT nChnNames = dwChnNamLen / MAX_CHANNELNAME;
		for (UINT inam=0; inam<nChnNames; inam++)
		{
			fwrite(ChnSettings[inam].szName, 1, MAX_CHANNELNAME, f);
		}
	}
	// Writing mix plugins info
	SaveMixPlugins(f, FALSE);
	// Writing song message
	dwPos = dwHdrPos + dwExtra + (header.insnum + header.smpnum + header.patnum) * 4;
	if (header.special & 1)
	{
		dwPos += strlen(m_lpszSongComments) + 1;
		fwrite(m_lpszSongComments, 1, strlen(m_lpszSongComments)+1, f);
	}
	// Writing instruments
	for (UINT nins=1; nins<=header.insnum; nins++)
	{
		memset(&iti, 0, sizeof(iti));
		iti.id = 0x49504D49;	// "IMPI"
		iti.trkvers = 0x211;
		if (Headers[nins])
		{
			INSTRUMENTHEADER *penv = Headers[nins];
			memset(smpcount, 0, sizeof(smpcount));
			memcpy(iti.filename, penv->filename, 12);
			memcpy(iti.name, penv->name, 26);
			iti.mbank = penv->wMidiBank;
			iti.mpr = penv->nMidiProgram;
			iti.mch = penv->nMidiChannel;
			iti.nna = penv->nNNA;
			iti.dct = penv->nDCT;
			iti.dca = penv->nDNA;
			iti.fadeout = penv->nFadeOut >> 5;
			iti.pps = penv->nPPS;
			iti.ppc = penv->nPPC;
			iti.gbv = (BYTE)(penv->nGlobalVol << 1);
			iti.dfp = (BYTE)penv->nPan >> 2;
			if (!(penv->dwFlags & ENV_SETPANNING)) iti.dfp |= 0x80;
			iti.rv = penv->nVolSwing;
			iti.rp = penv->nPanSwing;
			iti.ifc = penv->nIFC;
			iti.ifr = penv->nIFR;
			iti.nos = 0;
			for (UINT i=0; i<NOTE_MAX; i++) if (penv->Keyboard[i] < MAX_SAMPLES)
			{
				UINT smp = penv->Keyboard[i];
				if ((smp) && (!smpcount[smp]))
				{
					smpcount[smp] = 1;
					iti.nos++;
				}
				iti.keyboard[i*2] = penv->NoteMap[i] - 1;
				iti.keyboard[i*2+1] = smp;
			}
			// Writing Volume envelope
			if (penv->dwFlags & ENV_VOLUME) iti.volenv.flags |= 0x01;
			if (penv->dwFlags & ENV_VOLLOOP) iti.volenv.flags |= 0x02;
			if (penv->dwFlags & ENV_VOLSUSTAIN) iti.volenv.flags |= 0x04;
			if (penv->dwFlags & ENV_VOLCARRY) iti.volenv.flags |= 0x08;
			iti.volenv.num = (BYTE)penv->nVolEnv;
			iti.volenv.lpb = (BYTE)penv->nVolLoopStart;
			iti.volenv.lpe = (BYTE)penv->nVolLoopEnd;
			iti.volenv.slb = penv->nVolSustainBegin;
			iti.volenv.sle = penv->nVolSustainEnd;
			// Writing Panning envelope
			if (penv->dwFlags & ENV_PANNING) iti.panenv.flags |= 0x01;
			if (penv->dwFlags & ENV_PANLOOP) iti.panenv.flags |= 0x02;
			if (penv->dwFlags & ENV_PANSUSTAIN) iti.panenv.flags |= 0x04;
			if (penv->dwFlags & ENV_PANCARRY) iti.panenv.flags |= 0x08;
			iti.panenv.num = (BYTE)penv->nPanEnv;
			iti.panenv.lpb = (BYTE)penv->nPanLoopStart;
			iti.panenv.lpe = (BYTE)penv->nPanLoopEnd;
			iti.panenv.slb = penv->nPanSustainBegin;
			iti.panenv.sle = penv->nPanSustainEnd;
			// Writing Pitch Envelope
			if (penv->dwFlags & ENV_PITCH) iti.pitchenv.flags |= 0x01;
			if (penv->dwFlags & ENV_PITCHLOOP) iti.pitchenv.flags |= 0x02;
			if (penv->dwFlags & ENV_PITCHSUSTAIN) iti.pitchenv.flags |= 0x04;
			if (penv->dwFlags & ENV_PITCHCARRY) iti.pitchenv.flags |= 0x08;
			if (penv->dwFlags & ENV_FILTER) iti.pitchenv.flags |= 0x80;
			iti.pitchenv.num = (BYTE)penv->nPitchEnv;
			iti.pitchenv.lpb = (BYTE)penv->nPitchLoopStart;
			iti.pitchenv.lpe = (BYTE)penv->nPitchLoopEnd;
			iti.pitchenv.slb = (BYTE)penv->nPitchSustainBegin;
			iti.pitchenv.sle = (BYTE)penv->nPitchSustainEnd;
			// Writing Envelopes data
			for (UINT ev=0; ev<25; ev++)
			{
				iti.volenv.data[ev*3] = penv->VolEnv[ev];
				iti.volenv.data[ev*3+1] = penv->VolPoints[ev] & 0xFF;
				iti.volenv.data[ev*3+2] = penv->VolPoints[ev] >> 8;
				iti.panenv.data[ev*3] = penv->PanEnv[ev] - 32;
				iti.panenv.data[ev*3+1] = penv->PanPoints[ev] & 0xFF;
				iti.panenv.data[ev*3+2] = penv->PanPoints[ev] >> 8;
				iti.pitchenv.data[ev*3] = penv->PitchEnv[ev] - 32;
				iti.pitchenv.data[ev*3+1] = penv->PitchPoints[ev] & 0xFF;
				iti.pitchenv.data[ev*3+2] = penv->PitchPoints[ev] >> 8;
			}
		} else
		// Save Empty Instrument
		{
			for (UINT i=0; i<NOTE_MAX; i++) iti.keyboard[i*2] = i;
			iti.ppc = 5*12;
			iti.gbv = 128;
			iti.dfp = 0x20;
			iti.ifc = 0xFF;
		}
		if (!iti.nos) iti.trkvers = 0;
		// Writing instrument
		inspos[nins-1] = dwPos;
		dwPos += sizeof(ITINSTRUMENT);

		memcpy(&writeiti, &iti, sizeof(ITINSTRUMENT));

		writeiti.fadeout = bswapLE16(writeiti.fadeout);
		writeiti.id = bswapLE32(writeiti.id);
		writeiti.trkvers = bswapLE16(writeiti.trkvers);
		writeiti.mbank = bswapLE16(writeiti.mbank);

		fwrite(&writeiti, 1, sizeof(ITINSTRUMENT), f);
	}
	// Writing sample headers
	memset(&itss, 0, sizeof(itss));
	for (UINT hsmp=0; hsmp<header.smpnum; hsmp++)
	{
		smppos[hsmp] = dwPos;
		dwPos += sizeof(ITSAMPLESTRUCT);
		fwrite(&itss, 1, sizeof(ITSAMPLESTRUCT), f);
	}
	// Writing Patterns
	for (UINT npat=0; npat<header.patnum; npat++)
	{
		DWORD dwPatPos = dwPos;
		UINT len;
		if (!Patterns[npat]) continue;
		patpos[npat] = dwPos;
		patinfo[0] = 0;
		patinfo[1] = bswapLE16(PatternSize[npat]);
		patinfo[2] = 0;
		patinfo[3] = 0;
		// Check for empty pattern
		if (PatternSize[npat] == 64)
		{
			MODCOMMAND *pzc = Patterns[npat];
			UINT iz, nz = PatternSize[npat] * m_nChannels;
			for (iz=0; iz<nz; iz++)
			{
				if ((pzc[iz].note) || (pzc[iz].instr)
				 || (pzc[iz].volcmd) || (pzc[iz].command)) break;
			}
			if (iz == nz)
			{
				patpos[npat] = 0;
				continue;
			}
		}
		fwrite(patinfo, 8, 1, f);
		dwPos += 8;
		memset(chnmask, 0xFF, sizeof(chnmask));
		memset(lastvalue, 0, sizeof(lastvalue));
		MODCOMMAND *m = Patterns[npat];
		for (UINT row=0; row<PatternSize[npat]; row++)
		{
			len = 0;
			for (UINT ch=0; ch<m_nChannels; ch++, m++)
			{
				BYTE b = 0;
				UINT command = m->command;
				UINT param = m->param;
				UINT vol = 0xFF;
				UINT note = m->note;
				if (note) b |= 1;
				if ((note) && (note < 0x80)) note--; // 0xfe->0x80 --Toad
				if (m->instr) b |= 2;
				if (m->volcmd)
				{
					UINT volcmd = m->volcmd;
					switch(volcmd)
					{
					case VOLCMD_VOLUME:			vol = m->vol; if (vol > 64) vol = 64; break;
					case VOLCMD_PANNING:		vol = m->vol + 128; if (vol > 192) vol = 192; break;
					case VOLCMD_VOLSLIDEUP:		vol = 85 + ConvertVolParam(m->vol); break;
					case VOLCMD_VOLSLIDEDOWN:	vol = 95 + ConvertVolParam(m->vol); break;
					case VOLCMD_FINEVOLUP:		vol = 65 + ConvertVolParam(m->vol); break;
					case VOLCMD_FINEVOLDOWN:	vol = 75 + ConvertVolParam(m->vol); break;
					case VOLCMD_VIBRATOSPEED:	vol = 203 + ConvertVolParam(m->vol); break;
					case VOLCMD_VIBRATO:		vol = 203; break;
					case VOLCMD_TONEPORTAMENTO:	vol = 193 + ConvertVolParam(m->vol); break;
					case VOLCMD_PORTADOWN:		vol = 105 + ConvertVolParam(m->vol); break;
					case VOLCMD_PORTAUP:		vol = 115 + ConvertVolParam(m->vol); break;
					default:					vol = 0xFF;
					}
				}
				if (vol != 0xFF) b |= 4;
				if (command)
				{
					S3MSaveConvert(&command, &param, TRUE);
					if (command) b |= 8;
				}
				// Packing information
				if (b)
				{
					// Same note ?
					if (b & 1)
					{
						if ((note == lastvalue[ch].note) && (lastvalue[ch].volcmd & 1))
						{
							b &= ~1;
							b |= 0x10;
						} else
						{
							lastvalue[ch].note = note;
							lastvalue[ch].volcmd |= 1;
						}
					}
					// Same instrument ?
					if (b & 2)
					{
						if ((m->instr == lastvalue[ch].instr) && (lastvalue[ch].volcmd & 2))
						{
							b &= ~2;
							b |= 0x20;
						} else
						{
							lastvalue[ch].instr = m->instr;
							lastvalue[ch].volcmd |= 2;
						}
					}
					// Same volume column byte ?
					if (b & 4)
					{
						if ((vol == lastvalue[ch].vol) && (lastvalue[ch].volcmd & 4))
						{
							b &= ~4;
							b |= 0x40;
						} else
						{
							lastvalue[ch].vol = vol;
							lastvalue[ch].volcmd |= 4;
						}
					}
					// Same command / param ?
					if (b & 8)
					{
						if ((command == lastvalue[ch].command) && (param == lastvalue[ch].param) && (lastvalue[ch].volcmd & 8))
						{
							b &= ~8;
							b |= 0x80;
						} else
						{
							lastvalue[ch].command = command;
							lastvalue[ch].param = param;
							lastvalue[ch].volcmd |= 8;
						}
					}
					if (b != chnmask[ch])
					{
						chnmask[ch] = b;
						buf[len++] = (ch+1) | 0x80;
						buf[len++] = b;
					} else
					{
						buf[len++] = ch+1;
					}
					if (b & 1) buf[len++] = note;
					if (b & 2) buf[len++] = m->instr;
					if (b & 4) buf[len++] = vol;
					if (b & 8)
					{
						buf[len++] = command;
						buf[len++] = param;
					}
				}
			}
			buf[len++] = 0;
			dwPos += len;
			patinfo[0] += len;
			fwrite(buf, 1, len, f);
		}
		fseek(f, dwPatPos, SEEK_SET);
		patinfo[0] = bswapLE16(patinfo[0]); // byteswap -- Toad
		fwrite(patinfo, 8, 1, f);
		fseek(f, dwPos, SEEK_SET);
	}
	// Writing Sample Data
	for (UINT nsmp=1; nsmp<=header.smpnum; nsmp++)
	{
		MODINSTRUMENT *psmp = &Ins[nsmp];
		memset(&itss, 0, sizeof(itss));
		memcpy(itss.filename, psmp->name, 12);
		memcpy(itss.name, m_szNames[nsmp], 26);
		itss.id = 0x53504D49;
		itss.gvl = (BYTE)psmp->nGlobalVol;
		if (m_nInstruments)
		{
			for (UINT iu=1; iu<=m_nInstruments; iu++) if (Headers[iu])
			{
				INSTRUMENTHEADER *penv = Headers[iu];
				for (UINT ju=0; ju<128; ju++) if (penv->Keyboard[ju] == nsmp)
				{
					itss.flags = 0x01;
					break;
				}
			}
		} else
		{
			itss.flags = 0x01;
		}
		if (psmp->uFlags & CHN_LOOP) itss.flags |= 0x10;
		if (psmp->uFlags & CHN_SUSTAINLOOP) itss.flags |= 0x20;
		if (psmp->uFlags & CHN_PINGPONGLOOP) itss.flags |= 0x40;
		if (psmp->uFlags & CHN_PINGPONGSUSTAIN) itss.flags |= 0x80;
		itss.C5Speed = psmp->nC4Speed;
		if (!itss.C5Speed) // if no C5Speed assume it is XM Sample
		{ 
			UINT period;

			/**
			 * C5 note => number 61, but in XM samples:
			 * RealNote = Note + RelativeTone
			 */
			period = GetPeriodFromNote(61+psmp->RelativeTone, psmp->nFineTune, 0);
						
			if (period)
				itss.C5Speed = GetFreqFromPeriod(period, 0, 0);
			/**
			 * If it didn`t work, it may not be a XM file;
			 * so put the default C5Speed, 8363Hz.
			 */
	 		if (!itss.C5Speed) itss.C5Speed = 8363;
		}

		itss.length = psmp->nLength;
		itss.loopbegin = psmp->nLoopStart;
		itss.loopend = psmp->nLoopEnd;
		itss.susloopbegin = psmp->nSustainStart;
		itss.susloopend = psmp->nSustainEnd;
		itss.vol = psmp->nVolume >> 2;
		itss.dfp = psmp->nPan >> 2;
		itss.vit = autovibxm2it[psmp->nVibType & 7];
		itss.vis = psmp->nVibRate;
		itss.vid = psmp->nVibDepth;
		itss.vir = (psmp->nVibSweep < 64) ? psmp->nVibSweep * 4 : 255;
		if (psmp->uFlags & CHN_PANNING) itss.dfp |= 0x80;
		if ((psmp->pSample) && (psmp->nLength)) itss.cvt = 0x01;
		UINT flags = RS_PCM8S;
#ifndef NO_PACKING
		if (nPacking)
		{
			if ((!(psmp->uFlags & (CHN_16BIT|CHN_STEREO)))
			 && (CanPackSample((char *)psmp->pSample, psmp->nLength, nPacking)))
			{
				flags = RS_ADPCM4;
				itss.cvt = 0xFF;
			}
		} else
#endif // NO_PACKING
		{
			if (psmp->uFlags & CHN_STEREO)
			{
				flags = RS_STPCM8S;
				itss.flags |= 0x04;
			}
			if (psmp->uFlags & CHN_16BIT)
			{
				itss.flags |= 0x02;
				flags = (psmp->uFlags & CHN_STEREO) ? RS_STPCM16S : RS_PCM16S;
			}
		}
		itss.samplepointer = dwPos;
		fseek(f, smppos[nsmp-1], SEEK_SET);

		itss.id = bswapLE32(itss.id);
		itss.length = bswapLE32(itss.length);
		itss.loopbegin = bswapLE32(itss.loopbegin);
		itss.loopend = bswapLE32(itss.loopend);
		itss.C5Speed = bswapLE32(itss.C5Speed);
		itss.susloopbegin = bswapLE32(itss.susloopbegin);
		itss.susloopend = bswapLE32(itss.susloopend);
		itss.samplepointer = bswapLE32(itss.samplepointer);

		fwrite(&itss, 1, sizeof(ITSAMPLESTRUCT), f);
		fseek(f, dwPos, SEEK_SET);
		if ((psmp->pSample) && (psmp->nLength))
		{
			dwPos += WriteSample(f, psmp, flags);
		}
	}
	// Updating offsets
	fseek(f, dwHdrPos, SEEK_SET);
	
	/* <Toad> Now we can byteswap them ;-) */
	UINT WW;
	UINT WX;
	WX = (UINT)header.insnum;
	WX <<= 2;
	for (WW=0; WW < (WX>>2); WW++)
	       inspos[WW] = bswapLE32(inspos[WW]);

	WX = (UINT)header.smpnum;
	WX <<= 2;
	for (WW=0; WW < (WX>>2); WW++)
	       smppos[WW] = bswapLE32(smppos[WW]);

	WX=(UINT)header.patnum;
	WX <<= 2;
	for (WW=0; WW < (WX>>2); WW++)
	       patpos[WW] = bswapLE32(patpos[WW]);
	
	if (header.insnum) fwrite(inspos, 4, header.insnum, f);
	if (header.smpnum) fwrite(smppos, 4, header.smpnum, f);
	if (header.patnum) fwrite(patpos, 4, header.patnum, f);
	fclose(f);
	return TRUE;
}

#ifdef _MSC_VER
//#pragma warning(default:4100)
#endif
#endif // MODPLUG_NO_FILESAVE

//////////////////////////////////////////////////////////////////////////////
// IT 2.14 compression

DWORD ITReadBits(DWORD &bitbuf, UINT &bitnum, LPBYTE &ibuf, CHAR n)
//-----------------------------------------------------------------
{
	DWORD retval = 0;
	UINT i = n;

	if (n > 0)
	{
		do
		{
			if (!bitnum)
			{
				bitbuf = *ibuf++;
				bitnum = 8;
			}
			retval >>= 1;
			retval |= bitbuf << 31;
			bitbuf >>= 1;
			bitnum--;
			i--;
		} while (i);
		i = n;
	}
	return (retval >> (32-i));
}

#define IT215_SUPPORT
void ITUnpack8Bit(signed char *pSample, DWORD dwLen, LPBYTE lpMemFile, DWORD dwMemLength, BOOL b215)
//-------------------------------------------------------------------------------------------
{
	signed char *pDst = pSample;
	LPBYTE pSrc = lpMemFile;
	DWORD wHdr = 0;
	DWORD wCount = 0;
	DWORD bitbuf = 0;
	UINT bitnum = 0;
	BYTE bLeft = 0, bTemp = 0, bTemp2 = 0;

	while (dwLen)
	{
		if (!wCount)
		{
			wCount = 0x8000;
			wHdr = bswapLE16(*((LPWORD)pSrc));
			pSrc += 2;
			bLeft = 9;
			bTemp = bTemp2 = 0;
			bitbuf = bitnum = 0;
		}
		DWORD d = wCount;
		if (d > dwLen) d = dwLen;
		// Unpacking
		DWORD dwPos = 0;
		do
		{
			WORD wBits = (WORD)ITReadBits(bitbuf, bitnum, pSrc, bLeft);
			if (bLeft < 7)
			{
				DWORD i = 1 << (bLeft-1);
				DWORD j = wBits & 0xFFFF;
				if (i != j) goto UnpackByte;
				wBits = (WORD)(ITReadBits(bitbuf, bitnum, pSrc, 3) + 1) & 0xFF;
				bLeft = ((BYTE)wBits < bLeft) ? (BYTE)wBits : (BYTE)((wBits+1) & 0xFF);
				goto Next;
			}
			if (bLeft < 9)
			{
				WORD i = (0xFF >> (9 - bLeft)) + 4;
				WORD j = i - 8;
				if ((wBits <= j) || (wBits > i)) goto UnpackByte;
				wBits -= j;
				bLeft = ((BYTE)(wBits & 0xFF) < bLeft) ? (BYTE)(wBits & 0xFF) : (BYTE)((wBits+1) & 0xFF);
				goto Next;
			}
			if (bLeft >= 10) goto SkipByte;
			if (wBits >= 256)
			{
				bLeft = (BYTE)(wBits + 1) & 0xFF;
				goto Next;
			}
		UnpackByte:
			if (bLeft < 8)
			{
				BYTE shift = 8 - bLeft;
				signed char c = (signed char)(wBits << shift);
				c >>= shift;
				wBits = (WORD)c;
			}
			wBits += bTemp;
			bTemp = (BYTE)wBits;
			bTemp2 += bTemp;
#ifdef IT215_SUPPORT
			pDst[dwPos] = (b215) ? bTemp2 : bTemp;
#else
			pDst[dwPos] = bTemp;
#endif
		SkipByte:
			dwPos++;
		Next:
			if (pSrc >= lpMemFile+dwMemLength+1) return;
		} while (dwPos < d);
		// Move On
		wCount -= d;
		dwLen -= d;
		pDst += d;
	}
}


void ITUnpack16Bit(signed char *pSample, DWORD dwLen, LPBYTE lpMemFile, DWORD dwMemLength, BOOL b215)
//--------------------------------------------------------------------------------------------
{
	signed short *pDst = (signed short *)pSample;
	LPBYTE pSrc = lpMemFile;
	DWORD wHdr = 0;
	DWORD wCount = 0;
	DWORD bitbuf = 0;
	UINT bitnum = 0;
	BYTE bLeft = 0;
	signed short wTemp = 0, wTemp2 = 0;

	while (dwLen)
	{
		if (!wCount)
		{
			wCount = 0x4000;
			wHdr = bswapLE16(*((LPWORD)pSrc));
			pSrc += 2;
			bLeft = 17;
			wTemp = wTemp2 = 0;
			bitbuf = bitnum = 0;
		}
		DWORD d = wCount;
		if (d > dwLen) d = dwLen;
		// Unpacking
		DWORD dwPos = 0;
		do
		{
			DWORD dwBits = ITReadBits(bitbuf, bitnum, pSrc, bLeft);
			if (bLeft < 7)
			{
				DWORD i = 1 << (bLeft-1);
				DWORD j = dwBits;
				if (i != j) goto UnpackByte;
				dwBits = ITReadBits(bitbuf, bitnum, pSrc, 4) + 1;
				bLeft = ((BYTE)(dwBits & 0xFF) < bLeft) ? (BYTE)(dwBits & 0xFF) : (BYTE)((dwBits+1) & 0xFF);
				goto Next;
			}
			if (bLeft < 17)
			{
				DWORD i = (0xFFFF >> (17 - bLeft)) + 8;
				DWORD j = (i - 16) & 0xFFFF;
				if ((dwBits <= j) || (dwBits > (i & 0xFFFF))) goto UnpackByte;
				dwBits -= j;
				bLeft = ((BYTE)(dwBits & 0xFF) < bLeft) ? (BYTE)(dwBits & 0xFF) : (BYTE)((dwBits+1) & 0xFF);
				goto Next;
			}
			if (bLeft >= 18) goto SkipByte;
			if (dwBits >= 0x10000)
			{
				bLeft = (BYTE)(dwBits + 1) & 0xFF;
				goto Next;
			}
		UnpackByte:
			if (bLeft < 16)
			{
				BYTE shift = 16 - bLeft;
				signed short c = (signed short)(dwBits << shift);
				c >>= shift;
				dwBits = (DWORD)c;
			}
			dwBits += wTemp;
			wTemp = (signed short)dwBits;
			wTemp2 += wTemp;
#ifdef IT215_SUPPORT
			pDst[dwPos] = (b215) ? wTemp2 : wTemp;
#else
			pDst[dwPos] = wTemp;
#endif
		SkipByte:
			dwPos++;
		Next:
			if (pSrc >= lpMemFile+dwMemLength+1) return;
		} while (dwPos < d);
		// Move On
		wCount -= d;
		dwLen -= d;
		pDst += d;
		if (pSrc >= lpMemFile+dwMemLength) break;
	}
}


UINT CSoundFile::SaveMixPlugins(FILE *f, BOOL bUpdate)
//----------------------------------------------------
{
	DWORD chinfo[64];
	CHAR s[32];
	DWORD nPluginSize, writeSwapDWORD;
	SNDMIXPLUGININFO writePluginInfo;
	UINT nTotalSize = 0;
	UINT nChInfo = 0;

	for (UINT i=0; i<MAX_MIXPLUGINS; i++)
	{
		PSNDMIXPLUGIN p = &m_MixPlugins[i];
		if ((p->Info.dwPluginId1) || (p->Info.dwPluginId2))
		{
			nPluginSize = sizeof(SNDMIXPLUGININFO)+4; // plugininfo+4 (datalen)
			if ((p->pMixPlugin) && (bUpdate))
			{
				p->pMixPlugin->SaveAllParameters();
			}
			if (p->pPluginData)
			{
				nPluginSize += p->nPluginDataSize;
			}
			if (f)
			{
				s[0] = 'F';
				s[1] = 'X';
				s[2] = '0' + (i/10);
				s[3] = '0' + (i%10);
				fwrite(s, 1, 4, f);
				writeSwapDWORD = bswapLE32(nPluginSize);
				fwrite(&writeSwapDWORD, 1, 4, f);

				// Copy Information To Be Written for ByteSwapping
				memcpy(&writePluginInfo, &p->Info, sizeof(SNDMIXPLUGININFO));
				writePluginInfo.dwPluginId1 = bswapLE32(p->Info.dwPluginId1);
				writePluginInfo.dwPluginId2 = bswapLE32(p->Info.dwPluginId2);
				writePluginInfo.dwInputRouting = bswapLE32(p->Info.dwInputRouting);
				writePluginInfo.dwOutputRouting = bswapLE32(p->Info.dwOutputRouting);
				for (UINT j=0; j<4; j++)
				{
				        writePluginInfo.dwReserved[j] = bswapLE32(p->Info.dwReserved[j]);
				}

				fwrite(&writePluginInfo, 1, sizeof(SNDMIXPLUGININFO), f);
				writeSwapDWORD = bswapLE32(m_MixPlugins[i].nPluginDataSize);
				fwrite(&writeSwapDWORD, 1, 4, f);
				if (m_MixPlugins[i].pPluginData)
				{
					fwrite(m_MixPlugins[i].pPluginData, 1, m_MixPlugins[i].nPluginDataSize, f);
				}
			}
			nTotalSize += nPluginSize + 8;
		}
	}
	for (UINT j=0; j<m_nChannels; j++)
	{
		if (j < 64)
		{
			if ((chinfo[j] = ChnSettings[j].nMixPlugin) != 0)
			{
				nChInfo = j+1;
				chinfo[j] = bswapLE32(chinfo[j]); // inplace BS
			}
		}
	}
	if (nChInfo)
	{
		if (f)
		{
			nPluginSize = bswapLE32(0x58464843);
			fwrite(&nPluginSize, 1, 4, f);
			nPluginSize = nChInfo*4;
			writeSwapDWORD = bswapLE32(nPluginSize);
			fwrite(&writeSwapDWORD, 1, 4, f);
			fwrite(chinfo, 1, nPluginSize, f);
		}
		nTotalSize += nChInfo*4 + 8;
	}
	return nTotalSize;
}


UINT CSoundFile::LoadMixPlugins(const void *pData, UINT nLen)
//-----------------------------------------------------------
{
	const BYTE *p = (const BYTE *)pData;
	UINT nPos = 0;

	while (nPos+8 < nLen)
	{
		DWORD nPluginSize;
		UINT nPlugin;

		nPluginSize = bswapLE32(*(DWORD *)(p+nPos+4));
		if (nPluginSize > nLen-nPos-8) break;;
		if ((bswapLE32(*(DWORD *)(p+nPos))) == 0x58464843)
		{
			for (UINT ch=0; ch<64; ch++) if (ch*4 < nPluginSize)
			{
				ChnSettings[ch].nMixPlugin = bswapLE32(*(DWORD *)(p+nPos+8+ch*4));
			}
		} else
		{
			if ((p[nPos] != 'F') || (p[nPos+1] != 'X')
			 || (p[nPos+2] < '0') || (p[nPos+3] < '0'))
			{
				break;
			}
			nPlugin = (p[nPos+2]-'0')*10 + (p[nPos+3]-'0');
			if ((nPlugin < MAX_MIXPLUGINS) && (nPluginSize >= sizeof(SNDMIXPLUGININFO)+4))
			{
				DWORD dwExtra = bswapLE32(*(DWORD *)(p+nPos+8+sizeof(SNDMIXPLUGININFO)));
				m_MixPlugins[nPlugin].Info = *(const SNDMIXPLUGININFO *)(p+nPos+8);
				m_MixPlugins[nPlugin].Info.dwPluginId1 = bswapLE32(m_MixPlugins[nPlugin].Info.dwPluginId1);
				m_MixPlugins[nPlugin].Info.dwPluginId2 = bswapLE32(m_MixPlugins[nPlugin].Info.dwPluginId2);
				m_MixPlugins[nPlugin].Info.dwInputRouting = bswapLE32(m_MixPlugins[nPlugin].Info.dwInputRouting);
				m_MixPlugins[nPlugin].Info.dwOutputRouting = bswapLE32(m_MixPlugins[nPlugin].Info.dwOutputRouting);
				for (UINT j=0; j<4; j++)
				{
				        m_MixPlugins[nPlugin].Info.dwReserved[j] = bswapLE32(m_MixPlugins[nPlugin].Info.dwReserved[j]);
				}
				if ((dwExtra) && (dwExtra <= nPluginSize-sizeof(SNDMIXPLUGININFO)-4))
				{
					m_MixPlugins[nPlugin].nPluginDataSize = 0;
					m_MixPlugins[nPlugin].pPluginData = new signed char [dwExtra];
					if (m_MixPlugins[nPlugin].pPluginData)
					{
						m_MixPlugins[nPlugin].nPluginDataSize = dwExtra;
						memcpy(m_MixPlugins[nPlugin].pPluginData, p+nPos+8+sizeof(SNDMIXPLUGININFO)+4, dwExtra);
					}
				}
			}
		}
		nPos += nPluginSize + 8;
	}
	return nPos;
}

