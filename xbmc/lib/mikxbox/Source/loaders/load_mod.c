/*	MikMod sound library
	(c) 1998, 1999, 2000, 2001, 2002 Miodrag Vallat and others - see file
	AUTHORS for complete list.

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

  $Id$

  Generic MOD loader (Protracker, StarTracker, FastTracker, etc)

==============================================================================*/
#include "xbsection_start.h"


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <ctype.h>
#include <stdio.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <string.h>
#include "mikmod.h"
#include "mikmod_internals.h"

#ifdef USE_MOD_FORMAT

#ifdef SUNOS
extern int fprintf(FILE *, const char *, ...);
#endif

/*========== Module structure */

typedef struct MSAMPINFO {
	CHAR samplename[23];		/* 22 in module, 23 in memory */
	UWORD length;
	UBYTE finetune;
	UBYTE volume;
	UWORD reppos;
	UWORD replen;
} MSAMPINFO;

typedef struct MODULEHEADER {
	CHAR songname[21];			/* the songname.. 20 in module, 21 in memory */
	MSAMPINFO samples[31];		/* all sampleinfo */
	UBYTE songlength;			/* number of patterns used */
	UBYTE magic1;				/* should be 127 */
	UBYTE positions[128];		/* which pattern to play at pos */
	UBYTE magic2[4];			/* string "M.K." or "FLT4" or "FLT8" */
} MODULEHEADER;

typedef struct MODTYPE {
	CHAR id[5];
	UBYTE channels;
	CHAR *name;
} MODTYPE;

typedef struct MODNOTE {
	UBYTE a, b, c, d;
} MODNOTE;

/*========== Loader variables */

#define MODULEHEADERSIZE 0x438

static CHAR protracker[] = "Protracker";
static CHAR startrekker[] = "Startrekker";
static CHAR fasttracker[] = "Fasttracker";
static CHAR oktalyser[] = "Oktalyser";
static CHAR oktalyzer[] = "Oktalyzer";
static CHAR taketracker[] = "TakeTracker";
static CHAR orpheus[] = "Imago Orpheus (MOD format)";

static MODULEHEADER *mh = NULL;
static MODNOTE *patbuf = NULL;
static int modtype, trekker;

/*========== Loader code */

/* given the module ID, determine the number of channels and the tracker
   description ; also alters modtype */
static BOOL MOD_CheckType(UBYTE *id, UBYTE *numchn, CHAR **descr)
{
	modtype = trekker = 0;

	/* Protracker and variants */
	if ((!memcmp(id, "M.K.", 4)) || (!memcmp(id, "M!K!", 4))) {
		*descr = protracker;
		modtype = 0;
		*numchn = 4;
		return 1;
	}
	
	/* Star Tracker */
	if (((!memcmp(id, "FLT", 3)) || (!memcmp(id, "EXO", 3))) &&
		(isdigit(id[3]))) {
		*descr = startrekker;
		modtype = trekker = 1;
		*numchn = id[3] - '0';
		if (*numchn == 4 || *numchn == 8)
			return 1;
#ifdef MIKMOD_DEBUG
		else
			fprintf(stderr, "\rUnknown FLT%d module type\n", *numchn);
#endif
		return 0;
	}

	/* Oktalyzer (Amiga) */
	if (!memcmp(id, "OKTA", 4)) {
		*descr = oktalyzer;
		modtype = 1;
		*numchn = 8;
		return 1;
	}

	/* Oktalyser (Atari) */
	if (!memcmp(id, "CD81", 4)) {
		*descr = oktalyser;
		modtype = 1;
		*numchn = 8;
		return 1;
	}

	/* Fasttracker */
	if ((!memcmp(id + 1, "CHN", 3)) && (isdigit(id[0]))) {
		*descr = fasttracker;
		modtype = 1;
		*numchn = id[0] - '0';
		return 1;
	}
	/* Fasttracker or Taketracker */
	if (((!memcmp(id + 2, "CH", 2)) || (!memcmp(id + 2, "CN", 2)))
		&& (isdigit(id[0])) && (isdigit(id[1]))) {
		if (id[3] == 'H') {
			*descr = fasttracker;
			modtype = 2;		/* this can also be Imago Orpheus */
		} else {
			*descr = taketracker;
			modtype = 1;
		}
		*numchn = (id[0] - '0') * 10 + (id[1] - '0');
		return 1;
	}

	return 0;
}

static BOOL MOD_Test(void)
{
	UBYTE id[4], numchn;
	CHAR *descr;

	_mm_fseek(modreader, MODULEHEADERSIZE, SEEK_SET);
	if (!_mm_read_UBYTES(id, 4, modreader))
		return 0;

	if (MOD_CheckType(id, &numchn, &descr))
		return 1;

	return 0;
}

static BOOL MOD_Init(void)
{
	if (!(mh = (MODULEHEADER *)_mm_malloc(sizeof(MODULEHEADER))))
		return 0;
	return 1;
}

static void MOD_Cleanup(void)
{
	_mm_free(mh);
	_mm_free(patbuf);
}

/*
Old (amiga) noteinfo:

_____byte 1_____   byte2_    _____byte 3_____   byte4_
/                \ /      \  /                \ /      \
0000          0000-00000000  0000          0000-00000000

Upper four    12 bits for    Lower four    Effect command.
bits of sam-  note period.   bits of sam-
ple number.                  ple number.

*/

static UBYTE ConvertNote(MODNOTE *n, UBYTE lasteffect)
{
	UBYTE instrument, effect, effdat, note;
	UWORD period;
	UBYTE lastnote = 0;

	/* extract the various information from the 4 bytes that make up a note */
	instrument = (n->a & 0x10) | (n->c >> 4);
	period = (((UWORD)n->a & 0xf) << 8) + n->b;
	effect = n->c & 0xf;
	effdat = n->d;

	/* Convert the period to a note number */
	note = 0;
	if (period) {
		for (note = 0; note < 7 * OCTAVE; note++)
			if (period >= npertab[note])
				break;
		if (note == 7 * OCTAVE)
			note = 0;
		else
			note++;
	}

	if (instrument) {
		/* if instrument does not exist, note cut */
		if ((instrument > 31) || (!mh->samples[instrument - 1].length)) {
			UniPTEffect(0xc, 0);
			if (effect == 0xc)
				effect = effdat = 0;
		} else {
			/* Protracker handling */
			if (!modtype) {
				/* if we had a note, then change instrument... */
				if (note)
					UniInstrument(instrument - 1);
				/* ...otherwise, only adjust volume... */
				else {
					/* ...unless an effect was specified, which forces a new
					   note to be played */
					if (effect || effdat) {
						UniInstrument(instrument - 1);
						note = lastnote;
					} else
						UniPTEffect(0xc,
									mh->samples[instrument -
												1].volume & 0x7f);
				}
			} else {
				/* Fasttracker handling */
				UniInstrument(instrument - 1);
				if (!note)
					note = lastnote;
			}
		}
	}
	if (note) {
		UniNote(note + 2 * OCTAVE - 1);
		lastnote = note;
	}

	/* Convert pattern jump from Dec to Hex */
	if (effect == 0xd)
		effdat = (((effdat & 0xf0) >> 4) * 10) + (effdat & 0xf);

	/* Volume slide, up has priority */
	if ((effect == 0xa) && (effdat & 0xf) && (effdat & 0xf0))
		effdat &= 0xf0;

	/* Handle ``heavy'' volumes correctly */
	if ((effect == 0xc) && (effdat > 0x40))
		effdat = 0x40;
	
	/* An isolated 100, 200 or 300 effect should be ignored (no
	   "standalone" porta memory in mod files). However, a sequence such
	   as 1XX, 100, 100, 100 is fine. */
	if ((!effdat) && ((effect == 1)||(effect == 2)||(effect ==3)) &&
		(lasteffect < 0x10) && (effect != lasteffect))
		effect = 0;

	UniPTEffect(effect, effdat);
	if (effect == 8)
		of.flags |= UF_PANNING;
	
	return effect;
}

static UBYTE *ConvertTrack(MODNOTE *n, int numchn)
{
	int t;
	UBYTE lasteffect = 0x10;	/* non existant effect */

	UniReset();
	for (t = 0; t < 64; t++) {
		lasteffect = ConvertNote(n,lasteffect);
		UniNewline();
		n += numchn;
	}
	return UniDup();
}

/* Loads all patterns of a modfile and converts them into the 3 byte format. */
static BOOL ML_LoadPatterns(void)
{
	int t, s, tracks = 0;

	if (!AllocPatterns())
		return 0;
	if (!AllocTracks())
		return 0;
	
	/* Allocate temporary buffer for loading and converting the patterns */
	if (!(patbuf = (MODNOTE *)_mm_calloc(64U * of.numchn, sizeof(MODNOTE))))
		return 0;

	if (trekker && of.numchn == 8) {
		/* Startrekker module dual pattern */
		for (t = 0; t < of.numpat; t++) {
			for (s = 0; s < (64U * 4); s++) {
				patbuf[s].a = _mm_read_UBYTE(modreader);
				patbuf[s].b = _mm_read_UBYTE(modreader);
				patbuf[s].c = _mm_read_UBYTE(modreader);
				patbuf[s].d = _mm_read_UBYTE(modreader);
			}
			for (s = 0; s < 4; s++)
				if (!(of.tracks[tracks++] = ConvertTrack(patbuf + s, 4)))
					return 0;
			for (s = 0; s < (64U * 4); s++) {
				patbuf[s].a = _mm_read_UBYTE(modreader);
				patbuf[s].b = _mm_read_UBYTE(modreader);
				patbuf[s].c = _mm_read_UBYTE(modreader);
				patbuf[s].d = _mm_read_UBYTE(modreader);
			}
			for (s = 0; s < 4; s++)
				if (!(of.tracks[tracks++] = ConvertTrack(patbuf + s, 4)))
					return 0;
		}
	} else {
		/* Generic module pattern */
		for (t = 0; t < of.numpat; t++) {
			/* Load the pattern into the temp buffer and convert it */
			for (s = 0; s < (64U * of.numchn); s++) {
				patbuf[s].a = _mm_read_UBYTE(modreader);
				patbuf[s].b = _mm_read_UBYTE(modreader);
				patbuf[s].c = _mm_read_UBYTE(modreader);
				patbuf[s].d = _mm_read_UBYTE(modreader);
			}
			for (s = 0; s < of.numchn; s++)
				if (!(of.tracks[tracks++] = ConvertTrack(patbuf + s, of.numchn)))
					return 0;
		}
	}
	return 1;
}

static BOOL MOD_Load(BOOL curious)
{
	int t, scan;
	SAMPLE *q;
	MSAMPINFO *s;
	CHAR *descr;

	/* try to read module header */
	_mm_read_string((CHAR *)mh->songname, 20, modreader);
	mh->songname[20] = 0;		/* just in case */

	for (t = 0; t < 31; t++) {
		s = &mh->samples[t];
		_mm_read_string(s->samplename, 22, modreader);
		s->samplename[22] = 0;	/* just in case */
		s->length = _mm_read_M_UWORD(modreader);
		s->finetune = _mm_read_UBYTE(modreader);
		s->volume = _mm_read_UBYTE(modreader);
		s->reppos = _mm_read_M_UWORD(modreader);
		s->replen = _mm_read_M_UWORD(modreader);
	}

	mh->songlength = _mm_read_UBYTE(modreader);
	mh->magic1 = _mm_read_UBYTE(modreader);
	_mm_read_UBYTES(mh->positions, 128, modreader);
	_mm_read_UBYTES(mh->magic2, 4, modreader);

	if (_mm_eof(modreader)) {
		_mm_errno = MMERR_LOADING_HEADER;
		return 0;
	}

	/* set module variables */
	of.initspeed = 6;
	of.inittempo = 125;
	if (!(MOD_CheckType(mh->magic2, &of.numchn, &descr))) {
		_mm_errno = MMERR_NOT_A_MODULE;
		return 0;
	}
	if (trekker && of.numchn == 8)
		for (t = 0; t < 128; t++)
			/* if module pretends to be FLT8, yet the order table
			   contains odd numbers, chances are it's a lying FLT4... */
			if (mh->positions[t] & 1) {
				of.numchn = 4;
				break;
			}
	if (trekker && of.numchn == 8)
		for (t = 0; t < 128; t++)
			mh->positions[t] >>= 1;

	of.songname = DupStr(mh->songname, 21, 1);
	of.numpos = mh->songlength;
	of.reppos = 0;

	/* Count the number of patterns */
	of.numpat = 0;
	for (t = 0; t < of.numpos; t++)
		if (mh->positions[t] > of.numpat)
			of.numpat = mh->positions[t];

	/* since some old modules embed extra patterns, we have to check the
	   whole list to get the samples' file offsets right - however we can find
	   garbage here, so check carefully */
	scan = 1;
	for (t = of.numpos; t < 128; t++)
		if (mh->positions[t] >= 0x80)
			scan = 0;
	if (scan)
		for (t = of.numpos; t < 128; t++) {
			if (mh->positions[t] > of.numpat)
				of.numpat = mh->positions[t];
			if ((curious) && (mh->positions[t]))
				of.numpos = t + 1;
		}
	of.numpat++;
	of.numtrk = of.numpat * of.numchn;

	if (!AllocPositions(of.numpos))
		return 0;
	for (t = 0; t < of.numpos; t++)
		of.positions[t] = mh->positions[t];

	/* Finally, init the sampleinfo structures  */
	of.numins = of.numsmp = 31;
	if (!AllocSamples())
		return 0;
	s = mh->samples;
	q = of.samples;
	for (t = 0; t < of.numins; t++) {
		/* convert the samplename */
		q->samplename = DupStr(s->samplename, 23, 1);
		/* init the sampleinfo variables and convert the size pointers */
		q->speed = finetune[s->finetune & 0xf];
		q->volume = s->volume & 0x7f;
		q->loopstart = (ULONG)s->reppos << 1;
		q->loopend = q->loopstart + ((ULONG)s->replen << 1);
		q->length = (ULONG)s->length << 1;
		q->flags = SF_SIGNED;
		/* Imago Orpheus creates MODs with 16 bit samples, check */
		if ((modtype == 2) && (s->volume & 0x80)) {
			q->flags |= SF_16BITS;
			descr = orpheus;
		}
		if (s->replen > 2)
			q->flags |= SF_LOOP;

		s++;
		q++;
	}

	of.modtype = strdup(descr);

	if (!ML_LoadPatterns())
		return 0;
	
	return 1;
}

static CHAR *MOD_LoadTitle(void)
{
	CHAR s[21];

	_mm_fseek(modreader, 0, SEEK_SET);
	if (!_mm_read_UBYTES(s, 20, modreader))
		return NULL;
	s[20] = 0;					/* just in case */

	return (DupStr(s, 21, 1));
}

/*========== Loader information */

MIKMODAPI MLOADER load_mod = {
	NULL,
	"Standard module",
	"MOD (31 instruments)",
	MOD_Init,
	MOD_Test,
	MOD_Load,
	MOD_Cleanup,
	MOD_LoadTitle
};

#endif //USE_MOD_FORMAT

/* ex:set ts=4: */
