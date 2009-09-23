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

  $Id: load_far.c,v 1.29 1999/10/25 16:31:41 miod Exp $

  Farandole (FAR) module loader

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "unimod_priv.h"

/*========== Module structure */

typedef struct FARHEADER1
  {
    UBYTE id[4];		/* file magic */
    CHAR songname[40];		/* songname */
    CHAR blah[3];		/* 13,10,26 */
    UWORD headerlen;		/* remaining length of header in bytes */
    UBYTE version;
    UBYTE onoff[16];
    UBYTE edit1[9];
    UBYTE speed;
    UBYTE panning[16];
    UBYTE edit2[4];
    UWORD stlen;
  }
FARHEADER1;

typedef struct FARHEADER2
  {
    UBYTE orders[256];
    UBYTE numpat;
    UBYTE snglen;
    UBYTE loopto;
    UWORD patsiz[256];
  }
FARHEADER2;

typedef struct FARSAMPLE
  {
    CHAR samplename[32];
    ULONG length;
    UBYTE finetune;
    UBYTE volume;
    ULONG reppos;
    ULONG repend;
    UBYTE type;
    UBYTE loop;
  }
FARSAMPLE;

typedef struct FARNOTE
  {
    UBYTE note, ins, vol, eff;
  }
FARNOTE;

/*========== Loader variables */

static CHAR FAR_Version[] = "Farandole";
static FARHEADER1 *mh1 = NULL;
static FARHEADER2 *mh2 = NULL;
static FARNOTE *pat = NULL;

static unsigned char FARSIG[4 + 3] =
{'F', 'A', 'R', 0xfe, 13, 10, 26};

/*========== Loader code */

BOOL 
FAR_Test (void)
{
  UBYTE id[47];

  if (!_mm_read_UBYTES (id, 47, modreader))
    return 0;
  if ((memcmp (id, FARSIG, 4)) || (memcmp (id + 44, FARSIG + 4, 3)))
    return 0;
  return 1;
}

BOOL 
FAR_Init (void)
{
  if (!(mh1 = (FARHEADER1 *) _mm_malloc (sizeof (FARHEADER1))))
    return 0;
  if (!(mh2 = (FARHEADER2 *) _mm_malloc (sizeof (FARHEADER2))))
    return 0;
  if (!(pat = (FARNOTE *) _mm_malloc (256 * 16 * 4 * sizeof (FARNOTE))))
    return 0;

  return 1;
}

void 
FAR_Cleanup (void)
{
  _mm_free (mh1);
  _mm_free (mh2);
  _mm_free (pat);
}

static UBYTE *
FAR_ConvertTrack (FARNOTE * n, int rows)
{
  int t, vibdepth = 1;

  UniReset ();
  for (t = 0; t < rows; t++)
    {
      if (n->note)
	{
	  UniInstrument (n->ins);
	  UniNote (n->note + 3 * OCTAVE - 1);
	}
      if (n->vol & 0xf)
	UniPTEffect (0xc, (n->vol & 0xf) << 2);
      if (n->eff)
	switch (n->eff >> 4)
	  {
	  case 0x3:		/* porta to note */
	    UniPTEffect (0x3, (n->eff & 0xf) << 4);
	    break;
	  case 0x5:		/* set vibrato depth */
	    vibdepth = n->eff & 0xf;
	    break;
	  case 0x6:		/* vibrato */
	    UniPTEffect (0x4, ((n->eff & 0xf) << 4) | vibdepth);
	    break;
	  case 0x7:		/* volume slide up */
	    UniPTEffect (0xa, (n->eff & 0xf) << 4);
	    break;
	  case 0x8:		/* volume slide down */
	    UniPTEffect (0xa, n->eff & 0xf);
	    break;
	  case 0xf:		/* set speed */
	    UniPTEffect (0xf, n->eff & 0xf);
	    break;

	    /* others not yet implemented */
	  default:
#ifdef MIKMOD_DEBUG
	    fprintf (stderr, "\rFAR: unsupported effect %02X\n", n->eff);
#endif
	    break;
	  }

      UniNewline ();
      n += 16;
    }
  return UniDup ();
}

BOOL 
FAR_Load (BOOL curious)
{
  int t, u, tracks = 0;
  SAMPLE *q;
  FARSAMPLE s;
  FARNOTE *crow;
  UBYTE smap[8];

  /* try to read module header (first part) */
  _mm_read_UBYTES (mh1->id, 4, modreader);
  _mm_read_SBYTES (mh1->songname, 40, modreader);
  _mm_read_SBYTES (mh1->blah, 3, modreader);
  mh1->headerlen = _mm_read_I_UWORD (modreader);
  mh1->version = _mm_read_UBYTE (modreader);
  _mm_read_UBYTES (mh1->onoff, 16, modreader);
  _mm_read_UBYTES (mh1->edit1, 9, modreader);
  mh1->speed = _mm_read_UBYTE (modreader);
  _mm_read_UBYTES (mh1->panning, 16, modreader);
  _mm_read_UBYTES (mh1->edit2, 4, modreader);
  mh1->stlen = _mm_read_I_UWORD (modreader);

  /* init modfile data */
  of.modtype = strdup (FAR_Version);
  of.songname = DupStr (mh1->songname, 40, 1);
  of.numchn = 16;
  of.initspeed = mh1->speed;
  of.inittempo = 80;
  of.reppos = 0;
  for (t = 0; t < 16; t++)
    of.panning[t] = mh1->panning[t] << 4;

  /* read songtext into comment field */
  if (mh1->stlen)
    if (!ReadComment (mh1->stlen))
      return 0;

  /* try to read module header (second part) */
  _mm_read_UBYTES (mh2->orders, 256, modreader);
  mh2->numpat = _mm_read_UBYTE (modreader);
  mh2->snglen = _mm_read_UBYTE (modreader);
  mh2->loopto = _mm_read_UBYTE (modreader);
  _mm_read_I_UWORDS (mh2->patsiz, 256, modreader);

  of.numpos = mh2->snglen;
  if (!AllocPositions (of.numpos))
    return 0;
  for (t = 0; t < of.numpos; t++)
    {
      if (mh2->orders[t] == 0xff)
	break;
      of.positions[t] = mh2->orders[t];
    }

  /* count number of patterns stored in file */
  of.numpat = 0;
  for (t = 0; t < 256; t++)
    if (mh2->patsiz[t])
      if ((t + 1) > of.numpat)
	of.numpat = t + 1;
  of.numtrk = of.numpat * of.numchn;

  /* seek across eventual new data */
  _mm_fseek (modreader, mh1->headerlen - (869 + mh1->stlen), SEEK_CUR);

  /* alloc track and pattern structures */
  if (!AllocTracks ())
    return 0;
  if (!AllocPatterns ())
    return 0;

  for (t = 0; t < of.numpat; t++)
    {
      UBYTE rows = 0, tempo;

      memset (pat, 0, 256 * 16 * 4 * sizeof (FARNOTE));
      if (mh2->patsiz[t])
	{
	  rows = _mm_read_UBYTE (modreader);
	  tempo = _mm_read_UBYTE (modreader);

	  crow = pat;
	  /* file often allocates 64 rows even if there are less in pattern */
	  if (mh2->patsiz[t] < 2 + (rows * 16 * 4))
	    {
	      _mm_errno = MMERR_LOADING_PATTERN;
	      return 0;
	    }
	  for (u = (mh2->patsiz[t] - 2) / 4; u; u--, crow++)
	    {
	      crow->note = _mm_read_UBYTE (modreader);
	      crow->ins = _mm_read_UBYTE (modreader);
	      crow->vol = _mm_read_UBYTE (modreader);
	      crow->eff = _mm_read_UBYTE (modreader);
	    }

	  if (_mm_eof (modreader))
	    {
	      _mm_errno = MMERR_LOADING_PATTERN;
	      return 0;
	    }

	  crow = pat;
	  of.pattrows[t] = rows;
	  for (u = 16; u; u--, crow++)
	    if (!(of.tracks[tracks++] = FAR_ConvertTrack (crow, rows)))
	      {
		_mm_errno = MMERR_LOADING_PATTERN;
		return 0;
	      }
	}
      else
	tracks += 16;
    }

  /* read sample map */
  if (!_mm_read_UBYTES (smap, 8, modreader))
    {
      _mm_errno = MMERR_LOADING_HEADER;
      return 0;
    }

  /* count number of samples used */
  of.numins = 0;
  for (t = 0; t < 64; t++)
    if (smap[t >> 3] & (1 << (t & 7)))
      of.numins = t + 1;
  of.numsmp = of.numins;

  /* alloc sample structs */
  if (!AllocSamples ())
    return 0;

  q = of.samples;
  for (t = 0; t < of.numsmp; t++)
    {
      q->speed = 8363;
      q->flags = SF_SIGNED;
      if (smap[t >> 3] & (1 << (t & 7)))
	{
	  _mm_read_SBYTES (s.samplename, 32, modreader);
	  s.length = _mm_read_I_ULONG (modreader);
	  s.finetune = _mm_read_UBYTE (modreader);
	  s.volume = _mm_read_UBYTE (modreader);
	  s.reppos = _mm_read_I_ULONG (modreader);
	  s.repend = _mm_read_I_ULONG (modreader);
	  s.type = _mm_read_UBYTE (modreader);
	  s.loop = _mm_read_UBYTE (modreader);

	  q->samplename = DupStr (s.samplename, 32, 1);
	  q->length = s.length;
	  q->loopstart = s.reppos;
	  q->loopend = s.repend;
	  q->volume = s.volume << 2;

	  if (s.type & 1)
	    q->flags |= SF_16BITS;
	  if (s.loop & 8)
	    q->flags |= SF_LOOP;

	  q->seekpos = _mm_ftell (modreader);
	  _mm_fseek (modreader, q->length, SEEK_CUR);
	}
      else
	q->samplename = DupStr (NULL, 0, 0);
      q++;
    }
  return 1;
}

CHAR *
FAR_LoadTitle (void)
{
  CHAR s[40];

  _mm_fseek (modreader, 4, SEEK_SET);
  if (!_mm_read_UBYTES (s, 40, modreader))
    return NULL;

  return (DupStr (s, 40, 1));
}

/*========== Loader information */

MLOADER load_far =
{
  NULL,
  "FAR",
  "FAR (Farandole Composer)",
  FAR_Init,
  FAR_Test,
  FAR_Load,
  FAR_Cleanup,
  FAR_LoadTitle
};

/* ex:set ts=4: */
