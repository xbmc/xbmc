/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>,
 *          Adam Goode       <adam@evdebs.org> (endian and char fixes for PPC)
*/

//////////////////////////////////////////////
// PTM PolyTracker module loader            //
//////////////////////////////////////////////
#include "stdafx.h"
#include "sndfile.h"

//#pragma warning(disable:4244)

#pragma pack(1)

typedef struct PTMFILEHEADER
{
	CHAR songname[28];		// name of song, asciiz string
	CHAR eof;				// 26
	BYTE version_lo;		// 03 version of file, currently 0203h
	BYTE version_hi;		// 02
	BYTE reserved1;			// reserved, set to 0
	WORD norders;			// number of orders (0..256)
	WORD nsamples;			// number of instruments (1..255)
	WORD npatterns;			// number of patterns (1..128)
	WORD nchannels;			// number of channels (voices) used (1..32)
	WORD fileflags;			// set to 0
	WORD reserved2;			// reserved, set to 0
	DWORD ptmf_id;			// song identification, 'PTMF' or 0x464d5450
	BYTE reserved3[16];		// reserved, set to 0
	BYTE chnpan[32];		// channel panning settings, 0..15, 0 = left, 7 = middle, 15 = right
	BYTE orders[256];		// order list, valid entries 0..nOrders-1
	WORD patseg[128];		// pattern offsets (*16)
} PTMFILEHEADER, *LPPTMFILEHEADER;

#define SIZEOF_PTMFILEHEADER	608


typedef struct PTMSAMPLE
{
	BYTE sampletype;		// sample type (bit array)
	CHAR filename[12];		// name of external sample file
	BYTE volume;			// default volume
	WORD nC4Spd;			// C4 speed
	WORD sampleseg;			// sample segment (used internally)
	WORD fileofs[2];		// offset of sample data
	WORD length[2];			// sample size (in bytes)
	WORD loopbeg[2];		// start of loop
	WORD loopend[2];		// end of loop
	WORD gusdata[8];
	char  samplename[28];	// name of sample, asciiz  // changed from CHAR
	DWORD ptms_id;			// sample identification, 'PTMS' or 0x534d5450
} PTMSAMPLE;

#define SIZEOF_PTMSAMPLE	80

#pragma pack()


BOOL CSoundFile::ReadPTM(const BYTE *lpStream, DWORD dwMemLength)
//---------------------------------------------------------------
{
	PTMFILEHEADER pfh = *(LPPTMFILEHEADER)lpStream;
	DWORD dwMemPos;
	UINT nOrders;

	pfh.norders = bswapLE16(pfh.norders);
	pfh.nsamples = bswapLE16(pfh.nsamples);
	pfh.npatterns = bswapLE16(pfh.npatterns);
	pfh.nchannels = bswapLE16(pfh.nchannels);
	pfh.fileflags = bswapLE16(pfh.fileflags);
	pfh.reserved2 = bswapLE16(pfh.reserved2);
	pfh.ptmf_id = bswapLE32(pfh.ptmf_id);
	for (UINT j=0; j<128; j++)
        {
	        pfh.patseg[j] = bswapLE16(pfh.patseg[j]);
	}

	if ((!lpStream) || (dwMemLength < 1024)) return FALSE;
	if ((pfh.ptmf_id != 0x464d5450) || (!pfh.nchannels)
	 || (pfh.nchannels > 32)
	 || (pfh.norders > 256) || (!pfh.norders)
	 || (!pfh.nsamples) || (pfh.nsamples > 255)
	 || (!pfh.npatterns) || (pfh.npatterns > 128)
	 || (SIZEOF_PTMFILEHEADER+pfh.nsamples*SIZEOF_PTMSAMPLE >= (int)dwMemLength)) return FALSE;
	memcpy(m_szNames[0], pfh.songname, 28);
	m_szNames[0][28] = 0;
	m_nType = MOD_TYPE_PTM;
	m_nChannels = pfh.nchannels;
	m_nSamples = (pfh.nsamples < MAX_SAMPLES) ? pfh.nsamples : MAX_SAMPLES-1;
	dwMemPos = SIZEOF_PTMFILEHEADER;
	nOrders = (pfh.norders < MAX_ORDERS) ? pfh.norders : MAX_ORDERS-1;
	memcpy(Order, pfh.orders, nOrders);
	for (UINT ipan=0; ipan<m_nChannels; ipan++)
	{
		ChnSettings[ipan].nVolume = 64;
		ChnSettings[ipan].nPan = ((pfh.chnpan[ipan] & 0x0F) << 4) + 4;
	}
	for (UINT ismp=0; ismp<m_nSamples; ismp++, dwMemPos += SIZEOF_PTMSAMPLE)
	{
		MODINSTRUMENT *pins = &Ins[ismp+1];
		PTMSAMPLE *psmp = (PTMSAMPLE *)(lpStream+dwMemPos);

		lstrcpyn(m_szNames[ismp+1], psmp->samplename, 28);
		memcpy(pins->name, psmp->filename, 12);
		pins->name[12] = 0;
		pins->nGlobalVol = 64;
		pins->nPan = 128;
		pins->nVolume = psmp->volume << 2;
		pins->nC4Speed = bswapLE16(psmp->nC4Spd) << 1;
		pins->uFlags = 0;
		if ((psmp->sampletype & 3) == 1)
		{
			UINT smpflg = RS_PCM8D;
			DWORD samplepos = (psmp->fileofs[1] << 16) +
				psmp->fileofs[0];
			pins->nLength = bswapLE32(*(LPDWORD)(psmp->length));
			pins->nLoopStart = bswapLE32(*(LPDWORD)(psmp->loopbeg));
			pins->nLoopEnd = bswapLE32(*(LPDWORD)(psmp->loopend));
			samplepos = bswapLE32(samplepos);
			if (psmp->sampletype & 4) pins->uFlags |= CHN_LOOP;
			if (psmp->sampletype & 8) pins->uFlags |= CHN_PINGPONGLOOP;
			if (psmp->sampletype & 16)
			{
				pins->uFlags |= CHN_16BIT;
				pins->nLength >>= 1;
				pins->nLoopStart >>= 1;
				pins->nLoopEnd >>= 1;
				smpflg = RS_PTM8DTO16;
			}
			if ((pins->nLength) && (samplepos) && (samplepos < dwMemLength))
			{
				ReadSample(pins, smpflg, (LPSTR)(lpStream+samplepos), dwMemLength-samplepos);
			}
		}
	}
	// Reading Patterns
	for (UINT ipat=0; ipat<pfh.npatterns; ipat++)
	{
		dwMemPos = ((UINT)pfh.patseg[ipat]) << 4;
		if ((!dwMemPos) || (dwMemPos >= dwMemLength)) continue;
		PatternSize[ipat] = 64;
		if ((Patterns[ipat] = AllocatePattern(64, m_nChannels)) == NULL) break;
		//
		MODCOMMAND *m = Patterns[ipat];
		for (UINT row=0; ((row < 64) && (dwMemPos < dwMemLength)); )
		{
			UINT b = lpStream[dwMemPos++];

			if (dwMemPos >= dwMemLength) break;
			if (b)
			{
				UINT nChn = b & 0x1F;

				if (b & 0x20)
				{
					if (dwMemPos + 2 > dwMemLength) break;
					m[nChn].note = lpStream[dwMemPos++];
					m[nChn].instr = lpStream[dwMemPos++];
				}
				if (b & 0x40)
				{
					if (dwMemPos + 2 > dwMemLength) break;
					m[nChn].command = lpStream[dwMemPos++];
					m[nChn].param = lpStream[dwMemPos++];
					if ((m[nChn].command == 0x0E) && ((m[nChn].param & 0xF0) == 0x80))
					{
						m[nChn].command = CMD_S3MCMDEX;
					} else
					if (m[nChn].command < 0x10)
					{
						ConvertModCommand(&m[nChn]);
					} else
					{
						switch(m[nChn].command)
						{
						case 16:
							m[nChn].command = CMD_GLOBALVOLUME;
							break;
						case 17:
							m[nChn].command = CMD_RETRIG;
							break;
						case 18:
							m[nChn].command = CMD_FINEVIBRATO;
							break;
						default:
							m[nChn].command = 0;
						}
					}
				}
				if (b & 0x80)
				{
					if (dwMemPos >= dwMemLength) break;
					m[nChn].volcmd = VOLCMD_VOLUME;
					m[nChn].vol = lpStream[dwMemPos++];
				}
			} else
			{
				row++;
				m += m_nChannels;
			}
		}
	}
	return TRUE;
}

