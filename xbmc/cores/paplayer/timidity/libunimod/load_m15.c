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

  $Id: load_m15.c,v 1.29 1999/10/25 16:31:41 miod Exp $

  15 instrument MOD loader
  Also supports Ultimate Sound Tracker (old M15 format)

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "unimod_priv.h"

/*========== Module Structure */

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
    CHAR songname[21];		/* the songname.., 20 in module, 21 in memory */
    MSAMPINFO samples[15];	/* all sampleinfo */
    UBYTE songlength;		/* number of patterns used */
    UBYTE magic1;		/* should be 127 */
    UBYTE positions[128];	/* which pattern to play at pos */
  }
MODULEHEADER;

typedef struct MODNOTE
  {
    UBYTE a, b, c, d;
  }
MODNOTE;

/*========== Loader variables */

static MODULEHEADER *mh = NULL;
static MODNOTE *patbuf = NULL;
static BOOL ust_loader = 0;	/* if TRUE, load as an ust module. */

/* known file formats which can confuse the loader */
#define REJECT 2
static char *signatures[REJECT] =
{
  "CAKEWALK",			/* cakewalk midi files */
  "SZDD"			/* Microsoft compressed files */
};
static int siglen[REJECT] =
{8, 4};

/*========== Loader code */

static BOOL 
LoadModuleHeader (MODULEHEADER * mh)
{
  int t, u;

  _mm_read_string (mh->songname, 20, modreader);
  mh->songname[20] = 0;		/* just in case */

  /* sanity check : title should contain printable characters and a bunch
     of null chars */
  for (t = 0; t < 20; t++)
    if ((mh->songname[t]) && (mh->songname[t] < 32))
      return 0;
  for (t = 0; (mh->songname[t]) && (t < 20); t++);
  if (t < 20)
    for (; t < 20; t++)
      if (mh->songname[t])
	return 0;

  for (t = 0; t < 15; t++)
    {
      MSAMPINFO *s = &mh->samples[t];

      _mm_read_string (s->samplename, 22, modreader);
      s->samplename[22] = 0;	/* just in case */
      s->length = _mm_read_M_UWORD (modreader);
      s->finetune = _mm_read_UBYTE (modreader);
      s->volume = _mm_read_UBYTE (modreader);
      s->reppos = _mm_read_M_UWORD (modreader);
      s->replen = _mm_read_M_UWORD (modreader);

      /* sanity check : sample title should contain printable characters and
         a bunch of null chars */
      for (u = 0; u < 20; u++)
	if ((s->samplename[u]) && (s->samplename[u] < /*32 */ 14))
	  return 0;
      for (u = 0; (s->samplename[u]) && (u < 20); u++);
      if (u < 20)
	for (; u < 20; u++)
	  if (s->samplename[u])
	    return 0;

      /* sanity check : finetune values */
      if (s->finetune >> 4)
	return 0;
    }

  mh->songlength = _mm_read_UBYTE (modreader);
  mh->magic1 = _mm_read_UBYTE (modreader);	/* should be 127 */

  /* sanity check : no more than 128 positions, restart position in range */
  if ((!mh->songlength) || (mh->songlength > 128))
    return 0;
  /* values encountered so far are 0x6a and 0x78 */
  if (((mh->magic1 & 0xf8) != 0x78) && (mh->magic1 != 0x6a) && (mh->magic1 > mh->songlength))
    return 0;

  _mm_read_UBYTES (mh->positions, 128, modreader);

  /* sanity check : pattern range is 0..63 */
  for (t = 0; t < 128; t++)
    if (mh->positions[t] > 63)
      return 0;

  return (!_mm_eof (modreader));
}

/* Checks the patterns in the modfile for UST / 15-inst indications.
   For example, if an effect 3xx is found, it is assumed that the song 
   is 15-inst.  If a 1xx effect has dat greater than 0x20, it is UST.   

   Returns:  0 indecisive; 1 = UST; 2 = 15-inst                               */
static int 
CheckPatternType (int numpat)
{
  int t;
  UBYTE eff, dat;

  for (t = 0; t < numpat * (64U * 4); t++)
    {
      /* Load the pattern into the temp buffer and scan it */
      _mm_read_UBYTE (modreader);
      _mm_read_UBYTE (modreader);
      eff = _mm_read_UBYTE (modreader);
      dat = _mm_read_UBYTE (modreader);

      switch (eff)
	{
	case 1:
	  if (dat > 0x1f)
	    return 1;
	  if (dat < 0x3)
	    return 2;
	  break;
	case 2:
	  if (dat > 0x1f)
	    return 1;
	  return 2;
	case 3:
	  if (dat)
	    return 2;
	  break;
	default:
	  return 2;
	}
    }
  return 0;
}

static BOOL 
M15_Test (void)
{
  int t, numpat;
  MODULEHEADER mh;

  ust_loader = 0;
  if (!LoadModuleHeader (&mh))
    return 0;

  /* reject other file types */
  for (t = 0; t < REJECT; t++)
    if (!memcmp (mh.songname, signatures[t], siglen[t]))
      return 0;

  if (mh.magic1 > 127)
    return 0;
  if ((!mh.songlength) || (mh.songlength > mh.magic1))
    return 0;

  for (t = 0; t < 15; t++)
    {
      /* all finetunes should be zero */
      if (mh.samples[t].finetune)
	return 0;

      /* all volumes should be <= 64 */
      if (mh.samples[t].volume > 64)
	return 0;

      /* all instrument names should begin with s, st-, or a number */
      if (mh.samples[t].samplename[0] == 's')
	{
	  if ((memcmp (mh.samples[t].samplename, "st-", 3)) &&
	      (memcmp (mh.samples[t].samplename, "ST-", 3)) &&
	      (*mh.samples[t].samplename))
	    ust_loader = 1;
	}
      else if ((mh.samples[t].samplename[0] < '0') ||
	       (mh.samples[t].samplename[0] > '9'))
	ust_loader = 1;

      if (mh.samples[t].length > 4999 || mh.samples[t].reppos > 9999)
	{
	  ust_loader = 0;
	  if (mh.samples[t].length > 32768)
	    return 0;
	}

      /* if loop information is incorrect as words, but correct as bytes,
	 this is likely to be an ust-style module */
      if((mh.samples[t].reppos + mh.samples[t].replen > mh.samples[t].length) &&
	 (mh.samples[t].reppos + mh.samples[t].replen < (mh.samples[t].length << 1)))
	{
	  ust_loader = 1;
	  return 1;
	}
    }

  for (numpat = 0, t = 0; t < mh.songlength; t++)
    if (mh.positions[t] > numpat)
      numpat = mh.positions[t];
  numpat++;
  switch (CheckPatternType (numpat))
    {
    case 0:			/* indecisive, so check more clues... */
      break;
    case 1:
      ust_loader = 1;
      break;
    case 2:
      ust_loader = 0;
      break;
    }
  return 1;
}

static BOOL 
M15_Init (void)
{
  if (!(mh = (MODULEHEADER *) _mm_malloc (sizeof (MODULEHEADER))))
    return 0;
  return 1;
}

static void 
M15_Cleanup (void)
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
M15_ConvertNote (MODNOTE * n)
{
  UBYTE instrument, effect, effdat, note;
  UWORD period;
  UBYTE lastnote = 0;

  /* decode the 4 bytes that make up a single note */
  instrument = n->c >> 4;
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
      if ((instrument > 15) || (!mh->samples[instrument - 1].length))
	{
	  UniPTEffect (0xc, 0);
	  if (effect == 0xc)
	    effect = effdat = 0;
	}
      else
	{
	  /* if we had a note, then change instrument... */
	  if (note)
	    UniInstrument (instrument - 1);
	  /* ...otherwise, only adjust volume... */
	  else
	    {
	      /* ...unless an effect was specified, which forces a new note
	         to be played */
	      if (effect || effdat)
		{
		  UniInstrument (instrument - 1);
		  note = lastnote;
		}
	      else
		UniPTEffect (0xc, mh->samples[instrument - 1].volume & 0x7f);
	    }
	}
    }
  if (note)
    {
      UniNote (note + 2 * OCTAVE - 1);
      lastnote = note;
    }

  /* Handle ``heavy'' volumes correctly */
  if ((effect == 0xc) && (effdat > 0x40))
    effdat = 0x40;

  /* Convert pattern jump from Dec to Hex */
  if (effect == 0xd)
    effdat = (((effdat & 0xf0) >> 4) * 10) + (effdat & 0xf);

  /* Volume slide, up has priority */
  if ((effect == 0xa) && (effdat & 0xf) && (effdat & 0xf0))
    effdat &= 0xf0;

  if (ust_loader)
    {
      switch (effect)
	{
	case 0:
	case 3:
	  break;
	case 1:
	  UniPTEffect (0, effdat);
	  break;
	case 2:
	  if (effdat & 0xf)
	    UniPTEffect (1, effdat & 0xf);
	  if (effdat >> 2)
	    UniPTEffect (2, effdat >> 2);
	  break;
	default:
	  UniPTEffect (effect, effdat);
	  break;
	}
    }
  else {
    /* Ignore 100, 200 and 300 (there is no porta memory in mod files) */
    if ((!effdat) && ((effect == 1)||(effect == 2)||(effect == 3)))
      effect = 0;

    UniPTEffect (effect, effdat);
  }
}

static UBYTE *
M15_ConvertTrack (MODNOTE * n)
{
  int t;

  UniReset ();
  for (t = 0; t < 64; t++)
    {
      M15_ConvertNote (n);
      UniNewline ();
      n += 4;
    }
  return UniDup ();
}

/* Loads all patterns of a modfile and converts them into the 3 byte format. */
static BOOL 
M15_LoadPatterns (void)
{
  int t, s, tracks = 0;

  if (!AllocPatterns ())
    return 0;
  if (!AllocTracks ())
    return 0;

  /* Allocate temporary buffer for loading and converting the patterns */
  if (!(patbuf = (MODNOTE *) _mm_calloc (64U * 4, sizeof (MODNOTE))))
    return 0;

  for (t = 0; t < of.numpat; t++)
    {
      /* Load the pattern into the temp buffer and convert it */
      for (s = 0; s < (64U * 4); s++)
	{
	  patbuf[s].a = _mm_read_UBYTE (modreader);
	  patbuf[s].b = _mm_read_UBYTE (modreader);
	  patbuf[s].c = _mm_read_UBYTE (modreader);
	  patbuf[s].d = _mm_read_UBYTE (modreader);
	}

      for (s = 0; s < 4; s++)
	if (!(of.tracks[tracks++] = M15_ConvertTrack (patbuf + s)))
	  return 0;
    }
  return 1;
}

static BOOL 
M15_Load (BOOL curious)
{
  int t, scan;
  SAMPLE *q;
  MSAMPINFO *s;

  /* try to read module header */
  if (!LoadModuleHeader (mh))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  if (ust_loader)
    of.modtype = strdup ("Ultimate Soundtracker");
  else
    of.modtype = strdup ("Soundtracker");

  /* set module variables */
  of.initspeed = 6;
  of.inittempo = 125;
  of.numchn = 4;
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

  /* Finally, init the sampleinfo structures */
  of.numins = of.numsmp = 15;
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
      q->volume = s->volume;
      if (ust_loader)
	q->loopstart = s->reppos;
      else
	q->loopstart = s->reppos << 1;
      q->loopend = q->loopstart + (s->replen << 1);
      q->length = s->length << 1;

      q->flags = SF_SIGNED;
      if(ust_loader)
        q->flags |= SF_UST_LOOP;
      if(s->replen > 2)
        q->flags |= SF_LOOP;

      /* fix replen if repend>length */
      if (q->loopend > q->length)
	q->loopend = q->length;

      s++;
      q++;
    }

  if (!M15_LoadPatterns ())
    return 0;
  ust_loader = 0;

  return 1;
}

static CHAR *
M15_LoadTitle (void)
{
  CHAR s[21];

  _mm_fseek (modreader, 0, SEEK_SET);
  if (!_mm_read_UBYTES (s, 20, modreader))
    return NULL;
  s[20] = 0;			/* just in case */
  return (DupStr (s, 21, 1));
}

/*========== Loader information */

MLOADER load_m15 =
{
  NULL,
  "15-instrument module",
  "MOD (15 instruments)",
  M15_Init,
  M15_Test,
  M15_Load,
  M15_Cleanup,
  M15_LoadTitle
};

/* ex:set ts=4: */
