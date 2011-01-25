/*      MikMod sound library
   (c) 1998, 1999 Miodrag Vallat and others - see file AUTHORS for
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

  $Id: load_mod.c,v 1.28 1999/10/25 16:31:41 miod Exp $

  Generic MOD loader (Protracker, StarTracker, FastTracker, etc)

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "unimod_priv.h"

/*========== Module structure */

typedef struct MSAMPINFO
  {
    CHAR samplename[23];	/* 22 in module, 23 in memory */
    UWORD length;
    UBYTE finetune;
    UBYTE volume;
    UWORD reppos;
    UWORD replen;
  }
MSAMPINFO;

typedef struct MODULEHEADER
  {
    CHAR songname[21];		/* the songname.. 20 in module, 21 in memory */
    MSAMPINFO samples[31];	/* all sampleinfo */
    UBYTE songlength;		/* number of patterns used */
    UBYTE magic1;		/* should be 127 */
    UBYTE positions[128];	/* which pattern to play at pos */
    UBYTE magic2[4];		/* string "M.K." or "FLT4" or "FLT8" */
  }
MODULEHEADER;

typedef struct MODTYPE
  {
    CHAR id[5];
    UBYTE channels;
    CHAR *name;
  }
MODTYPE;

typedef struct MODNOTE
  {
    UBYTE a, b, c, d;
  }
MODNOTE;

/*========== Loader variables */

#define MODULEHEADERSIZE 1084

static CHAR protracker[] = "Protracker";
static CHAR startracker[] = "Startracker";
static CHAR fasttracker[] = "Fasttracker";
static CHAR ins15tracker[] = "15-instrument";
static CHAR oktalyzer[] = "Oktalyzer";
static CHAR taketracker[] = "TakeTracker";
static CHAR orpheus[] = "Imago Orpheus (MOD format)";

#define MODTYPE_COUNT 24
static MODTYPE modtypes[MODTYPE_COUNT + 1] =
{
  {"M.K.", 4, protracker},	/* protracker 4 channel */
  {"M!K!", 4, protracker},	/* protracker 4 channel */
  {"FLT4", 4, startracker},	/* startracker 4 channel */
  {"2CHN", 2, fasttracker},	/* fasttracker 2 channel */
  {"4CHN", 4, fasttracker},	/* fasttracker 4 channel */
  {"6CHN", 6, fasttracker},	/* fasttracker 6 channel */
  {"8CHN", 8, fasttracker},	/* fasttracker 8 channel */
  {"10CH", 10, fasttracker},	/* fasttracker 10 channel */
  {"12CH", 12, fasttracker},	/* fasttracker 12 channel */
  {"14CH", 14, fasttracker},	/* fasttracker 14 channel */
  {"15CH", 15, fasttracker},	/* fasttracker 15 channel */
  {"16CH", 16, fasttracker},	/* fasttracker 16 channel */
  {"18CH", 18, fasttracker},	/* fasttracker 18 channel */
  {"20CH", 20, fasttracker},	/* fasttracker 20 channel */
  {"22CH", 22, fasttracker},	/* fasttracker 22 channel */
  {"24CH", 24, fasttracker},	/* fasttracker 24 channel */
  {"26CH", 26, fasttracker},	/* fasttracker 26 channel */
  {"28CH", 28, fasttracker},	/* fasttracker 28 channel */
  {"30CH", 30, fasttracker},	/* fasttracker 30 channel */
  {"32CH", 32, fasttracker},	/* fasttracker 32 channel */
  {"CD81", 8, oktalyzer},	/* atari oktalyzer 8 channel */
  {"OKTA", 8, oktalyzer},	/* atari oktalyzer 8 channel */
  {"16CN", 16, taketracker},	/* taketracker 16 channel */
  {"32CN", 32, taketracker},	/* taketracker 32 channel */
  {"    ", 4, ins15tracker}	/* 15-instrument 4 channel */
};

static MODULEHEADER *mh = NULL;
static MODNOTE *patbuf = NULL;
static int modtype = 0;

/*========== Loader code */

static BOOL 
MOD_Test (void)
{
  UBYTE id[4];

  _mm_fseek (modreader, MODULEHEADERSIZE - 4, SEEK_SET);
  if (!_mm_read_UBYTES (id, 4, modreader))
    return 0;

  /* find out which ID string */
  for (modtype = 0; modtype < MODTYPE_COUNT; modtype++)
    if (!memcmp (id, modtypes[modtype].id, 4))
      return 1;

  return 0;
}

static BOOL 
MOD_Init (void)
{
  if (!(mh = (MODULEHEADER *) _mm_malloc (sizeof (MODULEHEADER))))
    return 0;
  return 1;
}

static void 
MOD_Cleanup (void)
{
  _mm_free (mh);
  _mm_free (patbuf);
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

static UWORD npertab[7 * OCTAVE] =
{
	/* -> Tuning 0 */
  1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016, 960, 906,
  856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
  428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
  214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
  107, 101, 95, 90, 85, 80, 75, 71, 67, 63, 60, 56,

  53, 50, 47, 45, 42, 40, 37, 35, 33, 31, 30, 28,
  27, 25, 24, 22, 21, 20, 19, 18, 17, 16, 15, 14
};


static void 
ConvertNote (MODNOTE * n)
{
  UBYTE instrument, effect, effdat, note;
  UWORD period;
  UBYTE lastnote = 0;

  /* extract the various information from the 4 bytes that make up a note */
  instrument = (n->a & 0x10) | (n->c >> 4);
  period = (((UWORD) n->a & 0xf) << 8) + n->b;
  effect = n->c & 0xf;
  effdat = n->d;

  /* Convert the period to a note number */
  note = 0;
  if (period)
    {
      for (note = 0; note < 7 * OCTAVE; note++)
	if (period >= npertab[note])
	  break;
      if (note == 7 * OCTAVE)
	note = 0;
      else
	note++;
    }

  if (instrument)
    {
      /* if instrument does not exist, note cut */
      if ((instrument > 31) || (!mh->samples[instrument - 1].length))
	{
	  UniPTEffect (0xc, 0);
	  if (effect == 0xc)
	    effect = effdat = 0;
	}
      else
	{
	  /* Protracker handling */
	  if (modtype <= 2)
	    {
	      /* if we had a note, then change instrument... */
	      if (note)
		UniInstrument (instrument - 1);
	      /* ...otherwise, only adjust volume... */
	      else
		{
		  /* ...unless an effect was specified, which forces a new
		     note to be played */
		  if (effect || effdat)
		    {
		      UniInstrument (instrument - 1);
		      note = lastnote;
		    }
		  else
		    UniPTEffect (0xc, mh->samples[instrument - 1].volume & 0x7f);
		}
	    }
	  else
	    {
	      /* Fasttracker handling */
	      UniInstrument (instrument - 1);
	      if (!note)
		note = lastnote;
	    }
	}
    }
  if (note)
    {
      UniNote (note + 2 * OCTAVE - 1);
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

  /* Ignore 100, 200 and 300 (there is no porta memory in mod files) */
#if 0		/* space_debris.mod uses 300 and porta memory!! */
  if ((!effdat) && ((effect == 1)||(effect == 2)||(effect ==3)))
    effect = 0;
#endif

  UniPTEffect (effect, effdat);
}

static UBYTE *
ConvertTrack (MODNOTE * n)
{
  int t;

  UniReset ();
  for (t = 0; t < 64; t++)
    {
      ConvertNote (n);
      UniNewline ();
      n += of.numchn;
    }
  return UniDup ();
}

/* Loads all patterns of a modfile and converts them into the 3 byte format. */
static BOOL 
ML_LoadPatterns (void)
{
  int t, s, tracks = 0;

  if (!AllocPatterns ())
    return 0;
  if (!AllocTracks ())
    return 0;

  /* Allocate temporary buffer for loading and converting the patterns */
  if (!(patbuf = (MODNOTE *) _mm_calloc (64U * of.numchn, sizeof (MODNOTE))))
    return 0;

  for (t = 0; t < of.numpat; t++)
    {
      /* Load the pattern into the temp buffer and convert it */
      for (s = 0; s < (64U * of.numchn); s++)
	{
	  patbuf[s].a = _mm_read_UBYTE (modreader);
	  patbuf[s].b = _mm_read_UBYTE (modreader);
	  patbuf[s].c = _mm_read_UBYTE (modreader);
	  patbuf[s].d = _mm_read_UBYTE (modreader);
	}
      for (s = 0; s < of.numchn; s++)
	if (!(of.tracks[tracks++] = ConvertTrack (patbuf + s)))
	  return 0;
    }
  return 1;
}

static BOOL 
MOD_Load (BOOL curious)
{
  int t, scan;
  SAMPLE *q;
  MSAMPINFO *s;
  BOOL is_orpheus = 0;

  /* try to read module header */
  _mm_read_string ((CHAR *) mh->songname, 20, modreader);
  mh->songname[20] = 0;		/* just in case */

  for (t = 0; t < 31; t++)
    {
      s = &mh->samples[t];
      _mm_read_string (s->samplename, 22, modreader);
      s->samplename[22] = 0;	/* just in case */
      s->length = _mm_read_M_UWORD (modreader);
      s->finetune = _mm_read_UBYTE (modreader);
      s->volume = _mm_read_UBYTE (modreader);
      s->reppos = _mm_read_M_UWORD (modreader);
      s->replen = _mm_read_M_UWORD (modreader);
    }

  mh->songlength = _mm_read_UBYTE (modreader);
  mh->magic1 = _mm_read_UBYTE (modreader);
  _mm_read_UBYTES (mh->positions, 128, modreader);
  _mm_read_UBYTES (mh->magic2, 4, modreader);

  if (_mm_eof (modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* set module variables */
  of.initspeed = 6;
  of.inittempo = 125;
  of.numchn = modtypes[modtype].channels;
  of.songname = DupStr (mh->songname, 21, 1);
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
    for (t = of.numpos; t < 128; t++)
      {
	if (mh->positions[t] > of.numpat)
	  of.numpat = mh->positions[t];
	if ((curious) && (mh->positions[t]))
	  of.numpos = t + 1;
      }
  of.numpat++;
  of.numtrk = of.numpat * of.numchn;

  if (!AllocPositions (of.numpos))
    return 0;
  for (t = 0; t < of.numpos; t++)
    of.positions[t] = mh->positions[t];

  /* Finally, init the sampleinfo structures  */
  of.numins = of.numsmp = 31;
  if (!AllocSamples ())
    return 0;
  s = mh->samples;
  q = of.samples;
  for (t = 0; t < of.numins; t++)
    {
      /* convert the samplename */
      q->samplename = DupStr (s->samplename, 23, 1);
      /* init the sampleinfo variables and convert the size pointers */
      q->speed = finetune[s->finetune & 0xf];
      q->volume = s->volume & 0x7f;
      q->loopstart = (ULONG) s->reppos << 1;
      q->loopend = q->loopstart + ((ULONG) s->replen << 1);
      q->length = (ULONG) s->length << 1;
      q->flags = SF_SIGNED;
      /* Imago Orpheus creates MODs with 16 bit samples, check */
      if ((modtypes[modtype].name == fasttracker) && (s->volume & 0x80))
	{
	  q->flags |= SF_16BITS;
	  is_orpheus = 1;
	}

      if (s->replen > 1)
	q->flags |= SF_LOOP;
      /* fix replen if repend > length */
      if (q->loopend > q->length)
	q->loopend = q->length;

      s++;
      q++;
    }

  if (is_orpheus)
    of.modtype = strdup (orpheus);
  else
    of.modtype = strdup (modtypes[modtype].name);

  if (!ML_LoadPatterns ())
    return 0;
  return 1;
}

static CHAR *
MOD_LoadTitle (void)
{
  CHAR s[21];

  _mm_fseek (modreader, 0, SEEK_SET);
  if (!_mm_read_UBYTES (s, 20, modreader))
    return NULL;
  s[20] = 0;			/* just in case */

  return (DupStr (s, 21, 1));
}

/*========== Loader information */

MLOADER load_mod =
{
  NULL,
  "Standard module",
  "MOD (31 instruments)",
  MOD_Init,
  MOD_Test,
  MOD_Load,
  MOD_Cleanup,
  MOD_LoadTitle
};

/* ex:set ts=4: */
