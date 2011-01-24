/*	MikMod sound library
	(c) 1999, 2000 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: load_okt.c,v 1.3 2000/02/14 22:10:22 miod Exp $

  Oktalyzer (OKT) module loader

==============================================================================*/

/*
	Written by UFO <ufo303@poczta.onet.pl>
	based on the file description compiled by Harald Zappe
	                                                      <zappe@gaea.sietec.de>

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>

#include "unimod_priv.h"

/*========== Module blocks */

/* sample information */
typedef struct OKTSAMPLE {
	CHAR sampname[20];
	ULONG len;
	UWORD loopbeg;
	UWORD looplen;
	UBYTE volume;
} OKTSAMPLE;

typedef struct OKTNOTE {
	UBYTE note, ins, eff, dat;
} OKTNOTE;

/*========== Loader variables */

static OKTNOTE *okttrk = NULL;

/*========== Loader code */

BOOL OKT_Test(void)
{
	CHAR id[8];

	if (!_mm_read_UBYTES(id, 8, modreader))
		return 0;
	if (!memcmp(id, "OKTASONG", 8))
		return 1;

	return 0;
}

/*	Pattern analysis routine.
	Effects not implemented (yet) : (in decimal)
	11 Arpeggio 4: Change note every 50Hz tick between N,H,N,L
	12 Arpeggio 5: Change note every 50Hz tick between H,H,N
                   N = normal note being played in this channel (1-36)
                   L = normal note number minus upper four bits of 'data'.
                   H = normal note number plus  lower four bits of 'data'.
    13 Decrease note number by 'data' once per tick.
    17 Increase note number by 'data' once per tick.
    21 Decrease note number by 'data' once per line.
    30 Increase note number by 'data' once per line.
*/
static UBYTE *OKT_ConvertTrack(UBYTE patrows)
{
	int t;
	UBYTE ins, note, eff, dat;

	UniReset();
	for (t = 0; t < patrows; t++) {
		note = okttrk[t].note;
		ins = okttrk[t].ins;
		eff = okttrk[t].eff;
		dat = okttrk[t].dat;

		if (note) {
			UniNote(note + 3*OCTAVE - 1);
			UniInstrument(ins);
		}

		if (eff)
			switch (eff) {
			  case 1:			/* Porta Up */
				UniPTEffect(0x1, dat);
				break;
			  case 2:			/* Portamento Down */
				UniPTEffect(0x2, dat);
				break;
			  case 10:			/* Arpeggio 3 supported */
				UniPTEffect(0x0, dat);
				break;
			  case 15:	/* Amiga filter toggle, ignored */
				break;
			  case 25:			/* Pattern Jump */
				UniPTEffect(0xb, dat);
				break;
			  case 27:			/* Release - similar to Keyoff */
				UniWriteByte(UNI_KEYOFF);
				break;
			  case 28:			/* Set Tempo */
				UniPTEffect(0xf, dat);
				break;
			  case 31:			/* volume Control */
				if (dat <= 0x40)
					UniPTEffect(0xc, dat);
				else if (dat <= 0x50)
					UniEffect(UNI_XMEFFECTA, (dat - 0x40));	/* fast fade out */
				else if (dat <= 0x60)
					UniEffect(UNI_XMEFFECTA, (dat - 0x50) << 4);	/* fast fade in */
				else if (dat <= 0x70)
					UniEffect(UNI_XMEFFECTEB, (dat - 0x60));	/* slow fade out */
				else if (dat <= 0x80)
					UniEffect(UNI_XMEFFECTEA, (dat - 0x70));	/* slow fade in */
				break;
#ifdef MIKMOD_DEBUG
			  default:
				fprintf(stderr, "\rUnimplemented effect (%02d,%02x)\n",
						eff, dat);
#endif
			}

		UniNewline();
	}
	return UniDup();
}

/* Read "channel modes" i.e. channel number and panning information */
static void OKT_doCMOD(void)
{
	/* amiga channel panning table */
	UBYTE amigapan[4] = { 0x00, 0xff, 0xff, 0x00 };
	int t;

	of.numchn = 0;

	for (t = 0; t < 4; t++)
		if (_mm_read_M_UWORD(modreader)) {
			/* two channels tied to the same Amiga hardware voice */
			of.panning[of.numchn++] = amigapan[t];
			of.panning[of.numchn++] = amigapan[t];
		} else
			/* one channel tied to the Amiga hardware voice */
			of.panning[of.numchn++] = amigapan[t];
}

/* Read sample information */
static BOOL OKT_doSAMP(int len)
{
	int t;
	SAMPLE *q;
	OKTSAMPLE s;

	of.numins = of.numsmp = (len / 0x20);
	if (!AllocSamples())
		return 0;

	for (t = 0, q = of.samples; t < of.numins; t++, q++) {
		_mm_read_UBYTES(s.sampname, 20, modreader);
		s.len = _mm_read_M_ULONG(modreader);
		s.loopbeg = _mm_read_M_UWORD(modreader);
		s.looplen = _mm_read_M_UWORD(modreader);
		_mm_read_UBYTE(modreader);
		s.volume = _mm_read_UBYTE(modreader);
		_mm_read_M_UWORD(modreader);

		if (_mm_eof(modreader)) {
			_mm_errno = MMERR_LOADING_SAMPLEINFO;
			return 0;
		}

		if (!s.len)
			q->seekpos = q->length = q->loopstart = q->loopend = q->flags = 0;
		else {
			s.len--;
			/* sanity checks */
			if (s.loopbeg > s.len)
				s.loopbeg = s.len;
			if (s.loopbeg + s.looplen > s.len)
				s.looplen = s.len - s.loopbeg;
			if (s.looplen < 2)
				s.looplen = 0;

			q->length = s.len;
			q->loopstart = s.loopbeg;
			q->loopend = s.looplen + q->loopstart;
			q->volume = s.volume;
			q->flags = SF_SIGNED;

			if (s.looplen)
				q->flags |= SF_LOOP;
		}
		q->samplename = DupStr(s.sampname, 20, 1);
		q->speed = 8363;
	}
	return 1;
}

/* Read speed information */
static void OKT_doSPEE(void)
{
	int tempo = _mm_read_M_UWORD(modreader);

	of.initspeed = tempo;
}

/* Read song length information */
static void OKT_doSLEN(void)
{
	of.numpat = _mm_read_M_UWORD(modreader);
}

/* Read pattern length information */
static void OKT_doPLEN(void)
{
	of.numpos = _mm_read_M_UWORD(modreader);
}

/* Read order table */
static BOOL OKT_doPATT(void)
{
	int t;

	if (!of.numpos || !AllocPositions(of.numpos))
		return 0;

	for (t = 0; t < 128; t++)
		if (t < of.numpos)
			of.positions[t] = (UWORD)_mm_read_UBYTE(modreader);
		else
			break;

	return 1;
}

static BOOL OKT_doPBOD(int patnum)
{
	char *patbuf;
	int rows, i;
	int u;

	if (!patnum) {
		of.numtrk = of.numpat * of.numchn;

		if (!AllocTracks() || !AllocPatterns())
			return 0;
	}

	/* Read pattern */
	of.pattrows[patnum] = rows = _mm_read_M_UWORD(modreader);

	if (!(okttrk = (OKTNOTE *) _mm_calloc(rows, sizeof(OKTNOTE))) ||
	    !(patbuf = (char *)_mm_calloc(rows * of.numchn, sizeof(OKTNOTE))))
		return 0;
	_mm_read_UBYTES(patbuf, rows * of.numchn * sizeof(OKTNOTE), modreader);
	if (_mm_eof(modreader)) {
		_mm_errno = MMERR_LOADING_PATTERN;
		return 0;
	}

	for (i = 0; i < of.numchn; i++) {
		for (u = 0; u < rows; u++) {
			okttrk[u].note = patbuf[(u * of.numchn + i) * sizeof(OKTNOTE)];
			okttrk[u].ins = patbuf[(u * of.numchn + i) * sizeof(OKTNOTE) + 1];
			okttrk[u].eff = patbuf[(u * of.numchn + i) * sizeof(OKTNOTE) + 2];
			okttrk[u].dat = patbuf[(u * of.numchn + i) * sizeof(OKTNOTE) + 3];
		}

		if (!(of.tracks[patnum * of.numchn + i] = OKT_ConvertTrack(rows)))
			return 0;
	}
	_mm_free(patbuf);
	_mm_free(okttrk);
	return 1;
}

static void OKT_doSBOD(int insnum)
{
	of.samples[insnum].seekpos = _mm_ftell(modreader);
}

BOOL OKT_Load(BOOL curious)
{
	UBYTE id[4];
	ULONG len;
	ULONG fp;
	BOOL seen_cmod = 0, seen_samp = 0, seen_slen = 0, seen_plen = 0, seen_patt
			= 0, seen_spee = 0;
	int patnum = 0, insnum = 0;

	/* skip OKTALYZER header */
	_mm_fseek(modreader, 8, SEEK_SET);
	of.songname = strdup("");

	of.modtype = strdup("Amiga Oktalyzer");
	of.numpos = of.reppos = 0;
	
	/* default values */
	of.initspeed = 6;
	of.inittempo = 125;
	
	while (1) {
		/* read block header */
		_mm_read_UBYTES(id, 4, modreader);
		len = _mm_read_M_ULONG(modreader);
		
		if (_mm_eof(modreader))
			break;
		fp = _mm_ftell(modreader);
		
		if (!memcmp(id, "CMOD", 4)) {
			if (!seen_cmod) {
				OKT_doCMOD();
				seen_cmod = 1;
			} else {
				_mm_errno = MMERR_LOADING_HEADER;
				return 0;
			}
		} else if (!memcmp(id, "SAMP", 4)) {
			if (!seen_samp && OKT_doSAMP(len))
				seen_samp = 1;
			else {
				_mm_errno = MMERR_LOADING_HEADER;
				return 0;
			}
		} else if (!memcmp(id, "SPEE", 4)) {
			if (!seen_spee) {
				OKT_doSPEE();
				seen_spee = 1;
			} else {
				_mm_errno = MMERR_LOADING_HEADER;
				return 0;
			}
		} else if (!memcmp(id, "SLEN", 4)) {
			if (!seen_slen) {
				OKT_doSLEN();
				seen_slen = 1;
			} else {
				_mm_errno = MMERR_LOADING_HEADER;
				return 0;
			}
		} else if (!memcmp(id, "PLEN", 4)) {
			if (!seen_plen) {
				OKT_doPLEN();
				seen_plen = 1;
			} else {
				_mm_errno = MMERR_LOADING_HEADER;
				return 0;
			}
		} else if (!memcmp(id, "PATT", 4)) {
			if (!seen_plen) {
				_mm_errno = MMERR_LOADING_HEADER;
				return 0;
			}
			if (!seen_patt && OKT_doPATT())
				seen_patt = 1;
			else {
				_mm_errno = MMERR_LOADING_HEADER;
				return 0;
			}
		} else if (!memcmp(id,"PBOD", 4)) {
			/* need to know numpat and numchn */
			if (!seen_slen || !seen_cmod || (patnum >= of.numpat)) {
				_mm_errno = MMERR_LOADING_HEADER;
				return 0;
			}
			if (!OKT_doPBOD(patnum++)) {
				_mm_errno = MMERR_LOADING_PATTERN;
				return 0;
			}
		} else if (!memcmp(id,"SBOD",4)) {
			/* need to know numsmp */
			if (!seen_samp) {
				_mm_errno = MMERR_LOADING_HEADER;
				return 0;
			}
			while ((insnum < of.numins) && !of.samples[insnum].length)
				insnum++;
			if (insnum >= of.numins) {
				_mm_errno = MMERR_LOADING_HEADER;
				return 0;
			}
			OKT_doSBOD(insnum++);
		}

		/* goto next block start position */
		_mm_fseek(modreader, fp + len, SEEK_SET);
	}

	if (!seen_cmod || !seen_samp || !seen_patt ||
		!seen_slen || !seen_plen || (patnum != of.numpat)) {
		_mm_errno = MMERR_LOADING_HEADER;
		return 0;
	}

	return 1;
}

CHAR *OKT_LoadTitle(void)
{
	return strdup("");
}

/*========== Loader information */

MLOADER load_okt = {
	NULL,
	"OKT",
	"OKT (Amiga Oktalyzer)",
	NULL,
	OKT_Test,
	OKT_Load,
	NULL,
	OKT_LoadTitle
};

/* ex:set ts=4: */
